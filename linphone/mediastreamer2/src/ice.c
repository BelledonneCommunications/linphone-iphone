/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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

#if !defined(WIN32) && !defined(_WIN32_WCE)
#ifdef __APPLE__
#include <sys/types.h>
#endif
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "mediastreamer2/ice.h"
#include "mediastreamer2/mscommon.h"

#include <math.h>

static void 
ice_sendtest( struct IceCheckList *checklist, struct CandidatePair *remote_candidate, Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              UInt96 *tid);

static void 
ice_sendtest( struct IceCheckList *checklist, struct CandidatePair *remote_candidate, Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              UInt96 *tid)
{	
   StunMessage req;
   char buf[STUN_MAX_MESSAGE_SIZE];
   int len = STUN_MAX_MESSAGE_SIZE;
   
   memset(&req, 0, sizeof(StunMessage));

   stunBuildReqSimple( &req, username, FALSE, FALSE, 1);
   req.hasMessageIntegrity=TRUE;

   /* 7.1.1.1
   The attribute MUST be set equal to the priority that would be
   assigned, based on the algorithm in Section 4.1.2, to a peer
   reflexive candidate, should one be learned as a consequence of this
   check */
   req.hasPriority = TRUE;
   req.priority.priority = (UInt32)(pow((double)2,(double)24)*(110) + pow((double)2,(double)8)*(65535) + pow((double)2,(double)0)*(256 - remote_candidate->remote_candidate.component_id));

   /* TODO: put this parameter only for the candidate selected */
   if (remote_candidate->nominated_pair==1)
	   req.hasUseCandidate = TRUE;

   if (remote_candidate->rem_controlling==1)
	   {
		   req.hasIceControlled = TRUE;
		   req.iceControlled.value = checklist->tiebreak_value;
	   }
   else
	   {
		   req.hasIceControlling = TRUE;
		   req.iceControlling.value	= checklist->tiebreak_value;
	   }

   /* TODO: not yet implemented? */
   req.hasFingerprint = TRUE;
   
   len = stunEncodeMessage( &req, buf, len, password );

   memcpy(tid , &(req.msgHdr.tr_id), sizeof(req.msgHdr.tr_id));

   sendMessage( myFd, buf, len, dest->addr, dest->port );	
}

int ice_restart(struct IceCheckList *checklist)
{
	struct CandidatePair *remote_candidates = NULL;
	int pos;

	int count_waiting=0;
	int count=0;

	if (checklist==NULL)
		return 0;
	remote_candidates = checklist->cand_pairs;
	if (remote_candidates==NULL)
		return 0;

	for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
	{
		if (strcasecmp(remote_candidates[pos].local_candidate.cand_type, "srflx")==0)
		{
			/* search for a highest priority "equivalent" pair */
			int pos2;
			for (pos2=0;pos2<pos && remote_candidates[pos2].remote_candidate.conn_addr[0]!='\0';pos2++)
			{
				/* same "base" address (origin of STUN connectivity check to the remote candidate */
				if (strcasecmp(remote_candidates[pos].local_candidate.rel_addr, /* base address for "reflexive" address */
					remote_candidates[pos2].local_candidate.conn_addr)==0) /* base address for "host" address */
				{
					/* if same target remote candidate: -> remove the one with lowest priority */
					if (strcasecmp(remote_candidates[pos].remote_candidate.conn_addr,
						remote_candidates[pos2].remote_candidate.conn_addr)==0)
					{    
						/* useless cpair */                 
						ms_message("ice.c: Removing useless pair (idx=%i)", pos);
						remote_candidates[pos].connectivity_check = ICE_PRUNED;

					}

				}

			}
		}
	}

	/* no currently nominated pair */
	checklist->nominated_pair_index = -1;

	for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
	{
		if (remote_candidates[pos].connectivity_check == ICE_PRUNED)
			continue;
		if (remote_candidates[pos].connectivity_check == ICE_FROZEN)
			remote_candidates[pos].connectivity_check = ICE_WAITING;
	}

	checklist->Ta = 40;
	for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
	{
		if (remote_candidates[pos].connectivity_check == ICE_PRUNED)
			continue;
		if (remote_candidates[pos].connectivity_check == ICE_WAITING)
			count_waiting++;
		count++;
	}
	checklist->RTO = MAX(200, count*checklist->Ta*count_waiting);
	return 0;
}

