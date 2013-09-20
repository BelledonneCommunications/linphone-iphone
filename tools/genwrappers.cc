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
#include <fstream>
#include <sstream>
#include <list>
#include <map>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>

using namespace::std;


class Type{
public:
	enum BasicType{
		Void,
		Boolean,
		Integer,
		Float,
		String,
		Enum,
		Class
	};
	static Type* addType(BasicType bt, const string &name){
		Type* ret;
		if ((ret=mTypes[name])==0){
			cout<<"Adding new "<<(bt==Enum ? "enum" : "class")<<" type '"<<name<<"'"<<endl;
			ret=mTypes[name]=new Type(bt,name);
		}else if (bt==Enum){
			ret->mBasic=Enum;
		}
		return ret;
	}
	static Type *getType(const std::string &tname){
		if (strstr(tname.c_str(),"char")!=0 && strchr(tname.c_str(),'*')!=0){
			return &sStringType;
		}else if (tname.find("int")==0){
			return &sIntegerType;
		}else if (tname.find("float")==0){
			return &sFloatType;
		}else if (tname.find("bool_t")==0){
			return &sBooleanType;
		}else if (tname.find("void")!=string::npos){
			return &sVoidType;
		}else if (tname.find("enum")==0){
			return addType(Enum,tname.c_str()+strlen("enum "));
		}else{/*an object?*/
			
			string tmp=tname;
			size_t pos;
			
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
			return addType(Class,tmp);
		}
		cerr<<"Unhandled type name"<<tname<<endl;
		return NULL;
	}
	const string &getName()const{
		return mName;
	}
	BasicType getBasicType()const{
		return mBasic;
	}
private:
	BasicType mBasic;
	string mName;
	Type(BasicType basic, const std::string &tname="") : mBasic(basic), mName(tname){
	}
	static Type sStringType;
	static Type sIntegerType;
	static Type sVoidType;
	static Type sBooleanType;
	static Type sFloatType;
	static std::map<string,Type*> mTypes;
};

Type Type::sStringType(Type::String);
Type Type::sIntegerType(Type::Integer);
Type Type::sVoidType(Type::Void);
Type Type::sBooleanType(Type::Boolean);
Type Type::sFloatType(Type::Float);
std::map<string,Type*> Type::mTypes;

class Argument{
public:
	Argument(Type *type, const string &argname, bool isConst, bool isPointer) : mType(type), mName(argname), mConst(isConst), mPointer(isPointer){
		if (!isPointer) mConst=false;
	}
	Type *getType()const{
		return mType;
	}
	bool isConst()const{
		return mConst;
	}
	const string &getName()const{
		return mName;
	}
	bool isPointer()const{
		return mPointer;
	}
	const string &getHelp()const{
		return mHelp;
	}
private:
	
	Type *mType;
	string mName;
	string mHelp;
	bool mConst;
	bool mPointer;
};

