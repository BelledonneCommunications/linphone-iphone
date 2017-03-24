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
#include <belle-sip/object.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>

using namespace linphone;
using namespace std;


const std::string Object::sUserDataKey = "cppUserData";

Object::Object(void *ptr, bool takeRef):
		enable_shared_from_this<Object>(), mPrivPtr(ptr) {
	if(takeRef) belle_sip_object_ref(mPrivPtr);
	belle_sip_object_data_set((belle_sip_object_t *)ptr, "cpp_object", this, NULL);
}

Object::~Object() {
	if(mPrivPtr != NULL) {
		belle_sip_object_data_set((::belle_sip_object_t *)mPrivPtr, "cpp_object", NULL, NULL);
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

void *linphone::Object::sharedPtrToCPtr(const std::shared_ptr< const linphone::Object > &sharedPtr) {
	if (sharedPtr == nullptr) return NULL;
	else return sharedPtr->mPrivPtr;
}

static void deleteCppUserDataMap(std::map<std::string,void *> *userDataMap) {
	delete userDataMap;
}

std::map<std::string,void *> &Object::getUserData() const {
	map<string,void *> *userData = (map<string,void *> *)belle_sip_object_data_get((::belle_sip_object_t *)mPrivPtr, sUserDataKey.c_str());
	if (userData == NULL) {
		userData = new map<string,void *>();
		belle_sip_object_data_set((::belle_sip_object_t *)mPrivPtr, sUserDataKey.c_str(), userData, (belle_sip_data_destroy)deleteCppUserDataMap);
	}
	return *userData;
}

linphone::Object *linphone::Object::getBackPtrFromCPtr(const void *ptr) {
	return (Object *)belle_sip_object_data_get((::belle_sip_object_t *)ptr, "cpp_object");
}


std::string ListenableObject::sListenerDataName = "cpp_listener";

ListenableObject::ListenableObject(void *ptr, bool takeRef): Object(ptr, takeRef) {
	shared_ptr<Listener> *cppListener = (shared_ptr<Listener> *)belle_sip_object_data_get((::belle_sip_object_t *)mPrivPtr, sListenerDataName.c_str());
	if (cppListener == NULL) {
		cppListener = new shared_ptr<Listener>();
		belle_sip_object_data_set((::belle_sip_object_t *)mPrivPtr, sListenerDataName.c_str(), cppListener, (belle_sip_data_destroy)deleteListenerPtr);
	}
}

void ListenableObject::setListener(const std::shared_ptr<Listener> &listener) {
	shared_ptr<Listener> &curListener = *(shared_ptr<Listener> *)belle_sip_object_data_get((::belle_sip_object_t *)mPrivPtr, sListenerDataName.c_str());
	curListener = listener;
}

std::shared_ptr<Listener> & ListenableObject::getListenerFromObject(void *object) {
	return *(std::shared_ptr<Listener> *)belle_sip_object_data_get((::belle_sip_object_t *)object, sListenerDataName.c_str());
}


std::string MultiListenableObject::sListenerListName = "cpp_listeners";

MultiListenableObject::MultiListenableObject(void *ptr, bool takeRef): Object(ptr, takeRef) {
	if (ptr != NULL) {
		if (belle_sip_object_data_get((::belle_sip_object_t *)ptr, sListenerListName.c_str()) == NULL) {
			list<shared_ptr<Listener> > *listeners = new list<shared_ptr<Listener> >;
			belle_sip_object_data_set((::belle_sip_object_t *)ptr, sListenerListName.c_str(), listeners, (belle_sip_data_destroy)deleteListenerList);
		}
	}
}

std::list<std::shared_ptr<Listener> > &MultiListenableObject::getListeners() const {
	return *(std::list<std::shared_ptr<Listener> > *)belle_sip_object_data_get((::belle_sip_object_t *)mPrivPtr, sListenerListName.c_str());
}

void MultiListenableObject::addListener(const std::shared_ptr<Listener> &listener) {
	std::list<std::shared_ptr<Listener> > &listeners = *(std::list<std::shared_ptr<Listener> > *)belle_sip_object_data_get((::belle_sip_object_t *)mPrivPtr, sListenerListName.c_str());
	listeners.push_back(listener);
}

void MultiListenableObject::removeListener(const std::shared_ptr<Listener> &listener) {
	std::list<std::shared_ptr<Listener> > &listeners = *(std::list<std::shared_ptr<Listener> > *)belle_sip_object_data_get((::belle_sip_object_t *)mPrivPtr, sListenerListName.c_str());
	listeners.remove(listener);
}