int ice_sound_send_stun_request(RtpSession *session, struct IceCheckList *checklist, uint64_t ctime)
{
	struct CandidatePair *remote_candidates = NULL;

	if (checklist==NULL)
		return 0;
	remote_candidates = checklist->cand_pairs;
	if (remote_candidates==NULL)
		return 0;

	{
		struct CandidatePair *cand_pair;
		int media_socket = rtp_session_get_rtp_socket(session);
		StunAddress4 stunServerAddr;
		StunAtrString username;
		StunAtrString password;
		bool_t res;
		int pos;

		/* prepare ONCE tie-break value */
		if (checklist->tiebreak_value==0) {
			checklist->tiebreak_value = random() * (0x7fffffffffffffff/0x7fff);
		}

		cand_pair=NULL;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
		{
			cand_pair = &remote_candidates[pos];
			if (cand_pair->connectivity_check == ICE_PRUNED)
			{
				cand_pair=NULL;
				continue;
			}
			if (cand_pair->connectivity_check == ICE_WAITING)
				break;
			if (cand_pair->connectivity_check == ICE_IN_PROGRESS)
				break;
			if (cand_pair->connectivity_check == ICE_SUCCEEDED)
				break;
			cand_pair=NULL;
		}

		if (cand_pair==NULL)
			return 0; /* nothing to do: every pair is FAILED, FROZEN or PRUNED */

		/* start first WAITING pair */
		cand_pair=NULL;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
		{
			cand_pair = &remote_candidates[pos];
			if (cand_pair->connectivity_check == ICE_PRUNED)
			{
				cand_pair=NULL;
				continue;
			}
			if (cand_pair->connectivity_check == ICE_WAITING)
				break;
			cand_pair=NULL;
		}

		if (cand_pair!=NULL)
		{
			cand_pair->connectivity_check = ICE_IN_PROGRESS;
			cand_pair->retransmission_number=0;
			cand_pair->retransmission_time=ctime+checklist->RTO;
			/* keep same rem_controlling for retransmission */
			cand_pair->rem_controlling = checklist->rem_controlling;
		}

		/* try no nominate a pair if we are ready */
		if (cand_pair==NULL && checklist->nominated_pair_index<0)
		{
			for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
			{
				cand_pair = &remote_candidates[pos];
				if (cand_pair->connectivity_check == ICE_PRUNED)
				{
					cand_pair=NULL;
					continue;
				}
				if (cand_pair->connectivity_check == ICE_SUCCEEDED)
				{
					break;
				}
				cand_pair=NULL;
			}

			/* ALWAYS accept "host" candidate that have succeeded */
			if (cand_pair!=NULL
				&& (strcasecmp(cand_pair->remote_candidate.cand_type, "host")==0))
			{
				checklist->nominated_pair_index = pos;
				cand_pair->nominated_pair = 1;
				cand_pair->connectivity_check = ICE_IN_PROGRESS;
				cand_pair->retransmission_number=0;
				cand_pair->retransmission_time=ctime+checklist->RTO;
				/* keep same rem_controlling for retransmission */
				cand_pair->rem_controlling = checklist->rem_controlling;
				/* send a new STUN with USE-CANDIDATE */
				ms_message("ice.c: ICE_SUCCEEDED a pair has been nominated (%i)", pos);
				checklist->keepalive_time=ctime+15*1000;
			}
			else if (cand_pair!=NULL)
			{
				struct CandidatePair *cand_pair2;
				int pos2;
				for (pos2=0;pos2<pos && remote_candidates[pos2].remote_candidate.conn_addr[0]!='\0';pos2++)
				{
					cand_pair2 = &remote_candidates[pos2];
					if (cand_pair2->connectivity_check == ICE_PRUNED)
					{
						cand_pair2=NULL;
						continue;
					}
					if (cand_pair2->connectivity_check == ICE_IN_PROGRESS
						||cand_pair2->connectivity_check == ICE_WAITING)
					{
						break;
					}
					cand_pair2=NULL;
				}

				if (cand_pair2!=NULL)
				{
					/* a better candidate is still tested */
					cand_pair=NULL;
				}
				else
				{
					checklist->nominated_pair_index = pos;
					cand_pair->nominated_pair = 1;
					cand_pair->connectivity_check = ICE_IN_PROGRESS;
					cand_pair->retransmission_number=0;
					cand_pair->retransmission_time=ctime+checklist->RTO;
					/* keep same rem_controlling for retransmission */
					cand_pair->rem_controlling = checklist->rem_controlling;
					/* send a new STUN with USE-CANDIDATE */
					ms_message("ice.c: ICE_SUCCEEDED a pair has been nominated (%i)", pos);
					checklist->keepalive_time=ctime+15*1000;
				}
			}
		}

		if (cand_pair==NULL)
		{
			/* no WAITING pair: retransmit after RTO */
			for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
			{
				cand_pair = &remote_candidates[pos];
				if (cand_pair->connectivity_check == ICE_PRUNED)
				{
					cand_pair=NULL;
					continue;
				}
				if (cand_pair->connectivity_check == ICE_IN_PROGRESS
					&& ctime > cand_pair->retransmission_time)
				{
					if (cand_pair->retransmission_number>7)
					{
						ms_message("ice.c: ICE_FAILED for candidate pair! %s:%i -> %s:%i",
							cand_pair->local_candidate.conn_addr,
							cand_pair->local_candidate.conn_port,
							cand_pair->remote_candidate.conn_addr,
							cand_pair->remote_candidate.conn_port);

						cand_pair->connectivity_check = ICE_FAILED;
						cand_pair=NULL;
						continue;
					}

					cand_pair->retransmission_number++;
					cand_pair->retransmission_time=ctime+checklist->RTO;
					break;
				}
				cand_pair=NULL;
			}
		}

		if (cand_pair==NULL)
		{
			if (checklist->nominated_pair_index<0)
				return 0;

			/* send STUN indication each 15 seconds: keepalive */
			if (ctime>checklist->keepalive_time)
			{
				checklist->keepalive_time=ctime+15*1000;
				for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
				{
					cand_pair = &remote_candidates[pos];
					if (cand_pair->connectivity_check == ICE_SUCCEEDED)
					{
						res = stunParseServerName(cand_pair->remote_candidate.conn_addr,
							&stunServerAddr);
						if ( res == TRUE )
						{
							StunMessage req;
							char buf[STUN_MAX_MESSAGE_SIZE];
							int len = STUN_MAX_MESSAGE_SIZE;
							stunServerAddr.port = cand_pair->remote_candidate.conn_port;
							memset(&req, 0, sizeof(StunMessage));
							stunBuildReqSimple( &req, NULL, FALSE, FALSE, 1);
							req.msgHdr.msgType = (STUN_METHOD_BINDING|STUN_INDICATION);
							len = stunEncodeMessage( &req, buf, len, NULL);
							sendMessage( media_socket, buf, len, stunServerAddr.addr, stunServerAddr.port );
						}
					}
				}
			}

			return 0;
		}

		username.sizeValue = 0;
		password.sizeValue = 0;

		/* username comes from "ice-ufrag" (rfrag:lfrag) */
		/* ufrag and pwd are in first row only */
		snprintf(username.value, sizeof(username.value), "%s:%s",
			checklist->rem_ice_ufrag,
			checklist->loc_ice_ufrag);
		username.sizeValue = (UInt16)strlen(username.value);


		snprintf(password.value, sizeof(password.value), "%s",
			checklist->rem_ice_pwd);
		password.sizeValue = (UInt16)strlen(password.value);


		res = stunParseServerName(cand_pair->remote_candidate.conn_addr,
			&stunServerAddr);
		if ( res == TRUE )
		{
			stunServerAddr.port = cand_pair->remote_candidate.conn_port;
			ice_sendtest(checklist, &remote_candidates[pos], media_socket, &stunServerAddr, &username, &password,
				&(cand_pair->tid));
		}
	}

	return 0;
}

