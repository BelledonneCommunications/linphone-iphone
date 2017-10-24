/*
 * object.h
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

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <memory>

#include "base-object.h"
#include "property-container.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

/*
 * Main Object of Linphone. Can be shared but is not Clonable.
 * Supports properties and shared from this.
 * Must be built with ObjectFactory.
 */
class LINPHONE_PUBLIC Object : public BaseObject, public PropertyContainer {
	friend class ObjectFactory;

public:
	virtual ~Object () = default;

protected:
	explicit Object (ObjectPrivate &p);

	std::shared_ptr<Object> getSharedFromThis ();
	std::shared_ptr<const Object> getSharedFromThis () const;

private:
	L_DECLARE_PRIVATE(Object);
	L_DISABLE_COPY(Object);
};

class ObjectFactory {
public:
	template<typename T, class ...Args>
	static inline std::shared_ptr<T> create (Args &&...args) {
		static_assert(std::is_base_of<Object, T>::value, "Not an object.");
		std::shared_ptr<T> object = std::make_shared<T>(args...);
		setPublic(object);
		return object;
	}

private:
	ObjectFactory () = delete;

	static void setPublic (const std::shared_ptr<Object> &object);

	L_DISABLE_COPY(ObjectFactory);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _OBJECT_H_
