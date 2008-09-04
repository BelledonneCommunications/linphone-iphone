/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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
package net.jxta.impl.peergroup;

import net.jxta.access.AccessService;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.Element;
import net.jxta.endpoint.EndpointService;
import net.jxta.exception.PeerGroupException;
import net.jxta.exception.ProtocolNotSupportedException;
import net.jxta.exception.ServiceNotFoundException;
import net.jxta.id.ID;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peer.PeerInfoService;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.pipe.PipeService;
import net.jxta.platform.JxtaLoader;
import net.jxta.platform.Module;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.resolver.ResolverService;
import net.jxta.service.Service;
import java.util.logging.Logger;

import java.io.IOException;
import java.net.URI;
import java.util.Iterator;

/**
 * PeerGroupInterface provides a pure interface object that permits interaction
 * with the actual PeerGroup implementation without giving access to the real
 * object.
 * <p/>
 * This class defines immutable objects. It has no control over the wrapped
 * peer group object's life cycle. It serves to make weak PeerGroup interface
 * object.
 */
class PeerGroupInterface implements PeerGroup {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(PeerGroupInterface.class.getName());

    /**
     * The peer group instance which backs this interface object.
     */
    protected PeerGroup groupImpl;

    /**
     * Constructs an interface object that front-ends a given GenericPeerGroup.
     * @param theRealThing the real PeerGroup
     */
    PeerGroupInterface(PeerGroup theRealThing) {
        groupImpl = theRealThing;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        PeerGroup temp = groupImpl;

        if (null != temp) {
            return temp.equals(target);
        } else {
            return super.equals(target);
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * An implementation suitable for debugging. <b>Don't try to parse
     * this string!</b> All of the information is available from other sources.
     */
    @Override
    public String toString() {
        PeerGroup temp = groupImpl;

        if (null != temp) {
            return temp.toString();
        } else {
            return super.toString();
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This is here for class hierarchy reasons. It is normaly ignored. By
     * definition, the interface object protects the real object's start/stop
     * methods from being called.
     */

    public void init(PeerGroup pg, ID assignedID, Advertisement impl) {}

    /**
     * {@inheritDoc}
     * <p/>
     * This is here for class hierarchy reasons. It is normaly ignored. By
     * definition, the interface object protects the real object's start/stop
     * methods from being called.
     */
    public int startApp(String[] arg) {
        return 0;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This is here for class hierarchy reasons. It is normaly ignored. By
     * definition, the interface object protects the real object's start/stop
     * methods from being called.
     */
    public void stopApp() {}

    /**
     * {@inheritDoc}
     */
    public Service getInterface() {
        return this;
    }

    /**
     * {@inheritDoc}
     */
    public PeerGroup getWeakInterface() {
        return this;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Does nothing.
     */
    public void unref() {}

    /**
     * {@inheritDoc}
     */
    public Advertisement getImplAdvertisement() {
        return groupImpl.getImplAdvertisement();
    }

    /**
     * {@inheritDoc}
     */
    public ThreadGroup getHomeThreadGroup() {
        return groupImpl.getHomeThreadGroup();
    }

    /**
     * {@inheritDoc}
     */
    public URI getStoreHome() {
        return groupImpl.getStoreHome();
    }

    /**
     * {@inheritDoc}
     */
    public JxtaLoader getLoader() {
        return groupImpl.getLoader();
    }

    /**
     * {@inheritDoc}
     */
    public boolean isRendezvous() {
        return groupImpl.isRendezvous();
    }

    /**
     * {@inheritDoc}
     */
    public PeerGroupAdvertisement getPeerGroupAdvertisement() {
        return groupImpl.getPeerGroupAdvertisement();
    }

    /**
     * {@inheritDoc}
     */
    public PeerAdvertisement getPeerAdvertisement() {
        return groupImpl.getPeerAdvertisement();
    }

    /**
     * {@inheritDoc}
     */
    public Service lookupService(ID name) throws ServiceNotFoundException {
        return groupImpl.lookupService(name);
    }

    /**
     * {@inheritDoc}
     */
    public Service lookupService(ID name, int roleIndex) throws ServiceNotFoundException {
        return groupImpl.lookupService(name, roleIndex);
    }

    /**
     * {@inheritDoc}
     */
    public Iterator getRoleMap(ID name) {
        return groupImpl.getRoleMap(name);
    }

    /**
     * {@inheritDoc}
     */
    public boolean compatible(Element compat) {
        return groupImpl.compatible(compat);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * FIXME 20031103 jice Ideally, we'd need the groupAPI to offer a means to
     * loadModule() without making a counted reference, so that group services
     * can loadModule() things without preventing group termination. This could
     * be achieved elegantly by making this the only behaviour available through
     * a weak GroupInterface. So it would be enough to obtain a weak interface
     * from one's group and then use its loadmodule method rather than that of
     * the strong group reference.  However, that's a bit too big a change to be
     * decided without more careful consideration.
     */
    public Module loadModule(ID assignedID, Advertisement impl) throws ProtocolNotSupportedException, PeerGroupException {
        return groupImpl.loadModule(assignedID, impl);
    }

    /**
     * {@inheritDoc}
     */
    public Module loadModule(ID assignedID, ModuleSpecID specID, int where) {
        return groupImpl.loadModule(assignedID, specID, where);
    }

    /**
     * {@inheritDoc}
     */
    public void publishGroup(String name, String description) throws IOException {
        groupImpl.publishGroup(name, description);
    }

    /*
     * Valuable application helpers: Various methods to instantiate groups.
     */

    /**
     * {@inheritDoc}
     */
    public PeerGroup newGroup(Advertisement pgAdv) throws PeerGroupException {
        return groupImpl.newGroup(pgAdv);
    }

    /**
     * {@inheritDoc}
     */
    public PeerGroup newGroup(PeerGroupID gid, Advertisement impl, String name, String description) throws PeerGroupException {
        return groupImpl.newGroup(gid, impl, name, description);
    }

    /**
     * {@inheritDoc}
     */
    public PeerGroup newGroup(PeerGroupID gid) throws PeerGroupException {
        return groupImpl.newGroup(gid);
    }

    /*
     * shortcuts to the well-known services, in order to avoid calls to lookup.
     */

    /**
     * {@inheritDoc}
     */
    public RendezVousService getRendezVousService() {
        return groupImpl.getRendezVousService();
    }

    /**
     * {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        return groupImpl.getEndpointService();
    }

    /**
     * {@inheritDoc}
     */
    public ResolverService getResolverService() {
        return groupImpl.getResolverService();
    }

    /**
     * {@inheritDoc}
     */
    public DiscoveryService getDiscoveryService() {
        return groupImpl.getDiscoveryService();
    }

    /**
     * {@inheritDoc}
     */
    public PeerInfoService getPeerInfoService() {
        return groupImpl.getPeerInfoService();
    }

    /**
     * {@inheritDoc}
     */
    public MembershipService getMembershipService() {
        return groupImpl.getMembershipService();
    }

    /**
     * {@inheritDoc}
     */
    public PipeService getPipeService() {
        return groupImpl.getPipeService();
    }

    /**
     * {@inheritDoc}
     */
    public AccessService getAccessService() {
        return groupImpl.getAccessService();
    }

    /*
     * A few convenience methods. This information is available from
     * the peer and peergroup advertisement.
     */

    /**
     * {@inheritDoc}
     */
    public PeerGroupID getPeerGroupID() {
        return groupImpl.getPeerGroupID();
    }

    /**
     * {@inheritDoc}
     */
    public PeerID getPeerID() {
        return groupImpl.getPeerID();
    }

    /**
     * {@inheritDoc}
     */
    public String getPeerGroupName() {
        return groupImpl.getPeerGroupName();
    }

    /**
     * {@inheritDoc}
     */
    public String getPeerName() {
        return groupImpl.getPeerName();
    }

    /**
     * {@inheritDoc}
     */
    public ConfigParams getConfigAdvertisement() {
        ConfigParams configAdvertisement = groupImpl.getConfigAdvertisement();

        if (configAdvertisement == null) {
            return null;
        }
        return configAdvertisement.clone();
    }

    /**
     * {@inheritDoc}
     */
    public ModuleImplAdvertisement getAllPurposePeerGroupImplAdvertisement() throws Exception {
        return groupImpl.getAllPurposePeerGroupImplAdvertisement();
    }

    public PeerGroup getParentGroup() {
        return groupImpl.getParentGroup();
    }
}