static int
_ice_get_localip_for (struct sockaddr_storage *saddr, size_t saddr_len, char *loc, int size)
{
	int err, tmp;
	int sock;
	struct sockaddr_storage addr;
	socklen_t addr_len;

	strcpy (loc, "127.0.0.1");    /* always fallback to local loopback */

	sock = socket (saddr->ss_family, SOCK_DGRAM, 0);
	tmp = 1;
	err = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &tmp, sizeof (int));
	if (err < 0)
	{
		ms_error("ice.c: Error in setsockopt");
		closesocket (sock);
		return -1;
	}
	err = connect (sock, (struct sockaddr*)saddr, saddr_len);
	if (err < 0)
	{
		ms_error("ice.c: Error in connect");
		closesocket (sock);
		return -1;
	}
	addr_len = sizeof (addr);
	err = getsockname (sock, (struct sockaddr *) &addr, (socklen_t*)&addr_len);
	if (err != 0)
	{
		ms_error("ice.c: Error in getsockname");
		closesocket (sock);
		return -1;
	}

	err = getnameinfo ((struct sockaddr *) &addr, addr_len, loc, size, NULL, 0, NI_NUMERICHOST);
	if (err != 0)
	{
		ms_error("ice.c: Error in getnameinfo");
		closesocket (sock);
		return -1;
	}
	closesocket (sock);
	/* ms_message("ice.c: Outgoing interface for sending STUN answer is %s", loc); */
	return 0;
}

static void
_ice_createErrorResponse(StunMessage *response, int cl, int number, const char* msg)
{
	response->msgHdr.msgType = (STUN_METHOD_BINDING | STUN_ERR_RESP);
	response->hasErrorCode = TRUE;
	response->errorCode.errorClass = cl;
	response->errorCode.number = number;
	strcpy(response->errorCode.reason, msg);
	response->errorCode.sizeReason = strlen(msg);
}