class Method{
public:
	enum PropertyBehaviour{
		None,
		Read,
		Write
	};
	Method(const std::string &uid, Argument* return_arg, const std::string &name, const list<Argument*> &args, bool isConst, bool isStatic){
		mUid=uid;
		mReturn=return_arg;
		mName=name;
		mArgs=args;
		mConst=isConst;
		mStatic=isStatic;
		analyseProperties();
	}
	void setHelp(const std::string &help){
		mHelp=help;
	}
	Argument *getReturnArg()const{
		return mReturn;
	}
	const string &getName()const{
		return mName;
	}
	const list<Argument*> &getArgs()const {
		return mArgs;
	}
	bool isConst()const{
		return mConst;
	}
	bool isStatic()const{
		return mStatic;
	}
	const string &getHelp(){
		return mHelp;
	}
	PropertyBehaviour getPropertyBehaviour()const{
		return mPropertyBehaviour;
	}
	const string &getPropertyName()const{
		return mPropertyName;
	}
private:
	void analyseProperties(){
		size_t enabled_pos;
		mPropertyBehaviour=None;
		
		if (mName.find("get")==0 && mArgs.size()==0){
			mPropertyName=mName.substr(3,string::npos);
			if (!mPropertyName.empty()){
				mPropertyName[0]=tolower(mPropertyName[0]);
				mPropertyBehaviour=Read;
			}
		}else if (mName.find("enable")==0 && mArgs.size()==1){
			mPropertyName=mName.substr(6,string::npos);
			if (!mPropertyName.empty()){
				mPropertyName[0]=tolower(mPropertyName[0]);
				mPropertyBehaviour=Write;
			}
		}else if (mName.find("set")==0 && mArgs.size()==1){
			mPropertyName=mName.substr(3,string::npos);
			if (!mPropertyName.empty()){
				mPropertyName[0]=tolower(mPropertyName[0]);
				mPropertyBehaviour=Write;
			}
		}else if ((enabled_pos=mName.rfind("Enabled"))!=string::npos && mArgs.size()==0){
			size_t goodpos=mName.size()-7;
			if (enabled_pos==goodpos){
				mPropertyName=mName.substr(0,goodpos);
				if (!mPropertyName.empty()){
					mPropertyBehaviour=Read;
				}
			}
		}
		if (mPropertyBehaviour==None)
			mPropertyName="";
	}
	string mUid;
	Argument *mReturn;
	string mName;
	list<Argument*> mArgs;
	string mHelp;
	string mPropertyName; /*if it can be a property*/
	PropertyBehaviour mPropertyBehaviour;
	bool mConst;
	bool mStatic;
};

class Property{
public:
	enum Attribute{
		ReadOnly,
		ReadWrite
	};
	Property(Attribute attr, const string &name, Type *type, const string &help) : mAttr(attr), mName(name), mType(type), mHelp(help){
	}
	const string &getName()const{
		return mName;
	}
	const string &getHelp()const{
		return mHelp;
	}
	void setHelp(const string &help){
		mHelp=help;
	}
	Attribute getAttribute()const{
		return mAttr;
	}
	void setAttribute(Attribute attr){
		mAttr=attr;
	}
	Type* getType()const{
		return mType;
	}
private:
	Attribute mAttr;
	string mName;
	Type *mType;
	string mHelp;
};

/*actually a class or an enum*/
class Class{
public:
	Class(const std::string &name): mName(name){
	}
	Type::BasicType getType(){
		return Type::getType(mName)->getBasicType();
	}
	void addMethod(Method *method){
		if (mMethods.find(method->getName())==mMethods.end())
			mMethods.insert(make_pair(method->getName(),method));
	}
	void setHelp(const std::string &help){
		mHelp=help;
	}
	const list<Method*> getMethods()const{
		list<Method*> ret;
		map<string,Method*>::const_iterator it;
		for(it=mMethods.begin();it!=mMethods.end();++it){
			ret.push_back((*it).second);
		}
		return ret;
	}
	const string &getName()const{
		return mName;
	}
	const string &getHelp()const{
		return mHelp;
	}
	const list<Property*> getProperties(){
		list<Property*> ret;
		map<string,Property*>::const_iterator it;
		for(it=mProperties.begin();it!=mProperties.end();++it){
			ret.push_back((*it).second);
		}
		return ret;
	}
	void computeProperties(){
		map<string,Method*>::const_iterator it;
		Property *prop;
		for (it=mMethods.begin();it!=mMethods.end();++it){
			Method *m=(*it).second;
			if (m->getPropertyBehaviour()==Method::Read){
				prop=mProperties[m->getPropertyName()];
				if (prop==NULL){
					prop=new Property(Property::ReadOnly,m->getPropertyName(),m->getReturnArg()->getType(), m->getHelp());
					mProperties[m->getPropertyName()]=prop;
				}
			}else if (m->getPropertyBehaviour()==Method::Write){
				prop=mProperties[m->getPropertyName()];
				if (prop==NULL){
					prop=new Property(Property::ReadWrite,m->getPropertyName(),m->getArgs().front()->getType(), m->getHelp());
					mProperties[m->getPropertyName()]=prop;
				}else{
					prop->setHelp(m->getHelp());
					prop->setAttribute(Property::ReadWrite);
				}
			}
		}
	}
private:
	map<string,Method*> mMethods;
	map<string,Property*> mProperties;
	string mName;
	string mHelp;
};

