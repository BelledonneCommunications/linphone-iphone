/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc1889) stack.
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


/* ortp-config-win32.h.  Generated manually...  */
#define ORTP_VERSION		"0.8.2"
#define ORTP_MAJOR_VERSION	0
#define ORTP_MINOR_VERSION	8
#define ORTP_MICRO_VERSION	2
#define ORTP_EXTRA_VERSION

/* define the debug mode */
#define RTP_DEBUG 1

#define HAVE_SRTP 1

/* enables SO_REUSEADDR socket option in the rtp_session_set_local_addr() function */
#define SO_REUSE_ADDR 1
