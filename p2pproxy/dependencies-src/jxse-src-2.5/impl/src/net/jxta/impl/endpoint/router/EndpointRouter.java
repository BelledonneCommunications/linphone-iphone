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

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageReceiver;
import net.jxta.endpoint.MessageSender;
import net.jxta.endpoint.MessageTransport;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.MessengerEvent;
import net.jxta.endpoint.MessengerEventListener;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.endpoint.LoopbackMessenger;
import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.util.TimerThreadNamer;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.AccessPointAdvertisement;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.RouteAdvertisement;
import net.jxta.service.Service;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

public class EndpointRouter implements EndpointListener, MessageReceiver, MessageSender, MessengerEventListener, Module {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(EndpointRouter.class.getName());

    /**
     * Until we decide otherwise, the router is *by definition* handling
     * virtually addressed messages.
     */
    private final static String ROUTER_PROTOCOL_NAME = "jxta";

    /**
     * Router Service Name
     */
    private final static String ROUTER_SERVICE_NAME = "EndpointRouter";

    /**
     * how long we are willing to wait for a response from an async
     * getMessenger. We do not wait long at all because it is non-critical
     * that we get the answer synchronously. The goal is to avoid starting
     * a route discovery if there's a chance to get a direct connection.
     * However, we will still take advantage of the direct route if it is
     * found while we wait for the route discovery result. If that happens,
     * the only wrong is that we used some bandwidth doing a route discovery
     * that wasn't needed after all.
     */
    public final static long ASYNC_MESSENGER_WAIT = 3L * TimeUtils.ASECOND;

    /**
     * MessageTransport Control operation
     */
    public final static Integer GET_ROUTE_CONTROL = 0; // Return RouteControl Object
    public final static int RouteControlOp = 0; // Return RouteControl Object

    /**
     * MAX timeout (seconds) for route discovery after that timeout
     * the peer will bail out from finding a route
     */
    private final static long MAX_FINDROUTE_TIMEOUT = 60L * TimeUtils.ASECOND;

    /**
     * How long do we wait (seconds) before retrying to make a connection
     * to an endpoint destination
     */
    private final static long MAX_ASYNC_GETMESSENGER_RETRY = 30L * TimeUtils.ASECOND;

    /**
     * These are peers which we know multi-hop routes for.
     */
    private final Map<ID, RouteAdvertisement> routedRoutes = new HashMap<ID, RouteAdvertisement>(16);

    /**
     * A record of failures.
     * <p/>
     * Values are the time of failure as {@link java.lang.Long}. If
     * {@code Long.MAX_VALUE} then a connect attempt is current in progress.
     */
    private final Map<PeerID, Long> triedAndFailed = new HashMap<PeerID, Long>();

    /**
     * local peer ID as an endpointAddress.
     */
    private EndpointAddress localPeerAddr = null;

    /**
     * local Peer ID
     */
    private PeerID localPeerId = null;

    /**
     * The Endpoint Service we are routing for.
     */
    private EndpointService endpoint = null;

    /**
     * PeerGroup handle
     */
    private PeerGroup group = null;

    private ID assignedID = null;
    
    /**
     * If {@code true} this service has been closed.
     */
    private boolean stopped = false;

    /**
     * Whenever we initiate connectivity to a peer (creating a direct route).
     * we remember that we need to send our route adv to that peer. So that
     * it has a chance to re-establish the connection from its side, if need
     * be. The route adv is actually sent piggy-backed on the first message
     * that goes there.
     */
    private final Set<EndpointAddress> newDestinations = Collections.synchronizedSet(new HashSet<EndpointAddress>());

    /**
     * A pool of messengers categorized by logical address.
     * This actually is the direct routes map.
     */
    private Destinations destinations;

    /**
     * A record of expiration time of known bad routes we received a NACK route
     */
    private final Map<EndpointAddress, BadRoute> badRoutes = new HashMap<EndpointAddress, BadRoute>();

    /**
     * We record queries when first started and keep them pending for
     * a while. Threads coming in the meanwhile wait for a result without
     * initiating a query. Thus threads may wait passed the point where
     * the query is no-longer pending, and, although they could initiate
     * a new one, they do not.
     * <p/>
     * However, other threads coming later may initiate a new query. So a
     * query is not re-initiated systematically on a fixed schedule. This
     * mechanism also serves to avoid infinite recursions if we're looking
     * for the route to a rendezvous (route queries will try to go there
     * themselves).
     * <p/>
     * FIXME: jice@jxta.org 20020903 this is approximate. We can do
     * cleaner/better than this, but it's already an inexpensive improvement
     * over what used before.
     * <p/>
     * FIXME: tra@jxta.org 20030818 the pending hashmap should be moved
     * in the routeResolver class.
     */
    private final Map<PeerID, ClearPendingQuery> pendingQueries = 
            Collections.synchronizedMap(new HashMap<PeerID, ClearPendingQuery>());

    /**
     * Timer by which we schedule the clearing of pending queries.
     */
    private final Timer timer = new Timer("EndpointRouter Timer", true);

    /**
     * PeerAdv tracking.
     * The peer adv is modified every time a new public address is
     * enabled/disabled. One of such cases is the connection/disconnection
     * from a relay. Since these changes are to the embedded route adv
     * and since we may embed our route adv in messages, we must keep it
     * up-to-date.
     */
    private PeerAdvertisement lastPeerAdv = null;
    private int lastModCount = -1;

    /**
     * Route info for the local peer (updated along with lastPeerAdv).
     */
    private RouteAdvertisement localRoute = null;

    /**
     * Route CM persistent cache
     */
    private RouteCM routeCM = null;

    /**
     * Route Resolver
     */
    private RouteResolver routeResolver;

    class ClearPendingQuery extends TimerTask {
        final PeerID peerID;
        volatile boolean failed = false;
        long nextRouteResolveAt = 0;

        ClearPendingQuery(PeerID peerID) {
            this.peerID = peerID;
            // We schedule for one tick at one minute and another at 5 minutes
            // after the second, we cancel ourselves.
            timer.schedule(this, TimeUtils.AMINUTE, 5L * TimeUtils.AMINUTE);
            nextRouteResolveAt = TimeUtils.toAbsoluteTimeMillis(20L * TimeUtils.ASECOND);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void run() {
            try {
                if (failed) {
                    // Second tick.
                    // This negative cache info is expired.
                    pendingQueries.remove(peerID);

                    this.cancel();
                } else {
                    // First timer tick. We're done trying. This is now a negative
                    // cache info. For the next 5 minutes that destination fails
                    // immediately unless it unexpectedly gets finaly resolved.
                    failed = true;
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in timer task " + Thread.currentThread().getName() + " for " + peerID, all);
                }
            }
        }

        public synchronized boolean isTimeToResolveRoute() {
            if (TimeUtils.toRelativeTimeMillis(nextRouteResolveAt) > 0) {
                return false;
            }
            // nextRouteResolveAt is passed. Set the next time to retry from now.
            nextRouteResolveAt = TimeUtils.toAbsoluteTimeMillis(20L * TimeUtils.ASECOND);
            return true;
        }

        public boolean isFailed() {
            return failed;
        }
    }

    RouteAdvertisement getMyLocalRoute() {

        // Update our idea of the local peer adv. If it has change,
        // update our idea of the local route adv.
        // If nothing has changed, do not do any work.
        // In either case, return the local route adv as it is after this
        // refresh.

        // Race condition possible but tolerable: if two threads discover
        // the change in the same time, lastPeerAdv and lastModCount
        // could become inconsistent. That'll be straightened out the
        // next time someone looks. The inconsistency can only trigger
        // an extraneous update.

        PeerAdvertisement newPadv = group.getPeerAdvertisement();
        int newModCount = newPadv.getModCount();

        if ((lastPeerAdv != newPadv) || (lastModCount != newModCount) || (null == localRoute)) {
            lastPeerAdv = newPadv;
            lastModCount = newModCount;
        } else {
            // The current version is good.
            return localRoute;
        }

        // Get its EndpointService advertisement
        XMLElement endpParam = (XMLElement)
                newPadv.getServiceParam(PeerGroup.endpointClassID);

        if (endpParam == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("no Endpoint SVC Params");
            }

            // Return whatever we had so far.
            return localRoute;
        }

        // get the Route Advertisement element
        Enumeration paramChilds = endpParam.getChildren(RouteAdvertisement.getAdvertisementType());
        XMLElement param;

        if (paramChilds.hasMoreElements()) {
            param = (XMLElement) paramChilds.nextElement();
        } else {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("no Endpoint Route Adv");
            }

            // Return whatever we had so far.
            return localRoute;
        }

