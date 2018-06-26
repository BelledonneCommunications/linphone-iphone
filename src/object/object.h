/*
 * object.h
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

#ifndef _L_OBJECT_H_
#define _L_OBJECT_H_

#include <mutex>

#include "base-object.h"
#include "property-container.h"

// =============================================================================

// Must be used in Object or ObjectPrivate.
#define L_SYNC() \
	static_assert( \
		!std::is_base_of<Object, decltype(this)>::value && !std::is_base_of<ObjectPrivate, decltype(this)>::value, \
		"Unable to lock. Instance is not an Object or ObjectPrivate." \
	); \
	const std::lock_guard<Object::Lock> synchronized(const_cast<Object::Lock &>(getLock()));

LINPHONE_BEGIN_NAMESPACE

#ifdef _WIN32
	// TODO: Avoid this error. Maybe with a custom enabled_shared_from_this.
	// Disable C4251 triggered by std::enabled_shared_from_this.
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif // ifdef _WIN32

/*
 * Main Object of Linphone. Can be shared but is not Clonable.
 * Supports properties and shared from this.
 * Must be built with ObjectFactory.
 */
class LINPHONE_PUBLIC Object :
	public std::enable_shared_from_this<Object>,
	public BaseObject,
	public PropertyContainer {
public:
	typedef std::recursive_mutex Lock;

	std::shared_ptr<Object> getSharedFromThis ();
	std::shared_ptr<const Object> getSharedFromThis () const;

protected:
	explicit Object (ObjectPrivate &p);

	const Lock &getLock () const;

private:
	L_DECLARE_PRIVATE(Object);
	L_DISABLE_COPY(Object);
};

#ifdef _WIN32
	#pragma warning(pop)
#endif // ifdef _WIN32

LINPHONE_END_NAMESPACE

#endif // ifndef _L_OBJECT_H_
