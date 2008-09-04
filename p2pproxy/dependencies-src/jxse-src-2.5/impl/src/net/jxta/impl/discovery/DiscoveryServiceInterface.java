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


import java.util.Enumeration;
import java.io.IOException;

import net.jxta.discovery.DiscoveryListener;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.id.ID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.service.Service;


/**
 * Provides a pure interface object that permits interaction with the actual
 * Discovery Service implementation without giving access to the real object.
 */
public final class DiscoveryServiceInterface implements DiscoveryService {

    private DiscoveryServiceImpl impl;

    /**
     * Only authorized constructor
     *
     * @param  theRealThing  The actual discovery implementation
     */
    protected DiscoveryServiceInterface(DiscoveryServiceImpl theRealThing) {
        impl = theRealThing;
    }

    /**
     *  {@inheritDoc}
     */
    public Service getInterface() {
        return this;
    }

    /**
     *  {@inheritDoc}
     */
    public Advertisement getImplAdvertisement() {
        return impl.getImplAdvertisement();
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
    public void init(PeerGroup pg, ID assignedID, Advertisement impl) {}
    
    /**
     *  {@inheritDoc}
     *
     * <p/>This is here for temporary class hierarchy reasons.
     * it is ALWAYS ignored. By definition, the interface object
     * protects the real object's start/stop methods from being called
     */
    public int startApp(String[] arg) {
        return 0;
    }
    
    /**
     *  {@inheritDoc}
     *
     * <p/>This is here for temporary class hierarchy reasons.
     * it is ALWAYS ignored. By definition, the interface object
     * protects the real object's start/stop methods from being called
     *
     * <p/>This request is currently ignored.
     */
    public void stopApp() {}

    /**
     *  {@inheritDoc}
     */
    public int getRemoteAdvertisements(String peer, int type, String attribute, String value, int threshold) {

        return impl.getRemoteAdvertisements(peer, type, attribute, value, threshold);
    }

    /**
     *  {@inheritDoc}
     */
    public int getRemoteAdvertisements(String peer, int type, String attribute, String value, int threshold, DiscoveryListener listener) {
        return impl.getRemoteAdvertisements(peer, type, attribute, value, threshold, listener);
    }

    /**
     *  {@inheritDoc}
     */
    public Enumeration getLocalAdvertisements(int type, String attribute, String value) throws IOException {
        return impl.getLocalAdvertisements(type, attribute, value);
    }

    /**
     *  {@inheritDoc}
     */
    public void flushAdvertisement(Advertisement adv) throws IOException {

        impl.flushAdvertisement(adv);
    }

    /**
     *  {@inheritDoc}
     */
    public void flushAdvertisements(String id, int type) throws IOException {
        impl.flushAdvertisements(id, type);
    }

    /**
     *  {@inheritDoc}
     */
    public long getAdvExpirationTime(ID id, int type) {
        return impl.getAdvExpirationTime(id, type);
    }

    /**
     *  {@inheritDoc}
     */
    public long getAdvLifeTime(ID id, int type) {
        return impl.getAdvLifeTime(id, type);
    }

    /**
     *  {@inheritDoc}
     */
    public long getAdvExpirationTime(Advertisement adv) {
        return impl.getAdvExpirationTime(adv);
    }

    /**
     *  {@inheritDoc}
     */
    public long getAdvLifeTime(Advertisement adv) {
        return impl.getAdvLifeTime(adv);
    }

    /**
     *  {@inheritDoc}
     */
    public void publish(Advertisement adv) throws IOException {
        impl.publish(adv);
    }

    /**
     *  {@inheritDoc}
     */
    public void publish(Advertisement adv, long lifetime, long expiration) throws IOException {

        impl.publish(adv, lifetime, expiration);
    }

    /**
     *  {@inheritDoc}
     */
    public void remotePublish(Advertisement adv) {
        impl.remotePublish(adv);
    }
    
    /**
     *  {@inheritDoc}
     */
    public void remotePublish(Advertisement adv, long expiration) {
        impl.remotePublish(adv, expiration);
    }

    /**
     *  {@inheritDoc}
     */
    public void remotePublish(String peerid, Advertisement adv) {
        impl.remotePublish(peerid, adv);
    }

    /**
     *  {@inheritDoc}
     */
    public void remotePublish(String peerid, Advertisement adv, long expiration) {
        impl.remotePublish(peerid, adv, expiration);
    }

    /**
     *  {@inheritDoc}
     */
    public synchronized void addDiscoveryListener(DiscoveryListener listener) {
        impl.addDiscoveryListener(listener);
    }

    /**
     *  {@inheritDoc}
     */
    public synchronized boolean removeDiscoveryListener(DiscoveryListener listener) {
        return (impl.removeDiscoveryListener(listener));
    }
}

