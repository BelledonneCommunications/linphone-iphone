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

struct _CallMember{
	char *name;
	GList *flows;
	RtpProfile *profile;
};

typedef struct _CallMember CallMember;

struct _EndPoint{
	int protocol;
	char *host;
	char *file;
	int port;
	int pt;
};

typedef struct _EndPoint EndPoint;

struct _Flows{
  struct _MediaFlow *flow;
  EndPoint *rx_endpoint;
  EndPoint *tx_endpoint;
};

typedef struct _Flows Flows;

CallMember *call_member_new(char *);

int call_member_setup_flow(CallMember *member, struct _MediaFlow *flow, char *rx_enndpoint, char *tx_endpoint);

/* Internal functions */
EndPoint *parse_end_point(char *endpoint);

char *remove_slash(char[]);

int to_digits(char*);

int pt_digits(char*);

