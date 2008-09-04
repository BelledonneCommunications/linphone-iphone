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

/**
 * This class is used to manage a persistent CM cache  of route
 * for the router
 */
package net.jxta.impl.endpoint.router;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.XMLElement;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.util.LRUCache;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.AccessPointAdvertisement;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import net.jxta.impl.endpoint.EndpointUtils;

class RouteCM implements Module {
    
    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(RouteCM.class.getName());
    
    /**
     * Default expiration time for Route advertisements. This is the amount
     * of time which advertisements will live in caches. After this time, the
     * advertisement should be refreshed from the source.
     */
    public final static long DEFAULT_EXPIRATION = 20L * TimeUtils.AMINUTE;
    
    /**
     * If {@code true} then the CM is used to persistently store route
     * advertisements. If {@code false} then only the in-memory route table is
     * used.
     */
    public final static boolean USE_CM_DEFAULT = true;
    
    /**
     * If {@code true} then the CM is used to persistently store route
     * advertisements. If {@code false} then only the in-memory route table is
     * used.
     * <p/>
     * We start out {@code false} until the module is started.
     */
    private boolean useCM = false;
    
    /**
     * If {@code true} then the CM is used to persistently store route
     * advertisements. If {@code false} then only the in-memory route table is
     * used.
     */
    private boolean useCMConfig = USE_CM_DEFAULT;
    
    /**
     * PeerGroup Service Handle
     */
    private PeerGroup group = null;
    private LRUCache<ID, RouteAdvertisement> lruCache = new LRUCache<ID, RouteAdvertisement>(100);
    
    /**
     * EndpointRouter pointer
     */
        
    /**
     * {@inheritDoc}
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
            
            param = paramBlock.getChildren("useCM");
            if (param.hasMoreElements()) {
                useCMConfig = Boolean.getBoolean(((XMLElement) param.nextElement()).getTextValue());
            }
        }
        
        this.group = group;
        
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
            configInfo.append("\n\t\tGroup : ").append(group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID : ").append(group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID : ").append(group.getPeerID());
            
            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tUse Route CM : ").append(useCMConfig);
            
            LOG.config(configInfo.toString());
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public int startApp(String[] arg) {
        // ok, we are initialized, go ahead and enable CM usage desired
        useCM = useCMConfig;
        return Module.START_OK;
    }
    
    /**
     * {@inheritDoc}
     */
    public void stopApp() {
        useCM = false;
    }
    
    /**
     * return routeCM usage
     *
     * @return true if enabled
     */
    boolean useRouteCM() {
        return useCM;
    }
    
    /**
     * toggles whether to use the RouteCM
     * @param enable if <code>true</code> it enables use of persistent store
     */
    void enableRouteCM(boolean enable) {
        useCM = enable;
    }
    
    /**
     * Get route advertisements from the local discovery cache.
     * We collect straight RouteAdvertisements as well as what can be
     * found in PeerAdvertisements.
     *
     * <p/>We can find both, and there's no way to know which is most relevant,
     * so we have to return all and let the invoker try its luck with each.
     *
     * @param peerID the target peer's ID.
     * @return Route Advertisements for the specified peer.
     */
    protected Iterator<RouteAdvertisement> getRouteAdv(ID peerID) {
        DiscoveryService discovery;
        
        // check if we use the CM, if not then nothing
        // to retrieve
        if (!useCM) {
            return Collections.<RouteAdvertisement>emptyList().iterator();
        } else {
            discovery = group.getDiscoveryService();
            if (null == discovery) {
                return Collections.<RouteAdvertisement>emptyList().iterator();
            }
        }
        
        String peerIDStr = peerID.toString();
        List<RouteAdvertisement> result = new ArrayList<RouteAdvertisement>(2);
        if (lruCache.contains(peerID)) {
            result.add(lruCache.get(peerID));
            return result.iterator();
        }
        // check first if we have a route advertisement
        Enumeration<Advertisement> advs = null;
        
        try {
            advs = discovery.getLocalAdvertisements(DiscoveryService.ADV, RouteAdvertisement.DEST_PID_TAG, peerIDStr);
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed discovering routes for " + peerIDStr, failed);
            }
        }
        
