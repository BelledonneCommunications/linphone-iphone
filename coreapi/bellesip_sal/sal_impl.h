/*
linphone
Copyright (C) 2012  Belledonne Communications, Grenoble, France

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef SAL_IMPL_H_
#define SAL_IMPL_H_

#include "c-wrapper/internal/c-sal.h"
#include "belle-sip/belle-sip.h"
#include "belle-sip/belle-sdp.h"


belle_sdp_session_description_t * media_description_to_sdp(const SalMediaDescription *desc);
int sdp_to_media_description(belle_sdp_session_description_t  *sdp, SalMediaDescription *desc);

bool_t _sal_compute_sal_errors(belle_sip_response_t* response, SalReason* sal_reason, char* reason, size_t reason_size);
SalReason _sal_reason_from_sip_code(int code);
/*create SalAuthInfo by copying username and realm from suth event*/
SalAuthInfo* sal_auth_info_create(belle_sip_auth_event_t* event) ;


#endif /* SAL_IMPL_H_ */
