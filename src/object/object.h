/*
 * object.h
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

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "utils/general.h"

// =============================================================================

namespace LinphonePrivate {
	class Object;

	class ObjectPrivate {
	public:
		virtual ~ObjectPrivate () = default;

	protected:
		Object *mPublic = nullptr;

	private:
		L_DECLARE_PUBLIC(Object);
	};

	class Object {
	public:
		virtual ~Object () {
			delete mPrivate;
		}

	protected:
		explicit Object (ObjectPrivate &p) : mPrivate(&p) {
			mPrivate->mPublic = this;
		}

		ObjectPrivate *mPrivate = nullptr;

	private:
		L_DECLARE_PRIVATE(Object);
		L_DISABLE_COPY(Object);
	};
}

#endif // ifndef _OBJECT_H_
