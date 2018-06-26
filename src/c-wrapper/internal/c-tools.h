/*
 * c-tools.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _L_C_TOOLS_H_
#define _L_C_TOOLS_H_

#include <list>

#include <belle-sip/types.h>

#include "linphone/utils/utils.h"

#include "object/property-container.h"

// =============================================================================
// Internal.
// =============================================================================

#ifdef DEBUG
	#define L_INTERNAL_WRAPPER_CONSTEXPR
#else
	#define L_INTERNAL_WRAPPER_CONSTEXPR constexpr
#endif

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// MetaInfo.
// -----------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// IsCppObject traits.
// ---------------------------------------------------------------------------

template<typename CppType>
struct IsCppObject {
	enum {
		value = std::is_base_of<BaseObject, CppType>::value || std::is_base_of<ClonableObject, CppType>::value
	};
};

template<typename CppPrivateType>
struct IsPrivateCppObject {
	enum {
		value = std::is_base_of<BaseObjectPrivate, CppPrivateType>::value ||
		std::is_base_of<ClonableObjectPrivate, CppPrivateType>::value
	};
};

template<typename CppType>
struct IsRegisteredCppObject {
	enum {
		value = CppTypeMetaInfo<CppType>::defined && (
			!CppTypeMetaInfo<CppType>::isSubtype ||
			std::is_base_of<typename CTypeMetaInfo<typename CppTypeMetaInfo<CppType>::cType>::cppType, CppType>::value
		)
	};
};

// ---------------------------------------------------------------------------
// IsDefined traits.
// ---------------------------------------------------------------------------

template<typename CppType>
struct IsDefinedBaseCppObject {
	enum {
		value = IsRegisteredCppObject<CppType>::value && std::is_base_of<BaseObject, CppType>::value
	};
};

template<typename CppType>
struct IsDefinedCppObject {
	enum {
		value = IsDefinedBaseCppObject<CppType>::value && std::is_base_of<Object, CppType>::value
	};
};

template<typename CppType>
struct IsDefinedClonableCppObject {
	enum {
		value = IsRegisteredCppObject<CppType>::value && std::is_base_of<ClonableObject, CppType>::value
	};
};

class Wrapper {
private:
	// ---------------------------------------------------------------------------
	// Wrapped Objects.
	// ---------------------------------------------------------------------------

	enum class WrappedObjectOwner : int {
		External,
		Internal
	};

	template<typename CppType>
	struct WrappedBaseObject {
		belle_sip_object_t base;
		std::shared_ptr<CppType> cppPtr;
		std::weak_ptr<CppType> weakCppPtr;

		// By default: External.
		WrappedObjectOwner owner;
	};

	template<typename CppType>
	struct WrappedClonableObject {
		belle_sip_object_t base;
		CppType *cppPtr;

		// By default: External.
		WrappedObjectOwner owner;
	};

	// ---------------------------------------------------------------------------
	// Debug tools.
	// ---------------------------------------------------------------------------

	#ifdef DEBUG
		static void setName (belle_sip_object_t *cObject, const BaseObject *cppObject);
		static void setName (belle_sip_object_t *cObject, const ClonableObject *cppObject);
	#endif

	// ---------------------------------------------------------------------------
	// Runtime checker.
	// ---------------------------------------------------------------------------

	[[noreturn]] static inline void abort (const char *message) {
		std::cerr << "[FATAL C-WRAPPER]" << message << std::endl;
		std::terminate();
	}

	// ---------------------------------------------------------------------------
	// Get cpp ptr (shared if BaseObject, not shared if ClonableObject)
	// from cpp object.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static inline std::shared_ptr<CppType> getResolvedCppPtr (const CppType *cppObject) {
		if (L_UNLIKELY(!cppObject))
			return nullptr;

		try {
			typedef typename std::decay<decltype(*cppObject->getSharedFromThis())>::type SharedFromThisType;

			return std::static_pointer_cast<CppType>(
				std::const_pointer_cast<SharedFromThisType>(cppObject->getSharedFromThis())
			);
		} catch (const std::bad_weak_ptr &e) {
			abort(e.what());
		}

		L_ASSERT(false);
		return nullptr;
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static constexpr const CppType *getResolvedCppPtr (const CppType *cppObject) {
		return cppObject;
	}

	// ---------------------------------------------------------------------------
	// Casts.
	// ---------------------------------------------------------------------------

	template<
		typename CppDerivedPrivateType,
		typename CppBasePrivateType,
		typename = typename std::enable_if<IsPrivateCppObject<CppDerivedPrivateType>::value, CppDerivedPrivateType>::type
	>
	static L_INTERNAL_WRAPPER_CONSTEXPR CppDerivedPrivateType *cast (CppBasePrivateType *base) {
		#ifdef DEBUG
			if (!base)
				return static_cast<CppDerivedPrivateType *>(base);

			CppDerivedPrivateType *derived = dynamic_cast<CppDerivedPrivateType *>(base);
			if (!derived)
				abort("Invalid cast.");
			return derived;
		#else
			return static_cast<CppDerivedPrivateType *>(base);
		#endif
	}

public:
	// ---------------------------------------------------------------------------
	// Back ptr reset, used by uninit c object function.
	// ---------------------------------------------------------------------------

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static void uninitBaseCppObject (CType *cObject) {
		WrappedBaseObject<CppType> *wrappedObject = reinterpret_cast<WrappedBaseObject<CppType> *>(cObject);

		std::shared_ptr<CppType> cppObject = wrappedObject->owner == WrappedObjectOwner::Internal
			? wrappedObject->weakCppPtr.lock()
			: wrappedObject->cppPtr;

		if (cppObject)
			cppObject->setCBackPtr(nullptr);

		wrappedObject->cppPtr.~shared_ptr();
		wrappedObject->weakCppPtr.~weak_ptr();
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static void uninitClonableCppObject (CType *cObject) {
		WrappedClonableObject<CppType> *wrappedObject = reinterpret_cast<WrappedClonableObject<CppType> *>(cObject);
		if (wrappedObject->owner == WrappedObjectOwner::External)
			delete wrappedObject->cppPtr;
	}

	// ---------------------------------------------------------------------------
	// Belle sip handlers.
	// Deal with floating references.
	// ---------------------------------------------------------------------------

	static void onBelleSipFirstRef (belle_sip_object_t *base) {
		WrappedBaseObject<BaseObject> *wrappedObject = reinterpret_cast<WrappedBaseObject<BaseObject> *>(base);
		if (wrappedObject->owner == WrappedObjectOwner::Internal)
			wrappedObject->cppPtr = wrappedObject->weakCppPtr.lock();
	}

	static void onBelleSipLastRef (belle_sip_object_t *base) {
		WrappedBaseObject<BaseObject> *wrappedObject = reinterpret_cast<WrappedBaseObject<BaseObject> *>(base);
		if (wrappedObject->owner == WrappedObjectOwner::Internal)
			wrappedObject->cppPtr.reset();
	}

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
	// Deal with cpp ptr destruction.
	// ---------------------------------------------------------------------------

	template<
		typename CppType,
		typename = typename std::enable_if<std::is_base_of<BaseObject, CppType>::value, CppType>::type
	>
	static void handleObjectDestruction (CppType *cppObject) {
		void *value = cppObject->getCBackPtr();
		if (value && static_cast<WrappedBaseObject<CppType> *>(value)->owner == WrappedObjectOwner::Internal)
			belle_sip_object_unref(value);
	}

	template<
		typename CppType,
		typename = typename std::enable_if<std::is_base_of<ClonableObject, CppType>::value, CppType>::type
	>
	static void handleClonableObjectDestruction (CppType *cppObject) {
		void *value = cppObject->getCBackPtr();
		if (value && static_cast<WrappedClonableObject<CppType> *>(value)->owner == WrappedObjectOwner::Internal)
			belle_sip_object_unref(value);
	}

	// ---------------------------------------------------------------------------
	// Get c/cpp ptr helpers.
	// ---------------------------------------------------------------------------

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static inline std::shared_ptr<CppType> getCppPtrFromC (CType *cObject) {
		#ifdef DEBUG
			typedef typename CTypeMetaInfo<CType>::cppType BaseType;
			typedef CppType DerivedType;

			std::shared_ptr<BaseType> cppObject;
			{
				WrappedBaseObject<BaseType> *wrappedObject = reinterpret_cast<WrappedBaseObject<BaseType> *>(cObject);
				cppObject = wrappedObject->owner == WrappedObjectOwner::Internal
					? wrappedObject->weakCppPtr.lock()
					: wrappedObject->cppPtr;
			}

			if (!cppObject)
				abort("Cpp Object is null.");

			std::shared_ptr<DerivedType> derivedCppObject = std::static_pointer_cast<DerivedType>(cppObject);
			if (!derivedCppObject)
				abort("Invalid derived cpp object.");

			return derivedCppObject;
		#else
			WrappedBaseObject<CppType> *wrappedObject = reinterpret_cast<WrappedBaseObject<CppType> *>(cObject);
			return wrappedObject->owner == WrappedObjectOwner::Internal
				? wrappedObject->weakCppPtr.lock()
				: wrappedObject->cppPtr;
		#endif
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static inline std::shared_ptr<const CppType> getCppPtrFromC (const CType *cObject) {
		return getCppPtrFromC<typename std::remove_const<CType>::type, CppType>(const_cast<CType *>(cObject));
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
				abort("Cpp Object is null.");

			DerivedType *derivedCppObject = dynamic_cast<DerivedType *>(cppObject);
			if (!derivedCppObject)
				abort("Invalid derived cpp object.");

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
		return getCppPtrFromC(const_cast<CType *>(cObject));
	}

	// ---------------------------------------------------------------------------
	// Set c/cpp ptr helpers.
	// ---------------------------------------------------------------------------

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static inline void setCppPtrFromC (CType *cObject, const std::shared_ptr<CppType> &cppObject) {
		WrappedBaseObject<CppType> *wrappedObject = reinterpret_cast<WrappedBaseObject<CppType> *>(cObject);
		std::shared_ptr<CppType> oldCppObject;

		if (wrappedObject->owner == WrappedObjectOwner::Internal) {
			oldCppObject = wrappedObject->weakCppPtr.lock();
			wrappedObject->weakCppPtr = cppObject;
			if (reinterpret_cast<belle_sip_object_t *>(cObject)->ref > 1)
				wrappedObject->cppPtr = cppObject;
			else
				wrappedObject->cppPtr.reset();
		} else {
			oldCppObject = wrappedObject->cppPtr;
			wrappedObject->cppPtr = cppObject;
		}

		if (oldCppObject)
			oldCppObject->setCBackPtr(nullptr);
		cppObject->setCBackPtr(cObject);

		#ifdef DEBUG
			setName(reinterpret_cast<belle_sip_object_t *>(cObject), cppObject.get());
		#endif
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static inline void setCppPtrFromC (CType *cObject, CppType *cppObject) {
		CppType **cppObjectAddr = &reinterpret_cast<WrappedClonableObject<CppType> *>(cObject)->cppPtr;
		if (*cppObjectAddr == cppObject)
			return;

		if (reinterpret_cast<WrappedClonableObject<CppType> *>(cObject)->owner == WrappedObjectOwner::External)
			delete *cppObjectAddr;

		*cppObjectAddr = cppObject;
		(*cppObjectAddr)->setCBackPtr(cObject);

		#ifdef DEBUG
			setName(reinterpret_cast<belle_sip_object_t *>(cObject), cppObject);
		#endif
	}

	// ---------------------------------------------------------------------------
	// Get c back ptr resolver helpers.
	// ---------------------------------------------------------------------------

private:
	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static inline typename CppTypeMetaInfo<CppType>::cType *getCBackPtrResolver (const std::shared_ptr<CppType> &cppObject) {
		return getCBackPtr(cppObject);
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static inline typename CppTypeMetaInfo<CppType>::cType *getCBackPtrResolver (const CppType *cppObject) {
		if (L_UNLIKELY(!cppObject))
			return nullptr;

		typedef typename CppTypeMetaInfo<CppType>::cType RetType;

		void *value = cppObject->getCBackPtr();
		if (value)
			return static_cast<RetType *>(value);

		RetType *cObject = CppTypeMetaInfo<CppType>::init();
		reinterpret_cast<WrappedClonableObject<CppType> *>(cObject)->owner = WrappedObjectOwner::Internal;
		setCppPtrFromC(cObject, const_cast<CppType *>(cppObject));

		return cObject;
	}

	// ---------------------------------------------------------------------------
	// Get c back ptr helpers.
	// ---------------------------------------------------------------------------

public:
	template<
		typename CppType,
		typename = typename std::enable_if<
			IsDefinedCppObject<CppType>::value || IsDefinedClonableCppObject<CppType>::value, CppType
		>::type
	>
	static inline typename CppTypeMetaInfo<CppType>::cType *getCBackPtr (const CppType *cppObject) {
		return getCBackPtrResolver(getResolvedCppPtr(cppObject));
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static inline typename CppTypeMetaInfo<CppType>::cType *getCBackPtr (const std::shared_ptr<CppType> &cppObject) {
		if (L_UNLIKELY(!cppObject))
			return nullptr;

		typedef typename CppTypeMetaInfo<CppType>::cType RetType;

		void *value = cppObject->getCBackPtr();
		if (value)
			return static_cast<RetType *>(value);

		RetType *cObject = CppTypeMetaInfo<CppType>::init();
		reinterpret_cast<WrappedBaseObject<CppType> *>(cObject)->owner = WrappedObjectOwner::Internal;
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
		typename = typename std::enable_if<IsDefinedBaseCppObject<CppType>::value, CppType>::type
	>
	static inline bctbx_list_t *getResolvedCListFromCppList (const std::list<std::shared_ptr<CppType>> &cppList) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList)
			result = bctbx_list_append(result, belle_sip_object_ref(getCBackPtr(value)));
		return result;
	}

	template<
		typename CppType,
		typename = typename std::enable_if<IsDefinedClonableCppObject<CppType>::value, CppType>::type
	>
	static inline bctbx_list_t *getResolvedCListFromCppList (const std::list<CppType> &cppList) {
		bctbx_list_t *result = nullptr;
		for (const auto &value : cppList) {
			auto cValue = getCBackPtr(new CppType(value));
			reinterpret_cast<WrappedClonableObject<CppType> *>(cValue)->owner = WrappedObjectOwner::External;
			result = bctbx_list_append(result, cValue);
		}
		return result;
	}

	template<
		typename CType,
		typename CppType = typename CTypeMetaInfo<CType>::cppType,
		typename = typename std::enable_if<IsDefinedCppObject<CppType>::value, CppType>::type
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

#undef L_INTERNAL_WRAPPER_CONSTEXPR

#define L_INTERNAL_C_NO_XTOR(C_OBJECT)
#define L_INTERNAL_C_NO_CLONE_C(C_DEST_OBJECT, C_SRC_OBJECT)

#define L_INTERNAL_DECLARE_C_OBJECT(C_TYPE, ...) \
	static_assert(LinphonePrivate::CTypeMetaInfo<Linphone ## C_TYPE>::defined, "Type is not defined."); \
	static_assert( \
		LinphonePrivate::IsDefinedBaseCppObject< \
			LinphonePrivate::CTypeMetaInfo<Linphone ## C_TYPE>::cppType \
		>::value, \
		"Type is not declared as base object." \
	); \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		std::shared_ptr<L_CPP_TYPE_OF_C_TYPE(C_TYPE)> cppPtr; \
		std::weak_ptr<L_CPP_TYPE_OF_C_TYPE(C_TYPE)> weakCppPtr; \
		int owner; \
		__VA_ARGS__ \
	};

#define L_INTERNAL_DECLARE_C_OBJECT_FUNCTIONS(C_TYPE, CONSTRUCTOR, DESTRUCTOR) \
	BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## C_TYPE); \
	Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init () { \
		Linphone ## C_TYPE *object = belle_sip_object_new(Linphone ## C_TYPE); \
		new(&object->cppPtr) std::shared_ptr<L_CPP_TYPE_OF_C_TYPE(C_TYPE)>(); \
		new(&object->weakCppPtr) std::weak_ptr<L_CPP_TYPE_OF_C_TYPE(C_TYPE)>(); \
		CONSTRUCTOR(object); \
		return object; \
	} \
	static void _linphone_ ## C_TYPE ## _uninit (Linphone ## C_TYPE *object) { \
		DESTRUCTOR(object); \
		LinphonePrivate::Wrapper::uninitBaseCppObject(object); \
	} \
	BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## C_TYPE); \
	BELLE_SIP_INSTANCIATE_VPTR2( \
		Linphone ## C_TYPE, \
		belle_sip_object_t, \
		_linphone_ ## C_TYPE ## _uninit, \
		NULL, \
		NULL, \
		LinphonePrivate::Wrapper::onBelleSipFirstRef, \
		LinphonePrivate::Wrapper::onBelleSipLastRef, \
		FALSE \
	);

#define L_INTERNAL_DECLARE_C_CLONABLE_OBJECT(C_TYPE, ...) \
	static_assert(LinphonePrivate::CTypeMetaInfo<Linphone ## C_TYPE>::defined, "Type is not defined."); \
	static_assert( \
		LinphonePrivate::IsDefinedClonableCppObject< \
			LinphonePrivate::CTypeMetaInfo<Linphone ## C_TYPE>::cppType \
		>::value, \
		"Type is not declared as clonable object." \
	); \
	struct _Linphone ## C_TYPE { \
		belle_sip_object_t base; \
		L_CPP_TYPE_OF_C_TYPE(C_TYPE) *cppPtr; \
		int owner; \
		__VA_ARGS__ \
	}; \

#define L_INTERNAL_DECLARE_C_CLONABLE_OBJECT_FUNCTIONS(C_TYPE, CONSTRUCTOR, DESTRUCTOR, CLONE_C) \
	BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## C_TYPE); \
	Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init () { \
		Linphone ## C_TYPE *object = belle_sip_object_new(Linphone ## C_TYPE); \
		CONSTRUCTOR(object); \
		return object; \
	} \
	static void _linphone_ ## C_TYPE ## _uninit (Linphone ## C_TYPE * object) { \
		DESTRUCTOR(object); \
		LinphonePrivate::Wrapper::uninitClonableCppObject(object); \
	} \
	static void _linphone_ ## C_TYPE ## _clone (Linphone ## C_TYPE *dest, const Linphone ## C_TYPE *src) { \
		L_ASSERT(src->cppPtr); \
		dest->cppPtr = new L_CPP_TYPE_OF_C_TYPE(C_TYPE)(*src->cppPtr); \
		CLONE_C(dest, src); \
	} \
	BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## C_TYPE); \
	BELLE_SIP_INSTANCIATE_VPTR( \
		Linphone ## C_TYPE, belle_sip_object_t, \
		_linphone_ ## C_TYPE ## _uninit, \
		_linphone_ ## C_TYPE ## _clone, \
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
	LinphonePrivate::CTypeMetaInfo<Linphone ## C_TYPE>::cppType

#define L_CPP_TYPE_OF_C_OBJECT(C_OBJECT) \
	LinphonePrivate::CTypeMetaInfo<std::remove_const<std::remove_pointer<decltype(C_OBJECT)>::type>::type>::cppType

// -----------------------------------------------------------------------------
// C object declaration.
// -----------------------------------------------------------------------------

// Declare wrapped C object with constructor/destructor.
#define L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(C_TYPE, CONSTRUCTOR, DESTRUCTOR, ...) \
	L_INTERNAL_DECLARE_C_OBJECT(C_TYPE, __VA_ARGS__) \
	L_INTERNAL_DECLARE_C_OBJECT_FUNCTIONS(C_TYPE, CONSTRUCTOR, DESTRUCTOR)

// Declare wrapped C object.
#define L_DECLARE_C_OBJECT_IMPL(C_TYPE, ...) \
	L_INTERNAL_DECLARE_C_OBJECT(C_TYPE, __VA_ARGS__) \
	L_INTERNAL_DECLARE_C_OBJECT_FUNCTIONS(C_TYPE, L_INTERNAL_C_NO_XTOR, L_INTERNAL_C_NO_XTOR)

// Declare clonable wrapped C object with constructor/destructor and clone_c.
#define L_DECLARE_C_CLONABLE_OBJECT_IMPL_WITH_XTORS(C_TYPE, CONSTRUCTOR, DESTRUCTOR, CLONE_C, ...) \
	L_INTERNAL_DECLARE_C_CLONABLE_OBJECT(C_TYPE, __VA_ARGS__) \
	L_INTERNAL_DECLARE_C_CLONABLE_OBJECT_FUNCTIONS(C_TYPE, CONSTRUCTOR, DESTRUCTOR, CLONE_C)

// Declare clonable wrapped C object.
#define L_DECLARE_C_CLONABLE_OBJECT_IMPL(C_TYPE, ...) \
	L_INTERNAL_DECLARE_C_CLONABLE_OBJECT(C_TYPE, __VA_ARGS__) \
	L_INTERNAL_DECLARE_C_CLONABLE_OBJECT_FUNCTIONS(C_TYPE, L_INTERNAL_C_NO_XTOR, L_INTERNAL_C_NO_XTOR, L_INTERNAL_C_NO_CLONE_C)

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
	LinphonePrivate::Wrapper::getCppPtrFromC(C_OBJECT)
#define L_GET_CPP_PTR_FROM_C_OBJECT_2_ARGS(C_OBJECT, CPP_TYPE) \
	LinphonePrivate::Wrapper::getCppPtrFromC< \
		std::remove_pointer<decltype(C_OBJECT)>::type, \
		LinphonePrivate::CPP_TYPE \
	>(C_OBJECT)

#define L_GET_CPP_PTR_FROM_C_OBJECT_MACRO_CHOOSER(...) \
	L_EXPAND(L_GET_ARG_3(__VA_ARGS__, L_GET_CPP_PTR_FROM_C_OBJECT_2_ARGS, L_GET_CPP_PTR_FROM_C_OBJECT_1_ARGS))

#define L_GET_CPP_PTR_FROM_C_OBJECT(...) \
	L_EXPAND(L_GET_CPP_PTR_FROM_C_OBJECT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

// Set the cpp-ptr of a wrapped C object.
#define L_SET_CPP_PTR_FROM_C_OBJECT(C_OBJECT, CPP_OBJECT) \
	LinphonePrivate::Wrapper::setCppPtrFromC(C_OBJECT, CPP_OBJECT)

// Get the private data of a shared or simple cpp-ptr.
#define L_GET_PRIVATE_1_ARGS(CPP_OBJECT) \
	LinphonePrivate::Wrapper::getPrivate(LinphonePrivate::Utils::getPtr(CPP_OBJECT))
#define L_GET_PRIVATE_2_ARGS(CPP_OBJECT, CPP_TYPE) \
	LinphonePrivate::Wrapper::cast<CPP_TYPE ## Private>(L_GET_PRIVATE_1_ARGS(CPP_OBJECT))

#define L_GET_PRIVATE_MACRO_CHOOSER(...) \
	L_EXPAND(L_GET_ARG_3(__VA_ARGS__, L_GET_PRIVATE_2_ARGS, L_GET_PRIVATE_1_ARGS))

#define L_GET_PRIVATE(...) \
	L_EXPAND(L_GET_PRIVATE_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

// Get the private data of a shared or simple cpp-ptr of a wrapped C object.
#define L_GET_PRIVATE_FROM_C_OBJECT_1_ARGS(C_OBJECT) \
	L_GET_PRIVATE_1_ARGS(LinphonePrivate::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT_1_ARGS(C_OBJECT)))
#define L_GET_PRIVATE_FROM_C_OBJECT_2_ARGS(C_OBJECT, CPP_TYPE) \
	L_GET_PRIVATE_1_ARGS(LinphonePrivate::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT_2_ARGS(C_OBJECT, CPP_TYPE)))

#define L_GET_PRIVATE_FROM_C_OBJECT_MACRO_CHOOSER(...) \
	L_EXPAND(L_GET_ARG_3(__VA_ARGS__, L_GET_PRIVATE_FROM_C_OBJECT_2_ARGS, L_GET_PRIVATE_FROM_C_OBJECT_1_ARGS))

#define L_GET_PRIVATE_FROM_C_OBJECT(...) \
	L_EXPAND(L_GET_PRIVATE_FROM_C_OBJECT_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

// Get the wrapped C object of a C++ object.
#define L_GET_C_BACK_PTR(CPP_OBJECT) \
	LinphonePrivate::Wrapper::getCBackPtr(CPP_OBJECT)

// Get/set user data on a wrapped C object.
#define L_GET_USER_DATA_FROM_C_OBJECT(C_OBJECT) \
	LinphonePrivate::Wrapper::getUserData( \
		LinphonePrivate::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT)) \
	)
#define L_SET_USER_DATA_FROM_C_OBJECT(C_OBJECT, VALUE) \
	LinphonePrivate::Wrapper::setUserData( \
		LinphonePrivate::Utils::getPtr(L_GET_CPP_PTR_FROM_C_OBJECT(C_OBJECT)), \
		VALUE \
	)

// Transforms cpp list and c list.
#define L_GET_C_LIST_FROM_CPP_LIST(CPP_LIST) \
	LinphonePrivate::Wrapper::getCListFromCppList(CPP_LIST)
#define L_GET_CPP_LIST_FROM_C_LIST(C_LIST, TYPE) \
	LinphonePrivate::Wrapper::getCppListFromCList<TYPE>(C_LIST)

// Transforms cpp list and c list and convert cpp object to c object.
#define L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(CPP_LIST) \
	LinphonePrivate::Wrapper::getResolvedCListFromCppList(CPP_LIST)
#define L_GET_RESOLVED_CPP_LIST_FROM_C_LIST(C_LIST, C_TYPE) \
	LinphonePrivate::Wrapper::getResolvedCppListFromCList<Linphone ## C_TYPE>(C_LIST)

#endif // ifndef _L_C_TOOLS_H_
