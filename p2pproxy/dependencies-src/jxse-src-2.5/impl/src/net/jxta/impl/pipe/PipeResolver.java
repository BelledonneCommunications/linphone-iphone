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
package net.jxta.impl.pipe;

import net.jxta.credential.Credential;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.OutgoingMessageEvent;
import net.jxta.id.ID;
import net.jxta.impl.cm.Srdi;
import net.jxta.impl.cm.Srdi.SrdiInterface;
import net.jxta.impl.cm.SrdiIndex;
import net.jxta.impl.protocol.PipeResolverMsg;
import net.jxta.impl.protocol.ResolverQuery;
import net.jxta.impl.protocol.SrdiMessageImpl;
import net.jxta.impl.resolver.InternalQueryHandler;
import net.jxta.impl.util.TimeUtils;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.PipeID;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.protocol.PipeResolverMessage;
import net.jxta.protocol.PipeResolverMessage.MessageType;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.ResolverSrdiMsg;
import net.jxta.protocol.SrdiMessage;
import net.jxta.protocol.SrdiMessage.Entry;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.rendezvous.RendezVousStatus;
import net.jxta.resolver.ResolverService;
import net.jxta.resolver.SrdiHandler;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.EventListener;
import java.util.EventObject;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * This class implements the Resolver interfaces for a PipeServiceImpl.
 */
class PipeResolver implements SrdiInterface, InternalQueryHandler, SrdiHandler, PipeRegistrar {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(PipeResolver.class.getName());

    private final static String PipeResolverName = "JxtaPipeResolver";
    private final static String srdiIndexerFileName = "pipeResolverSrdi";

    /**
     * Local SRDI GC Interval
     */
    private final static long GcDelay = 1 * TimeUtils.AMINUTE;

    /**
     * Constant for pipe event listeners to signify any query id.
     */
    final static int ANYQUERY = 0;

    /**
     * The current query ID. The next value returned by {@link #getNextQueryID()}
     * will be one greater than this value.
     */
    private static int currentQueryID = 1;

    /**
     * Group we are working for
     */
    private PeerGroup myGroup = null;
    /**
     * Group we are working for
     */
    private EndpointService endpoint = null;

    /**
     * Resolver Service we will register with
     */
    private ResolverService resolver = null;

    /**
     * The discovery service we will use
     */
    private DiscoveryService discovery = null;

    /**
     * Membership Service we will use
     */
    private MembershipService membership = null;

    private Srdi srdi = null;
    private Thread srdiThread = null;
    private SrdiIndex srdiIndex = null;
    private RendezVousService rendezvous = null;

    /**
     * The locally registered {@link net.jxta.pipe.InputPipe}s
     */
    private final Map<ID, InputPipe> localInputPipes = new HashMap<ID, InputPipe>();

    /**
     * Registered listeners for pipe events.
     */
    private final Map<ID, Map<Integer, Listener>> outputpipeListeners = new HashMap<ID, Map<Integer, Listener>>();

    /**
     * Encapsulates current Membership Service credential.
     */
    final static class CurrentCredential {
        /**
         * The current default credential
         */
        final Credential credential;

        /**
         * The current default credential in serialized XML form.
         */
        final XMLDocument credentialDoc;

        CurrentCredential(Credential credential, XMLDocument credentialDoc) {
            this.credential = credential;
            this.credentialDoc = credentialDoc;
        }
    }

    /**
     * The current Membership service default credential.
     */
    CurrentCredential currentCredential;

    /**
     * Listener we use for membership property events.
     */
    private class CredentialListener implements PropertyChangeListener {

        /**
         * Standard Constructor
         */
        CredentialListener() {}

        /**
         * {@inheritDoc}
         */
        public void propertyChange(PropertyChangeEvent evt) {
            if (MembershipService.DEFAULT_CREDENTIAL_PROPERTY.equals(evt.getPropertyName())) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("New default credential event");
                }

