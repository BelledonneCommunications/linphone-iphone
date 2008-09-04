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
package net.jxta.impl.rendezvous.rdv;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.impl.protocol.RdvConfigAdv;
import net.jxta.impl.rendezvous.PeerConnection;
import net.jxta.impl.rendezvous.RdvWalk;
import net.jxta.impl.rendezvous.RdvWalker;
import net.jxta.impl.rendezvous.RendezVousPropagateMessage;
import net.jxta.impl.rendezvous.RendezVousServiceImpl;
import net.jxta.impl.rendezvous.StdRendezVousService;
import net.jxta.impl.rendezvous.limited.LimitedRangeWalk;
import net.jxta.impl.rendezvous.rendezvousMeter.ClientConnectionMeter;
import net.jxta.impl.rendezvous.rendezvousMeter.RendezvousMeterBuildSettings;
import net.jxta.impl.rendezvous.rpv.PeerView;
import net.jxta.impl.util.TimeUtils;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.platform.Module;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.rendezvous.RendezvousEvent;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TimerTask;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * A JXTA {@link net.jxta.rendezvous.RendezVousService} implementation which
 * implements the rendezvous server portion of the standard JXTA Rendezvous
 * Protocol (RVP).
 *
 * @see net.jxta.rendezvous.RendezVousService
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-rvp" target="_blank">JXTA Protocols Specification : Rendezvous Protocol</a>
 */
public class RdvPeerRdvService extends StdRendezVousService {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(RdvPeerRdvService.class.getName());

    public static final String RDV_WALK_SVC_NAME = "RdvWalkSvcName";
    public static final String RDV_WALK_SVC_PARAM = "RdvWalkSvcParam";

    public final static long GC_INTERVAL = 2 * TimeUtils.AMINUTE;
    public final static long DEFAULT_LEASE_DURATION = 20L * TimeUtils.AMINUTE;
    public final static int DEFAULT_MAX_CLIENTS = 200;

    /**
     * Duration of leases we offer measured in relative milliseconds.
     */
    private final long LEASE_DURATION;

    /**
     * The maximum number of simultaneous clients we will allow.
     */
    private final int MAX_CLIENTS;

    /**
     * The clients which currently have a lease with us.
     */
    private final Map<ID, ClientConnection> clients = Collections.synchronizedMap(new HashMap<ID, ClientConnection>());

    private RdvWalk walk = null;
    private RdvWalker walker = null;

    /**
     * The peer view for this rendezvous server.
     */
    public final PeerView rpv;

    /**
     * Constructor for the RdvPeerRdvService object
     *
     * @param group      the peer group
     * @param rdvService the rendezvous service object
     */
    public RdvPeerRdvService(PeerGroup group, RendezVousServiceImpl rdvService) {

        super(group, rdvService);

        Advertisement adv = null;
        ConfigParams confAdv = group.getConfigAdvertisement();

        // Get the config. If we do not have a config, we're done; we just keep
        // the defaults (edge peer/no auto-rdv)
        if (confAdv != null) {
            try {
                XMLDocument configDoc = (XMLDocument) confAdv.getServiceParam(rdvService.getAssignedID());

                if (null != configDoc) {
                    adv = AdvertisementFactory.newAdvertisement(configDoc);
                }
            } catch (java.util.NoSuchElementException failed) {// ignored
            }
        }

        RdvConfigAdv rdvConfigAdv;

        if (!(adv instanceof RdvConfigAdv)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Creating new RdvConfigAdv for defaults.");
            }

            rdvConfigAdv = (RdvConfigAdv) AdvertisementFactory.newAdvertisement(RdvConfigAdv.getAdvertisementType());
        } else {
            rdvConfigAdv = (RdvConfigAdv) adv;
        }

        if (rdvConfigAdv.getMaxTTL() > 0) {
            MAX_TTL = rdvConfigAdv.getMaxTTL();
        } else {
            MAX_TTL = StdRendezVousService.DEFAULT_MAX_TTL;
        }

