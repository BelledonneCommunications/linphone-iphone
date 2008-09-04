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

package de.javawi.jstun.test.demo;

import java.net.BindException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;
import java.util.logging.FileHandler;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import de.javawi.jstun.test.DiscoveryTest;

public class DiscoveryTestDemo implements Runnable {
	InetAddress iaddress;
	
	public DiscoveryTestDemo(InetAddress iaddress) {
		this.iaddress = iaddress;
	}
	
	public void run() {
		try {
			DiscoveryTest test = new DiscoveryTest(iaddress, "jstun.javawi.de", 3478);
			//DiscoveryTest test = new DiscoveryTest(iaddress, "stun.sipgate.net", 10000);
			// iphone-stun.freenet.de:3478
			// larry.gloo.net:3478
			// stun.xten.net:3478
			// stun.sipgate.net:10000
			System.out.println(test.test());
		} catch (BindException be) {
			System.out.println(iaddress.toString() + ": " + be.getMessage());
		} catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
	}
	
	public static void main(String args[]) {
		try {
			Handler fh = new FileHandler("logging.txt");
			fh.setFormatter(new SimpleFormatter());
			Logger.getLogger("de.javawi.stun").addHandler(fh);
			Logger.getLogger("de.javawi.stun").setLevel(Level.ALL);

			Enumeration<NetworkInterface> ifaces = NetworkInterface.getNetworkInterfaces();
			while (ifaces.hasMoreElements()) {
				NetworkInterface iface = ifaces.nextElement();
				Enumeration<InetAddress> iaddresses = iface.getInetAddresses();
				while (iaddresses.hasMoreElements()) {
					InetAddress iaddress = iaddresses.nextElement();
					if (Class.forName("java.net.Inet4Address").isInstance(iaddress)) {
						if ((!iaddress.isLoopbackAddress()) && (!iaddress.isLinkLocalAddress())) {
							Thread thread = new Thread(new DiscoveryTestDemo(iaddress));
							thread.start();
						}
					}
				}
			}
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}
	}
}
