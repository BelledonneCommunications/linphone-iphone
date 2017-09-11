/*
 * c-tools.h
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _C_TOOLS_H_
#define _C_TOOLS_H_

#include <memory>
#include <string>

// From coreapi.
#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Wrapper {
private:
	template<typename T>
	struct WrappedObject {
		belle_sip_object_t base;
		std::shared_ptr<T> cppPtr;
	};

public:
	template<typename T>
	static inline decltype (std::declval<T>().getPrivate()) getPrivate (T *object) {
		if (!object)
			return nullptr;
		return object->getPrivate();
	}

	template<typename T>
	static inline std::shared_ptr<T> getCppPtrFromC (void *object) {
		L_ASSERT(object);
		return static_cast<WrappedObject<T> *>(object)->cppPtr;
	}

	template<typename T>
	static inline std::shared_ptr<const T> getCppPtrFromC (const void *object) {
		L_ASSERT(object);
		return static_cast<const WrappedObject<const T> *>(object)->cppPtr;
	}

	template<typename T>
	static inline void setCppPtrFromC (void *object, std::shared_ptr<T> &cppPtr) {
		L_ASSERT(object);
		static_cast<WrappedObject<T> *>(object)->cppPtr = cppPtr;
	}

private:
	Wrapper ();

	L_DISABLE_COPY(Wrapper);
};

LINPHONE_END_NAMESPACE

// -----------------------------------------------------------------------------

#define L_DECLARE_C_STRUCT_IMPL(STRUCT, C_NAME) \
	struct _Linphone ## STRUCT { \
		belle_sip_object_t base; \
		std::shared_ptr<LINPHONE_NAMESPACE::STRUCT> cppPtr; \
	}; \
	BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## STRUCT); \
	static Linphone ## STRUCT *_linphone_ ## C_NAME ## _init() { \
		Linphone ## STRUCT * object = belle_sip_object_new(Linphone ## STRUCT); \
		new(&object->cppPtr) std::shared_ptr<LINPHONE_NAMESPACE::STRUCT>(); \
		return object; \
	} \
	static void _linphone_ ## C_NAME ## _uninit(Linphone ## STRUCT * object) { \
		object->cppPtr.~shared_ptr (); \
	} \
	static void _linphone_ ## C_NAME ## _clone(Linphone ## STRUCT * dest, const Linphone ## STRUCT * src) { \
		new(&dest->cppPtr) std::shared_ptr<LINPHONE_NAMESPACE::STRUCT>(); \
		dest->cppPtr = std::make_shared<LINPHONE_NAMESPACE::STRUCT>(*src->cppPtr.get()); \
	} \
	BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## STRUCT); \
	BELLE_SIP_INSTANCIATE_VPTR(Linphone ## STRUCT, belle_sip_object_t, \
	_linphone_ ## C_NAME ## _uninit, \
	_linphone_ ## C_NAME ## _clone, \
	NULL, \
	FALSE \
	);

#define L_DECLARE_C_STRUCT_NEW_DEFAULT(STRUCT, C_NAME) \
	Linphone ## STRUCT * linphone_ ## C_NAME ## _new() { \
		Linphone ## STRUCT * object = _linphone_ ## C_NAME ## _init(); \
		object->cppPtr = std::make_shared<LINPHONE_NAMESPACE::STRUCT>(); \
		return object; \
	}

#define L_STRING_TO_C(STR) ((STR).empty() ? NULL : (STR).c_str())
#define L_C_TO_STRING(STR) ((STR) == NULL ? std::string() : (STR))

#define L_GET_CPP_PTR_FROM_C_STRUCT(OBJECT, TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppPtrFromC<LINPHONE_NAMESPACE::TYPE>(OBJECT)

#define L_SET_CPP_PTR_FROM_C_STRUCT(OBJECT, CPP_PTR) \
	LINPHONE_NAMESPACE::Wrapper::setCppPtrFromC(OBJECT, CPP_PTR)

#define L_GET_PRIVATE(OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getPrivate(OBJECT)

#define L_GET_PRIVATE_FROM_C_STRUCT(OBJECT, TYPE) \
	L_GET_PRIVATE(L_GET_CPP_PTR_FROM_C_STRUCT(OBJECT, TYPE).get())

#endif // ifndef _C_TOOLS_H_