int ice_process_stun_message(RtpSession *session, struct IceCheckList *checklist, OrtpEvent *evt)
{
	struct CandidatePair *remote_candidates = NULL;
	int switch_to_address = -1;
	StunMessage msg;
	bool_t res;
	int highest_priority_success=-1;
	OrtpEventData *evt_data = ortp_event_get_data(evt);
	mblk_t *mp = evt_data->packet;
	struct sockaddr_in *udp_remote;
	char src6host[NI_MAXHOST];
	int recvport = 0;
	int i;

	udp_remote = (struct sockaddr_in*)&evt_data->ep->addr;

	memset( &msg, 0 , sizeof(msg) );
	res = stunParseMessage((char*)mp->b_rptr, mp->b_wptr-mp->b_rptr, &msg);
	if (!res)
	{
		ms_error("ice.c: Malformed STUN packet.");
		return -1;
	}

	if (checklist==NULL)
	{
		ms_error("ice.c: dropping STUN packet: ice is not configured");
		return -1;
	}

	remote_candidates = checklist->cand_pairs;
	if (remote_candidates==NULL)
	{
		ms_error("ice.c: dropping STUN packet: ice is not configured");
		return -1;
	}

	/* prepare ONCE tie-break value */
	if (checklist->tiebreak_value==0) {
		checklist->tiebreak_value = random() * (0x7fffffffffffffff/0x7fff);
	}

	memset (src6host, 0, sizeof (src6host));

	{
		struct sockaddr_storage *aaddr = (struct sockaddr_storage *)&evt_data->ep->addr;
		if (aaddr->ss_family==AF_INET)
			recvport = ntohs (((struct sockaddr_in *) udp_remote)->sin_port);
		else
			recvport = ntohs (((struct sockaddr_in6 *) &evt_data->ep->addr)->sin6_port);
	}
	i = getnameinfo ((struct sockaddr*)&evt_data->ep->addr, evt_data->ep->addrlen,
		src6host, NI_MAXHOST,
		NULL, 0, NI_NUMERICHOST);
	if (i != 0)
	{
		ms_error("ice.c: Error with getnameinfo");
		return -1;
	}

	if (STUN_IS_REQUEST(msg.msgHdr.msgType))
		ms_message("ice.c: STUN_CONNECTIVITYCHECK: Request received from: %s:%i",
		src6host, recvport);
	else if (STUN_IS_INDICATION(msg.msgHdr.msgType))
		ms_message("ice.c: SUN_INDICATION: Request Indication received from: %s:%i",
		src6host, recvport);
	else
		ms_message("ice.c: STUN_ANSWER: Answer received from: %s:%i",
		src6host, recvport);

	{
		int pos;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
		{
			struct CandidatePair *cand_pair = &remote_candidates[pos];

			if (cand_pair->connectivity_check == ICE_SUCCEEDED)
			{
				highest_priority_success=pos;
				break;
			}
		}
	}

	if (STUN_IS_INDICATION(msg.msgHdr.msgType))
	{
		return 0;
	}
	else if (STUN_IS_REQUEST(msg.msgHdr.msgType))
	{
		StunMessage resp;
		StunAtrString hmacPassword;
		StunAddress4 remote_addr;
		int rtp_socket;

		memset( &resp, 0 , sizeof(resp));
		remote_addr.addr = ntohl(udp_remote->sin_addr.s_addr);
		remote_addr.port = ntohs(udp_remote->sin_port);

		rtp_socket = rtp_session_get_rtp_socket(session);

		resp.msgHdr.magic_cookie = ntohl(msg.msgHdr.magic_cookie);
		for (i=0; i<12; i++ )
		{
			resp.msgHdr.tr_id.octet[i] = msg.msgHdr.tr_id.octet[i];
		}

		/* check mandatory params */

		if (!msg.hasUsername)
		{
			char buf[STUN_MAX_MESSAGE_SIZE];
			int len = sizeof(buf);
			ms_error("ice.c: STUN_CONNECTIVITYCHECK: Missing USERNAME attribute in connectivity check");
			_ice_createErrorResponse(&resp, 4, 32, "Missing USERNAME attribute");
			len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
			if (len)
				sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
			return -1;
		}
		if (!msg.hasMessageIntegrity)
		{
			char buf[STUN_MAX_MESSAGE_SIZE];
			int len = sizeof(buf);
			ms_error("ice.c: STUN_CONNECTIVITYCHECK: Missing MESSAGEINTEGRITY attribute in connectivity check");
			_ice_createErrorResponse(&resp, 4, 1, "Missing MESSAGEINTEGRITY attribute");
			len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
			if (len)
				sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
			return -1;
		}

		/*
		The password associated with that transport address ID is used to verify
		the MESSAGE-INTEGRITY attribute, if one was present in the request.
		*/
		char hmac[20];
		stunCalculateIntegrity_shortterm(hmac, (char*)mp->b_rptr, mp->b_wptr-mp->b_rptr-24, checklist->loc_ice_pwd);
		if (memcmp(msg.messageIntegrity.hash, hmac, 20)!=0)
		{
			char buf[STUN_MAX_MESSAGE_SIZE];
			int len = sizeof(buf);
			ms_error("ice.c: STUN_CONNECTIVITYCHECK: Wrong MESSAGEINTEGRITY attribute in connectivity check");
			_ice_createErrorResponse(&resp, 4, 1, "Wrong MESSAGEINTEGRITY attribute");
			len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
			if (len)
				sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
			return -1;
		}


		/* 7.2.1.1. Detecting and Repairing Role Conflicts */
		/* TODO */
		if (!msg.hasIceControlling && !msg.hasIceControlled)
		{
			char buf[STUN_MAX_MESSAGE_SIZE];
			int len = sizeof(buf);
			ms_error("ice.c: STUN_CONNECTIVITYCHECK: Missing either ICE-CONTROLLING or ICE-CONTROLLED attribute");
			_ice_createErrorResponse(&resp, 4, 87, "Missing either ICE-CONTROLLING or ICE-CONTROLLED attribute");
			len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
			if (len)
				sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
			return -1;
		}

		if (checklist->rem_controlling==0 && msg.hasIceControlling) {
			/* If the agent's tie-breaker is larger than or equal
			to the contents of the ICE-CONTROLLING attribute
			-> send 487, and do not change ROLE */
			if (checklist->tiebreak_value >= msg.iceControlling.value) {
				char buf[STUN_MAX_MESSAGE_SIZE];
				int len = sizeof(buf);
				ms_error("ice.c: STUN_CONNECTIVITYCHECK: 487 Role Conflict");
				_ice_createErrorResponse(&resp, 4, 87, "Role Conflict");
				len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
				if (len)
					sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
				return -1;
			}
			else {
				int pos;
				for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
				{
					/* controller agent */
					int G = remote_candidates[pos].remote_candidate.priority;
					/* controlled agent */	
					int D = remote_candidates[pos].local_candidate.priority;
					remote_candidates[pos].pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);

				}
				checklist->rem_controlling = 1;
				/* reset all to initial WAITING state? */
				ms_message("ice.c: STUN_CONNECTIVITYCHECK: tiebreaker -> reset all to ICE_WAITING state");
				for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
				{
					if (remote_candidates[pos].connectivity_check == ICE_PRUNED)
						continue;
					remote_candidates[pos].connectivity_check = ICE_WAITING;
					memset(&remote_candidates[pos].tid , 0, sizeof(remote_candidates[pos].tid));
					remote_candidates[pos].retransmission_time = 0;
					remote_candidates[pos].retransmission_number = 0;
				}
			}
		}

		if (checklist->rem_controlling==1 && msg.hasIceControlled) {

			/* If the agent's tie-breaker is larger than or equal
			to the contents of the ICE-CONTROLLED attribute
			-> change ROLE */
			if (checklist->tiebreak_value >= msg.iceControlled.value) {
				int pos;
				for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
				{
					/* controller agent */
					int G = remote_candidates[pos].local_candidate.priority;
					/* controlled agent */	
					int D = remote_candidates[pos].remote_candidate.priority;
					remote_candidates[pos].pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);

				}
				checklist->rem_controlling = 0;
				/* reset all to initial WAITING state? */
				ms_message("ice.c: STUN_CONNECTIVITYCHECK: tiebreaker -> reset all to ICE_WAITING state");
				for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
				{
					if (remote_candidates[pos].connectivity_check == ICE_PRUNED)
						continue;
					remote_candidates[pos].connectivity_check = ICE_WAITING;
					memset(&remote_candidates[pos].tid , 0, sizeof(remote_candidates[pos].tid));
					remote_candidates[pos].retransmission_time = 0;
					remote_candidates[pos].retransmission_number = 0;
				}
			}
			else {
				char buf[STUN_MAX_MESSAGE_SIZE];
				int len = sizeof(buf);
				ms_error("ice.c: STUN_CONNECTIVITYCHECK: 487 Role Conflict");
				_ice_createErrorResponse(&resp, 4, 87, "Role Conflict");
				len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
				if (len)
					sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
				return -1;
			}
		}

		/* 7.2.1.3. Learning Peer Reflexive Candidates */
