/*
 * Copyright (c) 2002-2007 Sun Micro//Systems, Inc.  All rights reserved.
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
package net.jxta.impl.rendezvous.rpv;

import java.io.IOException;

import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.OutgoingMessageEvent;
import net.jxta.endpoint.OutgoingMessageEventListener;
import net.jxta.impl.util.TimeUtils;
import net.jxta.protocol.RdvAdvertisement;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

/**
 * An element of the PeerView.
 *
 * <p/>The destination address (peerID) is part of PeerViewDestination, which implements the
 * comparable interface. That makes it possible to sort and create ordered lists of
 * PeerViewElements, and to search these lists while knowing only a destination address.
 */
public final class PeerViewElement extends PeerViewDestination implements OutgoingMessageEventListener {

    /**
     *  Logger
     */
    private final static transient Logger LOG = Logger.getLogger(PeerViewElement.class.getName());

    /**
     * EndpointService that this PeerViewElement must use.
     */
    private final EndpointService endpoint;

    /**
     * Absolute time in milliseconds at which this element was created.
     */
    private final long created;

    /**
     * Absolute time in milliseconds at which this element was created.
     */
    private long lastUpdate = 0;

    /**
     * The encapsulated RdvAdvertisement for the Peer this instance
     * represents.
     */
    private RdvAdvertisement radv = null;

    /**
     * True is the remote peer is known to be alive, false otherwise.
     * It is always alive at birth. It may die soon after and we want to
     * generate an event in that case.
     */
    private boolean alive = true;

    /**
     * If true then we are not accepting new messages until something unclogs.
     */
    private volatile boolean throttling = false;

    /**
     * PeerView that owns this PeerViewElement.
     */
    private PeerView peerview = null;

    /**
     *  A cached Messenger for sending to the destination peer.
     */
    private Messenger cachedMessenger = null;

    /**
     * Initialize from a RdvAdvertisement.
     *
     * @param endpoint The endpoint service.
     * @param radv the RdvAdvertisement from which to initialize
     */
    PeerViewElement(EndpointService endpoint, RdvAdvertisement radv) {

        super(radv.getPeerID());

        this.endpoint = endpoint;
        this.radv = radv;

        created = TimeUtils.timeNow();
        lastUpdate = created;
    }

