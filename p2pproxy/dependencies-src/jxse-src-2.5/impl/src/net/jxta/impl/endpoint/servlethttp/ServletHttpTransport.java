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
package net.jxta.impl.endpoint.servlethttp;

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;

import java.net.UnknownHostException;
import java.util.concurrent.Executor;

import java.util.logging.Logger;
import java.util.logging.Level;

import net.jxta.logging.Logging;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attribute;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.id.ID;
import net.jxta.meter.MonitorResources;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.TransportAdvertisement;
import net.jxta.platform.Module;

import net.jxta.exception.PeerGroupException;

import net.jxta.impl.endpoint.transportMeter.TransportBindingMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeterBuildSettings;
import net.jxta.impl.endpoint.transportMeter.TransportServiceMonitor;
import net.jxta.impl.endpoint.IPUtils;
import net.jxta.impl.protocol.HTTPAdv;

import net.jxta.impl.meter.*;
import net.jxta.impl.peergroup.StdPeerGroup;

/**
 * A JXTA Message Transport
 *
 * <p/>This class is really a facade for the following:<ul>
 * <li>An HTTP client message sender</li>
 * <li>An HTTP-server-based message receiver</li>
 * </ul>
 */
public final class ServletHttpTransport implements Module {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(ServletHttpTransport.class.getName());

    /**
     * The name of the protocol
     */
    private final static String DEFAULT_HTTP_PROTOCOL_NAME = "http";

    String HTTP_PROTOCOL_NAME = DEFAULT_HTTP_PROTOCOL_NAME;

    /**
     * PeerGroup we are working for
     */
    PeerGroup group;
    ID assignedID;
    ModuleImplAdvertisement implAdvertisement;
    
    /**
     * The executor used by HttpClientMessenger
     */
    Executor executor;

    /**
     * The endpoint we attach to.
     */
    private EndpointService endpoint = null;

    /**
     * If {@code true} then we are configured to act as an HTTP client. This
     * means we will poll for messages.
     */
    private boolean configClient = false;

    /**
     * If {@code true} then we are configured to act as an HTTP server. This
     * means we will accept connections from polling clients.
     */
    private boolean configServer = false;

    /**
     * The HttpMessageSender instance
     */
    private HttpMessageSender sender = null;

    /**
     * The HttpMessageReceiver instance
     */
    private HttpMessageReceiver receiver = null;

    /**
     * The TransportMeter for this httpTransport
     */
    private TransportMeter transportMeter;

    /**
     * The TransportBindingMeter for unknown connections (pings/errors)
     */
    private TransportBindingMeter unknownTransportBindingMeter;

    /**
     * The InetAddress of the network interface we are bound to otherwise the
     * ALL/ANY address.
     */
    InetAddress usingInterface = null;

    /**
     * Port number to which we are bound.
     */
    int usingPort = 0;

    /**
     * The addresses which we will advertise and by which we may be reached.
     */
    private List<EndpointAddress> publicAddresses = null;

    /**
     * Our preferred return address which we will use as the "source" for
     * messages we send.
     */
    private EndpointAddress publicAddress;

    /**
     * {@inheritDoc}
     */
    public synchronized void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {
        this.group = group;
        this.assignedID = assignedID;
        implAdvertisement = (ModuleImplAdvertisement) impl;
        
        this.executor = ((StdPeerGroup) group).getExecutor();

        // Get out invariable parameters from the implAdv
        XMLElement param = (XMLElement) implAdvertisement.getParam();

        if (param != null) {
            Enumeration list = param.getChildren("Proto");

            if (list.hasMoreElements()) {
                XMLElement pname = (XMLElement) list.nextElement();
                String configProtoName = pname.getTextValue();
                if (null != configProtoName) {
                    HTTP_PROTOCOL_NAME = configProtoName.trim();
                }
            }
        }

        ConfigParams peerAdv = group.getConfigAdvertisement();

        param = (XMLElement) peerAdv.getServiceParam(assignedID);

        Enumeration httpChilds = param.getChildren(TransportAdvertisement.getAdvertisementType());

        // get the TransportAdv
        if (httpChilds.hasMoreElements()) {
            param = (XMLElement) httpChilds.nextElement();
            Attribute typeAttr = param.getAttribute("type");

            if (!HTTPAdv.getAdvertisementType().equals(typeAttr.getValue())) {
                throw new IllegalArgumentException("Transport adv is not an http adv");
            }

            if (httpChilds.hasMoreElements()) {
                throw new IllegalArgumentException("Configuration contained multiple http advertisements");
            }
        } else {
            throw new IllegalArgumentException("Configuration did not contain http advertisement");
        }

        Advertisement paramsAdv = AdvertisementFactory.newAdvertisement(param);

        if (!(paramsAdv instanceof HTTPAdv)) {
            throw new IllegalArgumentException("Provided advertisement was not a " + HTTPAdv.getAdvertisementType());
        }

        HTTPAdv httpAdv = (HTTPAdv) paramsAdv;

        // determine the local interface to use. If the user specifies
        // one, use that. Otherwise, use the all the available interfaces.
        String interfaceAddressStr = httpAdv.getInterfaceAddress();
        boolean publicAddressOnly = httpAdv.getPublicAddressOnly();

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

        usingPort = httpAdv.getPort();

        configClient = httpAdv.isClientEnabled();

        configServer = httpAdv.isServerEnabled();

        publicAddresses = getPublicAddresses(configServer, httpAdv.getServer(), usingInterface, usingPort, publicAddressOnly);

        if (!configClient && !configServer) {
            throw new IllegalArgumentException("Neither incoming nor outgoing connections configured.");
        }

        publicAddress = publicAddresses.get(0);

        // Tell tell the world about our configuration.
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring HTTP Message Transport : " + assignedID);

            if (implAdvertisement != null) {
                configInfo.append("\n\tImplementation:");
                configInfo.append("\n\t\tModule Spec ID: ").append(implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description: ").append(implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI: ").append(implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code: ").append(implAdvertisement.getCode());
            }
            configInfo.append("\n\tGroup Params:");
            configInfo.append("\n\t\tGroup: ").append(group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID: ").append(group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID: ").append(group.getPeerID());
            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tProtocol: ").append(httpAdv.getProtocol());
            configInfo.append("\n\t\tClient Enabled: ").append(configClient);
            configInfo.append("\n\t\tServer Enabled: ").append(configServer);
            configInfo.append("\n\t\tPublic address: ").append(httpAdv.getServer() == null ? "(unspecified)" : httpAdv.getServer());
            configInfo.append("\n\t\tInterface address: ").append(interfaceAddressStr == null ? "(unspecified)" : interfaceAddressStr);
            configInfo.append("\n\t\tUnicast Server Bind Addr: ").append(IPUtils.getHostAddress(usingInterface)).append(":").append(usingPort);
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
    public synchronized int startApp(String[] args) {
        endpoint = group.getEndpointService();

        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }

            return START_AGAIN_STALLED;
        }

        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            TransportServiceMonitor transportServiceMonitor = (TransportServiceMonitor) MonitorManager.getServiceMonitor(group,
                    MonitorResources.transportServiceMonitorClassID);

            if (transportServiceMonitor != null) {
                transportMeter = transportServiceMonitor.createTransportMeter("HTTP", publicAddress);
                unknownTransportBindingMeter = transportMeter.getTransportBindingMeter(TransportMeter.UNKNOWN_PEER,
                        TransportMeter.UNKNOWN_ADDRESS);
            }
        }

