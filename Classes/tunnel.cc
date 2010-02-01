/*
*  C Implementation: tunnel
*
* Description: 
*
*
* Author: Simon Morlat <simon.morlat@linphone.org>, (C) 2009
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include "axtunnel/client.hh"

#include <ortp/rtpsession.h>
#include "linphonecore.h"

/*
remember to build eXosip with:
make DEFS="-DHAVE_CONFIG_H -Drecvfrom=eXosip_recvfrom -Dsendto=eXosip_sendto -Dselect=eXosip_select"
*/

using namespace axtel;

static TunnelSocket *sip_socket;
static TunnelSocket *rtp_socket=0;
static TunnelClient* linphone_iphone_tun=0;
static bool linphone_iphone_tun_enable=false;

extern "C" void linphone_iphone_log_handler(int lev, const char *fmt, va_list args);

extern "C" int eXosip_sendto(int fd,const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){
	if(linphone_iphone_tun == 0 || !linphone_iphone_tun_enable) {
		return sendto(fd, buf, len, flags, to, tolen);
	} else {
		if (sip_socket==NULL) sip_socket=linphone_iphone_tun->createSocket(5060);
		return sip_socket->sendto(buf,len,to,tolen);
	}
}

extern "C" int eXosip_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen){
	if(linphone_iphone_tun == 0 || !linphone_iphone_tun_enable ) {
		return recvfrom(fd,buf,len,flags,from,fromlen);
	} else {
		if (sip_socket==NULL) sip_socket=linphone_iphone_tun->createSocket(5060);
		return sip_socket->recvfrom(buf,len,from,*fromlen);
	}
}

extern "C" int eXosip_select(int nfds, fd_set *s1, fd_set *s2, fd_set *s3, struct timeval *tv){
	if(linphone_iphone_tun == 0 || !linphone_iphone_tun_enable) {
		return select(nfds,s1,s2,s3,tv); 
	} else {
		struct timeval begin,cur;
		if (sip_socket==NULL) sip_socket=linphone_iphone_tun->createSocket(5060);
		if (tv!=0 && tv->tv_sec){
			unsigned int i;
			fd_set tmp;
			
			for(i=0;i<sizeof(*s1)*8;++i){
				/*hack for the wakeup fd of eXosip: we don't know its number but the important
				 is that it does not block*/
				if (FD_ISSET(i,s1)){
					fcntl(i,F_SETFL,O_NONBLOCK);
				}
			}
			/*this is the select from udp.c, the one that is interesting to us*/
			gettimeofday(&begin,NULL);
			do{
				struct timeval abit;
				abit.tv_sec=0;
				abit.tv_usec=20000;
				if (sip_socket->hasData()) return 1;
				gettimeofday(&cur,NULL);
				if (cur.tv_sec-begin.tv_sec>tv->tv_sec) return 0;
				memcpy(&tmp,s1,sizeof(tmp));
				if (select(nfds,&tmp,s2,s3,&abit)==1) return 2;
			}while(1);
			
		}else{
			return select(nfds,s1,s2,s3,tv);
		}
	}
}

static int audio_sendto(struct _RtpTransport *t, mblk_t *msg , int flags, const struct sockaddr *to, socklen_t tolen){
	int size;
	msgpullup(msg,-1);
	size=msgdsize(msg);
	rtp_socket->sendto(msg->b_rptr,size,to,tolen);
	return size;
}

static int audio_recvfrom(struct _RtpTransport *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen){
	int err=rtp_socket->recvfrom(msg->b_wptr,msg->b_datap->db_lim-msg->b_datap->db_base,from,*fromlen);
	if (err>0) return err;
	return 0;
}


static RtpTransport audio_transport={
	NULL,
	NULL,
	audio_sendto,
	audio_recvfrom
};


extern "C" void linphone_iphone_tunneling_init(const char* ip,unsigned int port,bool isDebug){
	if (isDebug) {
		SetLogHandler(&linphone_iphone_log_handler);
		SetLogLevel(AXTUNNEL_ERROR|AXTUNNEL_WARN);
	} else {
		SetLogLevel(0);
	}
	linphone_iphone_tun = new TunnelClient(ip,port); 
}

extern "C" void linphone_iphone_enable_tunneling(LinphoneCore* lc){ 
	if (rtp_socket==0) rtp_socket=linphone_iphone_tun->createSocket(7078);
	linphone_core_set_audio_transports(lc,&audio_transport,NULL);
	linphone_iphone_tun_enable = true;
}

extern "C" void linphone_iphone_disable_tunneling(LinphoneCore* lc){
	linphone_iphone_tun_enable = false;
	linphone_core_set_audio_transports(lc,NULL,NULL);
}

extern "C" int linphone_iphone_tunneling_isready(){
	return (linphone_iphone_tun!=0) && linphone_iphone_tun->isReady();
}