class Project{
public:
	Project(const string &name="wrapper") : mName(name){
	}
	Class *getClass(const std::string &name){
		Class *ret;
		if ((ret=mClasses[name])==NULL){
			ret=mClasses[name]=new Class(name);
		}
		return ret;
	}
	list<Class*> getClasses()const{
		list<Class*> ret;
		map<string,Class*>::const_iterator it;
		for(it=mClasses.begin();it!=mClasses.end();++it){
			ret.push_back((*it).second);
		}
		return ret;
	}
	const string &getName()const{
		return mName;
	}
	void analyse(){
		list<Class*> classes=getClasses();
		for_each(classes.begin(),classes.end(),mem_fun(&Class::computeProperties));
	}
private:
	map<string,Class*> mClasses;
	string mName;
};

class OutputGenerator{
public:
	virtual void generate(Project *proj)=0;
};

class CplusplusGenerator : public OutputGenerator{
public:
	CplusplusGenerator(){
	}
	virtual void generate(Project *proj){
		list<Class*> classes=proj->getClasses();
		mCurProj=proj;
#ifndef WIN32
		mkdir(proj->getName().c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);
#else
		_mkdir(proj->getName().c_str());
#endif
		for_each(classes.begin(),classes.end(),bind1st(mem_fun(&CplusplusGenerator::writeClass),this));
	}
private:
	void writeClass(Class *klass){
		ostringstream filename;
		
		if (klass->getType()!=Type::Class) return;
		filename<<mCurProj->getName()<<"/"<<klass->getName()<<".hh";
		mOutfile.open(filename.str().c_str());
		if (!mOutfile.is_open()){
			cerr<<"Could not write into "<<filename.str()<<endl;
			return;
		}
		const list<Method*> &methods=klass->getMethods();
		mCurClass=klass;
		mOutfile<<"/* Wrapper generated by lp-gen-wrappers, do not edit*/"<<endl;
		mOutfile<<endl;
		mOutfile<<"#include <string>"<<endl;
		mOutfile<<endl;
		if (!mCurProj->getName().empty())
			mOutfile<<"namespace "<<mCurProj->getName()<<"{"<<endl<<endl;
		mOutfile<<"class "<<klass->getName()<<"{"<<endl;
		mOutfile<<"public:"<<endl;
		for_each(methods.begin(),methods.end(),bind1st(mem_fun(&CplusplusGenerator::writeMethod),this));
		mOutfile<<"}"<<endl<<endl;
		if (!mCurProj->getName().empty())
			mOutfile<<"} //end of namespace "<<mCurProj->getName()<<endl;
		mOutfile<<endl;
		mOutfile.close();
	}
	void writeArgument(Argument *arg, bool isReturn=false){
		Type *type=arg->getType();
		
		if (type->getBasicType()==Type::Class){
			if (arg->isConst()){
				mOutfile<<"const ";
			}
			mOutfile<<type->getName();
			if (arg->isPointer())
				mOutfile<<"*";
		}else if (type->getBasicType()==Type::Integer){
			mOutfile<<"int";
		}else if (type->getBasicType()==Type::Enum){
			mOutfile<<type->getName();
		}else if (type->getBasicType()==Type::String){
			if (!isReturn)
				mOutfile<<"const std::string &";
			else 
				mOutfile<<"std::string";
		}else if (type->getBasicType()==Type::Void){
			mOutfile<<"void";
		}else if (type->getBasicType()==Type::Boolean){
			mOutfile<<"bool";
		}
		if (!isReturn && !arg->getName().empty())
			mOutfile<<" "<<arg->getName();
	}
	void writeTabs(int ntabs){
		int i;
		for(i=0;i<ntabs;++i)
			mOutfile<<"\t";
	}
	void writeHelpComment(const std::string &comment, int ntabs){
		size_t i;
		int curindex=0;
		writeTabs(ntabs);
		mOutfile<<" * ";
		for(i=0;i<comment.size();i++,curindex++){
			
			if (comment[i]=='\n' || (curindex>100 && comment[i]==' ')){
				mOutfile<<endl;
				writeTabs(ntabs);
				mOutfile<<" * ";
				curindex=0;
			}else mOutfile<<comment[i];
		}
	}
	void writeMethod(Method *method){
		Argument *retarg=method->getReturnArg();
		const list<Argument*> &args=method->getArgs();
		list<Argument*>::const_iterator it;
		
		writeTabs(1);
		mOutfile<<"/**"<<endl;
		writeHelpComment(method->getHelp(),1);
		mOutfile<<endl;
		writeTabs(1);
		mOutfile<<"**/"<<endl;
		
		writeTabs(1);
		writeArgument(retarg,true);
		mOutfile<<" "<<method->getName()<<"(";
		
		for(it=args.begin();it!=args.end();++it){
			if (it!=args.begin()) mOutfile<<", ";
			writeArgument(*it);
		}
		mOutfile<<")";
		if (method->isConst()) mOutfile<<"const";
		mOutfile<<";"<<endl;
		mOutfile<<endl;
	}
	ofstream mOutfile;
	Project *mCurProj;
	Class *mCurClass;
};