        // build the new route
        try {
            // Stick the localPeerID in-there, since that was what
            // every single caller of getMyLocalRoute did so far.

            RouteAdvertisement route = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(param);

            route.setDestPeerID(localPeerId);
            localRoute = route;
        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure extracting route", ex);
            }
        }

        return localRoute;
    }

    /**
     * listener object to synchronize on asynchronous getMessenger
     */
    private static class EndpointGetMessengerAsyncListener implements MessengerEventListener {

        private final EndpointRouter router;
        private final EndpointAddress logDest;

        volatile boolean hasResponse = false;
        volatile boolean isGone = false;
        private Messenger messenger = null;

        /**
         * Constructor
         *
         * @param router the router
         * @param dest   logical destination
         */
        EndpointGetMessengerAsyncListener(EndpointRouter router, EndpointAddress dest) {
            this.router = router;
            this.logDest = dest;
        }

        /**
         * {@inheritDoc}
         */
        public boolean messengerReady(MessengerEvent event) {

            Messenger toClose = null;

            synchronized (this) {
                hasResponse = true;
                if (event != null) {
                    messenger = event.getMessenger();
                    if (null != messenger) {
                        if(!logDest.equals(messenger.getLogicalDestinationAddress())) {
                            // Ooops, wrong number !
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.warning("Incorrect Messenger logical destination : " + logDest + "!=" + messenger.getLogicalDestinationAddress());
                            }

                            toClose = messenger;
                            messenger = null;
                        }
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("null messenger for dest :" + logDest);
                        }
                    }
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("null messenger event for dest :" + logDest);
                    }
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                if (messenger == null) {
                    LOG.fine("error creating messenger for dest :" + logDest);
                } else {
                    LOG.fine("got a new messenger for dest :" + logDest);
                }
            }

            // We had to release the lock on THIS before we can get the lock on
            // the router. (Or face a dead lock - we treat this as a lower level
            // lock)

            if (messenger == null) {
                if (toClose != null) {
                    toClose.close();
                }

                // we failed to get a messenger, we need to update the try and
                // failed as it currently holds an infinite timeout to permit
                // another thread to retry that destination. We only retry
                // every MAX_ASYNC_GETMESSENGER_RETRY seconds

                router.noMessenger(logDest);

                synchronized (this) {
                    // Only thing that can happen is that we notify for nothing
                    // We took the lock when updating hasResult, so, the event
                    // will not be missed. 
                    // FIXME It would be more logical to let the waiter do the 
                    // above if (!isGone) as in the case of success below. 
                    // However, we'll minimize changes for risk management 
                    // reasons.
                    notify();
                }
                return false;
            }

            // It worked. Update router cache entry if we have to.

            synchronized (this) {
                if (!isGone) {
                    notify(); // Waiter will do the rest.
                    return true;
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("async caller gone add the messenger " + logDest);
            }
            return router.newMessenger(event);
        }

        /**
         * Wait on the async call for ASYNC_MESSENGER_WAIT
         * then bailout. The messenger will be added whenever
         * the async getMessenger will return
         *
         * @param quick if true return a messenger immediately if available,
         *              otherwise wait the Messenger resolution to be completed
         * @return the Messenger if one available
         */
        public synchronized Messenger waitForMessenger(boolean quick) {
            if (!quick) {
                long quitAt = TimeUtils.toAbsoluteTimeMillis(ASYNC_MESSENGER_WAIT);

                while (TimeUtils.toRelativeTimeMillis(quitAt) > 0) {
                    try {
                        // check if we got a response already
                        if (hasResponse) { // ok, we got a response
                            break;
                        }
                        wait(ASYNC_MESSENGER_WAIT);
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                        break;
                    }
                }
            }

            // mark the fact that the caller is bailing out
            isGone = true;
            return messenger;
        }
    }

    /**
     * isLocalRoute is a shallow test. It tells you that there used to be a
     * local route that worked the last time it was tried.
     *
     * @param peerAddress Address of the destination who's route is desired.
     * @return {@code true} if we know a direct route to the specified address
     *         otherwise {@code false}.
     */
    boolean isLocalRoute(EndpointAddress peerAddress) {
        return destinations.isCurrentlyReachable(peerAddress);
    }

    /**
     * Get a Messenger for the specified destination if a direct route is known.
     *
     * @param peerAddress The peer who's messenger is desired.
     * @param hint        A route hint to use if a new Messenger must be created.
     * @return Messenger for direct route or {@code null} if none could be
     *         found or created.
     */
    Messenger ensureLocalRoute(EndpointAddress peerAddress, RouteAdvertisement hint) {

        // We need to make sure that there is a possible connection to that peer
        // If we have a decent (not closed, busy or available) transport 
        // messenger in the pool, then we're done. Else we activly try to make 
        // one.

        // See if we already have a messenger.
        Messenger messenger = destinations.getCurrentMessenger(peerAddress);

        if (messenger != null) {
            return messenger;
        }

        // Ok, try and make one. Pass the route hint info
        messenger = findReachableEndpoint(peerAddress, false, hint);
        if (messenger == null) {
            // We must also zap it from our positive cache: if we remembered it 
            // working, we should think again.
            destinations.noOutgoingMessenger(peerAddress);
            return null; // No way.
        }

        destinations.addOutgoingMessenger(peerAddress, messenger);

        // We realy did bring something new. Give relief to those that have been
        // waiting for it.
        synchronized (this) {
            notifyAll();
        }

        // NOTE to maintainers: Do not remove any negative cache info
        // or route here. It is being managed by lower-level routines.
        // The presence of a messenger in the pool has many origins,
        // each case is different and taken care of by lower level
        // routines.
        return messenger;
    }

    /**
     * Send a message to a given logical destination if it maps to some
     * messenger in our messenger pool or if such a mapping can be found and
     * added.
     *
     * @param destination peer-based address to send the message to.
     * @param message     the message to be sent.
     * @throws java.io.IOException if an io error occurs
     */
    void sendOnLocalRoute(EndpointAddress destination, Message message) throws IOException {

        IOException lastIoe = null;
        Messenger sendVia;

        // Try sending the message as long as we get have transport messengers
        // to try with. They close when they fail, which puts them out of cache
        // or pool if any, so we will not see a broken one a second time. We'll
        // try the next one until we run out of options.

        while ((sendVia = ensureLocalRoute(destination, null)) != null) {

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sending " + message + " to " + destination + " via " + sendVia);
            }

            try {
                // FIXME 20040413 jice Maybe we should use the non-blocking mode
                // and let excess messages be dropped given the threading issue
                // still existing in the input circuit (while routing messages
                // through).

                sendVia.sendMessageB(message, EndpointRouter.ROUTER_SERVICE_NAME, null);

                // If we reached that point, we're done.
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Sent " + message + " to " + destination);
                }
                return;

            } catch (IOException ioe) {
                // Can try again, with another messenger (most likely).
                lastIoe = ioe;
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Trying next messenger to " + destination);
            }
            // try the next messenger if there is one.
        }

        // Now see why we're here.
        // If we're here for no other reason than failing to get a messenger
        // say so. Otherwise, report the failure from the last time we tried.
        if (lastIoe == null) {
            lastIoe = new IOException("No reachable endpoints for " + destination);
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.log(Level.FINE, "Could not send to " + destination, lastIoe);
        }

        throw lastIoe;
    }

    /**
     * Default constructor
     */
    public EndpointRouter() {
    }

    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {

        timer.schedule(new TimerThreadNamer("EndpointRouter Timer for " + group.getPeerGroupID()), 0);

        this.group = group;
        this.assignedID = assignedID;
        ModuleImplAdvertisement implAdvertisement = (ModuleImplAdvertisement) impl;

        localPeerId = group.getPeerID();
        localPeerAddr = pid2addr(group.getPeerID());

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Router Transport : " + assignedID);

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
            configInfo.append("\n\t\tProtocol : ").append(getProtocolName());
            configInfo.append("\n\t\tPublic Address : ").append(localPeerAddr);

            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized int startApp(String[] arg) {
        endpoint = group.getEndpointService();

        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }
            return START_AGAIN_STALLED;
        }

        Service needed = group.getResolverService();

        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Endpoint Router start stalled until resolver service available");
            }
            return Module.START_AGAIN_STALLED;
        }

        needed = group.getMembershipService();

        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Endpoint Router start stalled until membership service available");
            }
            return Module.START_AGAIN_STALLED;
        }

        needed = group.getRendezVousService();

        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Endpoint Router start stalled until rendezvous service available");
            }
            return Module.START_AGAIN_STALLED;
        }

        destinations = new Destinations(endpoint);

        try {
            // FIXME tra 20030818  Should be loaded as a service
            // when we have service dependency. When loaded as a true service should
            // not have to pass the EnpointRouter object. The issue is we need an
            // api to obtain the real object from the PeerGroup API.
            routeCM = new RouteCM();

            // FIXME tra 20030818  Should be loaded as a service
            // when we have service dependency. When loaded as a true service should
            // not have to pass the EnpointRouter object. The issue is we need an
            // api to obtain the real object from the PeerGroup API.
            routeResolver = new RouteResolver(this);

            // initialize persistent CM route Cache
            // FIXME tra 20030818 Should be loaded as service when complete
            // refactoring is done.
            routeCM.init(group, assignedID, null);

            // initialize the route resolver
            // FIXME tra 20030818 Should be loaded as service when complete
            // refactoring is done.
            routeResolver.init(group, assignedID, null);
        } catch (PeerGroupException failure) {
            return -1;
        }

        int status;

        // FIXME tra 20031015 Should be started as a service when refactored work
        // completed
        status = routeCM.startApp(arg);
        if (status != 0) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Route CM failed to start : " + status);
            }

            return status;
        }

        // FIXME tra 20031015 Should be started as a service when refactored work
        // completed
        status = routeResolver.startApp(arg);
        if (status != 0) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Route Resolver failed to start : " + status);
            }
            return status;
        }
        // publish my local route adv
        routeCM.publishRoute(getMyLocalRoute());

        // FIXME tra 20031015 is there a risk for double registration  when 
        // startApp() is recalled due to failure to get the discovery service
        // by the Route Resolver service.

        // NOTE: Endpoint needs to be registered before we register the endpoint 
        // resolver. This is bringing a more complex issue of service loading 
        // dependencies.
        endpoint.addMessengerEventListener(this, EndpointService.MediumPrecedence);

        endpoint.addIncomingMessageListener(this, ROUTER_SERVICE_NAME, null);

        if (endpoint.addMessageTransport(this) == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Transport registration refused");
            }

            return -1;
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info(group + " : Router Message Transport started.");
        }
        return status;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Careful that stopApp() could in theory be called before startApp().
     */
    public synchronized void stopApp() {
        stopped = true;
        
        if (endpoint != null) {
            endpoint.removeIncomingMessageListener(ROUTER_SERVICE_NAME, null);
            endpoint.removeMessengerEventListener(this, EndpointService.MediumPrecedence);
            endpoint.removeMessageTransport(this);
        }

        // FIXME tra 20030818 should be unloaded as a service
        routeCM.stopApp();

        // FIXME tra 20030818 should be unloaded as a service
        routeResolver.stopApp();

        destinations.close();
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info(group + " : Router Message Transport stopped.");
        }
    }

    /**
     * {@inheritDoc}
     */
    public boolean isConnectionOriented() {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public boolean allowsRouting() {
        // Yes, this is the router, and it does not allow routing.
        // Otherwise we would have a chicken and egg problem.
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        return endpoint;
    }

    /**
     * {@inheritDoc}
     */
    public EndpointAddress getPublicAddress() {
        return localPeerAddr;
    }

    /**
     * {@inheritDoc}
     */
    public Iterator<EndpointAddress> getPublicAddresses() {
        return Collections.singletonList(getPublicAddress()).iterator();
    }

    /**
     * {@inheritDoc}
     */
    public String getProtocolName() {
        return ROUTER_PROTOCOL_NAME;
    }

    /**
     * Given a peer id, return an address to reach that peer.
     * The address may be for a directly reachable peer, or
     * for the first gateway along a route to reach the peer.
     * If we do not have a route to the peer, we will use the
     * Peer Routing Protocol to try to discover one.  We will
     * wait up to 30 seconds for a route to be discovered.
     *
     * @param peerAddress the peer we are trying to reach.
     * @param seekRoute   whether to go as far as issuing a route query, or just fish in our cache.
     *                    when forwarding a message we allow ourselves to mend a broken source-issued route but we
     *                    won't go as far as seeking one from other peers. When originating a message, on the other end
     *                    we will aggressively try to find route.
     * @param hint        whether we are passed a route hint to be used, in that case that route
     *                    hint should be used
     * @return an EndpointAddress at which that peer should be reachable.
     */
    EndpointAddress getGatewayAddress(EndpointAddress peerAddress, boolean seekRoute, RouteAdvertisement hint) {
        PeerID peerID = addr2pid(peerAddress);

        try {
            // FIXME 20021215 jice Replace this junk with a background task; 
            // separate the timings of route disco from the timeouts of
            // the requesting threads. EndpointAddress result = null;

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Searching local" + (seekRoute ? " & remote" : "") + " for route for " + peerAddress);
            }

            // If we can't get a route within the timeout, give up for now.
            long quitAt = TimeUtils.toAbsoluteTimeMillis(MAX_FINDROUTE_TIMEOUT);

            // Time we need to wait before we can start issue a find route request
            // to give a chance for the async messenger to respond (success or failure)
            long findRouteAt = TimeUtils.toAbsoluteTimeMillis(ASYNC_MESSENGER_WAIT);

            EndpointAddress addr;

            while (TimeUtils.toRelativeTimeMillis(quitAt) > 0) {
                // Then check if by any chance we can talk to it directly.
                Messenger directMessenger = ensureLocalRoute(peerAddress, hint);

                if (null != directMessenger) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Found direct route for " + peerAddress + " via " + directMessenger.getDestinationAddress());
                    }
                    return peerAddress;
                }

                // Otherwise, look for a long route.
                // check if we got a hint. If that's the case use it
                RouteAdvertisement route;

                if (hint != null) {
                    route = hint;
                } else {
                    route = getRoute(peerAddress, seekRoute);
                }

                if (route != null && route.size() > 0) {
                    addr = pid2addr(route.getLastHop().getPeerID());
                    if (ensureLocalRoute(addr, null) != null) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Found last hop remote address: " + peerAddress + " -> " + route.getLastHop().getPeerID());
                        }

                        // Ensure local route removes negative cache info about
                        // addr. We also need to remove that about peerAddress.
                        return addr;
                    } else { // need to try the first hop
                        addr = pid2addr(route.getFirstHop().getPeerID());

                        if (ensureLocalRoute(addr, null) != null) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Found first hop remote address first hop: " + peerAddress + " -> "
                                          + route.getFirstHop().getPeerID());
                            }

                            // Ensure local route removes negative cache info about addr.
                            return addr;
                        } else {
                            removeRoute(peerID);
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Found no reachable route to " + peerAddress);
                            }
                        }
                    }
                }

                // For messages we didn't originate we don't seek routes.
                if (!seekRoute) {
                    break;
                }

                // Check that route resolution is enabled if
                // not then bail out, there is nothing more
                // that we can do.
                if (!routeResolver.useRouteResolver()) {
                    break;
                }

                // due to the asynchronous nature of getting our messenger we
                // need to handle the multi-entrance of issueing a route
                // discovery. A route discovery needs to be generated only
                // either if we have no pending request (it completed or we had
                // no information so we did not created one), or we tried and
                // we failed, or we waited at least ASYNC_MESSENGER_WAIT to get
                // a chance for the async request to respond before we can
                // issue the route discovery
                Long nextTry = triedAndFailed.get(peerID);

                if ((nextTry == null) || (nextTry < TimeUtils.toAbsoluteTimeMillis(MAX_ASYNC_GETMESSENGER_RETRY))
                        || (TimeUtils.toRelativeTimeMillis(findRouteAt) <= 0)) {

                    // If it is already hopeless (negative cache), just give up.
                    // Otherwise, try and recover the route. If a query is not
                    // already pending, we may trigger a route discovery before we
                    // wait. Else, just wait. The main problem we have here is that
                    // the same may re-enter because the resolver query sent by
                    // findRoute ends up with the rendezvous service trying to
                    // resolve the same destiation if the destination  happens to be
                    // the start of the walk. In that situation we will re-enter
                    // at every findRoute attempt until the query becomes "failed".
                    // However, we do want to do more than one findRoute because
                    // just one attempt can fail for totaly fortuitous or temporary
                    // reasons. A tradeoff is to do a very limitted number of attempts
                    // but still more than one. Over the minute for which the query
                    // is not failed, isTimeToRety will return true at most twice
                    // so that'll be a total of three attempts: once every 20 seconds.
                    boolean doFind = false;
                    ClearPendingQuery t;

                    synchronized (pendingQueries) {
                        t = pendingQueries.get(peerID);

                        if (t == null) {
                            doFind = true;
                            t = new ClearPendingQuery(peerID);
                            pendingQueries.put(peerID, t);
                        } else {
                            if (t.isFailed()) {
                                break;
                            }
                            if (t.isTimeToResolveRoute()) {
                                doFind = true;
                            }
                        }
                    }

                    // protect against the async messenger request. We only
                    // look for a route after the first iteration by
                    // that time we will have bailed out from the async call
                    if (doFind) {
                        routeResolver.findRoute(peerAddress);
                        // we do not need to check the CM, route table will
                        // be updated when the route response arrive. This reduces
                        // CM activities when we wait for the route response
                        seekRoute = false;
                    }
                }

                // Now, wait. Responses to our query may occur asynchronously.
                // threads.
                synchronized (this) {
                    // We can't possibly do everything above while synchronized,
                    // so we could miss an event of interrest. But some changes
                    // are not readily noticeable anyway, so we must wake up
                    // every so often to retry.
                    try {
                        // we only need to wait if we haven't got a messenger
                        // yet.
                        if (destinations.getCurrentMessenger(peerAddress) == null) {
                            wait(ASYNC_MESSENGER_WAIT);
                        }
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                    }
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No route to " + peerAddress);
            }
            return null;

        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "getGatewayAddress exception", ex);
            }
            return null;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    public boolean ping(EndpointAddress addr) {

        EndpointAddress plainAddr = new EndpointAddress(addr, null, null);

        try {
            return (getGatewayAddress(plainAddr, true, null) != null);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.FINE, "Ping failure (exception) for : " + plainAddr, e);
            }
            return false;
        }
    }

    /**
     * Receives notifications of new messengers being generated by the
     * underlying network transports.
     * <p/>
     * IMPORTANT: Incoming messengers only. If/when this is used for
     * outgoing, some things have to change:
     * <p/>
     * For example we do not need to send the welcome msg, but for
     * outgoing messengers, we would need to.
     * <p/>
     * Currently, newMessenger handles the outgoing side.
     *
     * @param event the new messenger event.
     */
    public boolean messengerReady(MessengerEvent event) {

        Messenger messenger = event.getMessenger();
        Object source = event.getSource();
        EndpointAddress logDest = messenger.getLogicalDestinationAddress();

        if (source instanceof MessageSender && !((MessageSender) source).allowsRouting()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Ignoring messenger to :" + logDest);
            }
            return false;
        }

        // We learned that a transport messenger has just been announced.
        // Nobody else took it, so far, so we'll take it. Incoming messengers
        // are not pooled by the endpoint service. We do pool them for our 
        // exclusive use.

        boolean taken = destinations.addIncomingMessenger(logDest, messenger);

        // Note to maintainers: Do not remove any route or negative cache info
        // here. Here is why: The messenger we just obtained was made out of an 
        // incoming connection. It brings no proof whatsoever that the peer is 
        // reachable at our initiative. In general, there is nothing to gain in
        // removing our knowlege of a long route, or a pending route query, or a
        // triedAndFailed record, other than to force trying a newly obtained 
        // set of addresses. They will not stop us from using this messenger as 
        // long as it works. The only good thing we can do here, is waking up 
        // those that may be waiting for a connection.

        synchronized (this) {
            notifyAll();
        }
        return taken;
    }

    /**
     * Call when an asynchronous new messenger could not be obtained.
     *
     * @param logDest the failed logical destination
     */
    void noMessenger(EndpointAddress logDest) {

        // Switch to short timeout if there was an infinite one.
        // Note if there's one, it is either short or inifinite. So we
        // look at the value only in the hope it is less expensive
        // than doing a redundant put.

        PeerID peerID = addr2pid(logDest);

        synchronized (this) {
            Long curr = triedAndFailed.get(peerID);

            if (curr != null && curr > TimeUtils.toAbsoluteTimeMillis(MAX_ASYNC_GETMESSENGER_RETRY)) {
                triedAndFailed.put(peerID, TimeUtils.toAbsoluteTimeMillis(MAX_ASYNC_GETMESSENGER_RETRY));
            }
        }
    }

    /**
     * Call when an asynchronous new messenger is ready.
     * (name is not great).
     *
     * @param event the new messenger event.
     * @return always returns true
     */
    boolean newMessenger(MessengerEvent event) {

        Messenger messenger = event.getMessenger();
        EndpointAddress logDest = messenger.getLogicalDestinationAddress();

        // We learned that a new transport messenger has just been announced.
        // We pool it for our exclusive use.
        destinations.addOutgoingMessenger(logDest, messenger);

        // Here's a new connection. Wakeup those that may be waiting for that.
        synchronized (this) {
            notifyAll();
        }
        return true;
    }

    /**
     * Get the routed route, if any, for a given peer id.
     *
     * @param peerAddress the peer who's route is desired.
     * @param seekRoute   boolean to indicate  if we should search for a route
     *                    if we don't have one
     * @return a route advertisement describing the direct route to the peer.
     */
    RouteAdvertisement getRoute(EndpointAddress peerAddress, boolean seekRoute) {

        ID peerID = addr2pid(peerAddress);

        // check if we have a valid route
        RouteAdvertisement route;

        synchronized (this) {
            route = routedRoutes.get(peerID);
        }
        if (route != null || !seekRoute) { // done
            return route;
        }

        // No known route and we're allowed to search for one

        // check if there is route advertisement available
        Iterator<RouteAdvertisement> allRadvs = routeCM.getRouteAdv(peerID);

        while (allRadvs.hasNext()) {
            route = allRadvs.next();
            Vector<AccessPointAdvertisement> hops = route.getVectorHops();

            // no hops, uninterresting: this needs to be a route
            if (hops.isEmpty()) {
                continue;
            }

            // let's check if we can speak to any of the hops in the route
            // we try them in reverse order so we shortcut the route
            // in the process
            RouteAdvertisement newRoute = (RouteAdvertisement)
                    AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());

            newRoute.setDest(route.getDest().clone());
            Vector<AccessPointAdvertisement> newHops = new Vector<AccessPointAdvertisement>();

            // build the route from the available hops
            for (int i = hops.size() - 1; i >= 0; i--) {
                ID hopID = hops.elementAt(i).getPeerID();

                // If the local peer is one of the first reachable
                // hop in the route, that route is worthless to us.
                if (localPeerId.equals(hopID)) {
                    break;
                }

                EndpointAddress addr = pid2addr(hopID);

                if (ensureLocalRoute(addr, null) != null) {
                    // we found a valid hop return the corresponding
                    // route from that point
                    for (int j = i; j <= hops.size() - 1; j++) {
                        newHops.add(hops.elementAt(j).clone());
                    }
                    // make sure we have a real route at the end
                    if (newHops.isEmpty()) {
                        break;
                    }

                    newRoute.setHops(newHops);
                    // try to set the route
                    if (setRoute(newRoute, false)) {
                        // We got one; we're done.
                        return newRoute;
                    } else {
                        // For some reason the route table does not
                        // want that route. Move on to the next adv; it
                        // unlikely that a longer version of the same would
                        // be found good.
                        break;
                    }
                }
            }
        }

        // no route found
        return null;
    }

    /**
     * Check if a route is valid.
     * Currently, only loops are detected.
     *
     * @param routeAdvertisement The route to check.
     * @return {@code true} if the route is valid otherwise {@code false}.
     */
    private boolean checkRoute(RouteAdvertisement routeAdvertisement) {

        if (0 == routeAdvertisement.size()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("route is empty");
            }
            return false;
        }

        if (routeAdvertisement.containsHop(localPeerId)) {
            // The route does contain this local peer. Using this route
            // would create a loop. Discard.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("route contains this peer - loopback");
            }
            return false;
        }

        PeerID destPid = routeAdvertisement.getDest().getPeerID();

        if (routeAdvertisement.containsHop(destPid)) {
            // May be it is fixable, may be not. See to it.
            Vector<AccessPointAdvertisement> hops = routeAdvertisement.getVectorHops();

            // It better be the last hop. Else this is a broken route.
            hops.remove(hops.lastElement());
            if (routeAdvertisement.containsHop(destPid)) {
                // There was one other than last: broken route.
                return false;
            }
        }

        if (routeAdvertisement.hasALoop()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("route has a loop ");
            }
            return false;
        } else {
            // Seems to be a potential good route.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("route is ok");
            }
            return true;
        }
    }

    // Adds a new long route provided there not a direct one already.
    // Replaces any longer route.  return true if the route was truely new.
    // The whole deal must be synch. We do not want to add a long route
    // while a direct one is being added in parallell or other stupid things like that.

    /**
     * set new route info
     *
     * @param route new route to learn
     * @param force true if the route was obtained by receiving
     *              a message
     * @return true if route was truly new
     */
    boolean setRoute(RouteAdvertisement route, boolean force) {
        PeerID peerID;
        EndpointAddress peerAddress;
        boolean pushNeeded = false;
        boolean status;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("setRoute:");
        }

        if (route == null) {
            return false;
        }

        synchronized (this) {
            try {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine(route.display());
                }
                peerID = route.getDest().getPeerID();
                peerAddress = pid2addr(peerID);

                // Check if we are in the case where we are
                // setting a new route as we received a message
                // always force the new route setup when we received a
                // a message
                if (!force) {
                    // check if we have some bad NACK route info for
                    // this destination
                    BadRoute badRoute = badRoutes.get(peerAddress);

                    if (badRoute != null) {
                        Long nextTry = badRoute.getExpiration();

                        if (nextTry > System.currentTimeMillis()) {
                            // check if the route we have in the NACK cache match the
                            // new one. Need to make sure that we clean the route
                            // from any endpoint addresses as the badRoute cache only
                            // contains PeerIDs
                            RouteAdvertisement routeClean = route.cloneOnlyPIDs();

                            if (routeClean.equals(badRoute.getRoute())) {
                                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                    LOG.fine("try to use a known bad route");
                                }
                                return false;
                            }
                        } else {
                            // expired info, just flush NACK route cache
                            badRoutes.remove(peerAddress);
                        }
                    }
                } else {
                    // we get a new route
                    badRoutes.remove(peerAddress);
                }

                // Check if the route makes senses (loop detection)
                if (!checkRoute(route)) {
                    // Route is invalid. Drop it.
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Route is invalid");
                    }
                    return false;
                }

                // check if we can reach the first hop in the route
                // We only do a shallow test of the first hop. Whether more effort
                // is worth doing or not is decided (and done) by the invoker.
                if (!isLocalRoute(pid2addr(route.getFirstHop().getPeerID()))) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Unreachable route - ignore");
                    }
                    return false;
                }

            } catch (Exception ez1) {
                // The vector must be empty, which is not supposed to happen.
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Got an empty route - discard" + route.display());
                }
                return false;
            }

            // add the new route
            try {
                // push the route to SRDI only if it is a new route. the intent is
                // to minimize SRDI traffic. The SRDIinformation is more of the order
                // this peer has a route to this destination, it does not need to be
                // updated verey time the route is updated. Information about knowing
                // that this peer has a route is more important that the precise
                // route information

                // SRDI is run only if the peer is acting as a rendezvous
                if (group.isRendezvous()) {
                    if (!routedRoutes.containsKey(peerID)) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("push new SRDI route " + peerID);
                        }
                        pushNeeded = true;
                    }
                }

                // new route so publish the known route in our cache
                if (!routedRoutes.containsKey(peerID)) {
                    routeCM.createRoute(route);
                    newDestinations.add(peerAddress);
                }

                // Remove any endpoint addresses from the route
                // as part of the cloning. We just keep track
                // of PIDs in our route table
                RouteAdvertisement newRoute = route.cloneOnlyPIDs();

                routedRoutes.put(peerID, newRoute);

                // We can get rid of any negative info we had. We have
                // a new and different route.
                badRoutes.remove(peerAddress);
                notifyAll(); // Wakeup those waiting for a route.
                status = true;
            } catch (Exception e2) {
                // We failed, leave things as they are.
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("   failed setting route with " + e2);
                }
                status = false;
            }
        }
        // due to the potential high latency of making the
        // srdi revolver push we don't want to hold the lock
        // on the EndpointRouter object as we may have to
        // discover a new route to a rendezvous
        if (pushNeeded && status) {
            // we are pushing the SRDI entry to a replica peer
            routeResolver.pushSrdi(null, peerID);
        }
        return status;
    }

    /**
     * This method is used to remove a route
     *
     * @param peerID route to peerid to be removed
     */
    void removeRoute(PeerID peerID) {
        boolean needRemove;

        synchronized (this) {
            needRemove = false;
            if (routedRoutes.containsKey(peerID)) {
                if (group.isRendezvous()) {
                    // Remove the SRDI cache entry from the SRDI cache
                    needRemove = true;
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("remove SRDI route " + peerID);
                    }
                }
                routedRoutes.remove(peerID);
            }
        }

        // due to the potential high latency of pushing
        // the SRDI message we don't want to hold the EndpointRouter
        // object lock
        if (needRemove) {
            // We are trying to flush it from the replica peer
            // Note: this is not guarantee to work if the peerview
            // is out of sync. The SRDI index will be locally
            // repaired as the peerview converge
            routeResolver.removeSrdi(null, peerID);
        }
    }

    /**
     * {@inheritDoc}
     */
    public void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {
        EndpointAddress srcPeerAddress;
        EndpointAddress destPeer;
        EndpointAddress lastHop = null;
        boolean connectLastHop = false;
        EndpointAddress origSrcAddr;
        EndpointAddress origDstAddr;
        Vector origHops = null; // original route of the message
        EndpointRouterMessage routerMsg;
        EndpointAddress nextHop = null;
        RouteAdvertisement radv;

        // We do not want the existing header to be ignored of course.
        routerMsg = new EndpointRouterMessage(msg, false);
        if (!routerMsg.msgExists()) {
            // The sender did not use this router
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Discarding " + msg + ". No routing info.");
            }
            return;
        }

        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(routerMsg.display());
            }
            origSrcAddr = routerMsg.getSrcAddress();
            origDstAddr = routerMsg.getDestAddress();

            // convert the src and dest addresses into canonical
            // form stripping service info
            srcPeerAddress = new EndpointAddress(origSrcAddr, null, null);
            destPeer = new EndpointAddress(origDstAddr, null, null);

            lastHop = routerMsg.getLastHop();

            // See if there's an originator full route adv inthere.
            // That's a good thing to keep.
            radv = routerMsg.getRouteAdv();
            if (radv != null) {

                // publish the full route adv. Also, leave it the
                // message.  It turns out to be extremely usefull to
                // peers downstream, specially the destination. If
                // this here peer wants to embed his own radv, it will
                // have to wait; the one in the message may not come
                // again.

                // FIXME - jice@jxta.org 20040413 : all this could wait (couldn't it ?)
                // until we know it's needed, therefore parsing could wait as well.

                // Looks like a safe bet to try and ensure a
                // connection in the opposite direction if there
                // isn't one already.

                if (pid2addr(radv.getDestPeerID()).equals(lastHop)) {
                    connectLastHop = true;
                }

                // Make the most of that new adv.
                setRoute(radv, true);
                updateRouteAdv(radv);
            }
        } catch (Exception badHdr) {
            // Drop it, we do not even know the destination
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Bad routing header or bad message. Dropping " + msg);
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Exception: ", badHdr);
            }
            return;
        }

        // Is this a loopback ?
        if ((srcPeerAddress != null) && srcPeerAddress.equals(localPeerAddr)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("dropped loopback");
            }
            return;
        }

        // Are we already sending to ourself. This may occur
        // if some old advertisements for our EA is still
        // floating around
        if ((lastHop != null) && lastHop.equals(localPeerAddr)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("dropped loopback from impersonating Peer");
            }
            return;
        }

        // We have to try and reciprocate the connection, so that we
        // have chance to learn reverse routes early enough.  If we do
        // not already have a messenger, then we must know a route adv
        // for that peer in order to be able to connect. Otherwise,
        // the attempt will fail and we'll be left with a negative
        // entry without having realy tried anything.  To prevent that
        // we rely on the presence of a radv in the router message. If
        // there's no radv, two possibilities:
        //
        // - It is not the first contact from that peer and we already
        // have tried (with or without success) to reciprocate.
        //
        // - It is the first contact from that peer but it has not
        // embedded its radv. In the most likely case (an edge peer
        // connecting to a rdv), the edge peer will have no difficulty
        // finding the reverse route, provided that we do not make a
        // failed attempt right now.
        //
        // Conclusion: if there's no embedded radv in the message, do
        // nothing.

        if (connectLastHop) {
            ensureLocalRoute(lastHop, radv);
        }

        try {

            // Normalize the reverseHops vector from the message and, if it
            // looks well formed and usefull, learn from it.  Do we have a
            // direct route to the origin ?  If yes, we'll zap the revers
            // route in the message: we're a much better router.  else,
            // learn from it. As a principle we regard given routes to be
            // better than existing ones.
            Vector<AccessPointAdvertisement> reverseHops = routerMsg.getReverseHops();

            if (reverseHops == null) {
                reverseHops = new Vector<AccessPointAdvertisement>();
            }

            // check if we do not have a direct route
            // in that case we don't care to learn thelong route
            if (!isLocalRoute(srcPeerAddress)) {
                // Check if the reverseRoute info looks correct.
                if (lastHop != null) {
                    // since we are putting the lasthop in the
                    // reverse route to indicate the validity of
                    // lastop to reach the previous hop, we have
                    // the complete route, just clone it. (newRoute
                    // owns the given vector)

                    if ((reverseHops.size() > 0) && reverseHops.firstElement().getPeerID().equals(addr2pid(lastHop))) {

                        // Looks like a good route to learn
                        setRoute(RouteAdvertisement.newRoute(addr2pid(srcPeerAddress), null, (Vector<AccessPointAdvertisement>) reverseHops.clone()), true);

                    }

                }
            }

            // If this peer is the final destination, then we're done. Just let
            // it be pushed up the stack. We must not leave our header;
            // it is now meaningless and none of the upper layers business.
            // All we have to do is to supply the end-2-end src and dest
            // so that the endpoint demux routine can do its job.
            if (destPeer.equals(localPeerAddr)) {
                // Removing the header.
                routerMsg.clearAll();
                routerMsg.updateMessage();
                // receive locally
                endpoint.processIncomingMessage(msg, origSrcAddr, origDstAddr);
                return;
            }

            // WATCHOUT: if this peer is part of the reverse route
            // it means that we've seen that message already: there's
            // a loop between routers ! If that happens drop that
            // message as if it was burning our fingers

            // First build the ap that we might add to the reverse route.
            // We need it to look for ourselves in reverseHops. (contains
            // uses equals(). equals will work because we always include
            // in reversehops aps that have only a peerAddress.

            AccessPointAdvertisement selfAp = (AccessPointAdvertisement)
                    AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());

            selfAp.setPeerID(localPeerId);

            if (reverseHops.contains(selfAp)) {

                // Danger, bail out !
                // Better not to try to NACK for now, but get rid of our own
                // route. If we're sollicited again, there won't be a loop
                // and we may NACK.
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Routing loop detected. Message dropped");
                }
                removeRoute(addr2pid(destPeer));
                return;
            }

            // Update reverseHops. That is, add ourselves to the list.

            // We will only add the current hop to the reverse
            // route if we know how to talk back to the previous hop
            // it is important to point the difference between the lastHop
            // and the reverse route entry. The lastHop indicates where the
            // message came from but does not specify whether it is able to
            // route messages in the other direction. The reverse route, if
            // present, provides that information.

            // FIXME - jice@jxta.org 20040413 : HERE comes the use of connectLastHop. Could have waited till here.

            if (isLocalRoute(lastHop)) { // ok we have direct route back, at least we hope :-)
                reverseHops.add(0, selfAp); // Update our vector
                routerMsg.prependReverseHop(selfAp); // Update the message, this preserves the cache.
            } else {

                // We cannot talk to our previous hop, well
                // check if we have route to the src and use it as
                // our reverse route. We could do more. But let's keep
                // it to the minimum at this point.
                RouteAdvertisement newReverseRoute = routedRoutes.get(addr2pid(srcPeerAddress));

                if (newReverseRoute != null) {
                    // we found a new route back from our cache so let's use it
                    reverseHops = (Vector<AccessPointAdvertisement>) newReverseRoute.getVectorHops().clone();
                    // ok add ourselve to the reverse route
                    reverseHops.add(0, selfAp);
                } else {
                    // no new route found, sorry. In the worst
                    // case it is better to not have reverse route
                    reverseHops = null;
                }

                // In both cases above, we replace the hops completely.
                // The cache is of no use and is lost.
                routerMsg.setReverseHops(reverseHops);
            }

            // Get the next peer into the forward route
            origHops = routerMsg.getForwardHops();
            if (origHops != null) {
                nextHop = getNextHop(origHops);
            }

            // see if we can shortcut to the destination with no effort.
            // If that works it's all very easy.
            if (isLocalRoute(destPeer)) {

                // There is a local route - use it
                // Clear the forward path which is probably wrong
                routerMsg.setForwardHops(null);
                nextHop = destPeer;
            } else {
                if (nextHop == null) {

                    // No next hop. Use the destPeer instead. (but, unlike when
                    // we shortcut it deliberately, don't know if we can have a direct route
                    // yet). This is perfectly normal if we're just the last
                    // hop before the destination and we have closed the direct connection
                    // with it since we declared to be a router to it.

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("No next hop in forward route - Using destination as next hop");
                    }
                    nextHop = destPeer;

                    // That forward path is exhausted. It will not be usefull anymore.
                    // either we reach the destination directly and there will be
                    // no need for a NACK further down, or we will need to find an alternate
                    // route.
                    routerMsg.setForwardHops(null);
                }

                // We must be do better than look passively for a direct
                // route. The negative cache will take care of reducing the
                // implied load. If we do not, then we never re-establish
                // a broken local route until the originating peer seeks a
                // new route. Then the result is roughly the same plus
                // the overhead of route seeking...worse, if we're in the
                // path from the originator to it's only rdv, then the
                // originator does not stand a chance until it re-seeds !

                if (ensureLocalRoute(nextHop, null) == null) {

                    // need to look for a long route.
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Forward route element broken - trying alternate route");
                    }

                    // While we're at it, we might as well get rid of our own
                    // route to the destination if it goes through the same hop
                    // by any chance.
                    // FIXME: idealy, each time we get a broken local route
                    // we'd want to get rid of all routes that start from there
                    // but that's one more map to maintain.

                    RouteAdvertisement route = getRoute(destPeer, false);

                    if (route == null) {
                        cantRoute("No new route to repair the route - drop message", null, origSrcAddr, destPeer, origHops);
                        return;
                    }

                    if (pid2addr(route.getFirstHop().getPeerID()).equals(nextHop)) {
                        // Our own route is just as rotten as the sender's. Get rid
                        // of it.
                        removeRoute(addr2pid(destPeer));
                        cantRoute("No better route to repair the route - drop message", null, origSrcAddr, destPeer, origHops);
                        return;
                    }

                    // optimization to see if we can reach
                    // directly the last hop of that route
                    EndpointAddress addr = pid2addr(route.getLastHop().getPeerID());

                    if (isLocalRoute(addr)) {
                        // FIXME - jice@jxta.org 20030723. Should update our route table to reflect the shortcut.

                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Found new remote route via : " + addr);
                        }

                        // set the forward path to null no next hop
                        // FIXME: Not true. the last hop is not the destination.
                        // There could be a need for us receiving a NACK and that won't be
                        // possible. We should leave the next hop in the fw path. Just like
                        // we do when forwarding along the existing route.

                        routerMsg.setForwardHops(null);
                    } else { // need to check the first hop
                        Vector<AccessPointAdvertisement> newHops = (Vector<AccessPointAdvertisement>) route.getVectorHops().clone();

                        // FIXME: remove(0) seems wrong
                        // There could be a need for us receiving a NACK and that won't be
                        // possible. We should leave the next hop in the fw path. Just like
                        // we do when forwarding along the existing route.
                        addr = pid2addr(newHops.remove(0).getPeerID());
                        if (!isLocalRoute(addr)) {
                            // Our own route is provably rotten
                            // as well. Get rid of it.
                            removeRoute(addr2pid(destPeer));
                            cantRoute("No usable route to repair the route - drop message", null, origSrcAddr, destPeer, origHops);
                            return;
                        }
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Found new remote route via : " + addr);
                        }

                        // NB: setForwardHops does not clone.
                        routerMsg.setForwardHops(newHops);
                    }
                    // If we're here. addr is our new nextHop.
                    nextHop = addr;
                }
            }

            // The first time we talk to a peer to which we have
            // initiated a connection, we must include our local
            // route adv in the routerMsg. However, we give priority to
            // a route adv that's already in the message and which we pass along.
            // In that case, our own will go next time. Note: we care only for
            // nextHop, not for the final destination. We give our radv to a far
            // destination only if we originate a message to it; not when forwarding.
            // JC: give priority to our own radv instead. It can be critical.

            // 20040301 tra: May be the case we haven't yet initialize our
            // own local route. For example still waiting for our relay connection
            RouteAdvertisement myRoute = getMyLocalRoute();

            if ((myRoute != null) && destinations.isWelcomeNeeded(nextHop)) {
                routerMsg.setRouteAdv(myRoute);
            }

            // We always modify the router message within the message
            routerMsg.setLastHop(localPeerAddr);
            routerMsg.updateMessage();

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Trying to forward to " + nextHop);
            }

            sendOnLocalRoute(nextHop, msg);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Successfully forwarded to " + nextHop);
            }
        } catch (Exception e) {
            cantRoute("Failed to deliver or forward message for " + destPeer, e, origSrcAddr, destPeer, origHops);
        }
    }

    private void cantRoute(String logMsg, Exception exception, EndpointAddress origSrcAddr, EndpointAddress destPeer, Vector origHops) {
        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
            if (exception == null) {
                LOG.warning(logMsg);
            } else {
                LOG.log(Level.WARNING, logMsg, exception);
            }
        }
        routeResolver.generateNACKRoute(addr2pid(origSrcAddr), addr2pid(destPeer), origHops);
    }

    /**
     * Return the address of the next hop in this vector
     *
     * @param hops of forward hops in the route
     * @return next hop to be used
     */
    private EndpointAddress getNextHop(Vector hops) {
        // check if we have a real route
        if ((hops == null) || (hops.size() == 0)) {
            return null;
        }

        // find the next hop.
        for (Enumeration e = hops.elements(); e.hasMoreElements();) {
            AccessPointAdvertisement ap = (AccessPointAdvertisement) e.nextElement();

            if (localPeerId.equals(ap.getPeerID())) {

                // If found at the end, no next hop
                if (!e.hasMoreElements()) {
                    return null;
                }
                return pid2addr(((AccessPointAdvertisement) e.nextElement()).getPeerID());
            }
        }

        // The peer is not into the vector. Since we have got that
        // message, the best we can do is to send it to the first gateway
        // in the forward path.
        return pid2addr(((AccessPointAdvertisement) hops.elementAt(0)).getPeerID());
    }

    /**
     * lame hard-coding
     *
     * @param p message transport
     * @return true if fast
     */
    private boolean isFast(MessageTransport p) {
        String name = p.getProtocolName();

        return name.equals("tcp") || name.equals("beep");
    }

    private boolean isRelay(MessageTransport p) {
        String name = p.getProtocolName();

        return name.equals("relay");
    }

    /**
     * Given a list of addresses, find the best reachable endpoint.
     * <p/>
     * <ul>
     * <li>The address returned must be reachable.</li>
     * <li>We prefer an address whose protocol is, in order:</li>
     * <ol>
     * <li>connected and fast.</li>
     * <li>connected and slow.</li>
     * <li>unconnected and fast.</li>
     * <li>unconnected and slow</li>
     * </ol></li>
     * </ul>
     *
     * @param dest      destination address.
     * @param mightWork A list of addresses to evaluate reachability.
     * @param exist     true if there already are existing messengers for
     *                  the given destinations but we want one more. It may lead us to reject
     *                  certain addresses that we would otherwise accept.
     * @return The endpoint address for which we found a local route otherwise
     *         null
     */
    Messenger findBestReachableEndpoint(EndpointAddress dest, List<EndpointAddress> mightWork, boolean exist) {

        List<Integer> rankings = new ArrayList<Integer>(mightWork.size());
        List<EndpointAddress> worthTrying = new ArrayList<EndpointAddress>(mightWork.size());

        // First rank the available addresses by type rejecting those which
        // cant be used.

        for (Object aMightWork : mightWork) {
            EndpointAddress addr = (EndpointAddress) aMightWork;

            // skip our own type
            if (getProtocolName().equals(addr.getProtocolName())) {
                continue;
            }
            int rank = -1;
            Iterator<MessageTransport> eachTransport = endpoint.getAllMessageTransports();

            while (eachTransport.hasNext()) {
                MessageTransport transpt = eachTransport.next();

                if (!transpt.getProtocolName().equals(addr.getProtocolName())) {
                    continue;
                }

                // must be a sender
                if (!(transpt instanceof MessageSender)) {
                    continue;
                }

                MessageSender sender = (MessageSender) transpt;

                // must allow routing
                if (!sender.allowsRouting()) {
                    // This protocol should not be used for routing.
                    continue;
                }
                rank += 1;
                if (sender.isConnectionOriented()) {
                    rank += 2;
                }

                if (isRelay(transpt)) {
                    // That should prevent the relay for ever being used
                    // when the relay may be sending through the router.
                    if (exist) {
                        rank -= 1000;
                    }
                }
                if (isFast(transpt)) {
                    rank += 4;
                }
            }

            // if its worth trying then insert it into the rankings.
            if (rank >= 0) {
                for (int eachCurrent = 0; eachCurrent <= rankings.size(); eachCurrent++) {
                    if (rankings.size() == eachCurrent) {
                        rankings.add(rank);
                        worthTrying.add(addr);
                        break;
                    }
                    if (rank > rankings.get(eachCurrent)) {
                        rankings.add(eachCurrent, rank);
                        worthTrying.add(eachCurrent, addr);
                        break;
                    }
                }
            }
        }

        // now that we have them ranked, go through them until we get a
        // successful messenger.
        rankings = null;

        for (EndpointAddress addr : worthTrying) {

            try {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Trying : " + addr);
                }
                // We use an async getMessenger as we do not
                // want to wait too long to obtain our messenger
                // We will still wait ASYNCMESSENGER_WAIT to see
                // if we can get the messenger before bailing out

                // Create the listener object for that request
                EndpointGetMessengerAsyncListener getMessengerListener = new EndpointGetMessengerAsyncListener(this, dest);
                boolean stat = endpoint.getMessenger(getMessengerListener, new EndpointAddress(addr, ROUTER_SERVICE_NAME, null)
                        ,
                        null);

                if (!stat) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Failed to create async messenger to : " + addr);
                    }
                    // we failed to get a messenger, we need to update the try and
                    // failed as it currently holds an infinite timeout to permit
                    // another thread to retry that destination. We only retry
                    // every MAX_ASYNC_GETMESSENGER_RETRY seconds
                    synchronized (this) {
                        triedAndFailed.put(addr2pid(dest), TimeUtils.toAbsoluteTimeMillis(MAX_ASYNC_GETMESSENGER_RETRY));
                    }
                    continue;
                }

                // wait to see if we can get the Async messenger
                // If there is a long route to that destination, do not
                // wait on the direct route.
                // It may happen that we are actually
                // trying to reach a different peer and this is just part of
                // shortcuting the route via the one of the hops. In that case
                // this test is not entirely accurate. We might still decide
                // to wait when we shouldn't (we're no worse than before, then)
                // But, in most cases, this is going to help.
                boolean quick = (getRoute(dest, false) != null);
                Messenger messenger = getMessengerListener.waitForMessenger(quick);

                if (messenger == null) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("did not get our async messenger. continue");
                    }
                } else {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("we got our async messenger, proceed");
                    }
                    // Success we got a messenger synchronously. Remove
                    // the negative cache entry.
                    synchronized (this) {
                        triedAndFailed.remove(addr2pid(dest));
                        notifyAll();
                    }
                    return messenger;
                }
            } catch (RuntimeException e) {
                // That address is somehow broken.
                // Cache that result for a while.
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "failed checking route", e);
                }
            }
        }
        return null;
    }

    /**
     * Read the route advertisement for a peer and find a suitable transport
     * endpoint for sending to that peer either directly or via one of
     * the advertised peer router
     *
     * @param destPeerAddress dest address
     * @param exist           use existing messengers, avoid creating a new one
     * @param hint            route hint
     * @return a reachable messenger
     */
    Messenger findReachableEndpoint(EndpointAddress destPeerAddress, boolean exist, RouteAdvertisement hint) {

        PeerID destPeerID = addr2pid(destPeerAddress);

        // findReachableEndpoint is really lazy because what it does is expensive.
        // When needed, the negative info that prevents its from working
        // too much is removed. (see calls to ensureLocalRoute).
        synchronized (this) {
            Long nextTry = triedAndFailed.get(destPeerID);

            if (nextTry != null) {
                if (nextTry > TimeUtils.timeNow()) {
                    return null;
                }
            }

            // We are the first thread trying this destination. Let's preclude
            // any other threads from attempting to do anything while we are
            // trying that destination. Other threads will have a chance if they
            // are still waiting when this thread is done. We will update
            // triedAndFailed when we get the async notification that we got or
            // we failed to get a messenger.

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Temporarly adding " + destPeerAddress.toString() + " to triedAndFailed, while attempting connection");
            }
            triedAndFailed.put(destPeerID, TimeUtils.toAbsoluteTimeMillis(Long.MAX_VALUE));
        }

        // Never tried or it was a long time ago.
        // Get (locally) the advertisements of this peer
        Iterator<RouteAdvertisement> advs;

        try {
            // try to use the hint that was given to us
            if (hint != null) {
                advs = Collections.singletonList(hint).iterator();
            } else {
                // Ok extract from the CM
                advs = routeCM.getRouteAdv(destPeerID);
            }

            // Check if we got any advertisements
            List<EndpointAddress> addrs = new ArrayList<EndpointAddress>();

            while (advs.hasNext()) {
                RouteAdvertisement adv = advs.next();

                String saddr = null;

                // add the destination endpoint
                for (Enumeration<String> e = adv.getDest().getEndpointAddresses(); e.hasMoreElements();) {
                    try {
                        saddr = e.nextElement();
                        addrs.add(new EndpointAddress(saddr));
                    } catch (Throwable ex) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(" bad address in route adv : " + saddr);
                        }
                    }
                }
            }

            // ok let's go and try all these addresses
            if (!addrs.isEmpty()) {
                Messenger bestMessenger = findBestReachableEndpoint(destPeerAddress, addrs, exist);

                if (bestMessenger != null) {
                    // Found a direct route. Return it.
                    // Tried+failed has been cleaned.
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("found direct route");
                    }
                    return bestMessenger;
                }
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("findReachableEndpoint : Failed due to empty address list");
                }
            }
        } catch (RuntimeException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure looking for an address ", e);
            }
        }

        // We're done trying. Since we did not find anything at all (or failed,
        // during the atempt) the triedFailed record is still set to infinite 
        // value. Reset it to finite.
        // There is a small chance that another thread did find
        // something in parallel, but that's very unlikely and
        // if it is rare enough then the damage is small.
        synchronized (this) {
            triedAndFailed.put(destPeerID, TimeUtils.toAbsoluteTimeMillis(MAX_ASYNC_GETMESSENGER_RETRY));
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("did not find a direct route to :" + destPeerAddress);
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    public Messenger getMessenger(EndpointAddress addr, Object hint) {
        RouteAdvertisement routeHint = null;
        EndpointAddress plainAddr = new EndpointAddress(addr, null, null);
        
        // If the dest is the local peer, just loop it back without going
        // through the router.
        if (plainAddr.equals(localPeerAddr)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("return LoopbackMessenger");
            }
            return new LoopbackMessenger(group, endpoint, localPeerAddr, addr, addr);
        }

        try {
            // try and add that hint to our cache of routes (that may be our only route).
            if (hint != null && hint instanceof RouteAdvertisement) {
                routeHint = ((RouteAdvertisement) hint).clone();
                AccessPointAdvertisement firstHop = routeHint.getFirstHop();
                PeerID firstHopPid;
                EndpointAddress firstHopAddr = null;

                // If the firstHop is equal to the destination, clean that up,
                // that's a direct route. If the first hop is the local peer
                // leave it there but treat it as a local route. That's what
                // it is from the local peer point of view.
                if (firstHop != null) {

                    firstHopPid = firstHop.getPeerID();
                    firstHopAddr = pid2addr(firstHopPid);

                    if (firstHopAddr.equals(addr)) {
                        routeHint.removeHop(firstHopPid);
                        firstHop = null;
                    } else if (firstHopPid.equals(localPeerId)) {
                        firstHop = null;
                    }

                }

                if (firstHop == null) {
                    // The hint is a direct route. Make sure that we have the
                    // route adv so that we can actually connect.

                    // we only need to publish this route if we don't know about
                    // it yet.
                    EndpointAddress da = pid2addr(routeHint.getDestPeerID());

                    if (!isLocalRoute(da) && !routedRoutes.containsKey(routeHint.getDestPeerID())) {
                        routeCM.publishRoute(routeHint);
                    }

                } else {
                    // For the hint to be useful, we must actively try the first
                    // hop. It is possible that we do not know it yet and that's
                    // not a reason to ignore the hint (would ruin the purpose
                    // in most cases).
                    RouteAdvertisement routeFirstHop = null;

                    // Manufacture a RA just that as just the routerPeer as a
                    // destination. We only need to publish this route if we
                    // don't know about it yet.
                    if (!isLocalRoute(firstHopAddr) && !routedRoutes.containsKey(firstHop.getPeerID())) {

                        routeFirstHop = (RouteAdvertisement)
                                AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());
                        routeFirstHop.setDest(firstHop.clone());

                        // Here we used to pass a second argument with value
                        // true which forced updateRouteAdv to ignore a
                        // pre-existing identical adv and remove negative cache
                        // information anyway. The reason for doing that was
                        // that sometimes the new route adv does already exist
                        // but has not yet been tried. We cannot do that; it
                        // exposes us too much to retrying incessantly the same
                        // address. A hint cannot be trusted to such an extent.
                        // The correct remedy is to be able to tell accurately
                        // if there really is an untried address in that radv,
                        // which requires a sizeable refactoring. in the
                        // meantime just let the negative cache play its role.
                        updateRouteAdv(routeFirstHop);
                    }

                    // if we constructed the route hint then passes it in the
                    // past we were just relying on the CM now that the CM can
                    // be disabled, we have to pass the argument.
                    if (ensureLocalRoute(firstHopAddr, routeFirstHop) != null) {
                        setRoute(routeHint.clone(), false);
                    }
                }
            }

        } catch (Throwable ioe) {
            // Enforce a stronger semantic to hint. If the application passes
            // a hint that is rotten then this is an application problem
            // we should not try to fix what was given to us.
            return null;
        }

        try {
            // Build a persistent RouterMessenger around it that will add our
            // header. If a hint was passed to us we just use it as it. Too bad
            // if it is not the the right one. In that mode it is the
            // responsibility of the application to make sure that a correct
            // hint was passed.
            return new RouterMessenger(addr, this, routeHint);
        } catch (IOException caught) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Can\'t generate messenger for addr " + addr, caught);
            }
            return null;
        }
    }

    /**
     * Updates the router element of a message and returns the peerAddress address of
     * the next hop (where to send the message).
     * <p/>
     * Currently, address message is only called for messages that we
     * originate. As a result we will always aggressively seek a route if needed.
     *
     * @param message    the message for which to compute/update a route.
     * @param dstAddress the final destination of the route which the message be set to follow.
     * @return EndpointAddress The address (logical) where to send the message next. Null if there
     *         is nowhere to send it to.
     */
    EndpointAddress addressMessage(Message message, EndpointAddress dstAddress) {
        if (endpoint == null) {
            return null;
        }

        // We need to create a RouterMessage
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Create a new EndpointRouterMessage " + dstAddress);
        }

        // Specify that we do not want an existing msg parsed.
        EndpointRouterMessage routerMsg = new EndpointRouterMessage(message, true);

        if (routerMsg.isDirty()) {
            // Oops there was one in the message already. This must be a
            // low-level protocol looping back through the router. The relay can
            // be led to do that in some corner cases.
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Probable transport recursion");
            }
            throw new IllegalStateException("RouterMessage element already present");
        }

        routerMsg.setSrcAddress(localPeerAddr);
        routerMsg.setDestAddress(dstAddress);

        EndpointAddress theGatewayAddress;
        EndpointAddress dstAddressPlain = new EndpointAddress(dstAddress, null, null);

        try {
            RouteAdvertisement route = null;

            theGatewayAddress = getGatewayAddress(dstAddressPlain, true, null);

            if (theGatewayAddress == null) {
                // Cleanup the message, so that the invoker
                // may retry (with a different hint, for example).
                routerMsg.clearAll();
                routerMsg.updateMessage();
                return null;
            }

            // Check that we're actually going through a route; we could have one
            // but not be using it, because we know of a volatile shortcut.

            // FIXME: jice@jxta.org - 20030512: This is not very clean:
            // getGatewayAddress should be giving us the route that it's using, if any.
            // By doing the fetch ourselves, not only do we waste CPU hashing
            // twice, but we could also get a different route !

            if (!theGatewayAddress.equals(dstAddressPlain)) {
                route = getRoute(dstAddressPlain, false);
            }

            // If we're going through a route for that, stuff it in the
            // message. NB: setForwardHops does not clone.
            if (route != null) {
                routerMsg.setForwardHops((Vector<AccessPointAdvertisement>) route.getVectorHops().clone());
            }

            // set the last hop info to point to the local peer info
            // The recipient takes last hop to be the last peer that the message has traversed
            // before arriving.
            routerMsg.setLastHop(localPeerAddr);

            // The first time we talk to a peer to which we have
            // initiated a connection, we must include our local
            // route adv in the routerMsg.
            RouteAdvertisement myRoute = getMyLocalRoute();

            if (myRoute != null) {
                // FIXME - jice@jxta.org 20040430 : use destinations instead of newDestinations, even for routed ones.
                boolean newDest = newDestinations.remove(dstAddressPlain);
                boolean newGatw = destinations.isWelcomeNeeded(theGatewayAddress);

                if (newDest || newGatw) {
                    routerMsg.setRouteAdv(myRoute);
                }
            }

            // Push the router header onto the message.
            // That's all we have to do for now.

            routerMsg.updateMessage();
        } catch (Exception ez1) {
            // Not much we can do
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not fully address message", ez1);
            }
            return null;
        }

        return theGatewayAddress;
    }

    /**
     * {@inheritDoc}
     */
    public Object transportControl(Object operation, Object value) {
        if (!(operation instanceof Integer)) {
            return null;
        }

        int op = (Integer) operation;

        switch (op) {
            case RouteControlOp: // Get a Router Control Object
                return new RouteControl(this, localPeerId);

            default:
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Invalid Transport Control operation argument");
                }

                return null;
        }
    }

    /**
     * Convert a Router EndpointAddress into a PeerID
     *
     * @param addr the address to extract peerAddress from
     * @return the PeerID
     */
    static PeerID addr2pid(EndpointAddress addr) {
        URI asURI = null;

        try {
            asURI = new URI(ID.URIEncodingName, ID.URNNamespace + ":" + addr.getProtocolAddress(), null);
            return (PeerID) IDFactory.fromURI(asURI);
        } catch (URISyntaxException ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Error converting a source address into a virtual address : " + addr, ex);
            }
        } catch (ClassCastException cce) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Error converting a source address into a virtual address: " + addr, cce);
            }
        }

        return null;
    }

    /**
     * Convert an ID into a Router Endpoint Address
     *
     * @param pid The ID who's equivalent Endpoint Address is desired.
     * @return The ID as an EndpointAddress.
     */
    static EndpointAddress pid2addr(ID pid) {
        return new EndpointAddress(ROUTER_PROTOCOL_NAME, pid.getUniqueValue().toString(), null, null);
    }

    /**
     * check if it is a new route adv
     *
     * @param route route advertisement
     */
    void updateRouteAdv(RouteAdvertisement route) {
        updateRouteAdv(route, false);
    }

    /**
     * check if it is a new route adv
     *
     * @param route route advertisement
     * @param force enforce the route
     */
    void updateRouteAdv(RouteAdvertisement route, boolean force) {
        try {
            PeerID pID = route.getDestPeerID();

            // check if we updated the route
            if (routeCM.updateRoute(route)) {
                // We just dumped an adv for that dest, so we want to do a real check
                // on its new addresses. Remove the entry from the negative cache.
                synchronized (this) {
                    Long nextTry = triedAndFailed.get(pID);

                    if (nextTry != null) {
                        // only remove if we do not have a pending request (infinite retry)
                        // we take the conservative approach to avoid creating multiple
                        // async thread blocked on the same destination
                        if (nextTry <= TimeUtils.toAbsoluteTimeMillis(MAX_ASYNC_GETMESSENGER_RETRY)) {
                            triedAndFailed.remove(pID);
                            notifyAll();
                        }
                    }
                }
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Route for " + pID + " is same as existing route, not publishing it");
                }

                if (force) {
                    synchronized (this) {
                        Long nextTry = triedAndFailed.get(pID);

                        if (nextTry != null) {
                            // only remove if we do not have a pending request (infinite retry)
                            // we take the conservative approach to avoid creating multiple
                            // async thread blocked on the same destination
                            if (nextTry <= TimeUtils.toAbsoluteTimeMillis(MAX_ASYNC_GETMESSENGER_RETRY)) {
                                triedAndFailed.remove(pID);
                                notifyAll();
                            }
                        }
                    }
                }
            }
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to publish route advertisement", e);
            }
        }
    }

    /**
     * is there a pending route query for that destination
     *
     * @param peerID destination address
     * @return true or false
     */
    boolean isPendingRouteQuery(PeerID peerID) {
        return pendingQueries.containsKey(peerID);
    }

    /**
     * get a pending route query info
     *
     * @param peerID destination address
     * @return pending route query info
     */
    ClearPendingQuery getPendingRouteQuery(PeerID peerID) {
        return pendingQueries.get(peerID);
    }

    /**
     * Do we have a long route for that destination
     *
     * @param peerID destination address
     * @return true or false
     */
    boolean isRoutedRoute(PeerID peerID) {
        return peerID != null && routedRoutes.containsKey(peerID);
    }

    /**
     * Snoop if we have a messenger
     *
     * @param addr destination address
     * @return Messenger
     */
    Messenger getCachedMessenger(EndpointAddress addr) {
        return destinations.getCurrentMessenger(addr);
    }

    /**
     * Get all direct route destinations
     *
     * @return Iterator iterations of all endpoint destinations
     */
    Iterator<EndpointAddress> getAllCachedMessengerDestinations() {
        return destinations.allDestinations().iterator();
    }

    /**
     * Get all long route destinations
     *
     * @return Iterator iterations of all routed route destinations
     */
    Iterator<Map.Entry<ID, RouteAdvertisement>> getRoutedRouteAllDestinations() {
        return routedRoutes.entrySet().iterator();
    }

    /**
     * Get all long route destination addresses
     *
     * @return Iterator iterations of all routed route addresses
     */
    Iterator<ID> getAllRoutedRouteAddresses() {
        return routedRoutes.keySet().iterator();
    }

    /**
     * Get all pendingRouteQuery destinations
     *
     * @return All pending route query destinations
     */
    Collection<Map.Entry<PeerID, ClearPendingQuery>> getPendingQueriesAllDestinations() {
        List<Map.Entry<PeerID, ClearPendingQuery>> copy = new ArrayList<Map.Entry<PeerID, ClearPendingQuery>>(
                pendingQueries.size());

        synchronized (pendingQueries) {
            copy.addAll(pendingQueries.entrySet());
        }

        return copy;
    }

    /**
     * Get the route CM cache Manager
     *
     * @return the route CM cache Manager
     */
    RouteCM getRouteCM() {
        return routeCM;
    }

    /**
     * Get the route resolver manager
     *
     * @return the route resolver Manager
     */
    RouteResolver getRouteResolver() {
        return routeResolver;
    }

    /**
     * set bad route entry
     *
     * @param addr     of the bad route
     * @param badRoute bad route info
     */
    synchronized void setBadRoute(EndpointAddress addr, BadRoute badRoute) {
        badRoutes.put(addr, badRoute);
    }

    /**
     * get bad route entry
     *
     * @param addr of the bad route
     * @return BadRoute bad route info
     */
    synchronized BadRoute getBadRoute(EndpointAddress addr) {
        return badRoutes.get(addr);
    }
}
