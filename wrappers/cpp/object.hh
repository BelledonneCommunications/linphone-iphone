/*
object.hh
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

#ifndef _LINPHONE_OBJECT_HH
#define _LINPHONE_OBJECT_HH

#include <memory>
#include <list>
#include <map>
#include <belle-sip/object.h>
#include <bctoolbox/list.h>

namespace linphone {
	
	class Object;
	class Listener;
	
	
	class AbstractBctbxListWrapper {
	public:
		AbstractBctbxListWrapper(): mCList(NULL) {}
		virtual ~AbstractBctbxListWrapper() {}
		::bctbx_list_t *c_list() {return mCList;}
		
	protected:
		::bctbx_list_t *mCList;
	};
	
	
	template <class T>
	class ObjectBctbxListWrapper: public AbstractBctbxListWrapper {
	public:
		ObjectBctbxListWrapper(const std::list<std::shared_ptr<T> > &cppList);
		virtual ~ObjectBctbxListWrapper();
	};
	
	
	class StringBctbxListWrapper: public AbstractBctbxListWrapper {
	public:
		StringBctbxListWrapper(const std::list<std::string> &cppList);
		virtual ~StringBctbxListWrapper();
	};
	
	
	class Object: public std::enable_shared_from_this<Object> {
	public:
		Object(::belle_sip_object_t *ptr, bool takeRef=true);
		virtual ~Object();
		
	public:
		template <class T>
		void setData(const std::string &key, T &data) {
			std::map<std::string,void *> &userData = getUserData();
			userData[key] = &data;
		}
		template <class T>
		T &getData(const std::string &key) {
			std::map<std::string,void *> &userData = getUserData();
			void *ptr = userData[key];
			if (ptr == NULL) {
				throw std::out_of_range("unknown data key [" + key + "]");
			} else {
				return *(T *)ptr;
			}
		}
		void unsetData(const std::string &key);
		bool dataExists(const std::string &key);
	
	public:
		template <class T>
		static std::shared_ptr<T> cPtrToSharedPtr(::belle_sip_object_t *ptr, bool takeRef=true) {
			if (ptr == NULL) {
				return nullptr;
			} else {
				T *cppPtr = (T *)belle_sip_object_data_get(ptr, "cpp_object");
				if (cppPtr == NULL) {
					return std::make_shared<T>(ptr, takeRef);
				} else {
					return std::static_pointer_cast<T,Object>(cppPtr->shared_from_this());
				}
			}
		}
		static ::belle_sip_object_t *sharedPtrToCPtr(const std::shared_ptr<const Object> &sharedPtr);
		
	protected:
		static std::string cStringToCpp(const char *cstr);
		static std::string cStringToCpp(char *cstr);
		static const char *cppStringToC(const std::string &cppstr);
		
		template <class T>
		static std::list<std::shared_ptr<T> > bctbxObjectListToCppList(const ::bctbx_list_t *bctbxList) {
			std::list<std::shared_ptr<T> > cppList;
			for(const ::bctbx_list_t *it=bctbxList; it!=NULL; it=it->next) {
				std::shared_ptr<T> newObj = Object::cPtrToSharedPtr<T>((::belle_sip_object_t *)it->data);
				cppList.push_back(newObj);
			}
			return cppList;
		}
		
		static std::list<std::string> bctbxStringListToCppList(const ::bctbx_list_t *bctbxList);
		static std::list<std::string> cStringArrayToCppList(const char **cArray);
	
	private:
		std::map<std::string,void *> &getUserData() const;
		template <class T> static void deleteSharedPtr(std::shared_ptr<T> *ptr) {if (ptr != NULL) delete ptr;}
		static void deleteString(std::string *str) {if (str != NULL) delete str;}
	
	protected:
		::belle_sip_object_t *mPrivPtr;
	
	private:
		static const std::string sUserDataKey;
	};
	
	
	class Listener {
		public:
			Listener(): mCbs(NULL) {}
			virtual ~Listener() {setCallbacks(NULL);}
		
		public:
			void setCallbacks(::belle_sip_object_t *cbs) {
				if (mCbs != NULL) belle_sip_object_unref(mCbs);
				if (cbs == NULL) mCbs = NULL;
				else mCbs = belle_sip_object_ref(cbs);
			}
			belle_sip_object_t *getCallbacks() {return mCbs;}
		
		private:
			::belle_sip_object_t *mCbs;
	};
	
	
	class ListenableObject: public Object {
	protected:
		ListenableObject(::belle_sip_object_t *ptr, bool takeRef=true);
		void setListener(const std::shared_ptr<Listener> &listener);
	
	protected:
		static std::shared_ptr<Listener> & getListenerFromObject(::belle_sip_object_t *object);
	
	private:
		static void deleteListenerPtr(std::shared_ptr<Listener> *ptr) {delete ptr;}
	
	private:
		static std::string sListenerDataName;
	};
	
	class MultiListenableObject: public Object {
		friend class Factory;
		
	protected:
		MultiListenableObject(::belle_sip_object_t *ptr, bool takeRef=true);
		virtual ~MultiListenableObject();
		
	protected:
		void addListener(const std::shared_ptr<Listener> &listener);
		void removeListener(const std::shared_ptr<Listener> &listener);
	
	private:
		static void deleteListenerList(std::list<std::shared_ptr<Listener> > *listeners) {delete listeners;}
	
	private:
		static std::string sListenerListName;
	};
	
};

#endif // _LINPHONE_OBJECT_HH
