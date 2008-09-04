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

package net.jxta.impl.discovery;


import net.jxta.credential.Credential;
import net.jxta.discovery.DiscoveryEvent;
import net.jxta.discovery.DiscoveryListener;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.OutgoingMessageEvent;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.impl.cm.Cm;
import net.jxta.impl.cm.Srdi;
import net.jxta.impl.cm.SrdiIndex;
import net.jxta.impl.peergroup.StdPeerGroup;
import net.jxta.impl.protocol.DiscoveryConfigAdv;
import net.jxta.impl.protocol.DiscoveryQuery;
import net.jxta.impl.protocol.DiscoveryResponse;
import net.jxta.impl.protocol.ResolverQuery;
import net.jxta.impl.protocol.ResolverResponse;
import net.jxta.impl.protocol.SrdiMessageImpl;
import net.jxta.impl.resolver.InternalQueryHandler;
import net.jxta.impl.util.TimeUtils;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.DiscoveryQueryMsg;
import net.jxta.protocol.DiscoveryResponseMsg;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.ResolverSrdiMsg;
import net.jxta.protocol.SrdiMessage;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.rendezvous.RendezvousEvent;
import net.jxta.rendezvous.RendezvousListener;
import net.jxta.resolver.ResolverService;
import net.jxta.resolver.SrdiHandler;
import net.jxta.service.Service;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.net.URI;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * This Discovery Service implementation provides a mechanism to discover
 * Advertisements using the Resolver service and SRDI.
 * <p/>
 * <p/>This implementation uses the standard JXTA Peer Discovery Protocol
 * (PDP).
 * <p/>
 * <p/>The DiscoveryService service also provides a way to obtain information
 * from a specified peer and request other peer advertisements, this method is
 * particularly useful in the case of a portal where new relationships may be
 * established starting from a predetermined peer (perhaps described in address
 * book, or through an invitation).
 *
 * @see net.jxta.discovery.DiscoveryService
 * @see net.jxta.protocol.DiscoveryQueryMsg
 * @see net.jxta.impl.protocol.DiscoveryQuery
 * @see net.jxta.protocol.DiscoveryResponseMsg
 * @see net.jxta.impl.protocol.DiscoveryResponse
 * @see net.jxta.resolver.ResolverService
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pdp" target="_blank">JXTA Protocols Specification : Peer Discovery Protocol</a>
 */
public class DiscoveryServiceImpl implements DiscoveryService, InternalQueryHandler, RendezvousListener, SrdiHandler, Srdi.SrdiInterface {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(DiscoveryServiceImpl.class.getName());

    /**
     * adv types
     */
    final static String[] dirname = {"Peers", "Groups", "Adv"};

    /**
     * The Query ID which will be associated with remote publish operations.
     */
    private final static int REMOTE_PUBLISH_QUERYID = 0;

    private final static String srdiIndexerFileName = "discoverySrdi";

    /**
     * The current resolver query ID. static to make debugging easier.
     */
    private final static AtomicInteger qid = new AtomicInteger(0);

    /**
     * The maximum number of responses we will return for ANY query.
     */
    private final static int MAX_RESPONSES = 50;

    /**
     * The cache manager we're going to use to cache jxta advertisements
     */
    protected Cm cm;

    /**
     * assignedID as a String.
     */
    private PeerGroup group = null;
    private String handlerName = null;
    private ModuleImplAdvertisement implAdvertisement = null;

    private ResolverService resolver = null;
    private RendezVousService rendezvous = null;
    private MembershipService membership = null;

    private PeerID localPeerId = null;

    private boolean localonly = false;
    private boolean alwaysUseReplicaPeer = false;

    private boolean stopped = true;

    /**
     * The table of discovery listeners.
     */
    private Set<DiscoveryListener> listeners = new HashSet<DiscoveryListener>();

    /**
     * The table of discovery query listeners.
     */
    private Hashtable<Integer, DiscoveryListener> listenerTable = new Hashtable<Integer, DiscoveryListener>();

    private final String checkPeerAdvLock = new String("Check/Update PeerAdvertisement Lock");
    private PeerAdvertisement lastPeerAdv = null;
    private int lastModCount = -1;

    private boolean isRdv = false;

    private SrdiIndex srdiIndex = null;
    private Srdi srdi = null;
    private Thread srdiThread = null;

    private CredentialListener membershipCredListener = null;
    private Credential credential = null;
    private StructuredDocument credentialDoc = null;

    private long initialDelay = 60 * TimeUtils.ASECOND;
    private long runInterval = 30 * TimeUtils.ASECOND;

    /**
     * the discovery interface object
     */
    private DiscoveryService discoveryInterface = null;

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

