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

#include "object/property-container.h"

// =============================================================================
// Internal.
// =============================================================================

LINPHONE_BEGIN_NAMESPACE

template<typename CppType>
struct CppTypeToCType {
	enum { defined = false };
};

template<typename CType>
struct CTypeToCppType {
	enum { defined = false };
};

template<typename CppType>
struct CObjectInitializer {};

class Wrapper {
private:
	template<typename CppType>
	struct IsCppObject {
		enum { value = std::is_base_of<Object, CppType>::value || std::is_base_of<ClonableObject, CppType>::value };
	};

	template<typename CType>
	struct WrappedObject {
		belle_sip_object_t base;
		std::shared_ptr<CType> cppPtr;
	};

	template<typename CType>
	struct WrappedClonableObject {
		belle_sip_object_t base;
		CType *cppPtr;
	};

public:
	// ---------------------------------------------------------------------------
	// Get private data of cpp Object.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename = typename std::enable_if<IsCppObject<CppType>::value, CppType>::type
	>
	static inline decltype (std::declval<CppType>().getPrivate()) getPrivate (CppType *cppObject) {
		L_ASSERT(cppObject);
		return cppObject->getPrivate();
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsCppObject<CppType>::value, CppType>::type
	>
	static inline decltype (std::declval<CppType>().getPrivate()) getPrivate (const std::shared_ptr<CppType> &cppObject) {
		L_ASSERT(cppObject);
		return cppObject->getPrivate();
	}

	// ---------------------------------------------------------------------------
	// Get c/cpp ptr helpers.
	// ---------------------------------------------------------------------------

	// Get Object.
	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<Object, CppType>::value, CppType>::type
	>
	static inline std::shared_ptr<CppType> getCppPtrFromC (CType *cObject) {
		L_ASSERT(cObject);
		return reinterpret_cast<WrappedObject<CppType> *>(cObject)->cppPtr;
	}

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<Object, CppType>::value, CppType>::type
	>
	static inline std::shared_ptr<const CppType> getCppPtrFromC (const CType *cObject) {
		L_ASSERT(cObject);
		return reinterpret_cast<const WrappedObject<CppType> *>(cObject)->cppPtr;
	}

	// Get ClonableObject.
	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static inline CppType *getCppPtrFromC (CType *cObject) {
		L_ASSERT(cObject);
		return reinterpret_cast<WrappedClonableObject<CppType> *>(cObject)->cppPtr;
	}

	template<
		typename CppType,
		typename CType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static inline const CppType *getCppPtrFromC (const CType *cObject) {
		L_ASSERT(cObject);
		return reinterpret_cast<const WrappedClonableObject<CppType> *>(cObject)->cppPtr;
	}

	// Set Object.
	template<
		typename CppType,
		typename = typename std::enable_if<std::is_base_of<Object, CppType>::value, CppType>::type
	>
	static inline void setCppPtrFromC (void *cObject, const std::shared_ptr<CppType> &cppObject) {
		L_ASSERT(cObject);
		static_cast<WrappedObject<CppType> *>(cObject)->cppPtr = cppObject;
		cppObject->setProperty("LinphonePrivate::Wrapper::cBackPtr", cObject);
	}

	// Set ClonableObject.
	template<
		typename CppType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static inline void setCppPtrFromC (void *cObject, const CppType *cppObject) {
		L_ASSERT(cObject);

		CppType **cppObjectAddr = &static_cast<WrappedClonableObject<CppType> *>(cObject)->cppPtr;
		if (*cppObjectAddr == cppObject)
			return;
		delete *cppObjectAddr;

		*cppObjectAddr = new CppType(*cppObject);
		(*cppObjectAddr)->setProperty("LinphonePrivate::Wrapper::cBackPtr", cObject);
	}

	// Macro helpers.
	template<typename T>
	static inline T *getCppPtr (const std::shared_ptr<T> &cppObject) {
		return cppObject.get();
	}

	template<typename T>
	static inline T *getCppPtr (T *cppObject) {
		return cppObject;
	}

	template<typename T>
	static inline const T *getCppPtr (const std::shared_ptr<const T> &cppObject) {
		return cppObject.get();
	}

	template<typename T>
	static inline const T *getCppPtr (const T *cppObject) {
		return cppObject;
	}

	// ---------------------------------------------------------------------------
	// Get c back ptr helpers.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename = typename std::enable_if<std::is_base_of<Object, CppType>::value, CppType>::type
	>
	static inline typename CppTypeToCType<CppType>::type *getCBackPtr (const std::shared_ptr<CppType> &cppObject) {
		typedef typename CppTypeToCType<CppType>::type RetType;

		Variant variant = cppObject->getProperty("LinphonePrivate::Wrapper::cBackPtr");
		void *value = variant.getValue<void *>();
		if (value)
			return reinterpret_cast<RetType *>(value);

		RetType *cObject = CObjectInitializer<CppType>::init();
		setCppPtrFromC(cObject, cppObject);
		return cObject;
	}

	template<
		typename CppType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static inline typename CppTypeToCType<CppType>::type *getCBackPtr (const CppType *cppObject) {
		typedef typename CppTypeToCType<CppType>::type RetType;

		Variant v = cppObject->getProperty("LinphonePrivate::Wrapper::cBackPtr");
		void *value = v.getValue<void *>();
		if (value)
			return reinterpret_cast<RetType *>(value);

		RetType *cObject = CObjectInitializer<CppType>::init();
		setCppPtrFromC(cObject, cppObject);
		return cObject;
	}

	// ---------------------------------------------------------------------------
	// Get/set user data.
	// ---------------------------------------------------------------------------

	static inline void *getUserData (const std::shared_ptr<const PropertyContainer> &propertyContainer) {
		L_ASSERT(propertyContainer);
		return propertyContainer->getProperty("LinphonePrivate::Wrapper::userData").getValue<void *>();
	}

	static inline void *getUserData (const PropertyContainer *propertyContainer) {
		L_ASSERT(propertyContainer);
		return propertyContainer->getProperty("LinphonePrivate::Wrapper::userData").getValue<void *>();
	}

	static inline void setUserData (const std::shared_ptr<PropertyContainer> &propertyContainer, void *value) {
		L_ASSERT(propertyContainer);
		propertyContainer->setProperty("LinphonePrivate::Wrapper::userData", value);
	}

	static inline void setUserData (PropertyContainer *propertyContainer, void *value) {
		L_ASSERT(propertyContainer);
		propertyContainer->setProperty("LinphonePrivate::Wrapper::userData", value);
	}

	// ---------------------------------------------------------------------------
	// List helpers.
	// ---------------------------------------------------------------------------

	template<typename T>
	static inline bctbx_list_t *getCListFromCppList (const std::list<T*> &cppList) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, value);
		return result;
	}

	template<typename T>
	static inline std::list<T*> getCppListFromCList (const bctbx_list_t *cList) {
		std::list<T> result;
		for (auto it = cList; it; it = bctbx_list_next(it))
			result.push_back(static_cast<T>(bctbx_list_get_data(it)));
		return result;
	}

	template<typename CppType, typename CType>
	static inline bctbx_list_t *getCListOfStructPtrFromCppListOfCppObj (const std::list<std::shared_ptr<CppType>> cppList, CType *(*cTypeAllocator)()) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, getCBackPtr(value));
		return result;
	}

	template<typename CppType, typename CType>
	static inline bctbx_list_t *getCListOfStructPtrFromCppListOfCppObj (const std::list<CppType> cppList, CType *(*cTypeAllocator)()) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, getCBackPtr(value));
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

