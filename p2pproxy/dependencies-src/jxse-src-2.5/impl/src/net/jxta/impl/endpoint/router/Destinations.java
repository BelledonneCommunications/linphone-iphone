/*
 * Copyright (c) 2004-2007 Sun Microsystems, Inc.  All rights reserved.
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


import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Messenger;
import net.jxta.impl.util.TimeUtils;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.lang.ref.Reference;
import java.lang.ref.SoftReference;
import java.util.*;


/**
 * This class is a repository of wisdom regarding destinations. It also provides
 * a messenger if there is one. Currently, the wisdom is very limited and is
 * only about direct destinations (for which a messenger once existed). The
 * wisdom that can be obtained is:
 *
 * <p/><ul>
 * <li> is there a messenger at hand (incoming or otherwise).</li>
 * 
 * <li> is it likely that one can be made from this end, should the one we
 * have break. (the last attempt succeeded, not only incoming, and that was
 * not long ago).</li>
 * 
 * <li> is either of the above true, (are we confident we can get a
 * messenger as of right now one way or the other).</li>
 * 
 * <li> are we supposed to send a welcome to that destination (we can't
 * remember having done it).</li>
 * </ul>
 * 
 * <p/>This could be extended to manage more of the life cycle, such as knowing
 * about messengers being resolved or having failed to. This primitive interface
 * is temporary; it is only meant to replace messengerPool without having to
 * change the router too much.
 */

class Destinations {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(Destinations.class.getName());

    /**
     * Shared Timer which handles cleanup of expired Wisdom.
     */
    private final static transient Timer cleanup = new Timer("Endpoint Destinations GC", true);

    private final Map<EndpointAddress, Wisdom> wisdoms = new HashMap<EndpointAddress, Wisdom>(64);

    /**
     * If {@code true} then we are shutting down.
     */
    private volatile boolean stopped = false;

    /**
     * The endpoint service we are working for.
     */
    private final EndpointService endpoint;

    /**
     * The wisdom GC task for this instance.
     */
    private final WisdomGCTask wisdomGC;

    /**
     * Stores knowledge about one particular destination.
     * 
     * <p/>It does not provide any synchronization. This is provided by the Destinations class.
     */
    final class Wisdom {

        /**
         * How long we consider that a past outgoingMessenger is an indication 
         * that one is possible in the future.
         */
        static final long EXPIRATION = 10 * TimeUtils.AMINUTE;

        /**
         * The channel we last used, if any. They disappear faster than the 
         * canonical, but, as long as the canonical is around, they can be 
         * obtained at a near-zero cost.
         */
        private Reference<Messenger> outgoingMessenger;

        /**
         * The channel we last used if it happens to be an incoming messenger. We keep
         * a strong reference to it.
         */
        private Messenger incomingMessenger;

        /**
         * The transport destination address of the messenger we're caching (if 
         * not incoming).
         */
        private EndpointAddress xportDest;

        /**
         * This tells when the outgoing messenger information expires. Incoming
         * messengers have no expiration per se.  We draw no conclusion from
         * their past presence; only current presence. A wisdom is totally
         * expired (and may thus be removed) when its outgoing messenger
         * information is expired AND it has no incoming messenger.
         */
        private long expiresAt = 0;

        /**
         * When a new destination is added, we're supposed to send our welcome 
         * along with the first message. This tells whether isWelcomeNeeded was 
         * once invoked or not.
         */
        private boolean welcomeNeeded = true;

        /**
         * @param messenger The messenger to cache information about.
         * @param incoming  If true, this is an incoming messenger, which means 
         * that if the channel is lost it cannot be re-obtained. It must 
         * strongly referenced until it closes for disuse, or breaks.
         */
        Wisdom(Messenger messenger, boolean incoming) {
            if (incoming) {
                addIncomingMessenger(messenger);
            } else {
                addOutgoingMessenger(messenger);
            }
        }

        /**
         * Reports whether a welcome message is needed. The first time we're 
         * asked we say "true". Subsequently, always "false".
         *
         * <p/>This assumes that the welcome was sent following the first test.
         * (so, ask only if you'll send the welcome when told to).
         *
         * @return {@code true} if this is the first time this method is invoked.
         */
        synchronized boolean isWelcomeNeeded() {
            boolean res = welcomeNeeded;

            welcomeNeeded = false;
            return res;
        }

