/*
 * base-object.h
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

#ifndef _L_BASE_OBJECT_H_
#define _L_BASE_OBJECT_H_

#include "linphone/utils/general.h"

#include "object-head.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class BaseObjectPrivate;

/*
 * Base Object of Linphone. Cannot be cloned. Can be Shared.
 * It's the base class of Object. It's useful for lightweight entities
 * like Events.
 */
class LINPHONE_PUBLIC BaseObject {
	L_OBJECT;

public:
	virtual ~BaseObject ();

protected:
	explicit BaseObject (BaseObjectPrivate &p);

	BaseObjectPrivate *mPrivate = nullptr;

private:
	L_DECLARE_PRIVATE(BaseObject);
	L_DISABLE_COPY(BaseObject);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_BASE_OBJECT_H_
