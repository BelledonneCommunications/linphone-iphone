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
package net.jxta.impl.endpoint.tcp;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.nio.channels.CancelledKeyException;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.ClosedSelectorException;
import java.nio.channels.IllegalBlockingModeException;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.EmptyStackException;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.Stack;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.RejectedExecutionException;
import java.util.logging.Level;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attribute;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.MessageReceiver;
import net.jxta.endpoint.MessageSender;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.MessengerEvent;
import net.jxta.endpoint.MessengerEventListener;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.logging.Logging;
import net.jxta.meter.MonitorResources;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.TransportAdvertisement;

import net.jxta.impl.endpoint.IPUtils;
import net.jxta.impl.endpoint.LoopbackMessenger;
import net.jxta.impl.endpoint.transportMeter.TransportBindingMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeterBuildSettings;
import net.jxta.impl.endpoint.transportMeter.TransportServiceMonitor;
import net.jxta.impl.meter.MonitorManager;
import net.jxta.impl.peergroup.StdPeerGroup;
import net.jxta.impl.protocol.TCPAdv;
import net.jxta.impl.util.TimeUtils;


/**
 * This class implements the TCP Message Transport.
 *
 * @see net.jxta.endpoint.MessageTransport
 * @see net.jxta.endpoint.MessagePropagater
 * @see net.jxta.endpoint.MessageReceiver
 * @see net.jxta.endpoint.MessageSender
 * @see net.jxta.endpoint.EndpointService
 * @see <a href="http://spec.jxta.org/v1.0/docbook/JXTAProtocols.html#trans-tcpipt">JXTA Protocols Specification : Standard JXTA Transport Bindings</a>
 */
public class TcpTransport implements Module, MessageSender, MessageReceiver {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(TcpTransport.class.getName());

    /**
     * The TCP send buffer size.
     * The size of the buffer used to store outgoing messages
     * This should be set to the maximum message size (smaller is allowed).
     */
    static final int SendBufferSize = 64 * 1024; // 64 KBytes

    /**
     * The TCP receive buffer size
     */
    static final int RecvBufferSize = 64 * 1024; // 64 KBytes

    /**
     * The amount of time the socket "lingers" after we close it locally.
     * Linger enables the remote socket to finish receiving any pending data
     * at its own rate.
     * Note: LingerDelay time unit is seconds
     */
    static final int LingerDelay = 2 * 60;

    /**
     * Connection  timeout
     * use the same system property defined by URLconnection, otherwise default to 10 seconds.
     */
    static int connectionTimeOut = 10 * (int) TimeUtils.ASECOND;

    // Java's default is 50
    static final int MaxAcceptCnxBacklog = 50;

    private String serverName = null;
    private final List<EndpointAddress> publicAddresses = new ArrayList<EndpointAddress>();
    private EndpointAddress publicAddress = null;

    private String interfaceAddressStr;
    InetAddress usingInterface;
    private int serverSocketPort;
    private int restrictionPort = -1;
    private IncomingUnicastServer unicastServer = null;

    private boolean isClosed = false;

    private long messagesSent = 0;
    private long messagesReceived = 0;
    private long bytesSent = 0;
    private long bytesReceived = 0;
    private long connectionsAccepted = 0;

    PeerGroup group = null;
    EndpointService endpoint = null;
    Executor executor;

    private String protocolName = "tcp";
    private TransportMeter unicastTransportMeter;
    private TransportMeter multicastTransportMeter;

    private boolean publicAddressOnly = false;

    private MessengerEventListener messengerEventListener = null;

    private Thread messengerSelectorThread;
    Selector messengerSelector = null;

    private final Map<TcpMessenger, SocketChannel> regisMap = new ConcurrentHashMap<TcpMessenger, SocketChannel>();
    private final Set<SocketChannel> unregisMap = Collections.synchronizedSet(new HashSet<SocketChannel>());

    /**
     * This is the thread group into which we will place all of the threads
     * we create. THIS HAS NO EFFECT ON SCHEDULING. Java thread groups are
     * only for organization and naming.
     */
    ThreadGroup myThreadGroup = null;

