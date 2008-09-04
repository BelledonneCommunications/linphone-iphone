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
#include <string.h>
#include "common.h"
#include "callmember.h"
#include "mediaflow.h"


CallMember *call_member_new(char *name){
  CallMember *member = (CallMember*) g_malloc(sizeof(CallMember));
  api_trace("call_member_new: creating %s", name);
  member->name = name;
  member->flows = NULL;
  member->profile = NULL;
  return member;
}

int call_member_set_rtp_profile(CallMember *member, RtpProfile *profile){
	member->profile = profile;
	return 1;
}

int call_member_setup_flow(CallMember *member, MediaFlow *flow, char* rx, char *tx){
  Members *mem = (Members*) g_malloc(sizeof(Members));
  Flows *flows = (Flows*) g_malloc(sizeof(Flows));
  api_trace("call_member_setup_flow: setting up flow for: CallMember->%s , MediaFlow->%s", member->name, flow->id);
  mem->member = member;
  mem->rx_endpoint = parse_end_point(rx);
  mem->tx_endpoint = parse_end_point(tx);
  flow->members = g_list_append(flow->members, mem);

  flows->flow = flow;
  flows->rx_endpoint = parse_end_point(rx);
  flows->tx_endpoint = parse_end_point(tx);
  member->flows = g_list_append(member->flows, flows);
  return 1;
}

EndPoint *parse_end_point(char *endpoint){
	EndPoint *result = (EndPoint*) g_malloc(sizeof(EndPoint));
	int i=0,len1,len2,len, tlen;
	char *str2, temp[30], *host_str;
	//api_trace("parse_end_point: parsing %s\n", endpoint);
	result->pt = -1;
	while(1){
		str2 = (char*) strpbrk(endpoint, ":");
		if(str2 == NULL){ 
			str2 = (char*) strpbrk(endpoint, ";");
			if(str2 == NULL){
				len = strlen(endpoint); 
			}
			else{
				len1 = strlen(endpoint);
				len2 = strlen(str2);
				len = len1-len2;
			}
		}
		else{
			len1 = strlen(endpoint);
			len2 = strlen(str2);
			len = len1-len2;
		}
		strncpy(temp,endpoint,len);
		temp[len] = '\0';
		tlen = strlen(temp);
		if((tlen >= 2)&&(temp[0] == '/')&&(temp[1] == '/')){
			host_str = remove_slash(temp);
		}
		switch(i){
			case 0: if(strcmp(temp,"rtp")==0){
					result->protocol=MEDIA_RTP;
				}
				else if(strcmp(temp,"oss")==0){
					result->protocol=MEDIA_OSS;
				}
				else if(strcmp(temp,"alsa")==0){
					result->protocol=MEDIA_ALSA;
				}
				else if(strcmp(temp,"file")==0){
					result->protocol=MEDIA_FILE;
				}
				break;
			case 1: if(result->protocol==MEDIA_FILE){
					result->file=host_str;
				}
				else{
					result->host = host_str;
				}
				break;
			case 2: result->port = to_digits(temp);
				break;
			case 3: result->pt = pt_digits(temp);
				break;
			default://result->options[result->nOptions++] = temp;
				break;
		}
		if(str2 != NULL) endpoint = str2+1;
		else break;
		i++;
	}
	return result;		
}

int to_digits(char *str){
	int nu=0,a,len,i;
	len = strlen(str);
	for(i=0;i<len;i++){
		a=str[i];
		a=a-'0';
		nu = nu*10+a;
	}
	return nu;
}

int pt_digits(char *str){
	int len;
	len = strlen(str);
	if((len>3)&&(str[0]=='p')&&(str[1]=='t')&&(str[2]=='=')){
		return to_digits(str+3);
	}
	else{
		api_warn("Wrong parameters passed in the endpoints");
		return 0;
		//ERROR handling
	}
}
char *remove_slash(char var[]){
	char *temp = (char*) g_malloc(10*sizeof(char));
	int len,i;
	len=strlen(var);
	for(i=2;i<len;i++){
		temp[i-2]=var[i];
	}
	return temp;
}

