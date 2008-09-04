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
package net.jxta.impl.endpoint.mcast;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MulticastSocket;
import java.net.SocketException;
import java.net.URI;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.NoSuchElementException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executor;
import java.util.logging.Level;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attribute;
import net.jxta.document.MimeMediaType;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.MessagePropagater;
import net.jxta.endpoint.MessengerEventListener;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.endpoint.WireFormatMessage;
import net.jxta.endpoint.WireFormatMessageFactory;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.logging.Logging;
import net.jxta.meter.MonitorResources;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.TransportAdvertisement;

import net.jxta.impl.endpoint.EndpointServiceImpl;
import net.jxta.impl.endpoint.IPUtils;
import net.jxta.impl.endpoint.msgframing.MessagePackageHeader;
import net.jxta.impl.endpoint.transportMeter.TransportBindingMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeterBuildSettings;
import net.jxta.impl.endpoint.transportMeter.TransportServiceMonitor;
import net.jxta.impl.meter.MonitorManager;
import net.jxta.impl.peergroup.StdPeerGroup;
import net.jxta.impl.protocol.TCPAdv;

/**
 * This class implements the IP Multicast Message Transport.
 * <p/>
 * <b>Important Note:</b> This implementation was formerly a portion of the TCP
 * Message Transport and currently uses the TCP Transport's configuration 
 * advertisement.
 *
 * @see net.jxta.endpoint.MessageTransport
 * @see net.jxta.endpoint.MessagePropagater
 * @see net.jxta.endpoint.EndpointService
 * @see <a href="http://spec.jxta.org/v1.0/docbook/JXTAProtocols.html#trans-tcpipt">JXTA Protocols Specification : Standard JXTA Transport Bindings</a>
 */
public class McastTransport implements Runnable, Module, MessagePropagater {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(McastTransport.class.getName());

    /**
     * Well known service class identifier: mcast message transport
     */
    public final static ModuleClassID MCAST_TRANSPORT_CLASSID =
            ModuleClassID.create(URI.create("urn:jxta:uuid-0C801F65D38F421C9884D706B337B81105"));

    /**
     * Well known service spec identifier: mcast message transport
     */
    public final static ModuleSpecID MCAST_TRANSPORT_SPECID =
            ModuleSpecID.create(URI.create("urn:jxta:uuid-0C801F65D38F421C9884D706B337B8110106"));

    /**
     * The Protocol name we will use for our endpoint addresses.
     */
    private String protocolName = "mcast";

    /**
     * Our Source Addres.
     */
    private EndpointAddress ourSrcAddr = null;

    /**
     * The Source Address Element we attach to all of the messages we send.
     */
    private MessageElement msgSrcAddrElement = null;

    /**
     * The name of the  local interface that we bind to.
     */
    private String interfaceAddressStr;

    /**
     * The address of the local interface address that be bind to.
     */
    private InetAddress usingInterface;

    /**
     * If {@code true} then we are closed otherwise {@code false}
     */
    private boolean isClosed = false;

    /**
     * The name of multicast address we will send/receive upon.
     */
    private String multicastAddress = "224.0.1.85";

    /**
     * The multicast address we will send/receive upon.
     */
    private InetAddress multicastInetAddress;

    /**
     * The port number will send/receive upon.
     */
    private int multicastPort = 1234;

    /**
     * The "return address" we will advertise.
     */
    private EndpointAddress publicAddress = new EndpointAddress(protocolName, multicastAddress + ":" + Integer.toString(multicastPort), null, null);

    /**
     * The maximum size of multicast messages we will send and the size of the
     * {@code DatagramPacket}s we will allocate.
     */
    private int multicastPacketSize = 16384;

    /**
     * The socket we use to send and receive.
     */
    private MulticastSocket multicastSocket = null;

    /**
     * Daemon thread which services the multicast socket and receives datagrams.
     */
    private Thread multicastThread = null;

    /**
     * Thread pooling/queing multicast datagram processor.
     */
    private DatagramProcessor multicastProcessor;

