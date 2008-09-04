/*
	The objective of the media_api is to construct and run the necessary processing 
	on audio and video data flows for a given call (two party call) or conference.
	Copyright (C) 2001  Sharath Udupa skuds@gmx.net 

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

#include "common.h"
#include "mediaflow.h"
#include "callmember.h"

//other includes required to be done here
#define MemberA 1
#define MemberB 2


struct _BasicCall{
	CallMember *memberA, *memberB;
	GList *flows;  			//linked list of MediaFlows
};

typedef struct _BasicCall BasicCall;


BasicCall *basic_call_new();

CallMember *basic_call_get_member(BasicCall *call, int member_nu);

void basic_call_add_flow(BasicCall *call, MediaFlow *flow);

int basic_call_start_flow(BasicCall *call, MediaFlow *flow);

int basic_call_stop_flow(BasicCall *call, MediaFlow *flow);

int basic_call_start_all_flows(BasicCall *call);

int basic_call_destroy(BasicCall *call);

