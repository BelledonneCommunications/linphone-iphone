/*
 *  C Implementation: tunnel
 *
 * Description: 
 *
 *
 * Author: Simon Morlat <simon.morlat@linphone.org>, (C) 2009
 *
 * Copyright (C) 2010  Belledonne Comunications, Grenoble, France
 *
 */


#include "TunnelManager.hh"

#include "ortp/rtpsession.h"
#include "linphonecore.h"
#include "linphonecore_utils.h"
#include "eXosip2/eXosip_transport_hook.h"
#include "tunnel/udp_mirror.hh"

#ifdef ANDROID
#include <android/log.h>
#endif


using namespace belledonnecomm;
using namespace ::std;

Mutex TunnelManager::sMutex;

int TunnelManager::eXosipSendto(int fd,const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen,void* userdata){
	TunnelManager* lTunnelMgr=(TunnelManager*)userdata;
	
	sMutex.lock();
	if (lTunnelMgr->mSipSocket==NULL){
		sMutex.unlock();
		return len;
	}
	lTunnelMgr->mSipSocket->sendto(buf,len,to,tolen);
	sMutex.unlock();
	//ignore the error in all cases, retransmissions might be successful.
	return len;
}

int TunnelManager::eXosipRecvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen,void* userdata){
	TunnelManager* lTunnelMgr=(TunnelManager*)userdata;
	int err;
	sMutex.lock();
	if (lTunnelMgr->mSipSocket==NULL){
		sMutex.unlock();
		return 0;//let ignore the error
	}
	err=lTunnelMgr->mSipSocket->recvfrom(buf,len,from,*fromlen);
	sMutex.unlock();
	return err;
}

int TunnelManager::eXosipSelect(int max_fds, fd_set *s1, fd_set *s2, fd_set *s3, struct timeval *tv,void* userdata){
	struct timeval begin,cur;
	TunnelManager* lTunnelMgr=(TunnelManager*)userdata;
	if (s1 && tv!=0 && tv->tv_sec){
		/*this is the select from udp.c, the one that is interesting to us*/
		NativeSocket udp_fd=(NativeSocket)eXosip_get_udp_socket();
		NativeSocket controlfd=(NativeSocket)eXosip_get_control_fd();

		FD_ZERO(s1);
		gettimeofday(&begin,NULL);
		do{
			struct timeval abit;

			abit.tv_sec=0;
			abit.tv_usec=20000;
			sMutex.lock();
			if (lTunnelMgr->mSipSocket!=NULL){
				if (lTunnelMgr->mSipSocket->hasData()) {
					sMutex.unlock();
					/* we make exosip believe that it has udp data to read*/
					FD_SET(udp_fd,s1);
					return 1;
				}
			}
			sMutex.unlock();
			gettimeofday(&cur,NULL);
			if (cur.tv_sec-begin.tv_sec>tv->tv_sec) {
				FD_SET(controlfd,s1);
				FD_SET(udp_fd,s1);
				return 0;
			}
			FD_ZERO(s1);
			FD_SET(controlfd,s1);
			if (select(max_fds,s1,s2,s3,&abit)==1) {
				return 1;
			}
		}while(1);
		
	}else{
		/*select called from other places, only the control fd is present */
		return select(max_fds,s1,s2,s3,tv);
	}
}


void TunnelManager::addServer(const char *ip, int port,unsigned int udpMirrorPort,unsigned int delay) {
	if (ip == NULL) {
		ip = "";
		ms_warning("Adding tunnel server with empty ip, it will not work!");
	}
	addServer(ip,port);
	mUdpMirrorClients.push_back(UdpMirrorClient(ServerAddr(ip,udpMirrorPort),delay));
}

void TunnelManager::addServer(const char *ip, int port) {
	if (ip == NULL) {
		ip = "";
		ms_warning("Adding tunnel server with empty ip, it will not work!");
	}
	mServerAddrs.push_back(ServerAddr(ip,port));
	if (mTunnelClient) mTunnelClient->addServer(ip,port);
}

void TunnelManager::cleanServers() {
	mServerAddrs.clear();

	UdpMirrorClientList::iterator it;
	mAutoDetectStarted=false;
	for (it = mUdpMirrorClients.begin(); it != mUdpMirrorClients.end();) {
		UdpMirrorClient& s=*it++;
		s.stop();
	}
	mUdpMirrorClients.clear();
	if (mTunnelClient) mTunnelClient->cleanServers();
}