        while ((null != advs) && advs.hasMoreElements()) {
            Advertisement adv = advs.nextElement();
            
            if (!(adv instanceof RouteAdvertisement)) {
                continue;
            }
            
            RouteAdvertisement route = (RouteAdvertisement) adv;
            
            if (!result.contains(route)) {
                result.add(route);
            }
        }
        
        // get the local peer advertisements for the peer.
        advs = null;
        try {
            advs = discovery.getLocalAdvertisements(DiscoveryService.PEER, "PID", peerIDStr);
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed discovering peer advertisements for " + peerIDStr, failed);
            }
        }
        
        while ((null != advs) && advs.hasMoreElements()) {
            Advertisement adv = advs.nextElement();
            
            if (!(adv instanceof PeerAdvertisement)) {
                continue;
            }
            
            PeerAdvertisement padv = (PeerAdvertisement) adv;
            
            RouteAdvertisement route = EndpointUtils.extractRouteAdv(padv);
            
            // Publish the route if it was previously unknown.
            if (!result.contains(route)) {
                // We found a new route just publish it locally
                try {
                    // XXX 20060106 bondolo These publication values won't be obeyed if
                    // the route had been previously published.

                    //FIXME by hamada: This operation may lead to overwriting an existing and valid rout adv, no?
                    discovery.publish(route, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
                } catch (IOException failed) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Failed publishing route", failed);
                    }
                }
                result.add(route);
            }
        }
        return result.iterator();
    }
    
    /**
     * Create a new persistent route to the cache only if we can find set of
     * endpoint addresses
     *
     * @param route to be published
     */
    protected void createRoute(RouteAdvertisement route) {
        DiscoveryService discovery;
                
        // check if CM is used
        if (!useCM) {
            return;
        } else {
            discovery = group.getDiscoveryService();
            
            if (null == discovery) {
                return;
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("try to publish route ");
        }
        // we need to retrieve the current adv to get all the known
        // endpoint addresses
        try {
            RouteAdvertisement newRoute = (RouteAdvertisement)
                    AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());
            
            PeerID pId = route.getDestPeerID();
            
            String realPeerID = pId.toString();
            
            // check first if we have a route advertisement
            Enumeration<Advertisement> advs = discovery.getLocalAdvertisements(DiscoveryService.ADV, RouteAdvertisement.DEST_PID_TAG, realPeerID);
            
            if (!advs.hasMoreElements()) {
                // No route, sorry
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("could not find a route advertisement " + realPeerID);
                }
                return;
            }
            
            // make sure we are returning the longest route we know either
            // from the peer or route advertisement
            Advertisement adv = advs.nextElement();

            if (adv instanceof RouteAdvertisement) {
                RouteAdvertisement dest = (RouteAdvertisement) adv;

                newRoute.setDest(dest.getDest());
            }
            
            // let's get the endpoint addresses for each hops
            Vector<AccessPointAdvertisement> newHops = new Vector<AccessPointAdvertisement>();
            
            Enumeration<AccessPointAdvertisement> e = route.getHops();
            
            while (e.hasMoreElements()) {
                AccessPointAdvertisement ap = e.nextElement();
                
                realPeerID = ap.getPeerID().toString();
                
                // check first if we have a route advertisement
                advs = discovery.getLocalAdvertisements(DiscoveryService.ADV, RouteAdvertisement.DEST_PID_TAG, realPeerID);
                if (!advs.hasMoreElements()) {
                    // No route, sorry
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("could not find a route advertisement for hop " + realPeerID);
                    }
                    return;
                }
                adv = advs.nextElement();
                // ensure it is a RouteAdvertisement
                if (adv instanceof RouteAdvertisement) {
                    newHops.add(((RouteAdvertisement) adv).getDest());
                }
            }
            
            // last check to see that we have a route
            if (newHops.isEmpty()) {
                return;
            }
            
            newRoute.setHops(newHops);
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("publishing new route \n" + newRoute.display());
            }
            lruCache.put(route.getDestPeerID(), route);
            // XXX 20060106 bondolo These publication values won't be obeyed if
            // the route had been previously published.
            discovery.publish(newRoute, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "error publishing route" + route.display(), ex);
            }
        }
    }
    
    /**
     * Publish a route advertisement to the CM
     *
     * @param route advertisement to be published
     */
    protected void publishRoute(RouteAdvertisement route) {
        DiscoveryService discovery;
                
        // check if CM is in used, if not nothing to do
        if (!useCM) {
            return;
        } else {
            discovery = group.getDiscoveryService();
            if (null == discovery) {
                return;
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Publishing route for " + route.getDestPeerID());
        }
        
        // publish route adv
        if (!lruCache.contains(route.getDestPeerID())) {
            try {
                // XXX 20060106 bondolo These publication values won't be obeyed if
                // the route had been previously published.
                discovery.publish(route, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
            } catch (Exception ex) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "error publishing route adv \n" + route, ex);
                }
            }
        }
        lruCache.put(route.getDestPeerID(), route);
    }

    /**
     * flush route adv from CM
     *
     * @param peerID the PeerID
     */
    protected void flushRoute(ID peerID) {
        DiscoveryService discovery;
                
        // check if CM is in used, if not nothing to do
        if (!useCM) {
            return;
        } else {
            discovery = group.getDiscoveryService();
            if (null == discovery) {
                return;
            }
        }
        
        // leqt's remove any advertisements (route, peer) related to this peer
        // this should force a route query to try to find a new route
        // check first if we have a route advertisement
        String peerIDStr = peerID.toString();
        
        Enumeration<Advertisement> advs = null;
        
        // Flush the local route advertisements for the peer.
        try {
            advs = discovery.getLocalAdvertisements(DiscoveryService.ADV, RouteAdvertisement.DEST_PID_TAG, peerIDStr);
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure recovering route advertisements.", failed);
            }
        }
        
        while ((null != advs) && advs.hasMoreElements()) {
            Advertisement adv = advs.nextElement();
            
            if (!(adv instanceof RouteAdvertisement)) {
                continue;
            }
            
            // ok so let's delete the advertisement
            try {
                discovery.flushAdvertisement(adv);
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("removed RouteAdvertisement for " + peerIDStr);
                }
            } catch (IOException ex) {// protect against flush IOException when the entry is not there
            }
        }
        
        // Flush the local peer advertisements for the peer.
        advs = null;
        try {
            advs = discovery.getLocalAdvertisements(DiscoveryService.PEER, "PID", peerIDStr);
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed discovering peer advertisements for " + peerIDStr, failed);
            }
        }
        
        while ((null != advs) && advs.hasMoreElements()) {
            Advertisement adv = advs.nextElement();
            
            if (!(adv instanceof PeerAdvertisement)) {
                continue;
            }
            
            // ok so let's delete the advertisement
            try {
                discovery.flushAdvertisement(adv);
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("removed PeerAdvertisement for " + peerIDStr);
                }
            } catch (IOException ex) {// protect against flush IOException when the entry is not there
            }
        }
        // remove it from the cache as well
        lruCache.remove(peerID);
    }
    
    /**
     * publish or update new route from the advertisement cache
     *
     * @param route to be published or updated
     * @return boolean  true or false if adv cache was updated
     */
    protected boolean updateRoute(RouteAdvertisement route) {
        DiscoveryService discovery;
        
        // check if CM is in used
        if (!useCM) {
            return false;
        } else {
            discovery = group.getDiscoveryService();
            if (null == discovery) {
                return true;
            }
        }
        
        try {
            String realPeerID = route.getDestPeerID().toString();
            
            // check first if we have a route advertisement
            Enumeration<Advertisement> advs = discovery.getLocalAdvertisements(DiscoveryService.ADV, RouteAdvertisement.DEST_PID_TAG,  realPeerID);
            
            if (advs.hasMoreElements()) {
                Advertisement adv = advs.nextElement();

                if (adv instanceof RouteAdvertisement) {
                    RouteAdvertisement oldRouteAdv = (RouteAdvertisement) adv;

                    // check if the old route is equal to the new route
                    if (!route.equals(oldRouteAdv)) {
                        // publish the new route
                        // XXX 20060106 bondolo These publication values won't be obeyed if
                        // the route had been previously published.
                        discovery.publish(route, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
                        lruCache.put(route.getDestPeerID(), route);
                        return true;
                    }
                }
            } else {
                // publish the new route
                discovery.publish(route, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
                lruCache.put(route.getDestPeerID(), route);
                return true;
            }
        } catch (Exception e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "  failure to publish route advertisement response", e);
            }
        }
        return false;
    }
}