    /**
     * The maximum number of write selectors we will maintain in our cache per
     * transport instance.
     */
    protected final static int MAX_WRITE_SELECTORS = 50;

    /**
     * A cache we maintain for selectors writing messages to the socket.
     */
    private final static Stack<Selector> writeSelectorCache = new Stack<Selector>();

    /**
     * The number of excess write selectors believed to be in the pool.
     */
    private int extraWriteSelectors = 0;

    /**
     * Construct a new TcpTransport instance
     */
    public TcpTransport() {
        // Add some selectors to the pool.
        try {
            for (int i = 0; i < MAX_WRITE_SELECTORS; i++) {
                writeSelectorCache.add(Selector.open());
            }
        } catch (IOException ex) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Failed adding selector to  write selector pool");
            }
        }

        try {
            String connectTOStr = System.getProperty("sun.net.client.defaultConnectTimeout");

            if (connectTOStr != null) {
                connectionTimeOut = Integer.parseInt(connectTOStr);
            }
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Could not parse system property: sun.net.client.defaultConnectTimeout");
            }
        }
    }

    /**
     * Gets the number of 'connectionsAccepted'.
     *
     * @return the number of 'connectionsAccepted'.
     */
    public long getConnectionsAccepted() {
        return connectionsAccepted;
    }

    /**
     * increment the number of connectionsAccepted sent by 1
     */
    public void incrementConnectionsAccepted() {
        connectionsAccepted++;
    }

    /**
     * increment the number of messages sent by 1
     */
    public void incrementMessagesSent() {
        messagesSent++;
    }

    /**
     * increment the number of messages received by 1
     */
    public void incrementMessagesReceived() {
        messagesReceived++;
    }

    /**
     * increment the number of bytes sent
     *
     * @param bytes the number of bytes to be added
     */
    public void incrementBytesSent(long bytes) {
        bytesSent += bytes;
    }

    /**
     * increment the number of bytes received
     *
     * @param bytes the number of bytes to be added
     */
    public void incrementBytesReceived(long bytes) {
        bytesReceived += bytes;
    }

    /**
     * Gets the number of 'messagesSent'.
     *
     * @return the number of 'messagesSent'.
     */
    public long getMessagesSent() {
        return messagesSent;
    }

    /**
     * Gets the number of 'messagesReceived'.
     *
     * @return the number of 'messagesReceived'.
     */
    public long getMessagesReceived() {
        return messagesReceived;
    }

    /**
     * Gets the number of 'bytesSent'.
     *
     * @return the number of 'bytesSent'.
     */
    public long getBytesSent() {
        return bytesSent;
    }

    /**
     * Gets the number of 'bytesReceived'.
     *
     * @return the number of 'bytesReceived'.
     */
    public long getBytesReceived() {
        return bytesReceived;
    }

    /**
     * {@inheritDoc}
     */
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }

        if (null == target) {
            return false;
        }

        if (target instanceof TcpTransport) {
            TcpTransport likeMe = (TcpTransport) target;

            if (!getProtocolName().equals(likeMe.getProtocolName())) {
                return false;
            }

            Iterator<EndpointAddress> itsAddrs = likeMe.publicAddresses.iterator();

            for (EndpointAddress publicAddress1 : publicAddresses) {
                if (!itsAddrs.hasNext()) {
                    return false;
                } // it has fewer than i do.

                EndpointAddress mine = publicAddress1;
                EndpointAddress its = itsAddrs.next();

                if (!mine.equals(its)) {
                    // content didnt match
                    return false;
                }
            }
            // ran out at the same time?
            return (!itsAddrs.hasNext());
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public int hashCode() {
        return getPublicAddress().hashCode();
    }

    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {

        this.group = group;
        ModuleImplAdvertisement implAdvertisement = (ModuleImplAdvertisement) impl;

        this.executor = ((StdPeerGroup) group).getExecutor();

        ConfigParams configAdv = group.getConfigAdvertisement();

        // Get out invariable parameters from the implAdv
        XMLElement param = (XMLElement) implAdvertisement.getParam();

        if (param != null) {
            Enumeration<XMLElement> list = param.getChildren("Proto");

            if (list.hasMoreElements()) {
                XMLElement pname = list.nextElement();
                protocolName = pname.getTextValue();
            }
        }

        // Get our peer-defined parameters in the configAdv
        param = (XMLElement) configAdv.getServiceParam(assignedID);
        if (null == param) {
            throw new IllegalArgumentException(TransportAdvertisement.getAdvertisementType() + " could not be located.");
        }

        Enumeration<XMLElement> tcpChilds = param.getChildren(TransportAdvertisement.getAdvertisementType());

        // get the TransportAdv
        if (tcpChilds.hasMoreElements()) {
            param = tcpChilds.nextElement();
            Attribute typeAttr = param.getAttribute("type");

            if (!TCPAdv.getAdvertisementType().equals(typeAttr.getValue())) {
                throw new IllegalArgumentException("transport adv is not a " + TCPAdv.getAdvertisementType());
            }

            if (tcpChilds.hasMoreElements()) {
                throw new IllegalArgumentException("Multiple transport advs detected for " + assignedID);
            }
        } else {
            throw new IllegalArgumentException(TransportAdvertisement.getAdvertisementType() + " could not be located.");
        }

        Advertisement paramsAdv = null;

        try {
            paramsAdv = AdvertisementFactory.newAdvertisement(param);
        } catch (NoSuchElementException notThere) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Could not find parameter document", notThere);
            }
        }

        if (!(paramsAdv instanceof TCPAdv)) {
            throw new IllegalArgumentException("Provided Advertisement was not a " + TCPAdv.getAdvertisementType());
        }

        TCPAdv adv = (TCPAdv) paramsAdv;

        // determine the local interface to use. If the user specifies
        // one, use that. Otherwise, use the all the available interfaces.
        interfaceAddressStr = adv.getInterfaceAddress();
        if (interfaceAddressStr != null) {
            try {
                usingInterface = InetAddress.getByName(interfaceAddressStr);
            } catch (UnknownHostException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Invalid address for local interface address, using default");
                }
                usingInterface = IPUtils.ANYADDRESS;
            }
        } else {
            usingInterface = IPUtils.ANYADDRESS;
        }

        serverName = adv.getServer();

        // Even when server is not enabled, we use the serverSocketPort as a 
        // discriminant for the simulated network partitioning, human readable
        // messages, and a few things of that sort.
        serverSocketPort = adv.getPort();

        // should we expose other than a public address if one was specified?
        publicAddressOnly = adv.getPublicAddressOnly();

        // Start the servers
        if (adv.isServerEnabled()) {
            try {
                unicastServer = new IncomingUnicastServer(this, usingInterface, serverSocketPort, adv.getStartPort(), adv.getEndPort());
            } catch (IOException failed) {
                throw new PeerGroupException("Failed to open server socket.", failed);
            }

            InetSocketAddress boundAddress = unicastServer.getLocalSocketAddress();

            // TODO bondolo 20040628 Save the port back as a preference to TCPAdv
            /*
            if(-1 != adv.getStartPort()) {
                adv.setPort(boundAddress.getPort());
            }
            */

            // Build the publicAddresses :
            // first in the list is the "public server name". We don't try to
            // resolve this since it might not be resolvable in the context we 
            // are running in, we just assume it's good.
            if (serverName != null) {
                // use speced server name.
                EndpointAddress newAddr = new EndpointAddress(protocolName, serverName, null, null);
                publicAddresses.add(newAddr);
            }

            // then add the rest of the local interfaces as appropriate. Unless
            // we find an non-loopback interface, we're in local only mode.
            boolean localOnly = true;

            if (usingInterface.equals(IPUtils.ANYADDRESS)) {
                // its wildcarded
                Iterator eachLocal = IPUtils.getAllLocalAddresses();
                List<EndpointAddress> wildAddrs = new ArrayList<EndpointAddress>();

                while (eachLocal.hasNext()) {
                    InetAddress anAddress = (InetAddress) eachLocal.next();
                    String hostAddress = IPUtils.getHostAddress(anAddress);
                    EndpointAddress newAddr = new EndpointAddress(protocolName,
                            hostAddress + ":" + Integer.toString(boundAddress.getPort()), null, null);

                    // don't add it if its already in the list
                    if (!anAddress.isLoopbackAddress()) {
                        localOnly = false;
                    }

                    if (!publicAddresses.contains(newAddr)) {
                        wildAddrs.add(newAddr);
                    }
                }

                // we sort them so that later equals() will be deterministic.
                // the result of IPUtils.getAllLocalAddresses() is not known to 
                // be sorted.
                Collections.sort(wildAddrs, new Comparator<EndpointAddress>() {
                    public int compare(EndpointAddress one, EndpointAddress two) {
                        return one.toString().compareTo(two.toString());
                    }

                    public boolean equals(Object that) {
                        return (this == that);
                    }
                });

                // Add public addresses:
                // don't add them if we have a hand-set public address and the
                // publicAddressOnly property is set.
                if (!(serverName != null && publicAddressOnly)) {
                    publicAddresses.addAll(wildAddrs);
                }
            } else {
                // use specified interface
                if (!usingInterface.isLoopbackAddress()) {
                    localOnly = false;
                }

                String hostAddress = IPUtils.getHostAddress(usingInterface);
                EndpointAddress newAddr = new EndpointAddress(protocolName,
                        hostAddress + ":" + Integer.toString(boundAddress.getPort()), null, null);

                // Add public address:
                // don't add it if its already in the list
                // don't add it if specified as public address and publicAddressOnly
                if (!(serverName != null && publicAddressOnly)) {
                    if (!publicAddresses.contains(newAddr)) {
                        publicAddresses.add(newAddr);
                    }
                }
            }

            // If the only available interface is LOOPBACK, then make sure we 
            // use only that (that includes resetting the outgoing/listening 
            // interface from ANYADDRESS to LOOPBACK).

            if (localOnly) {
                usingInterface = IPUtils.LOOPBACK;
                publicAddresses.clear();
                String hostAddress = IPUtils.getHostAddress(usingInterface);
                EndpointAddress pubAddr = new EndpointAddress(protocolName,
                        hostAddress + ":" + Integer.toString(boundAddress.getPort()), null, null);

                publicAddresses.add(pubAddr);
            }

            // Set the "preferred" public address. This is the address we will 
            // use for identifying outgoing requests.
            publicAddress = publicAddresses.get(0);
        } else {
            // Only the outgoing interface matters.
            // Verify that ANY interface does not in fact mean LOOPBACK only. If
            // that's the case, we want to make that explicit, so that 
            // consistency checks regarding the allowed use of that interface
            // work properly.
            if (usingInterface.equals(IPUtils.ANYADDRESS)) {
                boolean localOnly = true;
                Iterator eachLocal = IPUtils.getAllLocalAddresses();

                while (eachLocal.hasNext()) {
                    InetAddress anAddress = (InetAddress) eachLocal.next();

                    if (!anAddress.isLoopbackAddress()) {
                        localOnly = false;
                        break;
                    }
                }

                if (localOnly) {
                    usingInterface = IPUtils.LOOPBACK;
                }
            }

            // The "public" address is just an internal label
            // it is not usefull to anyone outside.
            // IMPORTANT: we set the port to zero, to signify that this address
            // is not realy usable.
            String hostAddress = IPUtils.getHostAddress(usingInterface);

            publicAddress = new EndpointAddress(protocolName, hostAddress + ":0", null, null);
        }

        // Tell tell the world about our configuration.
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring TCP Message Transport : " + assignedID);

            if (implAdvertisement != null) {
                configInfo.append("\n\tImplementation :");
                configInfo.append("\n\t\tModule Spec ID: ").append(implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description : ").append(implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI : ").append(implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code : ").append(implAdvertisement.getCode());
            }

            configInfo.append("\n\tGroup Params:");
            configInfo.append("\n\t\tGroup : ").append(group);
            configInfo.append("\n\t\tPeer ID: ").append(group.getPeerID());

            configInfo.append("\n\tConfiguration:");
            configInfo.append("\n\t\tProtocol: ").append(protocolName);
            configInfo.append("\n\t\tPublic address: ").append(serverName == null ? "(unspecified)" : serverName);
            configInfo.append("\n\t\tInterface address: ").append(
                    interfaceAddressStr == null ? "(unspecified)" : interfaceAddressStr);

            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tUsing Interface: ").append(usingInterface.getHostAddress());

            if (null != unicastServer) {
                if (-1 == unicastServer.getStartPort()) {
                    configInfo.append("\n\t\tUnicast Server Bind Addr: ").append(usingInterface.getHostAddress()).append(":").append(
                            serverSocketPort);
                } else {
                    configInfo.append("\n\t\tUnicast Server Bind Addr: ").append(usingInterface.getHostAddress()).append(":").append(serverSocketPort).append(" [").append(unicastServer.getStartPort()).append("-").append(unicastServer.getEndPort()).append(
                            "]");
                }
                configInfo.append("\n\t\tUnicast Server Bound Addr: ").append(unicastServer.getLocalSocketAddress());
            } else {
                configInfo.append("\n\t\tUnicast Server : disabled");
            }

            configInfo.append("\n\t\tPublic Addresses: ");
            configInfo.append("\n\t\t\tDefault Endpoint Addr : ").append(publicAddress);

            for (EndpointAddress anAddr : publicAddresses) {
                configInfo.append("\n\t\t\tEndpoint Addr : ").append(anAddr);
            }
            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized int startApp(String[] arg) {
        endpoint = group.getEndpointService();

        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }
            return Module.START_AGAIN_STALLED;
        }

        try {
            messengerSelector = SelectorProvider.provider().openSelector();
        } catch (IOException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not create a messenger selector", e);
            }
        }

        messengerSelectorThread = new Thread(group.getHomeThreadGroup(), new MessengerSelectorThread(), "TCP Transport MessengerSelectorThread for " + this);
        messengerSelectorThread.setDaemon(true);
        messengerSelectorThread.start();

        // We're fully ready to function.
        messengerEventListener = endpoint.addMessageTransport(this);

        if (messengerEventListener == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Transport registration refused");
            }
            return -1;
        }

        // Cannot start before registration, we could be announcing new messengers while we
        // do not exist yet ! (And get an NPE because we do not have the messenger listener set).

        if (unicastServer != null) {
            if (!unicastServer.start()) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe("Unable to start TCP Unicast Server");
                }
                return -1;
            }
        }

        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            TransportServiceMonitor transportServiceMonitor = (TransportServiceMonitor) MonitorManager.getServiceMonitor(group,
                    MonitorResources.transportServiceMonitorClassID);

            if (transportServiceMonitor != null) {
                unicastTransportMeter = transportServiceMonitor.createTransportMeter("TCP", publicAddress);
            }
        }

        isClosed = false;

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("TCP Message Transport started.");
        }
        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void stopApp() {
        if (isClosed) {
            return;
        }

        isClosed = true;

        if (unicastServer != null) {
            unicastServer.stop();
            unicastServer = null;
        }

        Thread temp = messengerSelectorThread;

        if (null != temp) {
            temp.interrupt();
            try {
                messengerSelector.close();
            } catch (IOException failed) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "IO error occured while closing server socket", failed);
                }
            }
        }

        // Inform the pool that we don't need as many write selectors.
        synchronized (writeSelectorCache) {
            extraWriteSelectors += MAX_WRITE_SELECTORS;
        }

        endpoint.removeMessageTransport(this);

        endpoint = null;
        group = null;

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info(MessageFormat.format("Total bytes sent : {0}", getBytesSent()));
            LOG.info(MessageFormat.format("Total Messages sent : {0}", getMessagesSent()));
            LOG.info(MessageFormat.format("Total bytes received : {0}", getBytesReceived()));
            LOG.info(MessageFormat.format("Total Messages received : {0}", getMessagesReceived()));
            LOG.info(MessageFormat.format("Total connections accepted : {0}", getConnectionsAccepted()));

            LOG.info("TCP Message Transport shut down.");
        }
    }

    /**
     * {@inheritDoc}
     */
    public String getProtocolName() {
        return protocolName;
    }

    /**
     * {@inheritDoc}
     */
    public EndpointAddress getPublicAddress() {
        return publicAddress;
    }

    /**
     * {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        return (EndpointService) endpoint.getInterface();
    }

    /**
     * {@inheritDoc}
     */
    public Object transportControl(Object operation, Object Value) {
        return null;
    }

    /**
     * {@inheritDoc}
     */
    public Iterator<EndpointAddress> getPublicAddresses() {
        return Collections.unmodifiableList(publicAddresses).iterator();
    }

    /**
     * {@inheritDoc}
     */
    public boolean isConnectionOriented() {
        return true;
    }

    /**
     * {@inheritDoc}
     */
    public boolean allowsRouting() {
        return true;
    }

    public Messenger getMessenger(EndpointAddress dst, Object hintIgnored) {
        return getMessenger(dst, hintIgnored, true);
    }

    /**
     * {@inheritDoc}
     */
    public Messenger getMessenger(EndpointAddress dst, Object hintIgnored, boolean selfDestruct) {

        if (!dst.getProtocolName().equalsIgnoreCase(getProtocolName())) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Cannot make messenger for protocol: " + dst.getProtocolName());
            }
            return null;
        }

        EndpointAddress plainAddr = new EndpointAddress(dst, null, null);

        // If the destination is one of our addresses including loopback, we 
        // return a loopback messenger.
        if (publicAddresses.contains(plainAddr)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("return LoopbackMessenger for addr : " + dst);
            }
            return new LoopbackMessenger(group, endpoint, getPublicAddress(), dst,
                    new EndpointAddress("jxta", group.getPeerID().getUniqueValue().toString(), null, null));
        }

        try {
            // Right now we do not want to "announce" outgoing messengers because they get pooled and so must
            // not be grabbed by a listener. If "announcing" is to be done, that should be by the endpoint
            // and probably with a subtely different interface.
            return new TcpMessenger(dst, this, selfDestruct);
        } catch (Exception caught) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.log(Level.FINER, "Could not get messenger for " + dst, caught);
                } else {
                    LOG.warning("Could not get messenger for " + dst + " : " + caught.getMessage());
                }
            }
            if (caught instanceof RuntimeException) {
                throw (RuntimeException) caught;
            }
            return null;
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This implementation tries to open a connection, and after tests the
     * result.
     */
    public boolean ping(EndpointAddress addr) {
        boolean result = false;
        EndpointAddress endpointAddress;
        long pingStartTime = 0;

        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            pingStartTime = System.currentTimeMillis();
        }

        endpointAddress = new EndpointAddress(addr, null, null);

        try {
            // Too bad that this one will not get pooled. On the other hand ping is
            // not here too stay.
            TcpMessenger tcpMessenger = new TcpMessenger(endpointAddress, this);

            if (TransportMeterBuildSettings.TRANSPORT_METERING) {
                TransportBindingMeter transportBindingMeter = tcpMessenger.getTransportBindingMeter();

                if (transportBindingMeter != null) {
                    transportBindingMeter.ping(System.currentTimeMillis() - pingStartTime);
                }
            }
            result = true;
        } catch (Throwable e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failure pinging " + addr.toString(), e);
            }
            if (TransportMeterBuildSettings.TRANSPORT_METERING) {
                TransportBindingMeter transportBindingMeter = getUnicastTransportBindingMeter(null, endpointAddress);

                if (transportBindingMeter != null) {
                    transportBindingMeter.pingFailed(System.currentTimeMillis() - pingStartTime);
                }
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("ping to " + addr.toString() + " == " + result);
        }
        return result;
    }

    /**
     * Getter for property 'restrictionPort'.
     *
     * @return Value for property 'restrictionPort'.
     */
    int getRestrictionPort() {
        return restrictionPort;
    }

    TransportBindingMeter getUnicastTransportBindingMeter(PeerID peerID, EndpointAddress destinationAddress) {
        if (unicastTransportMeter != null) {
            return unicastTransportMeter.getTransportBindingMeter(
                    (peerID != null) ? peerID.toString() : TransportMeter.UNKNOWN_PEER, destinationAddress);
        } else {
            return null;
        }
    }

    void messengerReadyEvent(Messenger newMessenger, EndpointAddress connAddr) {
        messengerEventListener.messengerReady(new MessengerEvent(this, newMessenger, connAddr));
    }

    /**
     * Getter for property 'server'.
     *
     * @return Value for property 'server'.
     */
    IncomingUnicastServer getServer() {
        return unicastServer;

    }

    /**
     * Get a write selector from the cache.
     *
     * @return A write selector.
     * @throws InterruptedException If interrupted while waiting for a selector
     *                              to become available.
     */
    Selector getSelector() throws InterruptedException {
        synchronized (writeSelectorCache) {
            Selector selector = null;
            try {
                if (!writeSelectorCache.isEmpty()) {
                    selector = writeSelectorCache.pop();
                }
            } catch (EmptyStackException ese) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("No write selector available, waiting for one");
                }
            }

            int attempts = 0;
            while (selector == null && attempts < 2) {
                writeSelectorCache.wait(connectionTimeOut);
                try {
                    if (!writeSelectorCache.isEmpty()) {
                        selector = writeSelectorCache.pop();
                    }
                } catch (EmptyStackException ese) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Failed to get a write selector available, waiting for one", ese);
                    }
                }
                attempts++;
            }

            return selector;
        }
    }

    /**
     * Return the <code>Selector</code> to the cache
     *
     * @param selector the selector to put back into the pool
     */
    void returnSelector(Selector selector) {
        synchronized (writeSelectorCache) {
            if (extraWriteSelectors > 0) {
                // Allow the selector to be discarded.
                extraWriteSelectors--;
            } else {
                writeSelectorCache.push(selector);
                // it does not hurt to notify, even if there are no waiters
                writeSelectorCache.notify();
            }
        }
    }

    /**
     * Waits for incoming data on channels and sends it to the appropriate
     * messenger object.
     */
    private class MessengerSelectorThread implements Runnable {

        /**
         * {@inheritDoc}
         */
        public void run() {
            try {
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("MessengerSelectorThread polling started");
                }

                while (!isClosed) {
                    try {
                        int selectedKeys = 0;

                        // Update channel registerations.
                        updateChannelRegisterations();

                        try {
                            // this can be interrupted through wakeup
                            selectedKeys = messengerSelector.select();
                        } catch (CancelledKeyException cke) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.log(Level.FINE, "Key was cancelled", cke);
                            }
                        }

                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(MessageFormat.format("MessengerSelector has {0} selected keys", selectedKeys));
                        }

                        if (selectedKeys == 0 && messengerSelector.selectNow() == 0) {
                            // We were probably just woken.
                            continue;
                        }

                        Set<SelectionKey> keySet = messengerSelector.selectedKeys();

                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(MessageFormat.format("KeySet has {0} selected keys", keySet.size()));
                        }

                        Iterator<SelectionKey> it = keySet.iterator();

                        while (it.hasNext()) {
                            SelectionKey key = it.next();

                            // remove it from the SelectedKeys Set
                            it.remove();

                            if (key.isValid()) {
                                try {
                                    if (key.isReadable() && key.channel().isOpen()) {
                                        // ensure this channel is not selected again until the thread is done with it
                                        // TcpMessenger is expected to reset the interestOps back to OP_READ
                                        // Without this, expect multiple threads to execute on the same event, until
                                        // the first thread completes reading all data available
                                        key.interestOps(key.interestOps() & ~SelectionKey.OP_READ);

                                        // get the messenger
                                        TcpMessenger msgr = (TcpMessenger) key.attachment();

                                        // process the data
                                        try {
                                            executor.execute(msgr);
                                        } catch (RejectedExecutionException re) {
                                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                                LOG.log(Level.FINE,
                                                        MessageFormat.format("Executor rejected task for messenger :{0}", msgr.toString()), re);
                                            }
                                        }
                                    }
                                } catch (CancelledKeyException cce) {
                                    //in case the key was canceled after the selection
                                }
                            } else {
                                // unregister it, no need to keep invalid/closed channels around
                                try {
                                    key.channel().close();
                                } catch (IOException io) {
                                    // avoids breaking out of the selector loop
                                }
                                key.cancel();
                                key = null;
                            }
                        }
                    } catch (ClosedSelectorException cse) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("IO Selector closed");
                        }
                    } catch (InterruptedIOException woken) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Thread inturrupted", woken);
                        }
                    } catch (IOException e1) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "An exception occurred while selecting keys", e1);
                        }
                    } catch (SecurityException e2) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "A security exception occurred while selecting keys", e2);
                        }
                    }
                }

                // XXX 20070205 bondolo What should we do about the channels 
                // that are still registered with the selector and any pending 
                // updates?

            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && Logging.SHOW_SEVERE) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable", all);
                }
            } finally {
                messengerSelectorThread = null;
            }
        }
    }

    /**
     * Registers the channel with the Read selector and attaches the messenger to the channel
     *
     * @param channel   the socket channel.
     * @param messenger the messenger to attach to the channel.
     */
    void register(SocketChannel channel, TcpMessenger messenger) {
        regisMap.put(messenger, channel);
        messengerSelector.wakeup();
    }

    /**
     * Unregisters the channel with the Read selector
     *
     * @param channel the socket channel.
     */
    void unregister(SocketChannel channel) {
        unregisMap.add(channel);
        messengerSelector.wakeup();
    }

    /**
     * Registers all newly accepted and returned (by TcpMessenger) channels.
     * Removes all closing TcpMessengers.
     */
    private synchronized void updateChannelRegisterations() {

        if (!regisMap.isEmpty() && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("Registering {0} channels with MessengerSelectorThread", regisMap.size()));
        }

        if (!regisMap.isEmpty()) {
            Iterator<Map.Entry<TcpMessenger, SocketChannel>> eachMsgr = regisMap.entrySet().iterator();

            while (eachMsgr.hasNext()) {
                Map.Entry<TcpMessenger, SocketChannel> anEntry = eachMsgr.next();
                TcpMessenger msgr = anEntry.getKey();
                SocketChannel channel = anEntry.getValue();
                SelectionKey key = channel.keyFor(messengerSelector);

                try {
                    if (key == null) {
                        key = channel.register(messengerSelector, SelectionKey.OP_READ, msgr);
                    }
                    key.interestOps(key.interestOps() | SelectionKey.OP_READ);
                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.finer(MessageFormat.format("Key interestOps on channel {0}, bit set :{1}", channel, key.interestOps()));
                    }
                } catch (ClosedChannelException e) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Failed to register Channel with messenger selector", e);
                    }
                    // it's best a new messenger is created when a new messenger is requested
                    msgr.close();
                } catch (CancelledKeyException e) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Key is already cancelled, removing key from registeration map", e);
                    }
                } catch (IllegalBlockingModeException e) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Invalid blocking channel mode, closing messenger", e);
                    }
                    // messenger state is unknown
                    msgr.close();
                }
                // remove it from the table
                eachMsgr.remove();
            }
        }

        // Unregister and close channels.
        if (!unregisMap.isEmpty() && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("Unregistering {0} channels with MessengerSelectorThread", unregisMap.size()));
        }
        if (!unregisMap.isEmpty()) {
            Iterator<SocketChannel> eachChannel;

            synchronized (unregisMap) {
                List<SocketChannel> allChannels = new ArrayList<SocketChannel>(unregisMap);
                unregisMap.clear();
                eachChannel = allChannels.iterator();
            }

            while (eachChannel.hasNext()) {
                SocketChannel aChannel = eachChannel.next();
                SelectionKey key = aChannel.keyFor(messengerSelector);
                if (null != key) {
                    try {
                        key.cancel();
                    } catch (CancelledKeyException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Key is already cancelled, removing key from registeration map", e);
                        }
                    }
                }
            }
        }
    }
}