void TunnelManager::reconnect(){
	if (mTunnelClient)
		mTunnelClient->reconnect();
}

void TunnelManager::setCallback(StateCallback cb, void *userdata) {
	mCallback=cb;
	mCallbackData=userdata;
}

static void sCloseRtpTransport(RtpTransport *t, void *userData){
	TunnelSocket *s=(TunnelSocket*)userData;
	TunnelManager *manager=(TunnelManager*)s->getUserPointer();
	manager->closeRtpTransport(t, s);
}
void TunnelManager::closeRtpTransport(RtpTransport *t, TunnelSocket *s){
	mTunnelClient->closeSocket(s);
	ms_free(t);
}

static RtpTransport *sCreateRtpTransport(void* userData, int port){
	return ((TunnelManager *) userData)->createRtpTransport(port);
}

RtpTransport *TunnelManager::createRtpTransport(int port){
	TunnelSocket *socket=mTunnelClient->createSocket(port);
	socket->setUserPointer(this);
	RtpTransport *t=ms_new0(RtpTransport,1);
	t->t_getsocket=NULL;
	t->t_recvfrom=customRecvfrom;
	t->t_sendto=customSendto;
	t->t_close=sCloseRtpTransport;
	t->data=socket;
	return t;
}

void TunnelManager::start() {
	if (!mTunnelClient) {
		mTunnelClient = new TunnelClient();
		mTunnelClient->setCallback((StateCallback)tunnelCallback,this);
		list<ServerAddr>::iterator it;
		for(it=mServerAddrs.begin();it!=mServerAddrs.end();++it){
			const ServerAddr &addr=*it;
			mTunnelClient->addServer(addr.mAddr.c_str(), addr.mPort);
		}
		mTunnelClient->setHttpProxy(mHttpProxyHost.c_str(), mHttpProxyPort, mHttpUserName.c_str(), mHttpPasswd.c_str());
	}
	mTunnelClient->start();

	if (mSipSocket == NULL) mSipSocket =mTunnelClient->createSocket(5060);
}

bool TunnelManager::isStarted() {
	return mTunnelClient != 0 && mTunnelClient->isStarted();
}

bool TunnelManager::isReady() const {
	return mTunnelClient && mTunnelClient->isReady() && mReady;
}

int TunnelManager::customSendto(struct _RtpTransport *t, mblk_t *msg , int flags, const struct sockaddr *to, socklen_t tolen){
	int size;
	msgpullup(msg,-1);
	size=msgdsize(msg);
	((TunnelSocket*)t->data)->sendto(msg->b_rptr,size,to,tolen);
	return size;
}

int TunnelManager::customRecvfrom(struct _RtpTransport *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen){
	int err=((TunnelSocket*)t->data)->recvfrom(msg->b_wptr,msg->b_datap->db_lim-msg->b_datap->db_base,from,*fromlen);
	if (err>0) return err;
	return 0;
}


TunnelManager::TunnelManager(LinphoneCore* lc) :TunnelClientController()
,mCore(lc)
,mSipSocket(NULL)
,mCallback(NULL)
,mEnabled(false)
,mTunnelClient(NULL)
,mAutoDetectStarted(false)
,mReady(false)
,mHttpProxyPort(0){

	mExosipTransport.data=this;
	mExosipTransport.recvfrom=eXosipRecvfrom;
	mExosipTransport.sendto=eXosipSendto;
	mExosipTransport.select=eXosipSelect;
	linphone_core_add_iterate_hook(mCore,(LinphoneCoreIterateHook)sOnIterate,this);
	mTransportFactories.audio_rtcp_func=sCreateRtpTransport;
	mTransportFactories.audio_rtcp_func_data=this;
	mTransportFactories.audio_rtp_func=sCreateRtpTransport;
	mTransportFactories.audio_rtp_func_data=this;
	mTransportFactories.video_rtcp_func=sCreateRtpTransport;
	mTransportFactories.video_rtcp_func_data=this;
	mTransportFactories.video_rtp_func=sCreateRtpTransport;
	mTransportFactories.video_rtp_func_data=this;
}

TunnelManager::~TunnelManager(){
	stopClient();
}

