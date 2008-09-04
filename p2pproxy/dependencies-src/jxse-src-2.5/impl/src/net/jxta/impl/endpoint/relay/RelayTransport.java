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
package net.jxta.impl.endpoint.relay;

import java.util.NoSuchElementException;

import java.util.logging.Level;

import net.jxta.logging.Logging;

import java.util.logging.Logger;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.id.ID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.platform.Module;

import net.jxta.exception.PeerGroupException;

import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.protocol.RelayConfigAdv;
import net.jxta.pipe.PipeService;

/**
 * The Relay Server supports the following commands:
 *
 * CONNECT - message contains PEERID, optional LEASE
 * DISCONNECT - message contains PEERID.
 * GETSERVER - message contains PEERID.
 */

public final class RelayTransport implements EndpointListener, Module {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(RelayTransport.class.getName());

    // // constants ////
    static final String protocolName = "relay";

    static final String RELAY_NS = "relay";
    static final String REQUEST_ELEMENT = "request";
    static final String RESPONSE_ELEMENT = "response";
    static final String PEERID_ELEMENT = "peerid";
    static final String LEASE_ELEMENT = "lease";
    static final String RELAY_ADV_ELEMENT = "relayAdv";

    static final String CONNECT_REQUEST = "connect";
    static final MessageElement CONNECT_REQUEST_ELEMENT = new StringMessageElement(REQUEST_ELEMENT, CONNECT_REQUEST, null);
    static final String DISCONNECT_REQUEST = "disconnect";
    static final MessageElement DISCONNECT_REQUEST_ELEMENT = new StringMessageElement(REQUEST_ELEMENT, DISCONNECT_REQUEST, null);
    static final String PID_REQUEST = "pid";
    static final MessageElement PID_REQUEST_ELEMENT = new StringMessageElement(REQUEST_ELEMENT, PID_REQUEST, null);

    static final String CONNECTED_RESPONSE = "connected";
    static final MessageElement CONNECTED_RESPONSE_ELEMENT = new StringMessageElement(RESPONSE_ELEMENT, CONNECTED_RESPONSE, null);
    static final String DISCONNECTED_RESPONSE = "disconnected";
    static final MessageElement DISCONNECTED_RESPONSE_ELEMENT = new StringMessageElement(RESPONSE_ELEMENT, DISCONNECTED_RESPONSE
            ,
            null);
    static final String PID_RESPONSE = "pid";
    static final MessageElement PID_RESPONSE_ELEMENT = new StringMessageElement(RESPONSE_ELEMENT, PID_RESPONSE, null);

    static final int DEFAULT_MAX_CLIENTS = 150;

    static final int DEFAULT_MAX_SERVERS = 1;

    // Note the weird time below can be decreased but should not be increased
    // otherwise there will not be enough traffic for the other side to
    // keep the connection open.
    static final long DEFAULT_LEASE = TimeUtils.ANHOUR;
    static final long DEFAULT_STALL_TIMEOUT = 15 * TimeUtils.ASECOND;

    static final long DEFAULT_POLL_INTERVAL = 15 * TimeUtils.ASECOND; // (the poll costs very little)

    static final long DEFAULT_BROADCAST_INTERVAL = 10 * TimeUtils.AMINUTE;

    static final int DEFAULT_CLIENT_QUEUE_SIZE = 20;

    private PeerGroup group = null;

    private String serviceName = null;

