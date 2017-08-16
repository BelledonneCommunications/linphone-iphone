/*
 * c-private-types.h
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

#ifndef _C_PRIVATE_TYPES_H_
#define _C_PRIVATE_TYPES_H_

#include <memory>

#include "conference/params/media-session-params.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif

// =============================================================================
// C Structures.
// =============================================================================

struct _LinphoneCallParams{
	belle_sip_object_t base;
	void *user_data;
	std::shared_ptr<LinphonePrivate::MediaSessionParams> msp;
};

#ifdef __cplusplus
	}
#endif

#endif // ifndef _C_PRIVATE_TYPES_H_