    /**
     * The peer group we are working for.
     */
    private PeerGroup group = null;

    /**
     * The Module Class ID we were assigned in init().
     */
    private ID assignedID = null;

    /**
     * The impl advertisement we were provided in init().
     */
    private ModuleImplAdvertisement implAdvertisement = null;

    /**
     * The endpoint service we are working for.
     */
    private EndpointService endpoint = null;

    private TransportMeter multicastTransportMeter;
    private TransportBindingMeter multicastTransportBindingMeter;
    private transient boolean disabled = false;

    /**
     * Construct a new McastTransport instance
     */
    public McastTransport() {
    }

    /**
     * {@inheritDoc}
     */
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }

        if (target instanceof McastTransport) {
            McastTransport likeMe = (McastTransport) target;
            return getProtocolName().equals(likeMe.getProtocolName()) && getPublicAddress().equals(likeMe.getPublicAddress());
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
        this.assignedID = assignedID;
        this.implAdvertisement = (ModuleImplAdvertisement) impl;

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
        param = (XMLElement) configAdv.getServiceParam(PeerGroup.tcpProtoClassID);
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
        
        // Check if we are disabled. If so, don't bother with the rest of config.
        if (!adv.getMulticastState()) {
            disabled = true;
            return;
        }

        // Determine the local interface to use. If the user specifies one, use
        // that. Otherwise, use the all the available interfaces.
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

        // Start the servers

        // Only the outgoing interface matters.
        // Verify that ANY interface does not in fact mean LOOPBACK only.
        // If that's the case, we want to make that explicit, so that
        // consistency checks regarding the allowed use of that interface work
        // properly.
        if (usingInterface.equals(IPUtils.ANYADDRESS)) {
            boolean localOnly = true;
            Iterator<InetAddress> eachLocal = IPUtils.getAllLocalAddresses();
            while (eachLocal.hasNext()) {
                InetAddress anAddress = eachLocal.next();
                if (!anAddress.isLoopbackAddress()) {
                    localOnly = false;
                    break;
                }
            }

            if (localOnly) {
                usingInterface = IPUtils.LOOPBACK;
            }
        }

        ourSrcAddr = new EndpointAddress(group.getPeerID(), null, null);
        msgSrcAddrElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NAME, ourSrcAddr.toString(), null);

        // Get the multicast configuration.
        multicastAddress = adv.getMulticastAddr();
        multicastPort = adv.getMulticastPort();

        // XXX 20070711 bondolo We resolve the address only once. Perhaps we should do this dynamically?
        try {
            multicastInetAddress = InetAddress.getByName(multicastAddress);
        } catch (UnknownHostException notValid) {
            IllegalArgumentException failed = new IllegalArgumentException("Invalid or unknown host name :" + multicastAddress);
            failed.initCause(notValid);
            throw failed;
        }

        assert multicastInetAddress.isMulticastAddress();
        publicAddress = new EndpointAddress(protocolName, multicastAddress + ":" + Integer.toString(multicastPort), null, null);
        multicastPacketSize = adv.getMulticastSize();

        // Create the multicast input socket
        try {
            multicastSocket = new MulticastSocket(new InetSocketAddress(usingInterface, multicastPort));
        } catch (IOException failed) {
            throw new PeerGroupException("Could not open multicast socket", failed);
        }

        try {
            // Surprisingly, "true" means disable....
            multicastSocket.setLoopbackMode(false);
        } catch (SocketException ignored) {
            // We may not be able to set loopback mode. It is inconsistent
            // whether an error will occur if the set fails.
        }

        // Tell tell the world about our configuration.
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring IP Multicast Message Transport : " + assignedID);

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
            configInfo.append("\n\t\tInterface address: ").append(interfaceAddressStr == null ? "(unspecified)" : interfaceAddressStr);
            configInfo.append("\n\t\tMulticast Addr: ").append(multicastAddress);
            configInfo.append("\n\t\tMulticast Port: ").append(multicastPort);
            configInfo.append("\n\t\tMulticast Packet Size: ").append(multicastPacketSize);

            configInfo.append("\n\tBound To :");
            configInfo.append("\n\t\tUsing Interface: ").append(usingInterface.getHostAddress());

            configInfo.append("\n\t\tMulticast Server Bind Addr: ").append(multicastSocket.getLocalSocketAddress());
            configInfo.append("\n\t\tPublic Address: ").append(publicAddress);

            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized int startApp(String[] arg) {
        if (disabled) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("IP Multicast Message Transport disabled.");
            }
            return Module.START_DISABLED;
        }

        endpoint = group.getEndpointService();
        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }
            return Module.START_AGAIN_STALLED;
        }

        isClosed = false;

        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            TransportServiceMonitor transportServiceMonitor = (TransportServiceMonitor) MonitorManager.getServiceMonitor(group,
                    MonitorResources.transportServiceMonitorClassID);

            if (transportServiceMonitor != null) {
                multicastTransportMeter = transportServiceMonitor.createTransportMeter("Multicast", publicAddress);
                multicastTransportBindingMeter = getMulticastTransportBindingMeter(publicAddress);
                // Since multicast is connectionless, force it to appear outbound connected
                multicastTransportBindingMeter.connectionEstablished(true, 0);
                // Since multicast is connectionless, force it to appear inbound connected
                multicastTransportBindingMeter.connectionEstablished(false, 0);
            }
        }

        // We're fully ready to function.
        MessengerEventListener messengerEventListener = endpoint.addMessageTransport(this);
        if (messengerEventListener == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Transport registration refused");
            }
            return -1;
        }

        // Cannot start before registration
        multicastProcessor = new DatagramProcessor(((StdPeerGroup) group).getExecutor());
        multicastThread = new Thread(group.getHomeThreadGroup(), this, "IP Multicast Listener for " + publicAddress);
        multicastThread.setDaemon(true);
        multicastThread.start();

        try {
            multicastSocket.joinGroup(multicastInetAddress);
        } catch (IOException soe) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Could not join multicast group, setting Multicast off");
            }
            return -1;
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("IP Multicast Message Transport started.");
        }

        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void stopApp() {
        if (isClosed || disabled) {
            return;
        }

        isClosed = true;

        if (multicastSocket != null) {
            multicastSocket.close();
            multicastSocket = null;
        }

        if (null != multicastProcessor) {
            multicastProcessor.stop();
            multicastProcessor = null;
        }

        endpoint.removeMessageTransport(this);

        if (TransportMeterBuildSettings.TRANSPORT_METERING && (multicastTransportBindingMeter != null)) {
            // Since multicast is connectionless, force it to appear outbound disconnected
            multicastTransportBindingMeter.connectionClosed(true, 0);
            // Since multicast is connectionless, force it to appear inbound disconnected
            multicastTransportBindingMeter.connectionClosed(false, 0);
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
     * <p/>
     * Handles incoming multicasts and enqueues them with the datagram processor.
     */
    public void run() {

        try {
            while (!isClosed) {
                byte[] buffer = new byte[multicastPacketSize];
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);

                try {
                    multicastSocket.receive(packet);

                    if (isClosed) {
                        return;
                    }

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("multicast message received from :" + packet.getAddress().getHostAddress());
                    }

                    // This operation is blocking and may take a long time to
                    // return. As a result we may lose datagram packets because
                    // we are not calling
                    // {@link MulticastSocket#receive(DatagramPacket)} often
                    // enough.
                    multicastProcessor.put(packet);
                } catch (InterruptedException woken) {
                    Thread.interrupted();
                } catch (InterruptedIOException woken) {
                    Thread.interrupted();
                } catch (Exception e) {
                    if (isClosed) {
                        return;
                    }
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE) && (!isClosed)) {
                        LOG.log(Level.SEVERE, "failure during multicast receive", e);
                    }
                    break;
                }
            }
        } catch (Throwable all) {
            if (isClosed) {
                return;
            }
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            }
        } finally {
            multicastThread = null;
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Synchronized to not allow concurrent IP multicast: this naturally bounds
     * the usage of ip-multicast boolean be linear and not exponential.
     */
    public synchronized boolean propagate(Message message, String pName, String pParams, int initalTTL) {
        long sendStartTime = System.currentTimeMillis();
        int numBytesInPacket = 0;

        try {
            message.replaceMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NS, msgSrcAddrElement);

            // First build the destination and source addresses
            EndpointAddress destAddr = new EndpointAddress(publicAddress, pName, pParams);
            MessageElement dstAddressElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NAME, destAddr.toString(), null);

            message.replaceMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NS, dstAddressElement);

            WireFormatMessage serialed = WireFormatMessageFactory.toWire(message, WireFormatMessageFactory.DEFAULT_WIRE_MIME, null);
            MessagePackageHeader header = new MessagePackageHeader();

            header.setContentTypeHeader(serialed.getMimeType());
            header.setContentLengthHeader(serialed.getByteLength());

            ByteArrayOutputStream buffer = new ByteArrayOutputStream(multicastPacketSize);

            buffer.write('J');
            buffer.write('X');
            buffer.write('T');
            buffer.write('A');
            header.sendToStream(buffer);
            serialed.sendToStream(buffer);
            buffer.flush();
            buffer.close();
            numBytesInPacket = buffer.size();

            if ((buffer.size() > multicastPacketSize) && Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Multicast datagram exceeds multicast size.");
            }

            DatagramPacket packet = new DatagramPacket(buffer.toByteArray(), numBytesInPacket, multicastInetAddress, multicastPort);

            multicastSocket.send(packet);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sent Multicast message to :" + pName + "/" + pParams);
            }

            if (TransportMeterBuildSettings.TRANSPORT_METERING && (multicastTransportBindingMeter != null)) {
                multicastTransportBindingMeter.messageSent(true, message, System.currentTimeMillis() - sendStartTime, numBytesInPacket);
            }
            return true;
        } catch (IOException e) {
            if (TransportMeterBuildSettings.TRANSPORT_METERING && (multicastTransportBindingMeter != null)) {
                multicastTransportBindingMeter.sendFailure(true, message, System.currentTimeMillis() - sendStartTime, numBytesInPacket);
            }

            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Multicast socket send failed", e);
            }
            return false;
        }
    }

    /**
     * Handle a byte buffer from a multi-cast.
     *
     * @param packet the message packet.
     */
    void processMulticast(DatagramPacket packet) {
        int size = packet.getLength();
        byte[] buffer = packet.getData();

        long messageReceiveBeginTime = System.currentTimeMillis();

        try {
            if (size < 4) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("damaged multicast discarded");
                }
                throw new IOException("damaged multicast discarded : too short");
            }

            if (('J' != buffer[0]) || ('X' != buffer[1]) || ('T' != buffer[2]) || ('A' != buffer[3])) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("damaged multicast discarded");
                }
                throw new IOException("damaged multicast discarded : incorrect signature");
            }

            ByteBuffer bbuffer = ByteBuffer.wrap(buffer, 4, size - 4);
            MessagePackageHeader header = new MessagePackageHeader();

            if (!header.readHeader(bbuffer)) {
                throw new IOException("Failed to read framing header");
            }

            MimeMediaType msgMime = header.getContentTypeHeader();
            // TODO 20020730 bondolo@jxta.org Do something with content-coding here.

            // read the message!
            Message msg = WireFormatMessageFactory.fromBuffer(bbuffer, msgMime, null);

            // Extract the source and destination
            MessageElement srcAddrElem = msg.getMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NS, EndpointServiceImpl.MESSAGE_SOURCE_NAME);
            if (null == srcAddrElem) {
                throw new IOException("No Source Address in " + msg);
            }

            msg.removeMessageElement(srcAddrElem);

            EndpointAddress srcAddr = new EndpointAddress(srcAddrElem.toString());

            if (srcAddr.equals(ourSrcAddr)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "Discard loopback multicast message");
                }
                return;
            }

            MessageElement dstAddrElem = msg.getMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NS, EndpointServiceImpl.MESSAGE_DESTINATION_NAME);
            if (null == dstAddrElem) {
                throw new IOException("No Destination Address in " + msg);
            }

            msg.removeMessageElement(dstAddrElem);

            EndpointAddress dstAddr = new EndpointAddress(dstAddrElem.toString());

            // Handoff the message to the EndpointService Manager
            endpoint.processIncomingMessage(msg, srcAddr, dstAddr);

            if (TransportMeterBuildSettings.TRANSPORT_METERING && (multicastTransportBindingMeter != null)) {
                multicastTransportBindingMeter.messageReceived(false, msg, messageReceiveBeginTime - System.currentTimeMillis(), size);
            }
        } catch (Exception e) {
            if (TransportMeterBuildSettings.TRANSPORT_METERING && (multicastTransportBindingMeter != null)) {
                multicastTransportBindingMeter.receiveFailure(false, messageReceiveBeginTime - System.currentTimeMillis(), size);
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Discard incoming multicast message", e);
            }
        }
    }

    TransportBindingMeter getMulticastTransportBindingMeter(EndpointAddress destinationAddress) {
        if (multicastTransportMeter != null) {
            return multicastTransportMeter.getTransportBindingMeter(group.getPeerID(), destinationAddress);
        } else {
            return null;
        }
    }

    /**
     * Handles incoming datagram packets. This implementation uses the peer
     * group Executor service to process the datagram packets, but limits the
     * number of concurrent tasks.
     */
    private class DatagramProcessor implements Runnable {

        /**
         * The maximum number of datagrams we will simultaneously process.
         */
        private static final int MAX_SIMULTANEOUS_PROCESSING = 5;

        /**
         * The executor to which we will issue tasks.
         */
        final Executor executor;

        /**
         * Queue of datagrams waiting to be executed. The queue is quite small.
         * The goal is not to cache datagrams in memory. If we can't keep up it
         * is better that we drop messages.
         */
        final BlockingQueue<DatagramPacket> queue = new ArrayBlockingQueue<DatagramPacket>(MAX_SIMULTANEOUS_PROCESSING + 1);

        /**
         * The number of executor tasks we are currently using.
         */
        int currentTasks = 0;

        /**
         * If {@code true} then this processor has been stopped.
         */
        volatile boolean stopped = false;

        /**
         * Default constructor
         * @param executor the threadpool
         */
        DatagramProcessor(Executor executor) {
            this.executor = executor;
        }

        /**
         * Stops this thread
         */
        void stop() {
            queue.clear();
            stopped = true;
        }

        /**
         * Puts a datagram on the queue. The enqueue operation is blocking and
         * may take a significant amount of time.
         *
         * @param packet the datagram
         * @throws InterruptedException if interrupted
         */
        void put(DatagramPacket packet) throws InterruptedException {
            boolean execute = false;

            if (stopped) {
                return;
            }

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.log(Level.FINER, "Queuing incoming datagram packet : " + packet);
            }

            // push the datagram
            queue.put(packet);

            // See if we can start a new executor.
            synchronized (this) {
                if (!stopped && (currentTasks < MAX_SIMULTANEOUS_PROCESSING)) {
                    currentTasks++;
                    execute = true;
                }
            }

            // If it's ok, start a new executor outside of the synchronization.
            if (execute) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINER, "Starting new executor datagram processing task");
                }
                executor.execute(this);
            }
        }

        /**
         * {@inheritDoc}
         */
        public void run() {
            try {
                DatagramPacket packet;
                while (!stopped && (null != (packet = queue.poll()))) {
                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.log(Level.FINER, "Processing incoming datagram packet : " + packet);
                    }
                    processMulticast(packet);
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable", all);
                }
            } finally {
                synchronized (this) {
                    currentTasks--;
                }
            }
        }
    }
}
