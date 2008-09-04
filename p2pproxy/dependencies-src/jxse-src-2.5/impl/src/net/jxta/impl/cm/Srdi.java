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
package net.jxta.impl.cm;

import net.jxta.credential.Credential;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.protocol.ResolverSrdiMsgImpl;
import net.jxta.impl.protocol.SrdiMessageImpl;
import net.jxta.impl.util.JxtaHash;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.RdvAdvertisement;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverSrdiMsg;
import net.jxta.protocol.SrdiMessage;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.rendezvous.RendezVousStatus;
import net.jxta.rendezvous.RendezvousEvent;
import net.jxta.rendezvous.RendezvousListener;
import net.jxta.resolver.ResolverService;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.math.BigInteger;
import java.net.URI;
import java.net.URISyntaxException;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.Vector;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Srdi is a service which provides SRDI functionalities such as :
 * <p/>
 * <ul>
 * <li>pushing of SRDI messages to a another peer/propagate</li>
 * <li>replication of an SRDI Message to other peers in a given peerview</li>
 * <li>given an expression SRDI provides a independently calculated starting point</li>
 * <li>Forwarding a ResolverQuery, and taking care of hopCount, random selection</li>
 * <li>registers with the RendezvousService to determine when to share SrdSRDIi Entries</li>
 * and whether to push deltas, or full a index</li>
 * <li>provides a SrdiInterface giving to provide a generic SRDI message definition</li>
 * </ul>
 * <p/>
 * If Srdi is started as a thread it performs periodic SRDI pushes of
 * indices and also has the ability to respond to rendezvous events.
 * <p/>
 * ResolverSrdiMessages define a ttl, to indicate to the receiving service
 * whether to replicate such message or not.
 * <p/>
 * In addition A ResolverQuery defines a hopCount to indicate how many
 * hops a query has been forwarded. This element could be used to detect/stop a
 * query forward loopback hopCount is checked to make ensure a query is not
 * forwarded more than twice.
 *
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-prp" target="_blank">JXTA Protocols Specification : Peer Resolver Protocol</a>
 */
public class Srdi implements Runnable, RendezvousListener {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(Srdi.class.getName());

    private PeerGroup group = null;
    private String handlername = null;
    private SrdiInterface srdiService = null;
    private SrdiIndex srdiIndex;
    private long connectPollInterval = 0;
    private long pushInterval = 0;

    private volatile boolean stop = false;
    private AtomicBoolean republishSignal = new AtomicBoolean(false);

    private ResolverService resolver;
    private MembershipService membership;
    private final JxtaHash jxtaHash = new JxtaHash();
    private CredentialListener membershipCredListener = null;
    private Credential credential = null;
    private StructuredDocument credentialDoc = null;
    private final String rdvEventLock;

    /**
     * Random number generator used for random result selection
     */
    private final static Random random = new Random();

    // This ought be to configurable/based on a function applied to the rpv size
    /**
     * Replication threshold (minimum number of rdv's in peer view before replication)
     */
    public final static int RPV_REPLICATION_THRESHOLD = 3;

    /**
     * Listener we use for membership property events.
     */
    private class CredentialListener implements PropertyChangeListener {

        /**
         * {@inheritDoc}
         */
        public void propertyChange(PropertyChangeEvent evt) {
            if ("defaultCredential".equals(evt.getPropertyName())) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("New default credential event");
                }