        boolean addIncomingMessenger(Messenger m) {

            // If we have no other incoming, we take it. No questions asked.
            Messenger currentIncoming = getIncoming();

            if (currentIncoming == null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Accepted new incoming messenger for " + m.getDestinationAddress());
                }

                incomingMessenger = m;
                return true;
            }

            // Now, check reachability (0 port number means no incoming connections).
            // If the old one looks better, prefer it.

            // Compute reachability of the new one.
            String originAddr = m.getDestinationAddress().getProtocolAddress();

            int index = originAddr.lastIndexOf(':');
            int srcPort = (index != -1) ? Integer.parseInt(originAddr.substring(index + 1)) : 0;
            boolean reachable = (srcPort != 0);

            // Compute reachability of the old one.
            originAddr = currentIncoming.getDestinationAddress().getProtocolAddress();

            index = originAddr.lastIndexOf(':');
            srcPort = (index != -1) ? Integer.parseInt(originAddr.substring(index + 1)) : 0;
            boolean currentReachable = (srcPort != 0);

            // The new one is less reachable than the old one. Keep the old one.
            if (currentReachable && !reachable) {
                return false;
            }

            incomingMessenger = m;

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Accepted new incoming messenger for " + m.getDestinationAddress());
            }

            return true;
        }

        boolean addOutgoingMessenger(Messenger m) {
            if (getOutgoing() != null) {
                return false;
            }
            this.outgoingMessenger = new SoftReference<Messenger>(m);
            xportDest = m.getDestinationAddress();
            expiresAt = TimeUtils.toAbsoluteTimeMillis(EXPIRATION);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Accepted new outgoing messenger for " + xportDest);
            }
            return true;
        }

        void noOutgoingMessenger() {
            outgoingMessenger = null;
            xportDest = null;
            expiresAt = 0;
        }

        /**
         * Returns an incoming messenger is there is a working one available.
         *
         * @return an incoming messenger, null if there's none
         */
        private Messenger getIncoming() {
            if (incomingMessenger != null) {
                if ((incomingMessenger.getState() & Messenger.USABLE) != 0) {
                    return incomingMessenger;
                }
                
                // forget about this broken messenger.
                incomingMessenger = null;
            }
            return null;
        }

        /**
         * Returns an outgoingMessenger if there is one or one can be made without delay.
         * Renews a broken one if it can be. Refreshes expiration time if a messenger is returned.
         *
         * @return an outgoing messenger, null if there's none
         */
        private Messenger getOutgoing() {

            if (outgoingMessenger == null) {
                return null;
            }

            // (If messenger is not null, it means that we also have a xportDest).

            Messenger messenger = outgoingMessenger.get();

            // If it is gone or broken, try and get a new one.
            if ((messenger == null) || ((messenger.getState() & Messenger.USABLE) == 0)) {

                messenger = endpoint.getMessengerImmediate(xportDest, null);

                // If this fails, it is hopeless: the address is bad or something like that. Make ourselves expired right away.
                if (messenger == null) {
                    outgoingMessenger = null;
                    xportDest = null;
                    expiresAt = 0;
                    return null;
                }

                // Renew the ref. The xportDest is the same.
                outgoingMessenger = new SoftReference<Messenger>(messenger);
            }

            // So we had one or could renew. But, does it work ?
            if ((messenger.getState() & (Messenger.USABLE & Messenger.RESOLVED)) == 0) {
                // We no-longer have the underlying connection. Let ourselves expire. Do not renew the expiration time.
                outgoingMessenger = null;
                xportDest = null;
                return null;
            }

            // Ok, we do have an outgoing messenger at the ready after all.
            expiresAt = TimeUtils.toAbsoluteTimeMillis(EXPIRATION);
            return messenger;
        }

        /**
         * Returns a channel for this destination if one is there or can be obtained
         * readily and works.
         * <p/>
         * <p/>We prefer the incoming connection to the outgoing for two
         * reasons:
         * <ul>
         * <li>The remote peer was able to reach us. We cannot be sure that
         * we can reach the remote peer.</li>
         * <li>The remote peer initiated the connection. It has a better
         * sense of when the connection should be closed or reopened than
         * we do.</li>
         *
         * @return a channel for this destination
         */
        Messenger getCurrentMessenger() {
            Messenger res = getIncoming();

            if (res != null) {
                return res;
            }

            return getOutgoing();
        }

        /**
         * @return true if we do have an outgoing messenger or, failing that, we had one not too long ago.
         */
        boolean isNormallyReachable() {
            return ((getOutgoing() != null) || (TimeUtils.toRelativeTimeMillis(expiresAt) >= 0));
        }

        /**
         * We think the destination is reachable somehow. Not sure how long.
         *
         * @return true if we have any kind of messenger or, failing that, we had an outgoing one not too long ago.
         */
        boolean isCurrentlyReachable() {
            return ((getIncoming() != null) || (getOutgoing() != null) || (TimeUtils.toRelativeTimeMillis(expiresAt) >= 0));
        }

        /**
         * @return true if this wisdom carries no positive information whatsoever.
         */
        boolean isExpired() {
            return !isCurrentlyReachable();
        }
    }

    /*
     * Internal mechanisms
     */

    /**
     *  Return any Wisdom for the specified destination. The address will
     *  be normalized to the base form.
     *
     *  @param destination The address of the wisdom that is sought.
     *  @return The Wisdom for this address or {@code null} if no Wisdom found.
     */
    private Wisdom getWisdom(EndpointAddress destination) {
        if (destination.getServiceName() != null) {
            destination = new EndpointAddress(destination, null, null);
        }
        return wisdoms.get(destination);
    }

    /**
     *  Add a Wisdom for the specified destination. The address will
     *  be normalized to the base form.
     *
     *  @param destination The address of the Wisdom that is being added.
     *  @param wisdom The Wisdom for this address to be added to the map.
     */
    private void addWisdom(EndpointAddress destination, Wisdom wisdom) {
        destination = new EndpointAddress(destination, null, null);
        wisdoms.put(destination, wisdom);
    }

    /*
     * General house keeping.
     */

    public Destinations(EndpointService endpoint) {

        this.endpoint = endpoint;

        wisdomGC = new WisdomGCTask();

        cleanup.schedule(wisdomGC, TimeUtils.AMINUTE, TimeUtils.AMINUTE);
    }

    /**
     * Shutdown this cache. (stop the gc)
     */
    public synchronized void close() {
        stopped = true;

        // forget everything.
        wisdoms.clear();

        wisdomGC.cancel();
    }

    /**
     * Handles cleanup of expired wisdoms
     */
    class WisdomGCTask extends TimerTask {

        /**
         * {@inheritDoc}
         * 
         * <p/>garbage collector. We use soft references to messengers, but we use
         * a strong hashmap to keep the wisdom around in a more predictable
         * manner. Entries are simply removed when they no-longer carry
         * relevant information; so there's no change in the total meaning of
         * the map when an entry is removed.
         */
        @Override
        public void run() {
            try {
                synchronized (Destinations.this) {
                    Iterator<Wisdom> eachWisdom = wisdoms.values().iterator();

                    while (eachWisdom.hasNext()) {
                        Wisdom w = eachWisdom.next();

                        if (w.isExpired()) {
                            eachWisdom.remove();
                        }
                    }
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && Destinations.LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in TimerTask :" + Thread.currentThread().getName(), all);
                }
            }
        }
    }

    public synchronized Collection<EndpointAddress> allDestinations() {

        Set<EndpointAddress> allKeys = wisdoms.keySet();
        List<EndpointAddress> res = new ArrayList<EndpointAddress>(allKeys);

        return res;
    }

    /*
     * information output
     */

    /**
     * If there is a messenger at hand (incoming or otherwise), return it.
     *
     * @param destination The destination as an endpoint address (is automatically normalized to protocol and address only).
     * @return A messenger to that destination if a resolved and usable one is available or can be made instantly. null otherwise.
     */
    public synchronized Messenger getCurrentMessenger(EndpointAddress destination) {
        Wisdom wisdom = getWisdom(destination);

        if (wisdom == null) {
            return null;
        }
        return wisdom.getCurrentMessenger();
    }

    /**
     * Is it likely that one can be made from this end. (the last attempt succeeded, not only incoming, and that was not long ago) ?
     * This is a conservative test. It means that declaring that we can route to that destination is a very safe bet, as opposed
     * to isNormallyReachable and getCurrentMessenger, which could be misleading if the only messenger we can ever get is incoming.
     * Not currently used. Should likely be.
     *
     * @param destination The destination as an endpoint address (is automatically normalized to protocol and address only).
     * @return true if it is likely that we can get a messenger to that destination in the future.
     */
    public synchronized boolean isNormallyReachable(EndpointAddress destination) {
        Wisdom wisdom = getWisdom(destination);

        return ((wisdom != null) && wisdom.isNormallyReachable());
    }

    /**
     * Do we already have a messenger or is it likely that we can make one? 
     * We is will return {@code true} more often than 
     * {@code isNormallyReachable()} since it can be true even when all we have 
     * is an incoming messenger.
     * 
     * <p/>Just testing that there is an entry is no-longer the same because we
     * may keep the entries beyond the point where we would keep them before, so
     * that we can add some longer-lived information in the future, and do not 
     * interfere as much with the gc thread.
     *
     * @param destination The destination as an endpoint address (is automatically normalized to protocol and address only).
     * @return true is we are confident that we can obtain a messenger, either because we can get one instantly, or because
     *         this destination is normally reachable. (So, it is ok to try and route to that destination, now).
     */
    public synchronized boolean isCurrentlyReachable(EndpointAddress destination) {
        Wisdom wisdom = getWisdom(destination);

        return ((wisdom != null) && wisdom.isCurrentlyReachable());
    }

    /**
     * Are we supposed to send a welcome to that destination (we can't remember having done it).
     * It is assumed that once true was returned, it will be acted upon. So, true is not returned a second time.
     *
     * @param destination The destination as an endpoint address (is automatically normalized to protocol and address only).
     * @return true if this a destination to whish we can't remember sending a welcome message.
     */
    public synchronized boolean isWelcomeNeeded(EndpointAddress destination) {
        Wisdom wisdom = getWisdom(destination);

        return ((wisdom != null) && wisdom.isWelcomeNeeded());
    }

    /*
     * information input.
     */

    /**
     * Here is a messenger that we were able to obtain.
     *
     * @param destination The destination as an endpoint address (is automatically normalized to protocol and address only).
     * @param messenger   The incoming messenger for that destination.
     * @return true if this messenger was added (keep it open). false otherwise (do what you want with it).
     */
    public synchronized boolean addOutgoingMessenger(EndpointAddress destination, Messenger messenger) {
        Wisdom wisdom = getWisdom(destination);

        if (wisdom != null) {
            return wisdom.addOutgoingMessenger(messenger);
        }
        addWisdom(destination, new Wisdom(messenger, false));
        return true;
    }

    /**
     * Here is an incoming messenger that just popped out.
     *
     * @param destination The destination as an endpoint address (is automatically normalized to protocol and address only).
     * @param messenger   The incoming messenger for that destination.
     * @return true if this messenger was added (keep it open). false otherwise (do what you want with it).
     */
    public synchronized boolean addIncomingMessenger(EndpointAddress destination, Messenger messenger) {
        Wisdom wisdom = getWisdom(destination);

        if (wisdom != null) {
            return wisdom.addIncomingMessenger(messenger);
        }
        addWisdom(destination, new Wisdom(messenger, true));
        return true;
    }

    /**
     * We tried to get a messenger but could not. We know that we do not have connectivity from our end, for now.  we may still
     * have an incoming. However, if we had to try and make a messenger, there probably isn't an incoming, but that's not our
     * business here. isNormallyReachable becomes false; but we can still try when solicited.
     *
     * @param destination The destination as an endpoint address (is automatically normalized to protocol and address only).
     */
    public synchronized void noOutgoingMessenger(EndpointAddress destination) {
        Wisdom wisdom = getWisdom(destination);

        if (wisdom != null) {
            wisdom.noOutgoingMessenger();
        }
    }
}
