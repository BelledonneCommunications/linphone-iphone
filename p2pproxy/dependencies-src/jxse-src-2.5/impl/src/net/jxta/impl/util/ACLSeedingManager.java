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


import java.io.InputStream;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;

import java.io.IOException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import net.jxta.impl.access.AccessList;
import net.jxta.impl.endpoint.EndpointUtils;


/**
 * Provides support for the optional access control list which determines which
 * peers may be used.
 */
public abstract class ACLSeedingManager implements SeedingManager {
    
    /**
     * Logger
     */
    private static final transient Logger LOG = Logger.getLogger(ACLSeedingManager.class.getName());
    
    /**
     * The interval in milliseconds at which the ACL be refreshed from the
     * source.
     */
    private static final long ACL_REFRESH_INTERVAL = 30 * TimeUtils.AMINUTE;
    
    /**
     *  The access control list which controls which hosts are allowed.
     */
    private final URI aclLocation;
    
    /**
     *  The last known modification time of the ACL.
     */
    private long aclLastModified = 0;
    
    /**
     *  Manages access to the seeds.
     */
    protected final AccessList acl = new AccessList();
    
    /**
     *  The absolute time in milliseconds after which we will attempt to refresh
     *  the access control list from the acl URI.
     */
    private long nextACLrefreshTime = 0;
    
    /**
     *  Constructs a new ACL seeding manager.
     *
     *  @param aclLocation The location of the ACL file or {@code null} if no
     *  ACL file should be used.
     */
    public ACLSeedingManager(URI aclLocation) {
        this.aclLocation = aclLocation;
        
        // Default to allowing all peers.
        acl.setGrantAll(true);
        if (null == aclLocation) {
            // forever.
            nextACLrefreshTime = Long.MAX_VALUE;
        }
    }

    /**
     * {@inheritDoc}
     *
     * <p/>Performs it's determination based solely on the list of peers in
     * the access list.
     */
    public boolean isAcceptablePeer(PeerAdvertisement peeradv) {
        RouteAdvertisement route = EndpointUtils.extractRouteAdv(peeradv);
        
        if (null != route) {
            return isAcceptablePeer(route);
        } else {
            // No route? It's only OK if we are approving everyone.
            return acl.getGrantAll();
        }        
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>Performs it's determination based solely on the list of peers in
     * the access list.
     */
    public synchronized boolean isAcceptablePeer(RouteAdvertisement radv) {
                
        // Refresh the ACL?
        
        if (TimeUtils.timeNow() > nextACLrefreshTime) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Updating ACL");
            }

            try {
                URL asURL = aclLocation.toURL();
                URLConnection connection = asURL.openConnection();
                
                connection.setDoInput(true);
                InputStream is = connection.getInputStream();
                
                long last_mod = connection.getLastModified();
                
                if ((last_mod == 0) || (last_mod > aclLastModified)) {
                    acl.setGrantAll(false);
                    acl.refresh(is);
                }
                
                nextACLrefreshTime = TimeUtils.toAbsoluteTimeMillis(ACL_REFRESH_INTERVAL);
            } catch (IOException failed) {
                // be lenient in response to failures.
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "ACL update failed. GRANTING ALL PERMISSIONS.", failed);
                }

                acl.setGrantAll(true);
                
                nextACLrefreshTime = TimeUtils.toAbsoluteTimeMillis(ACL_REFRESH_INTERVAL / 2);
            }
        }
        
        return acl.isAllowed(radv.getDestPeerID());
    }    
}
