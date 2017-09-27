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

#include "linphone/utils/utils.h"

// TODO: From coreapi. Remove me later.
#include "private.h"

#include "object/property-container.h"

// =============================================================================
// Internal.
// =============================================================================

LINPHONE_BEGIN_NAMESPACE

template<typename CppType>
struct CppTypeMetaInfo {
	enum {
		defined = false,
		isSubtype = false
	};
	typedef void cType;
};

template<typename CType>
struct CTypeMetaInfo {
	enum {
		defined = false
	};
	typedef void cppType;
};

class Wrapper {
private:
	template<typename CppType>
	struct IsCppObject {
		enum {
			value = std::is_base_of<Object, CppType>::value || std::is_base_of<ClonableObject, CppType>::value
		};
	};

	template<typename CppType>
	struct IsDefinedCppObject {
		enum {
			value = CppTypeMetaInfo<CppType>::defined && (
				!CppTypeMetaInfo<CppType>::isSubtype ||
				std::is_base_of<typename CTypeMetaInfo<typename CppTypeMetaInfo<CppType>::cType>::cppType, CppType>::value
			)
		};
	};

	template<typename CppType>
	struct IsDefinedNotClonableCppObject {
		enum {
			value = IsDefinedCppObject<CppType>::value && std::is_base_of<Object, CppType>::value
		};
	};

	template<typename CppType>
	struct IsDefinedClonableCppObject {
		enum {
			value = IsDefinedCppObject<CppType>::value && std::is_base_of<ClonableObject, CppType>::value
		};
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

	// ---------------------------------------------------------------------------
	// Runtime checker.
	// ---------------------------------------------------------------------------

	static inline void fatal (const char *message) {
		std::cout << "[FATAL C-WRAPPER]" << message << std::endl;
		exit(1);
	}

public:
	// ---------------------------------------------------------------------------
	// Get private data of cpp Object.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename = typename std::enable_if<IsCppObject<CppType>::value, CppType>::type
	>
	static constexpr decltype(std::declval<CppType>().getPrivate()) getPrivate (CppType *cppObject) {
		return cppObject->getPrivate();
	}

	// ---------------------------------------------------------------------------
	// Get c/cpp ptr helpers.
	// ---------------------------------------------------------------------------

