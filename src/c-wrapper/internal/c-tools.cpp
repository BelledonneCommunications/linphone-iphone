/*
 * c-tools.cpp
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

#ifdef DEBUG
	#include <typeinfo>
#endif

#include "c-tools.h"
#include "object/base-object.h"
#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

#ifdef DEBUG
	void Wrapper::setName (belle_sip_object_t *cObject, const BaseObject *cppObject) {
		belle_sip_object_set_name(cObject, typeid(*cppObject).name());
	}

	void Wrapper::setName (belle_sip_object_t *cObject, const ClonableObject *cppObject) {
		belle_sip_object_set_name(cObject, typeid(*cppObject).name());
	}
#endif

LINPHONE_END_NAMESPACE
