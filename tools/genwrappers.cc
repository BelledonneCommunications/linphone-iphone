/*
linphone
Copyright (C) 2013 Belledonne Communications SARL
Simon Morlat (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "software-desc.hh"
#include "generator.hh"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <sstream>



static bool isSpace(const char *str){
	for(;*str!='\0';++str){
		if (!isspace(*str)) return false;
	}
	return true;
}

//Convenient class for examining node recursively
class XmlNode{
public:
	XmlNode(const xmlNode *node=NULL) : mNode(node){
	}
	XmlNode getChild(const string &name)const{
		if (mNode==NULL) return XmlNode();
		xmlNode *it;
		for(it=mNode->children;it!=NULL;it=it->next){
			if (xmlStrcmp(it->name,(const xmlChar*)name.c_str())==0)
				return XmlNode(it);
		}
		return XmlNode();
	}
	XmlNode getChildRecursive(const string &name)const{
		if (mNode==NULL) return XmlNode();
		xmlNode *it;
		//find in direct children
		for(it=mNode->children;it!=NULL;it=it->next){
			if (xmlStrcmp(it->name,(const xmlChar*)name.c_str())==0)
				return XmlNode(it);
		}
		//recurse into children
		for(it=mNode->children;it!=NULL;it=it->next){
			XmlNode res=XmlNode(it).getChildRecursive(name);
			if (!res.isNull()) return res;
		}
		return XmlNode();
	}
	list<XmlNode> getChildren(const string &name)const{
		xmlNode *it;
		list<XmlNode> nodes;
		
		if (mNode==NULL) return nodes;
		for(it=mNode->children;it!=NULL;it=it->next){
			if (xmlStrcmp(it->name,(const xmlChar*)name.c_str())==0)
				nodes.push_back(XmlNode(it));
		}
		if (nodes.empty()) cerr<<"getChildren() no "<<name<<" found"<<endl;
		return nodes;
	}
	string getText()const{
		if (mNode==NULL) return "";
		XmlNode node=getChild("text");
		if (!node.isNull()) {
			const char *text=(const char*)node.mNode->content;
			if (!isSpace(text)) return string(text);
		}
		return "";
	}
	string getProp(const string &propname)const{
		if (mNode==NULL) return "";
		xmlChar *value;
		value=xmlGetProp((xmlNode*)mNode,(const xmlChar*)propname.c_str());
		if (value) return string((const char*)value);
		return "";
	}
	bool isNull()const{
		return mNode==NULL;
	}
private:
	const xmlNode *mNode;
};

static Argument *parseArgument(XmlNode node, bool isReturn){
	string name=node.getChild("declname").getText();
	Type *type=NULL;
	string typecontent=node.getChild("type").getText();
	bool isConst=false;
	bool isPointer=false;
	
	//find documented type if any
	string tname=node.getChild("type").getChild("ref").getText();
	if (!tname.empty()){
		type=Type::getType(tname);
	}else type=Type::getType(typecontent);
	
	//find const attribute if any
	if (typecontent.find("const")!=string::npos)
		isConst=true;
	
	if (typecontent.find("*")!=string::npos)
		isPointer=true;
	
	if (type==NULL) {
		return NULL;
	}
	//cout<<"Parsed argument "<<name<<" with type "<<type->getBasicType()<<" "<<type->getName()<<endl;
	return new Argument(type,!isReturn ? name : "",isConst,isPointer);
}

static string classNameToPrefix(const std::string &classname){
	char *tmp = new char[classname.size()*2];
	char *w=tmp;
	size_t i;
	
	for(i=0;i<classname.size();i++){
		char p=classname[i];
		if (isupper(p)){
			if (i!=0){
				*w++='_';
			}
			*w++=tolower(p);
		}else *w++=p;
	}
	*w++='\0';
	string ret(tmp);
	delete[] tmp;
	return ret;
}

static string makeMethodName(const string & suffix){
	char *tmp = new char[suffix.size()];
	char *w=tmp;
	size_t i;
	bool useUpper=false;
	
	for(i=0;i<suffix.size();i++){
		char p=suffix[i];
		
		if (p=='_'){
			if (i>0)
				useUpper=true;
		}else{
			if (useUpper)
				*w++=toupper(p);
			else 
				*w++=p;
			useUpper=false;
		}
	}
	*w++='\0';
	string ret(tmp);
	delete[] tmp;
	return ret;
}

static string extractMethodName(const string &c_name, const std::string& class_name){
	string prefix=classNameToPrefix(class_name);
	if (c_name.find(prefix)==0){
		return makeMethodName(c_name.substr(prefix.size(),string::npos));
	}
	return "";
}

static string getHelpBody(XmlNode myNode){
	ostringstream result;
	XmlNode brief=myNode.getChild("briefdescription");
	XmlNode detailed=myNode.getChild("detaileddescription");
	
	result<<brief.getText();
	result<<detailed.getChild("para").getText();
	//cout<<"getHelpBody():"<<result.str();
	return result.str();
}

static void parseFunction(Project *proj, xmlNode *node){
	string name;
	Argument *first_arg=NULL;
	string className;
	string methodName;
	Argument *retarg=NULL;
	list<Argument*> args;
	string help;
	XmlNode funcnode(node);
	XmlNode parameterlist;
	list<XmlNode> params;
	list<XmlNode> paramsHelp;
	list<XmlNode>::iterator it,helpit;
	
	name=funcnode.getChild("name").getText();
	params=funcnode.getChildren("param");
	parameterlist=funcnode.getChild("detaileddescription").getChildRecursive("parameterlist");
	if (parameterlist.isNull()) cerr<<"parameterlist not found"<<endl;
	paramsHelp=parameterlist.getChildren("parameteritem");
	
	for (it=params.begin(),helpit=paramsHelp.begin();it!=params.end();++it){
		Argument *a=parseArgument(*it,false);
		if (a){
			//add argument help
			if (!args.empty()){
				if (helpit!=paramsHelp.end()){
					XmlNode item=*helpit;
					a->setHelp(item.getChild("parameterdescription").getChild("para").getText());
				}else cerr<<"Undocumented parameter "<<a->getName()<<" in function "<<name<<endl;
			}
			args.push_back(a);
			if (helpit!=paramsHelp.end()) ++helpit;
		}
		else return;
	}
	help=getHelpBody(funcnode);

	retarg=parseArgument(funcnode,true);
	if (!retarg){
		cerr<<"Could not parse return argument of function "<<name<<endl;
		return;
	}
	
	if (!args.empty()) first_arg=args.front();
	if (!first_arg){
		cerr<<"Could not determine first argument of "<<name<<endl;
		return;
	}
	if (first_arg->getType()->getBasicType()!=Type::Class) return;
	className=first_arg->getType()->getName();
	methodName=extractMethodName(name,className);
	if (!methodName.empty() && methodName!="destroy"){
		//cout<<"Found "<<className<<"."<<methodName<<"()"<<endl;
		args.pop_front();
		Method *method=new Method("",retarg,methodName,args,first_arg->isConst(),false);
		method->setHelp(help);
		proj->getClass(className)->addMethod(method);
		delete first_arg;
	}
}

static string findCommon(const string &c1, const string & c2){
	size_t i;
	ostringstream res;
	for(i=0;i<c1.size() && i<c2.size();++i){
		if (tolower(c1[i])==tolower(c2[i]))
			res<<(char)c1[i];
		else break;
	}
	return res.str();
}

static string extractCallbackName(const string &c_name, const string & classname){
	string prefix=findCommon(c_name,classname);
	string res=c_name.substr(prefix.size(),string::npos);
	res[0]=tolower(res[0]);
	size_t pos=res.find("Cb");
	if (pos!=string::npos) res=res.substr(0,pos);
	return res;
}


static void parseCallback(Project *proj, XmlNode node){
	string argsstring=node.getChild("argsstring").getText();
	string name=node.getChild("name").getText();
	list<XmlNode> params=node.getChildRecursive("parameterlist").getChildren("parameteritem");
	list<XmlNode>::iterator it=params.begin();
	string rettype=node.getChild("type").getText();
	argsstring=argsstring.substr(argsstring.find('(')+1,string::npos);
	bool cont=true;
	list<Argument*> args;
	Type *firstArgType=NULL;
	
	rettype=rettype.substr(0,rettype.find('('));
	Argument *retarg=new Argument(Type::getType(rettype),"",false,rettype.find('*')!=string::npos);
	
	do{
		size_t comma=argsstring.find(',');
		size_t end=argsstring.find(')');
		if (comma!=string::npos && comma<end) end=comma;
		else cont=false;
		string arg=argsstring.substr(0,end);
		bool isConst=false;
		bool isPointer=false;
		
		size_t endtype=arg.find('*');
		if (endtype==string::npos) endtype=arg.rfind(' ');
		else isPointer=true;
		string typestring=arg.substr(0,endtype+1);
		Type *type=Type::getType(typestring);
		
		if (type==NULL) return;
		
		if (firstArgType==NULL) firstArgType=type;
		
		//find const attribute if any
		if (typestring.find("const")!=string::npos)
			isConst=true;
		
		string argname=arg.substr(endtype+1,end);
		argsstring=argsstring.substr(end+1,string::npos);
		Argument *argobj=new Argument(type,makeMethodName(argname),isConst,isPointer);
		if (it!=params.end()){
			argobj->setHelp((*it).getChild("parameterdescription").getChild("para").getText());
			++it;
		}
		args.push_back(argobj);
	}while(cont);
	
	if (firstArgType->getBasicType()!=Type::Class) return;
	Class *klass=proj->getClass(firstArgType->getName());
	Method *callback=new Method("", retarg, extractCallbackName(name,klass->getName()), args, false, false, true);
	//cout<<"Found callback "<<callback->getName()<<" with "<<args.size()<<" arguments."<<endl;
	callback->setHelp(node.getChild("detaileddescription").getChild("para").getText());
	klass->addMethod(callback);
	
	
}

static void parseEnum(Project *proj, XmlNode node){
	string name=node.getChild("name").getText();
	if (name[0]=='_') name.erase(0,1);
	Class *klass=proj->getClass(name);
	klass->setHelp(node.getChild("detaileddescription").getChild("para").getText());
	list<XmlNode> enumValues=node.getChildren("enumvalue");
	list<XmlNode>::iterator it;
	int value = 0;
	for (it=enumValues.begin();it!=enumValues.end();++it){
		string initializer = (*it).getChild("initializer").getText();
		if ((initializer.length() > 1) && (initializer.at(0) == '=')) {
			std::stringstream ss;
			if ((initializer.length() > 2) && (initializer.at(1) == '0')) {
				if ((initializer.length() > 3) && (initializer.at(2) == 'x')) {
					ss << std::hex << initializer.substr(3);
				} else {
					ss << std::oct << initializer.substr(2);
				}
			} else {
				ss << std::dec << initializer.substr(1);
			}
			ss >> value;
		}
		ConstField *cf=new ConstField(Type::getType("int"),(*it).getChild("name").getText(),value);
		cf->setHelp((*it).getChild("detaileddescription").getChild("para").getText());
		klass->addConstField(cf);
		value++;
	}
	
}

static void parseTypedef(Project *proj, xmlNode *node){
	XmlNode tdef(node);
	string typecontent=tdef.getChild("type").getText();
	string name=tdef.getChild("name").getText();
	if (typecontent.find("enum")==0){
		Type::addType(Type::Enum,name);
	}else if (typecontent.find("(*")!=string::npos){
		parseCallback(proj,node);
	}else
		proj->getClass(name)->setHelp(getHelpBody(node));
}

static void parseMemberDef(Project *proj, xmlNode *node){
	XmlNode member(node);
	string brief;
	string detailed;
	string kind;
	
	if (member.getChild("briefdescription").getText().empty() && 
		member.getChild("detaileddescription").getChild("para").getText().empty())
		return;
	if (member.getProp("id").find("group__")!=0)
		return;
	if (member.getChild("detaileddescription").getChildRecursive("xreftitle").getText()=="Deprecated")
		return;
	
	kind=member.getProp("kind");
	if (kind=="function"){
		parseFunction(proj,node);
	}else if (kind=="typedef"){
		parseTypedef(proj,node);
	}else if (kind=="enum"){
		parseEnum(proj,node);
	}
}

static void inspectNode(Project *proj, xmlNode *a_node){
	xmlNode *cur_node;
	
	for (cur_node = a_node; cur_node != NULL ; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			//printf("node type: Element, name: %s\n", cur_node->name);
			if (strcmp((const char*)cur_node->name,"memberdef")==0 ){
				//cout<<"Found memberdef"<<endl;
				parseMemberDef(proj,cur_node);
			}
		}
		if (cur_node->children) inspectNode(proj,cur_node->children);
	}
}

static int parse_file(Project *proj, const char *filename){
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;


	/*parse the file and get the DOM */
	doc = xmlReadFile(filename, NULL, XML_PARSE_RECOVER);

	if (doc == NULL) {
		cerr<<"xmlReadFile failed."<<endl;
		return -1;
	}

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);

	inspectNode(proj,root_element);

	/*free the document */
	xmlFreeDoc(doc);
	return 0;
}

