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
/***************************************************************************
 *            jitterctl.c
 *
 *  Mon Nov  8 11:53:21 2004
 *  Copyright  2004  Simon MORLAT
 *  Email simon.morlat@linphone.org
 ****************************************************************************/
 
#ifndef JITTERCTL_H
#define JITTERCTL_H


void jitter_control_init(JitterControl *ctl, int base_jiitt_time, PayloadType *pt);
void jitter_control_enable_adaptive(JitterControl *ctl, bool_t val);
void jitter_control_new_packet(JitterControl *ctl, uint32_t packet_ts, uint32_t cur_str_ts);
#define jitter_control_adaptive_enabled(ctl) ((ctl)->adaptive)
void jitter_control_set_payload(JitterControl *ctl, PayloadType *pt);
void jitter_control_update_corrective_slide(JitterControl *ctl);
static inline uint32_t jitter_control_get_compensated_timestamp(JitterControl *obj , uint32_t user_ts){
	return user_ts+obj->slide-obj->adapt_jitt_comp_ts;
}

#endif