                synchronized (PipeResolver.this) {
                    Credential cred = (Credential) evt.getNewValue();
                    XMLDocument credentialDoc;

                    if (null != cred) {
                        try {
                            credentialDoc = (XMLDocument) cred.getDocument(MimeMediaType.XMLUTF8);
                            currentCredential = new CurrentCredential(cred, credentialDoc);
                        } catch (Exception all) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Could not generate credential document", all);
                            }
                            currentCredential = null;
                        }
                    } else {
                        currentCredential = null;
                    }
                }
            }
        }
    }

    final CredentialListener membershipCredListener = new CredentialListener();

    /**
     * A pipe resolver event.
     */
    static class Event extends EventObject {

        private final ID peerid;
        private final ID pipeid;
        private final String type;
        private final int queryID;

        /**
         * Creates a new pipe resolution event
         *
         * @param source  The PipeResolver generating the event.
         * @param peerid  The peer on which the pipe was found
         * @param pipeid  the pipe which was found
         * @param type    the type of pipe which was found
         * @param queryid The query id associated with the response returned in this event
         */
        public Event(PipeResolver source, ID peerid, ID pipeid, String type, int queryid) {
            super(source);
            this.peerid = peerid;
            this.pipeid = pipeid;
            this.type = type;
            this.queryID = queryid;
        }

        /**
         * Returns the peer associated with the event
         *
         * @return peerid
         */
        public ID getPeerID() {
            return peerid;
        }

        /**
         * Returns the pipe associated with the event
         *
         * @return pipeid
         */
        public ID getPipeID() {
            return pipeid;
        }

        /**
         * Returns the type of the pipe that is associated with the event
         *
         * @return type
         */
        public String getType() {
            return type;
        }

        /**
         * Returns The query id associated with the response returned in this event
         *
         * @return query id associated with the response
         */
        public int getQueryID() {
            return queryID;
        }
    }


    /**
     * Pipe Resolver Event Listener. Implement this interface is you wish to
     * Receive Pipe Resolver events.
     */
    interface Listener extends EventListener {
        /**
         * Pipe Resolve event
         *
         * @param event event the PipeResolver Event
         * @return true if the event was handled otherwise false
         */
        boolean pipeResolveEvent(Event event);

        /**
         * A NAK Event was received for this pipe
         *
         * @param event event the PipeResolver Event
         * @return true if the event was handled otherwise false
         */
        boolean pipeNAKEvent(Event event);
    }

    /**
     * return the next query id.
     *
     * @return the next eligible query id.
     */
    static synchronized int getNextQueryID() {
        currentQueryID++;
        if (currentQueryID == Integer.MAX_VALUE) {
            currentQueryID = 1;
        }
        return currentQueryID;
    }

    /**
     * Constructor for the PipeResolver object
     *
     * @param peerGroup group for which this PipeResolver operates in
     */
    PipeResolver(PeerGroup peerGroup) {

        myGroup = peerGroup;
        resolver = myGroup.getResolverService();
        membership = myGroup.getMembershipService();
        rendezvous = myGroup.getRendezVousService();
        endpoint = myGroup.getEndpointService();

        // Register to the Generic ResolverServiceImpl
        resolver.registerHandler(PipeResolverName, this);

        // start srdi
        srdiIndex = new SrdiIndex(myGroup, srdiIndexerFileName, GcDelay);

        srdi = new Srdi(myGroup, PipeResolverName, this, srdiIndex, 2 * TimeUtils.AMINUTE, 1 * TimeUtils.AYEAR);
        srdiThread = new Thread(myGroup.getHomeThreadGroup(), srdi, "Pipe Resolver Srdi Thread");
        srdiThread.setDaemon(true);
        srdiThread.start();

        resolver.registerSrdiHandler(PipeResolverName, this);
        synchronized (this) {
            // register our credential listener.
            membership.addPropertyChangeListener(MembershipService.DEFAULT_CREDENTIAL_PROPERTY, membershipCredListener);
            try {
                // set the initial version of the default credential.
                currentCredential = null;
                Credential credential = membership.getDefaultCredential();
                XMLDocument credentialDoc;

                if (null != credential) {
                    credentialDoc = (XMLDocument) credential.getDocument(MimeMediaType.XMLUTF8);
                    currentCredential = new CurrentCredential(credential, credentialDoc);
                }
            } catch (Exception all) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "could not get default credential", all);
                }
            }
        }
    }

    private boolean isRendezvous() {
        if (rendezvous == null) {
            rendezvous = myGroup.getRendezVousService();
        }
        RendezVousStatus mode = rendezvous.getRendezVousStatus();
        return (mode == RendezVousStatus.RENDEZVOUS ||mode == RendezVousStatus.AUTO_RENDEZVOUS);
    }

    /**
     * {@inheritDoc}
     */
    public int processQuery(ResolverQueryMsg query) {
        return processQuery(query, null);
    }

    /**
     * {@inheritDoc}
     */
    public int processQuery(ResolverQueryMsg query, EndpointAddress srcAddr) {

        String queryFrom;

        if (null != srcAddr) {
            if ("jxta".equals(srcAddr.getProtocolName())) {
                queryFrom = ID.URIEncodingName + ":" + ID.URNNamespace + ":" + srcAddr.getProtocolAddress();
            } else {
                // we don't know who routed us the query. Assume it came from the source.
                queryFrom = query.getSrcPeer().toString();
            }
        } else {
            // we don't know who routed us the query. Assume it came from the source.
            queryFrom = query.getSrcPeer().toString();
        }

        String responseDest = query.getSrcPeer().toString();
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Starting for :" + query.getQueryId() + " from " + srcAddr);
        }

        Reader queryReader = new StringReader(query.getQuery());
        StructuredTextDocument doc = null;
        try {
            doc = (StructuredTextDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, queryReader);
        } catch (IOException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "discarding malformed request ", e);
            }
            // no sense in re-propagation here
            return ResolverService.OK;
        } finally {
            try {
                queryReader.close();
            } catch (IOException ignored) {
                // ignored
            }
            queryReader = null;
        }

        PipeResolverMessage pipeQuery;
        try {
            pipeQuery = new PipeResolverMsg(doc);
        } catch (IllegalArgumentException badDoc) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "discarding malformed request ", badDoc);
            }
            // no sense in re-propagation here
            return ResolverService.OK;
        } finally {
            doc = null;
        }

        // is it a query?
        if (!pipeQuery.getMsgType().equals(MessageType.QUERY)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("expected query - discarding.");
            }
            // no sense in re-propagation here
            return ResolverService.OK;
        }

        // see if it is a query directed at our peer.
        Set<ID> destPeers = pipeQuery.getPeerIDs();
        boolean directedQuery = !destPeers.isEmpty();
        boolean queryForMe = !directedQuery;

        if (directedQuery) {
            for (Object destPeer : destPeers) {
                ID aPeer = (ID) destPeer;
                if (aPeer.equals(myGroup.getPeerID())) {
                    queryForMe = true;
                    break;
                }
            }

            if (!queryForMe) {
                // It is an directed query, but request wasn't for this peer.
                if (query.getSrcPeer().toString().equals(queryFrom)) {
                    // we only respond if the original src was not the query forwarder
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("discarding query. Query not for us.");
                    }
                    // tell the resolver no further action is needed.
                    return ResolverService.OK;
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("responding to \'misdirected\' forwarded query.");
                }
                responseDest = queryFrom;
            }
        }

        PeerID peerID = null;
        if (queryForMe) {
            // look locally.
            InputPipe ip = findLocal((PipeID) pipeQuery.getPipeID());
            if ((ip != null) && (ip.getType().equals(pipeQuery.getPipeType()))) {
                peerID = myGroup.getPeerID();
            }
        }

        if ((null == peerID) && !directedQuery) {
            // This request was sent to everyone.
            if (myGroup.isRendezvous()) {
                // We are a RDV, allow the ResolverService to repropagate the query
                List<PeerID> results = srdiIndex.query(pipeQuery.getPipeType(), PipeAdvertisement.IdTag,
                        pipeQuery.getPipeID().toString(), 20);

                if (!results.isEmpty()) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("forwarding query to " + results.size() + " peers");
                    }
                    srdi.forwardQuery(results, query);
                    // tell the resolver no further action is needed.
                    return ResolverService.OK;
                }

                // we don't know anything, continue the walk.
                return ResolverService.Repropagate;
            } else {
                // We are an edge
                if (query.getSrcPeer().toString().equals(queryFrom)) {
                    // we only respond if the original src was not the query forwarder
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("discarding query.");
                    }

                    // tell the resolver no further action is needed.
                    return ResolverService.OK;
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("responding to query forwarded for \'misdirected\' query.");
                }
                responseDest = queryFrom;
            }
        }

        // Build the answer
        PipeResolverMessage pipeResp = new PipeResolverMsg();

        pipeResp.setMsgType(MessageType.ANSWER);
        pipeResp.setPipeID(pipeQuery.getPipeID());
        pipeResp.setPipeType(pipeQuery.getPipeType());
        if (null == peerID) {
            // respond negative.
            pipeResp.addPeerID(myGroup.getPeerID());
            pipeResp.setFound(false);
        } else {
            pipeResp.addPeerID(peerID);
            pipeResp.setFound(true);
            pipeResp.setInputPeerAdv(myGroup.getPeerAdvertisement());
        }

        // make a response from the incoming query
        ResolverResponseMsg res = query.makeResponse();

        CurrentCredential current = currentCredential;
        if (null != current) {
            res.setCredential(current.credentialDoc);
        }
        res.setResponse(pipeResp.getDocument(MimeMediaType.XMLUTF8).toString());

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending answer for query \'" + query.getQueryId() + "\' to : " + responseDest);
        }
        resolver.sendResponse(responseDest, res);
        return ResolverService.OK;
    }

    /**
     * {@inheritDoc}
     */
    public void processResponse(ResolverResponseMsg response) {
        processResponse(response, null);
    }

    /**
     * {@inheritDoc}
     */
    public void processResponse(ResolverResponseMsg response, EndpointAddress srcAddr) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("got a response for \'" + response.getQueryId() + "\'");
        }

        Reader resp = new StringReader(response.getResponse());
        StructuredTextDocument doc = null;
        try {
            doc = (StructuredTextDocument)
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, resp);
        } catch (Throwable e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "malformed response - discard", e);
            }
            return;
        } finally {
            try {
                resp.close();
            } catch (IOException ignored) {// ignored
            }
            resp = null;
        }

        PipeResolverMessage pipeResp;

        try {
            pipeResp = new PipeResolverMsg(doc);
        } catch (Throwable caught) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "malformed response - discarding.", caught);
            }
            return;
        } finally {
            doc = null;
        }

        // check if it's a response.
        if (!pipeResp.getMsgType().equals(MessageType.ANSWER)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("expected response - discarding.");
            }
            return;
        }

        PeerAdvertisement padv = pipeResp.getInputPeerAdv();

        if ((null != padv) && !(myGroup.getPeerID().equals(padv.getPeerID()))) {
            try {
                // This is not our own peer adv so we keep it only for the default
                // expiration time.
                if (null == discovery) {
                    discovery = myGroup.getDiscoveryService();
                }
                if (null != discovery) {
                    discovery.publish(padv, DiscoveryService.DEFAULT_EXPIRATION, DiscoveryService.DEFAULT_EXPIRATION);
                }
            } catch (IOException ignored) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("could not publish peer adv");
                }
            }
        }

        String ipId = pipeResp.getPipeID().toString();

        Set<ID> peerRsps = pipeResp.getPeerIDs();

        for (Object peerRsp : peerRsps) {
            // process each peer for which this response is about.
            PeerID peer = (PeerID) peerRsp;
            if (!pipeResp.isFound()) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("NACK for pipe \'" + ipId + "\' from peer " + peer);
                }

                // We have received a NACK. Remove that entry.
                srdiIndex.add(pipeResp.getPipeType(), PipeAdvertisement.IdTag, ipId, peer, 0);
            } else {
                long exp = getEntryExp(pipeResp.getPipeType(), PipeAdvertisement.IdTag, ipId, peer);
                if ((PipeServiceImpl.VERIFYINTERVAL / 2) > exp) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Using Expiration " + (PipeServiceImpl.VERIFYINTERVAL / 2) + " which is > " + exp);
                    }
                    // create antry only if one does not exist,or entry exists with
                    // lesser lifetime
                    // cache the result for half the verify interval
                    srdiIndex.add(pipeResp.getPipeType(), PipeAdvertisement.IdTag, ipId, peer,
                            (PipeServiceImpl.VERIFYINTERVAL / 2));
                } else {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("DB Expiration " + exp + " > " + (PipeServiceImpl.VERIFYINTERVAL / 2)
                                + " overriding attempt to decrease lifetime");
                    }
                }
            }

            // call listener for pipeid
            callListener(response.getQueryId(), pipeResp.getPipeID(), pipeResp.getPipeType(), peer, !pipeResp.isFound());
        }
    }

    private long getEntryExp(String pkey, String skey, String value, PeerID peerid) {
        List<SrdiIndex.Entry> list = srdiIndex.getRecord(pkey, skey, value);

        for (SrdiIndex.Entry entry : list) {
            if (entry.peerid.equals(peerid)) {
                // exp in millis
                return TimeUtils.toRelativeTimeMillis(entry.expiration);
            }
        }
        return -1;
    }

    /**
     * {@inheritDoc}
     */
    public boolean processSrdi(ResolverSrdiMsg message) {

        if (!isRendezvous()) {
            // avoid caching in non rendezvous mode
            return true;
        }
        if (message == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("no SRDI message");
            }
            return false;
        }

        if (message.getPayload() == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("empty SRDI message");
            }
            return false;
        }

        SrdiMessage srdiMsg;
        try {
            StructuredTextDocument asDoc = (StructuredTextDocument)
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, new StringReader(message.getPayload()));
            srdiMsg = new SrdiMessageImpl(asDoc);
        } catch (Throwable e) {
            // we don't understand this msg, let's skip it
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Invalid SRDI message", e);
            }
            return false;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Received an SRDI messsage with " + srdiMsg.getEntries().size() + " entries from " + srdiMsg.getPeerID());
        }

        for (Object o : srdiMsg.getEntries()) {
            Entry entry = (Entry) o;
            srdiIndex.add(srdiMsg.getPrimaryKey(), entry.key, entry.value, srdiMsg.getPeerID(), entry.expiration);
        }

        if (!PipeService.PropagateType.equals(srdiMsg.getPrimaryKey())) {
            // don't replicate entries for propagate pipes. For unicast type
            // pipes the replica is useful in finding pipe instances. Since
            // walking rather than searching is done for propagate pipes this
            // appropriate.
            srdi.replicateEntries(srdiMsg);
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    public void messageSendFailed(PeerID peerid, OutgoingMessageEvent e) {// so what.
    }

    /**
     * {@inheritDoc}
     */
    public void pushEntries(boolean all) {
        pushSrdi((PeerID) null, all);
    }

    /**
     * unregisters the resolver handler
     */
    void stop() {

        resolver.unregisterHandler(PipeResolverName);
        resolver.unregisterSrdiHandler(PipeResolverName);

        srdiIndex.stop();
        srdiIndex = null;
        // stop the srdi thread
        if (srdiThread != null) {
            srdi.stop();
        }
        srdiThread = null;
        srdi = null;

        membership.removePropertyChangeListener("defaultCredential", membershipCredListener);
        currentCredential = null;

        // Avoid cross-reference problems with GC
        myGroup = null;
        resolver = null;
        discovery = null;
        membership = null;

        outputpipeListeners.clear();

        // close the local pipes
        List<InputPipe> openLocalPipes = new ArrayList<InputPipe>(localInputPipes.values());
        for (InputPipe aPipe : openLocalPipes) {
            try {
                aPipe.close();
            } catch (Exception failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Failure closing " + aPipe);
                }
            }
        }

        localInputPipes.clear();
    }

    /**
     * {@inheritDoc}
     */
    public boolean register(InputPipe ip) {

        PipeID pipeID = (PipeID) ip.getPipeID();
        synchronized (this) {
            if (localInputPipes.containsKey(pipeID)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Existing registered InputPipe for " + pipeID);
                }
                return false;
            }

            // Register this input pipe
            boolean registered = endpoint.addIncomingMessageListener((EndpointListener) ip, "PipeService", pipeID.toString());
            if (!registered) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe("Existing registered Endpoint Listener for " + pipeID);
                }
                return false;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Registering local InputPipe for " + pipeID);
            }
            localInputPipes.put(pipeID, ip);
        }

        // Announce the pipe to SRDI so that others will know we are listening.
        pushSrdi(ip, true);

        // Call anyone who may be listening for this input pipe.
        callListener(0, pipeID, ip.getType(), myGroup.getPeerID(), false);
        return true;
    }

    /**
     * Return the local {@link net.jxta.pipe.InputPipe InputPipe}, if any, for the
     * specified {@link net.jxta.pipe.PipeID PipeID}.
     *
     * @param pipeID the PipeID who's InputPipe is desired.
     * @return The InputPipe object.
     */
    public InputPipe findLocal(PipeID pipeID) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Find local InputPipe for " + pipeID);
        }

        // First look if the pipe is a local InputPipe
        InputPipe ip = localInputPipes.get(pipeID);
        // Found it.
        if ((null != ip) && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("found local InputPipe for " + pipeID);
        }
        return ip;
    }

    /**
     * {@inheritDoc}
     */
    public boolean forget(InputPipe pipe) {

        PipeID pipeID = (PipeID) pipe.getPipeID();
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Unregistering local InputPipe for " + pipeID);
        }

        // Unconditionally announce the change to SRDI.
        // FIXME 20040529 bondolo This is overkill, it should be able to wait
        // until the deltas are pushed.
        pushSrdi(pipe, false);

        InputPipe ip;

        synchronized (this) {
            ip = localInputPipes.remove(pipeID);
        }

        if (pipe != ip) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Pipe removed was not the same as the pipe to be removed!");
            }
        }

        if (null != ip) {
            // remove the queue for the general demux
            EndpointListener removed = endpoint.removeIncomingMessageListener("PipeService", pipeID.toString());

            if ((null == removed) || (pipe != removed)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("removeIncomingMessageListener() did not remove correct pipe!");
                }
            }
        }

        return (ip != null);
    }

    /**
     * Add a pipe resolver listener
     *
     * @param listener listener
     * @param queryID    The query this callback is being made in response to.
     * @param pipeID The pipe which is the subject of the event.
     * @return true if sucessfully added
     */
    synchronized boolean addListener(ID pipeID, Listener listener, int queryID) {

        Map<Integer, Listener> perpipelisteners = outputpipeListeners.get(pipeID);

        // if no map for this pipeid, make one and add it to the top map.
        if (null == perpipelisteners) {
            perpipelisteners = new HashMap<Integer, Listener>();
            outputpipeListeners.put(pipeID, perpipelisteners);
        }

        boolean alreadyThere = perpipelisteners.containsKey(queryID);

        if (!alreadyThere) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("adding listener for " + pipeID + " / " + queryID);
            }

            perpipelisteners.put(queryID, listener);
        }

        return alreadyThere;
    }

    /**
     * Call the listener for the specified pipe id informing it about the
     * specified peer.
     *
     * @param qid    The query this callback is being made in response to.
     * @param pipeID The pipe which is the subject of the event.
     * @param type   The type of the pipe which is the subject of the event.
     * @param peer   The peer on which the remote input pipe was found.
     * @param NAK indicate whether the event is a nack
     */
    void callListener(int qid, ID pipeID, String type, PeerID peer, boolean NAK) {

        Event newevent = new Event(this, peer, pipeID, type, qid);
        boolean handled = false;

        while (!handled) {
            Listener pipeListener;

            synchronized (this) {
                Map<Integer, Listener> perpipelisteners = outputpipeListeners.get(pipeID);
                if (null == perpipelisteners) {
                    if ((ANYQUERY != qid) && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("No listener for pipe " + pipeID);
                    }
                    break;
                }
                pipeListener = perpipelisteners.get(qid);
            }

            if (null != pipeListener) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Calling Pipe resolver listener " + (NAK ? "NAK " : "") + "for " + pipeID);
                }

                try {
                    if (NAK) {
                        handled = pipeListener.pipeNAKEvent(newevent);
                    } else {
                        handled = pipeListener.pipeResolveEvent(newevent);
                    }
                } catch (Throwable ignored) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING
                                ,
                                "Uncaught Throwable in listener for: " + pipeID + "(" + pipeListener.getClass().getName() + ")", ignored);
                    }
                }
            }

            // if we havent tried it already, try it with the ANYQUERY
            if (ANYQUERY == qid) {
                break;
            }
            qid = ANYQUERY;
        }
    }

    /**
     * Remove a pipe resolver listener
     *
     * @param pipeID  listener to remove
     * @param queryID matching queryid.
     * @return listener object removed
     */
    synchronized Listener removeListener(ID pipeID, int queryID) {

        Map<Integer, Listener> perpipelisteners = outputpipeListeners.get(pipeID);

        if (null == perpipelisteners) {
            return null;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("removing listener for " + pipeID + " / " + queryID);
        }
        Listener removedListener = perpipelisteners.remove(queryID);

        if (perpipelisteners.isEmpty()) {
            outputpipeListeners.remove(pipeID);
        }

        return removedListener;
    }

    /**
     * Send a request to find an input pipe
     *
     * @param adv             the advertisement for the pipe we are seeking.
     * @param acceptablePeers the set of peers at which we wish the pipe to
     *                        be resolved. We will not accept responses from peers other than those
     *                        in this set. Empty set means all peers are acceptable.
     * @param queryID         the query ID to use for the query. if zero then a query
     *                        ID will be generated
     * @return the query id under which the request was sent
     */
    int sendPipeQuery(PipeAdvertisement adv, Set<? extends ID> acceptablePeers, int queryID) {

        // choose a query id if non-prechosen.
        if (0 == queryID) {
            queryID = getNextQueryID();
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine((acceptablePeers.isEmpty() ? "Undirected" : "Directed") + " query (" + queryID + ") for " + adv.getPipeID());
        }

        Collection<? extends ID> targetPeers = new ArrayList<ID>(acceptablePeers);

        // check local srdi to see if we have a potential answer
        List<? extends ID> knownLocations = srdiIndex.query(adv.getType(), PipeAdvertisement.IdTag, adv.getPipeID().toString(), 100);

        if (!knownLocations.isEmpty()) {
            // we think we know where the pipe might be...

            if (!acceptablePeers.isEmpty()) {
                // only keep those peers which are acceptable.
                knownLocations.retainAll(acceptablePeers);
            }

            // if the known locations contain any of the acceptable peers then
            // we will send a directed query to ONLY those peers.
            if (!knownLocations.isEmpty()) {
                targetPeers = knownLocations;

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Using SRDI cache results for directed query (" + queryID + ") for " + adv.getPipeID());
                }
            }
        }

        // build the pipe query message.
        PipeResolverMessage pipeQry = new PipeResolverMsg();

        pipeQry.setMsgType(MessageType.QUERY);
        pipeQry.setPipeID(adv.getPipeID());
        pipeQry.setPipeType(adv.getType());

        for (Object targetPeer : targetPeers) {
            pipeQry.addPeerID((PeerID) targetPeer);
        }

        StructuredTextDocument asDoc = (StructuredTextDocument) pipeQry.getDocument(MimeMediaType.XMLUTF8);

        // build the resolver query
        ResolverQuery query = new ResolverQuery();

        query.setHandlerName(PipeResolverName);
        query.setQueryId(queryID);
        query.setSrcPeer(myGroup.getPeerID());
        query.setQuery(asDoc.toString());

        CurrentCredential current = currentCredential;

        if (null != current) {
            query.setCredential(current.credentialDoc);
        }

        if (targetPeers.isEmpty()) {
            // we have no idea, walk the tree

            if (myGroup.isRendezvous()) {
                // We are a rdv, then send it to the replica peer.

                PeerID peer = srdi.getReplicaPeer(pipeQry.getPipeType() + PipeAdvertisement.IdTag + pipeQry.getPipeID().toString());
                if (null != peer) {
                    srdi.forwardQuery(peer, query);
                    return queryID;
                }
            }

            resolver.sendQuery(null, query);
        } else {
            // send it only to the peers whose result we would accept.

            for (ID targetPeer : targetPeers) {
                resolver.sendQuery(targetPeer.toString(), query);
            }
        }
        return queryID;
    }

    /**
     * {@inheritDoc}
     */
    SrdiIndex getSrdiIndex() {
        return srdiIndex;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This implementation knows nothing of deltas, it just pushes it all.
     */
    private void pushSrdi(PeerID peer, boolean all) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Pushing " + (all ? "all" : "deltas") + " SRDI to " + peer);
        }

        Map<String, List<Entry>> types = new HashMap<String, List<Entry>>();

        synchronized (this) {
            for (InputPipe ip : localInputPipes.values()) {
                Entry entry = new Entry(PipeAdvertisement.IdTag, ip.getPipeID().toString(), Long.MAX_VALUE);
                String type = ip.getType();
                List<Entry> entries = types.get(type);

                if (null == entries) {
                    entries = new ArrayList<Entry>();
                    types.put(type, entries);
                }
                entries.add(entry);
            }
        }

        for (String type : types.keySet()) {
            List<Entry> entries = types.get(type);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sending a Pipe SRDI messsage in " + myGroup.getPeerGroupID() + " of " + entries.size() + " entries of type " + type);
            }

            SrdiMessage srdiMsg = new SrdiMessageImpl(myGroup.getPeerID(), 1, type, entries);
            if (null == peer) {
                srdi.pushSrdi(null, srdiMsg);
            } else {
                srdi.pushSrdi(peer, srdiMsg);
            }
        }
    }

    /**
     * Push SRDI entry for the specified pipe
     *
     * @param ip     the pipe who's entry we are pushing
     * @param adding adding an entry for the pipe or expiring the entry?
     */
    private void pushSrdi(InputPipe ip, boolean adding) {

        srdiIndex.add(ip.getType(), PipeAdvertisement.IdTag, ip.getPipeID().toString(), myGroup.getPeerID(),
                adding ? Long.MAX_VALUE : 0);

        SrdiMessage srdiMsg;
        try {
            srdiMsg = new SrdiMessageImpl(myGroup.getPeerID(), 1, // ttl
                    ip.getType(), PipeAdvertisement.IdTag, ip.getPipeID().toString(), adding ? Long.MAX_VALUE : 0);

            if (myGroup.isRendezvous()) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Replicating a" + (adding ? "n add" : " remove") + " Pipe SRDI entry for pipe [" + ip.getPipeID()
                            + "] of type " + ip.getType());
                }

                srdi.replicateEntries(srdiMsg);
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Sending a" + (adding ? "n add" : " remove") + " Pipe SRDI messsage for pipe [" + ip.getPipeID()
                            + "] of type " + ip.getType());
                }
                srdi.pushSrdi(null, srdiMsg);
            }
        } catch (Throwable e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Uncaught throwable pushing SRDI entries", e);
            }
        }
    }
}
