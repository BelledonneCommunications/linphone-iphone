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

import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.endpoint.EndpointUtils;
import net.jxta.impl.rendezvous.rdv.RdvPeerRdvService;
import net.jxta.impl.rendezvous.rendezvousMeter.RendezvousMeterBuildSettings;
import net.jxta.impl.rendezvous.rpv.PeerViewElement;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.Timer;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Base class for providers which implement the JXTA Standard Rendezvous
 * Protocol.
 *
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-rvp" target="_blank">JXTA Protocols Specification : Rendezvous Protocol</a>
 */
public abstract class StdRendezVousService extends RendezVousServiceProvider {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(StdRendezVousService.class.getName());

    public final static String ConnectRequest = "Connect";
    public final static String DisconnectRequest = "Disconnect";
    public final static String ConnectedPeerReply = "ConnectedPeer";
    public final static String ConnectedLeaseReply = "ConnectedLease";
    public final static String ConnectedRdvAdvReply = "RdvAdvReply";

    /**
     * Default Maximum TTL.
     */
    protected static final int DEFAULT_MAX_TTL = 200;

    protected final String pName;
    protected final String pParam;

    /**
     * The registered handler for messages using the Standard Rendezvous
     * Protocol.
     *
     * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-rvp" target="_blank">JXTA Protocols Specification : Rendezvous Protocol
     */
    private StdRdvProtocolListener handler;

    protected final Timer timer;

    /**
     * Interface for listeners to : &lt;assignedID>/<group-unique>
     */
    protected interface StdRdvProtocolListener extends EndpointListener {}

    /**
     * Constructor
     *
     * @param group      the PeerGroup
     * @param rdvService the parent rendezvous service
     */
    protected StdRendezVousService(PeerGroup group, RendezVousServiceImpl rdvService) {

        super(group, rdvService);

        MAX_TTL = DEFAULT_MAX_TTL;

        pName = rdvService.getAssignedID().toString();
        pParam = group.getPeerGroupID().getUniqueValue().toString();

        timer = new Timer("StdRendezVousService Timer for " + group.getPeerGroupID(), true);
    }

    /**
     * {@inheritDoc}
     */
    protected int startApp(String[] argv, StdRdvProtocolListener handler) {

        this.handler = handler;

        rdvService.endpoint.addIncomingMessageListener(handler, pName, null);

        return super.startApp(argv);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void stopApp() {
        EndpointListener shouldbehandler = rdvService.endpoint.removeIncomingMessageListener(pName, null);

        if (handler != shouldbehandler) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Unregistered listener was not as expected." + handler + " != " + shouldbehandler);
            }
        }

        timer.cancel();

        super.stopApp();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void processReceivedMessage(Message message, RendezVousPropagateMessage propHdr, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        if (srcAddr.getProtocolName().equalsIgnoreCase("jxta")) {
            String idstr = ID.URIEncodingName + ":" + ID.URNNamespace + ":" + srcAddr.getProtocolAddress();

            ID peerid;
            try {
                peerid = IDFactory.fromURI(new URI(idstr));
            } catch (URISyntaxException badID) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Bad ID in message", badID);
                }
                return;
            }

