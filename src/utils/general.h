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

// =============================================================================

#ifndef _GENERAL_H_
#define _GENERAL_H_

#define L_DECLARE_PRIVATE(CLASS) \
	inline CLASS ## Private * getPrivate() { \
		return reinterpret_cast<CLASS ## Private *>(mPrivate); \
	} \
	inline const CLASS ## Private *getPrivate() const { \
		return reinterpret_cast<const CLASS ## Private *>(mPrivate); \
	} \
	friend class CLASS ## Private;

#define L_DECLARE_PUBLIC(CLASS) \
	inline CLASS * getPublic() { \
		return static_cast<CLASS *>(mPublic); \
	} \
	inline const CLASS *getPublic() const { \
		return static_cast<const CLASS *>(mPublic); \
	} \
	friend class CLASS;

#define L_DISABLE_COPY(CLASS) \
	CLASS(const CLASS &) = delete; \
	CLASS &operator= (const CLASS &) = delete;

#define L_D(CLASS) CLASS ## Private * const d = getPrivate();
#define L_Q(CLASS) CLASS * const q = getPublic();

#endif // ifndef _GENERAL_H_
