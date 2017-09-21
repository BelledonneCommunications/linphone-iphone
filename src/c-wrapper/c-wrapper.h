/*
 * c-wrapper.h
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

#ifndef _C_WRAPPER_H_
#define _C_WRAPPER_H_

#include "linphone/api/c-types.h"

#include "internal/c-tools.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

template<typename CppType, typename CType>
struct CppTypeToCType {
	enum { Defined = false };
};

LINPHONE_END_NAMESPACE

#define L_REGISTER_TYPE(CPP_TYPE, C_TYPE) \
	extern Linphone ## C_TYPE *_linphone_ ## C_TYPE ## _init (); \
	namespace LINPHONE_NAMESPACE { \
		class CPP_TYPE; \
	}; \
	template<> \
	struct LINPHONE_NAMESPACE::CppTypeToCType<LINPHONE_NAMESPACE::CPP_TYPE, Linphone ## C_TYPE> { \
		enum { Defined = true }; \
		typedef C_TYPE cType; \
		typedef LINPHONE_NAMESPACE::CPP_TYPE cppType; \
	};

// =============================================================================

L_REGISTER_TYPE(Call, Call);
L_REGISTER_TYPE(ChatMessage, ChatMessage);
L_REGISTER_TYPE(ChatRoom, ChatRoom);
L_REGISTER_TYPE(Participant, Participant);

#endif // ifndef _C_WRAPPER_H_
