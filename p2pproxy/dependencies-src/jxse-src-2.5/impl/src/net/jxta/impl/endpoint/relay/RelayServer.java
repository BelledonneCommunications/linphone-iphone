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

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Random;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.MessageSender;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.MessengerEvent;
import net.jxta.endpoint.MessengerEventListener;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.access.AccessList;
import net.jxta.impl.protocol.RelayConfigAdv;
import net.jxta.impl.util.TimeUtils;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.protocol.RdvAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;
import net.jxta.impl.endpoint.EndpointUtils;

/**
 * Relay server that maintains outgoing message queues, leases, etc.
 */
public class RelayServer implements MessageSender, MessengerEventListener, Runnable {
    
    /**
     *  Logger
     */
    private final static transient Logger LOG = Logger.getLogger(RelayServer.class.getName());
    
    private final static int MAX_CACHED_SERVERS = 20;
    
    /**
     * The EndpointService for the RelayService
     */
    private EndpointService endpointService;
    
    /**
     * The DiscoveryService for the RelayService
     */
    private DiscoveryService discoveryService;
    
    /**
     * The public address is of the form relay://peerId
     */
    private final EndpointAddress publicAddress;
    
    /**
     *  Map of the current clients
     */
    private final Map<String, RelayServerClient> relayedClients = new HashMap<String, RelayServerClient>();
    
    protected final PeerGroup group;
    protected final String serviceName;
    private final int maxClients;
    private final long maxLeaseDuration;
    private final long stallTimeout;
    private final int clientQueueSize;
    private final long minBroadcastInterval;
    
    protected final String peerId;

    protected final AccessList acl;
    protected File aclFile;
    protected long refreshTime = 0;

    protected long aclFileLastModified = 0;
    private static final long ACL_REFRESH_PERIOD = 30 * TimeUtils.AMINUTE;
    
    protected RelayServerCache relayServerCache;
    
    private Thread gcThread = null;

    private MessengerEventListener messengerEventListener = null;
    
    /**
     * constructor
     */
    public RelayServer(PeerGroup group, String serviceName, RelayConfigAdv relayConfigAdv) {
        
        this.group = group;
        peerId = group.getPeerID().getUniqueValue().toString();
        publicAddress = new EndpointAddress(RelayTransport.protocolName, peerId, null, null);
        
        this.serviceName = serviceName;
        
        this.maxClients = (-1 != relayConfigAdv.getMaxClients())
                ? relayConfigAdv.getMaxClients()
                : RelayTransport.DEFAULT_MAX_CLIENTS;
        this.clientQueueSize = (-1 != relayConfigAdv.getClientMessageQueueSize())
                ? relayConfigAdv.getClientMessageQueueSize()
                : RelayTransport.DEFAULT_CLIENT_QUEUE_SIZE;
        this.maxLeaseDuration = (-1 != relayConfigAdv.getServerLeaseDuration())
                ? relayConfigAdv.getServerLeaseDuration()
                : RelayTransport.DEFAULT_LEASE;
        this.minBroadcastInterval = (-1 != relayConfigAdv.getAnnounceInterval())
                ? relayConfigAdv.getAnnounceInterval()
                : RelayTransport.DEFAULT_BROADCAST_INTERVAL;
        this.stallTimeout = (-1 != relayConfigAdv.getStallTimeout())
                ? relayConfigAdv.getStallTimeout()
                : RelayTransport.DEFAULT_STALL_TIMEOUT;

        aclFile = new File(new File(group.getStoreHome()), "relayACL.xml");
        aclFileLastModified = aclFile.lastModified();
        this.acl = new AccessList();
        try {
            acl.init(aclFile);
            this.refreshTime = System.currentTimeMillis() + ACL_REFRESH_PERIOD;
        } catch (IOException io) {
            acl.setGrantAll(true);
            this.refreshTime = Long.MAX_VALUE;
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("RelayServer Access Control granting all permissions");
            }
        }
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Relay Server");

            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tGroup : ").append(group);
            configInfo.append("\n\t\tPeer ID : ").append(group.getPeerID());

            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tService Name : ").append(serviceName);
            configInfo.append("\n\t\tMax Relay Clients : ").append(maxClients);
            configInfo.append("\n\t\tMax Lease Length : ").append(maxLeaseDuration).append("ms.");
            configInfo.append("\n\t\tBroadcast Interval : ").append(minBroadcastInterval).append("ms.");
            configInfo.append("\n\t\tStall Timeout : ").append(stallTimeout).append("ms.");
            
