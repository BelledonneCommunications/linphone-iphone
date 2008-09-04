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

import java.util.Enumeration;

import net.jxta.discovery.DiscoveryService;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.OutgoingMessageEvent;
import net.jxta.endpoint.OutgoingMessageEventListener;
import net.jxta.id.ID;
import net.jxta.impl.util.TimeUtils;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import java.util.logging.Level;

import net.jxta.logging.Logging;

import java.util.logging.Logger;

import net.jxta.impl.endpoint.EndpointUtils;

/**
 * Manages a connection with a remote client or a rendezvous peer.
 */
public abstract class PeerConnection implements OutgoingMessageEventListener {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(PeerConnection.class.getName());

    protected final PeerGroup group;
    protected final EndpointService endpoint;

    /**
     * ID of the remote peer.
     */
    protected final ID peerid;

    /**
     * Cached name of the peer for display purposes.
     */
    protected String peerName = null;

    /**
     * If true then we believe we are still connected to the remote peer.
     */
    protected volatile boolean connected = true;

    /**
     * The absolute time in milliseconds at which we expect this connection to
     * expire unless renewed.
     */
    protected long leasedTil = -1;

    /**
     * A cached messenger to be used for sending messages to the remote peer.
     */
    protected Messenger cachedMessenger = null;

    /**
     * Constructor for the PeerConnection object
     *
     * @param group    group context
     * @param endpoint the endpoint service to use for sending messages.
     * @param peerid   destination peerid
     */
    public PeerConnection(PeerGroup group, EndpointService endpoint, ID peerid) {
        this.group = group;
        this.endpoint = endpoint;
        this.peerid = peerid;

        this.peerName = peerid.toString();
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>performs PeerID comparison
     */
    @Override
    public boolean equals(Object obj) {
        return obj instanceof PeerConnection && peerid.equals(((PeerConnection) obj).peerid);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return peerid.hashCode();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        return getPeerName() + (connected ? " C" : " c") + " : " + Long.toString(TimeUtils.toRelativeTimeMillis(leasedTil));
    }

    /**
     * {@inheritDoc}
     */
    public void messageSendFailed(OutgoingMessageEvent event) {
        // If it's just a case of queue overflow, ignore it.
        if (event.getFailure() == null) {
            return;
        }
        setConnected(false);
    }

    /**
     * {@inheritDoc}
     */
    public void messageSendSucceeded(OutgoingMessageEvent event) {// hurray!
    }

    /**
     * Get the peer id of the peer associated with this connection.
     *
     * @return The peer id of the connected peer.
     */
    public ID getPeerID() {
        return peerid;
    }

    /**
     * Get the peer name. If the symbolic name is available, use it,
     * otherwise returns the peer id.
     *
     * @return The name of the connected peer.
     */
    public String getPeerName() {
        return peerName;
    }

    /**
     * set the peer name.
     *
     * @param name the peer name
     */
    protected void setPeerName(String name) {
        peerName = name;
    }

    /**
     * Set the lease duration in relative milliseconds.
     *
     * @param leaseDuration the lease duration in relative milliseconds.
     */
    protected void setLease(long leaseDuration) {
        leasedTil = TimeUtils.toAbsoluteTimeMillis(leaseDuration);
    }

    /**
     * Time at which the lease will expire in absolute milliseconds.
     *
     * @return The lease value
     */
    public long getLeaseEnd() {
        return leasedTil;
    }

    /**
     * Declare that we are connected for the specified amount of time.
     *
     * @param leaseDuration The duration of the lease in relative milliseconds.
     */
    protected void connect(long leaseDuration) {
        setLease(leaseDuration);
        setConnected(true);
    }

    /**
     * Test if the connection is still active.
     *
     * @return The connected value
     */
    public boolean isConnected() {
        connected &= (TimeUtils.toRelativeTimeMillis(leasedTil) >= 0);

        return connected;
    }

    /**
     * Set the connection state. This operation must be idempotent.
     *
     * @param isConnected The new connected state. Be very careful when
     *                    setting <code>true</code> state without setting a new lease.
     */
    public void setConnected(boolean isConnected) {
        connected = isConnected;
    }

    /**
     * Return a messenger suitable for communicating to this peer.
     *
     * @return a messenger for sending to this peer or <code>null</code> if
     *         none is available.
     * @deprecated Preferred style is to pass the connection object around and
     *             use the sendMessage method rather than getting the messenger.
     */
    @Deprecated
    protected Messenger getCachedMessenger() {

        // We don't do the check on existing messenger under synchronization
        // hence the temporary variable.
        Messenger result = cachedMessenger;

        if ((null == result) || result.isClosed()) {
            // We need a new messenger.
            PeerAdvertisement padv = null;

            DiscoveryService discovery = group.getDiscoveryService();

            // Try to see if we have a peer advertisement for this peer.
            // This is very likely.
            if (null != discovery) {
                try {
                    Enumeration each = discovery.getLocalAdvertisements(DiscoveryService.PEER, "PID", peerid.toString());

                    if (each.hasMoreElements()) {
                        padv = (PeerAdvertisement) each.nextElement();
                    }
                } catch (Exception ignored) {
                    //ignored
                }
            }
            result = getCachedMessenger(padv);
        }

        return result;
    }

    /**
     * Return a messenger suitable for communicating to this peer.
     *
     * @param padv A peer advertisement which will be used for route hints if
     *             a new messenger is needed.
     * @return a messenger for sending to this peer or <code>null</code> if
     *         none is available.
     */
    protected synchronized Messenger getCachedMessenger(PeerAdvertisement padv) {
        if ((null != padv) && !peerid.equals(padv.getPeerID())) {
            throw new IllegalArgumentException("Peer Advertisement does not match connection");
        }

        if ((null != padv) && (null != padv.getName())) {
            setPeerName(padv.getName());
        }

        // if we have a good messenger then re-use it.
        if ((null != cachedMessenger) && !cachedMessenger.isClosed()) {
            return cachedMessenger;
        }

        cachedMessenger = null;

        if (isConnected()) {
            // we only get new messengers while we are connected. It is not
            // worth the effort for a disconnected peer. We WILL use an existing
            // open messenger if we have one though.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Getting new cached Messenger for " + peerName);
            }

            RouteAdvertisement hint = null;

            if (null != padv) {
                hint = EndpointUtils.extractRouteAdv(padv);
            }

            EndpointAddress destAddress = new EndpointAddress(peerid, null, null);

            cachedMessenger = endpoint.getMessenger(destAddress, hint);

            if (null == cachedMessenger) {
                // no messenger? avoid doing more work.
                setConnected(false);
            }
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("connection closed : NOT getting new cached Messenger for " + peerName);
            }
        }

        return cachedMessenger;
    }

    /**
     * Send a message to the remote peer.
     *
     * @param msg     the message to send.
     * @param service The destination service.
     * @param param   Parameters for the destination service.
     * @return <true>true</true> if the message was queued to be sent,
     *         otherwise <code>false</code>. A <code>true</code> result does not mean
     *         that the destination peer will receive the message.
     */
    public boolean sendMessage(Message msg, String service, String param) {
        Messenger messenger = getCachedMessenger();

        if (null != messenger) {
            messenger.sendMessage(msg, service, param, this);
            return true;
        } else {
            return false;
        }
    }
}
