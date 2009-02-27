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
ice_sendtest( struct CandidatePair *remote_candidate, Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              int testNum, bool_t verbose , UInt96 *tid);

static void 
ice_sendtest( struct CandidatePair *remote_candidate, Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              int testNum, bool_t verbose , UInt96 *tid)
{	
   bool_t changePort=FALSE;
   bool_t changeIP=FALSE;
   bool_t discard=FALSE;

   StunMessage req;
   char buf[STUN_MAX_MESSAGE_SIZE];
   int len = STUN_MAX_MESSAGE_SIZE;
   
   switch (testNum)
   {
      case 1:
      case 10:
      case 11:
         break;
      case 2:
         /* changePort=TRUE; */
         changeIP=TRUE;
         break;
      case 3:
         changePort=TRUE;
         break;
      case 4:
         changeIP=TRUE;
         break;
      case 5:
         discard=TRUE;
         break;
      default:
         printf("Test %i is unkown\n", testNum);
         return ; /* error */
   }
   
   memset(&req, 0, sizeof(StunMessage));

   stunBuildReqSimple( &req, username, 
                       changePort , changeIP , 
                       testNum );
   req.hasMessageIntegrity=TRUE;

   req.hasPriority = TRUE;
   req.priority.priority = (UInt32)(pow((double)2,(double)24)*(110) + pow((double)2,(double)8)*(65535) + pow((double)2,(double)0)*(256 - remote_candidate->remote_candidate.component_id));

   /* TODO: put this parameter only for the candidate selected */
   if (remote_candidate->connectivity_check==VALID)
	   req.hasUseCandidate = TRUE;

   if (remote_candidate->rem_controlvalue==0)
	   {
		   /* calculated once only */
		   remote_candidate->rem_controlvalue = random();
		   remote_candidate->rem_controlvalue = remote_candidate->rem_controlvalue >> 32;
		   remote_candidate->rem_controlvalue = random();
	   }
   
   if (remote_candidate->rem_controlling==1)
	   {
		   req.hasIceControlled = TRUE;
		   req.iceControlled.value = remote_candidate->rem_controlvalue;
	   }
   else
	   {
		   req.hasIceControlling = TRUE;
		   req.iceControlling.value	= remote_candidate->rem_controlvalue;
	   }

   /* TODO: not yet implemented? */
   req.hasFingerprint = TRUE;
   
   len = stunEncodeMessage( &req, buf, len, password );

   memcpy(tid , &(req.msgHdr.tr_id), sizeof(req.msgHdr.tr_id));

   sendMessage( myFd, buf, len, dest->addr, dest->port );	
}