    /**
     *  {@inheritDoc}
     *  <p/>
     *  A simple implementation for debugging. Do not attempt to parse this value!
     */
    @Override
    public String toString() {
        StringBuilder asString = new StringBuilder();

        asString.append('\"');
        asString.append(radv.getName());
        asString.append('\"');
        asString.append(alive ? " A " : " a ");
        asString.append(isInPeerView() ? " P " : " p ");
        asString.append(throttling ? " T " : " t ");
        asString.append(" [");
        asString.append(TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), created) / TimeUtils.ASECOND);
        asString.append("/");
        asString.append(TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), lastUpdate) / TimeUtils.ASECOND);
        asString.append("]");
        return asString.toString();
    }

    /**
     * {@inheritDoc}
     */
    public void messageSendSucceeded(OutgoingMessageEvent e) {

        // As far as we know, connectivity is fine.
        setAlive(true, true);

        throttling = false;
    }

    /**
     * {@inheritDoc}
     */
    public void messageSendFailed(OutgoingMessageEvent e) {

        // As far as we know, connectivity is down.
        // Except if failure is null; then it's just a queue overflow.

        if (null != e.getFailure()) {
            setAlive(false, true);
        }

        throttling = (e.getFailure() == null);
    }

    /**
     * Return <code>true</code> if the remote peer is known to be alive,
     * <code>false</code> otherwise.
     *
     * @return Return <code>true</code> if the remote peer is known to be
     * alive, <code>false</code> otherwise.
     */
    public boolean isAlive() {
        return alive;
    }

    /**
     * Update the connection status based upon the result of the last message
     * send.
     *
     * <p/>We track the current dead-alive state and If we're in a peerview
     * notify it of the transitions from alive to dead.
     *
     * @param live The known liveness of our connection to this peer.
     * @param doNotify {@code true} will cause failure notifications to be sent.
     * {@code false} makes notifications the caller's responsibility.
     * @return {@code true} if a failure notification needs to be sent otherwise
     * {@code false}.
     */
    boolean setAlive(boolean live, boolean doNotify) {
        boolean mustNotify;

        synchronized (this) {
            mustNotify = alive && !live;
            alive = live;
        }

        // Since we do this out of sync, it is in theory possible that our alive
        // status has already changed. It is rare but will only cause a little
        // shake. So leave the sync behind, it causes a deadlock.
        if (mustNotify && doNotify) {
            PeerView temp = peerview;

            if (null != temp) {
                temp.notifyFailure(this, true);
            }
        }
        
        return mustNotify;
    }

    boolean isInPeerView() {
        return (null != peerview);
    }

    /**
     *  Sets the peerview
     */
    synchronized void setPeerView(PeerView pv) {
        if ((null != peerview) && (null != pv)) {
            throw new IllegalStateException("Element already in " + peerview);
        }

        peerview = pv;
    }

    /**
     *  Return the time in absolute milliseconds at which we last updated this peer.
     */
    long getLastUpdateTime() {
        return lastUpdate;
    }

    /**
     *  Sets the time in absolute milliseconds at which we last updated this peer.
     */
    void setLastUpdateTime(long last) {
        lastUpdate = last;
    }

    /**
     * Send a message to the peer which is represented by the current
     * PeerViewElement.
     *
     * @param msg the message to send
     *
     * @param serviceName the service name on the destination peer to
     * which the message will be demultiplexed
     *
     * @param serviceParam the service param on the destination peer
     * to which the message will be demultiplexed
     *
     * @return true if the message was successfully handed off to the
     * endpoint for delivery, false otherwise
     */
    public boolean sendMessage(Message msg, String serviceName, String serviceParam) {

        if (throttling) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Declining to send -- throttling on " + this);
            }
            return false;
        }

        Messenger sendVia = getCachedMessenger();

        if (null == sendVia) {
            // There is nothing really we can do.
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Could not get messenger for " + getPeerID());
            }

            OutgoingMessageEvent event = new OutgoingMessageEvent(msg,
                    new IOException("Couldn't get messenger for " + getPeerID()));

            messageSendFailed(event);
            return false;
        }

        sendVia.sendMessage(msg, serviceName, serviceParam, this);

        return true;
    }

    /**
     * Get the encapsulated Peer Advertisement.
     *
     * @return the Advertisement of the Peer represented by this
     * object
     */
    public RdvAdvertisement getRdvAdvertisement() {
        return radv;
    }

    /**
     * Set the encapsulated Peer Advertisement.
     *
     * @param adv is the RdvAdvertisement to be set.
     * @return RdvAdvertisement the old Advertisement of the Peer represented by this
     * object
     */
    RdvAdvertisement setRdvAdvertisement(RdvAdvertisement adv) {

        if (!radv.getPeerID().equals(adv.getPeerID())) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("adv refers to a different peer");
            }

            throw new IllegalArgumentException("adv refers to a different peer");
        }

        RdvAdvertisement old = radv;

        this.radv = adv;

        setLastUpdateTime(TimeUtils.timeNow());

        return old;
    }

    /**
     *  Return a messenger suitable for sending to this peer.
     *
     *  @return a messenger to this PVE peer or if {@code null} if peer is
     *  unreachable.
     */
    private Messenger getCachedMessenger() {

        boolean mustNotify = false;
        
        synchronized (this) {
            if ((null == cachedMessenger) || ((cachedMessenger.getState() & Messenger.USABLE) == 0)) {
                cachedMessenger = null;

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Getting cached Messenger for " + radv.getName());
                }

                cachedMessenger = endpoint.getMessengerImmediate(getDestAddress(), radv.getRouteAdv());
                
                if (null == cachedMessenger) {
                    mustNotify = setAlive(false, false);
                } else if ((cachedMessenger.getState() & Messenger.RESOLVED) != 0) {
                    mustNotify = setAlive(true, false);
                }
            }
        }

        // Since we do this out of sync, it is in theory possible that our alive
        // status has already changed. It is rare but will only cause a little
        // shake. So leave the sync behind, it causes a deadlock.
        if (mustNotify) {
            PeerView temp = peerview;

            if (null != temp) {
                temp.notifyFailure(this, true);
            }
        }

        return cachedMessenger;
    }
}
