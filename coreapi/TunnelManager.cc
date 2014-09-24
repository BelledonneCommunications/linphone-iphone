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
#include "private.h"

#ifdef ANDROID
#include <android/log.h>
#endif

belledonnecomm::TunnelManager *bcTunnel(const LinphoneTunnel *tunnel);

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

void TunnelManager::startClient() {
	ms_message("TunnelManager: Starting tunnel client");
	if (mTunnelClient == NULL) {
		mTunnelClient = new TunnelClient();
		mTunnelClient->setCallback((TunnelClientController::StateCallback)tunnelCallback,this);
		list<ServerAddr>::iterator it;
		for(it=mServerAddrs.begin();it!=mServerAddrs.end();++it){
			const ServerAddr &addr=*it;
			mTunnelClient->addServer(addr.mAddr.c_str(), addr.mPort);
		}
		mTunnelClient->setHttpProxy(mHttpProxyHost.c_str(), mHttpProxyPort, mHttpUserName.c_str(), mHttpPasswd.c_str());
	}
	mTunnelClient->start();
	linphone_core_set_rtp_transport_factories(mCore,&mTransportFactories);
	if(mTunnelizeSipPackets) {
		sal_enable_tunnel(mCore->sal, mTunnelClient);
	}
	mConnecting = true;
}

void TunnelManager::stopClient(){
	ms_message("TunnelManager: Stopping tunnel client");
	linphone_core_set_rtp_transport_factories(mCore,NULL);
	sal_disable_tunnel(mCore->sal);
	if (mTunnelClient){
		delete mTunnelClient;
		mTunnelClient=NULL;
	}
}