int main(int argc, char *argv[]){
	int i;
	string projectName="wrapper";
	OutputGenerator *gen=NULL;
	list<string> files;
	list<string>::iterator it;
	
	LIBXML_TEST_VERSION
	
	for(i=1;i<argc;i++){
		if (strcmp(argv[i],"--help")==0){
			fprintf(stderr,"%s: [--help] --output (c++, javascript) --project <project name> file1 file2...\nParses xml files generated by doxygen to output wrappers in a specified language.\n",argv[0]);
			return -1;
		}else if (strcmp(argv[i],"--output")==0){
			i++;
			if (strcmp(argv[i],"c++")==0){
				gen=new CplusplusGenerator();
			}else if (strcmp(argv[i],"javascript")==0){
				gen=new JavascriptGenerator();
			}
		}else if (strcmp(argv[i],"--project")==0){
			i++;
			projectName=argv[i];
		}else{
			files.push_back(argv[i]);
		}
	}
	
	if (gen==NULL) {
		cerr<<"No output generator selected !"<<endl;
		return -1;
	}
	Project *proj=new Project(projectName);
	for(it=files.begin();it!=files.end();it++){
		if (parse_file(proj,(*it).c_str())==-1){
			cerr<<"Parsing aborted."<<endl;
			return -1;
		}
	}
	proj->analyse();
	gen->generate(proj);
	return 0;
}