                synchronized (DiscoveryServiceImpl.this) {
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
     * {@inheritDoc}
     */
    public synchronized Service getInterface() {
        if (discoveryInterface == null) {
            discoveryInterface = new DiscoveryServiceInterface(this);
        }
        return discoveryInterface;
    }

    /**
     * {@inheritDoc}
     */
    public Advertisement getImplAdvertisement() {
        return implAdvertisement;
    }

    /**
     * {@inheritDoc}
     */
    public int getRemoteAdvertisements(String peer, int type, String attribute, String value, int threshold) {

        return getRemoteAdvertisements(peer, type, attribute, value, threshold, null);
    }

    /**
     * {@inheritDoc}
     */
    public int getRemoteAdvertisements(String peer, int type, String attribute, String value, int threshold, DiscoveryListener listener) {

        int myQueryID = qid.incrementAndGet();

        if (localonly || stopped) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("localonly, no network operations performed");
            }
            return myQueryID;
        }

        if (resolver == null) {
            // warn about calling the service before it started
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("resolver has not started yet, query discarded.");
            }
            return myQueryID;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            StringBuilder query = new StringBuilder(
                    "Sending query#" + myQueryID + " for " + threshold + " " + dirname[type] + " advs");

            if (attribute != null) {
                query.append("\n\tattr = ").append(attribute);
                if (value != null) {
                    query.append("\tvalue = ").append(value);
                }
            }
            LOG.fine(query.toString());
        }

        long t0 = System.currentTimeMillis();

        DiscoveryQueryMsg dquery = new DiscoveryQuery();

        dquery.setDiscoveryType(type);
        dquery.setAttr(attribute);
        dquery.setValue(value);
        dquery.setThreshold(threshold);

        if (listener != null) {
            listenerTable.put(myQueryID, listener);
        }

        ResolverQueryMsg query = new ResolverQuery();

        query.setHandlerName(handlerName);
        query.setCredential(credentialDoc);
        query.setSrcPeer(localPeerId);
        query.setQuery(dquery.toString());
        query.setQueryId(myQueryID);

        // check srdi
        if (peer == null && srdiIndex != null) {
            List<PeerID> res = srdiIndex.query(dirname[type], attribute, value, threshold);

            if (!res.isEmpty()) {
                srdi.forwardQuery(res, query, threshold);
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Srdi forward a query #" + myQueryID + " in " + (System.currentTimeMillis() - t0) + "ms.");
                }
                return myQueryID;
                // nothing in srdi, get a starting point in rpv
            } else if (group.isRendezvous() && attribute != null && value != null) {
                PeerID destPeer = srdi.getReplicaPeer(dirname[type] + attribute + value);

                if (destPeer != null) {
                    if (!destPeer.equals(group.getPeerID())) {
                        // forward query increments the hopcount to indicate getReplica
                        // has been invoked once
                        srdi.forwardQuery(destPeer, query);
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(
                                    "Srdi forward query #" + myQueryID + " to " + destPeer + " in "
                                            + (System.currentTimeMillis() - t0) + "ms.");
                        }
                        return myQueryID;
                    }
                }
            }
        }

        // no srdi, not a rendezvous, start the walk
        resolver.sendQuery(peer, query);
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            if (peer == null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Sent a query #" + myQueryID + " in " + (System.currentTimeMillis() - t0) + "ms.");
                }
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Sent a query #" + myQueryID + " to " + peer + " in " + (System.currentTimeMillis() - t0) + "ms.");
                }
            }
        }

        return myQueryID;
    }

    /**
     * {@inheritDoc}
     */
    public Enumeration<Advertisement> getLocalAdvertisements(int type, String attribute, String value) throws IOException {

        if ((type > 2) || (type < 0)) {
            throw new IllegalArgumentException("Unknown Advertisement type");
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            StringBuilder query = new StringBuilder("Searching for " + dirname[type] + " advs");

            if (attribute != null) {
                query.append("\n\tattr = ").append(attribute);
            }
            if (value != null) {
                query.append("\tvalue = ").append(value);
            }
            LOG.fine(query.toString());
        }

        return Collections.enumeration(search(type, attribute, value, Integer.MAX_VALUE, false, null));
    }

    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup pg, ID assignedID, Advertisement impl) throws PeerGroupException {

        group = pg;
        handlerName = assignedID.toString();
        implAdvertisement = (ModuleImplAdvertisement) impl;
        localPeerId = group.getPeerID();

        ConfigParams confAdv = pg.getConfigAdvertisement();

        // Get the config. If we do not have a config, we're done; we just keep
        // the defaults (edge peer/no auto-rdv)
        if (confAdv != null) {
            Advertisement adv = null;

            try {
                XMLDocument configDoc = (XMLDocument) confAdv.getServiceParam(assignedID);

                if (null != configDoc) {
                    adv = AdvertisementFactory.newAdvertisement(configDoc);
                }
            } catch (NoSuchElementException failed) {// ignored
            }

            if (adv instanceof DiscoveryConfigAdv) {
                DiscoveryConfigAdv discoConfigAdv = (DiscoveryConfigAdv) adv;

                alwaysUseReplicaPeer = discoConfigAdv.getForwardAlwaysReplica();

                localonly |= discoConfigAdv.getLocalOnly();

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    if (localonly) {
                        LOG.fine("localonly set to true via service parameters");
                    }
                    if (alwaysUseReplicaPeer) {
                        LOG.fine("alwaysUseReplicaPeer set to true via service parameters");
                    }
                }
            }
        }

        cm = ((StdPeerGroup) group).getCacheManager();
        cm.setTrackDeltas(!localonly);

        // Initialize the peer adv tracking.
        checkUpdatePeerAdv();

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Discovery Service : " + assignedID);

            if (implAdvertisement != null) {
                configInfo.append("\n\tImplementation :");
                configInfo.append("\n\t\tModule Spec ID: ").append(implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description : ").append(implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI : ").append(implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code : ").append(implAdvertisement.getCode());
            }
            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tGroup : ").append(group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID : ").append(group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID : ").append(group.getPeerID());
            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tLocal Only : ").append(localonly);
            configInfo.append("\n\t\tAlways Use ReplicaPeer : ").append(alwaysUseReplicaPeer);
            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public int startApp(String[] arg) {

        // Now we know that the resolver is going to be there.
        // The cm needs the resolver. The code is arranged so that
        // until the resolver and the cm are created, we just pretend
        // to be working. We have no requirement to be operational before
        // startApp() is called, but we must tolerate our public methods
        // being invoked. The reason for it is that services are registered
        // upon return from init() so that other services startApp() methods
        // can find them. (all startApp()s are called after all init()s - with
        // a few exceptions).

        resolver = group.getResolverService();

        if (null == resolver) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a resolver service");
            }

            return Module.START_AGAIN_STALLED;
        }

        membership = group.getMembershipService();

        if (null == membership) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a membership service");
            }

            return Module.START_AGAIN_STALLED;
        }

        rendezvous = group.getRendezVousService();

        if (null == rendezvous) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a rendezvous service");
            }

            return Module.START_AGAIN_STALLED;
        }

        // local only discovery
        if (!localonly) {
            resolver.registerHandler(handlerName, this);
        }

        // Get the initial credential doc
        synchronized (this) {
            membershipCredListener = new CredentialListener();
            membership.addPropertyChangeListener("defaultCredential", membershipCredListener);

            try {
                membershipCredListener.propertyChange(
                        new PropertyChangeEvent(membership, "defaultCredential", null, membership.getDefaultCredential()));
            } catch (Exception all) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not get credential", all);
                }
            }
        }

        if (rendezvous.isRendezVous()) {
            beRendezvous();
        } else {
            beEdge();
        }

        rendezvous.addListener(this);

        stopped = false;

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Discovery service started");
        }

        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>Detach from the resolver
     */
    public void stopApp() {

        stopped = true;
        boolean failed = false;

        membership.removePropertyChangeListener("defaultCredential", membershipCredListener);
        membershipCredListener = null;
        credential = null;
        credentialDoc = null;

        rendezvous.removeListener(this);

        if (resolver.unregisterHandler(handlerName) == null) {
            failed = true;
        }

        if (rendezvous.isRendezVous()) {
            if (resolver.unregisterSrdiHandler(handlerName) == null) {
                failed = true;
            }
        }

        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING) && failed) {
            LOG.warning("failed to unregister discovery from resolver.");
        }

        // stop the DiscoverySrdiThread
        if (srdiThread != null) {
            srdi.stop();
            srdi = null;
        }

        // Reset values in order to avoid cross-reference problems with GC
        resolver = null;
        group = null;
        membership = null;
        srdiIndex = null;
        srdiThread = null;
        rendezvous = null;

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Discovery service stopped.");
        }
    }

    /**
     * {@inheritDoc}
     */
    public void flushAdvertisements(String id, int type) throws IOException {
        if (stopped) {
            return;
        }
        if ((type >= PEER) && (type <= ADV)) {
            if (null != id) {
                ID advID = ID.create(URI.create(id));
                String advName = advID.getUniqueValue().toString();

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("flushing adv " + advName + " of type " + dirname[type]);
                }
                cm.remove(dirname[type], advName);
            } else {
                // XXX bondolo 20050902 For historical purposes we ignore null

                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Flush request by type IGNORED. You must delete advertisements individually.");
                }
            }
        } else {
            throw new IllegalArgumentException("Invalid Advertisement type.");
        }
    }

    /**
     * {@inheritDoc}
     */
    public void flushAdvertisement(Advertisement adv) throws IOException {
        if (stopped) {
            return;
        }

        int type;

        if (adv instanceof PeerAdvertisement) {
            type = PEER;
        } else if (adv instanceof PeerGroupAdvertisement) {
            type = GROUP;
        } else {
            type = ADV;
        }

        ID id = adv.getID();
        String advName;

        if (id != null && !id.equals(ID.nullID)) {
            advName = id.getUniqueValue().toString();
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Flushing adv " + advName + " of type " + dirname[type]);
            }
        } else {
            XMLDocument doc;

            try {
                doc = (XMLDocument) adv.getDocument(MimeMediaType.XMLUTF8);
            } catch (Exception everything) {
                IOException failure = new IOException("Failure removing Advertisement");

                failure.initCause(everything);
                throw failure;
            }
            advName = Cm.createTmpName(doc);
        }

        if (advName != null) {
            cm.remove(dirname[type], advName);
        }
    }

    /**
     * {@inheritDoc}
     */
    public void publish(Advertisement adv) throws IOException {
        publish(adv, DiscoveryService.DEFAULT_LIFETIME, DiscoveryService.DEFAULT_EXPIRATION);
    }

    /**
     * {@inheritDoc}
     */
    public void publish(Advertisement adv, long lifetime, long expiration) throws IOException {

        if (stopped) {
            return;
        }

        ID advID;
        String advName;

        int type;

        if (adv instanceof PeerAdvertisement) {
            type = PEER;
        } else if (adv instanceof PeerGroupAdvertisement) {
            type = GROUP;
        } else {
            type = ADV;
        }

        advID = adv.getID();

        // if we dont have a unique id for the adv, use the hash method
        if ((null == advID) || advID.equals(ID.nullID)) {
            XMLDocument doc;

            try {
                doc = (XMLDocument) adv.getDocument(MimeMediaType.XMLUTF8);
            } catch (Exception everything) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to generated document from advertisement", everything);
                }
                IOException failure = new IOException("Failed to generate document from advertisement");

                failure.initCause(everything);
                throw failure;
            }

            try {
                advName = Cm.createTmpName(doc);
            } catch (IllegalStateException ise) {
                IOException failure = new IOException("Failed to generate tempname from advertisement");

                failure.initCause(ise);
                throw failure;
            }
        } else {
            advName = advID.getUniqueValue().toString();
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(
                    "Publishing a " + adv.getAdvType() + " as " + dirname[type] + " / " + advName + "\n\texpiration : "
                            + expiration + "\tlifetime :" + lifetime);
        }

        // save it
        cm.save(dirname[type], advName, adv, lifetime, expiration);
    }

    /**
     * {@inheritDoc}
     */
    public void remotePublish(Advertisement adv) {
        remotePublish(null, adv, DiscoveryService.DEFAULT_EXPIRATION);
    }

    /**
     * {@inheritDoc}
     */
    public void remotePublish(Advertisement adv, long expiration) {
        remotePublish(null, adv, expiration);
    }

    /**
     * {@inheritDoc}
     */
    public void remotePublish(String peerid, Advertisement adv) {
        remotePublish(peerid, adv, DiscoveryService.DEFAULT_EXPIRATION);
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
    public void processResponse(ResolverResponseMsg response, EndpointAddress srcAddress) {
        if (stopped) {
            return;
        }

        long t0 = System.currentTimeMillis();
        DiscoveryResponse res;

        try {
            XMLDocument asDoc = (XMLDocument)
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8
                            ,
                            new StringReader(response.getResponse()));

            res = new DiscoveryResponse(asDoc);
        } catch (Exception e) {
            // we don't understand this msg, let's skip it
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to Read Discovery Response", e);
            }
            return;
        }

        /*
         PeerAdvertisement padv = res.getPeerAdvertisement();
         if (padv == null)
         return;
         
         if (LOG.isLoggable(Level.FINE)) {
         LOG.fine("Got a " + dirname[res.getDiscoveryType()] +
         " from "+padv.getName()+ " response : " +
         res.getQueryAttr() + " = " + res.getQueryValue());
         }
         
         try {
         // The sender does not put an expiration on that one, but
         // we do not want to keep it around for more than the
         // default duration. It may get updated or become invalid.
         publish(padv, PEER, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
         } catch (Exception e) {
         if (LOG.isLoggable(Level.FINE)) {
         LOG.fine(e, e);
         }
         return;
         }
         */
        Advertisement adv;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Processing responses for query #" + response.getQueryId());
        }

        Enumeration<Advertisement> en = res.getAdvertisements();
        Enumeration<Long> exps = res.getExpirations();

        while (en.hasMoreElements()) {
            adv = en.nextElement();
            long exp = exps.nextElement();

            if (exp > 0 && adv != null) {
                try {
                    publish(adv, exp, exp);
                } catch (Exception e) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Error publishing Advertisement", e);
                    }
                }
            }
        }

        DiscoveryEvent newevent = new DiscoveryEvent(srcAddress, res, response.getQueryId());

        DiscoveryListener dl = listenerTable.get(new Integer(response.getQueryId()));

        if (dl != null) {
            try {
                dl.discoveryEvent(new DiscoveryEvent(srcAddress, res, response.getQueryId()));
            } catch (Throwable all) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in listener :" + Thread.currentThread().getName(), all);
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("processed a response for query #" + response.getQueryId() + " in :" + (System.currentTimeMillis() - t0));
        }

        // are there any registered discovery listeners,
        // generate the event and callback.
        t0 = System.currentTimeMillis();

        DiscoveryListener[] allListeners = listeners.toArray(new DiscoveryListener[0]);

        for (DiscoveryListener allListener : allListeners) {
            try {
                allListener.discoveryEvent(newevent);
            } catch (Throwable all) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING
                            ,
                            "Uncaught Throwable in listener (" + allListener.getClass().getName() + ") :"
                                    + Thread.currentThread().getName()
                            ,
                            all);
                }
            }
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Called all listenters to query #" + response.getQueryId() + " in :" + (System.currentTimeMillis() - t0));
        }
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
    public int processQuery(ResolverQueryMsg query, EndpointAddress srcAddress) {
        if (stopped) {
            return ResolverService.OK;
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            if (srcAddress != null) {
                LOG.fine("Processing query #" + query.getQueryId() + " from:" + srcAddress);
            } else {
                LOG.fine("Processing query #" + query.getQueryId() + " from: unknown");
            }
        }

        List<String> results;
        List<Long> expirations = new ArrayList<Long>();
        DiscoveryQuery dq;
        long t0 = System.currentTimeMillis();

        try {
            XMLDocument asDoc = (XMLDocument)
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, new StringReader(query.getQuery()));

            dq = new DiscoveryQuery(asDoc);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Malformed query : ", e);
            }
            return ResolverService.OK;
        }

        if ((dq.getThreshold() < 0) || (dq.getDiscoveryType() < PEER) || (dq.getDiscoveryType() > ADV)) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Malformed query");
            }
            return ResolverService.OK;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(
                    "Got a " + dirname[dq.getDiscoveryType()] + " query #" + query.getQueryId() + " query :" + dq.getAttr()
                            + " = " + dq.getValue());
        }

        /*
         // Get the Peer Adv from the query and publish it.
         PeerAdvertisement padv = dq.getPeerAdvertisement();
         try {
         if (!(padv.getPeerID().toString()).equals(localPeerId)) {
         // publish others only. Since this one comes from outside,
         // we must not keep it beyond its expiration time.
         // FIXME: [jice@jxta.org 20011112] In theory there should
         // be an expiration time associated with it in the msg, like
         // all other items.
         publish(padv, PEER, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
         }
         } catch (Exception e) {
         if (LOG.isLoggable(Level.FINE)) {
         LOG.fine("Bad Peer Adv in Discovery Query", e);
         }
         }
         */

        /*
         *  threshold==0 and type==PEER is a special case. In this case we are
         *  responding for the purpose of providing our own adv only.
         */
        int thresh = Math.min(dq.getThreshold(), MAX_RESPONSES);

        /*
         *  threshold==0 and type==PEER is a special case. In this case we are
         *  responding for the purpose of providing our own adv only.
         */
        if ((dq.getDiscoveryType() == PEER) && (0 == dq.getThreshold())) {
            respond(query, dq, Collections.singletonList(group.getPeerAdvertisement().toString())
                    ,
                    Collections.singletonList(DiscoveryService.DEFAULT_EXPIRATION));
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Responding to query #" + query.getQueryId() + " in :" + (System.currentTimeMillis() - t0));
            }
            return ResolverService.OK;
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("start local search query" + dq.getAttr() + " " + dq.getValue());
            }
            results = search(dq.getDiscoveryType(), dq.getAttr(), dq.getValue(), thresh, true, expirations);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("start local search pruned " + results.size());
            }
        }

        // We only share the advs with > 0 expiration time.
        Iterator<Long> eachExpiration = expirations.iterator();
        Iterator eachAdv = results.iterator();

        while (eachExpiration.hasNext()) {
            eachAdv.next();

            if (eachExpiration.next() <= 0) {
                eachAdv.remove();
                eachExpiration.remove();
            }
        }

        if (!results.isEmpty()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Responding to " + dirname[dq.getDiscoveryType()] + " Query : " + dq.getAttr() + " = " + dq.getValue());
            }
            respond(query, dq, results, expirations);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Responded to query #" + query.getQueryId() + " in :" + (System.currentTimeMillis() - t0));
            }
            return ResolverService.OK;
        } else {
            // If this peer is a rendezvous, simply let the resolver
            // re-propagate the query.
            // If this peer is not a rendez, just discard the query.
            if (!group.isRendezvous()) {
                return ResolverService.OK;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Querying SrdiIndex query #" + query.getQueryId());
            }
            List<PeerID> res = srdiIndex.query(dirname[dq.getDiscoveryType()], dq.getAttr(), dq.getValue(), thresh);

            if (!res.isEmpty()) {
                srdi.forwardQuery(res, query, thresh);
                return ResolverService.OK;
            } else if (query.getHopCount() == 0) {
                PeerID destPeer = srdi.getReplicaPeer(dirname[dq.getDiscoveryType()] + dq.getAttr() + dq.getValue());

                // destPeer can be null in a small rpv (<3)
                if (destPeer != null) {
                    if (!destPeer.equals(group.getPeerID())) {
                        srdi.forwardQuery(destPeer, query);
                        return ResolverService.OK;
                    } else {
                        // start the walk since this peer is this the starting peer
                        query.incrementHopCount();
                    }
                }
            }
        }
        return ResolverService.Repropagate;
    }

    private void respond(ResolverQueryMsg query, DiscoveryQuery dq, List results, List<Long> expirations) {
        if (localonly || stopped) {
            return;
        }

        ResolverResponseMsg response;
        DiscoveryResponse dresponse = new DiscoveryResponse();

        // peer adv is optional, skip
        dresponse.setDiscoveryType(dq.getDiscoveryType());
        dresponse.setQueryAttr(dq.getAttr());
        dresponse.setQueryValue(dq.getValue());
        dresponse.setResponses(results);
        dresponse.setExpirations(expirations);

        // create a response from the query
        response = query.makeResponse();
        response.setCredential(credentialDoc);
        response.setResponse(dresponse.toString());

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Responding to " + query.getSrcPeer());
        }

        resolver.sendResponse(query.getSrcPeer().toString(), response);
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void addDiscoveryListener(DiscoveryListener listener) {

        listeners.add(listener);
    }

    /**
     * {@inheritDoc}
     */
    public synchronized boolean removeDiscoveryListener(DiscoveryListener listener) {

        Iterator<Map.Entry<Integer, DiscoveryListener>> e = listenerTable.entrySet().iterator();

        while (e.hasNext()) {
            Map.Entry<Integer, DiscoveryListener> anEntry = e.next();

            if (listener == anEntry.getValue()) {
                e.remove();
            }
        }
        return (listeners.remove(listener));
    }

    /**
     * {@inheritDoc}
     */
    public void remotePublish(String peerid, Advertisement adv, long timeout) {

        if (localonly || stopped) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("localonly, no network operations performed");
            }
            return;
        }
        int type;

        if (adv instanceof PeerAdvertisement) {
            type = PEER;
        } else if (adv instanceof PeerGroupAdvertisement) {
            type = GROUP;
        } else {
            type = ADV;
        }
        remotePublish(peerid, adv, type, timeout);
    }

    /*
     *  remote publish the advertisement
     */
    private void remotePublish(String peerid, Advertisement adv, int type, long expiration) {

        if (localonly || stopped) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("localonly, no network operations performed");
            }
            return;
        }

        // In case this is invoked before startApp().
        if (resolver == null) {
            return;
        }

        switch (type) {
            case PEER:
                if (adv instanceof PeerAdvertisement) {
                    break;
                }
                throw new IllegalArgumentException("Not a peer advertisement");

            case GROUP:
                if (adv instanceof PeerGroupAdvertisement) {
                    break;
                }
                throw new IllegalArgumentException("Not a peergroup advertisement");

            case ADV:
                break;

            default:
                throw new IllegalArgumentException("Unknown advertisement type");
        }

        List<String> advert = new ArrayList<String>(1);
        List<Long> expirations = new ArrayList<Long>(1);

        advert.add(adv.toString());
        expirations.add(expiration);

        DiscoveryResponseMsg dresponse = new DiscoveryResponse();

        dresponse.setDiscoveryType(type);
        dresponse.setResponses(advert);
        dresponse.setExpirations(expirations);

        ResolverResponseMsg pushRes = new ResolverResponse();

        pushRes.setHandlerName(handlerName);
        pushRes.setCredential(credentialDoc);
        pushRes.setQueryId(REMOTE_PUBLISH_QUERYID);
        pushRes.setResponse(dresponse.toString());

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Remote publishing ");
        }

        resolver.sendResponse(peerid, pushRes);
    }

    /**
     * Search for a doc, that matches attr, and value
     * bytes is set to true if the caller wants wire format of the
     * advertisement, or set to false if caller wants Advertisement
     * objects.
     *
     * @param type        Discovery type PEER, GROUP, ADV
     * @param threshold   the upper limit of responses from one peer
     * @param bytes       flag to indicate how the results are returned-- advs, or streams
     * @param expirations List containing the expirations associated with is returned
     * @param attr        attribute name to narrow discovery to Valid values for
     *                    this parameter are null (don't care), or exact element name in the
     *                    advertisement of interest (e.g. "Name")
     * @param value       Value
     * @return list of results either as docs, or Strings
     */
    private List search(int type, String attr, String value, int threshold, boolean bytes, List<Long> expirations) {

        if (stopped) {
            return new ArrayList();
        }

        if (type == PEER) {
            checkUpdatePeerAdv();
        }

        List results;

        if (threshold <= 0) {
            throw new IllegalArgumentException("threshold must be greater than zero");
        }

        if (expirations != null) {
            expirations.clear();
        }

        if (attr != null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Searching for " + threshold + " entries of type : " + dirname[type]);
            }
            // a discovery query with a specific search criteria.
            results = cm.search(dirname[type], attr, value, threshold, expirations);
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Getting " + threshold + " entries of type : " + dirname[type]);
            }
            // Returning any entry that exists
            results = cm.getRecords(dirname[type], threshold, expirations);
        }

        if (results.isEmpty() || bytes) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Returning " + results.size() + " results");
            }

            // nothing more to do;
            return results;
        }

        // Convert the input streams returned by the cm into Advertisements.

        List<Advertisement> advertisements = new ArrayList<Advertisement>();

        for (int i = 0; i < results.size(); i++) {
            InputStream bis = null;

            try {
                bis = (InputStream) results.get(i);
                XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, bis);
                Advertisement adv = AdvertisementFactory.newAdvertisement(asDoc);

                advertisements.add(adv);
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed building advertisment", e);
                }

                // we won't be including this advertisement so remove it's expiration.
                if (null != expirations) {
                    expirations.remove(i);
                }

            } finally {
                if (null != bis) {
                    try {
                        bis.close();
                    } catch (IOException ignored) {
                        // ignored
                    }
                }
                bis = null;
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Returning " + advertisements.size() + " advertisements");
        }

        return advertisements;
    }

    /**
     * {@inheritDoc}
     */
    public long getAdvExpirationTime(ID id, int type) {
        if (stopped) {
            return -1;
        }
        String advName;

        if (id != null && !id.equals(ID.nullID)) {
            advName = id.getUniqueValue().toString();
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Getting expiration time of " + advName + " of type " + dirname[type]);
            }
        } else {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("invalid attempt to get advertisement expiration time of NullID");
            }
            return -1;
        }

        return cm.getExpirationtime(dirname[type], advName);
    }

    /**
     * {@inheritDoc}
     */
    public long getAdvLifeTime(ID id, int type) {
        if (id == null || id.equals(ID.nullID) || stopped) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("invalid attempt to get advertisement lifetime of a NullID");
            }
            return -1;
        }

        String advName = id.getUniqueValue().toString();

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Getting lifetime of " + advName + " of type " + dirname[type]);
        }

        return cm.getLifetime(dirname[type], advName);
    }

    /**
     * {@inheritDoc}
     */
    public long getAdvExpirationTime(Advertisement adv) {
        if (stopped) {
            return -1;
        }
        int type;

        if (adv instanceof PeerAdvertisement) {
            type = PEER;
        } else if (adv instanceof PeerGroupAdvertisement) {
            type = GROUP;
        } else {
            type = ADV;
        }

        String advName;
        ID id = adv.getID();

        if (id != null && !id.equals(ID.nullID)) {
            advName = id.getUniqueValue().toString();
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("attempting to getAdvExpirationTime on " + advName + " of type " + dirname[type]);
            }
        } else {
            XMLDocument doc;

            try {
                doc = (XMLDocument) adv.getDocument(MimeMediaType.XMLUTF8);
            } catch (Exception everything) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to get document", everything);
                }
                return -1;
            }
            advName = Cm.createTmpName(doc);
        }

        return cm.getExpirationtime(dirname[type], advName);
    }

    /**
     * {@inheritDoc}
     */
    public long getAdvLifeTime(Advertisement adv) {
        if (stopped) {
            return -1;
        }
        int type;

        if (adv instanceof PeerAdvertisement) {
            type = PEER;
        } else if (adv instanceof PeerGroupAdvertisement) {
            type = GROUP;
        } else {
            type = ADV;
        }

        ID id = adv.getID();
        String advName;

        if (id != null && !id.equals(ID.nullID)) {
            advName = id.getUniqueValue().toString();
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("attempting to getAdvLifeTime " + advName + " of type " + dirname[type]);
            }
        } else {
            XMLDocument doc;

            try {
                doc = (XMLDocument) adv.getDocument(MimeMediaType.XMLUTF8);
            } catch (Exception everything) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to get document", everything);
                }
                return -1;
            }
            advName = Cm.createTmpName(doc);
        }
        return cm.getLifetime(dirname[type], advName);
    }

    /**
     * {@inheritDoc}
     */
    public boolean processSrdi(ResolverSrdiMsg message) {
        if (stopped) {
            return true;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("[" + group.getPeerGroupID() + "] Received an SRDI messsage");
        }

        SrdiMessage srdiMsg;

        try {
            XMLDocument asDoc = (XMLDocument)
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, new StringReader(message.getPayload()));

            srdiMsg = new SrdiMessageImpl(asDoc);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed parsing srdi message", e);
            }
            return false;
        }

        PeerID pid = srdiMsg.getPeerID();

        for (Object o : srdiMsg.getEntries()) {
            SrdiMessage.Entry entry = (SrdiMessage.Entry) o;

            srdiIndex.add(srdiMsg.getPrimaryKey(), entry.key, entry.value, pid, entry.expiration);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        "Primary Key [" + srdiMsg.getPrimaryKey() + "] key [" + entry.key + "] value [" + entry.value + "] exp ["
                                + entry.expiration + "]");
            }
        }
        srdi.replicateEntries(srdiMsg);
        return true;
    }

    /**
     * {@inheritDoc}
     */
    public void messageSendFailed(PeerID peerid, OutgoingMessageEvent e) {
        if (srdiIndex != null) {
            srdiIndex.remove(peerid);
        }
    }

    /**
     * {@inheritDoc}
     */
    public void pushEntries(boolean all) {

        pushSrdi(null, PEER, all);
        pushSrdi(null, GROUP, all);
        pushSrdi(null, ADV, all);
    }

    /**
     * push srdi entries
     *
     * @param all  if true push all entries, otherwise just deltas
     * @param peer peer id
     * @param type if true sends all entries
     */
    protected void pushSrdi(ID peer, int type, boolean all) {
        if (stopped) {
            return;
        }

        List<SrdiMessage.Entry> entries;

        if (all) {
            entries = cm.getEntries(dirname[type], true);
        } else {
            entries = cm.getDeltas(dirname[type]);
        }

        if (!entries.isEmpty()) {
            SrdiMessage srdiMsg;

            try {
                srdiMsg = new SrdiMessageImpl(group.getPeerID(), 1, // ttl of 1, ensure it is replicated
                        dirname[type], entries);

                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("Pushing " + entries.size() + (all ? " entries" : " deltas") + " of type " + dirname[type]);
                }
                srdi.pushSrdi(peer, srdiMsg);
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Exception pushing SRDI Entries", e);
                }
            }
        } else {
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("No" + (all ? " entries" : " deltas") + " of type " + dirname[type] + " to push");
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void rendezvousEvent(RendezvousEvent event) {

        int theEventType = event.getType();

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("[" + group.getPeerGroupName() + "] Processing " + event);
        }

        switch (theEventType) {

            case RendezvousEvent.RDVCONNECT:
            case RendezvousEvent.RDVRECONNECT:
                // start tracking deltas
                cm.setTrackDeltas(true);
                break;

            case RendezvousEvent.CLIENTCONNECT:
            case RendezvousEvent.CLIENTRECONNECT:
                break;

            case RendezvousEvent.RDVFAILED:
            case RendezvousEvent.RDVDISCONNECT:
                // stop tracking deltas until we connect again
                cm.setTrackDeltas(false);
                break;

            case RendezvousEvent.CLIENTFAILED:
            case RendezvousEvent.CLIENTDISCONNECT:
                break;

            case RendezvousEvent.BECAMERDV:
                beRendezvous();
                break;

            case RendezvousEvent.BECAMEEDGE:
                beEdge();
                break;

            default:
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning(MessageFormat.format("[{0}] Unexpected RDV event : {1}", group.getPeerGroupName(), event));
                }
                break;
        }
    }

    /**
     * Checks to see if the local peer advertisement has been updated and if
     * it has then republish it to the CM.
     */
    private void checkUpdatePeerAdv() {
        PeerAdvertisement newPadv = group.getPeerAdvertisement();
        int newModCount = newPadv.getModCount();

        boolean updated = false;

        synchronized (checkPeerAdvLock) {
            if ((lastPeerAdv != newPadv) || (lastModCount < newModCount)) {
                lastPeerAdv = newPadv;
                lastModCount = newModCount;
                updated = true;
            }

            if (updated) {
                // Publish the local Peer Advertisement
                try {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("publishing local advertisement");
                    }

                    // This is our own; we can publish it for a long time in our cache
                    publish(newPadv, INFINITE_LIFETIME, DEFAULT_EXPIRATION);
                } catch (Exception ignoring) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Could not publish local peer advertisement: ", ignoring);
                    }
                }
            }
        }
    }

    /**
     * Change the behavior to be an rendezvous Peer Discovery Service.
     * If the Service was acting as an Edge peer, cleanup.
     */
    private synchronized void beRendezvous() {

        if (isRdv && (srdi != null || srdiIndex != null)) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Already a rendezvous -- No Switch is needed");
            }
            return;
        }

        isRdv = true;

        // rdv peers do not need to track deltas
        cm.setTrackDeltas(false);

        if (srdiIndex == null) {
            srdiIndex = new SrdiIndex(group, srdiIndexerFileName);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("srdiIndex created");
            }

        }

        // Kill SRDI, create a new one.
        if (srdi != null) {
            srdi.stop();
            if (srdiThread != null) {
                srdiThread = null;
            }
            srdi = null;
        }

        if (!localonly) {
            srdi = new Srdi(group, handlerName, this, srdiIndex, initialDelay, runInterval);
            resolver.registerSrdiHandler(handlerName, this);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("srdi created, and registered as an srdi handler ");
            }
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Switched to Rendezvous peer role.");
        }
    }

    /**
     * Change the behavior to be an Edge Peer Discovery Service.
     * If the Service was acting as a Rendezvous, cleanup.
     */
    private synchronized void beEdge() {

        // make sure we have been here before
        if (!isRdv && srdiThread != null) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Already an Edge peer -- No Switch is needed.");
            }
            return;
        }

        isRdv = false;
        if (!rendezvous.getConnectedPeerIDs().isEmpty()) {
            // if we have a rendezvous connection track deltas, otherwise wait
            // for a connect event to set this option
            cm.setTrackDeltas(true);
        }
        if (srdiIndex != null) {
            srdiIndex.stop();
            srdiIndex = null;
            resolver.unregisterSrdiHandler(handlerName);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("stopped cache and unregistered from resolver");
            }
        }

        // Kill SRDI
        if (srdi != null) {
            srdi.stop();
            if (srdiThread != null) {
                srdiThread = null;
            }
            srdi = null;
        }

        if (!localonly) {
            // Create a new SRDI
            srdi = new Srdi(group, handlerName, this, null, initialDelay, runInterval);

            // only edge peers distribute srdi
            srdiThread = new Thread(group.getHomeThreadGroup(), srdi, "Discovery Srdi Thread");
            srdiThread.setDaemon(true);
            srdiThread.start();
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Started SRDIThread");
            }
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Switched to a Edge peer role.");
        }
    }
}