    private RelayClient relayClient = null;
    private RelayServer relayServer = null;

    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement implAdv) throws PeerGroupException {
        this.group = group;
        ModuleImplAdvertisement implAdvertisement = (ModuleImplAdvertisement) implAdv;

        this.serviceName = assignedID.getUniqueValue().toString();

        ConfigParams confAdv = group.getConfigAdvertisement();

        // Get the config. If we do not have a config, we're done; we just keep
        // the defaults (edge peer/no auto-rdv)
        RelayConfigAdv relayConfigAdv = null;

        if (confAdv != null) {
            Advertisement adv = null;

            try {
                XMLDocument configDoc = (XMLDocument) confAdv.getServiceParam(assignedID);

                if (null != configDoc) {
                    adv = AdvertisementFactory.newAdvertisement(configDoc);
                }
            } catch (NoSuchElementException failed) {
                //ignored
            } catch (IllegalArgumentException failed) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Error in relay advertisement", failed);
                }

                throw failed;
            }

            if (adv instanceof RelayConfigAdv) {
                relayConfigAdv = (RelayConfigAdv) adv;
            } else {
                relayConfigAdv = (RelayConfigAdv) AdvertisementFactory.newAdvertisement(RelayConfigAdv.getAdvertisementType());
            }
        }

        // XXX bondolo 20041030 I'd like to move these to startApp so that we 
        // can pass endpointService and share the instance.
        if (relayConfigAdv.isServerEnabled()) {
            relayServer = new RelayServer(group, serviceName, relayConfigAdv);
        }

        if (relayConfigAdv.isClientEnabled()) {
            relayClient = new RelayClient(group, serviceName, relayConfigAdv);
        }

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Relay Message Transport : " + assignedID);

            if (implAdvertisement != null) {
                configInfo.append("\n\tImplementation :");
                configInfo.append("\n\t\tModule Spec ID: ").append(implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description : ").append(implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI : ").append(implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code : ").append(implAdvertisement.getCode());
            }

            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tGroup : ").append(group);
            configInfo.append("\n\t\tPeer ID : ").append(group.getPeerID());

            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tService Name : ").append(serviceName);
            configInfo.append("\n\t\tisServer : ").append(relayConfigAdv.isServerEnabled());
            configInfo.append("\n\t\tisClient : ").append(relayConfigAdv.isClientEnabled());

            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public int startApp(String[] args) {
        EndpointService endpoint = group.getEndpointService();

        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }

            return START_AGAIN_STALLED;
        }

        // XXX bondolo 20041025 Server depends upon discovery and its non-optional.
        DiscoveryService discovery = group.getDiscoveryService();

        if (null == discovery) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a discovery service");
            }

            return START_AGAIN_STALLED;
        }

        // XXX bondolo 20041025 Server depends upon pipes and its non-optional.
        PipeService pipeService = group.getPipeService();

        if (null == pipeService) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a pipe service");
            }

            return START_AGAIN_STALLED;
        }

        endpoint.addIncomingMessageListener(this, serviceName, null);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Message Listener added " + serviceName);
        }

        if (relayServer != null) {
            if (!relayServer.startServer()) {
                return -1; // cannot start
            }
        }

        if (relayClient != null) {
            if (!relayClient.startClient()) {
                return -1; // cannot start
            }
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Relay Message Transport started");
        }

        return 0;
    }

    /**
     * {@inheritDoc}
     */
    public void stopApp() {
        // remove listener
        EndpointService endpoint = group.getEndpointService();

        if (endpoint == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("could not get EndpointService");
            }
        } else {
            endpoint.removeIncomingMessageListener(serviceName, null);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Message Listener removed " + serviceName);
            }
        }

        if (relayServer != null) {
            relayServer.stopServer();
        }

        if (relayClient != null) {
            relayClient.stopClient();
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Relay Message Transport stopped");
        }
    }

    /**
     * {@inheritDoc}
     */
    public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Started for " + message + "\tsrc=" + srcAddr);
        }

        MessageElement element;

        // check if it is a request
        element = message.getMessageElement(RelayTransport.RELAY_NS, REQUEST_ELEMENT);

        if (element != null) {
            // this is a request, pass it to the relayServer
            if (relayServer != null) {
                relayServer.handleRequest(message, dstAddr);
            }
        } else {
            // check if it is a response
            element = message.getMessageElement(RelayTransport.RELAY_NS, RESPONSE_ELEMENT);

            if (element != null) {
                // this is a response, pass it to the relayClient
                if (relayClient != null) {
                    relayClient.handleResponse(message, dstAddr);
                }
            }
        }
    }

    protected PeerGroup getGroup() {
        return group;
    }

    protected String getServiceName() {
        return serviceName;
    }

    // This is for reference only, the Desktop client implementation does not
    // use that server feature
    static Message createPIDRequestMessage() {
        Message message = new Message();
        message.addMessageElement(RELAY_NS, PID_REQUEST_ELEMENT);
        return message;
    }

    static Message createPIDResponseMessage(String pidStr) {
        Message message = new Message();
        message.addMessageElement(RELAY_NS, PID_RESPONSE_ELEMENT);
        setString(message, PEERID_ELEMENT, pidStr);

        return message;
    }

    static Message createConnectMessage(long lease, boolean doReturnAdv, boolean doFlushQueue) {
        Message message = new Message();
        String request = createConnectString(lease, doReturnAdv, doFlushQueue);
        setString(message, REQUEST_ELEMENT, request);
        return message;
    }

    static String createConnectString(long lease, boolean doReturnAdv, boolean doFlushQueue) {

        String request = CONNECT_REQUEST;

        if (lease > 0) {
            request += "," + Long.toString(lease);
        } else {
            request += ",";
        }

        if (doFlushQueue) {
            request += ",flush";
        } else {
            request += ",keep";
        }

        if (doReturnAdv) {
            request += ",true";
        } else {
            request += ",other";
        }

        return request;
    }

    static Message createConnectedMessage(long lease) {
        Message message = new Message();

        message.addMessageElement(RELAY_NS, CONNECTED_RESPONSE_ELEMENT);

        if (lease > 0) {
            setString(message, LEASE_ELEMENT, Long.toString(lease));
        }

        return message;
    }

    static Message createDisconnectMessage() {
        Message message = new Message();
        message.addMessageElement(RELAY_NS, DISCONNECT_REQUEST_ELEMENT);
        return message;
    }

    static Message createDisconnectedMessage() {
        Message message = new Message();
        message.addMessageElement(RELAY_NS, DISCONNECTED_RESPONSE_ELEMENT);
        return message;
    }

    /**
     * Convinence function for setting a string element with the relay namespace
     *
     * @param message the message
     * @param elementName message element name
     * @param value the value of the string
     */
    static void setString(Message message, String elementName, String value) {
        // create a new String Element with the given value
        StringMessageElement sme = new StringMessageElement(elementName, value, null);

        // add the new element to the message
        message.addMessageElement(RELAY_NS, sme);
    }

    /**
     * Convinence function for getting a String from the element with the given
     * tag and relay namespace
     *
     * @param message the message
     * @param elementName the value of the string
     * @return the string value, null if the element does not exist
     */
    static String getString(Message message, String elementName) {
        // get the requested element
        MessageElement element = message.getMessageElement(RelayTransport.RELAY_NS, elementName);

        // if the element is not present, return null
        if (element == null) {
            return null;
        }
        return element.toString();
    }
}
