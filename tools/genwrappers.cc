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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
	char tmp[classname.size()*2];
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
	return tmp;
}

static string makeMethodName(const string & suffix){
	char tmp[suffix.size()];
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
	return tmp;
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
		cout<<"Could not determine first argument of "<<name<<endl;
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

static void parseTypedef(Project *proj, xmlNode *node){
	XmlNode tdef(node);
	string typecontent=tdef.getChild("type").getText();
	string name=tdef.getChild("name").getText();
	if (typecontent.find("enum")==0){
		Type::addType(Type::Enum,name);
	}else if (typecontent.find("void(*")==0){
		// callbacks function, not really well parsed by doxygen
		Type::addType(Type::Callback,name);
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
	
	kind=member.getProp("kind");
	if (kind=="function"){
		parseFunction(proj,node);
	}else if (kind=="typedef"){
		parseTypedef(proj,node);
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
	doc = xmlReadFile(filename, NULL, 0);

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
	Project *proj=new Project();
	OutputGenerator *gen=NULL;
	list<string> files;
	list<string>::iterator it;
	
	LIBXML_TEST_VERSION
	
	for(i=1;i<argc;i++){
		if (strcmp(argv[i],"--help")==0){
			fprintf(stderr,"%s: [--help] --output (c++) file1 file2...\nParses xml files generated by doxygen to output wrappers in a specified language.\n",argv[0]);
			return -1;
		}if (strcmp(argv[i],"--output")==0){
			i++;
			if (strcmp(argv[i],"c++")==0){
				gen=new CplusplusGenerator();
			}else if (strcmp(argv[i],"javascript")==0){
				gen=new JavascriptGenerator();
			}
		}else{
			files.push_back(argv[i]);
		}
	}
	
	if (gen==NULL) {
		cerr<<"No output generator selected !"<<endl;
		return -1;
	}
	
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
