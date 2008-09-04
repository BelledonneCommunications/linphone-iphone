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

static void 
ice_sendtest( Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              int testNum, bool_t verbose , UInt128 *tid);

static void 
ice_sendtest( Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              int testNum, bool_t verbose , UInt128 *tid)
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
	
   len = stunEncodeMessage( &req, buf, len, password,verbose );

   memcpy(tid , &(req.msgHdr.id), sizeof(req.msgHdr.id));

   sendMessage( myFd, buf, len, dest->addr, dest->port, verbose );	
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

        for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.ipaddr[0]!='\0';pos++)
        {
            int media_socket = rtp_session_get_rtp_socket(session);
            StunAddress4 stunServerAddr;
            StunAtrString username;
            StunAtrString password;
            bool_t res;
            int  pad_size;

            struct CandidatePair *cand_pair = &remote_candidates[pos]; 
            username.sizeValue = 0;
            password.sizeValue = 0;

            /* set username to L3:1:R2:1 */
            snprintf(username.value, sizeof(username.value), "%s:%i:%s:%i",
                cand_pair->local_candidate.candidate_id,
                1,
                cand_pair->remote_candidate.candidate_id,
                1);
            username.sizeValue = (UInt16)strlen(username.value);
            pad_size = username.sizeValue % 4;

            username.value[username.sizeValue]='\0';
            username.value[username.sizeValue+1]='\0';
            username.value[username.sizeValue+2]='\0';
            username.value[username.sizeValue+3]='\0';

            username.sizeValue = username.sizeValue + 4 - pad_size;

            snprintf(password.value, sizeof(password.value), "%s",
                cand_pair->remote_candidate.password);
            password.sizeValue = (UInt16)strlen(password.value);

#if 0
            pad_size = password.sizeValue%4;
            password.value[password.sizeValue]='\0';
            password.value[password.sizeValue+1]='\0';
            password.value[password.sizeValue+2]='\0';
            password.value[password.sizeValue+3]='\0';
            password.sizeValue = password.sizeValue + pad_size;
#endif

            res = stunParseServerName(cand_pair->remote_candidate.ipaddr,
                &stunServerAddr);
            if ( res == TRUE )
            {
                stunServerAddr.port = cand_pair->remote_candidate.port;
                ice_sendtest(media_socket, &stunServerAddr, &username, &password, 1, 0/*FALSE*/,
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
    res = stunParseMessage((char*)mp->b_rptr, mp->b_wptr-mp->b_rptr, &msg, 0);
    if (!res)
    {
        ms_error("ice.c: Malformed STUN packet.");
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
	    if (msg.msgHdr.msgType == BindRequestMsg)
		    ms_message("ice.c: Request received from: %s:%i",
			            src6host, recvport);
		else
		    ms_message("ice.c: Answer received from: %s:%i",
			            src6host, recvport);
    }

    if (remote_candidates!=NULL) {
        int pos;
        for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.ipaddr[0]!='\0';pos++)
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

    if (msg.msgHdr.msgType == BindRequestMsg)
    {
        StunMessage resp;
        StunAddress4 dest;
        StunAtrString hmacPassword;
        StunAddress4 from;
        StunAddress4 secondary;
        StunAddress4 myAddr;
        StunAddress4 myAltAddr;
        bool_t changePort = FALSE;
        bool_t changeIp = FALSE;
        struct sockaddr_storage name;
        socklen_t namelen;
        char localip[128];
        int rtp_socket;
        memset(&name, '\0', sizeof(struct sockaddr_storage));
        memset(localip, '\0', sizeof(localip));
        _ice_get_localip_for ((struct sockaddr_storage*)&evt_data->ep->addr, evt_data->ep->addrlen, localip, 128);

        from.addr = ntohl(udp_remote->sin_addr.s_addr);
        from.port = ntohs(udp_remote->sin_port);
        
        secondary.addr = 0;
        secondary.port = 0;

        namelen = sizeof(struct sockaddr_storage);
        rtp_socket = rtp_session_get_rtp_socket(session);
        i = getsockname(rtp_socket, (struct sockaddr*)&name, &namelen);
        if (i!=0)
        {
            ms_error("ice.c: getsockname failed.");
            return -1;
        }

        myAddr.port = ntohs (((struct sockaddr_in*)&name)->sin_port);
        i = stunParseHostName(localip, &myAddr.addr, &myAddr.port, myAddr.port);
        if (!i)
        {
            ms_error("ice.c: stunParseHostName failed.");
            return -1;
        }
        myAddr.port = ntohs (((struct sockaddr_in*)&name)->sin_port);
    
        /* changed-address set to local address/port */
        myAltAddr = myAddr;
        dest.addr = 0;
        dest.port = 0;

        res = stunServerProcessMsg((char*)mp->b_rptr, mp->b_wptr-mp->b_rptr,
            &from,
            &secondary,
            &myAddr,
            &myAltAddr, 
            &resp,
            &dest,
            &hmacPassword,
            &changePort,
            &changeIp,
            FALSE );

        if (!res)
        {
            ms_error("ice.c: Failed to process STUN request.");
            return -1;
        }

        if (changePort == TRUE || changeIp == TRUE)
        {
            ms_error("ice.c: STUN request with changePort or changeIP refused.");
            return -1;
        }

        res=TRUE;
        if ( dest.addr == 0 ) res=FALSE;
        if ( dest.port == 0 ) res=FALSE;
        if (!res)
        {
            ms_error("ice.c: Missing destination value for response.");
            return -1;
        }

    
        if (msg.hasUsername!=TRUE || msg.username.sizeValue<=0)
        {
            /* reply 430 */
            ms_error("ice.c: Missing or bad username value.");
            return -1;
        }

        /*
        USERNAME is considered valid if its topmost portion (the part up to,
        but not including the second colon) corresponds to a transport address
        ID known to the agent.
        */
	    if (remote_candidates!=NULL) {
            int pos;
            for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.ipaddr[0]!='\0';pos++)
            {
                char username[256];
                struct CandidatePair *cand_pair = &remote_candidates[pos]; 
                size_t len = strlen(cand_pair->remote_candidate.candidate_id);

                if (cand_pair->connectivity_check == VALID)
                {
                    break;
                }

                memset(username, '\0', sizeof(username));
                snprintf(username, sizeof(username), "%s:%i:%s:%i",
                    cand_pair->remote_candidate.candidate_id,
                    1,
                    cand_pair->local_candidate.candidate_id,
                    1);

                if (len+3<msg.username.sizeValue
                    && strncmp(msg.username.value, cand_pair->remote_candidate.candidate_id, len)==0)
                {
                    char tmp[10];
                    int k;
                    snprintf(tmp, 10, "%s", msg.username.value + len +1);
                    for (k=0;k<10;k++)
                    {
                        if (tmp[k]=='\0')
                            break;
                        if (tmp[k]==':')
                        {
                            tmp[k]='\0';
                            break;
                        }
                    }
                    k = atoi(tmp);
                    /* TODO support for 2 stream RTP+RTCP */
                    if (k>0 && k<10 && k==1)
                    {
                        /* candidate-id found! */
#if 0
                        ms_message("ice.c: Find candidate id (index=%i) for incoming STUN request.", pos);
#endif

                        if (strncmp(msg.username.value, username, strlen(username))==0)
                        {
#ifdef RESTRICTIVE_ICE
                                ms_message("ice.c: Valid STUN request received (to=%s:%i from=%s:%i).",
                                cand_pair->remote_candidate.ipaddr, cand_pair->remote_candidate.port,
                                src6host, recvport);
				/* We can't be sure the remote end will receive our answer:
				connection could be only one way...
				*/
				if (cand_pair->connectivity_check != VALID)
                            {
                                switch_to_address = pos;
                            }
#else
				switch_to_address = pos;
#endif
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
                }
            }
        }
        
        /*
        The password associated with that transport address ID is used to verify
        the MESSAGE-INTEGRITY attribute, if one was present in the request.
        */


        {
            char buf[STUN_MAX_MESSAGE_SIZE];            
            int len = sizeof(buf);
            len = stunEncodeMessage( &resp, buf, len, &hmacPassword,FALSE );
            if (len)
                sendMessage( rtp_socket, buf, len, dest.addr, dest.port, FALSE );
        }
    }
    else
    {
        /* set state to RECV-VALID or VALID */
        StunMessage resp;
        StunAddress4 mappedAddr;
        memset(&resp, 0, sizeof(StunMessage));
        res = stunParseMessage((char*)mp->b_rptr, mp->b_wptr-mp->b_rptr,
            &resp, FALSE );
        if (!res)
        {
            ms_error("ice.c: Bad format for STUN answer.");
            return -1;
        }

        mappedAddr = resp.mappedAddress.ipv4;

	    if (remote_candidates!=NULL) {
            int pos;
            for (pos=0;pos<10 && remote_candidates[pos].remote_candidate.ipaddr[0]!='\0';pos++)
            {
                struct CandidatePair *cand_pair = &remote_candidates[pos];

                if (memcmp(&(cand_pair->tid), &(resp.msgHdr.id), sizeof(resp.msgHdr.id))==0)
                {
                    /* Youhouhouhou */
                    if (cand_pair->connectivity_check != VALID)
                    {
                        switch_to_address = pos;
                    }
#if 0
					ms_message("ice.c: Valid STUN answer received (to=%s:%i from=%s:%i)",
                        cand_pair->remote_candidate.ipaddr, cand_pair->remote_candidate.port,
                        src6host, recvport);
#endif
                    if (cand_pair->connectivity_check == SEND_VALID
                        || cand_pair->connectivity_check == VALID)
                    {
                        if (cand_pair->connectivity_check != VALID)
                        {
                            ms_message("ice.c: Switch to VALID mode for (to=%s:%i from=%s:%i)",
                                cand_pair->remote_candidate.ipaddr, cand_pair->remote_candidate.port,
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
            ms_warning("ice.c: Modifying remote socket: symmetric RTP (%s:%i)\n",
                src6host, recvport);
            memcpy(&session->rtp.rem_addr, &evt_data->ep->addr, evt_data->ep->addrlen);
			session->rtp.rem_addrlen=evt_data->ep->addrlen;
        }
    }
    return 0;
}

