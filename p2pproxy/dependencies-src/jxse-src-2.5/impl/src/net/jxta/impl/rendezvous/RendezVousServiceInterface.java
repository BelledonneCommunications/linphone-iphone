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
package net.jxta.impl.rendezvous;

import java.util.Collections;
import java.util.Enumeration;
import java.util.Vector;

import java.io.IOException;
import java.util.Collection;

import net.jxta.document.Advertisement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.id.ID;
import net.jxta.protocol.RdvAdvertisement;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.rendezvous.RendezVousStatus;
import net.jxta.rendezvous.RendezvousListener;
import net.jxta.service.Service;

import net.jxta.impl.rendezvous.rpv.PeerView;
import net.jxta.platform.Module;

/**
 * This class implements the RendezVousService interface.
 */
public class RendezVousServiceInterface implements RendezVousService {

    RendezVousServiceImpl impl = null;

    /**
     * The only authorized constructor.
     *
     * @param theRealThing the wrapped service
     */
    RendezVousServiceInterface(RendezVousServiceImpl theRealThing) {
        impl = theRealThing;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>Since THIS is already such an object, it returns itself.
     * FIXME: it is kind of absurd to have this method part of the
     * interface but we do not want to define two levels of Service interface
     * just for that.
     */
    public Service getInterface() {
        return this;
    }

    /**
     * {@inheritDoc}
     */
    public ModuleImplAdvertisement getImplAdvertisement() {
        return impl.getImplAdvertisement();
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>FIXME: This is meaningless for the interface object;
     * it is there only to satisfy the requirements of the
     * interface that we implement. Ultimately, the API should define
     * two levels of interfaces: one for the real service implementation
     * and one for the interface object. Right now it feels a bit heavy
     * to so that since the only different between the two would be
     * init() and may-be getName().
     */

    public void init(PeerGroup pg, ID assignedID, Advertisement impl) {
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>This is here for temporary class hierarchy reasons.
     * it is ALWAYS ignored. By definition, the interface object
     * protects the real object's start/stop methods from being called
     */
    public int startApp(String[] arg) {
        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>This is here for temporary class hierarchy reasons.
     * it is ALWAYS ignored. By definition, the interface object
     * protects the real object's start/stop methods from being called
     * <p/>
     * <p/>This request is currently ignored.
     */
    public void stopApp() {
    }

    /**
     * {@inheritDoc}
     */
    public void connectToRendezVous(PeerAdvertisement adv) throws IOException {
        impl.connectToRendezVous(adv);
    }

    /**
     * {@inheritDoc}
     */
    public void connectToRendezVous(EndpointAddress addr) throws IOException {
        impl.connectToRendezVous(addr);
    }

    /**
     * {@inheritDoc}
     */
    public void challengeRendezVous(ID peer, long delay) {
        impl.challengeRendezVous(peer, delay);
    }

    /**
     * {@inheritDoc}
     */
    public void disconnectFromRendezVous(ID rendezVous) {
        impl.disconnectFromRendezVous(rendezVous);
    }

    /**
     * {@inheritDoc}
     */
    public Enumeration<ID> getConnectedRendezVous() {
        Collection<ID> connectedPeers = getConnectedPeerIDs();

        return Collections.enumeration(connectedPeers);
    }

    /**
     * {@inheritDoc}
     */
    public Enumeration<ID> getDisconnectedRendezVous() {
        Collection<ID> empty = Collections.emptyList();

        return Collections.enumeration(empty);
    }

    /**
     * This portion is for peers that are RendezVous
     */

    /**
     * {@inheritDoc}
     */
    public void startRendezVous() {
        impl.startRendezVous();
    }

    /**
     * {@inheritDoc}
     */
    public void stopRendezVous() {
        impl.stopRendezVous();
    }

    /**
     * {@inheritDoc}
     */
    public Enumeration<ID> getConnectedPeers() {
        Collection<ID> connectedPeers = getConnectedPeerIDs();

        return Collections.enumeration(connectedPeers);
    }

    /**
     * {@inheritDoc}
     */
    public Vector<ID> getConnectedPeerIDs() {
        return impl.getConnectedPeerIDs();
    }

    /**
     * {@inheritDoc}
     */
    public boolean addPropagateListener(String serviceName, String serviceParam, EndpointListener listener) {

        return impl.addPropagateListener(serviceName, serviceParam, listener);
    }

    /**
     * {@inheritDoc}
     */
    public EndpointListener removePropagateListener(String serviceName, String serviceParam, EndpointListener listener) {

        return impl.removePropagateListener(serviceName, serviceParam, listener);
    }

    /**
     * {@inheritDoc}
     */
    public void addListener(RendezvousListener listener) {

        impl.addListener(listener);
    }

    /**
     * {@inheritDoc}
     */
    public boolean removeListener(RendezvousListener listener) {

        return impl.removeListener(listener);
    }

    /**
     * {@inheritDoc}
     */
    public void propagate(Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {

        impl.propagate(msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void propagate(Enumeration<? extends ID> destPeerIDs, Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {

        impl.propagate(destPeerIDs, msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void walk(Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {

        impl.walk(msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void walk(Vector<? extends ID> destPeerIDs, Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {

        impl.walk(destPeerIDs, msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public Vector<RdvAdvertisement> getLocalWalkView() {

        return impl.getLocalWalkView();
    }

    /**
     * {@inheritDoc}
     */
    public void propagateToNeighbors(Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {
        impl.propagateToNeighbors(msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void propagateInGroup(Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {
        impl.propagateInGroup(msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public boolean isConnectedToRendezVous() {
        return impl.isConnectedToRendezVous();
    }

    /**
     * {@inheritDoc}
     */
    public boolean isRendezVous() {
        return impl.isRendezVous();
    }

    /**
     * @inheritDoc
     */
    public RendezVousStatus getRendezVousStatus() {
        return impl.getRendezVousStatus();
    }

    /**
     * {@inheritDoc}
     */
    public boolean setAutoStart(boolean auto) {
        return impl.setAutoStart(auto);
    }

    /**
     * {@inheritDoc}
     */
    public boolean setAutoStart(boolean auto, long period) {
        return impl.setAutoStart(auto, period);
    }

    /**
     * Get the current peerview. This is for debugging purposes only.
     *
     * @return the peer view
     * @deprecated This is private for debugging and diagnostics only.
     */
    @Deprecated
    public PeerView getPeerView() {
        return impl.getPeerView();
    }

    /**
     * Get the current provider. This is for debugging purposes only.
     *
     * @return the provider
     * @deprecated This is private for debugging and diagnostics only.
     */
    @Deprecated
    public net.jxta.impl.rendezvous.RendezVousServiceProvider getRendezvousProvider() {
        return impl.getRendezvousProvider();
    }
}
