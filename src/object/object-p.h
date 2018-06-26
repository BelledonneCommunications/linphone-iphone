/*
 * object-p.h
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

#ifndef _L_OBJECT_P_H_
#define _L_OBJECT_P_H_

#include "base-object-p.h"
#include "object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

#ifdef _WIN32
	// TODO: Avoid this error.
	// Disable C4251 triggered by std::recursive_mutex.
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif // ifdef _WIN32

class LINPHONE_INTERNAL_PUBLIC ObjectPrivate : public BaseObjectPrivate {
protected:
	inline const Object::Lock &getLock () const {
		return lock;
	}

private:
	Object::Lock lock;

	L_DECLARE_PUBLIC(Object);
};

#ifdef _WIN32
	#pragma warning(pop)
#endif // ifdef _WIN32

LINPHONE_END_NAMESPACE

#endif // ifndef _L_OBJECT_P_H_
