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
#include <tunnel/client.hh>
#include <tunnel/udp_mirror.hh>
#include "linphonecore.h"
#include "linphone_tunnel.h"

#ifndef USE_BELLESIP
extern "C" {
	#include <eXosip2/eXosip_transport_hook.h>
}
#endif

namespace belledonnecomm {
/**
 * @addtogroup tunnel_client
 * @{
**/

	/**
	 * The TunnelManager class extends the LinphoneCore functionnality in order to provide an easy to use API to
	 * - provision tunnel servers ip addresses and ports
	 * - start/stop the tunneling service
	 * - be informed of connection and disconnection events to the tunnel server
	 * - perform auto-detection whether tunneling is required, based on a test of sending/receiving a flow of UDP packets.
	 *
	 * It takes in charge automatically the SIP registration procedure when connecting or disconnecting to a tunnel server.
	 * No other action on LinphoneCore is required to enable full operation in tunnel mode.
	**/
	class TunnelManager {

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
		 * Forces reconnection to the tunnel server.
		 * This method is useful when the device switches from wifi to Edge/3G or vice versa. In most cases the tunnel client socket
		 * won't be notified promptly that its connection is now zombie, so it is recommended to call this method that will cause
		 * the lost connection to be closed and new connection to be issued.
		**/
		void reconnect();
		/**
		 * @brief setMode
		 * @param mode
		 */
		void setMode(LinphoneTunnelMode mode);
		/**
		 * @brief Return the tunnel mode
		 * @return #LinphoneTunnelMode
		 */
		LinphoneTunnelMode getMode() const;
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
		void setHttpProxy(const char *host,int port, const char *username, const char *passwd);
		/**
		 * Indicate to the tunnel manager whether SIP packets must pass
		 * through the tunnel. That featurte is automatically enabled at
		 * the creation of the TunnelManager instance.
		 * @param enable If set to TRUE, SIP packets will pass through the tunnel.
		 * If set to FALSE, SIP packets will pass by the configured proxies.
		 */
		void tunnelizeSipPackets(bool enable);
		/**
		 * @brief Check whether the tunnel manager is set to tunnelize SIP packets
		 * @return True, SIP packets pass through the tunnel
		 */
		bool tunnelizeSipPacketsEnabled() const;
		/**
		 * @brief Constructor
		 * @param lc The LinphoneCore instance of which the TunnelManager will be associated to.
		 */
		TunnelManager(LinphoneCore* lc);
		/**
		 * @brief Destructor
		 */
		~TunnelManager();
		/**
		 * @brief Create an RtpTransport
		 * @param port
		 * @return
		 */
		RtpTransport *createRtpTransport(int port);
		/**
		 * @brief Destroy the given RtpTransport
		 * @param t
		 * @param s
		 */
		void closeRtpTransport(RtpTransport *t, TunnelSocket *s);
		/**
		 * @brief Get associated Linphone Core
		 * @return pointer on the associated LinphoneCore
		 */
		LinphoneCore *getLinphoneCore() const;
		/**
		 * @brief Check wehter the tunnel is connected
		 * @return True whether the tunnel is connected
		 */
		bool isConnected() const;

		bool isActivated() const;
	private:
		enum State {
			disabled,
			connecting,
			ready,
			autodetecting
		};

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
		static int customSendto(struct _RtpTransport *t, mblk_t *msg , int flags, const struct sockaddr *to, socklen_t tolen);
		static int customRecvfrom(struct _RtpTransport *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen);
		static int eXosipSendto(int fd,const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen,void* userdata);
		static int eXosipRecvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen,void* userdata);
		static int eXosipSelect(int nfds, fd_set *s1, fd_set *s2, fd_set *s3, struct timeval *tv,void* userdata);
		static void tunnelCallback(bool connected, TunnelManager *zis);
		static void sOnIterate(TunnelManager *zis);
		static void sUdpMirrorClientCallback(bool result, void* data);
		static void networkReachableCb(LinphoneCore *lc, bool_t reachable);

	private:
		void onIterate();
		void doRegistration();
		void doUnregistration();
		void startClient();
		bool startAutoDetection();
		void processTunnelEvent(const Event &ev);
		void processUdpMirrorEvent(const Event &ev);
		void postEvent(const Event &ev);

	private:
		LinphoneCore* mCore;
		LinphoneTunnelMode mMode;
		State mState;
		bool mTunnelizeSipPackets;
		TunnelClient* mTunnelClient;
		std::string mHttpUserName;
		std::string mHttpPasswd;
		std::string mHttpProxyHost;
		int mHttpProxyPort;
		LinphoneCoreVTable *mVTable;
		std::list <ServerAddr> mServerAddrs;
		UdpMirrorClientList mUdpMirrorClients;
		UdpMirrorClientList::iterator mCurrentUdpMirrorClient;
		LinphoneRtpTransportFactories mTransportFactories;
		Mutex mMutex;
		std::queue<Event> mEvq;
	};

/**
 * @}
**/

}



#endif /*__TUNNEL_CLIENT_MANAGER_H__*/