	#ifdef DEBUG
		#define L_INTERNAL_WRAPPER_CONSTEXPR
	#else
		#define L_INTERNAL_WRAPPER_CONSTEXPR constexpr
	#endif

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedNotClonableCppObject<CppType>::value, CppType>::type
	>
	static L_INTERNAL_WRAPPER_CONSTEXPR std::shared_ptr<CppType> getCppPtrFromC (CType *cObject) {
		#ifdef DEBUG
			typedef typename CTypeMetaInfo<CType>::cppType BaseType;
			typedef CppType DerivedType;

			std::shared_ptr<BaseType> cppObject = reinterpret_cast<WrappedObject<BaseType> *>(cObject)->cppPtr;
			if (!cppObject)
				fatal("Cpp Object is null.");

			std::shared_ptr<DerivedType> derivedCppObject = std::static_pointer_cast<DerivedType>(cppObject);
			if (!derivedCppObject)
				fatal("Invalid derived cpp object.");

			return derivedCppObject;
		#else
			return reinterpret_cast<WrappedObject<CppType> *>(cObject)->cppPtr;
		#endif
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedNotClonableCppObject<CppType>::value, CppType>::type
	>
	static L_INTERNAL_WRAPPER_CONSTEXPR std::shared_ptr<const CppType> getCppPtrFromC (const CType *cObject) {
		#ifdef DEBUG
			return getCppPtrFromC(const_cast<CType *>(cObject));
		#else
			return reinterpret_cast<const WrappedObject<CppType> *>(cObject)->cppPtr;
		#endif
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static L_INTERNAL_WRAPPER_CONSTEXPR CppType *getCppPtrFromC (CType *cObject) {
		#ifdef DEBUG
			typedef typename CTypeMetaInfo<CType>::cppType BaseType;
			typedef CppType DerivedType;

			BaseType *cppObject = reinterpret_cast<WrappedClonableObject<BaseType> *>(cObject)->cppPtr;
			if (!cppObject)
				fatal("Cpp Object is null.");

			DerivedType *derivedCppObject = dynamic_cast<DerivedType *>(cppObject);
			if (!derivedCppObject)
				fatal("Invalid derived cpp object.");

			return derivedCppObject;
		#else
			return reinterpret_cast<WrappedClonableObject<CppType> *>(cObject)->cppPtr;
		#endif
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static L_INTERNAL_WRAPPER_CONSTEXPR const CppType *getCppPtrFromC (const CType *cObject) {
		#ifdef DEBUG
			return getCppPtrFromC(const_cast<CType *>(cObject));
		#else
			return reinterpret_cast<const WrappedClonableObject<CppType> *>(cObject)->cppPtr;
		#endif
	}

	#undef L_INTERNAL_WRAPPER_CONSTEXPR

	// ---------------------------------------------------------------------------
	// Set c/cpp ptr helpers.
	// ---------------------------------------------------------------------------

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedNotClonableCppObject<CppType>::value, CppType>::type
	>
	static inline void setCppPtrFromC (CType *cObject, const std::shared_ptr<CppType> &cppObject) {
		reinterpret_cast<WrappedObject<CppType> *>(cObject)->cppPtr = cppObject;
		cppObject->setProperty("LinphonePrivate::Wrapper::cBackPtr", cObject);
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static inline void setCppPtrFromC (CType *cObject, const CppType *cppObject) {
		CppType **cppObjectAddr = &reinterpret_cast<WrappedClonableObject<CppType> *>(cObject)->cppPtr;
		if (*cppObjectAddr == cppObject)
			return;
		delete *cppObjectAddr;

		*cppObjectAddr = new CppType(*cppObject);
		(*cppObjectAddr)->setProperty("LinphonePrivate::Wrapper::cBackPtr", cObject);
	}

	// ---------------------------------------------------------------------------
	// Get c back ptr helpers.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedNotClonableCppObject<CppType>::value, CppType>::type
	>
	static inline typename CppTypeMetaInfo<CppType>::cType *getCBackPtr (const std::shared_ptr<CppType> &cppObject) {
		typedef typename CppTypeMetaInfo<CppType>::cType RetType;

		Variant variant = cppObject->getProperty("LinphonePrivate::Wrapper::cBackPtr");
		void *value = variant.getValue<void *>();
		if (value)
			return static_cast<RetType *>(value);

		RetType *cObject = CppTypeMetaInfo<CppType>::init();
		setCppPtrFromC(cObject, cppObject);
		return cObject;
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedNotClonableCppObject<CppType>::value, CppType>::type
	>
	static inline typename CppTypeMetaInfo<CppType>::cType *getCBackPtr (CppType *cppObject) {
		try {
			return getCBackPtr(std::static_pointer_cast<CppType>(cppObject->shared_from_this()));
		} catch (const std::bad_weak_ptr &e) {
			fatal(e.what());
		}

		L_ASSERT(false);
		return nullptr;
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static inline typename CppTypeMetaInfo<CppType>::cType *getCBackPtr (const CppType *cppObject) {
		typedef typename CppTypeMetaInfo<CppType>::cType RetType;

		Variant variant = cppObject->getProperty("LinphonePrivate::Wrapper::cBackPtr");
		void *value = variant.getValue<void *>();
		if (value)
			return static_cast<RetType *>(value);

		RetType *cObject = CppTypeMetaInfo<CppType>::init();
		setCppPtrFromC(cObject, cppObject);
		return cObject;
	}

	// ---------------------------------------------------------------------------
	// Get/set user data.
	// ---------------------------------------------------------------------------

	static inline void *getUserData (const PropertyContainer *propertyContainer) {
		L_ASSERT(propertyContainer);
		return propertyContainer->getProperty("LinphonePrivate::Wrapper::userData").getValue<void *>();
	}

	static inline void setUserData (PropertyContainer *propertyContainer, void *value) {
		L_ASSERT(propertyContainer);
		propertyContainer->setProperty("LinphonePrivate::Wrapper::userData", value);
	}

	// ---------------------------------------------------------------------------
	// List conversions.
	// ---------------------------------------------------------------------------

	template<typename T>
	static inline bctbx_list_t *getCListFromCppList (const std::list<T *> &cppList) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, value);
		return result;
	}

	template<typename T>
	static inline std::list<T *> getCppListFromCList (const bctbx_list_t *cList) {
		std::list<T> result;
		for (auto it = cList; it; it = bctbx_list_next(it))
			result.push_back(static_cast<T>(bctbx_list_get_data(it)));
		return result;
	}

	// ---------------------------------------------------------------------------
	// Resolved list conversions.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedNotClonableCppObject<CppType>::value, CppType>::type
	>
	static inline bctbx_list_t *getResolvedCListFromCppList (const std::list<std::shared_ptr<CppType>> &cppList) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, getCBackPtr(value));
		return result;
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static inline bctbx_list_t *getResolvedCListFromCppList (const std::list<CppType> &cppList) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, getCBackPtr(value));
		return result;
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedNotClonableCppObject<CppType>::value, CppType>::type
	>
	static inline std::list<std::shared_ptr<CppType>> getResolvedCppListFromCList (const bctbx_list_t *cList) {
		std::list<std::shared_ptr<CppType>> result;
		for (auto it = cList; it; it = bctbx_list_next(it))
			result.push_back(getCppPtrFromC(static_cast<CType *>(bctbx_list_get_data(it))));
		return result;
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static inline std::list<CppType> getResolvedCppListFromCList (const bctbx_list_t *cList) {
		std::list<CppType> result;
		for (auto it = cList; it; it = bctbx_list_next(it))
			result.push_back(*getCppPtrFromC(static_cast<CType *>(bctbx_list_get_data(it))));
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
	Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init () { \
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
	struct CppTypeMetaInfo<CPP_TYPE> { \
		enum { \
			defined = true, \
			isSubtype = false \
		}; \
		typedef Linphone ## C_TYPE cType; \
		static inline Linphone ## C_TYPE *init () { \
			return _linphone_ ## C_TYPE ## _init(); \
		} \
	}; \
	template<> \
	struct CTypeMetaInfo<Linphone ## C_TYPE> { \
		enum { defined = true }; \
		typedef CPP_TYPE cppType; \
	}; \
	LINPHONE_END_NAMESPACE

#define L_REGISTER_SUBTYPE(CPP_TYPE, CPP_SUBTYPE) \
	LINPHONE_BEGIN_NAMESPACE \
	class CPP_SUBTYPE; \
	static_assert(CppTypeMetaInfo<CPP_TYPE>::defined, "Base type is not defined"); \
	template<> \
	struct CppTypeMetaInfo<CPP_SUBTYPE> { \
		enum { \
			defined = true, \
			isSubtype = true \
		}; \
		typedef CppTypeMetaInfo<CPP_TYPE>::cType cType; \
		static inline typename CppTypeMetaInfo<CPP_TYPE>::cType *init () { \
			return CppTypeMetaInfo<CPP_TYPE>::init(); \
		} \
	}; \
	LINPHONE_END_NAMESPACE

#define L_CPP_TYPE_OF_C_TYPE(C_TYPE) \
	LINPHONE_NAMESPACE::CTypeMetaInfo<Linphone ## C_TYPE>::cppType

#define L_CPP_TYPE_OF_C_OBJECT(C_OBJECT) \
	LINPHONE_NAMESPACE::CTypeMetaInfo<std::remove_const<std::remove_pointer<decltype(C_OBJECT)>::type>::type>::cppType

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
	static_assert(LINPHONE_NAMESPACE::CTypeMetaInfo<Linphone ## C_TYPE>::defined, "Type is not defined."); \
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
		Linphone ## C_TYPE *object = _linphone_ ## C_TYPE ## _init(); \
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
#define L_INIT(C_TYPE) _linphone_ ## C_TYPE ## _init()

// Get/set the cpp-ptr of a wrapped C object.
#define L_GET_CPP_PTR_FROM_C_OBJECT_1_ARGS(C_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getCppPtrFromC(C_OBJECT)
#define L_GET_CPP_PTR_FROM_C_OBJECT_2_ARGS(C_OBJECT, CPP_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppPtrFromC< \
		std::remove_pointer<decltype(C_OBJECT)>::type, \
		LINPHONE_NAMESPACE::CPP_TYPE \
	>(C_OBJECT)

#define L_GET_CPP_PTR_FROM_C_OBJECT_MACRO_CHOOSER(...) \
	L_GET_ARG_3(__VA_ARGS__, L_GET_CPP_PTR_FROM_C_OBJECT_2_ARGS, L_GET_CPP_PTR_FROM_C_OBJECT_1_ARGS)

#define L_GET_CPP_PTR_FROM_C_OBJECT(...) \
	L_GET_CPP_PTR_FROM_C_OBJECT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

// Set the cpp-ptr of a wrapped C object.
#define L_SET_CPP_PTR_FROM_C_OBJECT(C_OBJECT, CPP_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::setCppPtrFromC(C_OBJECT, CPP_OBJECT)

// Get the private data of a shared or simple cpp-ptr.
#define L_GET_PRIVATE(CPP_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getPrivate(LINPHONE_NAMESPACE::Utils::getPtr(CPP_OBJECT))

// Get the private data of a shared or simple cpp-ptr of a wrapped C object.
#define L_GET_PRIVATE_FROM_C_OBJECT_1_ARGS(C_OBJECT) \
	L_GET_PRIVATE(LINPHONE_NAMESPACE::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT)))
#define L_GET_PRIVATE_FROM_C_OBJECT_2_ARGS(C_OBJECT, CPP_TYPE) \
	L_GET_PRIVATE(LINPHONE_NAMESPACE::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT, CPP_TYPE)))

#define L_GET_PRIVATE_FROM_C_OBJECT_MACRO_CHOOSER(...) \
	L_GET_ARG_3(__VA_ARGS__, L_GET_PRIVATE_FROM_C_OBJECT_2_ARGS, L_GET_PRIVATE_FROM_C_OBJECT_1_ARGS)

#define L_GET_PRIVATE_FROM_C_OBJECT(...) \
	L_GET_PRIVATE_FROM_C_OBJECT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

// Get the wrapped C object of a C++ object.
#define L_GET_C_BACK_PTR(CPP_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getCBackPtr(CPP_OBJECT)

// Get/set user data on a wrapped C object.
#define L_GET_USER_DATA_FROM_C_OBJECT(C_OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getUserData( \
		LINPHONE_NAMESPACE::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT)) \
	)
#define L_SET_USER_DATA_FROM_C_OBJECT(C_OBJECT, VALUE) \
	LINPHONE_NAMESPACE::Wrapper::setUserData( \
		LINPHONE_NAMESPACE::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT)), \
		VALUE \
	)

// Transforms cpp list and c list.
#define L_GET_C_LIST_FROM_CPP_LIST(CPP_LIST) \
	LINPHONE_NAMESPACE::Wrapper::getCListFromCppList(CPP_LIST)
#define L_GET_CPP_LIST_FROM_C_LIST(C_LIST, TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getCppListFromCList<TYPE>(C_LIST)

// Transforms cpp list and c list and convert cpp object to c object.
#define L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(CPP_LIST) \
	LINPHONE_NAMESPACE::Wrapper::getResolvedCListFromCppList(CPP_LIST)
#define L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(C_LIST, C_TYPE) \
	LINPHONE_NAMESPACE::Wrapper::getResolvedCppListFromCList<Linphone ## C_TYPE>(C_LIST)

#endif // ifndef _C_TOOLS_H_
