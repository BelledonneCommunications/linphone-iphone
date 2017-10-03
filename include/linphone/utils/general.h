/*
 * general.h
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

#ifndef _GENERAL_H_
#define _GENERAL_H_

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

#ifndef DEBUG
	#define L_ASSERT(CONDITION) static_cast<void>(false && (CONDITION))
#else
	#define L_ASSERT(CONDITION) ((CONDITION) ? static_cast<void>(0) : LinphonePrivate::l_assert(#CONDITION, __FILE__, __LINE__))
#endif

#ifndef _MSC_VER
	#define L_LIKELY(EXPRESSION) __builtin_expect(static_cast<bool>(EXPRESSION), true)
	#define L_UNLIKELY(EXPRESSION)  __builtin_expect(static_cast<bool>(EXPRESSION), false)
#else
	#define L_LIKELY(EXPRESSION) EXPRESSION
	#define L_UNLIKELY(EXPRESSION) EXPRESSION
#endif

class ClonableObject;
class ClonableObjectPrivate;
class Object;
class ObjectPrivate;

#define L_INTERNAL_DECLARE_PRIVATE(CLASS) \
	inline CLASS ## Private *getPrivate() { \
		return reinterpret_cast<CLASS ## Private *>(mPrivate); \
	} \
	inline const CLASS ## Private *getPrivate() const { \
		return reinterpret_cast<const CLASS ## Private *>(mPrivate); \
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

template<typename T>
inline ClonableObject *getPublicHelper (T *object, const ClonableObjectPrivate *context) {
	auto it = object->find(context);
	L_ASSERT(it != object->end());
	return it->second;
}

template<typename T>
inline const ClonableObject *getPublicHelper (const T *object, const ClonableObjectPrivate *context) {
	auto it = object->find(context);
	L_ASSERT(it != object->cend());
	return it->second;
}

template<typename T>
inline Object *getPublicHelper (T *object, const ObjectPrivate *) {
	return object;
}

template<typename T>
inline const Object *getPublicHelper (const T *object, const ObjectPrivate *) {
	return object;
}

#define L_DECLARE_PUBLIC(CLASS) \
	inline CLASS *getPublic () { \
		return static_cast<CLASS *>(getPublicHelper(mPublic, this)); \
	} \
	inline const CLASS *getPublic () const { \
		return static_cast<const CLASS *>(getPublicHelper(mPublic, this)); \
	} \
	friend class CLASS;

#define L_DISABLE_COPY(CLASS) \
	CLASS(const CLASS &) = delete; \
	CLASS &operator= (const CLASS &) = delete;

#define L_D() decltype(std::declval<decltype(*this)>().getPrivate()) const d = getPrivate();
#define L_Q() decltype(std::declval<decltype(*this)>().getPublic()) const q = getPublic();

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
