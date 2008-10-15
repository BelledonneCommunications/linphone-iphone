/*
 * This file is part of JSTUN. 
 * 
 * Copyright (c) 2005 Thomas King <king@t-king.de> - All rights
 * reserved.
 * 
 * This software is licensed under either the GNU Public License (GPL),
 * or the Apache 2.0 license. Copies of both license agreements are
 * included in this distribution.
 */

package org.linphone.p2pproxy.core.stun;

import java.net.*;

public class DiscoveryInfo {
	private boolean error = false;
	private int errorResponseCode = 0;
	private String errorReason;
	private boolean blockedUDP = false;
	private boolean fullCone = false;
	private boolean symmetric = false;
	private InetSocketAddress mTestSocketAddress;
	private InetSocketAddress mPublicSocketAddress;
	
	public DiscoveryInfo(InetSocketAddress aTestSocketAddress) {
		mTestSocketAddress = aTestSocketAddress;
	}
	
	public boolean isError() {
		return error;
	}
	
	public void setError(int responseCode, String reason) {
		this.error = true;
		this.errorResponseCode = responseCode;
		this.errorReason = reason;
	}
	


	public boolean isBlockedUDP() {
		if (error) return false;
		return blockedUDP;
	}

	public void setBlockedUDP() {
		this.blockedUDP = true;
	}
	
	public boolean isFullCone() {
		if (error) return false;
		return fullCone;
	}

	public void setFullCone() {
		this.fullCone = true;
	}

	public boolean isSymmetric() {
		if (error) return false;
		return symmetric;
	}

	public void setSymmetric() {
		this.symmetric = true;
	}

	public InetSocketAddress getLocalSocketAddress() {
		return mTestSocketAddress;
	}
	
	public InetSocketAddress getPublicSocketAddress() {
		return mPublicSocketAddress;
	}	
	public void setPublicSocketAddress(InetSocketAddress address) {
		mPublicSocketAddress = address;
	}	

	public String toString() {
		StringBuffer sb = new StringBuffer();
		sb.append("Network interface: ");
		try {
			sb.append(NetworkInterface.getByInetAddress(mTestSocketAddress.getAddress()).getName());
		} catch (SocketException se) {
			sb.append("unknown");
		}
		sb.append("\n");
		sb.append("Local Socket address: ");
		sb.append(mTestSocketAddress);
		sb.append("\n");
		if (error) {
			sb.append(errorReason + " - Responsecode: " + errorResponseCode);
			return sb.toString();
		}
		sb.append("Result: ");
		if (blockedUDP) sb.append("Firewall blocks UDP.\n");
		if (fullCone) sb.append("Full Cone NAT handles connections.\n");
		if (symmetric) sb.append("Symmetric Cone NAT handles connections.\n");
		sb.append("Public IP address: ");
		if (mPublicSocketAddress != null) {
			sb.append(mPublicSocketAddress.toString());
		} else {
			sb.append("unknown");
		}
		sb.append("\n");
		return sb.toString();
	}

}