bool TunnelManager::isConnected() const {
	return mTunnelClient != NULL && mTunnelClient->isReady();
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


TunnelManager::TunnelManager(LinphoneCore* lc) :
	mCore(lc),
#ifndef USE_BELLESIP
	mSipSocket(NULL),
	mExosipTransport(NULL),
#endif
	mMode(LinphoneTunnelModeDisable),
	mAutoDetecting(false),
	mConnecting(false),
	mScheduledRegistration(false),
	mTunnelizeSipPackets(true),
	mTunnelClient(NULL),
	mHttpProxyPort(0),
	mVTable(NULL)
{
	linphone_core_add_iterate_hook(mCore,(LinphoneCoreIterateHook)sOnIterate,this);
	mTransportFactories.audio_rtcp_func=sCreateRtpTransport;
	mTransportFactories.audio_rtcp_func_data=this;
	mTransportFactories.audio_rtp_func=sCreateRtpTransport;
	mTransportFactories.audio_rtp_func_data=this;
	mTransportFactories.video_rtcp_func=sCreateRtpTransport;
	mTransportFactories.video_rtcp_func_data=this;
	mTransportFactories.video_rtp_func=sCreateRtpTransport;
	mTransportFactories.video_rtp_func_data=this;
	mVTable = linphone_vtable_new();
	mVTable->network_reachable = networkReachableCb;
	linphone_core_add_listener(mCore, mVTable);
}

TunnelManager::~TunnelManager(){
	stopClient();
	linphone_core_remove_listener(mCore, mVTable);
	linphone_vtable_destroy(mVTable);
}

void TunnelManager::doRegistration(){
	if(mTunnelizeSipPackets) {
		LinphoneProxyConfig* lProxy;
		linphone_core_get_default_proxy(mCore, &lProxy);
		if (lProxy) {
			ms_message("TunnelManager: need to register");
			if(linphone_proxy_config_get_state(lProxy) != LinphoneRegistrationProgress) {
				linphone_proxy_config_refresh_register(lProxy);
				mScheduledRegistration = false;
			} else {
				ms_warning("TunnelManager: register difered. There is already a registration in progress");
				mScheduledRegistration = true;
			}
		} else {
			mScheduledRegistration = false;
		}
	}
}

void TunnelManager::processTunnelEvent(const Event &ev){
	if (ev.mData.mConnected){
		ms_message("Tunnel is connected");
		doRegistration();
	} else {
		ms_error("Tunnel has been disconnected");
	}
	mConnecting = false;
}

void TunnelManager::setMode(LinphoneTunnelMode mode) {
	if(mMode != mode) {
		ms_message("TunnelManager: Switching mode from %s to %s",
				   tunnel_mode_to_string(mMode),
				   tunnel_mode_to_string(mode));
		switch(mode) {
		case LinphoneTunnelModeEnable:
			mMode = mode;
			startClient();
			break;
		case LinphoneTunnelModeDisable:
			mMode = mode;
			stopClient();
			doRegistration();
			break;
		case LinphoneTunnelModeAuto:
			mMode = mode;
			autoDetect();
			break;
		default:
			ms_error("TunnelManager::setMode(): invalid mode (%d)", mode);
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
	if(mScheduledRegistration) {
		ms_message("Apply difered registration");
		doRegistration();
	}
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


LinphoneTunnelMode TunnelManager::getMode() const {
	return mMode;
}

void TunnelManager::processUdpMirrorEvent(const Event &ev){
	if (ev.mData.mHaveUdp) {
		ms_message("TunnelManager: auto detection test succeed");
		stopClient();
		doRegistration();
		mAutoDetecting = false;
	} else {
		ms_message("TunnelManager: auto detection test failed");
		mCurrentUdpMirrorClient++;
		if (mCurrentUdpMirrorClient !=mUdpMirrorClients.end()) {
			ms_message("TunnelManager: trying another udp mirror");
			UdpMirrorClient &lUdpMirrorClient=*mCurrentUdpMirrorClient;
			lUdpMirrorClient.start(TunnelManager::sUdpMirrorClientCallback,(void*)this);
		} else {
			ms_message("TunnelManager: all auto detection failed. Need ti enable tunnel");
			startClient();
			mAutoDetecting = false;
		}
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

void TunnelManager::networkReachableCb(LinphoneCore *lc, bool_t reachable) {
	TunnelManager *tunnel = bcTunnel(linphone_core_get_tunnel(lc));
	if(reachable && tunnel->getMode() == LinphoneTunnelModeAuto) {
		tunnel->autoDetect();
	}
}

void TunnelManager::autoDetect() {
	if(mAutoDetecting) {
		ms_error("TunnelManager: Cannot start auto detection. One auto detection is going on");
		return;
	}
	if (mUdpMirrorClients.empty()) {
		ms_error("TunnelManager: No UDP mirror server configured aborting auto detection");
		return;
	}
	mCurrentUdpMirrorClient = mUdpMirrorClients.begin();
	UdpMirrorClient &lUdpMirrorClient=*mCurrentUdpMirrorClient;
	lUdpMirrorClient.start(TunnelManager::sUdpMirrorClientCallback,(void*)this);
	mAutoDetecting = true;
}

void TunnelManager::setHttpProxyAuthInfo(const char* username,const char* passwd) {
	mHttpUserName=username?username:"";
	mHttpPasswd=passwd?passwd:"";
	if (mTunnelClient) mTunnelClient->setHttpProxyAuthInfo(username,passwd);
}

void TunnelManager::tunnelizeSipPackets(bool enable){
		mTunnelizeSipPackets = enable;
}

bool TunnelManager::tunnelizeSipPacketsEnabled() const {
	return mTunnelizeSipPackets;
}

void TunnelManager::setHttpProxy(const char *host,int port, const char *username, const char *passwd){
	mHttpUserName=username?username:"";
	mHttpPasswd=passwd?passwd:"";
	mHttpProxyPort=(port>0) ? port : 0;
	mHttpProxyHost=host ? host : "";
	if (mTunnelClient) mTunnelClient->setHttpProxy(host, port, username, passwd);
}

LinphoneCore *TunnelManager::getLinphoneCore() const{
	return mCore;
}