#if 0
		if () /* found in table */
		{
			resp.hasPriority = TRUE;
			resp.priority.priority = msg.priority.priority;
		}
#endif

		struct CandidatePair *cand_pair;
		int pos;
		cand_pair=NULL;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
		{
			cand_pair = &remote_candidates[pos]; 
			/* connectivity check is coming from a known remote candidate?
			we should also check the port...
			*/
			if (strcmp(cand_pair->remote_candidate.conn_addr, src6host)==0
				&& cand_pair->remote_candidate.conn_port==recvport)
			{
				ms_message("ice.c: STUN_CONNECTIVITYCHECK: connectivity check received from a know candidate pair (%s:%i)", src6host, recvport);
				if (cand_pair->connectivity_check==ICE_FROZEN
					|| cand_pair->connectivity_check==ICE_IN_PROGRESS
					|| cand_pair->connectivity_check==ICE_FAILED)
				{
					ms_message("ice.c: STUN_CONNECTIVITYCHECK: know candidate pair reset to ICE_WAITING (%s:%i)", src6host, recvport);
					cand_pair->connectivity_check = ICE_WAITING;
					if (msg.hasUseCandidate==TRUE && checklist->rem_controlling==0)
						cand_pair->nominated_pair = 1;
				}
				else if (cand_pair->connectivity_check==ICE_SUCCEEDED)
				{
					if (msg.hasUseCandidate==TRUE && checklist->rem_controlling==0)
					{
						cand_pair->nominated_pair = 1;

						/* USE-CANDIDATE is in STUN request and we already succeeded on that link */
						ms_warning("ice.c: STUN_CONNECTIVITYCHECK: USE-CANDIDATE in request -> concluding ICE with remote RTP: %s:%i -> %s:%i\n",
							cand_pair->local_candidate.conn_addr,
							cand_pair->local_candidate.conn_port,
							src6host, recvport);
						memcpy(&session->rtp.rem_addr, &evt_data->ep->addr, evt_data->ep->addrlen);
						session->rtp.rem_addrlen=evt_data->ep->addrlen;
					}
				}
				break;
			}
			cand_pair=NULL;
		}
		if (cand_pair==NULL)
		{
			struct CandidatePair new_pair;
			memset(&new_pair, 0, sizeof(struct CandidatePair));

			ms_message("ice.c: STUN_CONNECTIVITYCHECK: connectivity check received from an unknow candidate (%s:%i)", src6host, recvport);
			/* TODO: add the peer reflexive candidate */

			memcpy(&new_pair.local_candidate, &remote_candidates[0].local_candidate, sizeof(new_pair.local_candidate));

			new_pair.remote_candidate.foundation = 6;
			new_pair.remote_candidate.component_id = remote_candidates[0].remote_candidate.component_id;

			/* -> no known base address for peer */

			new_pair.remote_candidate.conn_port = recvport;
			snprintf(new_pair.remote_candidate.conn_addr, sizeof(new_pair.remote_candidate.conn_addr),
				"%s", src6host);

			/* take it from PRIORITY STUN attr */
			new_pair.remote_candidate.priority = msg.priority.priority;
			if (new_pair.remote_candidate.priority==0)
				new_pair.remote_candidate.priority =  (UInt32)(pow((double)2,(double)24)*(110) + pow((double)2,(double)8)*(65535) + pow((double)2,(double)0)*(256 - new_pair.remote_candidate.component_id));

			snprintf(new_pair.remote_candidate.cand_type, sizeof(cand_pair->remote_candidate.cand_type),
				"prflx");
			snprintf (new_pair.remote_candidate.transport,
							sizeof (new_pair.remote_candidate.transport),
							"UDP");

			if (checklist->rem_controlling==0)
			{
				int G = new_pair.local_candidate.priority;
				/* controlled agent */	
				int D = new_pair.remote_candidate.priority;
				new_pair.pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);
			}
			else
			{
				int G = new_pair.remote_candidate.priority;
				/* controlled agent */	
				int D = new_pair.local_candidate.priority;
				new_pair.pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);
			}
			new_pair.connectivity_check = ICE_WAITING;
			/* insert new pair candidate */
			if (msg.hasUseCandidate==TRUE && checklist->rem_controlling==0)
			{
				new_pair.nominated_pair = 1;
			}

			for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
			{
				if (pos==9)
				{
					ms_message("ice.c: STUN_CONNECTIVITYCHECK: No room left for adding new remote reflexive candidate");
					break;
				}
				if (new_pair.pair_priority > remote_candidates[pos].pair_priority)
				{
					/* move upper data */
					memmove(&remote_candidates[pos+1], &remote_candidates[pos], sizeof(struct CandidatePair)*(10-pos-1));
					memcpy(&remote_candidates[pos], &new_pair, sizeof(struct CandidatePair));

					if (checklist->nominated_pair_index>=pos)
						checklist->nominated_pair_index++;
					ms_message("ice.c: STUN_CONNECTIVITYCHECK: new learned remote reflexive candidate added (pos=%i)", pos);
					break;
				}
			}
		}