        if (configServer) {
            // Start the http server that runs the receiver.

            try {
                receiver = new HttpMessageReceiver(this, publicAddresses, usingInterface, usingPort);
                receiver.start();
            } catch (PeerGroupException e) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Could not start http message receiver", e);
                }
                return -1; // Can't go on; if we were configured to be a server we must make the failure obvious.
            }
        }

        if (configClient) {
            // create the MessageSender

            try {
                sender = new HttpMessageSender(this,
                        new EndpointAddress("jxta", group.getPeerID().getUniqueValue().toString(), null, null));
                sender.start();
            } catch (PeerGroupException e) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Could not start http message sender", e);
                }
                return -1; // Can't go on; if we were configured to be a server we must make the failure obvious.
            }
        }

        return 0;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void stopApp() {
        if (receiver != null) {
            receiver.stop();
        }

        if (sender != null) {
            sender.stop();
        }

        endpoint = null;
    }

    /**
     * {@inheritDoc}
     */
    EndpointService getEndpointService() {
        return endpoint;
    }

    private List<EndpointAddress> getPublicAddresses(boolean serverEnabled, String serverName, InetAddress usingInterface, int serverSocketPort, boolean publicAddressOnly) {
        List<EndpointAddress> publicAddresses = new ArrayList<EndpointAddress>();

        if (serverEnabled) {
            // Build the publicAddresses

            // first in the list is the "public server name". We don't try to
            // resolve this since it might not be resolvable in the context
            // we are running in, we just assume it's good.
            if (serverName != null) {
                // use speced server name.
                EndpointAddress newAddr = new EndpointAddress(HTTP_PROTOCOL_NAME, serverName, null, null);

                publicAddresses.add(newAddr);
                if (publicAddressOnly) {
                    return publicAddresses;
                }
            }
        }

        // then add the rest of the local interfaces as appropriate
        if (usingInterface.equals(IPUtils.ANYADDRESS)) {
            // its wildcarded
            Iterator<InetAddress> eachLocal = IPUtils.getAllLocalAddresses();
            List<EndpointAddress> wildAddrs = new ArrayList<EndpointAddress>();

            while (eachLocal.hasNext()) {
                InetAddress anAddress = eachLocal.next();

                String hostAddress = IPUtils.getHostAddress(anAddress);

                EndpointAddress newAddr = new EndpointAddress(HTTP_PROTOCOL_NAME,
                        hostAddress + ":" + Integer.toString(serverSocketPort), null, null);

                // don't add it if its already in the list
                if (!publicAddresses.contains(newAddr)) {
                    wildAddrs.add(newAddr);
                }
            }

            // we sort them so that later equals() will be deterministic.
            // the result of IPUtils.getAllLocalAddresses() is not known
            // to be sorted.
            Collections.sort(wildAddrs, new Comparator<EndpointAddress>() {
                public int compare(EndpointAddress one, EndpointAddress two) {
                    return one.toString().compareTo(two.toString());
                }

                @Override
                public boolean equals(Object that) {
                    return this == that;
                }
            });

            publicAddresses.addAll(wildAddrs);
        } else {
            // use specified interface
            String hostAddress = IPUtils.getHostAddress(usingInterface);

            EndpointAddress newAddr = new EndpointAddress(HTTP_PROTOCOL_NAME,
                    hostAddress + ":" + Integer.toString(serverSocketPort), null, null);

            // don't add it if its already in the list
            if (!publicAddresses.contains(newAddr)) {
                publicAddresses.add(newAddr);
            }
        }

        return publicAddresses;
    }

    TransportBindingMeter getTransportBindingMeter(String peerIDString, EndpointAddress destinationAddress) {
        if (transportMeter != null) {
            return transportMeter.getTransportBindingMeter((peerIDString != null) ? peerIDString : TransportMeter.UNKNOWN_PEER,
                    destinationAddress);
        } else {
            return null;
        }
    }

    TransportBindingMeter getUnknownTransportBindingMeter() {
        return unknownTransportBindingMeter;
    }
}