                synchronized (Srdi.this) {
                    credential = (Credential) evt.getNewValue();
                    credentialDoc = null;
                    if (null != credential) {
                        try {
                            credentialDoc = credential.getDocument(MimeMediaType.XMLUTF8);
                        } catch (Exception all) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Could not generate credential document", all);
                            }
                        }
                    }
                }
            }
        }
    }


    /**
     * Interface for pushing entries.
     */
    public interface SrdiInterface {

        /**
         * Pushe SRDI entries.
         *
         * @param all if true then push all entries otherwise just push
         *            those which have changed since the last push.
         */
        void pushEntries(boolean all);
    }

    /**
     * Starts the Srdi Service. wait for connectPollInterval prior to
     * pushing the index if connected to a rdv, otherwise index is
     * as soon as the Rendezvous connect occurs
     *
     * @param group               group context to operate in
     * @param handlername         the SRDI handlername
     * @param srdiService         the service utilizing this Srdi, for purposes of
     *                            callback push entries on events such as rdv connect/disconnect, etc.
     * @param srdiIndex           The index instance associated with this service
     * @param connectPollInterval initial timeout before the very first push of entries in milliseconds
     * @param pushInterval        the Interval at which the deltas are pushed in milliseconds
     */
    public Srdi(PeerGroup group, String handlername, SrdiInterface srdiService, SrdiIndex srdiIndex, long connectPollInterval, long pushInterval) {

        this.group = group;
        this.handlername = handlername;
        this.srdiService = srdiService;
        this.srdiIndex = srdiIndex;
        this.connectPollInterval = connectPollInterval;
        this.pushInterval = pushInterval;
        this.rdvEventLock = new String(handlername);
        membership = group.getMembershipService();

        resolver = group.getResolverService();

        group.getRendezVousService().addListener(this);

        synchronized (this) {
            membershipCredListener = new CredentialListener();
            membership.addPropertyChangeListener("defaultCredential", membershipCredListener);

            try {
                credential = membership.getDefaultCredential();

                if (null != credential) {
                    credentialDoc = credential.getDocument(MimeMediaType.XMLUTF8);
                } else {
                    credentialDoc = null;
                }
            } catch (Exception all) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "could not get credential", all);
                }
            }
        }
    }

    /**
     * stop the current running thread
     */
    public synchronized void stop() {

        if (stop) {
            return;
        }

        stop = true;

        RendezVousService rendezvous = group.getRendezVousService();

        if (null != rendezvous) {
            rendezvous.removeListener(this);
        }

        membership.removePropertyChangeListener("defaultCredential", membershipCredListener);
        membershipCredListener = null;

        // wakeup and die
        synchronized (rdvEventLock) {
            rdvEventLock.notify();
        }
    }

    /**
     * Replicates a SRDI message to other rendezvous'
     * entries are replicated by breaking out entries out of the message
     * and sorted out into rdv distribution bins. after which smaller messages
     * are sent to other rdv's
     *
     * @param srdiMsg srdi message to replicate
     */

    public void replicateEntries(SrdiMessage srdiMsg) {

        List<PeerID> rpv = getGlobalPeerView();

        if (srdiMsg.getScope() < SrdiMessage.REPLICATE || !group.isRendezvous() || rpv.size() < RPV_REPLICATION_THRESHOLD) {
            return;
        }

        Iterator allEntries = srdiMsg.getEntries().iterator();
        Map<PeerID, SrdiMessageImpl> bins = new HashMap<PeerID, SrdiMessageImpl>(rpv.size());

        while (allEntries.hasNext()) {
            SrdiMessage.Entry entry = (SrdiMessage.Entry) allEntries.next();
            PeerID destPeer = getReplicaPeer(srdiMsg.getPrimaryKey() + entry.key + entry.value);

            if (destPeer == null || destPeer.equals(group.getPeerID())) {
                // don't replicate message back to ourselves
                continue;
            }
            SrdiMessageImpl sm = bins.get(destPeer);

            if (sm == null) {
                sm = new SrdiMessageImpl();
                sm.setPrimaryKey(srdiMsg.getPrimaryKey());
                sm.setPeerID(srdiMsg.getPeerID());
                bins.put(destPeer, sm);
            }
            sm.addEntry(entry);
        }

        for (PeerID destPeer : bins.keySet()) {
            SrdiMessageImpl msg = bins.get(destPeer);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("[" + group.getPeerGroupName() + " / " + handlername + "] Forwarding replica Srdi to " + destPeer);
            }
            pushSrdi(destPeer, msg);
        }
    }

    /**
     * Push an SRDI message to a peer
     * ttl is 1, and therefore services receiving this message could
     * choose to replicate this message
     *
     * @param peer peer to push message to, if peer is null it is
     *             the message is propagated
     * @param srdi SRDI message to send
     */
    public void pushSrdi(ID peer, SrdiMessage srdi) {
        try {
            ResolverSrdiMsg resSrdi = new ResolverSrdiMsgImpl(handlername, credential, srdi.toString());

            if (null == peer) {
                resolver.sendSrdi(null, resSrdi);
            } else {
                resolver.sendSrdi(peer.toString(), resSrdi);
            }
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to send srdi message", e);
            }
        }
    }

    /**
     * Forwards a Query to a specific peer
     * hopCount is incremented to indicate this query is forwarded
     *
     * @param peer  peerid to forward query to
     * @param query The query
     */
    public void forwardQuery(PeerID peer, ResolverQueryMsg query) {

        query.incrementHopCount();
        if (query.getHopCount() > 2) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("hopCount exceeded. Not forwarding query " + query.getHopCount());
            }
            // query has been forwarded too many times
            return;
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("[{0} / {1}] Forwarding Query to {2}",
                    group.getPeerGroupName(), handlername, peer));
        }
        resolver.sendQuery(peer.toString(), query);
    }

    /**
     * Forwards a Query to a list of peers
     * hopCount is incremented to indicate this query is forwarded
     *
     * @param peers The peerids to forward query to
     * @param query The query
     */
    public void forwardQuery(List<PeerID> peers, ResolverQueryMsg query) {

        query.incrementHopCount();
        if (query.getHopCount() > 2) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("hopCount exceeded not forwarding query {0}", query.getHopCount()));
            }
            // query has been forwarded too many times
            return;
        }

        for (PeerID destPeer : peers) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("[{0} / {1}] Forwarding Query to {2}",
                        group.getPeerGroupName(), handlername, destPeer));
            }
            resolver.sendQuery(destPeer.toString(), query);
        }
    }

    /**
     * Forwards a Query to a list of peers
     * if the list of peers exceeds threshold, and random threshold is picked
     * from <code>peers</code>
     * hopCount is incremented to indicate this query is forwarded
     *
     * @param peers     The peerids to forward query to
     * @param query     The query
     * @param threshold number of peers to forward the query to
     */
    public void forwardQuery(List<PeerID> peers, ResolverQueryMsg query, int threshold) {

        if (query.getHopCount() > 2) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("[{0} / {1}] hopCount exceeded ({2}) not forwarding query.",
                        group.getPeerGroupName(), handlername, query.getHopCount()));
            }
            // query has been forwarded too many times
            return;
        }
        if (peers.size() <= threshold) {
            forwardQuery(peers, query);
        } else {
            // pick some random entries out of the list
            List<PeerID> newPeers = randomResult(peers, threshold);
            forwardQuery(newPeers, query);
        }
    }

    /**
     * returns a random List(threshold) from a given list
     *
     * @param result    starting set
     * @param threshold sub-set desired
     * @return sub-list of result
     */
    protected List<PeerID> randomResult(List<PeerID> result, int threshold) {
        if (threshold < result.size()) {
            List<PeerID> res = new ArrayList<PeerID>(threshold);
            for (int i = 0; i < threshold; i++) {
                int rand = random.nextInt(result.size());
                res.add(result.get(rand));
                result.remove(rand);
            }
            return res;
        }
        return result;
    }

    /**
     * Given an expression return a peer from the list peers in the peerview
     * this function is used to to give a replication point, and entry point
     * to query on a pipe
     *
     * @param expression expression to derive the mapping from
     * @return The replicaPeer value
     */
    public PeerID getReplicaPeer(String expression) {
        PeerID pid;
        List<PeerID> rpv = getGlobalPeerView();

        if (rpv.size() >= RPV_REPLICATION_THRESHOLD) {
            BigInteger digest;

            synchronized (jxtaHash) {
                jxtaHash.update(expression);
                digest = jxtaHash.getDigestInteger().abs();
            }
            BigInteger sizeOfSpace = java.math.BigInteger.valueOf(rpv.size());
            BigInteger sizeOfHashSpace = BigInteger.ONE.shiftLeft(8 * digest.toByteArray().length);
            int pos = (digest.multiply(sizeOfSpace)).divide(sizeOfHashSpace).intValue();

            pid = rpv.get(pos);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("[{0} / {1}] Found a direct peer {2}", group.getPeerGroupName(), handlername, pid));
            }
            return pid;
        } else {
            return null;
        }
    }

    /**
     * forward srdi message to another peer
     *
     * @param peerid       PeerID to forward query to
     * @param srcPid       The source originator
     * @param primaryKey   primary key
     * @param secondarykey secondary key
     * @param value        value of the entry
     * @param expiration   expiration in ms
     */
    public void forwardSrdiMessage(PeerID peerid, PeerID srcPid, String primaryKey, String secondarykey, String value, long expiration) {

        try {
            SrdiMessageImpl srdi = new SrdiMessageImpl(srcPid, // ttl of 0, avoids additional replication
                    0, primaryKey, secondarykey, value, expiration);

            ResolverSrdiMsgImpl resSrdi = new ResolverSrdiMsgImpl(handlername, credential, srdi.toString());

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("[{0} / {1}] Forwarding a SRDI messsage of type {2} to {3}", group.getPeerGroupName(),
                                handlername, primaryKey, peerid));
            }
            resolver.sendSrdi(peerid.toString(), resSrdi);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed forwarding SRDI Message", e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("fallthrough")
    public void rendezvousEvent(RendezvousEvent event) {

        int theEventType = event.getType();

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("[{0} / {1}] Processing {2}", group.getPeerGroupName(), handlername, event));
        }

        switch (theEventType) {

            case RendezvousEvent.RDVCONNECT:
                synchronized (rdvEventLock) {
                    // wake up the publish thread now.
                    rdvEventLock.notify();
                }

               /*
                *  FALLSTHRU
                */
            case RendezvousEvent.RDVRECONNECT:
                // No need to wake up the publish thread; reconnect should not force indices to be published.
                break;

            case RendezvousEvent.CLIENTCONNECT:
            case RendezvousEvent.CLIENTRECONNECT:
            case RendezvousEvent.BECAMERDV:
            case RendezvousEvent.BECAMEEDGE:
                // XXX 20031110 bondolo perhaps becoming edge one should cause it to wake up so that run() switch to
                // don't do anything.
                break;

            case RendezvousEvent.RDVFAILED:
            case RendezvousEvent.RDVDISCONNECT:
                republishSignal.set(true);
                break;

            case RendezvousEvent.CLIENTFAILED:
            case RendezvousEvent.CLIENTDISCONNECT:
                // we should flush the cache for the peer
                synchronized (rdvEventLock) {
                    if (group.isRendezvous() && (srdiIndex != null)) {
                        srdiIndex.remove((PeerID) event.getPeerID());
                    }
                }
                break;

            default:
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning(
                            MessageFormat.format("[{0} / {1}] Unexpected RDV event {2}", group.getPeerGroupName(), handlername, event));
                }
                break;
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Main processing method for the SRDI Worker thread
     * Send all entries, wait for pushInterval, then send deltas
     */
    public void run() {

        boolean waitingForRdv;
        boolean republish = true;

        try {
            while (!stop) {
                // upon connection we will have to republish
                republish |= republishSignal.compareAndSet(true, false);
                waitingForRdv = group.isRendezvous() || !group.getRendezVousService().isConnectedToRendezVous() ||
                        group.getRendezVousService().getRendezVousStatus() == RendezVousStatus.ADHOC;

                if (!waitingForRdv) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("[" + group.getPeerGroupName() + " / " + handlername + "] Pushing "
                                + (republish ? "all entries" : "deltas"));
                    }

                    srdiService.pushEntries(republish);
                    republish = false;
                }

                synchronized (rdvEventLock) {
                    try {
                        rdvEventLock.wait(waitingForRdv ? connectPollInterval : pushInterval);
                    } catch (InterruptedException e) {
                        Thread.interrupted();
                    }
                }
            }
        } catch (Throwable all) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE,
                        "Uncaught Throwable in " + Thread.currentThread().getName() + "[" + group.getPeerGroupName() + " / "
                                + handlername + "]",all);
            }
        }
    }

    /**
     * get the global peerview as the rendezvous service only returns
     * the peerview without the local RDV peer.  We need this
     * consistent view for the SRDI index if not each RDV will have a
     * different peerview, off setting the index even when the peerview
     * is stable
     *
     * @return the sorted list
     */
    public Vector<PeerID> getGlobalPeerView() {

        Vector<PeerID> global = new Vector<PeerID>();
        SortedSet<String> set = new TreeSet<String>();

        try {
            // get the local peerview
            List<RdvAdvertisement> rpv = group.getRendezVousService().getLocalWalkView();

            for (RdvAdvertisement padv : rpv) {
                set.add(padv.getPeerID().toString());
            }

            // add myself
            set.add(group.getPeerID().toString());

            // produce a vector of Peer IDs
            for (String aSet : set) {
                try {
                    PeerID peerID = (PeerID) IDFactory.fromURI(new URI(aSet));
                    global.add(peerID);
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("Bad PeerID ID in advertisement");
                } catch (ClassCastException badID) {
                    throw new IllegalArgumentException("ID was not a peerID");
                }
            }
        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure generating the global view", ex);
            }
        }
        return global;
    }
}