            if (!group.getPeerID().equals(peerid)) {
                PeerConnection pConn = getPeerConnection(peerid);

                if (null == pConn) {
                    PeerViewElement pve;

                    if (this instanceof RdvPeerRdvService) {
                        // cheap hack....
                        pve = ((RdvPeerRdvService) this).rpv.getPeerViewElement(peerid);
                    } else {
                        pve = null;
                    }

                    if (null == pve) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Received " + message + " (" + propHdr.getMsgId() + ") from unrecognized peer : " + peerid);
                        }

                        propHdr.setTTL(Math.min(propHdr.getTTL(), 3)); // will be reduced during repropagate stage.

                        // FIXME 20040503 bondolo need to add tombstones so that we don't end up spamming disconnects.
                        if (rdvService.isRendezVous() || (getPeerConnections().length > 0)) {
                            // edge peers with no rdv should not send disconnect.
                            sendDisconnect(peerid, null);
                        }
                    } else {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Received " + message + " (" + propHdr.getMsgId() + ") from " + pve);
                        }
                    }
                } else {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Received " + message + " (" + propHdr.getMsgId() + ") from " + pConn);
                    }
                }
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Received " + message + " (" + propHdr.getMsgId() + ") from loopback.");
                }
            }
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Received " + message + " (" + propHdr.getMsgId() + ") from network -- repropagating with TTL 2");
            }

            propHdr.setTTL(Math.min(propHdr.getTTL(), 3)); // will be reduced during repropagate stage.
        }
        super.processReceivedMessage(message, propHdr, srcAddr, dstAddr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void propagate(Enumeration<? extends ID> destPeerIDs, Message msg, String serviceName, String serviceParam, int initialTTL) {
        msg = msg.clone();
        int useTTL = Math.min(initialTTL, MAX_TTL);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(
                    "Propagating " + msg + "(TTL=" + useTTL + ") to :" + "\n\tsvc name:" + serviceName + "\tsvc params:"
                    + serviceParam);
        }

        RendezVousPropagateMessage propHdr = updatePropHeader(msg, getPropHeader(msg), serviceName, serviceParam, useTTL);

        if (null != propHdr) {
            int numPeers = 0;

            try {
                while (destPeerIDs.hasMoreElements()) {
                    ID dest = destPeerIDs.nextElement();

                    try {
                        PeerConnection pConn = getPeerConnection(dest);

                        // TODO: make use of PeerView connections as well
                        if (null == pConn) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Sending " + msg + " (" + propHdr.getMsgId() + ") to " + dest);
                            }

                            EndpointAddress addr = mkAddress(dest, PropSName, PropPName);

                            Messenger messenger = rdvService.endpoint.getMessengerImmediate(addr, null);

                            if (null != messenger) {
                                try {
                                    messenger.sendMessage(msg);
                                } catch (IOException ignored) {
                                    continue;
                                }
                            } else {
                                continue;
                            }
                        } else {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Sending " + msg + " (" + propHdr.getMsgId() + ") to " + pConn);
                            }

                            if (pConn.isConnected()) {
                                pConn.sendMessage(msg.clone(), PropSName, PropPName);
                            } else {
                                continue;
                            }
                        }
                        numPeers++;
                    } catch (Exception failed) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Failed to send " + msg + " (" + propHdr.getMsgId() + ") to " + dest);
                        }
                    }
                }
            } finally {
                if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                    rendezvousMeter.propagateToPeers(numPeers);
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Propagated " + msg + " (" + propHdr.getMsgId() + ") to " + numPeers + " peers.");
                }
            }
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Declined to send " + msg + " ( no propHdr )");
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void propagateToNeighbors(Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException {
        msg = msg.clone();
        int useTTL = Math.min(initialTTL, MAX_TTL);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Propagating " + msg + "(TTL=" + useTTL + ") to neighbors to :" + "\n\tsvc name:" + serviceName+ "\tsvc params:" + serviceParam);
        }

        RendezVousPropagateMessage propHdr = updatePropHeader(msg, getPropHeader(msg), serviceName, serviceParam, useTTL);

        if (null != propHdr) {
            try {
                sendToNetwork(msg, propHdr);

                if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                    rendezvousMeter.propagateToNeighbors();
                }
            } catch (IOException failed) {
                if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                    rendezvousMeter.propagateToNeighborsFailed();
                }

                throw failed;
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void repropagate(Message msg, RendezVousPropagateMessage propHdr, String serviceName, String serviceParam) {
        msg = msg.clone();

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Repropagating " + msg + " (" + propHdr.getMsgId() + ")");
        }

        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
            rendezvousMeter.receivedMessageRepropagatedInGroup();
        }

        try {
            propHdr = updatePropHeader(msg, propHdr, serviceName, serviceParam, MAX_TTL);

            if (null != propHdr) {
                // Note (hamada): This is an unnecessary operation, and serves
                // no purpose other than the additional loads it imposes on the
                // rendezvous.  Local subnet network operations should be (and are)
                // sufficient to achieve the goal.
                // sendToEachConnection(msg, propHdr);
                sendToNetwork(msg, propHdr);
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("No propagate header, declining to repropagate " + msg + ")");
                }
            }
        } catch (Exception ez1) {
            // Not much we can do
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                if (propHdr != null) {
                    LOG.log(Level.WARNING, "Failed to repropagate " + msg + " (" + propHdr.getMsgId() + ")", ez1);
                } else {
                    LOG.log(Level.WARNING, "Could to repropagate " + msg, ez1);
                }
            }
        }
    }

    /**
     * Returns the peer connection or null if not present.
     *
     * @param id the node ID
     * @return PeerConnection the peer connection or null if not present.
     */
    public abstract PeerConnection getPeerConnection(ID id);

    /**
     * Returns an array of the current peer connections.
     *
     * @return An array of the current peer connections.
     */
    protected abstract PeerConnection[] getPeerConnections();

    /**
     * Sends to all connected peers.
     * <p/>
     * Note: The original msg is not modified and may be reused upon return.
     *
     * @param msg     The message to be sent.
     * @param propHdr The propagation header associated with the message.
     * @return the number of nodes the message was sent to
     */
    protected int sendToEachConnection(Message msg, RendezVousPropagateMessage propHdr) {
        List<PeerConnection> peers = Arrays.asList(getPeerConnections());
        int sentToPeers = 0;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending " + msg + "(" + propHdr.getMsgId() + ") to " + peers.size() + " peers.");
        }

        for (PeerConnection pConn : peers) {
            // Check if this rendezvous has already processed this propagated message.
            if (!pConn.isConnected()) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Skipping " + pConn + " for " + msg + "(" + propHdr.getMsgId() + ") -- disconnected.");
                }
                // next!
                continue;
            }

            if (propHdr.isVisited(pConn.getPeerID().toURI())) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Skipping " + pConn + " for " + msg + "(" + propHdr.getMsgId() + ") -- already visited.");
                }
                // next!
                continue;
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sending " + msg + "(" + propHdr.getMsgId() + ") to " + pConn);
            }

            if (pConn.sendMessage(msg.clone(), PropSName, PropPName)) {
                sentToPeers++;
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sent " + msg + "(" + propHdr.getMsgId() + ") to " + sentToPeers + " of " + peers.size() + " peers.");
        }

        return sentToPeers;
    }

    /**
     * Sends a disconnect message to the specified peer.
     *
     * @param peerid The peer to be disconnected.
     * @param padv   The peer to be disconnected.
     */
    protected void sendDisconnect(ID peerid, PeerAdvertisement padv) {

        Message msg = new Message();

        // The request simply includes the local peer advertisement.
        try {
            msg.replaceMessageElement("jxta", new TextDocumentMessageElement(DisconnectRequest, getPeerAdvertisementDoc(), null));

            EndpointAddress addr = mkAddress(peerid, null, null);

            RouteAdvertisement hint = null;

            if (null != padv) {
                hint = EndpointUtils.extractRouteAdv(padv);
            }

            Messenger messenger = rdvService.endpoint.getMessengerImmediate(addr, hint);

            if (null == messenger) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Could not get messenger for " + peerid);
                }
                return;
            }

            messenger.sendMessage(msg, pName, pParam);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "sendDisconnect failed", e);
            }
        }
    }

    /**
     * Sends a disconnect message to the specified peer.
     *
     * @param pConn The peer to be disconnected.
     */
    protected void sendDisconnect(PeerConnection pConn) {

        Message msg = new Message();

        // The request simply includes the local peer advertisement.
        try {
            msg.replaceMessageElement("jxta", new TextDocumentMessageElement(DisconnectRequest, getPeerAdvertisementDoc(), null));

            pConn.sendMessage(msg, pName, pParam);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "sendDisconnect failed", e);
            }
        }
    }
}