void TunnelManager::stopClient(){
	eXosip_transport_hook_register(NULL);
	if (mSipSocket != NULL){
		sMutex.lock();
		mTunnelClient->closeSocket(mSipSocket);
		mSipSocket = NULL;
		sMutex.unlock();
	}
	if (mTunnelClient){
		delete mTunnelClient;
		mTunnelClient=NULL;
	}
}

void TunnelManager::processTunnelEvent(const Event &ev){
	LinphoneProxyConfig* lProxy;
	linphone_core_get_default_proxy(mCore, &lProxy);

	if (mEnabled && mTunnelClient->isReady()){
		ms_message("Tunnel is up, registering now");
		linphone_core_set_firewall_policy(mCore,LinphonePolicyNoFirewall);
		linphone_core_set_rtp_transport_factories(mCore,&mTransportFactories);
		eXosip_transport_hook_register(&mExosipTransport);
		//force transport to udp
		LCSipTransports lTransport;
		
		lTransport.udp_port=(0xDFFF&random())+1024;
		lTransport.tcp_port=0;
		lTransport.tls_port=0;
		lTransport.dtls_port=0;
		
		linphone_core_set_sip_transports(mCore, &lTransport);
		//register
		if (lProxy) {
			linphone_proxy_config_done(lProxy);
		}
		mReady=true;
	}else if (mEnabled && !mTunnelClient->isReady()){
		/* we got disconnected from the tunnel */
		if (lProxy && linphone_proxy_config_is_registered(lProxy)) {
			/*forces de-registration so that we register again when the tunnel is up*/
			linphone_proxy_config_edit(lProxy);
			linphone_core_iterate(mCore);
		}
		mReady=false;
	}
}

void TunnelManager::waitUnRegistration(){
	LinphoneProxyConfig* lProxy;
	linphone_core_get_default_proxy(mCore, &lProxy);
	if (lProxy && linphone_proxy_config_get_state(lProxy)==LinphoneRegistrationOk) {
		int i=0;
		linphone_proxy_config_edit(lProxy);
		//make sure unregister is sent and authenticated
		do{
			linphone_core_iterate(mCore);
			ms_usleep(20000);
			if (i>100){
				ms_message("tunnel: timeout for unregistration expired, giving up");
				break;
			}
			i++;
		}while(linphone_proxy_config_get_state(lProxy)!=LinphoneRegistrationCleared);
	}	
}

void TunnelManager::enable(bool isEnable) {
	ms_message("Turning tunnel [%s]",(isEnable?"on":"off"));
	if (isEnable && !mEnabled){
		mEnabled=true;
		//1 save transport and firewall policy
		linphone_core_get_sip_transports(mCore, &mRegularTransport);
		mPreviousFirewallPolicy=linphone_core_get_firewall_policy(mCore);
		//2 unregister
		waitUnRegistration();
		//3 insert tunnel
		start();
	}else if (!isEnable && mEnabled){
		//1 unregister
		waitUnRegistration();
		
		mEnabled=false;
		stopClient();
		mReady=false;
		linphone_core_set_rtp_transport_factories(mCore,NULL);

		eXosip_transport_hook_register(NULL);
		//Restore transport and firewall policy
		linphone_core_set_sip_transports(mCore, &mRegularTransport);
		linphone_core_set_firewall_policy(mCore, mPreviousFirewallPolicy);
		//register
		LinphoneProxyConfig* lProxy;
		linphone_core_get_default_proxy(mCore, &lProxy);
		if (lProxy) {
			linphone_proxy_config_done(lProxy);
		}

	}
}

void TunnelManager::tunnelCallback(bool connected, TunnelManager *zis){
	Event ev;
	ev.mType=TunnelEvent;
	ev.mData.mConnected=connected;
	zis->postEvent(ev);
}

void TunnelManager::onIterate(){
	mMutex.lock();
	while(!mEvq.empty()){
		Event ev=mEvq.front();
		mEvq.pop();
		mMutex.unlock();
		if (ev.mType==TunnelEvent)
			processTunnelEvent(ev);
		else if (ev.mType==UdpMirrorClientEvent){
			processUdpMirrorEvent(ev);
		}
		mMutex.lock();
	}
	mMutex.unlock();
}

/*invoked from linphone_core_iterate() */
void TunnelManager::sOnIterate(TunnelManager *zis){
	zis->onIterate();
}

