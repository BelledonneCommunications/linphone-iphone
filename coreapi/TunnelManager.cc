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
}

void TunnelManager::stopClient(){
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
	mTunnelClient(NULL),
	mIsConnected(false),
	mHttpProxyPort(0),
	mPreviousRegistrationEnabled(false),
	mTunnelizeSipPackets(true),
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

void TunnelManager::registration(){
	// registration occurs always after an unregistation has been made. First we
	// need to reset the previous registration mode
	LinphoneProxyConfig* lProxy;
	linphone_core_get_default_proxy(mCore, &lProxy);
	if (lProxy) {
		linphone_proxy_config_edit(lProxy);
		linphone_proxy_config_enable_register(lProxy,mPreviousRegistrationEnabled);
		linphone_proxy_config_done(lProxy);
	}
}

void TunnelManager::processTunnelEvent(const Event &ev){
	if (ev.mData.mConnected){
		ms_message("Tunnel is up, registering now");
		registration();
	} else {
		ms_error("Tunnel has been disconnected");
	}
}

void TunnelManager::waitUnRegistration() {
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
void TunnelManager::setMode(LinphoneTunnelMode mode) {
	if(mMode != mode) {
		waitUnRegistration();
		switch(mode) {
		case LinphoneTunnelModeEnable:
			mMode = mode;
			startClient();
			/* registration is done by proccessTunnelEvent() when the tunnel
			the tunnel succeed to connect */
			break;
		case LinphoneTunnelModeDisable:
			mMode = mode;
			stopClient();
			registration();
			break;
		case LinphoneTunnelModeAuto:
			mMode = mode;
			autoDetect();
			/* Registration is not needed because processUdpMirrorEvent() will
			call either connect() or disconnect(). Should disconnect() is called,
			processUdpMirrorEvent() care to call registratin() */
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
		LOGI("Tunnel is not required, disabling");
		stopClient();
		registration();
	} else {
		mCurrentUdpMirrorClient++;
		if (mCurrentUdpMirrorClient !=mUdpMirrorClients.end()) {
			// enable tunnel but also try backup server
			LOGI("Tunnel is required, enabling; Trying backup udp mirror");

			UdpMirrorClient &lUdpMirrorClient=*mCurrentUdpMirrorClient;
			lUdpMirrorClient.start(TunnelManager::sUdpMirrorClientCallback,(void*)this);
		} else {
			LOGI("Tunnel is required, enabling; no backup udp mirror available");
			startClient();
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
	// first check if udp mirrors was provisionned
	if (mUdpMirrorClients.empty()) {
		LOGE("No UDP mirror server configured aborting auto detection");
		return;
	}
	mCurrentUdpMirrorClient = mUdpMirrorClients.begin();
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
		if(isConnected()) {
			waitUnRegistration();
			if(mTunnelizeSipPackets) sal_enable_tunnel(mCore->sal, mTunnelClient);
			else sal_disable_tunnel(mCore->sal);
			registration();
		}
	}
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