            LOG.config(configInfo.toString());
        }
    }
    
    /**
     * Debug routine: returns the list of relayedClients with details.
     */
    public List<String> getRelayedClients() {
        List<String> res = new ArrayList<String>();

        for (Object o : Arrays.asList(relayedClients.values().toArray())) {
            String client = o.toString();

            res.add(client);
        }

        return res;
    }
    
    public boolean startServer() {
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Starting " + publicAddress.toString());
        }
        
        endpointService = group.getEndpointService();
        discoveryService = group.getDiscoveryService();
        
        if ((messengerEventListener = endpointService.addMessageTransport(this)) == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Transport registration refused");
            }
            return false;
        }
        
        try {
            discoveryService.publish(createRdvAdvertisement(group.getPeerAdvertisement(), serviceName));
        } catch (IOException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not publish Relay RdvAdvertisement", e);
            }
        }
        
        relayServerCache = new RelayServerCache(this);
        
        // start cache relay servers
        relayServerCache.startCache();
        
        endpointService.addMessengerEventListener(this, EndpointService.HighPrecedence);
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Relay Server started");
        }
        return true;
    }

    public void stopServer() {
        // stop cache relay servers
        relayServerCache.stopCache();
        relayServerCache = null;
        
        // remove messenger events listener since we do not have any clients
        endpointService.removeMessengerEventListener(this, EndpointService.HighPrecedence);
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Messenger Event Listener removed " + serviceName);
        }
        
        // Close all clients.
        // Get a list of the clients but leave them in the map;
        // they remove themselves by calling removeClient(this).
        // That's why we do not iterate through the real map to close them.
        
        RelayServerClient[] oldClients;
        
        synchronized (relayedClients) {
            oldClients = relayedClients.values().toArray(new RelayServerClient[0]);
        }
        
        int i = oldClients.length;
        
        while (i-- > 0) {
            oldClients[i].closeClient();
        }
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Stopped " + publicAddress);
        }        
    }
    
    /*
     * Methods inherited from MessageSender
     */
    
    /**
     * {@inheritDoc}
     */
    public EndpointAddress getPublicAddress() {
        return publicAddress;
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
    
    /**
     * {@inheritDoc}
     */
    public Object transportControl(Object operation, Object Value) {
        return null;
    }
    
    /**
     * {@inheritDoc}
     */
    public Messenger getMessenger(EndpointAddress destAddr, Object hintIgnored) {
        Messenger messenger = null;
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("getMessenger for dest " + destAddr);
        }
        
        if (!RelayTransport.protocolName.equals(destAddr.getProtocolName())) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("could not make messenger for protocol :" + destAddr.getProtocolName());
            }
            
            return null;
        }
        
        // check if we have a queue for this client
        RelayServerClient handler = getClient(destAddr.getProtocolAddress());
        
        if (handler != null) {
            messenger = handler.getMessenger(publicAddress, destAddr, false);
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("messenger for " + destAddr.getProtocolAddress() + " is " + messenger);
        }
        
        return messenger;
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    public boolean ping(EndpointAddress addr) {
        
        synchronized (relayedClients) {
            return (null != relayedClients.get(addr.getProtocolAddress()));
        }
    }
    
    /*
     * Methods inherited from MessageTransport
     */
    
    /**
     * {@inheritDoc}
     */
    public String getProtocolName() {
        return RelayTransport.protocolName;
    }
    
    /**
     * {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        return endpointService;
    }
    
    /**
     * {@inheritDoc}
     */
    public boolean messengerReady(MessengerEvent event) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("messengerReady");
        }
        
        Messenger newMessenger = event.getMessenger();
        Object source = event.getSource();
        EndpointAddress connectionAddress = event.getConnectionAddress();
        
        // Sanity check, this should not happen
        if (newMessenger == null || source == null || connectionAddress == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("there was not a messenger or not enough information");
            }
            
            return false;
        }
        
        // We do not grab just any messenger; that would replace the
        // existing one and then we could have a fight between the
        // front channel and the back channel from the same peer.
        // We only grab back-channel messengers that where explicitly
        // directed to the relay.
        if (!serviceName.equals(connectionAddress.getServiceName())) {
            return false;
        }
        
        // make sure that it is not a higher level messenger
        if (source instanceof MessageSender && !((MessageSender) source).allowsRouting()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("this is a higher level messenger");
            }
            
            return false;
        }
        
        // make sure that this is not one of our own.
        if (source == this || newMessenger instanceof RelayServerClient.RelayMessenger) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("this is a relay messenger");
            }
            
            return false;
        }
        
        // make sure that the messenger matches a possible client address
        EndpointAddress destAddr = newMessenger.getLogicalDestinationAddress();
        
        if (destAddr == null || !"jxta".equals(destAddr.getProtocolName())) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("LogicalDestinationAddress is not a \"jxta\" protocol");
            }
            
            return false;
        }
        
        // check if we have a queue for this client
        // In that case, we just give it the handler and be done.
        // We must not process the lease request that comes with a messenger
        // for an existing client. If we did, we would reply with a lease
        // response. Some connections can carry only one message and then
        // close. In that case, the client has to re-establish the connection
        // every time we respond. So, if we repond to all incoming connections
        // we're going nowhere. In some cases, the client realy wants a
        // response because it believes it is an initial connection while
        // we still have it from a previous session. In that case, the client
        // must try to send an explicit lease renewal message. (To which
        // we do respond).
        
        String clientPeerId = destAddr.getProtocolAddress();
        RelayServerClient handler = getClient(clientPeerId);
        
        if (handler != null) {
            return handler.addMessenger(newMessenger);
        }
        
        // Non-existent client. We want to process the
        // connection request and respond.
        // handleRequest may do whatever, but we always keep the
        // messenger. It was meant for us anyway.
        handleRequest(newMessenger, connectionAddress);
        return true;
    }

    protected void handleRequest(Messenger messenger, EndpointAddress connectionAddress) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("handleRequest from messenger");
        }
        
        // In this case, the request comes within the messenger's destination.
        String request = connectionAddress.getServiceParameter();
        
        // make sure that the messenger shows a client logical address
        EndpointAddress clientAddr = messenger.getLogicalDestinationAddress();
        
        if (clientAddr == null || !"jxta".equals(clientAddr.getProtocolName())) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("LogicalDestinationAddress is not a \"jxta\" protocol");
            }
            
            return;
        }
        
        String clientPeerId = clientAddr.getProtocolAddress();
        
        handleRequest(request, clientPeerId, messenger);
    }
    
    protected void handleRequest(Message message, EndpointAddress dstAddr) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("handleRequest from message");
        }
        
        String request = RelayTransport.getString(message, RelayTransport.REQUEST_ELEMENT);
        String clientPeerId = dstAddr.getServiceParameter();
        
        handleRequest(request, clientPeerId, null);
    }
    
    void handleRequest(String request, String clientPeerId, Messenger messenger) {
        // This request may come along with a messenger (if it is a renewal
        // post-disconnection or an initial lease request).
        
        if (request == null) {
            return;
        }
        
        request = request.toLowerCase();
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("request = " + request);
        }
        
        // only process the request if a client peer id was sent
        if (clientPeerId == null) {
            return;
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("clientPeerId = " + clientPeerId);
        }
        
        // The only valid anonymous request is a request to obtain a real pid.
        if ((clientPeerId.equals("unknown-unknown")) && (!request.startsWith(RelayTransport.PID_REQUEST))) {	
            return;
        }
        
        Message responseMessage = null;
        
        RelayServerClient closingHandler = null;
        boolean rawMessenger = false;
        boolean closeMessenger = false;
        
        // Figure out which request it is
        if (request.startsWith(RelayTransport.CONNECT_REQUEST)) {
            // Connect Request
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("connect clientPeerId = " + clientPeerId);
            }
            
            long requestedLease = maxLeaseDuration;
            boolean returnRelayAdv = false;
            boolean returnOtherRelayAdv = false;
            boolean flushQueue = false;
            
            String requestedLeaseString = null;
            
            // check if a lease value was specified
            int startIdx = request.indexOf(',');
            
            if (startIdx != -1) {
                // find the end of the lease value
                int endIdx = request.indexOf(',', startIdx + 1);
                
                if (endIdx == -1) {
                    requestedLeaseString = request.substring(startIdx + 1);
                } else {
                    requestedLeaseString = request.substring(startIdx + 1, endIdx);
                    String flags = request.substring(endIdx + 1);
                    
                    if (flags.endsWith("true")) {
                        returnRelayAdv = true;
                    } else if (flags.endsWith("other")) {
                        // This is an addition to the protocol. Newer
                        // clients will always set that in connection requests
                        // when not setting true. Only older clients use to
                        // set nothing at all.
                        returnOtherRelayAdv = true;
                    }
                    // Only two flag positions for now
                    // The inserted first position is another extention.
                    // Only newer clients use it. Older servers will not
                    // notice it because they only check how the request ends.
                    // So, new clients are also compatible with old servers.
                    if (flags.startsWith("flush")) {
                        flushQueue = true;
                    }
                }
            }
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        "request lease string = " + requestedLeaseString + "\treturn relay adv = " + returnRelayAdv
                        + "\n\treturn other relay adv = " + returnOtherRelayAdv + "\tflush queue = " + flushQueue);
            }
            
            if (requestedLeaseString != null) {
                try {
                    requestedLease = Long.parseLong(requestedLeaseString);
                } catch (NumberFormatException e) {
                    if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                        LOG.info("could not parse requested lease string");
                    }
                }
                
                if (requestedLease > maxLeaseDuration) {
                    requestedLease = maxLeaseDuration;
                }
            }
            
            // process the connect request
            EndpointAddress clientAddr = new EndpointAddress("jxta", clientPeerId, serviceName, peerId);
            
            // If we have a messenger, the clientHandler gets it.
            // If the client handler did not already exist, it will be
            // created only if we pass a messenger. We can no-longer create
            // new clients without an incoming messenger. We used to get one
            // from the router but no-longer. Now initial lease requests must
            // come as part of the messenger creation.
            
            RelayServerClient handler = addClient(clientPeerId, requestedLease, messenger, flushQueue);
            
            if (handler != null) {
                
                // the client was added, send a connected response
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("added client " + clientPeerId);
                }
                
                // Now get a messenger that goes through the handler and
                // sends messages out-of-band (and internal perk).
                // jice@jxta.org - 20021227 all this code is getting ridiculous
                // it has to be re-organized. Addind the outOfBand feature
                // to all RelayMessengers just for that is overkill. This
                // just a temporary patch. The real fix would be to respond
                // straight with the messenger we have. Unfortunately, sometimes
                // we have to respond without a messenger in our hands because
                // sending a message over an explicit connection is the only
                // way for existing clients to ask for a response when they
                // reconnect. We would need to change the protocol and add an
                // "initial connection" request type to fix that.
                
                messenger = handler.getMessenger(publicAddress, clientAddr, true);
                responseMessage = RelayTransport.createConnectedMessage(handler.getLeaseRemaining());
                // For protocol compatibility reasons, returnRelayAdv realy
                // means "return your own because I do not know it".
                // If returnOtherRelayAdv is true, then, we will return one
                // selected among those we know, for the enlightenment of the
                // other party.
                // If neither is true, we'll return no adv at all in order not
                // to confuse existing clients.
                
                RdvAdvertisement relayAdv = null;
                
                if (returnRelayAdv) {
                    relayAdv = createRdvAdvertisement(group.getPeerAdvertisement(), serviceName);
                } else if (returnOtherRelayAdv) {
                    relayAdv = relayServerCache.getRandomCacheAdv();
                }
                if (relayAdv != null) {
                    XMLDocument asDoc = (XMLDocument) relayAdv.getDocument(MimeMediaType.XMLUTF8);
                    
                    MessageElement relayAdvElement = new TextDocumentMessageElement(RelayTransport.RELAY_ADV_ELEMENT, asDoc, null);

                    responseMessage.addMessageElement(RelayTransport.RELAY_NS, relayAdvElement);
                }
            } else {
                // We can't keep the messenger.
                // the client was not added, send a disconnected response
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("could not add client " + clientPeerId);
                }
                
                // We do not get a messenger for ourselves here, so
                // just get one from the router ourselves, if we have to.
                // and can.
                if (messenger == null) {
                    // If we did not get one and manage to obtain one
                    // from the endpoint, we can use it in-line, but
                    // we must close it. (The only case).
                    messenger = endpointService.getMessenger(clientAddr);
                    if (messenger != null) {
                        closeMessenger = true;
                    }
                    
                } else {
                    
                    // This is the incoming messenger. We cannot use it
                    // synchronously. See, the use of BGSend, below.
                    
                    rawMessenger = true;
                }
                
                responseMessage = RelayTransport.createDisconnectedMessage();
                
                // add the relay advertisement of another know relay for the client to try
                RdvAdvertisement relayAdv = relayServerCache.getRandomCacheAdv();
                
                if (relayAdv != null) {
                    XMLDocument asDoc = (XMLDocument) relayAdv.getDocument(MimeMediaType.XMLUTF8);
                    
                    MessageElement relayAdvElement = new TextDocumentMessageElement(RelayTransport.RELAY_ADV_ELEMENT, asDoc, null);

                    responseMessage.addMessageElement(RelayTransport.RELAY_NS, relayAdvElement);
                }
            }
        } else if (RelayTransport.DISCONNECT_REQUEST.equals(request)) {
            // Disconnect Request, don't send a response
            if (clientPeerId != null) {
                closingHandler = removeClient(clientPeerId);
                if (closingHandler != null) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("removed client " + clientPeerId);
                    }
                }
            }
        } else if (RelayTransport.PID_REQUEST.equals(request)) {
            
            // Generate a PeerID in the same group as our PeerID.
            // The group which my peerID stems from is not necessarily
            // the group where I am running (more likely it is the net peer
            // group). Rather than guessing, get the group from our own PID.
            
            PeerGroupID groupOfMyPid = (PeerGroupID) group.getPeerID().getPeerGroupID();
            
            String pidStr = IDFactory.newPeerID(groupOfMyPid).toString();
            
            responseMessage = RelayTransport.createPIDResponseMessage(pidStr);
            
            // If there is a raw incoming messenger, that's what we
            // use. Else, we won't respond.
            rawMessenger = true;
        }
        
        // if there is a messenger and a response, send it
        if (messenger != null && responseMessage != null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("sending response to client " + clientPeerId);
            }
            
            // If rawMessenger, then this is the incoming
            // messenger brought in by messengerReady. In that case,
            // be carefull. It is synchronous and it could block this
            // here thread until the message can be sent. Which could
            // possibly imply that this here method returns...dead lock.
            // See HttpMessageServlet: messengerReady is called by
            // the same thread that subsequently picks up messages from
            // the BCMessenger. So, spawn a thread to reply.
            // FIXME: eventualy we should start replacing some listener
            // based code with state machines and event queues.
            
            if (rawMessenger) {
                
                // BGSend will *not* close the messenger after use
                // Because incoming messengers do not need to be closed.
                new BGSend(messenger, responseMessage, serviceName, peerId);
            } else {
                try {
                    messenger.sendMessage(responseMessage, serviceName, peerId);
                } catch (IOException e) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Could not send response message to " + clientPeerId, e);
                    }
                }
            }
        }
        
        if (closeMessenger) {
            messenger.close();
        }
        
        if (closingHandler != null) {
            closingHandler.closeClient();
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("relayedClients.size()=" + relayedClients.size());
        }
    }
    
    private RelayServerClient getClient(String clientPeerId) {
        RelayServerClient handler;
        
        synchronized (relayedClients) {
            handler = relayedClients.get(clientPeerId);
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("getClient(" + clientPeerId + ") = " + handler);
        }
        
        return handler;
    }
    
    // Add client is idempotent. It can be called for a client that already
    // exists. The flushqueue option instructs to clear the queue if the client
    // exists.
    private RelayServerClient addClient(String clientPeerId, long requestedLease, Messenger messenger, boolean flushQueue) {
        RelayServerClient handler;
        boolean isNewClient = false;
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("addClient(" + clientPeerId + ")");
        }
        
        synchronized (relayedClients) {
            // check if this client is already registered
            handler = relayedClients.get(clientPeerId);
            if (handler == null) {
                // make sure the maximum number of clients has not been reached
                // and make sure that we have a messenger to give to the new
                // clientHandler.
                if ((relayedClients.size() < maxClients) && (messenger != null) && !messenger.isClosed()) {
                    
                    // create a new handler
                    handler = new RelayServerClient(this, clientPeerId, requestedLease, stallTimeout, clientQueueSize);
                    
                    // add the handler to the list
                    relayedClients.put(clientPeerId, handler);
                    isNewClient = true;
                    
                    // check if this is the first client added
                    if (relayedClients.size() == 1) {
                        // start the gcThread if it is not already started
                        if (gcThread == null) {
                            gcThread = new Thread(group.getHomeThreadGroup(), this, "GC Thread for Relay at " + publicAddress);
                            gcThread.setDaemon(true);
                            gcThread.start();
                        }
                    }
                } else {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine(
                                "new client denied. nb clients: " + relayedClients.size() + "/" + maxClients + ", messenger: "
                                + messenger);
                    }
                }
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("added = " + (handler != null));
        }
        
        if (handler == null) {
            return null;
        }
        
        // renew the lease on the old handler
        // Watchout. The handler might have expired since we got it from the
        // map. RenewLease will tell us. In that case, tough luck. We don't
        // make a new one. FIXME: it's not nice to the client, but in no way
        // a disaster (and very rare).
        
        if (!handler.renewLease()) {
            return null;
        }
        
        if (flushQueue) {
            handler.flushQueue();
        }
        
        if (messenger != null) {
            handler.addMessenger(messenger);
            
            // We must force the router to learn the new relay connection as
            // a direct route, so that it replies to route queries even if we
            // never start talking to the client otherwise.
            // Here we do something rather acrobatic. We invoke messengerReady
            // recursively with a new relay messenger that the router will
            // catch as if it where an incoming messenger (which it is, sort
            // of). The cleaner alternative: call getMessenger with a hint
            // causes too much commotion: sometimes an unreachable tcp address
            // is tried before the hint, which blocks getMessenger for long.
            
            if (isNewClient) {
                EndpointAddress ear = new EndpointAddress(RelayTransport.protocolName, clientPeerId, null, null);
                
                MessengerEvent me = new MessengerEvent(this, handler.getMessenger(publicAddress, ear, false), null);
                
                messengerEventListener.messengerReady(me);
            }
        }
        
        return handler;
    }
    
    private RelayServerClient removeClient(String clientPeerId) {
        RelayServerClient handler;
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("removeClient(" + clientPeerId + ")");
        }
        
        synchronized (relayedClients) {
            handler = relayedClients.remove(clientPeerId);
            
            // check if there are any clients
            if (relayedClients.size() == 0) {
                // stop the gcThread
                if (gcThread != null) {
                    try {
                        gcThread.interrupt();
                    } catch (SecurityException e) {
                        // ignore this exception
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(e.toString());
                        }
                    }
                }
            }
        }
        
        return handler;
    }
    
    // this is only used by the RelayServerClient when it is closing and needs to remove itself
    protected void removeClient(String clientPeerId, RelayServerClient handler) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("removeClient(" + clientPeerId + "," + handler + ")");
        }
        
        synchronized (relayedClients) {
            RelayServerClient currentHandler = relayedClients.get(clientPeerId);
            
            // only remove the client if the current handler matches the passed one
            if (currentHandler == handler) {
                relayedClients.remove(clientPeerId);
            }
            
            // check if there are any clients
            if (relayedClients.size() == 0) {
                // stop the gcThread
                Thread temp = gcThread;
                
                if (temp != null) {
                    try {
                        temp.interrupt();
                    } catch (SecurityException e) {
                        // ignore this exception
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(e.toString());
                        }
                    }
                }
            }
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    public void run() {
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Starting lease gc thread");
        }
        
        try {
            while (true) {
                // check if there are any client handlers left
                synchronized (relayedClients) {
                    if (relayedClients.size() == 0) {
                        break;
                    }
                }
                
                // do the lease gc
                doClientGC();
                
                // check if there are any client handlers left
                synchronized (relayedClients) {
                    if (relayedClients.size() == 0) {
                        break;
                    }
                }
                
                // sleep for a while.
                try {
                    Thread.sleep(stallTimeout);
                } catch (InterruptedException e) {
                    Thread.interrupted();
                }
            }
        } catch (Throwable all) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            }
        } finally {
            gcThread = null;
            
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("stopping lease gc thread");
            }
        }
    }
    
    // checks for expired client handlers
    private void doClientGC() {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("start: check for expired client handler. # clients = " + relayedClients.size());
        }
        
        // get a snapshot of the client handlers
        RelayServerClient[] handlers;
        
        synchronized (relayedClients) {
            handlers = relayedClients.values().toArray(new RelayServerClient[0]);
        }
        
        // run through the client handlers
        int i = handlers.length;
        
        while (i-- > 0) {
            try {
                // simply calling isExpired will cause the handler to check
                // if it is expired and remove itself if expired
                handlers[i].isExpired();
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Exception during client gc", e);
                }
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("stop: check for expired client handler. # clients = " + relayedClients.size());
        }
    }
    
    private static class RelayServerCache implements PipeMsgListener, Runnable {
        final static ID pipeID = ID.create(
                URI.create("urn:jxta:uuid-59616261646162614E50472050325033DEADBEEFDEAFBABAFEEDBABE0000000F04"));
        
        final RelayServer server;
        final PipeAdvertisement pipeAdv;
        InputPipe inputPipe = null;
        
        volatile boolean doRun = false;
        Thread cacheThread = null;
        
        final Map<String, RdvAdvertisement> relayAdvCache = new HashMap<String, RdvAdvertisement>();
        
        final Random rand = new Random();
        
        protected RelayServerCache(RelayServer server) {
            this.server = server;
            
            pipeAdv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(PipeAdvertisement.getAdvertisementType());
            pipeAdv.setPipeID(pipeID);
            pipeAdv.setType(PipeService.PropagateType);
        }
        
        private int relayAdvCacheSize() {
            synchronized (relayAdvCache) {
                return relayAdvCache.size();
            }
        }
        
        protected RdvAdvertisement getRandomCacheAdv() {
            synchronized (relayAdvCache) {
                RdvAdvertisement[] items = relayAdvCache.values().toArray(new RdvAdvertisement[0]);
                
                if (items.length == 0) {
                    return null;
                }
                
                return items[rand.nextInt(items.length)];
            }
        }
        
        private boolean putCacheAdv(String peerId, RdvAdvertisement adv) {
            if (!server.acl.isAllowed(adv.getPeerID())) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Rejected cache entry for : " + peerId);
                }
                return false;
            }
            synchronized (relayAdvCache) {
                boolean replaced = (null != relayAdvCache.put(peerId, adv));
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine((replaced ? "Updated" : "Created") + " cache entry for : " + peerId);
                }
                
                if (relayAdvCache.size() >= MAX_CACHED_SERVERS) {
                    // New entry and map full. Remove one at random.
                    String[] keys = relayAdvCache.keySet().toArray(new String[0]);
                    
                    relayAdvCache.remove(keys[rand.nextInt(keys.length)]);
                }
                
                return replaced;
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        public void pipeMsgEvent(PipeMsgEvent event) {
            Message message = event.getMessage();
            
            if (message == null) {
                return;
            }
            
            boolean isResponse = (RelayTransport.getString(message, RelayTransport.RESPONSE_ELEMENT) != null);
            String peerId = RelayTransport.getString(message, RelayTransport.PEERID_ELEMENT);
            
            if (peerId == null || peerId.equals(server.peerId)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("pipeMsgEvent() discarding message no response PID defined, or loopback ");
                }
                return;
            }
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("pipeMsgEvent() from " + peerId);
            }
            
            MessageElement me = message.getMessageElement(RelayTransport.RELAY_NS, RelayTransport.RELAY_ADV_ELEMENT);
            
            if (null == me) {
                return;
            }
            
            Advertisement adv;
            try {
                // XXX bondolo 20041207 Force parsing of MessageElement as 
                // XMLUTF8 rather than the actual mime type associated with the
                // MessageElement since the advertisement is often incorrectly
                // stored as a String by older JXTA implementations.
                adv = AdvertisementFactory.newAdvertisement(MimeMediaType.XMLUTF8, me.getStream());
            } catch (IOException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed building relay advertisement", failed);
                }
                return;
            } catch (NoSuchElementException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not build relay advertisement", failed);
                }
                return;
            }
            
            if (!(adv instanceof RdvAdvertisement)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Response does not contain relay advertisement (" + adv.getAdvType() + ")");
                }
                return;
            }
            
            RdvAdvertisement radv = (RdvAdvertisement) adv;
            
            if (putCacheAdv(peerId, radv)) {
                
                // New entry, we might want to respond.
                // "someone" should respond; on average, one response
                // is all we want. And that response obviously should be
                // unicast.
                // We achieve an approximation of that by making a computation
                // that will result in "true" on average on only one peer
                // of the set, based on our idea of what the set is.
                // If we know very few other relays compared to what other
                // relays know, we are more likely to respond than they are.
                // So this is very approximate. We want to keep it simple
                // until we have time replace this lazy junk with something
                // sensible.
                
                // If it's a response already, the story stops here !
                if (isResponse) {
                    return;
                }
                
                // Here we go:
                int i = relayAdvCacheSize();
                long magic = server.peerId.hashCode() % i;
                
                if (rand.nextInt(i) == magic) {
                    
                    // Our number came out. Respond.
                    
                    // See if we have amunition to respond anyway.
                    // Very defensive. I care a lot more not to break anything
                    // at this stage, than to have optimal functionality.
                    
                    RdvAdvertisement myAdv = RelayServer.createRdvAdvertisement(server.group.getPeerAdvertisement(), server.serviceName);

                    // Need to convert the other party's string pid into
                    // a real pid.
                    PeerID otherPid = null;
                    try {
                        otherPid = (PeerID) IDFactory.fromURI(new URI(ID.URIEncodingName, ID.URNNamespace + ":" + peerId, null));
                    } catch (Exception ex) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Bad peerid : " + peerId, ex);
                        }
                        
                    }

                    PipeService pipeService = server.group.getPipeService();
                    if (pipeService == null) {
                        return; // Funny. We're receiving messages, after all.
                    }
                    
                    // FIXME: jice@jxta.org 20030131 - We're making a rather
                    // unorthodox use of the peer-subset feature of propagate
                    // pipes. Basically what this does is to send the message
                    // in unicast so that it is received on the propagate
                    // input pipe of the specified peer.
                    // The correct API, if it existed, would be respond().
                    
                    OutputPipe retPipe = null;
                    try {
                        retPipe = pipeService.createOutputPipe(pipeAdv, Collections.singleton(otherPid), 2 * TimeUtils.ASECOND);
                        if (retPipe == null) {
                            return;
                        }
                        
                        // create a new cache message
                        message = new Message();
                        
                        // String version of unique portion only. Per the protocol.
                        RelayTransport.setString(message, RelayTransport.PEERID_ELEMENT, server.peerId);
                        // Our own adv.
                        RelayTransport.setString(message, RelayTransport.RELAY_ADV_ELEMENT, myAdv.toString());
                        
                        // This is a response. New servers: do not respond! Old
                        // servers won't respond anyway.
                        RelayTransport.setString(message, RelayTransport.RESPONSE_ELEMENT, "t");
                        
                        retPipe.send(message);
                        
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Responded");
                        }
                        
                    } catch (IOException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Could not send reply on pipe ", e);
                        }
                    }
                    
                    if (retPipe != null) {
                        retPipe.close();
                    }
                }
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        public void run() {
            try {
                OutputPipe outputPipe = null;
                PipeService pipeService = server.group.getPipeService();
                
                while (doRun && inputPipe == null) {
                    try {
                        inputPipe = pipeService.createInputPipe(pipeAdv, this);
                    } catch (IOException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Could not create input pipe, try again");
                        }
                    } catch (IllegalStateException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Pipe Service not ready yet, try again");
                        }
                    }
                    
                    try {
                        Thread.sleep(TimeUtils.ASECOND);
                    } catch (InterruptedException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("wait interrupted");
                        }
                    }
                }
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Created input pipe");
                }
                
                while (doRun && outputPipe == null) {
                    try {
                        outputPipe = pipeService.createOutputPipe(pipeAdv, 5 * TimeUtils.ASECOND);
                    } catch (IOException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Could not create output pipe, try again");
                        }
                    } catch (IllegalStateException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Pipe Service not ready yet, try again");
                        }
                    }
                    
                    try {
                        Thread.sleep(TimeUtils.ASECOND);
                    } catch (InterruptedException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("wait interrupted ");
                        }
                    }
                }
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Created output pipe");
                }
                
                // Wait a little before mcasting our hello.
                // We depend on the rendezvous infrastructure for it to
                // work. It's pretty important to get the first one out
                // so that we may get a response from others. After that
                // the interval is very long (and its computation an total
                // nonsense) and so others do not talk much
                // either. We want to learn at least one other relay early on.
                // FIXME: jice@jxta.org 20030131 - We realy need to switch to
                // using peerview. It does all of that correctly.
                
                try {
                    Thread.sleep(10 * TimeUtils.ASECOND);
                } catch (InterruptedException e) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("wait interrupted");
                    }
                }
                
                while (doRun) {
                    RdvAdvertisement adv = RelayServer.createRdvAdvertisement(server.group.getPeerAdvertisement(), server.serviceName);
                    
                    // Make sure that the version that can be discovered
                    // is consistent.
                    try {
                        server.discoveryService.publish(adv);
                    } catch (IOException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Could not publish Relay RdvAdvertisement", e);
                        }
                    }
                    
                    if (adv != null) {
                        // create a new cache message
                        Message message = new Message();
                        
                        RelayTransport.setString(message, RelayTransport.PEERID_ELEMENT, server.peerId);
                        RelayTransport.setString(message, RelayTransport.RELAY_ADV_ELEMENT, adv.toString());
                        
                        try {
                            outputPipe.send(message);
                        } catch (IOException e) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.log(Level.FINE, "Could not send message on pipe ", e);
                            }
                        }
                    }
                    
                    long sleepTime = server.minBroadcastInterval
                            + ((server.relayedClients.size() + 1) * 100 / (server.maxClients + 1)) * server.minBroadcastInterval;
                    
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("sleepTime=" + sleepTime);
                    }
                    
                    try {
                        Thread.sleep(sleepTime);
                    } catch (InterruptedException e) {
                        Thread.interrupted();
                    }
                }
                outputPipe.close();
                if (System.currentTimeMillis() > server.refreshTime) {
                    server.refreshTime = System.currentTimeMillis() + ACL_REFRESH_PERIOD;
                    if (server.aclFile.lastModified() > server.aclFileLastModified) {
                        server.aclFileLastModified = server.aclFile.lastModified();
                        server.acl.refresh(server.aclFile);
                    }
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
                }
            } finally {
                cacheThread = null;
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Cache thread quitting.");
                }
            }
        }
        
        protected void startCache() {
            doRun = true;
            cacheThread = new Thread(server.group.getHomeThreadGroup(), this, "RelayCache Worker Thread for " + server.publicAddress);
            cacheThread.setDaemon(true);
            cacheThread.start();
        }
        
        protected void stopCache() {
            doRun = false;
            
            if (inputPipe != null) {
                inputPipe.close();
                inputPipe = null;
            }
            cacheThread.interrupt();
        }
    }
    

    /**
     *  Sends a message on an synchronous messenger.
     */
    static class BGSend extends Thread {
        
        Messenger mr;
        Message ms;
        String sn;
        String ps;
        
        BGSend(Messenger mr, Message ms, String sn, String ps) {
            super("Relay Background Sender");
            this.mr = mr;
            this.ms = ms;
            this.sn = sn;
            this.ps = ps;
            setDaemon(true);
            start();
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public void run() {
            try {
                mr.sendMessage(ms, sn, ps);
            } catch (IOException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed sending response " + ms + " to " + ps, e);
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
                }
            }
            
        }
    }
    
    private static RdvAdvertisement createRdvAdvertisement(PeerAdvertisement padv, String name) {
        try {
            // FIX ME: 10/19/2002 lomax@jxta.org. We need to properly set up the service ID. Unfortunately
            // this current implementation of the PeerView takes a String as a service name and not its ID.
            // Since currently, there is only PeerView per group (all peerviews share the same "service", this
            // is not a problem, but that will have to be fixed eventually.
            
            // create a new RdvAdvertisement
            RdvAdvertisement rdv = (RdvAdvertisement) AdvertisementFactory.newAdvertisement(
                    RdvAdvertisement.getAdvertisementType());
            
            rdv.setPeerID(padv.getPeerID());
            rdv.setGroupID(padv.getPeerGroupID());
            rdv.setServiceName(name);
            rdv.setName(padv.getName());
            
            RouteAdvertisement ra = EndpointUtils.extractRouteAdv(padv);
            
            if (null == ra) {
                // No route available
                return null;
            }
            
            // Insert it into the RdvAdvertisement.
            rdv.setRouteAdv(ra);
            
            return rdv;
        } catch (Exception ez) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Cannot create Local RdvAdvertisement: ", ez);
            }
            return null;
        }
    }
        
}