#define L_INTERNAL_C_OBJECT_NO_XTOR(C_OBJECT)

#define L_INTERNAL_DECLARE_C_OBJECT_FUNCTIONS(C_TYPE, CONSTRUCTOR, DESTRUCTOR) \
	BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## C_TYPE); \
	Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init() { \
		Linphone ## C_TYPE * object = belle_sip_object_new(Linphone ## C_TYPE); \
		new(&object->cppPtr) std::shared_ptr<L_CPP_TYPE_OF_C_TYPE(C_TYPE)>(); \
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
// Register type.
// -----------------------------------------------------------------------------

#define L_REGISTER_TYPE(CPP_TYPE, C_TYPE) \
	extern Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init (); \
	LINPHONE_BEGIN_NAMESPACE \
	class CPP_TYPE; \
	template<> \
	struct CppTypeToCType<CPP_TYPE> { \
		enum { defined = true }; \
		typedef Linphone ## C_TYPE type; \
	}; \
	template<> \
	struct CTypeToCppType<Linphone ## C_TYPE> { \
		enum { defined = true }; \
		typedef CPP_TYPE type; \
	}; \
	template<> \
	struct CObjectInitializer<CPP_TYPE> { \
		static Linphone ## C_TYPE *init () { \
			return _linphone_ ## C_TYPE ## _init(); \
		} \
	}; \
	LINPHONE_END_NAMESPACE

