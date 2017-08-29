/*
 * types.cpp
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

// From coreapi.
#include "private.h"

// Must be included before cpp headers.
#include "c-types.h"

#include "event-log/message-event.h"

// ================================================================²=============

using namespace std;

extern "C" {
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
	Linphone ## STRUCT * CNAME ## _new() { \
		Linphone ## STRUCT * object = _linphone_ ## C_NAME ## _init(); \
		object->cppPtr = make_shared<LINPHONE_NAMESPACE::STRUCT>(); \
		return object; \
	}

// -----------------------------------------------------------------------------
// Event log.
// -----------------------------------------------------------------------------

L_DECLARE_C_STRUCT_IMPL(EventLog, event_log);
L_DECLARE_C_STRUCT_NEW_DEFAULT(EventLog, event_log);

LinphoneEventLogType event_log_get_type (const LinphoneEventLog *eventLog) {
	return static_cast<LinphoneEventLogType>(eventLog->cppPtr->getType());
}

// -----------------------------------------------------------------------------
// Message Event.
// -----------------------------------------------------------------------------

L_DECLARE_C_STRUCT_IMPL(MessageEvent, message_event);

LinphoneMessageEvent *message_event_new (LinphoneMessage *message) {
	LinphoneMessageEvent *object = _linphone_message_event_init();
	// TODO: call make_shared with cppPtr.
	object->cppPtr = make_shared<LINPHONE_NAMESPACE::MessageEvent>(nullptr);
	return object;
}

LinphoneMessage *message_event_get_message (const LinphoneMessageEvent *messageEvent) {
	// TODO.
	return nullptr;
}
}
