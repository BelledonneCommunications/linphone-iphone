/*
 * clonable-object.h
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

#ifndef _L_CLONABLE_OBJECT_H_
#define _L_CLONABLE_OBJECT_H_

#include "object-head.h"
#include "property-container.h"

// =============================================================================

#define L_USE_DEFAULT_CLONABLE_OBJECT_SHARED_IMPL(CLASS) \
	CLASS::CLASS (const CLASS &other) : ClonableObject( \
		const_cast<std::decay<decltype(*other.getPrivate())>::type &>(*other.getPrivate()) \
	) {} \
	CLASS &CLASS::operator= (const CLASS &other) { \
		if (this != &other) \
			setRef(*other.getPrivate()); \
		return *this; \
	}

LINPHONE_BEGIN_NAMESPACE

/*
 * Clonable Object of Linphone. Generally it's just a data object with no
 * intelligence.
 */
class LINPHONE_PUBLIC ClonableObject : public PropertyContainer {
	L_OBJECT;

public:
	virtual ~ClonableObject ();

protected:
	explicit ClonableObject (ClonableObjectPrivate &p);

	// Change the ClonableObjectPrivate. Unref previous.
	void setRef (const ClonableObjectPrivate &p);

	ClonableObjectPrivate *mPrivate = nullptr;

private:
	L_DECLARE_PRIVATE(ClonableObject);

	// Yeah, it's a `ClonableObject` that cannot be copied.
	// Only inherited classes must implement copy.
	L_DISABLE_COPY(ClonableObject);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CLONABLE_OBJECT_H_
