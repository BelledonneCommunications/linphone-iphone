package org.linphone.core;

/**
 * The TunnelConfig interface allows to configure information about a tunnel server (voip anti blocking).
 * @author smorlat
 *
 */
public interface TunnelConfig {
	/**
	 * Get the hostname of the tunnel server
	 * @return
	 */
	String getHost();
	/**
	 * Set the hostname (or ip address) of the tunnel server.
	 * @param host
	 */
	void setHost(String host);
	/**
	 * Get the port where to connect.
	 * @return
	 */
	int getPort();
	/**
	 * Set the port where to connect to the tunnel server.
	 * When not set, the default value is used (443).
	 * @param port
	 */
	void setPort(int port);
	/**
	 * Get the hostname of the second tunnel server
	 * @return
	 */
	String getHost2();
	/**
	 * Set the hostname (or ip address) of the second tunnel server.
	 * @param host
	 */
	void setHost2(String host);
	/**
	 * Get the port where to connect to the second tunnel server.
	 * @return
	 */
	int getPort2();
	/**
	 * Set the port where to connect to the second tunnel server.
	 * When not set, the default value is used (443).
	 * @param port
	 */
	void setPort2(int port);
	/**
	 * Get the remote udp mirror port, which is used to check udp connectivity of the network.
	 * @return
	 */
	int getRemoteUdpMirrorPort();
	/**
	 * Set the udp mirror port, which is used to check udp connectivity.
	 * When not set, a default value of 12345 is used.
	 * @param remoteUdpMirrorPort
	 */
	void setRemoteUdpMirrorPort(int remoteUdpMirrorPort);
	/**
	 * Get the maximum amount of time for waiting for UDP packets to come back during 
	 * the UDP connectivity check, in milliseconds.
	 * 
	 * @return
	 */
	int getDelay();
	/**
	 * Set the maximum amount of time for waiting for UDP packets to come back during 
	 * the UDP connectivity check, in milliseconds.
	 */
	void setDelay(int delay);
}