#ifdef ANDROID
extern void linphone_android_log_handler(int prio, const char *fmt, va_list args);
static void linphone_android_tunnel_log_handler(int lev, const char *fmt, va_list args) {
	int prio;
	switch(lev){
	case TUNNEL_DEBUG:	prio = ANDROID_LOG_DEBUG;	break;
	case TUNNEL_INFO:	prio = ANDROID_LOG_INFO;	break;
	case TUNNEL_NOTICE:	prio = ANDROID_LOG_INFO;	break;
	case TUNNEL_WARN:	prio = ANDROID_LOG_WARN;	break;
	case TUNNEL_ERROR:	prio = ANDROID_LOG_ERROR;	break;
	default:		prio = ANDROID_LOG_DEFAULT;	break;
	}
	linphone_android_log_handler(prio, fmt, args);
}
#endif /*ANDROID*/

void TunnelManager::enableLogs(bool value) {
	enableLogs(value,NULL);
}

void TunnelManager::enableLogs(bool isEnabled,LogHandler logHandler) {
	if (logHandler != NULL)	SetLogHandler(logHandler);
#ifdef ANDROID
	else SetLogHandler(linphone_android_tunnel_log_handler);
#else
	else SetLogHandler(default_log_handler);
#endif

	if (isEnabled) {
		SetLogLevel(TUNNEL_ERROR|TUNNEL_WARN|TUNNEL_INFO);
	} else {
		SetLogLevel(TUNNEL_ERROR|TUNNEL_WARN);
	}
}
	

bool TunnelManager::isEnabled() {
	return mEnabled;
}

void TunnelManager::processUdpMirrorEvent(const Event &ev){
	if (ev.mData.mHaveUdp) {
		LOGI("Tunnel is not required, disabling");
		enable(false);
		mAutoDetectStarted = false;
	} else {
		mCurrentUdpMirrorClient++;
		if (mCurrentUdpMirrorClient !=mUdpMirrorClients.end()) {
			// enable tunnel but also try backup server
			LOGI("Tunnel is required, enabling; Trying backup udp mirror");
			
			UdpMirrorClient &lUdpMirrorClient=*mCurrentUdpMirrorClient;
			lUdpMirrorClient.start(TunnelManager::sUdpMirrorClientCallback,(void*)this);
		} else {
			LOGI("Tunnel is required, enabling; no backup udp mirror available");
			mAutoDetectStarted = false;
		}
		enable(true);
	}
}

void TunnelManager::postEvent(const Event &ev){
	mMutex.lock();
	mEvq.push(ev);
	mMutex.unlock();
}

void TunnelManager::sUdpMirrorClientCallback(bool isUdpAvailable, void* data) {
	TunnelManager* thiz = (TunnelManager*)data;
	Event ev;
	ev.mType=UdpMirrorClientEvent;
	ev.mData.mHaveUdp=isUdpAvailable;
	thiz->postEvent(ev);
}

void TunnelManager::autoDetect() {
	// first check if udp mirrors was provisionned
	if (mUdpMirrorClients.empty()) {
		LOGE("No UDP mirror server configured aborting auto detection");
		return;
	}
	if (mAutoDetectStarted) {
		LOGE("auto detection already in progress, restarting");
		(*mCurrentUdpMirrorClient).stop();
	}
	mAutoDetectStarted=true;
	mCurrentUdpMirrorClient =mUdpMirrorClients.begin();
	UdpMirrorClient &lUdpMirrorClient=*mCurrentUdpMirrorClient;
	lUdpMirrorClient.start(TunnelManager::sUdpMirrorClientCallback,(void*)this);
	
}

void TunnelManager::setHttpProxyAuthInfo(const char* username,const char* passwd) {
	mHttpUserName=username?username:"";
	mHttpPasswd=passwd?passwd:"";
	if (mTunnelClient) mTunnelClient->setHttpProxyAuthInfo(username,passwd);
}

void TunnelManager::setHttpProxy(const char *host,int port, const char *username, const char *passwd){
	mHttpUserName=username?username:"";
	mHttpPasswd=passwd?passwd:"";
	mHttpProxyPort=(port>0) ? port : 0;
	mHttpProxyHost=host ? host : "";
	if (mTunnelClient) mTunnelClient->setHttpProxy(host, port, username, passwd);
}

LinphoneCore *TunnelManager::getLinphoneCore(){
	return mCore;
}
