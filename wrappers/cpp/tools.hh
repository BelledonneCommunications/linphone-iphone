/*
tools.hh
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

#ifndef _TOOLS_HH
#define _TOOLS_HH

#include <bctoolbox/list.h>
#include <belle-sip/object.h>
#include <list>
#include <memory>
#include "object.hh"

namespace linphone {
	
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
		ObjectBctbxListWrapper(const std::list<std::shared_ptr<T> > &cppList) {
			for(auto it=cppList.cbegin(); it!=cppList.cend(); it++) {
				::belle_sip_object_t *cPtr = (::belle_sip_object_t *)Object::sharedPtrToCPtr(std::static_pointer_cast<Object,T>(*it));
				if (cPtr != NULL) belle_sip_object_ref(cPtr);
				mCList = bctbx_list_append(mCList, cPtr);
			}
		}
		virtual ~ObjectBctbxListWrapper() {
			mCList = bctbx_list_free_with_data(mCList, unrefData);
		}
		static std::list<std::shared_ptr<T> > bctbxListToCppList(const ::bctbx_list_t *bctbxList) {
			std::list<std::shared_ptr<T> > cppList;
			for(const ::bctbx_list_t *it=bctbxList; it!=NULL; it=it->next) {
				std::shared_ptr<T> newObj = Object::cPtrToSharedPtr<T>(it->data);
				cppList.push_back(newObj);
			}
			return cppList;
		}
	
	private:
		static void unrefData(void *data) {
			if (data != NULL) belle_sip_object_unref(data);
		}
	};
	
	
	class StringBctbxListWrapper: public AbstractBctbxListWrapper {
	public:
		StringBctbxListWrapper(const std::list<std::string> &cppList);
		virtual ~StringBctbxListWrapper();
		static std::list<std::string> bctbxListToCppList(const ::bctbx_list_t *bctbxList);
	};
	
	class StringUtilities {
	public:
		static std::string cStringToCpp(const char *cstr);
		static std::string cStringToCpp(char *cstr);
		static const char *cppStringToC(const std::string &cppstr);
		static std::list<std::string> cStringArrayToCppList(const char **cArray);
	};
	
};

#endif // _TOOLS_HH
