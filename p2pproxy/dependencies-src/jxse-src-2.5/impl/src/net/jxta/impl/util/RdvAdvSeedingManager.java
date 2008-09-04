/*
 * Copyright (c) 2002-2004 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util;

import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.RdvAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

/**
 * Adds the ability to discover RdvAdvs via Discovery.
 */
public class RdvAdvSeedingManager extends ACLSeedingManager {
    
    /**
     *  Logger
     */
    private static final transient Logger LOG = Logger.getLogger(URISeedingManager.class.getName());
    
    /**
     *  The minimum frequence at which we will update our seed lists.
     */
    final static long MIN_REFRESH_INTERVAL = 30 * TimeUtils.ASECOND;
    
    /**
     *  Group who's services we will utilize.
     */
    final PeerGroup group;
    
    /**
     *  The identifier which we use to distinguish our RdvAdvertisements.
     */
    final String serviceName;
    
    /**
     *  The absolute time in milliseconds at which we may sen our next remote
     *  discovery.
     */
    long nextRemoteDiscovery = 0;
    
    /**
     *  The Route Advertisements we have discovered.
     */
    final List<RouteAdvertisement> discoveredRoutes = new ArrayList<RouteAdvertisement>();
    
    /**
     * Creates a new instance of RdvAdvSeedingManager
     *
     * @param aclLocation The location of the ACL file or {@code null} if no
     * ACL file should be used.
     */
    public RdvAdvSeedingManager(URI aclLocation, PeerGroup group, String serviceName) {
        super(aclLocation);
        
        this.group = group;
        this.serviceName = serviceName;
    }
    
    /**
     *  Update seeds
     */
    private void refreshActiveSeeds() {
        DiscoveryService discovery = group.getDiscoveryService();
        
        if((null != discovery) && (TimeUtils.timeNow() > nextRemoteDiscovery)) {
            // Send a remote search hoping for future responses.
            discovery.getRemoteAdvertisements(null, DiscoveryService.ADV, RdvAdvertisement.ServiceNameTag, serviceName, 3);
            
            Enumeration<Advertisement> advs;
            try {
                advs = discovery.getLocalAdvertisements(DiscoveryService.ADV, RdvAdvertisement.ServiceNameTag, serviceName);
            } catch( IOException failed ) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure retrieving advertisements" , failed);
                }
                
                return;
            }
            
            synchronized(this) {
                discoveredRoutes.clear();
                
                while(advs.hasMoreElements()) {
                    Advertisement anAdv = advs.nextElement();
                    if(!(anAdv instanceof RdvAdvertisement)) {
                        continue;
                    }
                    
                    RdvAdvertisement rdvAdv = (RdvAdvertisement) anAdv;
                    RouteAdvertisement routeAdv = rdvAdv.getRouteAdv();
                    routeAdv.setDestPeerID(rdvAdv.getPeerID());
                    
                    discoveredRoutes.add(routeAdv);
                }
                
                Collections.shuffle(discoveredRoutes);
                
                if(discoveredRoutes.isEmpty()) {
                    // Be extra aggressive if we haven't found anything yet.
                    nextRemoteDiscovery = TimeUtils.toAbsoluteTimeMillis(MIN_REFRESH_INTERVAL / 2);
                } else {
                    nextRemoteDiscovery = TimeUtils.toAbsoluteTimeMillis(MIN_REFRESH_INTERVAL);
                }
            }
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public void stop() { 
        // do nothing.
    }
    
    /**
     *  {@inheritDoc}
     */
    public synchronized URI[] getActiveSeedURIs() {
        refreshActiveSeeds();        

        List<URI> results = new ArrayList<URI>();
        
        int eaIndex = 0;
        boolean addedEA;
        
        do {
            addedEA = false;
            
            for (RouteAdvertisement aRA : discoveredRoutes) {
                List<EndpointAddress> raEAs = aRA.getDestEndpointAddresses();
                if (eaIndex < raEAs.size()) {
                    URI seedURI = raEAs.get(eaIndex).toURI();
                    if(!results.contains(seedURI)) {
                        results.add(seedURI);
                    }
                    addedEA = true;
                }
            }
            
            // Next loop we use the next most preferred address.
            eaIndex++;
        } while (addedEA);
        
        return results.toArray(new URI[results.size()]);
    }
    
    /**
     *  {@inheritDoc}
     */
    public synchronized RouteAdvertisement[] getActiveSeedRoutes() {
        refreshActiveSeeds();        

        List<RouteAdvertisement> results = new ArrayList<RouteAdvertisement>();
        
        for( RouteAdvertisement eachRoute : discoveredRoutes ) {
            if(!results.contains(eachRoute)) {
                results.add(eachRoute);
            }
        }
        
        return results.toArray(new RouteAdvertisement[results.size()]);
    }
}
