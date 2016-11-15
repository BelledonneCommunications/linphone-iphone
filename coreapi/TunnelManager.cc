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
#include "linphone/core.h"
#include "linphone/core_utils.h"
#include "private.h"

#ifdef ANDROID
#include <android/log.h>
#endif

belledonnecomm::TunnelManager *bcTunnel(const LinphoneTunnel *tunnel);

using namespace belledonnecomm;
using namespace ::std;

void TunnelManager::addServer(const char *ip, int port,unsigned int udpMirrorPort,unsigned int delay) {
	if (ip == NULL) {
		ms_warning("Adding tunnel server with empty ip, it will not work!");
		return;
	}
	addServer(ip,port);
	mUdpMirrorClients.push_back(UdpMirrorClient(ServerAddr(ip,udpMirrorPort),delay));
}

void TunnelManager::addServer(const char *ip, int port) {
	if (ip == NULL) {
		ms_warning("Adding tunnel server with empty ip, it will not work!");
		return;
	}
	mServerAddrs.push_back(ServerAddr(ip,port));
	if (mTunnelClient) mTunnelClient->addServer(ip,port);
}

void TunnelManager::cleanServers() {
	mServerAddrs.clear();
	if (mLongRunningTaskId > 0) {
		sal_end_background_task(mLongRunningTaskId);
		mLongRunningTaskId = 0;
	}
	UdpMirrorClientList::iterator it;
	for (it = mUdpMirrorClients.begin(); it != mUdpMirrorClients.end();) {
		UdpMirrorClient& s=*it++;
		s.stop();
	}
	mUdpMirrorClients.clear();
	mCurrentUdpMirrorClient = mUdpMirrorClients.end();
	if (mTunnelClient) mTunnelClient->cleanServers();
}

void TunnelManager::reconnect(){
	if (mTunnelClient)
		mTunnelClient->reconnect();
}

static void sCloseRtpTransport(RtpTransport *t){
	TunnelSocket *s=(TunnelSocket*)t->data;
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
	ms_message("Creating tunnel RTP transport for local virtual port %i", port);
	return t;
}

void TunnelManager::startClient() {
	ms_message("TunnelManager: Starting tunnel client");
	if (!mTunnelClient){
		mTunnelClient = new TunnelClient(TRUE);
		sal_set_tunnel(mCore->sal, mTunnelClient);
		mTunnelClient->setCallback(tunnelCallback,this);
	}
	
	if (mVerifyServerCertificate) {
		const char *rootCertificatePath = linphone_core_get_root_ca(mCore);
		if (rootCertificatePath != NULL) {
			ms_message("TunnelManager: Load root certificate from %s", rootCertificatePath);
			mTunnelClient->setRootCertificate(rootCertificatePath); /* give the path to root certificate to the tunnel client in order to be able to verify the server certificate */
		} else {
			ms_warning("TunnelManager is set to verify server certificate but no root certificate is available in linphoneCore");
		}
	}
	mTunnelClient->cleanServers();
	list<ServerAddr>::iterator it;
	for(it=mServerAddrs.begin();it!=mServerAddrs.end();++it){
		const ServerAddr &addr=*it;
		mTunnelClient->addServer(addr.mAddr.c_str(), addr.mPort);
	}
	mTunnelClient->setHttpProxy(mHttpProxyHost.c_str(), mHttpProxyPort, mHttpUserName.c_str(), mHttpPasswd.c_str());
	if (!mTunnelClient->isStarted())
		mTunnelClient->start();
	else
		mTunnelClient->reconnect(); /*force a reconnection to take into account new parameters*/
	
}

