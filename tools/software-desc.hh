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

#ifndef software_desc_hh
#define software_desc_hh

#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <algorithm>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>


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
		Class,
		Callback,
		Array
	};
	static const char *sBasicTypeNames[];
	static Type* addType(BasicType bt, const string &name){
		Type* ret;
		if ((ret=mTypes[name])==0){
			//cout<<"Adding new "<<sBasicTypeNames[(int)bt]<<" type '"<<name<<"'"<<endl;
			ret=mTypes[name]=new Type(bt,name);
		}else if (bt!=Class){
			ret->mBasic=bt;
		}
		return ret;
	}
	static Type *getType(const std::string &tname){
		if (tname.find("(")!=string::npos) return NULL; //arrives when parsing function pointer declared inside function prototype
		if (strstr(tname.c_str(),"char")!=0 && strchr(tname.c_str(),'*')!=0){
			return &sStringType;
		}else if (tname.find("int")!=string::npos){
			return &sIntegerType;
		}else if (tname.find("size_t")!=string::npos){
			return &sIntegerType;
		}else if (tname.find("float")!=string::npos){
			return &sFloatType;
		}else if (tname.find("bool_t")!=string::npos){
			return &sBooleanType;
		}else if (tname.find("void")!=string::npos){
			return &sVoidType;
		}else if (tname.find("enum")!=string::npos){
			return addType(Enum,tname.c_str()+strlen("enum "));
		}else if (tname.find("MSList")!=string::npos){
			return &sArrayType;
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
	static Type sArrayType;
	static std::map<string,Type*> mTypes;
};



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
	void setHelp(const string &help){
		mHelp=help;
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
	Method(const std::string &uid, Argument* return_arg, const std::string &name, const list<Argument*> &args, bool isConst, bool isStatic, bool isCallback=false){
		mUid=uid;
		mReturn=return_arg;
		mName=name;
		mArgs=args;
		mConst=isConst;
		mStatic=isStatic;
		mIsCallback=isCallback;
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
	bool isCallback()const{
		return mIsCallback;
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
		}else if (mName.find("is")==0 && mArgs.size()==0){
			mPropertyName=mName.substr(2,string::npos);
			if (!mPropertyName.empty()){
				mPropertyName[0]=tolower(mPropertyName[0]);
				mPropertyBehaviour=Read;
			}
		}else if (mName.find("enable")==0 && mArgs.size()==1){
			mPropertyName=mName.substr(6,string::npos);
			if (!mPropertyName.empty()){
				mPropertyName[0]=tolower(mPropertyName[0]);
				mPropertyName+="Enabled";
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
					mPropertyName+="Enabled";
					mPropertyBehaviour=Read;
				}
			}
		}
		if (mPropertyBehaviour==None) {
			mPropertyName="";
			if (mName.find("create")==0) {
				mName="new"+mName.substr(6,string::npos);
			}
		}
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
	bool mIsCallback;
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

class ConstField{
public:
	ConstField(Type *type, const string &name, int value) : mType(type), mName(name), mValue(value){
	}
	void setHelp(const string & help){
		mHelp=help;
	}
	const string &getHelp()const{
		return mHelp;
	}
	const string & getName()const{
		return mName;
	}
	Type *getType()const{
		return mType;
	}
	int getValue()const{
		return mValue;
	}
	static string getCommonPrefix(list<ConstField *> fields){
		if (fields.size()<2) return "";
		list<ConstField*>::iterator it;
		string prefix=fields.front()->getName();
		int prefixsize;
		
		for (prefixsize=prefix.size();prefixsize>0;prefixsize--){
			bool isMatching=true;
			prefix=prefix.substr(0,prefixsize);
			
			for(it=fields.begin();it!=fields.end();++it){
				ConstField *cf=*it;
				if (prefix != cf->getName().substr(0,prefixsize)){
					isMatching=false;
					break;
				}
			}
			if (isMatching){
				//cout<<"enum prefix: "<<prefix<<endl;
				return prefix;
			}
		}
		return "";
	}
private:
	Type *mType;
	string mName;
	int mValue;
	string mHelp;
};

template <typename _type>
struct name_matcher{
	name_matcher(const string &name) : mName(name){}
	bool operator()(_type *cf){
		return cf->getName()==mName;
	}
	string mName;
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
	void addConstField(ConstField *field){
		list<ConstField*>::iterator it=find_if(mConstFields.begin(),mConstFields.end(),name_matcher<ConstField>(field->getName()));
		if (it==mConstFields.end())
			mConstFields.push_back(field);
	}
	void setHelp(const std::string &help){
		mHelp=help;
	}
	list<Method*> getMethods()const{
		list<Method*> ret;
		map<string,Method*>::const_iterator it;
		for(it=mMethods.begin();it!=mMethods.end();++it){
			ret.push_back((*it).second);
		}
		return ret;
	}
	const list<ConstField*> &getConstFields()const{
		return mConstFields;
	}
	const string &getName()const{
		return mName;
	}
	const string &getHelp()const{
		return mHelp;
	}
	list<Property*> getProperties(){
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
	list<ConstField*> mConstFields;
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
	void addCallback(Method *callback){
		list<Method*>::iterator it=find_if(mCallbacks.begin(),mCallbacks.end(),name_matcher<Method>(callback->getName()));
		if (it==mCallbacks.end())
			mCallbacks.push_back(callback);
	}
	const list<Method*> &getCallbacks()const{
		return mCallbacks;
	}
private:
	map<string,Class*> mClasses;
	list<Method*> mCallbacks;
	string mName;
};

#endif
