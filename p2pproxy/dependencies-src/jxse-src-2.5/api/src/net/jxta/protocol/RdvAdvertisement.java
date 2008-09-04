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
package net.jxta.protocol;


import java.io.ByteArrayInputStream;
import java.io.InputStream;

import net.jxta.document.ExtendableAdvertisement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;


/**
 *  This class defines a Rendezvous Advertisement. This advertisement is indexed
 *  on "RdvGroupId", "RdvPeerId", and "RdvServiceName"
 */
public abstract class RdvAdvertisement extends ExtendableAdvertisement {

    /**
     *  GroupID tag
     */
    public final static String GroupIDTag = "RdvGroupId";

    /**
     *  Name tag
     */
    public final static String NameTag = "Name";

    /**
     *  Rendezvous ID tag
     */
    public final static String PeerIDTag = "RdvPeerId";

    /**
     *  Route tage
     */
    public final static String RouteTag = "RdvRoute";

    /**
     *  Rendezvous service name tag
     */
    public final static String ServiceNameTag = "RdvServiceName";

    private PeerGroupID groupId = null;

    private transient ID hashID = null;
    private String name = null;
    private PeerID peerId = null;
    private RouteAdvertisement route = null;
    private String serviceName = null;

    /**
     *  Returns the identifying type of this Advertisement.
     *
     *@return    String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:RdvAdvertisement";
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     *  get the group id
     *
     *@return    String PeerGroupID
     */
    public PeerGroupID getGroupID() {
        return groupId;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public synchronized ID getID() {

        if (groupId == null) {
            throw new IllegalStateException("cannot build ID: no group id");
        }

        if (peerId == null) {
            throw new IllegalStateException("cannot build ID: no peer id");
        }

        if (serviceName == null) {
            throw new IllegalStateException("cannot build ID: no Service Name");
        }

        if (hashID == null) {
            try {
                // We have not yet built it. Do it now
                String seed = getAdvertisementType() + groupId.toString() + serviceName + peerId.toString();

                InputStream in = new ByteArrayInputStream(seed.getBytes());

                hashID = IDFactory.newCodatID(groupId, seed.getBytes(), in);
            } catch (Exception ez) {
                IllegalStateException failure = new IllegalStateException("cannot build ID");

                failure.initCause(ez);
                throw failure;
            }
        }
        return hashID;
    }

    /**
     *  get the symbolic name associated with the rdv
     *
     *@return    String the name field. null is returned if no name has been
     *      associated with the advertisement.
     */
    public String getName() {

        return name;
    }

    /**
     *  get the rdv peer id
     *
     *@return    PeerID
     */
    public PeerID getPeerID() {
        return peerId;
    }

    /**
     *  Get the Route Adv.
     *
     *@return    RouteAdvertisement or <code>null</code> if no
     */
    public RouteAdvertisement getRouteAdv() {
        return route;
    }

    /**
     *  get the rdv service name
     *
     *@return    String name
     */
    public String getServiceName() {

        return serviceName;
    }

    /**
     *  set the group Id
     *
     *@param  id  The new groupID value
     */
    public void setGroupID(PeerGroupID id) {
        groupId = id;
    }

    /**
     *  set the symbolic name associated with the rdv
     *
     *@param  n  the name this rdv adv should have.
     */
    public void setName(String n) {
        name = n;
    }

    /**
     *  set the peer Id
     *
     *@param  id  The new peerID value
     */
    public void setPeerID(PeerID id) {
        peerId = id;
    }

    /**
     *  set the RouteAdvertisement
     *
     *@param  route  RouteAdvertisement
     */
    public void setRouteAdv(RouteAdvertisement route) {
        this.route = route;
    }

    /**
     *  set the service name
     *
     *@param  n  The new serviceName value
     */

    public void setServiceName(String n) {
        serviceName = n;
    }

}