        if (rdvConfigAdv.getMaxClients() > 0) {
            MAX_CLIENTS = rdvConfigAdv.getMaxClients();
        } else {
            MAX_CLIENTS = DEFAULT_MAX_CLIENTS;
        }

        if (rdvConfigAdv.getLeaseDuration() > 0) {
            LEASE_DURATION = rdvConfigAdv.getLeaseDuration();
        } else {
            LEASE_DURATION = DEFAULT_LEASE_DURATION;
        }

        // Update the peeradv with that information:
        // XXX 20050409 bondolo How does this interact with auto-rdv?
        try {
            XMLDocument params = (XMLDocument)
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");
            Element e = params.createElement("Rdv", Boolean.TRUE.toString());

            params.appendChild(e);
            group.getPeerAdvertisement().putServiceParam(rdvService.getAssignedID(), params);
        } catch (Exception ohwell) {
            // don't worry about it for now. It'll still work.
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed adding service params", ohwell);
            }
        }

        PeerGroup advGroup = group.getParentGroup();

        if ((null == advGroup) || PeerGroupID.worldPeerGroupID.equals(advGroup.getPeerGroupID())) {
            // For historical reasons, we publish in our own group rather than
            // the parent if our parent is the world group.
            advGroup = null;
        }

        rpv = new PeerView(group, advGroup, rdvService,
                rdvService.getAssignedID().toString() + group.getPeerGroupID().getUniqueValue().toString());

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("RendezVous Service is initialized for " + group.getPeerGroupID() + " as a Rendezvous peer.");
        }
    }

    /**
     * Listener for
     * <p/>
     * &lt;assignedID>/&lt;group-unique>
     */
    private class StdRdvRdvProtocolListener implements StdRendezVousService.StdRdvProtocolListener {

        /**
         * {@inheritDoc}
         */
        public void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("[" + group.getPeerGroupID() + "] processing " + msg);
            }

            if (msg.getMessageElement("jxta", ConnectRequest) != null) {
                processLeaseRequest(msg);
            }

            if (msg.getMessageElement("jxta", DisconnectRequest) != null) {
                processDisconnectRequest(msg);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected int startApp(String[] argv) {

        super.startApp(argv, new StdRdvRdvProtocolListener());

        rpv.start();

        // The other services may not be fully functional but they're there
        // so we can start our subsystems.
        // As for us, it does not matter if our methods are called between init
        // and startApp().

        // Start the Walk protcol. Create a LimitedRange Walk
        walk = new LimitedRangeWalk(group, new WalkListener(), pName, pParam, rpv);

        // We need to use a Walker in order to propagate the request
        // when when have no answer.
        walker = walk.getWalker();

        timer.scheduleAtFixedRate(new GCTask(), GC_INTERVAL, GC_INTERVAL);

        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
            rendezvousMeter.startRendezvous();
        }

        rdvService.generateEvent(RendezvousEvent.BECAMERDV, group.getPeerID());

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("RdvPeerRdvService is started");
        }

        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void stopApp() {

        if (closed) {
            return;
        }

        closed = true;

        walk.stop();
        walk = null;

        rpv.stop();

        // Tell all our clientss that we are going down
        disconnectAllClients();

        clients.clear();

        super.stopApp();

        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
            rendezvousMeter.stopRendezvous();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void connectToRendezVous(EndpointAddress addr, Object hint) {
        throw new UnsupportedOperationException("Not supported by rendezvous");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void challengeRendezVous(ID peer, long delay) {
        throw new UnsupportedOperationException("Not supported by rendezvous");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void disconnectFromRendezVous(ID peerId) {

        throw new UnsupportedOperationException("Not supported by rendezvous");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isConnectedToRendezVous() {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Vector<ID> getConnectedPeerIDs() {

        Vector<ID> result = new Vector<ID>();
        List allClients = Arrays.asList(clients.values().toArray());

        for (Object allClient : allClients) {
            PeerConnection aConnection = (PeerConnection) allClient;

            if (aConnection.isConnected()) {
                result.add(aConnection.getPeerID());
            }
        }
        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void propagate(Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException {
        if (closed) {
            return;
        }

        msg = msg.clone();
        int useTTL = Math.min(initialTTL, MAX_TTL);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Propagating " + msg + "(TTL=" + useTTL + ") to :" +
                    "\n\tsvc name:" + serviceName + "\tsvc params:" + serviceParam);
        }

        RendezVousPropagateMessage propHdr = updatePropHeader(msg, getPropHeader(msg), serviceName, serviceParam, useTTL);

        if (null != propHdr) {
            walk(msg, PropSName, PropPName, useTTL);
            // hamada: this is a very expensive operation and therefore not a supported operation
            // sendToEachConnection(msg, propHdr);
            sendToNetwork(msg, propHdr);

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.propagateToGroup();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void propagateInGroup(Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException {
        if (closed) {
            return;
        }

        msg = msg.clone();
        int useTTL = Math.min(initialTTL, MAX_TTL);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Propagating " + msg + "(TTL=" + useTTL + ") in group to :" +
                    "\n\tsvc name:" + serviceName + "\tsvc params:" + serviceParam);
        }

        RendezVousPropagateMessage propHdr = updatePropHeader(msg, getPropHeader(msg), serviceName, serviceParam, useTTL);

        if (null != propHdr) {
            walk(msg, PropSName, PropPName, useTTL);
            sendToEachConnection(msg, propHdr);

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.propagateToGroup();
            }
        }
    }

    /**
     * @inheritDoc
     */
    @Override
    public PeerConnection getPeerConnection(ID peer) {
        return clients.get(peer);
    }

    /**
     * @inheritDoc
     */
    @Override
    protected PeerConnection[] getPeerConnections() {
        return clients.values().toArray(new PeerConnection[0]);
    }

    /**
     * Add a client to our collection of clients.
     *
     * @param padv  The advertisement of the peer to be added.
     * @param lease The lease duration in relative milliseconds.
     * @return the ClientConnection
     */
    private ClientConnection addClient(PeerAdvertisement padv, long lease) {
        ClientConnectionMeter clientConnectionMeter = null;

        int eventType;
        ClientConnection pConn;

        synchronized (clients) {
            pConn = clients.get(padv.getPeerID());

            // Check if the peer is already registered.
            if (null != pConn) {
                eventType = RendezvousEvent.CLIENTRECONNECT;
            } else {
                eventType = RendezvousEvent.CLIENTCONNECT;
                pConn = new ClientConnection(group, rdvService, padv.getPeerID());
                clients.put(padv.getPeerID(), pConn);
            }
        }

        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousServiceMonitor != null)) {
            clientConnectionMeter = rendezvousServiceMonitor.getClientConnectionMeter(padv.getPeerID());
        }

        if (RendezvousEvent.CLIENTCONNECT == eventType) {
            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (clientConnectionMeter != null)) {
                clientConnectionMeter.clientConnectionEstablished(lease);
            }
        } else {
            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (clientConnectionMeter != null)) {
                clientConnectionMeter.clientLeaseRenewed(lease);
            }
        }

        rdvService.generateEvent(eventType, padv.getPeerID());

        pConn.connect(padv, lease);

        return pConn;
    }

    /**
     * Removes the specified client from the clients collections.
     *
     * @param pConn     The connection object to remove.
     * @param requested If <code>true</code> then the disconnection was
     *                  requested by the remote peer.
     * @return the ClientConnection object of the client or <code>null</code>
     *         if the client was not known.
     */
    private ClientConnection removeClient(PeerConnection pConn, boolean requested) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Disconnecting client " + pConn);
        }

        if (pConn.isConnected()) {
            pConn.setConnected(false);
            sendDisconnect(pConn);
        }

        rdvService.generateEvent(requested ? RendezvousEvent.CLIENTDISCONNECT : RendezvousEvent.CLIENTFAILED, pConn.getPeerID());

        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousServiceMonitor != null)) {
            ClientConnectionMeter clientConnectionMeter = rendezvousServiceMonitor.getClientConnectionMeter(
                    (PeerID) pConn.getPeerID());

            clientConnectionMeter.clientConnectionDisconnected(requested);
        }

        return clients.remove(pConn.getPeerID());
    }

    private void disconnectAllClients() {
        for (Object o : Arrays.asList(clients.values().toArray())) {
            ClientConnection pConn = (ClientConnection) o;

            try {
                removeClient(pConn, false);
            } catch (Exception ez1) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "disconnectClient failed for" + pConn, ez1);
                }
            }
        }
    }

    /**
     * Handle a disconnection request
     *
     * @param msg Message containing the disconnection request.
     */
    private void processDisconnectRequest(Message msg) {

        PeerAdvertisement adv;

        try {
            MessageElement elem = msg.getMessageElement("jxta", DisconnectRequest);

            XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(elem);

            adv = (PeerAdvertisement) AdvertisementFactory.newAdvertisement(asDoc);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not process disconnect request", e);
            }
            return;
        }

        ClientConnection pConn = clients.get(adv.getPeerID());

        if (null != pConn) {
            pConn.setConnected(false); // Make sure we don't send a disconnect
            removeClient(pConn, true);
        }
    }

    /**
     * Handles a lease request message
     *
     * @param msg Message containing the lease request
     */
    private void processLeaseRequest(Message msg) {

        PeerAdvertisement padv;

        try {
            MessageElement elem = msg.getMessageElement("jxta", ConnectRequest);

            XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(elem);

            padv = (PeerAdvertisement) AdvertisementFactory.newAdvertisement(asDoc);
            msg.removeMessageElement(elem);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Cannot retrieve advertisment from lease request", e);
            }
            return;
        }

        // Publish the client's peer advertisement
        try {
            // This is not our own peer adv so we must not keep it longer than
            // its expiration time.
            DiscoveryService discovery = group.getDiscoveryService();

            if (null != discovery) {
                discovery.publish(padv, LEASE_DURATION * 2, 0);
            }
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Client peer advertisement publish failed", e);
            }
        }

        long lease;

        ClientConnection pConn = clients.get(padv.getPeerID());

        if (null != pConn) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Renewing client lease to " + pConn);
            }

            lease = LEASE_DURATION;
        } else {
            if (clients.size() < MAX_CLIENTS) {
                lease = LEASE_DURATION;

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Offering new client lease to " + padv.getName() + " [" + padv.getPeerID() + "]");
                }
            } else {
                lease = 0;

                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning(
                            "Max clients exceeded, declining lease request from: " + padv.getName() + " [" + padv.getPeerID()
                                    + "]");
                }
            }
        }

        if (lease > 0) {
            pConn = addClient(padv, lease);

            // FIXME 20041015 bondolo We're supposed to send a lease 0 if we can't accept new clients.
            sendLease(pConn, lease);
        }
    }

    /**
     * Sends a Connected lease reply message to the specified peer
     *
     * @param pConn The client peer.
     * @param lease lease duration.
     * @return Description of the Returned Value
     */
    private boolean sendLease(ClientConnection pConn, long lease) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending lease (" + lease + ") to " + pConn.getPeerName());
        }

        Message msg = new Message();

        msg.addMessageElement("jxta", new TextDocumentMessageElement(ConnectedRdvAdvReply, getPeerAdvertisementDoc(), null));

        msg.addMessageElement("jxta", new StringMessageElement(ConnectedPeerReply, group.getPeerID().toString(), null));

        msg.addMessageElement("jxta", new StringMessageElement(ConnectedLeaseReply, Long.toString(lease), null));

        return pConn.sendMessage(msg, pName, pParam);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void walk(Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException {
        if (closed) {
            return;
        }

        msg = msg.clone();
        int useTTL = Math.min(initialTTL, MAX_TTL);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(
                    "Undirected walk of " + msg + "(TTL=" + useTTL + ") to :" + "\n\tsvc name:" + serviceName + "\tsvc params:"
                            + serviceParam);
        }

        msg.replaceMessageElement("jxta", new StringMessageElement(RDV_WALK_SVC_NAME, serviceName, null));

        msg.replaceMessageElement("jxta", new StringMessageElement(RDV_WALK_SVC_PARAM, serviceParam, null));

        try {
            walker.walkMessage(null, msg, pName, pParam, useTTL);

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.walk();
            }
        } catch (IOException failure) {
            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.walkFailed();
            }

            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Cannot send message with Walker", failure);
            }

            throw failure;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void walk(Vector<? extends ID> destPeerIDs, Message msg, String serviceName, String serviceParam, int initialTTL) throws IOException {
        if (closed) {
            return;
        }

        msg = msg.clone();
        int useTTL = Math.min(initialTTL, MAX_TTL);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(
                    "Directed walk of " + msg + "(TTL=" + useTTL + ") to :" + "\n\tsvc name:" + serviceName + "\tsvc params:"
                            + serviceParam);
        }

        msg.replaceMessageElement("jxta", new StringMessageElement(RDV_WALK_SVC_NAME, serviceName, null));

        msg.replaceMessageElement("jxta", new StringMessageElement(RDV_WALK_SVC_PARAM, serviceParam, null));

        for (ID destPeerID : destPeerIDs) {
            try {
                walker.walkMessage((PeerID) destPeerID, msg.clone(), pName, pParam, useTTL);
            } catch (IOException failed) {
                if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                    rendezvousMeter.walkToPeersFailed();
                }

                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Cannot send message with Walker to: " + destPeerID, failed);
                }

                IOException failure = new IOException("Cannot send message with Walker to: " + destPeerID);

                failure.initCause(failed);

                throw failure;
            }
        }
        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
            rendezvousMeter.walkToPeers(destPeerIDs.size());
        }
    }

    /**
     * Periodic cleanup task
     */
    private class GCTask extends TimerTask {

        /**
         * {@inheritDoc
         */
        @Override
        public void run() {

            try {
                long gcStart = TimeUtils.timeNow();
                int gcedClients = 0;

                List allClients = Arrays.asList(clients.values().toArray());

                for (Object allClient : allClients) {
                    ClientConnection pConn = (ClientConnection) allClient;

                    try {
                        long now = TimeUtils.timeNow();

                        if (!pConn.isConnected() || (pConn.getLeaseEnd() < now)) {
                            // This client has dropped out or the lease is over.
                            // remove it.

                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("GC CLIENT: dropping " + pConn);
                            }

                            pConn.setConnected(false);
                            removeClient(pConn, false);
                            gcedClients++;
                        }
                    } catch (Exception e) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "GCTask failed for " + pConn, e);
                        }
                    }
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine(
                            "Client GC " + gcedClients + " of " + allClients.size() + " clients completed in "
                                    + TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), gcStart) + "ms.");
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
                }
            }
        }
    }


    /**
     * @inheritDoc
     */
    private class WalkListener implements EndpointListener {

        /**
         * {@inheritDoc}
         */
        public void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {

            MessageElement serviceME = msg.getMessageElement("jxta", RDV_WALK_SVC_NAME);

            if (null == serviceME) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Discarding " + msg + " because its missing service name element");
                }
                return;
            }

            msg.removeMessageElement(serviceME);
            String sName = serviceME.toString();

            MessageElement paramME = msg.getMessageElement("jxta", RDV_WALK_SVC_PARAM);

            String sParam;

            if (null == paramME) {
                sParam = null;
            } else {
                msg.removeMessageElement(paramME);
                sParam = paramME.toString();
            }

            EndpointAddress realDest = new EndpointAddress(dstAddr, sName, sParam);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        "Calling local listener for [" + realDest.getServiceName() + " / " + realDest.getServiceParameter()
                                + "] with " + msg);
            }

            rdvService.endpoint.processIncomingMessage(msg, srcAddr, realDest);

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING && (rendezvousMeter != null)) {
                rendezvousMeter.receivedMessageProcessedLocally();
            }
        }
    }
}
