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

package net.jxta.impl.resolver;


import net.jxta.document.Advertisement;
import net.jxta.id.ID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.ResolverSrdiMsg;
import net.jxta.resolver.QueryHandler;
import net.jxta.resolver.ResolverService;
import net.jxta.resolver.SrdiHandler;
import net.jxta.service.Service;


/**
 *  Provides a controlled interface to the Resolver Service.
 **/
public class ResolverServiceInterface implements ResolverService {
    
    private final ResolverServiceImpl impl;
    
    /**
     * Only authorized constructor
     * @param theRealThing reference to the impl
     */
    ResolverServiceInterface(ResolverServiceImpl theRealThing) {
        impl = theRealThing;
    }
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>Since THIS is already such an object, it returns itself.
     * FIXME: it is kind of absurd to have this method part of the
     * interface but we do not want to define two levels of Service interface
     * just for that.
     */
    public ResolverService getInterface() {
        return this;
    }
    
    /**
     *  {@inheritDoc}
     *
     * <p/>FIXME: This is meaningless for the interface object;
     * it is there only to satisfy the requirements of the
     * interface that we implement. Ultimately, the API should define
     * two levels of interfaces: one for the real service implementation
     * and one for the interface object. Right now it feels a bit heavy
     * to so that since the only different between the two would be
     * init() and may-be getName().
     */
    public void init(PeerGroup g, ID assignedID, Advertisement impl) {}
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>Does nothing in the interface object.
     */
    public int startApp(String[] arg) {
        return Module.START_OK;
    }
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>Does nothing in the interface object.
     */
    public void stopApp() {}
    
    /**
     *  {@inheritDoc}
     */
    public ModuleImplAdvertisement getImplAdvertisement() {
        return impl.getImplAdvertisement();
    }
    
    /**
     *  {@inheritDoc}
     */
    public QueryHandler registerHandler(String name, QueryHandler handler) {
        return impl.registerHandler(name, handler);
    }
    
    /**
     *  {@inheritDoc}
     */
    public QueryHandler unregisterHandler(String name) {
        return impl.unregisterHandler(name);
    }
    
    /**
     *  {@inheritDoc}
     */
    public SrdiHandler registerSrdiHandler(String name, SrdiHandler handler) {
        return impl.registerSrdiHandler(name, handler);
    }
    
    /**
     *  {@inheritDoc}
     */
    public SrdiHandler unregisterSrdiHandler(String name) {
        return impl.unregisterSrdiHandler(name);
    }
    
    /**
     *  {@inheritDoc}
     */
    public void  sendQuery(String rdvPeer, ResolverQueryMsg query) {
        impl.sendQuery(rdvPeer, query);
    }
    
    /**
     *  {@inheritDoc}
     */
    public void  sendResponse(String destPeer, ResolverResponseMsg response) {
        impl.sendResponse(destPeer, response);
    }
    
    /**
     *  {@inheritDoc}
     */
    public void sendSrdi(String destPeer, ResolverSrdiMsg srdi) {
        impl.sendSrdi(destPeer, srdi);
    }
}
