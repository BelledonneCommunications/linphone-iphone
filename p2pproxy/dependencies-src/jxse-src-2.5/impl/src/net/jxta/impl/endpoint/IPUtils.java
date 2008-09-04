/*
 * Copyright (c) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
 *  
 *  The Sun Project JXTA(TM) Software License
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *  
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation 
 *     and/or other materials provided with the distribution.
 *  
 *  3. The end-user documentation included with the redistribution, if any, must 
 *     include the following acknowledgment: "This product includes software 
 *     developed by Sun Microsystems, Inc. for JXTA(TM) technology." 
 *     Alternately, this acknowledgment may appear in the software itself, if 
 *     and wherever such third-party acknowledgments normally appear.
 *  
 *  4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must 
 *     not be used to endorse or promote products derived from this software 
 *     without prior written permission. For written permission, please contact 
 *     Project JXTA at http://www.jxta.org.
 *  
 *  5. Products derived from this software may not be called "JXTA", nor may 
 *     "JXTA" appear in their name, without prior written permission of Sun.
 *  
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 *  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SUN 
 *  MICROSYSTEMS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 *  JXTA is a registered trademark of Sun Microsystems, Inc. in the United 
 *  States and other countries.
 *  
 *  Please see the license information page at :
 *  <http://www.jxta.org/project/www/license.html> for instructions on use of 
 *  the license in source files.
 *  
 *  ====================================================================
 *  
 *  This software consists of voluntary contributions made by many individuals 
 *  on behalf of Project JXTA. For more information on Project JXTA, please see 
 *  http://www.jxta.org.
 *  
 *  This license is based on the BSD license adopted by the Apache Foundation. 
 */

package net.jxta.impl.endpoint;


import java.io.IOException;
import java.net.BindException;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import javax.net.ServerSocketFactory;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 * Utility methods for use by IP based transports.
 */
public final class IPUtils {
    
    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(IPUtils.class.getName());
    
    final static Random random = new Random();
    
    final static String IPV4ANYADDRESS = "0.0.0.0";
    final static String IPV6ANYADDRESS = "::";
    
    final static String IPV4LOOPBACK = "127.0.0.1";
    final static String IPV6LOOPBACK = "::1";
    
    /**
     * Constant which works as the IP "Any Address" value
     */
    public final static InetAddress ANYADDRESS;
    public final static InetAddress ANYADDRESSV4;
    public final static InetAddress ANYADDRESSV6;
    
    /**
     * Constant which works as the IP "Local Loopback" value;
     */
    public final static InetAddress LOOPBACK;
    public final static InetAddress LOOPBACKV4;
    public final static InetAddress LOOPBACKV6;
    
    /**
     * Socket factory to allow changing the way Sockets are created
     * and connected.  A null value is ok and results in the regular
     * connectToFromNoFactory being used.
     *
     * <p/>Plugin in a different implementation via setSocketFactory().
     */
    private static SocketFactory socketFactory;
    
    /**
     * Socket factory to allow changing the way Sockets are created
     * and connected.  A null value is ok and results in the regular
     * connectToFromNoFactory being used.
     *
     * <p/>Plugin in a different implementation via setSocketFactory().
     */
    private static ServerSocketFactory serverSocketFactory;
    
