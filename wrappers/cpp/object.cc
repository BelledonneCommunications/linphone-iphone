/*
object.cc
Copyright (C) 2017 Belledonne Communications SARL

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

#include "object.hh"
#include <bctoolbox/port.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>

using namespace linphone;
using namespace std;

template <class T>
ObjectBctbxListWrapper<T>::ObjectBctbxListWrapper(const std::list<std::shared_ptr<T> > &cppList): AbstractBctbxListWrapper() {
	for(auto it=cppList.cbegin(); it!=cppList.cend(); it++) {
		::belle_sip_object_t *cPtr = Object::sharedPtrToCPtr(static_pointer_cast<Object,T>(*it));
		if (cPtr != NULL) belle_sip_object_ref(cPtr);
		mCList = bctbx_list_append(mCList, cPtr);
	}
}

static void unrefData(void *data) {
	if (data != NULL) belle_sip_object_unref(data);
}

template <class T>
ObjectBctbxListWrapper<T>::~ObjectBctbxListWrapper() {
	mCList = bctbx_list_free_with_data(mCList, unrefData);
}

StringBctbxListWrapper::StringBctbxListWrapper(const std::list<std::string> &cppList): AbstractBctbxListWrapper() {
	for(auto it=cppList.cbegin(); it!=cppList.cend(); it++) {
		char *buffer = (char *)malloc(it->length()+1);
		strcpy(buffer, it->c_str());
		mCList = bctbx_list_append(mCList, buffer);
	}
}

StringBctbxListWrapper::~StringBctbxListWrapper() {
	mCList = bctbx_list_free_with_data(mCList, free);
}



const std::string Object::sUserDataKey = "cppUserData";

Object::Object(::belle_sip_object_t *ptr, bool takeRef):
		enable_shared_from_this<Object>(), mPrivPtr(ptr) {
	if(takeRef) belle_sip_object_ref(mPrivPtr);
	belle_sip_object_data_set(ptr, "cpp_object", this, NULL);
}

Object::~Object() {
	if(mPrivPtr != NULL) {
		belle_sip_object_data_set(mPrivPtr, "cpp_object", NULL, NULL);
		belle_sip_object_unref(mPrivPtr);
	}
}

void Object::unsetData(const std::string &key) {
	map<string,void *> userData = getUserData();
	map<string,void *>::iterator it = userData.find(key);
	if (it != userData.end()) userData.erase(it);
}

bool Object::dataExists(const std::string &key) {
	map<string,void *> userData = getUserData();
	return userData.find(key) != userData.end();
}

::belle_sip_object_t *Object::sharedPtrToCPtr(const std::shared_ptr<const Object> &sharedPtr) {
	if (sharedPtr == nullptr) return NULL;
	else return sharedPtr->mPrivPtr;
}

std::string Object::cStringToCpp(const char *cstr) {
	if (cstr == NULL) {
		return std::string();
	} else {
		return std::string(cstr);
	}
}

std::string Object::cStringToCpp(char *cstr) {
	if (cstr == NULL) {
		return std::string();
	} else {
		std::string cppStr = cstr;
		bctbx_free(cstr);
		return cppStr;
	}
}

const char *Object::cppStringToC(const std::string &cppstr) {
	if (cppstr.empty()) {
		return NULL;
	} else {
		return cppstr.c_str();
	}
}

list<string> Object::bctbxStringListToCppList(const ::bctbx_list_t *bctbxList) {
	list<string> cppList;
	for(const ::bctbx_list_t *it=bctbxList; it!=NULL; it=it->next) {
		cppList.push_back(string((char *)it->data));
	}
	return cppList;
}

std::list<std::string> Object::cStringArrayToCppList(const char **cArray) {
	list<string> cppList;
	int i;
	for(i=0; cArray[i]!=NULL; i++) {
		cppList.push_back(cArray[i]);
	}
	return cppList;
}

static void deleteCppUserDataMap(std::map<std::string,void *> *userDataMap) {
	delete userDataMap;
}

std::map<std::string,void *> &Object::getUserData() const {
	map<string,void *> *userData = (map<string,void *> *)belle_sip_object_data_get(mPrivPtr, sUserDataKey.c_str());
	if (userData == NULL) {
		userData = new map<string,void *>();
		belle_sip_object_data_set(mPrivPtr, sUserDataKey.c_str(), userData, (belle_sip_data_destroy)deleteCppUserDataMap);
	}
	return *userData;
}


std::string ListenableObject::sListenerDataName = "cpp_listener";

ListenableObject::ListenableObject(::belle_sip_object_t *ptr, bool takeRef): Object(ptr, takeRef) {
	shared_ptr<Listener> *cppListener = (shared_ptr<Listener> *)belle_sip_object_data_get(mPrivPtr, sListenerDataName.c_str());
	if (cppListener == NULL) {
		cppListener = new shared_ptr<Listener>();
		belle_sip_object_data_set(mPrivPtr, sListenerDataName.c_str(), cppListener, (belle_sip_data_destroy)deleteListenerPtr);
	}
}

void ListenableObject::setListener(const std::shared_ptr<Listener> &listener) {
	shared_ptr<Listener> &curListener = *(shared_ptr<Listener> *)belle_sip_object_data_get(mPrivPtr, sListenerDataName.c_str());
	curListener = listener;
}

std::shared_ptr<Listener> & ListenableObject::getListenerFromObject(::belle_sip_object_t *object) {
	return *(std::shared_ptr<Listener> *)belle_sip_object_data_get(object, sListenerDataName.c_str());
}


std::string MultiListenableObject::sListenerListName = "cpp_listeners";

MultiListenableObject::MultiListenableObject(::belle_sip_object_t *ptr, bool takeRef): Object(ptr, takeRef) {
	if (ptr != NULL) {
		list<shared_ptr<Listener> > *listeners = new list<shared_ptr<Listener> >;
		belle_sip_object_data_set(ptr, sListenerListName.c_str(), listeners, (belle_sip_data_destroy)deleteListenerList);
	}
}

MultiListenableObject::~MultiListenableObject() {
	if (mPrivPtr != NULL) {
		belle_sip_object_data_set(mPrivPtr, sListenerListName.c_str(), NULL, NULL);
	}
}

void MultiListenableObject::addListener(const std::shared_ptr<Listener> &listener) {
	std::list<std::shared_ptr<Listener> > &listeners = *(std::list<std::shared_ptr<Listener> > *)belle_sip_object_data_get(mPrivPtr, sListenerListName.c_str());
	listeners.push_back(listener);
}

void MultiListenableObject::removeListener(const std::shared_ptr<Listener> &listener) {
	std::list<std::shared_ptr<Listener> > &listeners = *(std::list<std::shared_ptr<Listener> > *)belle_sip_object_data_get(mPrivPtr, sListenerListName.c_str());
	listeners.remove(listener);
}
