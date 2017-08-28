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

#include <belle-sip/belle-sip.h>
#include <memory>

// From coreapi.
#include "private.h"

#include "event-log/event-log.h"

#include "c-types.h"

// ================================================================²=============

using namespace std;

extern "C" {
#define L_DECLARE_C_STRUCT_IMPL(STRUCT) \
  struct _Linphone ## STRUCT { \
    belle_sip_object_t base; \
    shared_ptr<LINPHONE_NAMESPACE::STRUCT> cppPtr; \
  }; \
  static void _linphone_ ## STRUCT ## _uninit(Linphone ## STRUCT * object) { \
    object->cppPtr.reset(); \
    object->cppPtr->~STRUCT (); \
  } \
  static void _linphone_ ## STRUCT ## _clone(Linphone ## STRUCT * dest, const Linphone ## STRUCT * src) { \
    new(&dest->cppPtr) shared_ptr<LINPHONE_NAMESPACE::STRUCT>(); \
    dest->cppPtr = make_shared<LINPHONE_NAMESPACE::STRUCT>(*src->cppPtr.get()); \
  } \
  BELLE_SIP_DECLARE_VPTR_NO_EXPORT(Linphone ## STRUCT); \
  BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(Linphone ## STRUCT); \
  BELLE_SIP_INSTANCIATE_VPTR(Linphone ## STRUCT, belle_sip_object_t, \
  _linphone_ ## STRUCT ## _uninit, \
  _linphone_ ## STRUCT ## _clone, \
  NULL, \
  FALSE \
  );

// -----------------------------------------------------------------------------
L_DECLARE_C_STRUCT_IMPL(EventLog);
}
