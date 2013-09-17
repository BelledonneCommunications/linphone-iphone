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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <list>
#include <map>


using namespace::std;


class Type{
public:
	enum BasicType{
		Integer,
		String,
		Klass
	};
	static Type *getType(const std::string &tname){
		if (strstr(tname.c_str(),"char")!=0 && strchr(tname.c_str(),'*')!=0){
			return &sStringType;
		}else if (strcmp(tname.c_str(),"int")==0){
			return &sIntegerType;
		}else{
			Type* ret;
			string tmp=tname;
			ssize_t pos;
			
			/*really ugly and slow*/
			
			pos=tmp.find('*');
			if (pos!=string::npos)
				tmp.erase(pos,1);
			
			pos=tmp.find("const");
			if (pos!=string::npos)
				tmp.erase(pos,strlen("const"));
			
			while ((pos=tmp.find(' '))!=string::npos){
				tmp.erase(pos,1);
			}
			
			if ((ret=mTypes[tmp])==0){
				cout<<"Adding new class type '"<<tmp<<"'"<<endl;
				ret=mTypes[tmp]=new Type(Type::Klass,tmp);
			}
			return ret;
		}
		cout<<"Unhandled type name"<<tname<<endl;
		return NULL;
	}
	const string &getName()const{
		return mName;
	}
private:
	BasicType mBasic;
	string mName;
	Type(BasicType basic, const std::string &tname="") : mBasic(basic), mName(tname){
	}
	static Type sStringType;
	static Type sIntegerType;
	static std::map<string,Type*> mTypes;
};

Type Type::sStringType(Type::String);
Type Type::sIntegerType(Type::Integer);
std::map<string,Type*> Type::mTypes;

class Argument{
public:
	Argument(Type *type, const string &argname, bool isConst) : mType(type), mName(argname), mConst(isConst){
	}
	Type *getType()const{
		return mType;
	}
private:
	bool mConst;
	Type *mType;
	string mName;
	string mHelp;
};

class Method{
public:
	Method(const std::string &uid, Argument* return_arg, const std::string &name, const list<Argument*> &args){
		mUid=uid;
		mReturn=return_arg;
		mName=name;
		mArgs=args;
	}
	void setHelp(const std::string &help){
		mHelp=help;
	}
private:
	string mUid;
	Argument *mReturn;
	string mName;
	list<Argument*> mArgs;
	string mHelp;
};

class Class{
public:
	Class(const std::string &name): mName(name){
	}
	void addMethod(Method *method){
		mMethods.push_back(method);
	}
	void setHelp(const std::string &help){
		mHelp=help;
	}
private:
	list<Method*> mMethods;
	string mName;
	string mHelp;
};

class Project{
public:
	Class *getClass(const std::string &name){
		Class *ret;
		if ((ret=mClasses[name])==NULL){
			ret=mClasses[name]=new Class(name);
		}
		return ret;
	}
private:
	map<string,Class*> mClasses;
};

static xmlNode * findChild(xmlNode *a_node, const char *element_name){
	xmlNode *cur_node;
	
	for (cur_node = a_node->children; cur_node != NULL ; cur_node = cur_node->next){
		if (strcmp((const char*)cur_node->name,(const char*)element_name)==0){
			return cur_node;
		}
	}
	return NULL;
}

static bool isSpace(const char *str){
	for(;*str!='\0';++str){
		if (!isspace(*str)) return false;
	}
	return true;
}

static string findChildContent(xmlNode *a_node, const char *element_name){
	xmlNode *node=findChild(a_node,element_name);
	string res;
	if (node) {
		xmlChar *text=xmlNodeGetContent(node);
		if (!isSpace((const char*)text))
			res=(char*)text;
		xmlFree(text);
	}
	return res;
}

#define nullOrEmpty(p)	(p==NULL || *p=='\0')

static Argument *parseArgument(xmlNode *node){
	string name=findChildContent(node,"name");
	
	xmlNode *typenode=findChild(node,"type");
	
	if (!typenode) {
		cout<<"Cannot find type from node."<<endl;
		return NULL;
	}
	
	string tname=findChildContent(typenode,"ref");
	if (tname.empty()){
		return NULL;
	}
	Type *type=Type::getType(tname);
	if (type==NULL) {
		return NULL;
	}
	return new Argument(type,name,false);
}

static string classNameToPrefix(const std::string &classname){
	char tmp[classname.size()*2];
	char *w=tmp;
	ssize_t i;
	
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
	ssize_t i;
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

static void parseFunction(Project *proj, xmlNode *node){
	string name;
	Argument *first_arg=NULL;
	xmlNode *cur_node;
	string className;
	string methodName;
	
	for (cur_node = node->children; cur_node != NULL ; cur_node = cur_node->next){
		if (strcmp((const char*)cur_node->name,"name")==0){
			xmlChar *content=xmlNodeGetContent(cur_node);
			name=(const char*)content;
			xmlFree(content);
		}else if (strcmp((const char*)cur_node->name,"param")==0){
			if (first_arg==NULL){
				first_arg=parseArgument(cur_node);
			}
		}
	}
	if (!first_arg){
		cout<<"Could not determine first argument of "<<name<<endl;
		return;
	}
	className=first_arg->getType()->getName();
	methodName=extractMethodName(name,className);
	if (!methodName.empty()){
		cout<<"Found "<<className<<"."<<methodName<<"()"<<endl;
	}
}

static void parseMemberDef(Project *proj, xmlNode *node){
	string brief;
	string detailed;
	const xmlChar *kind;
	
	brief=findChildContent(node,"briefdescription");
	detailed=findChildContent(node,"detaileddescription");
	
	if (brief.empty() && detailed.empty())
		return;
	
	kind=xmlGetProp(node,(const xmlChar*)"kind");
	//cout<<"Kind="<<(const char*)kind<<endl;
	if (kind && xmlStrcmp(kind,(const xmlChar*)"function")==0){
		parseFunction(proj,node);
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
}

int main(int argc, char *argv[]){
	int i;
	Project *proj=new Project();
	
	LIBXML_TEST_VERSION
	
	for(i=1;i<argc;i++){
		if (strcmp(argv[i],"--help")==0){
			fprintf(stderr,"%s: [--help] file1 file2...\nParses xml files generated by doxygen to output wrappers in a specified language.\n",argv[0]);
			return -1;
		}else{
			if (parse_file(proj,argv[i])==-1)
				break;
		}
	}
	
	return 0;
}