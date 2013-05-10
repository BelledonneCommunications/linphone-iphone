/*
 *  C Implementation: tunnel
 *
 * Description: 
 *
 *
 *
 *Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 */

#ifndef __TUNNEL_CLIENT_MANAGER_H__
#define __TUNNEL_CLIENT_MANAGER_H__
#include <list>
#include <string>
#include "tunnel/client.hh"
#include "linphonecore.h"

extern "C" {
	#include "eXosip2/eXosip_transport_hook.h"
}
namespace belledonnecomm {
class TunnelClient;
class UdpMirrorClient;
/**
 * @addtogroup tunnel_client 
 * @{
**/

	/**
	 * The TunnelManager class extends the LinphoneCore functionnality in order to provide an easy to use API to 
	 * - provision tunnel servers ip addresses and ports
	 * - start/stop the tunneling service
	 * - be informed of of connection and disconnection events to the tunnel server
	 * - perform auto-detection whether tunneling is required, based on a test of sending/receiving a flow of UDP packets.
	 * 
	 * It takes in charge automatically the SIP registration procedure when connecting or disconnecting to a tunnel server.
	 * No other action on LinphoneCore is required to enable full operation in tunnel mode.
	**/
	class TunnelManager : public TunnelClientController{
		
	public:
		/**
		 * Add a tunnel server. At least one should be provided to be able to connect.
		 * When several addresses are provided, the tunnel client may try each of them until it gets connected.
		 *
		 * @param ip tunnMethod definition for '-isInitialStateOn' not foundel server ip address
		 * @param port tunnel server tls port, recommended value is 443
		 */
		void addServer(const char *ip, int port);
		/**
		 *Add tunnel server with auto detection capabilities
		 *
		 * @param ip tunnel server ip address
		 * @param port tunnel server tls port, recommended value is 443
		 * @param udpMirrorPort remote port on the tunnel server side  used to test udp reachability
		 * @param delay udp packet round trip delay in ms considered as acceptable. recommanded value is 1000 ms.
		 */
		void addServer(const char *ip, int port,unsigned int udpMirrorPort,unsigned int delay);
		/**
		 * Removes all tunnel server address previously entered with addServer()
		**/ 
		void cleanServers();
		/**
		 * Register a state callback to be notified whenever the tunnel client is connected or disconnected to the tunnel server.
		 * @param cb application callback function to use for notifying of connection/disconnection events.
		 * @param userdata An opaque pointer passed to the callback, used optionally by the application to retrieve a context.
		**/		
		void setCallback(StateCallback cb, void *userdata);
		/**
		 * Start connecting to a tunnel server.
		 * At this step, nothing is tunneled yet. The enable() method must be used to state whether SIP and RTP traffic
		 * need to be tunneled or not.
		**/
		void start();
		/**
		 * Forces reconnection to the tunnel server.
		 * This method is useful when the device switches from wifi to Edge/3G or vice versa. In most cases the tunnel client socket
		 * won't be notified promptly that its connection is now zombie, so it is recommended to call this method that will cause
		 * the lost connection to be closed and new connection to be issued.
		**/
		void reconnect();
		/**
		 * Sets whether tunneling of SIP and RTP is required.
		 * @param isEnabled If true enter in tunneled mode, if false exits from tunneled mode.
		 * The TunnelManager takes care of refreshing SIP registration when switching on or off the tunneled mode.
		 *
		**/
		void enable(bool isEnabled);
		/**
		 * In auto detect mode, the tunnel manager try to establish a real time rtp cummunication with the tunnel server on  specified port.
		 *<br>In case of success, the tunnel is automatically turned off. Otherwise, if no udp commmunication is feasible, tunnel mode is turned on.
		 *<br> Call this method each time to run the auto detection algorithm
		 */
		void autoDetect();
		/**
		 * Returns a boolean indicating whether tunneled operation is enabled.
		**/
		bool isEnabled();
		/**
		 * Enables debug logs of the Tunnel subsystem.
		**/
		void enableLogs(bool isEnabled);
		/**
		 * Enables debugs logs of the Tunnel subsystem and specify a callback where to receive the debug messages.
		**/
		void enableLogs(bool isEnabled,LogHandler logHandler);
		/**
		 * iOS only feature: specify http proxy credentials.
		 * When the iOS device has an http proxy configured in the iOS settings, the tunnel client will connect to the server
		 * through this http proxy. Credentials might be needed depending on the proxy configuration.
		 * @param username The username.
		 * @param passwd The password.
		**/
		void setHttpProxyAuthInfo(const char* username,const char* passwd);
		~TunnelManager();
		TunnelManager(LinphoneCore* lc);
		/**
		 * Destroy the given RtpTransport.
		 */
		void closeRtpTransport(RtpTransport *t, TunnelSocket *s);

		/**
		 * Create an RtpTransport.
		 */
		RtpTransport *createRtpTransport(int port);

		/**
		 * Get associated Linphone Core.
		 */
		LinphoneCore *getLinphoneCore();
		virtual void setHttpProxy(const char *host,int port, const char *username, const char *passwd);
		virtual bool isReady() const;
	private:
		enum EventType{
			UdpMirrorClientEvent,
			TunnelEvent,
		};
		struct Event{
			EventType mType;
			union EventData{
				bool mConnected;
				bool mHaveUdp;
			}mData;
		};
		typedef std::list<UdpMirrorClient> UdpMirrorClientList;
		virtual bool isStarted();
		void onIterate();
		static int customSendto(struct _RtpTransport *t, mblk_t *msg , int flags, const struct sockaddr *to, socklen_t tolen);
		static int customRecvfrom(struct _RtpTransport *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen);
		static int eXosipSendto(int fd,const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen,void* userdata);
		static int eXosipRecvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen,void* userdata);
		static int eXosipSelect(int nfds, fd_set *s1, fd_set *s2, fd_set *s3, struct timeval *tv,void* userdata);
		static void tunnelCallback(bool connected, TunnelManager *zis);
		static void sOnIterate(TunnelManager *zis);
		static void sUdpMirrorClientCallback(bool result, void* data);
		void waitUnRegistration();
		void processTunnelEvent(const Event &ev);
		void processUdpMirrorEvent(const Event &ev);
		void postEvent(const Event &ev);
		LinphoneCore* mCore;
		LCSipTransports mRegularTransport;
		TunnelSocket *mSipSocket;
		eXosip_transport_hooks_t mExosipTransport;
		StateCallback mCallback;
		void * mCallbackData;
		bool mEnabled;
		std::queue<Event> mEvq;
		std::list <ServerAddr> mServerAddrs;
		UdpMirrorClientList mUdpMirrorClients;
		UdpMirrorClientList::iterator mCurrentUdpMirrorClient;
		TunnelClient* mTunnelClient;
		void stopClient();
		Mutex mMutex;
		static Mutex sMutex;
		bool mAutoDetectStarted;
		bool mReady;
		LinphoneRtpTransportFactories mTransportFactories;
		std::string mHttpUserName;
		std::string mHttpPasswd;
		std::string mHttpProxyHost;
		int mHttpProxyPort;
		LinphoneFirewallPolicy mPreviousFirewallPolicy;
	};

/**
 * @}
**/

}



#endif /*__TUNNEL_CLIENT_MANAGER_H__*/
