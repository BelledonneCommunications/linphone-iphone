/*
 * general.h
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#ifndef _GENERAL_H_
#define _GENERAL_H_

#ifdef __cplusplus
	#include <type_traits>
#endif

// =============================================================================

#ifdef __cplusplus
	#define LINPHONE_BEGIN_NAMESPACE namespace LinphonePrivate {
	#define LINPHONE_END_NAMESPACE }
#else
	#define LINPHONE_BEGIN_NAMESPACE
	#define LINPHONE_END_NAMESPACE
#endif

// -----------------------------------------------------------------------------

LINPHONE_BEGIN_NAMESPACE

#ifndef LINPHONE_PUBLIC
	#if defined(_MSC_VER)
		#ifdef LINPHONE_STATIC
			#define LINPHONE_PUBLIC
		#else
			#ifdef LINPHONE_EXPORTS
				#define LINPHONE_PUBLIC	__declspec(dllexport)
			#else
				#define LINPHONE_PUBLIC	__declspec(dllimport)
			#endif
		#endif
	#else
		#define LINPHONE_PUBLIC
	#endif
#endif

#ifndef LINPHONE_DEPRECATED
	#if defined(_MSC_VER)
		#define LINPHONE_DEPRECATED __declspec(deprecated)
	#else
		#define LINPHONE_DEPRECATED __attribute__((deprecated))
	#endif
#endif

// -----------------------------------------------------------------------------

#ifdef __cplusplus

void l_assert (const char *condition, const char *file, int line);

#ifdef DEBUG
	#define L_ASSERT(CONDITION) ((CONDITION) ? static_cast<void>(0) : LinphonePrivate::l_assert(#CONDITION, __FILE__, __LINE__))
#else
	#define L_ASSERT(CONDITION) static_cast<void>(false && (CONDITION))
#endif

#ifndef _MSC_VER
	#define L_LIKELY(EXPRESSION) __builtin_expect(static_cast<bool>(EXPRESSION), true)
	#define L_UNLIKELY(EXPRESSION)  __builtin_expect(static_cast<bool>(EXPRESSION), false)
#else
	#define L_LIKELY(EXPRESSION) EXPRESSION
	#define L_UNLIKELY(EXPRESSION) EXPRESSION
#endif

class BaseObject;
class BaseObjectPrivate;
class ClonableObject;
class ClonableObjectPrivate;
class Object;
class ObjectPrivate;

#define L_INTERNAL_CHECK_OBJECT_INHERITANCE(CLASS) \
	static_assert( \
		!(std::is_base_of<BaseObject, CLASS>::value && std::is_base_of<ClonableObject, CLASS>::value), \
		"Multiple inheritance between BaseObject and ClonableObject is not allowed." \
	);

#define L_INTERNAL_GET_BETTER_PRIVATE_ANCESTOR(CLASS) \
	std::conditional< \
		std::is_base_of<BaseObject, CLASS>::value, \
		BaseObject, \
		std::conditional< \
			std::is_base_of<ClonableObject, CLASS>::value, \
			ClonableObject, \
			CLASS \
		>::type \
	>::type

#define L_INTERNAL_DECLARE_PRIVATE(CLASS) \
	inline CLASS ## Private *getPrivate () { \
		L_INTERNAL_CHECK_OBJECT_INHERITANCE(CLASS); \
		return reinterpret_cast<CLASS ## Private *>( \
			L_INTERNAL_GET_BETTER_PRIVATE_ANCESTOR(CLASS)::mPrivate \
		); \
	} \
	inline const CLASS ## Private *getPrivate () const { \
		L_INTERNAL_CHECK_OBJECT_INHERITANCE(CLASS); \
		return reinterpret_cast<const CLASS ## Private *>( \
			L_INTERNAL_GET_BETTER_PRIVATE_ANCESTOR(CLASS)::mPrivate \
		); \
	} \
	friend class CLASS ## Private; \
	friend class Wrapper;

// Allows access to private internal data.
// Gives a control to C Wrapper.
#ifndef LINPHONE_TESTER
	#define L_DECLARE_PRIVATE(CLASS) L_INTERNAL_DECLARE_PRIVATE(CLASS)
#else
	#define L_DECLARE_PRIVATE(CLASS) \
		L_INTERNAL_DECLARE_PRIVATE(CLASS) \
		friend class Tester;
#endif

namespace Private {
	// See: http://en.cppreference.com/w/cpp/types/void_t
	template<typename... T> struct MakeVoid {
		typedef void type;
	};
	template<typename... T>
	using void_t = typename MakeVoid<T...>::type;

	template<typename T, typename U = void>
	struct IsMapContainerImpl : std::false_type {};

	template<typename T>
	struct IsMapContainerImpl<
		T,
		void_t<
			typename T::key_type,
			typename T::mapped_type,
			decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])
		>
	> : std::true_type {};
};

// Check if a type is a std container like map, unordered_map...
template<typename T>
struct IsMapContainer : Private::IsMapContainerImpl<T>::type {};

// Generic public helper.
template<
	typename R,
	typename P,
	typename C,
	typename = typename std::enable_if<!IsMapContainer<P>::value, P>::type
>
constexpr R *getPublicHelper (P *object, const C *) {
	return static_cast<R *>(object);
}

// Generic public helper. Deal with shared data.
template<
	typename R,
	typename P,
	typename C,
	typename = typename std::enable_if<IsMapContainer<P>::value, P>::type
>
inline R *getPublicHelper (const P *map, const C *context) {
	auto it = map->find(context);
	L_ASSERT(it != map->cend());
	return static_cast<R *>(it->second);
}

#define L_DECLARE_PUBLIC(CLASS) \
	inline CLASS *getPublic () { \
		L_ASSERT(mPublic); \
		return getPublicHelper<CLASS>(mPublic, this); \
	} \
	inline const CLASS *getPublic () const { \
		L_ASSERT(mPublic); \
		return getPublicHelper<const CLASS>(mPublic, this); \
	} \
	friend class CLASS;

#define L_DISABLE_COPY(CLASS) \
	CLASS (const CLASS &) = delete; \
	CLASS &operator= (const CLASS &) = delete;

// Get Private data.
#define L_D() decltype(getPrivate()) const d = getPrivate();

// Get Public data.
#define L_Q() decltype(getPublic()) const q = getPublic();

template<typename T, typename U>
struct AddConstMirror {
	typedef U type;
};

template<typename T, typename U>
struct AddConstMirror<const T, U> {
	typedef typename std::add_const<U>::type type;
};

// Get Private data of class in a multiple inheritance case.
#define L_D_T(CLASS, NAME) \
	auto const NAME = static_cast< \
		AddConstMirror< \
			std::remove_reference<decltype(*this)>::type, \
			CLASS ## Private \
		>::type * \
	>(CLASS::mPrivate);

// Get Private data of class in a multiple inheritance case.
#define L_Q_T(CLASS, NAME) \
	auto const NAME = static_cast< \
		AddConstMirror< \
			std::remove_reference<decltype(*this)>::type, \
			CLASS \
		>::type * \
	>(getPublic());

#define L_OVERRIDE_SHARED_FROM_THIS(CLASS) \
	inline std::shared_ptr<CLASS> getSharedFromThis () { \
		return std::static_pointer_cast<CLASS>(Object::getSharedFromThis()); \
	} \
	inline std::shared_ptr<const CLASS> getSharedFromThis () const { \
		return std::static_pointer_cast<const CLASS>(Object::getSharedFromThis()); \
	}

#define L_USE_DEFAULT_SHARE_IMPL(CLASS, PARENT_CLASS) \
	CLASS::CLASS (const CLASS &src) : PARENT_CLASS(*src.getPrivate()) {} \
	CLASS &CLASS::operator= (const CLASS &src) { \
		if (this != &src) \
			setRef(*src.getPrivate()); \
		return *this; \
	}

// -----------------------------------------------------------------------------
// Wrapper public.
// -----------------------------------------------------------------------------

#define L_DECL_C_STRUCT(STRUCT) typedef struct _ ## STRUCT STRUCT;
#define L_DECL_C_STRUCT_PREFIX_LESS(STRUCT) typedef struct STRUCT STRUCT;

#endif

LINPHONE_END_NAMESPACE

#endif // ifndef _GENERAL_H_
