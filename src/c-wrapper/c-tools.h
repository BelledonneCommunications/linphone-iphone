/*
 * c-tools.h
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

#ifndef _C_TOOLS_H_
#define _C_TOOLS_H_

// From coreapi.
#include "private.h"

// =============================================================================

#define L_DECLARE_C_STRUCT_IMPL(STRUCT, C_NAME) \
	struct _Linphone ## STRUCT { \
		belle_sip_object_t base; \
		shared_ptr<LINPHONE_NAMESPACE::STRUCT> cppPtr; \
	}; \
	BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## STRUCT); \
	static Linphone ## STRUCT *_linphone_ ## C_NAME ## _init() { \
		Linphone ## STRUCT * object = belle_sip_object_new(Linphone ## STRUCT); \
		new(&object->cppPtr) shared_ptr<LINPHONE_NAMESPACE::STRUCT>(); \
		return object; \
	} \
	static void _linphone_ ## C_NAME ## _uninit(Linphone ## STRUCT * object) { \
		object->cppPtr.reset(); \
		object->cppPtr->~STRUCT (); \
	} \
	static void _linphone_ ## C_NAME ## _clone(Linphone ## STRUCT * dest, const Linphone ## STRUCT * src) { \
		new(&dest->cppPtr) shared_ptr<LINPHONE_NAMESPACE::STRUCT>(); \
		dest->cppPtr = make_shared<LINPHONE_NAMESPACE::STRUCT>(*src->cppPtr.get()); \
	} \
	BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## STRUCT); \
	BELLE_SIP_INSTANCIATE_VPTR(Linphone ## STRUCT, belle_sip_object_t, \
	_linphone_ ## C_NAME ## _uninit, \
	_linphone_ ## C_NAME ## _clone, \
	NULL, \
	FALSE \
	);

#define L_DECLARE_C_STRUCT_NEW_DEFAULT(STRUCT, C_NAME) \
	Linphone ## STRUCT * linphone_ ## C_NAME ## _new() { \
		Linphone ## STRUCT * object = _linphone_ ## C_NAME ## _init(); \
		object->cppPtr = make_shared<LINPHONE_NAMESPACE::STRUCT>(); \
		return object; \
	}

#endif // ifndef _C_TOOLS_H_