    static {
        InetAddress GET_ADDRESS = null;
        
        try {
            GET_ADDRESS = InetAddress.getByName(IPV4ANYADDRESS);
        } catch (Exception ignored) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("failed to intialize ANYADDRESSV4. Not fatal");
            }
        }
        
        ANYADDRESSV4 = GET_ADDRESS;

        InetAddress GET_ANYADDRESSV6 = null;

        GET_ADDRESS = null;
        try {
            GET_ADDRESS = InetAddress.getByName(IPV6ANYADDRESS);
        } catch (Exception ignored) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("failed to intialize IPV6ANYADDRESS. Not fatal");
            }
        }
        
        ANYADDRESSV6 = GET_ADDRESS;
        
        ANYADDRESS = (ANYADDRESSV4 == null) ? ANYADDRESSV6 : ANYADDRESSV4;
        
        GET_ADDRESS = null;
        try {
            GET_ADDRESS = InetAddress.getByName(IPV4LOOPBACK);
        } catch (Exception ignored) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("failed to intialize IPV4LOOPBACK. Not fatal");
            }
        }
        
        LOOPBACKV4 = GET_ADDRESS;
        
        GET_ADDRESS = null;
        try {
            GET_ADDRESS = InetAddress.getByName(IPV6LOOPBACK);
        } catch (Exception ignored) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("failed to intialize ANYADDRESSV4. Not fatal");
            }
        }
        
        LOOPBACKV6 = GET_ADDRESS;
        
        LOOPBACK = (LOOPBACKV4 == null) ? LOOPBACKV6 : LOOPBACKV4;
        
        if (LOOPBACK == null || ANYADDRESS == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("failure initializing statics. Neither IPV4 nor IPV6 seem to work.");
            }
            
            throw new IllegalStateException("failure initializing statics. Neither IPV4 nor IPV6 seem to work.");
        }
    }
    
    /**
     * This is a static utility class, you don't make instances.
     */
    private IPUtils() {}
    
    /**
     * Provide an iterator which returns all of the local InetAddresses for this
     * host.
     *
     * @return iterator of InetAddress which is all of the InetAddress for all
     *         local interfaces.
     */
    public static Iterator<InetAddress> getAllLocalAddresses() {
        List<InetAddress> allAddr = new ArrayList<InetAddress>();
        
        Enumeration<NetworkInterface> allInterfaces = null;

        try {
            allInterfaces = NetworkInterface.getNetworkInterfaces();
        } catch (SocketException caught) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Could not get local interfaces list", caught);
            }
        }
        
        if (null == allInterfaces) {
            allInterfaces = Collections.enumeration(Collections.<NetworkInterface>emptyList());
        }
        
        while (allInterfaces.hasMoreElements()) {
            NetworkInterface anInterface = allInterfaces.nextElement();
            
            try {
                Enumeration<InetAddress> allIntfAddr = anInterface.getInetAddresses();
                
                while (allIntfAddr.hasMoreElements()) {
                    InetAddress anAddr = allIntfAddr.nextElement();
                    
                    if (anAddr.isLoopbackAddress() || anAddr.isAnyLocalAddress()) {
                        continue;
                    }
                    
                    if (!allAddr.contains(anAddr)) {
                        allAddr.add(anAddr);
                    }
                }
            } catch (Throwable caught) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Could not get addresses for " + anInterface, caught);
                }
            }
        }
        
        // if nothing suitable was found then return loopback address.
        if (allAddr.isEmpty() || Boolean.getBoolean("net.jxta.impl.IPUtils.localOnly")) {
            if (null != LOOPBACKV4) {
                allAddr.add(LOOPBACKV4);
            }
            
            if (null != LOOPBACKV6) {
                allAddr.add(LOOPBACKV6);
            }
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Adding loopback interfaces");
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Returning " + allAddr.size() + " addresses.");
        }
        
        return allAddr.iterator();
    }
    
    /**
     * Normalized version of {@link java.net.InetAddress#getHostAddress()} that
     * handles IPv6 addresss formatting using the style of IETF RFC 2732 and
     * also handles removal of IPv6 scoping identifiers.
     *
     * @see <a href="http://www.ietf.org/rfc/rfc2732.txt" target="_blank">IETF RFC 2732 <i>MIME : IPv6 Literal Addresses in URL's</i></a>
     * @param anAddress The address to format as a <tt>String</tt>.
     * @return The addresss formatted as a String.
     */
    public static String getHostAddress(InetAddress anAddress) {
        String hostAddress;
        
        if (anAddress instanceof Inet6Address) {
            hostAddress = anAddress.getHostAddress();
            int percentAt = hostAddress.indexOf('%');
            
            if (-1 == percentAt) {
                // no scoping identifier. Just add the brackets.
                hostAddress = "[" + hostAddress + "]";
            } else {
                // Remove scoping identifier. They aren't relevant when published.
                hostAddress = "[" + hostAddress.substring(0, percentAt) + "]";
            }
        } else {
            hostAddress = anAddress.getHostAddress();
        }
        
        return hostAddress;
    }
    
    /**
     *  Parses a String containing a SokectAddress formatted as either:
     *  <p/>
     *  <pre>
     *     &lt;host> ":" &lt;port>
     *
     *     "[" &lt;numeric_host> "]:" &lt;port>
     *  </pre>
     *  <p/>
     *  @param anAddress The address string to be parsed.
     *  @return The parsed address.
     */
    public static InetSocketAddress parseSocketAddress(String anAddress) {
        String hostAddress;
        String port;
        
        if (anAddress.startsWith("[")) {
            int endBracketAt = anAddress.indexOf(']');
            int portSeparatorAt = anAddress.lastIndexOf(':');
            
            if (-1 == endBracketAt) {
                throw new IllegalArgumentException("missing final ]");
            }
            
            if (-1 == portSeparatorAt) {
                throw new IllegalArgumentException("missing port separator");
            }
            
            if (portSeparatorAt < endBracketAt) {
                throw new IllegalArgumentException("missing port");
            }
            
            hostAddress = anAddress.substring(1, endBracketAt);
            port = anAddress.substring(portSeparatorAt);
        } else {
            int portSeparatorAt = anAddress.lastIndexOf(':');
            
            if (-1 == portSeparatorAt) {
                throw new IllegalArgumentException("missing port separator");
            }
            
            hostAddress = anAddress.substring(0, portSeparatorAt);
            port = anAddress.substring(portSeparatorAt + 1);
        }
        
        int portNum = Integer.parseInt(port);
        
        return InetSocketAddress.createUnresolved(hostAddress, portNum);
    }
    
    /**
     * Create a client socket using the configured socketFactory or
     * connectToFromNoFactory if none is available.
     *
     * @param inetAddress    Destination address
     * @param port           Destination port
     * @param usingInterface Interface to use
     * @param localPort      local port
     * @param timeout        timeout in millis
     * @return a client socket with the JDK1.4 method connect().
     * @throws IOException if an io error occurs
     */
    public static Socket connectToFrom(InetAddress inetAddress, int port, InetAddress usingInterface, int localPort, int timeout) throws IOException {
        if (socketFactory != null) {
            return socketFactory.createConnection(inetAddress, port, usingInterface, localPort, timeout);
        } else {
            return connectToFromNoFactory(inetAddress, port, usingInterface, localPort, timeout);
        }
    }
    
    /**
     * Create a client socket with the JDK1.4 method connect().
     *
     * @param inetAddress    Destination address
     * @param port           Destination port
     * @param usingInterface Interface to use
     * @param localPort      local port
     * @param timeout        timeout in millis
     * @return a client socket with the JDK1.4 method connect().
     * @throws IOException if an io error occurs
     */
    public static Socket connectToFromNoFactory(InetAddress inetAddress, int port, InetAddress usingInterface, int localPort, int timeout) throws IOException {
        
        Socket socket = new Socket();
        InetSocketAddress src = new InetSocketAddress(usingInterface, localPort);
        InetSocketAddress dst = new InetSocketAddress(inetAddress, port);

        socket.bind(src);
        socket.connect(dst, timeout);
        
        return socket;
    }
    
    /**
     * makes connectToFrom create sockets with this factory.
     *
     * @param sf is the socket factory to use or null if you want the
     *           default behaviour provided by connectToFromNoFactory().
     */
    public static void setSocketFactory(SocketFactory sf) {
        socketFactory = sf;
    }
    
    /**
     * returns the socketFactory used by connectToFrom() to create sockets, or
     * null if connectToFromNoFactory() is being used.
     *
     * @return the socket factory used by connectToFrom() or null if
     *         the connectToFromNoFactory() method is used to create Sockets.
     */
    public static SocketFactory getSocketFactory() {
        return socketFactory;
    }
    
    /**
     * makes connectToFrom create sockets with this factory.
     *
     * @param sf is the socket factory to use or null if you want the
     *           default behaviour provided by new SeverSocket().
     */
    public static void setServerSocketFactory(ServerSocketFactory sf) {
        serverSocketFactory = sf;
    }
    
    /**
     * returns the ServerSocketFactory to create server sockets, or
     * null if new SeverSocket() is being used.
     *
     * @return the socket factory used or null if
     *         the new SeverSocket() method is used to create ServerSockets.
     */
    public static ServerSocketFactory getServerSocketFactory() {
        return serverSocketFactory;
    }
    
    /**
     * Size of port groups we will probe.
     */
    final static int rangesize = 200;
    
    /**
     * Open a ServerSocket in the specified range.
     *
     * <p/>
     * The method used is done so that the entire range is examined if
     * needed while ensuring that the process eventually terminates if no port
     * is available.
     *
     * @param start       The lowest numbered port to try.
     * @param end         The highest numbered port to try.
     * @param backlog     the allowed backlog of unaccepted connections.
     * @param bindAddress the InetAddress to which to bind.
     * @return a ServerSocket in the specified range.
     * @throws IOException when the socket cannot be opened. (Lame, but that's what ServerSocket says).
     */
    public static ServerSocket openServerSocketInRange(int start, int end, int backlog, InetAddress bindAddress) throws IOException {
        ServerSocketFactory factory = getServerSocketFactory();
        
        if ((start < 1) || (start > 65535)) {
            throw new IllegalArgumentException("Invalid start port");
        }
        
        if ((end < 1) || (end > 65535) || (end < start)) {
            throw new IllegalArgumentException("Invalid end port");
        }
        
        // fill the inRange array.
        List<Integer> inRange = new ArrayList<Integer>(rangesize);

        for (int eachInRange = 0; eachInRange < rangesize; eachInRange++) {
            inRange.add(eachInRange, eachInRange);
        }
        
        // fill the ranges array.
        List<Integer> ranges = new ArrayList<Integer>();
        int starts = start;
        
        while (starts <= end) {
            ranges.add(starts);
            starts += rangesize;
        }
        
        // shuffle the ranges
        Collections.shuffle(ranges);
        while (!ranges.isEmpty()) {
            int range = ranges.remove(0);
            
            // reshuffle the inRange
            Collections.shuffle(inRange);
            
            for (int eachInRange = 0; eachInRange < rangesize; eachInRange++) {
                int tryPort = range + inRange.get(eachInRange);
                
                if (tryPort > end) {
                    continue;
                }
                
                try {
                    ServerSocket result;

                    if (null == factory) {
                        result = new ServerSocket(tryPort, backlog, bindAddress);
                    } else {
                        result = factory.createServerSocket(tryPort, backlog, bindAddress);
                    }
                    
                    return result;
                } catch (BindException failed) {// this one is busy. try another.
                }
            }
        }
        throw new BindException("All ports in range are in use.");
    }
}
