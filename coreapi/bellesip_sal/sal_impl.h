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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef SAL_IMPL_H_
#define SAL_IMPL_H_
#include "sal.h"
#include "belle-sip/belle-sip.h"
#include "belle-sip/belle-sdp.h"

struct Sal{
	SalCallbacks callbacks;
	MSList *pending_auths;/*MSList of SalOp */
	belle_sip_listener_callbacks_t listener_callbacks;
	belle_sip_stack_t* stack;
	belle_sip_provider_t *prov;
	belle_sip_header_user_agent_t* user_agent;
	void *up; /*user pointer*/
	int session_expires;
};


struct SalOp{
	SalOpBase base;
	belle_sip_listener_callbacks_t callbacks;
	belle_sip_request_t* request;
	belle_sip_response_t* response;
	SalAuthInfo auth_info;
	unsigned long int  registration_refresh_timer;
	bool_t sdp_offering;
};

belle_sdp_session_description_t * media_description_to_sdp(const SalMediaDescription *sal);
int sdp_to_media_description(belle_sdp_session_description_t  *sdp, SalMediaDescription *desc);
belle_sip_request_t* sal_op_build_request(SalOp *op,const char* method);

#endif /* SAL_IMPL_H_ */