class JavascriptGenerator : public OutputGenerator{
public:
	JavascriptGenerator(){
	}
	virtual void generate(Project *proj){
		list<Class*> classes=proj->getClasses();
		mCurProj=proj;
#ifndef WIN32
		mkdir(proj->getName().c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);
#else
		_mkdir(proj->getName().c_str());
#endif
		for_each(classes.begin(),classes.end(),bind1st(mem_fun(&JavascriptGenerator::writeClass),this));
	}
private:
	void writeClass(Class *klass){
		ostringstream filename;
		
		if (klass->getType()!=Type::Class) return;
		filename<<mCurProj->getName()<<"/"<<klass->getName()<<".js";
		mOutfile.open(filename.str().c_str());
		if (!mOutfile.is_open()){
			cerr<<"Could not write into "<<filename.str()<<endl;
			return;
		}
		const list<Method*> &methods=klass->getMethods();
		mCurClass=klass;
		mOutfile<<"/* Wrapper generated by lp-gen-wrappers, do not edit*/"<<endl;
		mOutfile<<endl;
		
		//if (!mCurProj->getName().empty())
		//	mOutfile<<"namespace "<<mCurProj->getName()<<"{"<<endl<<endl;
		mOutfile<<"/**"<<endl;
		mOutfile<<" * @external "<<klass->getName()<<endl;
		mOutfile<<" * "<<klass->getHelp()<<endl;
		mOutfile<<"**/"<<endl;
		
		list<Property*> properties=klass->getProperties();
		for_each(properties.begin(),properties.end(),bind1st(mem_fun(&JavascriptGenerator::writeProperty),this));
		mOutfile<<endl;
		for_each(methods.begin(),methods.end(),bind1st(mem_fun(&JavascriptGenerator::writeMethod),this));
		//if (!mCurProj->getName().empty())
		//	mOutfile<<"} //end of namespace "<<mCurProj->getName()<<endl;
		mOutfile<<endl;
		mOutfile.close();
	}
	void writeType(Type *type){
		switch(type->getBasicType()){
			case Type::Float:
			case Type::Integer:
				mOutfile<<"number";
			break;
			case Type::String:
				mOutfile<<"string";
			break;
			case Type::Boolean:
				mOutfile<<"boolean";
			break;
			case Type::Class:
			case Type::Enum:
				mOutfile<<"external:"<<type->getName();
			break;
			case Type::Void:
				mOutfile<<"void";
			break;
		}
	}
	void writeArgument(Argument *arg, bool isReturn=false){
		if (!isReturn){
			mOutfile<<" * @param {";
			writeType(arg->getType());
			mOutfile<<"} "<<arg->getName()<<" - "<<arg->getHelp()<<endl;
		}else{
			mOutfile<<" * @returns {";
			writeType(arg->getType());
			mOutfile<<"} "<<arg->getHelp()<<endl;
		}
	}
	void writeTabs(int ntabs){
		int i;
		for(i=0;i<ntabs;++i)
			mOutfile<<"\t";
	}
	void writeHelpComment(const std::string &comment, int ntabs){
		size_t i;
		int curindex=0;
		mOutfile<<" * ";
		for(i=0;i<comment.size();i++,curindex++){
			
			if (comment[i]=='\n' || (curindex>100 && comment[i]==' ')){
				mOutfile<<endl;
				mOutfile<<" * ";
				curindex=0;
			}else mOutfile<<comment[i];
		}
	}
	void writeProperty(Property *prop){
		mOutfile<<"/**"<<endl;
		writeHelpComment(prop->getHelp(),0);
		mOutfile<<endl;
		mOutfile<<" * @member {";
		writeType(prop->getType());
		mOutfile<<"} external:"<<mCurClass->getName()<<"#"<<prop->getName()<<endl;
		if (prop->getAttribute()==Property::ReadOnly)
			mOutfile<<" * @readonly"<<endl;
		mOutfile<<"**/"<<endl;
	}
	void writeMethod(Method *method){
		Argument *retarg=method->getReturnArg();
		const list<Argument*> &args=method->getArgs();
		list<Argument*>::const_iterator it;
		
		if (method->getPropertyBehaviour()!=Method::None) return;
		if (method->getName()=="ref" || method->getName()=="unref") return;
		
		mOutfile<<"/**"<<endl;
		writeHelpComment(method->getHelp(),1);
		mOutfile<<endl;
		mOutfile<<" * @function external:"<<mCurClass->getName()<<"#"<<method->getName()<<endl;
		
		for(it=args.begin();it!=args.end();++it){
			writeArgument(*it);
		}
		writeArgument(retarg,true);
		mOutfile<<"**/"<<endl;
		mOutfile<<endl;
	}
	ofstream mOutfile;
	Project *mCurProj;
	Class *mCurClass;
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

static Argument *parseArgument(xmlNode *node, bool isReturn){
	string name=findChildContent(node,"declname");
	Type *type=NULL;
	xmlNode *typenode=findChild(node,"type");
	string typecontent=findChildContent(node,"type");
	bool isConst=false;
	bool isPointer=false;
	
	if (!typenode) {
		cout<<"Cannot find type from node."<<endl;
		return NULL;
	}
	
	//find documented type if any
	string tname=findChildContent(typenode,"ref");
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
	cout<<"Parsed argument "<<name<<" with type "<<type->getBasicType()<<" "<<type->getName()<<endl;
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

static void parseFunction(Project *proj, xmlNode *node){
	string name;
	Argument *first_arg=NULL;
	xmlNode *cur_node;
	string className;
	string methodName;
	Argument *retarg=NULL;
	list<Argument*> args;
	string help;
	
	for (cur_node = node->children; cur_node != NULL ; cur_node = cur_node->next){
		if (strcmp((const char*)cur_node->name,"name")==0){
			xmlChar *content=xmlNodeGetContent(cur_node);
			name=(const char*)content;
			xmlFree(content);
		}else if (strcmp((const char*)cur_node->name,"param")==0){
			Argument *a=parseArgument(cur_node,false);
			if (a) args.push_back(a);
			else return;
		}else if (strcmp((const char*)cur_node->name,"detaileddescription")==0){
			help=findChildContent(cur_node,"para");
		}
	}
	retarg=parseArgument(node,true);
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
		cout<<"Found "<<className<<"."<<methodName<<"()"<<endl;
		args.pop_front();
		Method *method=new Method("",retarg,methodName,args,first_arg->isConst(),false);
		method->setHelp(help);
		proj->getClass(className)->addMethod(method);
		delete first_arg;
	}
}

static void parseTypedef(Project *proj, xmlNode *node){
	string typecontent=findChildContent(node,"type");
	string name=findChildContent(node,"name");
	if (typecontent.find("enum")==0){
		Type::addType(Type::Enum,name);
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
	if (kind){
		//cout<<"Kind="<<(const char*)kind<<endl;
		if (xmlStrcmp(kind,(const xmlChar*)"function")==0){
			parseFunction(proj,node);
		}else if (xmlStrcmp(kind,(const xmlChar*)"typedef")==0){
			parseTypedef(proj,node);
		}
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
