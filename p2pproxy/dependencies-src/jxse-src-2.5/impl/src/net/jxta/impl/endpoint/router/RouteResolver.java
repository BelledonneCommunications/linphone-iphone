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
package net.jxta.impl.endpoint.router;

import net.jxta.credential.Credential;
import net.jxta.document.*;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.OutgoingMessageEvent;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.impl.cm.Srdi;
import net.jxta.impl.cm.Srdi.SrdiInterface;
import net.jxta.impl.cm.SrdiIndex;
import net.jxta.impl.protocol.*;
import net.jxta.impl.util.TimeUtils;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.*;
import net.jxta.resolver.QueryHandler;
import net.jxta.resolver.ResolverService;
import net.jxta.resolver.SrdiHandler;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.util.*;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Handles dynamic route resolution.
 */
class RouteResolver implements Module, QueryHandler, SrdiHandler, SrdiInterface {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(RouteResolver.class.getName());

    /**
     * Router Service Name
     */
    public final static String routerSName = "EndpointRouter";

    private final static String srdiIndexerFileName = "routerSrdi";

    /**
     * Negative Route query acknowledgment
     */
    private final static int NACKROUTE_QUERYID = -1;

    /**
     * Bad route expiration. Amount of time we consider a route bad
     */
    private final static long BADROUTE_EXPIRATION = 2L * TimeUtils.AMINUTE;

    /**
     * Default dynamic route resolution configuration preference.
     */
    private final static boolean USE_ROUTE_RESOLVER_DEFAULT = true;

    /**
     * Configuration property that disables the usage
     * of dynamic route resolution. Dynamic routes
     * will not be discovered. set to true by default
     * can be overwritten via ConfigParams
     */
    private boolean useRouteResolver = USE_ROUTE_RESOLVER_DEFAULT;

    /**
     * PeerGroup Service Handle
     */
    private PeerGroup group = null;

    /**
     * Resolver service handle
     */
    private ResolverService resolver = null;

    /**
     * membership service
     */
    private MembershipService membership = null;

    /**
     * EndpointRouter pointer
     */
    private EndpointRouter router = null;

    /**
     * local peer ID as a endpointAddress.
     */
    private EndpointAddress localPeerAddr = null;

    /**
     * local Peer ID
     */
    private ID localPeerId = null;

    /**
     * Route CM Persistent cache
     */
    private RouteCM routeCM = null;

    /**
     *  The current resolver query ID. static to make debugging easier.
     */
    private final static AtomicInteger qid = new AtomicInteger(0);

    /**
     * SRDI route index
     */
    private SrdiIndex srdiIndex = null;

    /**
     * SRDI Index
     */
    private Srdi srdi = null;

    /**
     *  Encapsulates current Membership Service credential.
     */
    final static class CurrentCredential {

        /**
         *	The current default credential
         */
        final Credential credential;
        
        /**
         *	The current default credential in serialized XML form.
         */
        final XMLDocument credentialDoc;
        
        CurrentCredential(Credential credential, XMLDocument credentialDoc) {
            this.credential = credential;
            this.credentialDoc = credentialDoc;
        }
    }
    
    /**
     *   The current Membership service default credential.
     */
    CurrentCredential currentCredential;
    
    /**
     *  Listener we use for membership property events.
     */
    private class CredentialListener implements PropertyChangeListener {

        /**
         *  Standard Constructor
         */
        CredentialListener() {}
        
        /**
         *  {@inheritDoc}
         */
        public void propertyChange(PropertyChangeEvent evt) {
            if (MembershipService.DEFAULT_CREDENTIAL_PROPERTY.equals(evt.getPropertyName())) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("New default credential event");
                }