void TunnelManager::stopClient(){
	if (linphone_core_get_calls_nb(mCore) == 0){
		/*if no calls are running, we can decide to stop the client completely, so that the connection to the tunnel server is terminated.*/
		if (mTunnelClient) {
			ms_message("TunnelManager: stoppping tunnel client");
			mTunnelClient->stop();
		}
		/*otherwise, it doesn't really matter if the tunnel connection is kept alive even if it is not used anymore by the liblinphone.*/
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
	memset(&msg->recv_addr,0,sizeof(msg->recv_addr));
	int err=((TunnelSocket*)t->data)->recvfrom(msg->b_wptr,dblk_lim(msg->b_datap)-dblk_base(msg->b_datap),from,*fromlen);
	//to make ice happy
	inet_aton(((TunnelManager*)((TunnelSocket*)t->data)->getUserPointer())->mLocalAddr,&msg->recv_addr.addr.ipi_addr);
	msg->recv_addr.family = AF_INET;
	msg->recv_addr.port = htons((unsigned short)((TunnelSocket*)t->data)->getPort());
	if (err>0) return err;
	return 0;
}

TunnelManager::TunnelManager(LinphoneCore* lc) :
	mCore(lc),
	mMode(LinphoneTunnelModeDisable),
	mTunnelClient(NULL),
	mHttpProxyPort(0),
	mVTable(NULL),
	mLongRunningTaskId(0),
	mSimulateUdpLoss(false)
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
	mVTable = linphone_core_v_table_new();
	mVTable->network_reachable = networkReachableCb;
	linphone_core_add_listener(mCore, mVTable);
	linphone_core_get_local_ip_for(AF_INET, NULL, mLocalAddr);
	mAutodetectionRunning = false;
	mState = Off;
	mTargetState = Off;
	mStarted = false;
	mTunnelizeSipPackets = true;
}

TunnelManager::~TunnelManager(){
	if (mLongRunningTaskId > 0) {
		sal_end_background_task(mLongRunningTaskId);
		mLongRunningTaskId = 0;
	}
	for(UdpMirrorClientList::iterator udpMirror = mUdpMirrorClients.begin(); udpMirror != mUdpMirrorClients.end(); udpMirror++) {
		udpMirror->stop();
	}
	stopClient();
	if (mTunnelClient) {
		mTunnelClient->stop();
		delete mTunnelClient;
	}
	sal_set_tunnel(mCore->sal,NULL);
	linphone_core_remove_listener(mCore, mVTable);
	linphone_core_v_table_destroy(mVTable);
}

void TunnelManager::doRegistration(){
	LinphoneProxyConfig* lProxy;
	lProxy = linphone_core_get_default_proxy_config(mCore);
	if (lProxy) {
		ms_message("TunnelManager: New registration");
		lProxy->commit = TRUE;
	}
}

void TunnelManager::doUnregistration() {
	LinphoneProxyConfig *lProxy;
	lProxy = linphone_core_get_default_proxy_config(mCore);
	if(lProxy) {
		_linphone_proxy_config_unregister(lProxy);
	}
}

void TunnelManager::tunnelizeLiblinphone(){
	ms_message("LinphoneCore goes into tunneled mode.");
	mState = On; /*do this first because _linphone_core_apply_transports() will use it to know if tunnel listening point is to be used*/
	linphone_core_set_rtp_transport_factories(mCore,&mTransportFactories);
	if (mTunnelizeSipPackets) {
		doUnregistration();
		_linphone_core_apply_transports(mCore);
		doRegistration();
	}
}

void TunnelManager::untunnelizeLiblinphone(){
	ms_message("LinphoneCore leaves tunneled mode.");
	mState = Off;
	linphone_core_set_rtp_transport_factories(mCore, NULL);
	if (mTunnelizeSipPackets) {
		doUnregistration();
		_linphone_core_apply_transports(mCore);
		doRegistration();
	}
}


void TunnelManager::applyState() {
	if (!linphone_core_is_network_reachable(mCore)) return;
	if (mTargetState == On && mState == Off){
		if (!mTunnelClient || !mTunnelClient->isStarted()){
			startClient();
		}
		if (mTunnelClient->isReady()) tunnelizeLiblinphone();
	}else if (mTargetState == Off && mState == On){
		untunnelizeLiblinphone();
		stopClient();
	}
}

void TunnelManager::setState ( TunnelManager::State state ) {
	mTargetState = state;
	applyState();
}



void TunnelManager::processTunnelEvent(const Event &ev){
	if (ev.mData.mConnected){
		ms_message("TunnelManager: tunnel is connected");
		applyState();
	} else {
		ms_error("TunnelManager: tunnel has been disconnected");
	}
}

void TunnelManager::applyMode() {
	switch(mMode) {
	case LinphoneTunnelModeEnable:
		stopAutoDetection();
		setState(On);
		break;
	case LinphoneTunnelModeDisable:
		stopAutoDetection();
		setState(Off);
		break;
	case LinphoneTunnelModeAuto:
		if (linphone_core_is_network_reachable(mCore)) startAutoDetection();
		break;
	default:
		ms_error("TunnelManager::setMode(): invalid mode (%d)", (int)mMode);
	}
}


