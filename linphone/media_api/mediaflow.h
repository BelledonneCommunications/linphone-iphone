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

	You should have received a c:opy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

struct _MediaFlow{
	char *id;
	int type;
	GList *members;
	GList *flowDirections;
	GList *sync; //holds all the filters in this MediaFlow
};

typedef struct _MediaFlow MediaFlow;

struct _Members{
  struct _CallMember *member;
  struct _EndPoint *rx_endpoint;
  struct _EndPoint *tx_endpoint;
};

typedef struct _Members Members;

struct _FlowDirections{
	Members *source, *destination;
	MSFilter *recv,
		 *dec,
		 *play;
	MSFilter *read, 	//Filters used 
		 *enc,		//if type==DUPLEX
		 *send;
	GList *rtpSessions;
	int type;
};

typedef struct _FlowDirections FlowDirections;


MediaFlow *media_flow_new(char *id_string, int type);

int media_flow_setup_fd(MediaFlow*, struct _CallMember *, struct _CallMember *, int);

int media_flow_start_fd(FlowDirections *fd, MSSync *sync);

int media_flow_destroy(MediaFlow *flow);

/* Internal functions */
int find(gconstpointer, gconstpointer);

MSFilter *set_MSFilter(struct _EndPoint *, int, FlowDirections *);

MSFilter *set_CODECFilter(RtpProfile* , int, int);

