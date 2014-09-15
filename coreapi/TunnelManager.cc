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
#ifndef USE_BELLESIP
#include "eXosip2/eXosip_transport_hook.h"
#endif
#include "tunnel/udp_mirror.hh"
#include "private.h"

#ifdef ANDROID
#include <android/log.h>
#endif


using namespace belledonnecomm;
using namespace ::std;


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
}

static RtpTransport *sCreateRtpTransport(void* userData, int port){
	return ((TunnelManager *) userData)->createRtpTransport(port);
}

void sDestroyRtpTransport(RtpTransport *t){
	ms_free(t);
}

RtpTransport *TunnelManager::createRtpTransport(int port){
	TunnelSocket *socket=mTunnelClient->createSocket(port);
	socket->setUserPointer(this);
	RtpTransport *t=ms_new0(RtpTransport,1);
	t->t_getsocket=NULL;
	t->t_recvfrom=customRecvfrom;
	t->t_sendto=customSendto;
	t->t_close=sCloseRtpTransport;
	t->t_destroy=sDestroyRtpTransport;
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
	,mCallback(NULL)
	,mEnabled(false)
	,mTunnelClient(NULL)
	,mAutoDetectStarted(false)
	,mReady(false)
	,mHttpProxyPort(0)
	,mPreviousRegistrationEnabled(false)
	,mTunnelizeSipPackets(true){

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
	sal_disable_tunnel(mCore->sal);
	if (mTunnelClient){
		delete mTunnelClient;
		mTunnelClient=NULL;
	}
}

void TunnelManager::registration(){
	LinphoneProxyConfig* lProxy;

	//  tunnel was enabled
	if (isReady()){
		linphone_core_set_firewall_policy(mCore,LinphonePolicyNoFirewall);
		linphone_core_set_rtp_transport_factories(mCore,&mTransportFactories);
		if(mTunnelizeSipPackets) {
			sal_enable_tunnel(mCore->sal, mTunnelClient);
		}
	// tunnel was disabled
	} else {
		linphone_core_set_firewall_policy(mCore, mPreviousFirewallPolicy);
	}

	// registration occurs always after an unregistation has been made. First we
	// need to reset the previous registration mode
	linphone_core_get_default_proxy(mCore, &lProxy);
	if (lProxy) {
		linphone_proxy_config_edit(lProxy);
		linphone_proxy_config_enable_register(lProxy,mPreviousRegistrationEnabled);
		linphone_proxy_config_done(lProxy);
	}
}

void TunnelManager::processTunnelEvent(const Event &ev){
	if (mEnabled && mTunnelClient->isReady()){
		mReady=true;
		ms_message("Tunnel is up, registering now");
		registration();
	}else if (mEnabled && !mTunnelClient->isReady()){
		/* we got disconnected from the tunnel */
		mReady=false;
	}
}

void TunnelManager::waitUnRegistration(){
	LinphoneProxyConfig* lProxy;

	linphone_core_get_default_proxy(mCore, &lProxy);
	if (lProxy){
		mPreviousRegistrationEnabled=linphone_proxy_config_register_enabled(lProxy);
		if (linphone_proxy_config_is_registered(lProxy)) {
			int i=0;
			linphone_proxy_config_edit(lProxy);
			linphone_proxy_config_enable_register(lProxy,FALSE);
			linphone_proxy_config_done(lProxy);
			sal_unregister(lProxy->op);
			//make sure unregister is sent and authenticated
			do{
				linphone_core_iterate(mCore);
				ms_usleep(20000);
				if (i>100){
					ms_message("tunnel: timeout for unregistration expired, giving up");
					break;
				}
				i++;
			}while(linphone_proxy_config_is_registered(lProxy));
			ms_message("Unregistration %s", linphone_proxy_config_is_registered(lProxy)?"failed":"succeeded");
		}else{
			ms_message("No registration pending");
		}
	}
}

/*Each time tunnel is enabled/disabled, we need to unregister previous session and re-register. Since tunnel initialization
is asynchronous, we temporary disable auto register while tunnel sets up, and reenable it when re-registering. */
void TunnelManager::enable(bool isEnable) {
	ms_message("Turning tunnel [%s]", isEnable ?"on" : "off");
	if (isEnable && !mEnabled){
		mEnabled=true;
		//1 save firewall policy
		mPreviousFirewallPolicy=linphone_core_get_firewall_policy(mCore);
		//2 unregister
		waitUnRegistration();
		//3 insert tunnel
		start();
	}else if (!isEnable && mEnabled){
		//1 unregister
		waitUnRegistration();

		// 2 stop tunnel
		mEnabled=false;
		stopClient();
		mReady=false;
		linphone_core_set_rtp_transport_factories(mCore,NULL);
		sal_disable_tunnel(mCore->sal);

		// 3 register
		registration();
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
extern void linphone_android_log_handler(int prio, char *str);
static void linphone_android_tunnel_log_handler(int lev, const char *fmt, va_list args) {
	char str[4096];
	vsnprintf(str, sizeof(str) - 1, fmt, args);
	str[sizeof(str) - 1] = '\0';

	int prio;
	switch(lev){
	case TUNNEL_DEBUG:	prio = ANDROID_LOG_DEBUG;	break;
	case TUNNEL_INFO:	prio = ANDROID_LOG_INFO;	break;
	case TUNNEL_NOTICE:	prio = ANDROID_LOG_INFO;	break;
	case TUNNEL_WARN:	prio = ANDROID_LOG_WARN;	break;
	case TUNNEL_ERROR:	prio = ANDROID_LOG_ERROR;	break;
	default:		prio = ANDROID_LOG_DEFAULT;	break;
	}
	linphone_android_log_handler(prio, str);
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
	else SetLogHandler(tunnel_default_log_handler);
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

void TunnelManager::tunnelizeSipPackets(bool enable){
	if(enable != mTunnelizeSipPackets) {
		mTunnelizeSipPackets = enable;
		if(mEnabled && isReady()) {
			waitUnRegistration();
			registration();
		}
	}
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
