/*
 * c-wrapper.h
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

#include <list>
#include <memory>

// TODO: From coreapi. Remove me later.
#include "private.h"

#include "variant/variant.h"

// =============================================================================
// Internal.
// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Wrapper {
private:
	template<typename T>
	struct WrappedObject {
		belle_sip_object_t base;
		std::shared_ptr<T> cppPtr;
	};

	template<typename T>
	struct WrappedClonableObject {
		belle_sip_object_t base;
		T *cppPtr;
	};

public:
	template<typename T>
	static inline decltype (std::declval<T>().getPrivate()) getPrivate (T *object) {
		if (!object)
			return nullptr;
		return object->getPrivate();
	}

	template<typename T>
	static inline decltype (std::declval<T>().getPrivate()) getPrivate (const std::shared_ptr<T> &object) {
		if (!object)
			return nullptr;
		return object->getPrivate();
	}

	// ---------------------------------------------------------------------------
	// Get c/cpp ptr helpers.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<Object, CppType>::value, CppType>::type
	>
	static inline std::shared_ptr<CppType> getCppPtrFromC (CType *object) {
		L_ASSERT(object);
		return reinterpret_cast<WrappedObject<CppType> *>(object)->cppPtr;
	}

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<Object, CppType>::value, CppType>::type
	>
	static inline std::shared_ptr<const CppType> getCppPtrFromC (const CType *object) {
		L_ASSERT(object);
		return reinterpret_cast<const WrappedObject<CppType> *>(object)->cppPtr;
	}

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static inline CppType *getCppPtrFromC (CType *object) {
		L_ASSERT(object);
		return reinterpret_cast<WrappedClonableObject<CppType> *>(object)->cppPtr;
	}

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static inline const CppType *getCppPtrFromC (const CType *object) {
		L_ASSERT(object);
		return reinterpret_cast<const WrappedClonableObject<CppType> *>(object)->cppPtr;
	}

	template<typename T>
	static inline void setCppPtrFromC (void *object, const std::shared_ptr<T> &cppPtr) {
		L_ASSERT(object);
		static_cast<WrappedObject<T> *>(object)->cppPtr = cppPtr;
		cppPtr->setProperty("LinphonePrivate::Wrapper::cBackPtr", object);
	}

	template<typename T>
	static inline void setCppPtrFromC (void *object, const T *cppPtr) {
		L_ASSERT(object);
		T *oldPtr = reinterpret_cast<T *>(static_cast<WrappedClonableObject<T> *>(object)->cppPtr);
		if (oldPtr != cppPtr) {
			delete oldPtr;
			T **cppObject = &static_cast<WrappedClonableObject<T> *>(object)->cppPtr;
			*cppObject = new T(*cppPtr);
			(*cppObject)->setProperty("LinphonePrivate::Wrapper::cBackPtr", object);
		}
	}

	template<typename T>
	static T *getCppPtr (const std::shared_ptr<T> &cppPtr) {
		return cppPtr.get();
	}

	template<typename T>
	static T *getCppPtr (T *cppPtr) {
		return cppPtr;
	}

	template<typename T>
	static const T *getCppPtr (const std::shared_ptr<const T> &cppPtr) {
		return cppPtr.get();
	}

	template<typename T>
	static const T *getCppPtr (const T *cppPtr) {
		return cppPtr;
	}

	template<typename CType, typename CppType>
	static inline CType *getCBackPtr (const std::shared_ptr<CppType> &object, CType *(*cTypeAllocator)()) {
		Variant v = object->getProperty("LinphonePrivate::Wrapper::cBackPtr");
		void *value = v.getValue<void *>();
		if (!value) {
			CType *cObject = cTypeAllocator();
			setCppPtrFromC(cObject, object);
		}
		return reinterpret_cast<CType *>(value);
	}

	template<typename CType, typename CppType>
	static inline CType *getCBackPtr (const CppType *object, CType *(*cTypeAllocator)()) {
		Variant v = object->getProperty("LinphonePrivate::Wrapper::cBackPtr");
		void *value = v.getValue<void *>();
		if (!value) {
			CType *cObject = cTypeAllocator();
			setCppPtrFromC(cObject, object);
		}
		return reinterpret_cast<CType *>(value);
	}

	// ---------------------------------------------------------------------------

	template<typename T>
	static void *getUserData (const std::shared_ptr<T> &cppPtr) {
		Variant v = cppPtr->getProperty("LinphonePrivate::Wrapper::userData");
		return v.getValue<void *>();
	}

	template<typename T>
	static void *getUserData (T *cppPtr) {
		Variant v = cppPtr->getProperty("LinphonePrivate::Wrapper::userData");
		return v.getValue<void *>();
	}

	template<typename T>
	static inline void setUserData (const std::shared_ptr<T> &object, void *value) {
		L_ASSERT(object);
		object->setProperty("LinphonePrivate::Wrapper::userData", value);
	}

	template<typename T>
	static inline void setUserData (T *object, void *value) {
		L_ASSERT(object);
		object->setProperty("LinphonePrivate::Wrapper::userData", value);
	}

	// ---------------------------------------------------------------------------

	template<typename T>
	static inline bctbx_list_t *getCListFromCppList (const std::list<T> cppList) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, value);
		return result;
	}

	template<typename CppType, typename CType>
	static inline bctbx_list_t *getCListOfStructPtrFromCppListOfCppObj (const std::list<std::shared_ptr<CppType>> cppList, CType *(*cTypeAllocator)()) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, getCBackPtr(value, cTypeAllocator));
		return result;
	}

	template<typename CppType, typename CType>
	static inline bctbx_list_t *getCListOfStructPtrFromCppListOfCppObj (const std::list<CppType> cppList, CType *(*cTypeAllocator)()) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, getCBackPtr(value, cTypeAllocator));
		return result;
	}

	template<typename T>
	static inline std::list<T> getCppListFromCList (const bctbx_list_t *cList) {
		std::list<T> result;
		for (auto it = cList; it; it = bctbx_list_next(it))
			result.push_back(static_cast<T>(bctbx_list_get_data(it)));
		return result;
	}

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<Object, CppType>::value, CppType>::type
	>
	static inline std::list<std::shared_ptr<CppType>> getCppListOfCppObjFromCListOfStructPtr (const bctbx_list_t *cList) {
		std::list<std::shared_ptr<CppType>> result;
		for (auto it = cList; it; it = bctbx_list_next(it))
			result.push_back(getCppPtrFromC<CppType>(reinterpret_cast<CType *>(bctbx_list_get_data(it))));
		return result;
	}

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static inline std::list<CppType> getCppListOfCppObjFromCListOfStructPtr (const bctbx_list_t *cList) {
		std::list<CppType> result;
		for (auto it = cList; it; it = bctbx_list_next(it))
			result.push_back(*getCppPtrFromC<CppType>(reinterpret_cast<CType *>(bctbx_list_get_data(it))));
		return result;
	}

private:
	Wrapper ();

	L_DISABLE_COPY(Wrapper);
};

LINPHONE_END_NAMESPACE

#define L_INTERNAL_C_STRUCT_NO_XTOR(OBJECT)

#define L_INTERNAL_DECLARE_C_STRUCT_FUNCTIONS(CPP_CLASS, C_TYPE, CONSTRUCTOR, DESTRUCTOR) \
	BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## C_TYPE); \
	Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init() { \
		Linphone ## C_TYPE * object = belle_sip_object_new(Linphone ## C_TYPE); \
		new(&object->cppPtr) std::shared_ptr<LINPHONE_NAMESPACE::CPP_CLASS>(); \
		CONSTRUCTOR(object); \
		return object; \
	} \
	static void _linphone_ ## C_TYPE ## _uninit(Linphone ## C_TYPE * object) { \
		DESTRUCTOR(object); \
		object->cppPtr.~shared_ptr (); \
	} \
	BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## C_TYPE); \
	BELLE_SIP_INSTANCIATE_VPTR( \
		Linphone ## C_TYPE, \
		belle_sip_object_t, \
		_linphone_ ## C_TYPE ## _uninit, \
		NULL, \
		NULL, \
		FALSE \
	);

// =============================================================================
// Public Wrapper API.
// =============================================================================

// -----------------------------------------------------------------------------
// C object declaration.
// -----------------------------------------------------------------------------

// Declare wrapped C object with constructor/destructor.
#define L_DECLARE_C_STRUCT_IMPL_WITH_XTORS(CPP_CLASS, C_TYPE, CONSTRUCTOR, DESTRUCTOR, ...) \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		std::shared_ptr<LINPHONE_NAMESPACE::CPP_CLASS> cppPtr; \
		__VA_ARGS__ \
	}; \
	L_INTERNAL_DECLARE_C_STRUCT_FUNCTIONS(CPP_CLASS, C_TYPE, CONSTRUCTOR, DESTRUCTOR)

// Declare wrapped C object.
#define L_DECLARE_C_STRUCT_IMPL(CPP_CLASS, C_TYPE, ...) \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		std::shared_ptr<LINPHONE_NAMESPACE::CPP_CLASS> cppPtr; \
		__VA_ARGS__ \
	}; \
	L_INTERNAL_DECLARE_C_STRUCT_FUNCTIONS(CPP_CLASS, C_TYPE, L_INTERNAL_C_STRUCT_NO_XTOR, L_INTERNAL_C_STRUCT_NO_XTOR)

// Declare clonable wrapped C object.
#define L_DECLARE_C_CLONABLE_STRUCT_IMPL(CPP_CLASS, C_TYPE, ...) \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		LINPHONE_NAMESPACE::CPP_CLASS *cppPtr; \
		__VA_ARGS__ \
	}; \
	BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## C_TYPE); \
	Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init() { \
		return belle_sip_object_new(Linphone ## C_TYPE); \
	} \
	static void _linphone_ ## C_TYPE ## _uninit(Linphone ## C_TYPE * object) { \
		delete object->cppPtr; \
	} \
	static void _linphone_ ## C_TYPE ## _clone(Linphone ## C_TYPE * dest, const Linphone ## C_TYPE * src) { \
		L_ASSERT(src->cppPtr); \
		dest->cppPtr = new LINPHONE_NAMESPACE::CPP_CLASS(*src->cppPtr); \
	} \
	BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## C_TYPE); \
	BELLE_SIP_INSTANCIATE_VPTR(Linphone ## C_TYPE, belle_sip_object_t, \
	_linphone_ ## C_TYPE ## _uninit, \
	_linphone_ ## C_TYPE ## _clone, \
	NULL, \
	FALSE \
	);

#define L_DECLARE_C_STRUCT_NEW_DEFAULT(C_TYPE, C_NAME) \
	Linphone ## C_TYPE * linphone_ ## C_NAME ## _new() { \
		Linphone ## C_TYPE * object = _linphone_ ## C_TYPE ## _init(); \
		object->cppPtr = std::make_shared<LINPHONE_NAMESPACE::C_TYPE>(); \
		return object; \
	}

// -----------------------------------------------------------------------------
// Helpers.
// -----------------------------------------------------------------------------

// String conversions between C/C++.
#define L_STRING_TO_C(STR) ((STR).empty() ? NULL : (STR).c_str())
#define L_C_TO_STRING(STR) ((STR) == NULL ? std::string() : (STR))

// Get the cpp-ptr of a wrapped C object.
#define L_GET_CPP_PTR_FROM_C_STRUCT(OBJECT, CPP_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppPtrFromC< \
		LINPHONE_NAMESPACE::CPP_TYPE, \
		std::remove_pointer<decltype(OBJECT)>::type \
	>(OBJECT)

// Set the cpp-ptr of a wrapped C object.
#define L_SET_CPP_PTR_FROM_C_STRUCT(OBJECT, CPP_PTR) \
	LINPHONE_NAMESPACE::Wrapper::setCppPtrFromC(OBJECT, CPP_PTR)

// Get the private data of a shared or simple cpp-ptr.
#define L_GET_PRIVATE(OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getPrivate(OBJECT)

// Get the private data of a shared or simple cpp-ptr of a wrapped C object.
#define L_GET_PRIVATE_FROM_C_STRUCT(OBJECT, CPP_TYPE) \
	L_GET_PRIVATE(LINPHONE_NAMESPACE::Wrapper::getCppPtr( \
		L_GET_CPP_PTR_FROM_C_STRUCT(OBJECT, CPP_TYPE) \
	))

// Get the wrapped C object of a C++ object.
#define L_GET_C_BACK_PTR(OBJECT, C_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCBackPtr<Linphone ## C_TYPE>( \
		OBJECT, _linphone_ ## C_TYPE ## _init \
	)

// Get/set user data on a wrapped C object.
#define L_GET_USER_DATA_FROM_C_STRUCT(OBJECT, CPP_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getUserData( \
		L_GET_CPP_PTR_FROM_C_STRUCT(OBJECT, CPP_TYPE) \
	)
#define L_SET_USER_DATA_FROM_C_STRUCT(OBJECT, VALUE, CPP_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::setUserData( \
		L_GET_CPP_PTR_FROM_C_STRUCT(OBJECT, CPP_TYPE), \
		VALUE \
	)

#define L_GET_C_LIST_FROM_CPP_LIST(LIST, TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCListFromCppList<TYPE *>(LIST)
#define L_GET_CPP_LIST_FROM_C_LIST(LIST, TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppListFromCList<TYPE *>(LIST)
#define L_GET_C_LIST_OF_STRUCT_PTR_FROM_CPP_LIST_OF_CPP_OBJ(LIST, CPP_TYPE, C_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCListOfStructPtrFromCppListOfCppObj<LINPHONE_NAMESPACE::CPP_TYPE, Linphone ## C_TYPE>(LIST, _linphone_ ## C_TYPE ## _init)
#define L_GET_CPP_LIST_OF_CPP_OBJ_FROM_C_LIST_OF_STRUCT_PTR(LIST, CPP_TYPE, C_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppListOfCppObjFromCListOfStructPtr<LINPHONE_NAMESPACE::CPP_TYPE, Linphone ## C_TYPE>(LIST)

#endif // ifndef _C_TOOLS_H_
