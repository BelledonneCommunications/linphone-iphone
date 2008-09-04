/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef ortp_srtp_h
#define ortp_srtp_h

#include <srtp/srtp.h>
#include <ortp/rtpsession.h>

#ifdef __cplusplus
extern "C"{
#endif


err_status_t ortp_srtp_init(void);
err_status_t ortp_srtp_create(srtp_t *session, const srtp_policy_t *policy);
err_status_t ortp_srtp_dealloc(srtp_t session);
err_status_t ortp_srtp_add_stream(srtp_t session, const srtp_policy_t *policy);

bool_t ortp_srtp_supported(void);

int srtp_transport_new(srtp_t srtp, RtpTransport **rtpt, RtpTransport **rtcpt );

#ifdef __cplusplus
}
#endif

#endif
