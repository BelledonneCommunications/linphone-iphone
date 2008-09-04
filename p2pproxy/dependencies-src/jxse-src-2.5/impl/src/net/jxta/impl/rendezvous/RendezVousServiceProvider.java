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
package net.jxta.impl.rendezvous;

import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.id.UUID.UUID;
import net.jxta.impl.rendezvous.rendezvousMeter.RendezvousMeter;
import net.jxta.impl.rendezvous.rendezvousMeter.RendezvousMeterBuildSettings;
import net.jxta.impl.rendezvous.rendezvousMeter.RendezvousServiceMonitor;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.PeerAdvertisement;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * This abstract class must be extended for all RendezVous Service providers
 * that are managed by RendezVousServiceImpl.
 * <p/>
 * Implementors of providers are responsible for using appropriate
 * synchronization. The RendezvousServiceImpl provides synchronization control
 * only only those methods which involve changing the active provider.
 */
public abstract class RendezVousServiceProvider implements EndpointListener {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(RendezVousServiceProvider.class.getName());

    protected static final String PropSName = "JxtaPropagate";

    protected static final String RDV_MSG_NAMESPACE_NAME = "jxta";

    protected final String PropPName;
    protected final String PROP_HDR_ELEMENT_NAME;

    /**
     * Maximum TTL we will allow for propagation and repropagation issued by
     * this peer.
     */
    protected int MAX_TTL;

    protected final PeerGroup group;
    protected final RendezVousServiceImpl rdvService;
    protected boolean closed = false;

    private PeerAdvertisement cachedPeerAdv = null;
    private int cachedPeerAdvModCount = -1;
    private XMLDocument cachedPeerAdvDoc = null;

    protected RendezvousServiceMonitor rendezvousServiceMonitor = null;
    protected RendezvousMeter rendezvousMeter = null;