void TunnelManager::setMode(LinphoneTunnelMode mode) {
	if(mMode == mode) return;
	ms_message("TunnelManager: switching mode from %s to %s",
			   linphone_tunnel_mode_to_string(mMode),
			   linphone_tunnel_mode_to_string(mode));
	mMode = mode;
	applyMode();
	
}

void TunnelManager::stopLongRunningTask() {
	if (mLongRunningTaskId != 0) {
		sal_end_background_task(mLongRunningTaskId);
		mLongRunningTaskId = 0;
	}
}


void TunnelManager::tunnelCallback(bool connected, void *user_pointer){
	TunnelManager *zis = static_cast<TunnelManager*>(user_pointer);
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
	if (mAutodetectionRunning == false) return; /*auto detection was cancelled, for example by switching to disabled state*/
	if (mSimulateUdpLoss || !ev.mData.mHaveUdp) {
		if (mSimulateUdpLoss) {
			ms_message("TunnelManager: simulate UDP lost on %s:%d", mCurrentUdpMirrorClient->getServerAddress().mAddr.c_str(), mCurrentUdpMirrorClient->getServerAddress().mPort);
		} else {
			ms_message("TunnelManager: UDP mirror test failed on %s:%d", mCurrentUdpMirrorClient->getServerAddress().mAddr.c_str(), mCurrentUdpMirrorClient->getServerAddress().mPort);
		}
		mCurrentUdpMirrorClient++;
		if (mCurrentUdpMirrorClient !=mUdpMirrorClients.end()) {
			ms_message("TunnelManager: trying another UDP mirror on %s:%d", mCurrentUdpMirrorClient->getServerAddress().mAddr.c_str(), mCurrentUdpMirrorClient->getServerAddress().mPort);
			UdpMirrorClient &lUdpMirrorClient=*mCurrentUdpMirrorClient;
			lUdpMirrorClient.start(TunnelManager::sUdpMirrorClientCallback,(void*)this);
			mAutodetectionRunning = true;
			return;
		} else {
			ms_message("TunnelManager: all UDP mirror tests failed");
			setState(On);
		}
	} else {
		ms_message("TunnelManager: UDP mirror test success on %s:%d", mCurrentUdpMirrorClient->getServerAddress().mAddr.c_str(), mCurrentUdpMirrorClient->getServerAddress().mPort);
		setState(Off);
	}
	mAutodetectionRunning = false;
	stopLongRunningTask();
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
	
	if (reachable) {
		linphone_core_get_local_ip_for(AF_INET, NULL,tunnel->mLocalAddr);
		if (tunnel->getMode() == LinphoneTunnelModeAuto){
			tunnel->startAutoDetection();
			/*autodetection will call applyState() when finished*/
		}else{
			tunnel->applyState();
		}
	} else if (!reachable) {
		// if network is no more reachable, cancel autodetection if any
		tunnel->stopAutoDetection();
		//turn off the tunnel connection
		tunnel->stopClient();
		tunnel->untunnelizeLiblinphone();
	}
}

void TunnelManager::stopAutoDetection(){
	if (mAutodetectionRunning){
		for(UdpMirrorClientList::iterator udpMirror = mUdpMirrorClients.begin(); udpMirror != mUdpMirrorClients.end(); udpMirror++) {
			udpMirror->stop();
		}
		mAutodetectionRunning = false;
		stopLongRunningTask();
	}
}

bool TunnelManager::startAutoDetection() {
	if (mUdpMirrorClients.empty()) {
		ms_error("TunnelManager: No UDP mirror server configured aborting auto detection");
		return false;
	}
	ms_message("TunnelManager: Starting auto-detection");
	mCurrentUdpMirrorClient = mUdpMirrorClients.begin();
	if (mLongRunningTaskId == 0)
		 mLongRunningTaskId = sal_begin_background_task("Tunnel auto detect", NULL, NULL);
	UdpMirrorClient &lUdpMirrorClient=*mCurrentUdpMirrorClient;
	mAutodetectionRunning = true;
	lUdpMirrorClient.start(TunnelManager::sUdpMirrorClientCallback,(void*)this);
	return true;
}

bool TunnelManager::isActivated() const{
	return mState == On;
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

void TunnelManager::verifyServerCertificate(bool enable){
	mVerifyServerCertificate = enable;
}

bool TunnelManager::verifyServerCertificateEnabled() const {
	return mVerifyServerCertificate;
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

void TunnelManager::simulateUdpLoss(bool enabled) {
	mSimulateUdpLoss = enabled;
}
