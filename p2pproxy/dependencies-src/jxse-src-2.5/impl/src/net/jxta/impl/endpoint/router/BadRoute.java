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


import net.jxta.peer.PeerID;
import net.jxta.protocol.RouteAdvertisement;

import java.util.HashSet;
import java.util.Set;


/**
 * This class is used to cache negative route information. Bad routes are
 * represented by three fields:
 *
 * <p/><ul>
 * <li>Route advertisement,</li>
 * <li>Lists of hops that are known bad for that route.</li>
 * <li>Expiration time of the negative cache.</li>
 * </ul>
 */
class BadRoute {
    
    /**
     * The RouteAdvertisement with known bad hops.
     */
    private final RouteAdvertisement badRoute;
    
    /**
     * The PeerID of the known bad hops in the route.
     */
    private final Set<PeerID> badHops = new HashSet<PeerID>();
    
    /**
     * The absolute time at which this information becomes obsolete.
     */
    private long expiration;
    
    BadRoute(RouteAdvertisement route, long exp, Set<PeerID> hops) {
        this.badRoute = route;
        this.expiration = exp;
        this.badHops.addAll(hops);
    }
    
    /**
     * Return the bad route info
     *
     * @return bad route advertisement
     */
    public RouteAdvertisement getRoute() {
        if (badRoute != null) {
            return badRoute.clone();
        } else {
            return null;
        }
    }
    
    /**
     * Return the absolute time at which the this entry expires.
     *
     * @return The absolute time at which the this entry expires.
     */
    public long getExpiration() {
        return expiration;
    }
    
    /**
     * set the bad route expiration time
     *
     * @param exp bad route expiration time
     */
    public void setExpiration(long exp) {
        this.expiration = exp;
    }
    
    /**
     * return the known bad hops in the route
     *
     * @return bad route hops
     */
    public Set<PeerID> getBadHops() {
        Set<PeerID> hops = new HashSet<PeerID>(badHops);
        
        return hops;
    }
    
    /**
     * set bad hops into the bad route
     *
     * @param hops bad route hops
     */
    public void setBadHops(Set<PeerID> hops) {
        
        badHops.clear();
        addBadHops(hops);
    }
    
    /**
     * add bad hops into the bad route
     *
     * @param hops bad route hops
     */
    public void addBadHops(Set<PeerID> hops) {
        badHops.addAll(hops);
    }
    
    /**
     * add a bad hop into the bad route
     *
     * @param hop The bad route hop.
     */
    public void addBadHop(PeerID hop) {
        badHops.add(hop);
    }
    
    /**
     * {@inheritDoc}
     * 
     * <p/>Implementation useful for debugging. Don't depend on the format
     */
    @Override
    public String toString() {
        StringBuilder routeBuf = new StringBuilder();
        
        routeBuf.append("Bad ").append(getRoute().display());
        routeBuf.append("\tExp: ").append(getExpiration());
        routeBuf.append("\tHops: ");
        
        for (PeerID eachBadHop : getBadHops()) {
            routeBuf.append("\t\t").append(eachBadHop);
        }
        
        return routeBuf.toString();
    }
}