int ice_sound_send_stun_request(RtpSession *session, struct CandidatePair *remote_candidates, int round)
{
	int roll=250;
#if 0
    /* in "passive" mode (UA not behind a NATor behind a full cone NAT),
    wait a few delay before sending the first STUN request:
    this help to traverse */
    if (session->setup_passive>0)
    {
        return 0;
    }
#endif

	if (remote_candidates==NULL)
		return 0;

	if (round>500)
		roll=2*roll;

    if (round%roll==50 || round==10)
    {
        int pos;

#if 0
        /* do this only with application that support this */
        if (osip_strncasecmp(remote_useragent, "linphone/", 8)!=0)
        {
            /* use stun only with linphone to linphone softphone */
            return 0;
        }
#endif

        for (pos=0;pos<1 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
        {
            int media_socket = rtp_session_get_rtp_socket(session);
            StunAddress4 stunServerAddr;
            StunAtrString username;
            StunAtrString password;
            bool_t res;
#if 0
            int  pad_size;
#endif
			
            struct CandidatePair *cand_pair = &remote_candidates[pos]; 
            username.sizeValue = 0;
            password.sizeValue = 0;

			/* username comes from "ice-ufrag" (rfrag:lfrag) */
			/* ufrag and pwd are in first row only */
            snprintf(username.value, sizeof(username.value), "%s:%s",
                remote_candidates[0].rem_ice_ufrag,
                remote_candidates[0].loc_ice_ufrag);
            username.sizeValue = (UInt16)strlen(username.value);

			
            snprintf(password.value, sizeof(password.value), "%s",
                remote_candidates[0].rem_ice_pwd);
            password.sizeValue = (UInt16)strlen(password.value);


            res = stunParseServerName(cand_pair->remote_candidate.conn_addr,
                &stunServerAddr);
            if ( res == TRUE )
            {
                stunServerAddr.port = cand_pair->remote_candidate.conn_port;
                ice_sendtest(&remote_candidates[pos], media_socket, &stunServerAddr, &username, &password, 1, 0/*FALSE*/,
                    &(cand_pair->tid));
            }
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
}

int ice_process_stun_message(RtpSession *session, struct CandidatePair *remote_candidates, OrtpEvent *evt)
{
    int switch_to_address = -1;
    StunMessage msg;
    bool_t res;
    int already_worked_once=-1;
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

	if (remote_candidates==NULL)
    {
        ms_error("ice.c: dropping STUN packet: ice is not configured");
        return -1;
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
    } else
    {
	    if (STUN_IS_REQUEST(msg.msgHdr.msgType))
		    ms_message("ice.c: Request received from: %s:%i",
			            src6host, recvport);
		else
		    ms_message("ice.c: Answer received from: %s:%i",
			            src6host, recvport);
    }

	{
        int pos;
        for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
        {
            struct CandidatePair *cand_pair = &remote_candidates[pos];
#ifdef RESTRICTIVE_ICE
            if (cand_pair->connectivity_check == VALID
                ||cand_pair->connectivity_check == RECV_VALID)
            {
                already_worked_once=pos;
                break;
            }
#else
            if (cand_pair->connectivity_check == VALID
                ||cand_pair->connectivity_check == RECV_VALID
		||cand_pair->connectivity_check == SEND_VALID)
            {
                already_worked_once=pos;
                break;
            }
#endif
        }
    }

	if (STUN_IS_REQUEST(msg.msgHdr.msgType))
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
			 ms_error("Missing USERNAME attribute in connectivity check");
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
			 ms_error("Missing MESSAGEINTEGRITY attribute in connectivity check");
			 _ice_createErrorResponse(&resp, 4, 1, "Missing MESSAGEINTEGRITY attribute");
			 len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
			 if (len)
				 sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
			 return -1;
		 }

		if (remote_candidates==NULL)
			{
			 char buf[STUN_MAX_MESSAGE_SIZE];
			 int len = sizeof(buf);
			 ms_error("no password for checking MESSAGEINTEGRITY");
			 _ice_createErrorResponse(&resp, 4, 1, "no password for checking MESSAGEINTEGRITY");
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
		stunCalculateIntegrity_shortterm(hmac, (char*)mp->b_rptr, mp->b_wptr-mp->b_rptr-24, remote_candidates[0].loc_ice_pwd);
		if (memcmp(msg.messageIntegrity.hash, hmac, 20)!=0)
			{
			 char buf[STUN_MAX_MESSAGE_SIZE];
			 int len = sizeof(buf);
			 ms_error("Wrong MESSAGEINTEGRITY attribute in connectivity check (%s)", msg.messageIntegrity.hash, hmac);
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
			 ms_error("Missing either ICE-CONTROLLING or ICE-CONTROLLED attribute");
			 _ice_createErrorResponse(&resp, 4, 87, "Missing either ICE-CONTROLLING or ICE-CONTROLLED attribute");
			 len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
			 if (len)
				 sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
			 return -1;
		 }
		
		if ((remote_candidates[0].rem_controlling==0 && msg.hasIceControlling)
			||(remote_candidates[0].rem_controlling==1 && msg.hasIceControlled))
			{
			 char buf[STUN_MAX_MESSAGE_SIZE];
			 int len = sizeof(buf);
			 ms_error("487 Role Conflict");
			 _ice_createErrorResponse(&resp, 4, 87, "Role Conflict");
			 len = stunEncodeMessage(&resp, buf, len, &hmacPassword );
			 if (len)
				 sendMessage( rtp_socket, buf, len, remote_addr.addr, remote_addr.port);
			 return -1;
			}		



		/* 7.2.1.3. Learning Peer Reflexive Candidates */
#if 0
		if () /* found in table */
			{
				resp.hasPriority = TRUE;
				resp.priority.priority = msg.priority.priority;
			}
#endif

		/* TODO: the current algo is not ICE, but give similar results */
		int pos;
		for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
			{
				struct CandidatePair *cand_pair = &remote_candidates[pos]; 
				
				if (cand_pair->connectivity_check == VALID)
					{
						/* already found a valid check with highest priority */
						break;
					}
				
				/* connectivity check is coming from a known remote candidate?
				   we should also check the port...
				 */
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
										ms_message("ice.c: candidate id (index=%i) moved in VALID state (stunbindingrequest received).", pos);
										cand_pair->connectivity_check = VALID;
									}
							}
						else
							cand_pair->connectivity_check = SEND_VALID;
						
						/* we have a VALID one */
					}
			}
		
		
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
            ms_error("ice.c: Bad format for STUN answer.");
            return -1;
        }

        mappedAddr = resp.mappedAddress.ipv4;

	    if (remote_candidates!=NULL) {
            int pos;
            for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.conn_addr[0]!='\0';pos++)
            {
                struct CandidatePair *cand_pair = &remote_candidates[pos];

                if (memcmp(&(cand_pair->tid), &(resp.msgHdr.tr_id), sizeof(resp.msgHdr.tr_id))==0)
                {
                    /* Youhouhouhou */
                    if (cand_pair->connectivity_check != VALID)
                    {
                        switch_to_address = pos;
                    }
#if 0
					ms_message("ice.c: Valid STUN answer received (to=%s:%i from=%s:%i)",
                        cand_pair->remote_candidate.ipaddr, cand_pair->remote_candidate.conn_port,
                        src6host, recvport);
#endif
                    if (cand_pair->connectivity_check == SEND_VALID
                        || cand_pair->connectivity_check == VALID)
                    {
                        if (cand_pair->connectivity_check != VALID)
                        {
                            ms_message("ice.c: Switch to VALID mode for (to=%s:%i from=%s:%i)",
                                cand_pair->remote_candidate.conn_addr, cand_pair->remote_candidate.conn_port,
                                src6host, recvport);
                            cand_pair->connectivity_check = VALID;
                        }
                    }
                    else
                        cand_pair->connectivity_check = RECV_VALID;
                }
            }
        }
    }

    if (remote_candidates==NULL) {
        ms_warning("ice.c: STUN connectivity check is disabled but we received a STUN message (%s:%i)\n",
            src6host, recvport);
		return 0;
	}
	if (switch_to_address == -1)
        return 0;

	{
        /* skip symmetric RTP if any previous connection is working */
        if (switch_to_address<already_worked_once || already_worked_once==-1)
        {
            /* rtp_in_direct_mode = 1; */
            /* current destination address: snprintf(rtp_remote_addr, 256, "%s:%i", src6host, recvport); */
            ms_warning("ice.c: Modifying remote destination for RTP stream (%s:%i)\n",
                src6host, recvport);
            memcpy(&session->rtp.rem_addr, &evt_data->ep->addr, evt_data->ep->addrlen);
			session->rtp.rem_addrlen=evt_data->ep->addrlen;
        }
    }
    return 0;
}

