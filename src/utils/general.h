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

#define LINPHONE_NAMESPACE LinphonePrivate

#ifdef __cplusplus
	#define LINPHONE_BEGIN_NAMESPACE namespace LINPHONE_NAMESPACE {
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

// -----------------------------------------------------------------------------

#ifdef __cplusplus

void l_assert (const char *condition, const char *file, int line);

#ifdef DEBUG
	#define L_ASSERT(CONDITION) static_cast<void>(false && (CONDITION))
#else
	#define L_ASSERT(CONDITION) ((CONDITION) ? static_cast<void>(0) : l_assert(#CONDITION, __FILE__, __LINE__))
#endif

#define L_DECLARE_PRIVATE(CLASS) \
	inline CLASS ## Private * getPrivate() { \
		return reinterpret_cast<CLASS ## Private *>(mPrivate); \
	} \
	inline const CLASS ## Private *getPrivate() const { \
		return reinterpret_cast<const CLASS ## Private *>(mPrivate); \
	} \
	friend class CLASS ## Private;

class ClonableObject;
class ClonableObjectPrivate;
class Object;
class ObjectPrivate;

template<typename T>
inline ClonableObject *getPublicHelper (T *object, ClonableObjectPrivate *context) {
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
inline Object *getPublicHelper (T *object, ObjectPrivate *) {
	return object;
}

template<typename T>
inline const Object *getPublicHelper (const T *object, const ObjectPrivate *) {
	return object;
}

#define L_DECLARE_PUBLIC(CLASS) \
	inline CLASS * getPublic () { \
		return static_cast<CLASS *>(getPublicHelper(mPublic, this)); \
	} \
	inline const CLASS *getPublic () const { \
		return static_cast<const CLASS *>(getPublicHelper(mPublic, this)); \
	} \
	friend class CLASS;

#define L_DISABLE_COPY(CLASS) \
	CLASS(const CLASS &) = delete; \
	CLASS &operator= (const CLASS &) = delete;

#define L_D(CLASS) CLASS ## Private * const d = getPrivate();
#define L_Q(CLASS) CLASS * const q = getPublic();

#define L_USE_DEFAULT_SHARE_IMPL(CLASS, PARENT_CLASS) \
	CLASS::CLASS (const CLASS &src) : PARENT_CLASS(*src.getPrivate()) {} \
	CLASS &CLASS::operator= (const CLASS &src) { \
		if (this != &src) \
			setRef(*src.getPrivate()); \
		return *this; \
	}

#endif

LINPHONE_END_NAMESPACE

#endif // ifndef _GENERAL_H_