                synchronized (RouteResolver.this) {
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
     * @param router the router
     */
    RouteResolver(EndpointRouter router) {
        this.router = router;
    }

    /**
     * initialize  routeResolver
     */
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {

        ModuleImplAdvertisement implAdvertisement = (ModuleImplAdvertisement) impl;

        // extract Router service configuration properties
        ConfigParams confAdv = group.getConfigAdvertisement();
        XMLElement paramBlock = null;

        if (confAdv != null) {
            paramBlock = (XMLElement) confAdv.getServiceParam(assignedID);
        }

        if (paramBlock != null) {
            // get our tunable router parameter
            Enumeration param;

            param = paramBlock.getChildren("useRouteResolver");
            if (param.hasMoreElements()) {
                useRouteResolver = Boolean.getBoolean(((XMLElement) param.nextElement()).getTextValue());
            }
        }

        this.group = group;

        localPeerId = group.getPeerID();

        localPeerAddr = EndpointRouter.pid2addr(group.getPeerID());

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Router Transport Resolver : " + assignedID);

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

            configInfo.append("\n\tConfiguration:");
            configInfo.append("\n\t\tUse Route Resolver : ").append(useRouteResolver());
            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public int startApp(String[] arg) {

        resolver = group.getResolverService();

        if (null == resolver) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Endpoint Router start stalled until resolver service available");
            }
            return Module.START_AGAIN_STALLED;
        }

        membership = group.getMembershipService();

        if (null == membership) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Endpoint Router start stalled until membership service available");
            }
            return Module.START_AGAIN_STALLED;
        }

        resolver.registerHandler(routerSName, this);
        // create and register the srdi service
        srdiIndex = new SrdiIndex(group, srdiIndexerFileName);
        // Srdi is a thread but we are not going to start,
        // since the service is reactive.
        srdi = new Srdi(group, routerSName, this, srdiIndex, 0, 0);
        resolver.registerSrdiHandler(routerSName, this);

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

        // get the RouteCM cache service
        routeCM = router.getRouteCM();

        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Careful that stopApp() could in theory be called before startApp().
     */
    public void stopApp() {

        resolver.unregisterHandler(routerSName);

        // unregister SRDI
        resolver.unregisterSrdiHandler(routerSName);
        srdiIndex.stop();

        membership.removePropertyChangeListener("defaultCredential", membershipCredListener);
        currentCredential = null;

        resolver = null;
        srdi = null;
        membership = null;
    }

    /**
     * return routeResolver usage
     *
     * @return routeResolver usage
     */
    boolean useRouteResolver() {
        return useRouteResolver;
    }

    /**
     * enable routeResolver usage
     * @param enable if true, enables route resolver
     */
    void enableRouteResolver(boolean enable) {
        useRouteResolver = enable;
    }

    /**
     * issue a new route discovery resolver request
     *
     * @param peer the destination as a logical endpoint address
     */
    protected void findRoute(EndpointAddress peer) {

        RouteAdvertisement myRoute = router.getMyLocalRoute();

        // No need to pursue further if we haven't initialized our own route as
        // responding peers are not going to be able to respond to us.
        if (myRoute == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Cannot issue a find route if we don\'t know our own route");
            }
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Find route for peer = " + peer);
        }

        try {
            // create a new RouteQuery message
            RouteQuery doc;

            // check if we have some bad route information
            // for that peer, in that case pass the bad hop count
            BadRoute badRoute;

            badRoute = router.getBadRoute(peer);

            if (badRoute != null) {
                // ok we have a bad route
                // pass the bad hops info as part of the query
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("findRoute sends query: known bad Hops" + badRoute);
                }
                doc = new RouteQuery(EndpointRouter.addr2pid(peer), myRoute, badRoute.getBadHops());
            } else {
                doc = new RouteQuery(EndpointRouter.addr2pid(peer), myRoute, null);
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sending query for peer : " + peer);
            }

            XMLDocument credentialDoc;
            CurrentCredential current = currentCredential;

            if (null != current) {
                credentialDoc = current.credentialDoc;
            } else {
                credentialDoc = null;
            }
          
            ResolverQuery query = new ResolverQuery(routerSName, credentialDoc, localPeerId.toString(), doc.toString(), qid.incrementAndGet());