#if 0
		int pos;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
		{
			struct CandidatePair *cand_pair = &remote_candidates[pos]; 

			if (cand_pair->connectivity_check == ICE_SUCCEEDED)
			{
				/* already found a valid check with highest priority */
				break;
			}
			if (strcmp(cand_pair->remote_candidate.conn_addr, src6host)==0)
			{
				/* working one way: use it */
				switch_to_address = pos;
				if (cand_pair->connectivity_check == RECV_VALID
					|| cand_pair->connectivity_check == VALID)
				{
					if (cand_pair->connectivity_check != VALID)
					{
						switch_to_address = pos;
						ms_message("ice.c: STUN_CONNECTIVITYCHECK: candidate id (index=%i) moved in VALID state (stunbindingrequest received).", pos);
						cand_pair->connectivity_check = VALID;
					}
				}
				else
					cand_pair->connectivity_check = SEND_VALID;

				/* we have a VALID one */
			}
		}
#endif


		UInt32 cookie = 0x2112A442;
		resp.hasXorMappedAddress = TRUE;
		resp.xorMappedAddress.ipv4.port = remote_addr.port^(cookie>>16);
		resp.xorMappedAddress.ipv4.addr = remote_addr.addr^cookie;


		resp.msgHdr.msgType = (STUN_METHOD_BINDING | STUN_SUCCESS_RESP);

		resp.hasUsername = TRUE;
		memcpy(resp.username.value, msg.username.value, msg.username.sizeValue );
		resp.username.sizeValue = msg.username.sizeValue;

		/* ? any messageintegrity in response? */
		resp.hasMessageIntegrity = TRUE;

		const char serverName[] = "mediastreamer2 " STUN_VERSION;
		resp.hasSoftware = TRUE;
		memcpy( resp.softwareName.value, serverName, sizeof(serverName));
		resp.softwareName.sizeValue = sizeof(serverName);

		{
			char buf[STUN_MAX_MESSAGE_SIZE];
			int len = sizeof(buf);
			len = stunEncodeMessage( &resp, buf, len, &hmacPassword );
			if (len)
				sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
		}
	}
	else if (STUN_IS_SUCCESS_RESP(msg.msgHdr.msgType))
	{
		/* set state to RECV-VALID or VALID */
		StunMessage resp;
		StunAddress4 mappedAddr;
		memset(&resp, 0, sizeof(StunMessage));
		res = stunParseMessage((char*)mp->b_rptr, mp->b_wptr-mp->b_rptr,
			&resp );
		if (!res)
		{
			ms_error("ice.c: SUCCESS_RESPONSE: Bad format for STUN answer.");
			return -1;
		}

		if (resp.hasXorMappedAddress!=TRUE)
		{
			ms_error("ice.c: SUCCESS_RESPONSE: Missing XOR-MAPPED-ADDRESS in STUN answer.");		  
			return -1;
		}

		UInt32 cookie = 0x2112A442;
		UInt16 cookie16 = 0x2112A442 >> 16;
		mappedAddr.port = resp.xorMappedAddress.ipv4.port^cookie16;
		mappedAddr.addr = resp.xorMappedAddress.ipv4.addr^cookie;

		struct in_addr inaddr;
		char mapped_addr[64];
		inaddr.s_addr = htonl (mappedAddr.addr);
		snprintf(mapped_addr, sizeof(mapped_addr),
			"%s", inet_ntoa (inaddr));

		int pos;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
		{
			struct CandidatePair *cand_pair = &remote_candidates[pos];

			if (memcmp(&(cand_pair->tid), &(resp.msgHdr.tr_id), sizeof(resp.msgHdr.tr_id))==0)
			{
				if (strcmp(src6host, cand_pair->remote_candidate.conn_addr)!=0
					|| recvport!=cand_pair->remote_candidate.conn_port)
				{
					/* 7.1.2.2.  Success Cases
					-> must be a security issue: refuse non-symmetric answer */
					cand_pair->connectivity_check = ICE_FAILED;
					break;
				}

				/* Youhouhouhou */
				if (cand_pair->connectivity_check != ICE_SUCCEEDED)
				{
					ms_message("ice.c: SUCCESS_RESPONSE: ICE_SUCCEEDED for candidate pair! %s:%i -> %s:%i",
						cand_pair->local_candidate.conn_addr,
						cand_pair->local_candidate.conn_port,
						cand_pair->remote_candidate.conn_addr,
						cand_pair->remote_candidate.conn_port);

					if (checklist->rem_controlling==1 && cand_pair->nominated_pair>0)
					{
						/* USE-CANDIDATE was in previous STUN request sent */
						ms_warning("ice.c: SUCCESS_RESPONSE: USE-CANDIDATE in answer -> concluding ICE with remote RTP: %s:%i -> %s:%i\n",
							cand_pair->local_candidate.conn_addr,
							cand_pair->local_candidate.conn_port,
							src6host, recvport);
						memcpy(&session->rtp.rem_addr, &evt_data->ep->addr, evt_data->ep->addrlen);
						session->rtp.rem_addrlen=evt_data->ep->addrlen;
					}

					if (cand_pair->nominated_pair > 1 && checklist->rem_controlling==0)
					{
						/* USE-CANDIDATE is in STUN request and we already succeeded on that link */
						ms_warning("ice.c: SUCCESS_RESPONSE: answer on nominated pair -> concluding ICE with remote RTP: %s:%i -> %s:%i\n",
							cand_pair->local_candidate.conn_addr,
							cand_pair->local_candidate.conn_port,
							src6host, recvport);
						memcpy(&session->rtp.rem_addr, &evt_data->ep->addr, evt_data->ep->addrlen);
						session->rtp.rem_addrlen=evt_data->ep->addrlen;
					}

					cand_pair->connectivity_check = ICE_SUCCEEDED;
					if (mappedAddr.port == cand_pair->local_candidate.conn_port
						&& strcmp(mapped_addr, cand_pair->local_candidate.conn_addr)==0)
						ms_message("ice.c: SUCCESS_RESPONSE: no peer reflexive candidate was discovered (%s:%i)", src6host, recvport);
					else
					{
						int pos2;
						for (pos2=0;pos2<10 && remote_candidates[pos2].remote_candidate.conn_addr[0]!='\0';pos2++)
						{
							if (mappedAddr.port == remote_candidates[pos2].local_candidate.conn_port
								&& strcmp(mapped_addr, remote_candidates[pos2].local_candidate.conn_addr)==0
								&& cand_pair->remote_candidate.conn_port == remote_candidates[pos2].remote_candidate.conn_port
								&& strcmp(cand_pair->remote_candidate.conn_addr, remote_candidates[pos2].remote_candidate.conn_addr)==0)
							{
								if (remote_candidates[pos2].connectivity_check==ICE_PRUNED
									||remote_candidates[pos2].connectivity_check==ICE_FROZEN
									||remote_candidates[pos2].connectivity_check==ICE_FAILED
									|| remote_candidates[pos2].connectivity_check==ICE_IN_PROGRESS)
									remote_candidates[pos2].connectivity_check = ICE_WAITING; /* trigger check */
								ms_message("ice.c: SUCCESS_RESPONSE: found candidate pair matching XOR-MAPPED-ADDRESS (local candidate: %s:%i:%s)",
									remote_candidates[pos2].local_candidate.conn_addr,
									remote_candidates[pos2].local_candidate.conn_port,
									remote_candidates[pos2].local_candidate.cand_type);
								break;
							}
						}
						if (pos2==10 || remote_candidates[pos2].remote_candidate.conn_addr[0]=='\0')
						{
							struct CandidatePair new_pair;
							memset(&new_pair, 0, sizeof(struct CandidatePair));

							/* 7.1.2.2.1.  Discovering Peer Reflexive Candidates */
							ms_message("ice.c: SUCCESS_RESPONSE: a local peer reflexive candidate has been discovered (%s:%i)", src6host, recvport);
							/* If IP & port were different than mappedAddr, there was A NAT
							between me and remote destination. */
							memcpy(&new_pair.remote_candidate, &cand_pair->remote_candidate, sizeof(new_pair.remote_candidate));

							new_pair.local_candidate.foundation = 6;
							new_pair.local_candidate.component_id = cand_pair->local_candidate.component_id;

							/* what is my base address? */
							new_pair.local_candidate.rel_port = cand_pair->local_candidate.conn_port;
							snprintf(new_pair.local_candidate.rel_addr, sizeof(new_pair.local_candidate.rel_addr),
								"%s", cand_pair->local_candidate.conn_addr);

							new_pair.local_candidate.conn_port = mappedAddr.port;
							snprintf(new_pair.local_candidate.conn_addr, sizeof(new_pair.local_candidate.conn_addr),
								"%s", mapped_addr);

							/* take it from PRIORITY STUN attr */
							new_pair.local_candidate.priority = (UInt32)(pow((double)2,(double)24)*(110) + pow((double)2,(double)8)*(65535) + pow((double)2,(double)0)*(256 - new_pair.local_candidate.component_id));

							snprintf(new_pair.local_candidate.cand_type, sizeof(cand_pair->local_candidate.cand_type),
								"prflx");
							snprintf (new_pair.local_candidate.transport,
											sizeof (new_pair.local_candidate.transport),
											"UDP");

							if (checklist->rem_controlling==0)
							{
								int G = new_pair.local_candidate.priority;
								/* controlled agent */	
								int D = new_pair.remote_candidate.priority;
								new_pair.pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);
							}
							else
							{
								int G = new_pair.remote_candidate.priority;
								/* controlled agent */	
								int D = new_pair.local_candidate.priority;
								new_pair.pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);
							}
							new_pair.connectivity_check = ICE_WAITING;
							/* insert new pair candidate */
							for (pos2=0;pos2<10 && remote_candidates[pos2].remote_candidate.conn_addr[0]!='\0';pos2++)
							{
								if (pos2==9)
								{
									ms_message("ice.c: SUCCESS_RESPONSE: No room left for adding new local peer reflexive candidate");
									break;
								}
								if (new_pair.pair_priority > remote_candidates[pos2].pair_priority)
								{
									/* move upper data */
									memmove(&remote_candidates[pos2+1], &remote_candidates[pos2], sizeof(struct CandidatePair)*(10-pos2-1));
									memcpy(&remote_candidates[pos2], &new_pair, sizeof(struct CandidatePair));

									if (checklist->nominated_pair_index>=pos2)
										checklist->nominated_pair_index++;
									ms_message("ice.c: SUCCESS_RESPONSE: new discovered local peer reflexive candidate added (pos=%i)", pos2);
									break;
								}
							}

						}
					}
				}
			}
		}


	}
	else if (STUN_IS_ERR_RESP(msg.msgHdr.msgType))
	{
		StunMessage resp;
		memset(&resp, 0, sizeof(StunMessage));
		res = stunParseMessage((char*)mp->b_rptr, mp->b_wptr-mp->b_rptr,
			&resp );
		if (!res)
		{
			ms_error("ice.c: ERROR_RESPONSE: Bad format for STUN answer.");
			return -1;
		}

		int pos;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
		{
			struct CandidatePair *cand_pair = &remote_candidates[pos];

			if (memcmp(&(cand_pair->tid), &(resp.msgHdr.tr_id), sizeof(resp.msgHdr.tr_id))==0)
			{
				cand_pair->connectivity_check = ICE_FAILED;
				ms_message("ice.c: ERROR_RESPONSE: ICE_FAILED for candidate pair! %s:%i -> %s:%i",
					cand_pair->local_candidate.conn_addr,
					cand_pair->local_candidate.conn_port,
					cand_pair->remote_candidate.conn_addr,
					cand_pair->remote_candidate.conn_port);
				if (resp.hasErrorCode==TRUE && resp.errorCode.errorClass==4 && resp.errorCode.number==87)
				{
					if (remote_candidates[pos].rem_controlling==1)
					{
						int pos2;
						for (pos2=0;pos2<10 && remote_candidates[pos2].remote_candidate.conn_addr[0]!='\0';pos2++)
						{
							/* controller agent */
							int G = remote_candidates[pos2].local_candidate.priority;
							/* controlled agent */	
							int D = remote_candidates[pos2].remote_candidate.priority;
							remote_candidates[pos2].pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);
						}
						checklist->rem_controlling=0;
					}
					else
					{
						int pos2;
						for (pos2=0;pos2<10 && remote_candidates[pos2].remote_candidate.conn_addr[0]!='\0';pos2++)
						{
							/* controller agent */
							int G = remote_candidates[pos2].remote_candidate.priority;
							/* controlled agent */	
							int D = remote_candidates[pos2].local_candidate.priority;
							remote_candidates[pos2].pair_priority = (UInt64)pow((double)2,(double)32)*MIN(G,D) + 2*MAX(G,D) + (G>D?1:0);
						}
						checklist->rem_controlling=1;
					}
					/* reset all to initial WAITING state? */
					ms_message("ice.c: ERROR_RESPONSE: 487 -> reset all to ICE_WAITING state");
					for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
					{
						if (remote_candidates[pos].connectivity_check == ICE_PRUNED)
							continue;
						remote_candidates[pos].connectivity_check = ICE_WAITING;
						memset(&remote_candidates[pos].tid , 0, sizeof(remote_candidates[pos].tid));
						remote_candidates[pos].retransmission_time = 0;
						remote_candidates[pos].retransmission_number = 0;
					}
				}
			}
		}
	}

#if 0
	if (switch_to_address == -1)
		return 0;

	/* skip symmetric RTP if any previous connection is working */
	if (switch_to_address<highest_priority_success || highest_priority_success==-1)
	{
		/* rtp_in_direct_mode = 1; */
		/* current destination address: snprintf(rtp_remote_addr, 256, "%s:%i", src6host, recvport); */
		ms_warning("ice.c: Modifying remote destination for RTP stream (%s:%i)\n",
			src6host, recvport);
		memcpy(&session->rtp.rem_addr, &evt_data->ep->addr, evt_data->ep->addrlen);
		session->rtp.rem_addrlen=evt_data->ep->addrlen;
	}
#endif
	return 0;
}

