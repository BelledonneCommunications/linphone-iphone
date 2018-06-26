/*
 * core_private.h
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

#ifndef _CORE_PRIVATE_H_
#define _CORE_PRIVATE_H_

#include "linphone/types.h"
#include "private_structs.h"
#include "private_types.h"

struct _LinphoneCore {
	belle_sip_object_t base;
	std::shared_ptr<LinphonePrivate::Core> cppPtr;
	std::weak_ptr<LinphonePrivate::Core> weakCppPtr;
	int owner;
	LINPHONE_CORE_STRUCT_FIELDS
};

#endif /* _CORE_PRIVATE_H_ */
