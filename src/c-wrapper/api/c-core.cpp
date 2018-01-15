/*
 * c-core.cpp
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

#include "linphone/wrapper_utils.h"

#include "c-wrapper/c-wrapper.h"
#include "core/core.h"

#include "private_structs.h"

// =============================================================================

using namespace std;

static void _linphone_core_constructor (LinphoneCore *lc);
static void _linphone_core_destructor (LinphoneCore *lc);

L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(
	Core,
	_linphone_core_constructor, _linphone_core_destructor,
	LINPHONE_CORE_STRUCT_FIELDS
)

static void _linphone_core_constructor (LinphoneCore *lc) {
}

static void _linphone_core_destructor (LinphoneCore *lc) {
	if (lc->callsCache)
		bctbx_list_free_with_data(lc->callsCache, (bctbx_list_free_func)linphone_call_unref);
	_linphone_core_uninit(lc);
}