    /**
     *
     * @param group the peergroup
     * @param rdvService the implementation
     */
    protected RendezVousServiceProvider(PeerGroup group, RendezVousServiceImpl rdvService) {

        this.group = group;
        this.rdvService = rdvService;

        PropPName = this.group.getPeerGroupID().getUniqueValue().toString();
        PROP_HDR_ELEMENT_NAME = RendezVousPropagateMessage.Name + PropPName;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * EndpointListener for the JxtaPropagate/<peergroup-unique value>
     */
    public void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        RendezVousPropagateMessage propHdr = checkPropHeader(msg);

        if (null != propHdr) {
            // Get the destination real destination of the message
            String sName = propHdr.getDestSName();
            String sParam = propHdr.getDestSParam();

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Processing " + msg + "(" + propHdr.getMsgId() + ") for " + sName + "/" + sParam + " from " + srcAddr);
            }

            // Check if we have a local listener for this message
            processReceivedMessage(msg, propHdr, srcAddr, new EndpointAddress(dstAddr, sName, sParam));
        }
    }

    protected XMLDocument getPeerAdvertisementDoc() {
        PeerAdvertisement newPadv;

        synchronized (this) {
            newPadv = group.getPeerAdvertisement();
            int newModCount = newPadv.getModCount();

            if ((cachedPeerAdv != newPadv) || (cachedPeerAdvModCount != newModCount)) {
                cachedPeerAdv = newPadv;
                cachedPeerAdvModCount = newModCount;
            } else {
                newPadv = null;
            }

            if (null != newPadv) {
                cachedPeerAdvDoc = (XMLDocument) cachedPeerAdv.getDocument(MimeMediaType.XMLUTF8);
            }
        }

        return cachedPeerAdvDoc;
    }

    /**
     * Supply arguments and starts this service if it hadn't started by itself.
     * <p/>
     * Currently this service starts by itself and does not expect arguments.
     * @return 0 if successful
     * @param arg argument params
     */
    protected int startApp(String[] arg) {

        // All propagated messages originated by RendezvousService.propagate are handled by the
        // rendezvous service before being delivered to their local recipient.
        // This includes:
        // messages delivered here via netWorkPropagation. Therefore the rdv service has a special
        // endpointService listener for that purpose.
        // messages delivered here by rdv-to-rdv walk.
        try {
            if (!rdvService.endpoint.addIncomingMessageListener(this, PropSName, PropPName)) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe("Cannot register the propagation listener (already registered)");
                }

                return -1;
            }
        } catch (Exception ez1) {
            // Not much we can do here.
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Failed registering the endpoint listener", ez1);
            }

            return -1;
        }

        try {
            // Update the peeradv with our status
            if (rdvService.isRendezVous()) {
                XMLDocument params = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");
                XMLElement e = params.createElement("Rdv", Boolean.TRUE.toString());

                params.appendChild(e);
                group.getPeerAdvertisement().putServiceParam(rdvService.getAssignedID(), params);
            } else {
                group.getPeerAdvertisement().removeServiceParam(rdvService.getAssignedID());
            }
        } catch (Exception ignored) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not update Rdv Params in Peer Advertisement", ignored);
            }
        }

        return Module.START_OK;
    }

    /**
     * Ask this service to stop.
     */
    protected void stopApp() {
        EndpointListener shouldbeMe = rdvService.endpoint.removeIncomingMessageListener(PropSName, PropPName);

        if ((null != shouldbeMe) && (this != shouldbeMe)) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Unregistered listener was not as expected." + this + " != " + shouldbeMe);
            }
        }

        // Update the peeradv. We are not a rdv.
        group.getPeerAdvertisement().removeServiceParam(rdvService.getAssignedID());
    }

    /**
     * Set the RendezvousServiceMonitor. The RendezvousServiceMonitor is used to
     * meter the activity of the RendezvousService.
     *
     * @param rendezvousServiceMonitor The monitor.
     * @see net.jxta.impl.meter.MonitorManager
     */
    public void setRendezvousServiceMonitor(RendezvousServiceMonitor rendezvousServiceMonitor) {
        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING) {
            this.rendezvousServiceMonitor = rendezvousServiceMonitor;

            if (rendezvousServiceMonitor != null) {
                this.rendezvousMeter = rendezvousServiceMonitor.getRendezvousMeter();
            }
        }
    }

    /**
     * Resets the local idea of the lease to the specified value.
     * As a result a lease response must be sought and obtained within the
     * new specified delay or the rdv is considered disconnected.
     *
     * @param peer  The peer to be challenged
     * @param delay The delay
     */
    public abstract void challengeRendezVous(ID peer, long delay);

    /**
     * Attempt to connect to the specified rendezvous peer.
     *
     * @param addr The endpoint address of the rendezvous peer.
     * @param hint An optional hint which may be {@code null}.
     * @throws IOException If no connection could be made to the specified peer.
     */
    public abstract void connectToRendezVous(EndpointAddress addr, Object hint) throws IOException;

    /**
     * Remove a RendezVousService point.
     *
     * @param peerID the PeerId of the RendezVous to disconnect from.
     */
    public abstract void disconnectFromRendezVous(ID peerID);

    /**
     * Returns the peers that are currently connected to this peer.
     *
     * @return The peers that are currently connected to this peer.
     */
    public abstract Vector<ID> getConnectedPeerIDs();

    /**
     * Propagates a message onto as many peers on the local network
     * as possible. Typically the message will go to all the peers to
     * which at least one endpoint transport can address without using
     * the router.
     * <p/>
     * This method sends the message to all peers, rendezvous peers and
     * edge peer. This method of propagation is very expensive and should
     * not be frequently used. When rendezvous peers are used in order to
     * cache index of data, it is more efficient to use the walk() method.
     * <p/>
     * Only a single HOP at a time is performed. Messages are always
     * delivered to the destination handler on arrival. This handler
     * is responsible for repropagating further, if deemed appropriate.
     * <p/>
     * Loop and TTL control are performed automatically.
     * <p/>
     * Messages can be propagated via this method for the first time or
     * can be re-propagated by re-using a message that came in via propagation.
     * In the later case, the TTL and loop detection parameters CANNOT be
     * re-initialized. If one wants to "re-propagate" a message with a new TTL
     * and blank gateways list one must generate a completely new message.
     * This limits the risk of accidental propagation storms, although they
     * can always be engineered deliberately.
     * <p/>
     * Note: The original msg is not modified and may be reused upon return.
     *
     * @param msg          is the message to propagate.
     * @param serviceName  is the name of the service
     * @param serviceParam is the parameter of the service
     * @param initialTTL   is the maximum TTL of the message (note that the Rendezvous
     *                     Service implementation is free to decrease that value.
     * @throws java.io.IOException if an io error occurs
     */
    public abstract void propagate(Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException;

    /**
     * Propagates a message onto as many peers on the local network
     * as possible. Typically the message will go to all the peers to
     * which at least one endpoint transport can address without using
     * the router.
     * <p/>
     * This method sends the message to all peers, rendezvous peers and
     * edge peer. This method of propagation is very expensive and should
     * not be frequently used. When rendezvous peers are used in order to
     * cache index of data, it is more efficient to use the walk() method.
     * <p/>
     * Only a single HOP at a time is performed. Messages are always
     * delivered to the destination handler on arrival. This handler
     * is responsible for repropagating further, if deemed appropriate.
     * <p/>
     * Loop and TTL control are performed automatically.
     * <p/>
     * Messages can be propagated via this method for the first time or
     * can be re-propagated by re-using a message that came in via propagation.
     * In the later case, the TTL and loop detection parameters CANNOT be
     * re-initialized. If one wants to "re-propagate" a message with a new TTL
     * and blank gateways list one must generate a completely new message.
     * This limits the risk of accidental propagation storms, although they
     * can always be engineered deliberately.
     * <p/>
     * Note: The original msg is not modified and may be reused upon return.
     *
     * @param destPeerIds  An enumeration of the peers that are recipients of the
     *                     propagated message.
     * @param msg          is the message to propagate.
     * @param serviceName  is the name of the service
     * @param serviceParam is the parameter of the service
     * @param initialTTL   is the maximum TTL of the message (note that the Rendezvous
     *                     Service implementation is free to decrease that value.
     * @throws java.io.IOException if an io error occurs
     */
    public abstract void propagate(Enumeration<? extends ID> destPeerIds, Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException;

    /**
     * Propagates a message onto as many peers on the local network
     * as possible. Typically the message will go to all the peers to
     * which at least one endpoint transport can address without using
     * the router.
     * <p/>
     * Only a single HOP at a time is performed. Messages are always
     * delivered to the destination handler on arrival. This handler
     * is responsible for repropagating further, if deemed appropriate.
     * <p/>
     * Loop and TTL control are performed automatically.
     * <p/>
     * Messages can be propagated via this method for the first time or
     * can be re-propagated by re-using a message that came in via propagation.
     * In the later case, the TTL and loop detection parameters CANNOT be
     * re-initialized. If one wants to "re-propagate" a message with a new TTL
     * and blank gateways list one must generate a completely new message.
     * This limits the risk of accidental propagation storms, although they
     * can always be engineered deliberately.
     * <p/>
     * Note: The original msg is not modified and may be reused upon return.
     *
     * @param msg          is the message to propagate.
     * @param serviceName  is the name of the service
     * @param serviceParam is the parameter of the service
     * @param initialTTL   is the maximum TTL of the message (note that the Rendezvous
     *                     Service implementation is free to decrease that value.
     * @throws java.io.IOException if an io error occurs
     */
    public abstract void propagateToNeighbors(Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException;

    /**
     * Return true if connected to a rendezvous.
     *
     * @return true if connected to a rendezvous, false otherwise
     */
    public abstract boolean isConnectedToRendezVous();

    /**
     ** The following API is related to the new Rendezvous Peer walk mechanism.
     **
     **/

    /**
     * Walk a message through the rendezvous peers of the network: only
     * rendezvous peers will receive the message.
     * <p/>
     * Only a single HOP at a time is performed. Messages are always
     * delivered to the destination handler on arrival. This handler
     * is responsible for repropagating further, if deemed appropriate.
     * <p/>
     * Loop and TTL control are performed automatically.
     * <p/>
     * Messages can be propagated via this method for the first time or
     * can be re-propagated by re-using a message that came in via propagation.
     * In the later case, the TTL and loop detection parameters CANNOT be
     * re-initialized. If one wants to "re-propagate" a message with a new TTL
     * and blank gateways list one must generate a completely new message.
     * This limits the risk of accidental propagation storms, although they
     * can always be engineered deliberately.
     * <p/>
     * Note: The original msg is not modified and may be reused upon return.
     *
     * @param msg          is the message to walk.
     * @param serviceName  is the name of the service
     * @param serviceParam is the parameter of the service
     * @param initialTTL   is the maximum TTL of the message (note that the Rendezvous
     *                     Service implementation is free to decrease that value.
     * @throws IOException when walking the message is impossible (network failure)
     */
    public abstract void walk(Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException;

    /**
     * Walk a message through the rendezvous peers of the network: only
     * rendezvous peers will receive the message.
     * <p/>
     * Only a single HOP at a time is performed. Messages are always
     * delivered to the destination handler on arrival. This handler
     * is responsible for repropagating further, if deemed appropriate.
     * <p/>
     * Loop and TTL control are performed automatically.
     * <p/>
     * Messages can be propagated via this method for the first time or
     * can be re-propagated by re-using a message that came in via propagation.
     * In the later case, the TTL and loop detection parameters CANNOT be
     * re-initialized. If one wants to "re-propagate" a message with a new TTL
     * and blank gateways list one must generate a completely new message.
     * This limits the risk of accidental propagation storms, although they
     * can always be engineered deliberately.
     * <p/>
     * Note: The original msg is not modified and may be reused upon return.
     *
     * @param destPeerIDs  is a Vector of PeerID of the peers which are receiving
     *                     first the walker. Note that each entry in the Vector will create its own
     *                     walker.
     * @param msg          is the message to walk.
     * @param serviceName  is the name of the service
     * @param serviceParam is the parameter of the service
     * @param initialTTL   is the maximum TTL of the message (note that the Rendezvous
     *                     Service implementation is free to decrease that value.
     * @throws IOException when walking the message is impossible (network failure)
     */
    public abstract void walk(Vector<? extends ID> destPeerIDs, Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException;

    /**
     * Process a propagated message.
     *
     * @param message the message received
     * @param propHdr the message header
     * @param srcAddr the source address
     * @param dstAddr the message destination addreess
     */
    protected void processReceivedMessage(Message message, RendezVousPropagateMessage propHdr, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        EndpointListener listener = rdvService.endpoint.getIncomingMessageListener(dstAddr.getServiceName(),
                dstAddr.getServiceParameter());

        if (listener != null) {
            // We have a local listener for this message. Deliver it.

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Calling local listener " + listener.getClass().getName() + " for [" + dstAddr.getServiceName() + "/"
                        + dstAddr.getServiceParameter() + "] with " + message + " (" + propHdr.getMsgId() + ")");
            }

            rdvService.endpoint.processIncomingMessage(message, srcAddr, dstAddr);
            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.receivedMessageProcessedLocally();
            }

            // NOTE: this this is only beneficial in ad-hoc mode with no rendezvous infrastructure
            // with one node bridging two networks (2 interfaces).
            // This should not be a base feature but rather an ad-hoc mode only feature, as
            // it creates additional unnecessary traffic leading to message loss and latency.
            // hamada disabling this feature as a general base functionality.
            // This should addressed differently in Ad-hoc mode, where a multi-homed node would
            // repropagate when more than one interface is enabled

            // Pass the message on.
            // repropagate(message, propHdr, dstAddr.getServiceName(), dstAddr.getServiceParameter());
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No message listener found for ServiceName :" + dstAddr.getServiceName() + " ServiceParam :"
                        + dstAddr.getServiceParameter());
            }
        }
    }

    /**
     * Responsible for forwarding received messages to the rest of the network
     * as appropriate. This generally only makes sense for multicast or
     * broadcast scenarios.
     *
     * @param msg          the message to be repropagated.
     * @param propHdr      It's current propagation header.
     * @param serviceName  The destination service.
     * @param serviceParam The destination service parameter.
     */
    protected abstract void repropagate(Message msg, RendezVousPropagateMessage propHdr, String serviceName, String serviceParam);

    public abstract void propagateInGroup(Message msg, String serviceName, String serviceParam, int ttl) throws IOException;

    /**
     * Propagates the message via endpoint propagation (multicast/broadcast) on
     * all Message Transports.
     * <p/>
     * Note: The original msg is not modified and may be reused upon return.
     *
     * @param msg     The message to be propagated.
     * @param propHdr It's current propagation header.
     * @throws java.io.IOException if an io error occurs
     */
    protected void sendToNetwork(Message msg, RendezVousPropagateMessage propHdr) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Endpoint propagating " + msg + " (" + propHdr.getMsgId() + ")");
        }

        rdvService.endpoint.propagate(msg, PropSName, PropPName);
    }

    /**
     * Convenience method for constructing an endpoint address from an id
     *
     * @param destPeer peer id
     * @param serv     the service name (if any)
     * @param parm     the service param (if any)
     * @return endpointAddress for this peer id.
     */
    protected static EndpointAddress mkAddress(String destPeer, String serv, String parm) {
        ID asID;
        try {
            asID = IDFactory.fromURI(new URI(destPeer));
        } catch (URISyntaxException caught) {
            throw new IllegalArgumentException(caught.getMessage());
        }

        return mkAddress(asID, serv, parm);
    }

    /**
     * Convenience method for constructing an endpoint address from an id
     *
     * @param destPeer peer id
     * @param serv     the service name (if any)
     * @param parm     the service param (if any)
     * @return endpointAddress for this peer id.
     */
    protected static EndpointAddress mkAddress(ID destPeer, String serv, String parm) {
        EndpointAddress addr = new EndpointAddress(RDV_MSG_NAMESPACE_NAME, destPeer.getUniqueValue().toString(), serv, parm);
        return addr;
    }

    /**
     * Get propagate header from the message.
     *
     * @param msg The source message.
     * @return The message's propagate header if any, otherwise null.
     */
    protected RendezVousPropagateMessage getPropHeader(Message msg) {

        MessageElement elem = msg.getMessageElement(RDV_MSG_NAMESPACE_NAME, PROP_HDR_ELEMENT_NAME);

        if (elem == null) {
            return null;
        }

        try {
            StructuredDocument asDoc = StructuredDocumentFactory.newStructuredDocument(elem);
            return new RendezVousPropagateMessage(asDoc);
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not get prop header of " + msg, failed);
            }
            IllegalArgumentException failure = new IllegalArgumentException("Could not get prop header of " + msg);
            failure.initCause(failed);
            throw failure;
        }
    }

    /**
     * Check and updates the header message element
     *
     * @param msg the message to check
     * @return an upadate message
     */
    protected RendezVousPropagateMessage checkPropHeader(Message msg) {
        RendezVousPropagateMessage propHdr;

        try {
            propHdr = getPropHeader(msg);
            if (null == propHdr) {
                // No header. Discard the message
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Discarding " + msg + " -- missing propagate header.");
                }
                if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                    rendezvousMeter.invalidMessageReceived();
                }
                return null;
            }
        } catch (Exception failure) {
            // Bad header. Discard the message
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Discarding " + msg + " -- bad propagate header.", failure);
            }

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.invalidMessageReceived();
            }
            return null;
        }
        // Look at the Propagate header if any and check for loops.
        // Do not remove it; we do not have to change it yet, and we have
        // do look at it at different places and looking costs less on
        // incoming elements than on outgoing.

        // TTL detection. A message arriving with TTL <= 0 should not even
        // have been sent. Kill it.

        if (propHdr.getTTL() <= 0) {
            // This message is dead on arrival. Drop it.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Discarding " + msg + "(" + propHdr.getMsgId() + ") -- dead on arrival (TTl=" + propHdr.getTTL() + ").");
            }

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.receivedDeadMessage();
            }
            return null;
        }

        if (!rdvService.addMsgId(propHdr.getMsgId())) {
            // We already received this message - discard
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Discarding " + msg + "(" + propHdr.getMsgId() + ") -- feedback.");
            }

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.receivedDuplicateMessage();
            }
            return null;
        }

        // Loop detection
        if (propHdr.isVisited(group.getPeerID().toURI())) {
            // Loop is detected - discard.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Discarding " + msg + "(" + propHdr.getMsgId() + ") -- loopback.");
            }
            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.receivedLoopbackMessage();
            }
            return null;
        }
        // Message is valid
        return propHdr;
    }

    protected RendezVousPropagateMessage updatePropHeader(Message msg, RendezVousPropagateMessage propHdr, String serviceName, String serviceParam, int initialTTL) {

        boolean newHeader = false;

        if (null == propHdr) {
            propHdr = newPropHeader(serviceName, serviceParam, initialTTL);
            newHeader = true;
        } else {
            if (null == updatePropHeader(propHdr, initialTTL)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("TTL expired for " + msg + " (" + propHdr.getMsgId() + ") TTL=" + propHdr.getTTL());
                }
                return null;
            }
        }

        XMLDocument propHdrDoc = (XMLDocument) propHdr.getDocument(MimeMediaType.XMLUTF8);
        MessageElement elem = new TextDocumentMessageElement(PROP_HDR_ELEMENT_NAME, propHdrDoc, null);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine((newHeader ? "Added" : "Updated") + " prop header for " + msg + " (" + propHdr.getMsgId() + ") TTL = "
                    + propHdr.getTTL());
        }

        msg.replaceMessageElement(RDV_MSG_NAMESPACE_NAME, elem);
        return propHdr;
    }

    /**
     * Adds a propagation header to the given message with the given default
     * TTL. Also adds this peer to the path recorded in the message.
     *
     * @param serviceName the service name
     * @param serviceParam the parameter
     * @param initialTTL initial TTL
     * @return  a updated message with the proper TTL and ID
     */
    private RendezVousPropagateMessage newPropHeader(String serviceName, String serviceParam, int initialTTL) {

        RendezVousPropagateMessage propHdr = new RendezVousPropagateMessage();

        propHdr.setTTL(initialTTL);
        propHdr.setDestSName(serviceName);
        propHdr.setDestSParam(serviceParam);

        UUID msgID = rdvService.createMsgId();

        propHdr.setMsgId(msgID);
        rdvService.addMsgId(msgID);

        // Add this peer to the path.
        propHdr.addVisited(group.getPeerID().toURI());

        return propHdr;
    }

    /**
     * Updates the propagation header of the message. Also adds this peer to the
     * path recorded in the message. Returns true if the message should be
     * repropagated, false otherwise.
     *
     * @param propHdr The propHdr for the message.
     * @param maxTTL  The maximum TTL which will be allowed.
     * @return The updated propagate header if the message should be
     *         repropagated otherwise null.
     */
    private RendezVousPropagateMessage updatePropHeader(RendezVousPropagateMessage propHdr, int maxTTL) {

        int msgTTL = propHdr.getTTL();
        URI me = group.getPeerID().toURI();

        int useTTL = msgTTL;

        if (!propHdr.isVisited(me)) {
            // only reduce TTL if message has not previously visited us.
            useTTL--;
        }

        // ensure TTL does not exceed maxTTL
        useTTL = Math.min(useTTL, maxTTL);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Updating propagation header (" + propHdr.getMsgId() + ") TTL: " + msgTTL + "-->" + useTTL);
        }

        propHdr.setTTL(useTTL);

        // Add this peer to the path.
        propHdr.addVisited(me);

        // If message came in with TTL one or less, it was last trip. It can not go any further.
        return (useTTL <= 0) ? null : propHdr;
    }
}