#define L_ASSERT_C_TYPE(C_TYPE) \
	static_assert(LINPHONE_NAMESPACE::CTypeToCppType<Linphone ## C_TYPE>::defined, "Type is not defined."); \

#define L_CPP_TYPE_OF_C_TYPE(C_TYPE) \
	LINPHONE_NAMESPACE::CTypeToCppType<Linphone ## C_TYPE>::type

#define L_CPP_TYPE_OF_C_OBJECT(C_OBJECT) \
	LINPHONE_NAMESPACE::CTypeToCppType<std::remove_const<std::remove_pointer<decltype(C_OBJECT)>::type>::type>::type

// -----------------------------------------------------------------------------
// C object declaration.
// -----------------------------------------------------------------------------

// Declare wrapped C object with constructor/destructor.
#define L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(C_TYPE, CONSTRUCTOR, DESTRUCTOR, ...) \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		std::shared_ptr<L_CPP_TYPE_OF_C_TYPE(C_TYPE)> cppPtr; \
		__VA_ARGS__ \
	}; \
	L_INTERNAL_DECLARE_C_OBJECT_FUNCTIONS(C_TYPE, CONSTRUCTOR, DESTRUCTOR)

// Declare wrapped C object.
#define L_DECLARE_C_OBJECT_IMPL(C_TYPE, ...) \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		std::shared_ptr<L_CPP_TYPE_OF_C_TYPE(C_TYPE)> cppPtr; \
		__VA_ARGS__ \
	}; \
	L_INTERNAL_DECLARE_C_OBJECT_FUNCTIONS(C_TYPE, L_INTERNAL_C_OBJECT_NO_XTOR, L_INTERNAL_C_OBJECT_NO_XTOR)

// Declare clonable wrapped C object.
#define L_DECLARE_C_CLONABLE_STRUCT_IMPL(C_TYPE, ...) \
	L_ASSERT_C_TYPE(C_TYPE) \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		L_CPP_TYPE_OF_C_TYPE(C_TYPE) *cppPtr; \
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
		dest->cppPtr = new L_CPP_TYPE_OF_C_TYPE(C_TYPE)(*src->cppPtr); \
	} \
	BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## C_TYPE); \
	BELLE_SIP_INSTANCIATE_VPTR( \
		Linphone ## C_TYPE, belle_sip_object_t, \
		_linphone_ ## C_TYPE ## _uninit, \
		_linphone_ ## C_TYPE ## _clone, \
		NULL, \
		FALSE \
	);

#define L_DECLARE_C_OBJECT_NEW_DEFAULT(C_TYPE, C_NAME) \
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

// Call the init function of wrapped C object.
#define L_INIT(C_TYPE) _linphone_ ## C_TYPE ## _init ()

// Get/set the cpp-ptr of a wrapped C object.
#define L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getCppPtrFromC< \
		L_CPP_TYPE_OF_C_OBJECT(C_OBJECT), \
		std::remove_pointer<decltype(C_OBJECT)>::type \
	>(C_OBJECT)

// Set the cpp-ptr of a wrapped C object.
#define L_SET_CPP_PTR_FROM_C_OBJECT(C_OBJECT, CPP_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::setCppPtrFromC(C_OBJECT, CPP_OBJECT)

// Get the private data of a shared or simple cpp-ptr.
#define L_GET_PRIVATE(CPP_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getPrivate(CPP_OBJECT)

// Get the private data of a shared or simple cpp-ptr of a wrapped C object.
#define L_GET_PRIVATE_FROM_C_OBJECT(C_OBJECT) \
	L_GET_PRIVATE(LINPHONE_NAMESPACE::Wrapper::getCppPtr( \
		L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT) \
	))

// Get the wrapped C object of a C++ object.
#define L_GET_C_BACK_PTR(C_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getCBackPtr(C_OBJECT)

// Get/set user data on a wrapped C object.
#define L_GET_USER_DATA_FROM_C_OBJECT(C_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getUserData( \
		L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT) \
	)
#define L_SET_USER_DATA_FROM_C_OBJECT(C_OBJECT, VALUE) \
	LINPHONE_NAMESPACE::Wrapper::setUserData( \
		L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT), \
		VALUE \
	)

// Transforms cpp list and c list.
#define L_GET_C_LIST_FROM_CPP_LIST(CPP_LIST) \
	LINPHONE_NAMESPACE::Wrapper::getCListFromCppList(CPP_LIST)
#define L_GET_CPP_LIST_FROM_C_LIST(C_LIST, TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppListFromCList<TYPE>(C_LIST)

#define L_GET_C_LIST_OF_STRUCT_PTR_FROM_CPP_LIST_OF_CPP_OBJ(LIST, CPP_TYPE, C_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCListOfStructPtrFromCppListOfCppObj<LINPHONE_NAMESPACE::CPP_TYPE, Linphone ## C_TYPE>(LIST, _linphone_ ## C_TYPE ## _init)
#define L_GET_CPP_LIST_OF_CPP_OBJ_FROM_C_LIST_OF_STRUCT_PTR(LIST, CPP_TYPE, C_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppListOfCppObjFromCListOfStructPtr<LINPHONE_NAMESPACE::CPP_TYPE, Linphone ## C_TYPE>(LIST)

#endif // ifndef _C_TOOLS_H_