            // only run SRDI if we are a rendezvous
            // FIXME 20060106 bondolo This is not dynamic enough. The route 
            // resolver needs to respond to changes in rendezvous configuration 
            // at runtime.
            if (group.isRendezvous()) {

                // check where to send the query via SRDI
                List<PeerID> results;

                if (srdiIndex != null) {
                    // try to find a least 10 entries, will pick up one
                    // randomly. This will protect against retry. It is
                    // likely that a number of RDV will know about a route
                    results = srdiIndex.query("route", RouteAdvertisement.DEST_PID_TAG, EndpointRouter.addr2pid(peer).toString(), 10);

                    if (results != null && !results.isEmpty()) {
                        // use SRDI to send the query
                        // remove any non rdv peers from the candidate list
                        // and garbage collect the index in the process
                        List<PeerID> clean = cleanupAnyEdges(query.getSrcPeer(), results);

                        if (!clean.isEmpty()) {
                            // The purpose of incrementing the hopcount
                            // when an SRDI index match is found (we got a
                            // pointer to a rdv that should have the route) is to
                            // restrict any further forwarding. The increment
                            // count is only done when a matching SRDI index is
                            // found. Not when the replica is selected as we
                            // still need to forward the query.  This restriction
                            // is purposelly done to avoid too many longjumps
                            // within a walk.
                            query.incrementHopCount();

                            srdi.forwardQuery(clean, query, 1);
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("found an srdi entry forwarding query to SRDI peer");
                            }
                            return;
                        }
                    } else {
                        // it is not in our cache, look for the replica peer
                        // we need to send the query
                        PeerID destPeer = srdi.getReplicaPeer(EndpointRouter.addr2pid(peer).toString());

                        if (destPeer != null && !destPeer.equals(localPeerId)) {
                            // don't push anywhere if we do not have a replica
                            // or we are trying to push to ourself
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("processQuery srdiIndex DHT forward :" + destPeer);
                            }

                            srdi.forwardQuery(destPeer, query);
                            return;
                        } else {
                            LOG.fine("processQuery srdiIndex DHT forward resulted in no op");
                        }
                    }
                }
            }

            // if we reach that point then we just use the resolver walk
            resolver = group.getResolverService();
            if (resolver != null) {
                resolver.sendQuery(null, query);
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("find route query sent");
                }
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("cannot get the resolver service");
                }
            }
        } catch (Exception ee) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception in findRoute", ee);
            }
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This is called by the Generic ResolverServiceImpl when processing a
     * response to a query.
     */
    public void processResponse(ResolverResponseMsg response) {

        if (!useRouteResolver) { // Route resolver disabled
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("processResponse got a response");
        }

        // convert the response into a RouteResponse
        RouteResponse doc = null;

        try {
            Reader ip = new StringReader(response.getResponse());

            XMLDocument asDoc = (XMLDocument)
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, ip);

            doc = new RouteResponse(asDoc);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "malformed response - discard", e);
            }
            return;
        }

        RouteAdvertisement dstRoute = doc.getDestRoute();
        RouteAdvertisement srcRoute = doc.getSrcRoute();
        int queryId = response.getQueryId();

        EndpointAddress routingPeer = EndpointRouter.pid2addr(srcRoute.getDestPeerID());
        EndpointAddress destPeer = EndpointRouter.pid2addr(dstRoute.getDestPeerID());

        // check if we have a negative route response
        if (queryId == NACKROUTE_QUERYID) {
            AccessPointAdvertisement badHop = dstRoute.nextHop(EndpointRouter.addr2pid(routingPeer));

            PeerID badPeer;

            if (badHop != null) {
                badPeer = badHop.getPeerID();
            } else { // the bad hop is the final destination
                badPeer = dstRoute.getDestPeerID();
            }

            processBadRoute(badPeer, dstRoute);
            return;
        }

        // This is not our own peer adv, so we must not keep it
        // for more than its expiration time.
        // we only need to publish this route if
        // we don't know about it yet
        // XXX: here is where we could be more conservative and use isNormallyReachable() instead, thus excluding
        // incoming messengers.
        if ((!router.isLocalRoute(EndpointRouter.pid2addr(srcRoute.getDestPeerID())))
                && (!router.isRoutedRoute(srcRoute.getDestPeerID()))) {
            router.updateRouteAdv(srcRoute);
        }

        if (destPeer.equals(routingPeer)) {
            // The dest peer itself managed to respond to us. That means we
            // learned the route from the reverseRoute in the message
            // itself. So, there's nothing we need to do.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("learn route directly from the destination");
            }
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("learn route:" + routingPeer);
            }

            try {
                // build the candidate route using the
                // route response from the respondant peer
                RouteAdvertisement candidateRoute = RouteAdvertisement.newRoute(EndpointRouter.addr2pid(destPeer),
                        EndpointRouter.addr2pid(routingPeer),(Vector) dstRoute.getVectorHops().clone());

                // cleanup the candidate route from any loop and remove the local peer extra
                // cycle
                RouteAdvertisement.cleanupLoop(candidateRoute, (PeerID) localPeerId);

                // Is there anything left in that route (or did the respondant
                // believe that we are the last hop on the route - which
                // obviously we are not.
                if (candidateRoute.size() == 0) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Route response outdated: NACK responder");
                    }
                    generateNACKRoute(EndpointRouter.addr2pid(routingPeer), EndpointRouter.addr2pid(destPeer), dstRoute.getVectorHops());
                    return;
                }

                // get the address of the first hop in the route to verify that
                // we have a route (direct or long) to the first hop, so the route
                // is valid
                EndpointAddress candidateRouter = EndpointRouter.pid2addr(candidateRoute.getFirstHop().getPeerID());

                // check that we have a direct connection to the first hop
                if (router.ensureLocalRoute(candidateRouter, null) == null) {
                    // If we do not have a direct route to the candidate router check
                    // for a long route in that case stich the route
                    RouteAdvertisement routeToRouter = router.getRoute(candidateRouter, false);

                    if (routeToRouter == null) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Route response useless: no route to next router hop");
                        }
                        return;
                    }

                    // stich the route removing any loops and localPeer cycle
                    if (RouteAdvertisement.stichRoute(candidateRoute, routeToRouter, (PeerID) localPeerId)) {
                        router.setRoute(candidateRoute, false);
                    } else {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Route response error stiching route response");
                        }
                        return;
                    }
                } else {
                    // we have a direct connection with the first hop of the candidate route
                    // set the new route, which starts with the peer that replied to us.
                    router.setRoute(candidateRoute, false);
                }
            } catch (Exception ex) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure building response route", ex);
                    LOG.warning("               bad dstRoute: " + dstRoute.display());
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("finish process route response successfully");
            }
        }
    }

    /**
     * bad route, so let's remove everything we have so
     * we can start from scratch. We are maintaining a
     * bad route up to DEFAULT_ROUTE expiration after
     * that we consider it to be ok to retry the same route
     * We are removing both the route and peer advertisement
     * to force a new route query
     *
     * @param badHop source PeerID of NACK route info
     * @param dest   original route information
     */
    private void processBadRoute(PeerID badHop, RouteAdvertisement dest) {

        EndpointAddress addr = EndpointRouter.pid2addr(dest.getDestPeerID());

        if (addr == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("remove bad route has a bad route info - discard");
            }
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("remove bad route info for dest " + dest.display());
            if (badHop != null) {
                LOG.fine("remove bad route bad hop " + badHop);
            }
        }

        try {

            // check first that we still have the same route, we may already
            // using a new route
            RouteAdvertisement currentRoute = router.getRoute(addr, false);

            if (currentRoute == null) { // we already cleanup the route info
                return;
            }

            // check if we still have the old bad route, we may have
            // already updated the route
            if (!currentRoute.equals(dest)) {

                // check if the bad hop is not the destination
                // if it is then we still have a bad route
                if (badHop == null) {
                    // we could get the bad hop, so consider the route ok
                    return;
                }
                if (badHop.equals(EndpointRouter.addr2pid(addr))) {
                    // check if the new route may still contain the bad hop
                    // the known bad hop is the hop after the src peer that
                    // responded with a NACK route
                    // In this case we also consider the route bad
                    if (!currentRoute.containsHop(badHop)) {
                        return; // we are ok
                    } else {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("current route is bad because it contains known bad hop" + badHop);
                        }
                    }
                } else {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("current route is bad because it contains known bad destination" + badHop);
                    }
                }

            }

            // keep the bad one in a cache table so we don't retry them
            // right away. We use the default route timeout
            BadRoute badRoute = (router.getBadRoute(addr));

            if (badRoute != null) {
                if (badRoute.getExpiration() > TimeUtils.timeNow()) {// nothing to do. the information is still valid
                } else {
                    // It is ancient knowlege update it
                    badRoute.setExpiration(TimeUtils.toAbsoluteTimeMillis(BADROUTE_EXPIRATION));
                }

                // check if we have to add a new bad hop
                // to our bad route
                if (badHop != null) {
                    badRoute.addBadHop(badHop);
                    badRoute.setExpiration(TimeUtils.toAbsoluteTimeMillis(BADROUTE_EXPIRATION));
                }

                router.setBadRoute(addr, badRoute);
                return;
            } else {
                // create a new NACK route entry
                Set<PeerID> badHops;

                if (badHop != null) {
                    badHops = Collections.singleton(badHop);
                } else {
                    badHops = Collections.emptySet();
                }

                badRoute = new BadRoute(dest, TimeUtils.toAbsoluteTimeMillis(BADROUTE_EXPIRATION), badHops);
                router.setBadRoute(addr, badRoute);
            }

            // remove route from route CM
            routeCM.flushRoute(EndpointRouter.addr2pid(addr));

            // let's remove the remote route info from the routing table
            // we do this after we removed the entries from the CM
            // to avoid that another thread is putting back the entry
            router.removeRoute(EndpointRouter.addr2pid(addr));
        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "exception during bad route removal", ex);
            }
        }
    }

    /**
     * Process the Query, and generate response
     *
     * @param query the query to process
     */
    public int processQuery(ResolverQueryMsg query) {

        if (!useRouteResolver) { // Route resolver disabled
            return ResolverService.OK;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("processQuery starts");
        }

        RouteQuery routeQuery;
        try {
            Reader ip = new StringReader(query.getQuery());
            XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, ip);
            routeQuery = new RouteQuery(asDoc);
        } catch (RuntimeException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Malformed Route query ", e);
            }
            return ResolverService.OK;
        } catch (IOException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Malformed Route query ", e);
            }
            return ResolverService.OK;
        } 

        PeerID pId = routeQuery.getDestPeerID();

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Looking for route to " + pId);
        }

        RouteAdvertisement srcRoute = routeQuery.getSrcRoute();
        Collection<PeerID> badHops = routeQuery.getBadHops();

        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            StringBuilder badHopsDump = new StringBuilder("bad Hops :\n");

            for (ID aBadHop : badHops) {
                badHopsDump.append('\t').append(aBadHop);
            }

            LOG.finer(badHopsDump.toString());
        }

        // if our source route is not null, then publish it
        if (srcRoute != null) {
            if (!(srcRoute.getDestPeerID()).equals(localPeerId)) {
                // This is not our own peer adv so we must not keep it
                // longer than its expiration time.
                try {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Publishing sender route info " + srcRoute.getDestPeerID());
                    }

                    // we only need to publish this route if
                    // we don't know about it yet
                    // XXX: here is where we could be more conservative and use isNormallyReachable() instead, thus excluding
                    // incoming messengers.
                    if ((!router.isLocalRoute(EndpointRouter.pid2addr(srcRoute.getDestPeerID())))
                            && (!router.isRoutedRoute(srcRoute.getDestPeerID()))) {
                        routeCM.publishRoute(srcRoute);
                    }
                } catch (Exception e) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Could not publish Route Adv from query - discard", e);
                    }
                    return ResolverService.OK;
                }
            }
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No src Route in route query - discard ");
            }
            return ResolverService.OK;
        }

        if (pId == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Malformed route query request, no PeerId - discard");
            }
            return ResolverService.OK;
        }

        // We have more luck with that one because, since it is part of OUR
        // message, and not part of the resolver protocol, it is in OUR
        // format.
        EndpointAddress qReqAddr = EndpointRouter.pid2addr(pId);

        RouteAdvertisement route;

        // check if this peer has a route to the destination
        // requested
        boolean found = false;

        if (qReqAddr.equals(localPeerAddr)) {
            found = true;
            // return the route that is my local route
            route = router.getMyLocalRoute();
        } else {
            // only rendezvous can respond to route requests
            // if not we are generating too much traffic
            // XXX: here is where we could be more conservative and use isNormallyReachable() instead, thus excluding
            // incoming messengers.
            if (router.isLocalRoute(qReqAddr)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Peer has direct route to destination ");
                }
                // we should set the route to something  :-)

                found = true;

                // this peer has a direct route to the destination
                // return the short route advertisement we know for this peer
                // (For us it is zero hop, and we advertise ourself as the routing
                // peer in the response. The stiching is done by whoever gets that
                // response). May be there are more than one hop advertised in-there...
                // alternate routing peers...should we leave them ?
                // For now, we keep the full dest, but wack the hops.

                route = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());

                AccessPointAdvertisement ap = (AccessPointAdvertisement)
                        AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());

                ap.setPeerID(pId);
                route.setDest(ap);
            } else {
                route = router.getRoute(qReqAddr, false);
                if (route != null) {
                    found = true;
                    // check if we were given some bad hops info
                    // and see if the found route contains
                    // any of these bad hops. In that case, we need
                    // to mark this route as bad
                    for (PeerID aBadHop : badHops) {
                        // destination is known to be bad
                        if (EndpointRouter.addr2pid(qReqAddr).equals(aBadHop)) {
                            processBadRoute(aBadHop, route);
                            found = false;
                            break;
                        }

                        if (route.containsHop(aBadHop)) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Peer has bad route due to " + aBadHop);
                            }
                            processBadRoute(aBadHop, route);
                            found = false;
                            break;
                        }
                    }
                }
            }
        }

        if (!found) {
            // discard the request if we are not a rendezvous
            // else forward to the next peers
            if (!group.isRendezvous()) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("discard query forwarding as not a rendezvous");
                }
                return ResolverService.OK;
            }

            // did not find a route, check our srdi cache
            // make sure we protect against out of sync
            // SRDI index

            // srdi forwarding is only involved once the Index entry has
            // been found and we forwarded the resolver query. Afterward a
            // normal walk proceeds from the initial SRDI index pointing
            // rdv. This is done to protect against potential loopback
            // entries in the SRDI cache index due to out of sync peerview
            // and index.
            if (query.getHopCount() < 2) {

                // check local SRDI cache to see if we have the entry
                // we look for 10 entries, will pickup one randomly
                List<PeerID> results = srdiIndex.query("route", RouteAdvertisement.DEST_PID_TAG, pId.toString(), 10);

                if (results.size() > 0) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("processQuery srdiIndex lookup match :" + results.size());
                    }

                    // remove any non-rdv peers to avoid sending
                    // to a non-rdv peers and garbage collect the SRDI
                    // index in the process
                    List<PeerID> clean = cleanupAnyEdges(query.getSrcPeer(), results);

                    if (clean.size() > 0) {

                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("found an srdi entry forwarding query to SRDI peer");
                        }

                        // The purpose of incrementing the hopcount
                        // when an SRDI index match is found (we got a
                        // pointer to a rdv that should have the route) is to
                        // restrict any further forwarding. The increment
                        // count is only done when a matching SRDI index is
                        // found. Not when the replica is selected as we
                        // still need to forward the query.  This restriction
                        // is purposelly done to avoid too many longjumps
                        // within a walk.
                        query.incrementHopCount();

                        // Note: this forwards the query to 1 peer randomly
                        // selected from the result
                        srdi.forwardQuery(clean, query, 1);

                        // tell the resolver no further action is needed.
                        return ResolverService.OK;
                    }
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("did not find a route or SRDI index");
            }

            // force a walk
            return ResolverService.Repropagate;
        }

        // we found a route send the response
        try {
            if (route == null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("we should have had a route at this point");
                }
                return ResolverService.OK;
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("we have a route build route response" + route.display());
            }

            RouteAdvertisement myRoute = router.getMyLocalRoute();

            // make sure we initialized our local
            // route info as we will need it to respond. We may
            // not have our route if we are still
            // waiting for a relay connection.
            if (myRoute == null) {
                return ResolverService.OK;
            }

            RouteResponse routeResponse = new RouteResponse();

            routeResponse.setDestRoute(route);
            routeResponse.setSrcRoute(myRoute);

            if (routeResponse == null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("error creating route response");
                }
                return ResolverService.OK;
            }

            // construct a response from the query
            ResolverResponseMsg res = query.makeResponse();

            CurrentCredential current = currentCredential;

            if (null != current) {
                res.setCredential(current.credentialDoc);
            }
            res.setResponse(routeResponse.toString());

            resolver.sendResponse(query.getSrcPeer().toString(), res);
            return ResolverService.OK;

        } catch (Exception ee) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "processQuery: error while processing query ", ee);
            }
            return ResolverService.OK;
        }
    }

    /**
     * Return a route error in case a route was found to be invalid
     * as the current hop cannot find a way to forward the message to the
     * destination or any other hops in the forward part of the route.
     * In that case a negative route response is forwarded
     * to the original source of the message. Now of course we
     * do not have any way to guarantee that the NACK message will be
     * received by the sender, but the best we can do is to try :-)
     * <p/>
     * we send a query ID to NACKROUTE_QUERYID to indicate
     * this is a bad Route
     *
     * @param src      original source of the message
     * @param dest     original destination of the message
     * @param origHops original hops
     */
    protected void generateNACKRoute(PeerID src, PeerID dest, Vector<AccessPointAdvertisement> origHops) {

        // As long as the group is partially initialized, do not bother
        // trying to send NACKS. We can't: it just causes NPEs.
        if (resolver == null) {
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("generate NACK Route response " + src);
        }

        // check first, if we are not already in process of looking for a
        // route to the destination peer of the NACK. We should not try to
        // send a NACK to that destination at that point as this will block
        // our incoming processing thread while it is looking for a route to
        // that destination. If there a no pending route requests to that
        // destination then we can go ahead an attempt to send the NACK. At
        // the maximum we should have only one thread block while looking for
        // a specific destination. When we find a route to the destination,
        // the next NACK processing will be sent.

        if (router.isPendingRouteQuery(src)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("drop NACK due to pending route discovery " + src);
            }
            return;
        }

        // Generate a route response
        RouteAdvertisement route = (RouteAdvertisement)
                AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());

        AccessPointAdvertisement ap = (AccessPointAdvertisement)
                AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());

        ap.setPeerID(dest);
        route.setDest(ap);
        route.setHops(origHops);

        // set the the route of the peer that
        // detected the bad route
        RouteAdvertisement routeSrc = (RouteAdvertisement)
                AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());

        AccessPointAdvertisement apSrc = (AccessPointAdvertisement)
                AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());

        apSrc.setPeerID((PeerID) localPeerId);
        routeSrc.setDest(apSrc);

        RouteResponse routeResponse = new RouteResponse();

        routeResponse.setDestRoute(route);
        routeResponse.setSrcRoute(routeSrc);
        
        ResolverResponse res = new ResolverResponse();

        res.setHandlerName(routerSName);
        
        CurrentCredential current = currentCredential;

        if (null != current) {
            res.setCredential(current.credentialDoc); 
        } 
        
        res.setQueryId(NACKROUTE_QUERYID);        
        res.setResponse(routeResponse.toString());

        // send the NACK response back to the originator
        resolver.sendResponse(src.toString(), res);
    }

    /**
     * process an SRDI message request
     *
     * @param message SRDI resolver message
     */
    public boolean processSrdi(ResolverSrdiMsg message) {
        if(!group.isRendezvous()) {
            return true;
        }
        
        String value;
        SrdiMessage srdiMsg;

        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Received a SRDI messsage in group" + group.getPeerGroupName());
            }

            XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, new StringReader(message.getPayload()));
            srdiMsg = new SrdiMessageImpl(asDoc);
        } catch (Exception e) {
            // we don't understand this msg, let's skip it
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "corrupted SRDI message", e);
            }
            return false;
        }

        PeerID pid = srdiMsg.getPeerID();

        // filter messages that contain messages
        // about the local peer, so we don't enter
        // self-reference
        if (pid.equals(localPeerId)) {
            return false;
        }

        for (SrdiMessage.Entry entry : srdiMsg.getEntries()) {
            // drop any information  about ourself
            if (entry.key.equals(localPeerId.toString())) {
                continue;
            }
            value = entry.value;
            if (value == null) {
                value = "";
            }

            // Expiration of entries is taken care of by SrdiIdex, so we always add
            // FIXME hamada 20030314
            // All routes are added under the secondary key 'DstPID', it would be more correct to
            // Specify it in the message, but since versioning is not yet supported the following is
            // acceptable, since it is localized
            srdiIndex.add(srdiMsg.getPrimaryKey(), RouteAdvertisement.DEST_PID_TAG, entry.key, pid, entry.expiration);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Primary Key [" + srdiMsg.getPrimaryKey() + "] key [RouteAdvertisement.DEST_PID_TAG]" + " value [" + entry.key + "] exp [" + entry.expiration + "]");
            }
        }

        return true;
    }

    /**
     * {@inheritDoc}
     */
    public void pushEntries(boolean all) {
        // only send to the replica
        pushSrdi(null, all);
    }

    /*
     * push all srdi entries to the rednezvous SRDI cache (new connection)
     *
     *@param all if true push all entries, otherwise just deltas
     */
    protected void pushSrdi(String peer, boolean all) {

        SrdiMessage srdiMsg;
        Vector<SrdiMessage.Entry> routeIx = new Vector<SrdiMessage.Entry>();

        // 20021018 tra:Route info don't expire unless the peer disappears
        // This approach is used to limit the SRDI traffic. The key
        // point here is that SRDI is used to tell a peer that another
        // has a route to the destination it is looking for. The information
        // that SRDI cache is not so much the specific route info but rather
        // the fact that a peer has knowledge of a route to another peer
        // We don't want to update the SRDI cache on every route update.
        // The SRDI cache will be flushed when the peer disconnect from
        // the rendezvous.

        // We cannot support concurrent modification of the map while we
        // do that: we must synchronize...
        for (Iterator<ID> each = router.getAllRoutedRouteAddresses(); each.hasNext();) {
            ID pid = each.next();
            SrdiMessage.Entry entry = new SrdiMessage.Entry(pid.toString(), "", Long.MAX_VALUE);
            routeIx.addElement(entry);
        }

        try {
            // check if we have anything to send
            if (routeIx.size() == 0) {
                return;
            }

            srdiMsg = new SrdiMessageImpl(group.getPeerID(),
                    // one hop
                    1,
                    "route", routeIx);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sending a SRDI messsage of [All=" + all + "] routes");
            }
            // this will replicate entry to the  SRDI replica peers
            srdi.replicateEntries(srdiMsg);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "SRDI Push failed", e);
            }
        }
    }

    /*
     * push srdi entries to the SRDI rendezvous cache
     * @param all if true push all entries, otherwise just deltas
     */
    protected void pushSrdi(ID peer, PeerID id) {

        SrdiMessage srdiMsg;

        try {
            srdiMsg = new SrdiMessageImpl(group.getPeerID(), 1, // only one hop
                    "route", id.toString(), null, Long.MAX_VALUE); // maximum expiration
            // 20021018 tra:Route info don't expire unless the peer disappears
            // This approach is used to limit the SRDI traffic. The key
            // point here is that SRDI is used to tell a peer that another
            // has a route to the destination it is looking for. The information
            // that SRDI cache is not so much the specific route info but rather
            // the fact that a peer has knowledge of a route to another peer
            // We don't want to update the SRDI cache on every route update.
            // The SRDI cache will be flushed when the peer disconnect from
            // the rendezvous.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("sending a router SRDI message add route " + id);
            }
            if (peer == null) {
                PeerID destPeer = srdi.getReplicaPeer(id.toString());
                peer = destPeer;
            }
            // don't push anywhere if we do not have a replica
            // or we are trying to send the query to ourself
            if (!localPeerId.equals(peer)) {
                srdi.pushSrdi(peer, srdiMsg);
            }
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "SRDI push failed", e);
            }
        }
    }

    /**
     * remove a SRDI cache entry
     *
     * @param peer peer id we send the request, null for sending to all
     * @param id   peer id of the SRDI route that we want to remove
     *             from the cache
     */
    protected void removeSrdi(String peer, PeerID id) {

        SrdiMessage srdiMsg;

        try {
            srdiMsg = new SrdiMessageImpl(group.getPeerID(), 1, // only one hop
                    "route", id.toString(), null, // 0 means remove
                    0);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("sending a router SRDI message delete route " + id);
            }

            if (peer == null) {
                PeerID destPeer = srdi.getReplicaPeer(id.toString());

                // don't push anywhere if we do not have replica
                // or we are trying to push to ouself
                if (destPeer != null && (!destPeer.equals(localPeerId))) {
                    srdi.pushSrdi(destPeer, srdiMsg);
                }
            }
        } catch (Exception e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Removing srdi entry failed", e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void messageSendFailed(PeerID peerid, OutgoingMessageEvent e) {
        // when the resolver failed to send, we get a notification and
        // flush the SRDI cache entries for that destination
        removeSrdiIndex(peerid);
    }

    /**
     * cleanup any edge peers when trying to forward an SRDI query so we are
     * guaranteed to the best of our knowledge that the peer is a rendezvous.
     * This is not perfect, as it may take time for the peerview to converge but
     * at least we can remove any peers that is not a rendezvous.
     *
     * @param src     source
     * @param results vector of PeerIDs
     * @return cleaned up vector of PeerIDs
     */
    protected List<PeerID> cleanupAnyEdges(ID src, List<PeerID> results) {
        List<PeerID> clean = new ArrayList<PeerID>(results.size());

        // put the peerview as a vector of PIDs
        List<PeerID> rpvId = srdi.getGlobalPeerView();

        // remove any peers not in the current peerview
        // these peers may be gone or have become edges
        for (PeerID pid : results) {
            // eliminate the src of the query so we don't resend
            // the query to whom send it to us
            if (src.equals(pid)) {
                continue;
            }
            // remove the local also, so we don't send to ourself
            if (localPeerId.equals(pid)) {
                continue;
            }
            if (rpvId.contains(pid)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("valid rdv for SRDI forward " + pid);
                }
                clean.add(pid);
            } else {
                // cleanup our SRDI cache for that peer
                srdiIndex.remove(pid);
            }
        }
        return clean;
    }

    /**
     * remove SRDI index
     *
     * @param pid of the index to be removed
     */
    protected void removeSrdiIndex(PeerID pid) {
        srdiIndex.remove(pid);
    }
}
