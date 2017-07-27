/*
 * singleton.h
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

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include "object.h"

// =============================================================================

namespace LinphonePrivate {
	template<class T>
	class Singleton : public Object {
	public:
		static Singleton<T> *getInstance () {
			if (!mInstance)
				mInstance = new Singleton<T>();
			return mInstance;
		}

		virtual ~Singleton () = default;

	protected:
		explicit Singleton (ObjectPrivate *objectPrivate = nullptr) : Object(objectPrivate) {}

	private:
		static Singleton<T> *mInstance;

		L_DISABLE_COPY(Singleton);
	};

	template<class T>
	Singleton<T> *Singleton<T>::mInstance = nullptr;
}

#endif // ifndef _SINGLETON_H_
