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
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Random;
import java.util.Set;
import java.util.SortedSet;
import java.util.Timer;
import java.util.TimerTask;
import java.util.TreeSet;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeID;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.protocol.RdvAdvertisement;
import net.jxta.protocol.RouteAdvertisement;
import net.jxta.rendezvous.RendezvousEvent;
import net.jxta.rendezvous.RendezvousListener;

import net.jxta.impl.endpoint.EndpointUtils;
import net.jxta.impl.endpoint.relay.RelayReferralSeedingManager;
import net.jxta.impl.protocol.RdvConfigAdv;
import net.jxta.impl.rendezvous.RendezVousServiceImpl;
import net.jxta.impl.util.SeedingManager;
import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.util.URISeedingManager;

/**
 * This class models a Rendezvous Peer View (RPV):
 * ordered collection of all other Rendezvous Peers visible to
 * this peer.
 * <p/>
 * Presently this class implements a random "diffusion" algorithm
 * where each Peer periodically selects a randomly selected peer advertisement
 * from its view and sends it over to a randomly selected peer from its view.
 * Over time, this causes every peer to learn about every other peer, resulting
 * in a "converged" peer view.
 * <p/>
 * This diffusion process is bootstrapped by every peer sending their
 * own peer advertisements to some well-known, stable, "seed" peers on
 * startup.
 */
public final class PeerView implements EndpointListener, RendezvousListener {

    /**
     * Logger
     */
    private static final transient Logger LOG = Logger.getLogger(PeerView.class.getName());

    /**
     * Our service name
     */
    static final String SERVICE_NAME = "PeerView";

    /**
     * Namespace used for rdv message elements.
     */
    static final String MESSAGE_NAMESPACE = "jxta";

    /**
     * Element name of outgoing messages. Note that the element contains a
     * RdvAvertisement and <emphasis>not</emphasis> a Peer Advertisement.
     */
    static final String MESSAGE_ELEMENT_NAME = "PeerView.PeerAdv";

    /**
     * Element name of responses. Note that the element contains a
     * RdvAvertisement and <emphasis>not</emphasis> a Peer Advertisement.
     */
    static final String RESPONSE_ELEMENT_NAME = "PeerView.PeerAdv.Response";

    /**
     * Message element name for PeerView "Cached" Message Element
     */
    static final String CACHED_RADV_ELEMENT_NAME = "PeerView.Cached";

    /**
     * Optional message element that specifies by its presence in a peerview
     * message that the referenced peer is not the provider of the
     * RdvAdvertisement and the advertisement is a "hint" or referral from the
     * responding peer.
     * <p/>
     * In practice, when sending its own RdvAdvertisement, a peer does not
     * include this element, but when sending another peer's RdvAdvertisement,
     * this element is included.
     */
    static final MessageElement CACHED_RADV_ELEMENT = new StringMessageElement(CACHED_RADV_ELEMENT_NAME, Boolean.TRUE.toString(), null);

    /**
     * Message element name that specifies the route advertisement of the
     * source of the message.
     */
    static final String SRCROUTEADV_ELEMENT_NAME = "PeerView.SrcRouteAdv";

    /**
     * Message element name for PeerView "Edge" Message Element
     */
    static final String EDGE_ELEMENT_NAME = "PeerView.EdgePeer";

    /**
     * Optional message element that specifies by its presence in a peerview
     * message that the referenced peer is an edge peer and not a member of the
     * peerview.
     */
    static final MessageElement EDGE_ELEMENT = new StringMessageElement(EDGE_ELEMENT_NAME, Boolean.TRUE.toString(), null);

    /**
     * Message element name for PeerView "Failure" Message Element
     */
    static final String FAILURE_ELEMENT_NAME = "PeerView.Failure";

    /**
     * Optional message element that specifies by its presence in a peerview
     * message that the referenced peer has either failed or is quitting. If the
     * "cached" element is also set then the error is being reported by a third
     * party.
     */
    static final MessageElement FAILURE_ELEMENT = new StringMessageElement(FAILURE_ELEMENT_NAME, Boolean.TRUE.toString(), null);

    /**
     * This is the interval between adv exchange in seconds. This is
     * the main tunable runtime parameter for the diffusion
     * process. An interval that is too low will improve view
     * consistency at the expense of gratuitous network traffic. On
     * the other hand, an interval that is too high will cause the
     * view to become inconsistent. It is desirable to err on the side
     * of extra traffic.
     */
    private static final long DEFAULT_SEEDING_PERIOD = 5 * TimeUtils.ASECOND;

    private static final long WATCHDOG_PERIOD = 30 * TimeUtils.ASECOND;
    private static final long WATCHDOG_GRACE_DELAY = 5 * TimeUtils.AMINUTE;

    private static final long DEFAULT_BOOTSTRAP_KICK_INTERVAL = 3 * TimeUtils.ASECOND;

    private static final int MIN_BOOTLEVEL = 0;
    private static final int BOOTLEVEL_INCREMENT = 1;
    private static final int MAX_BOOTLEVEL = 6;

    /**
     * DEFAULT_SEEDING_RDVPEERS
     * <p/>
     * This value is the maximum number of rendezvous peers that will be
     * send our own advertisement at boot time.
     */
    //private static final int DEFAULT_SEEDING_RDVPEERS = 5;

    private final PeerGroup group;
    
    /**
     *  The group in which our propagate pipe will run.
     */
    private final PeerGroup advertisingGroup;
    private final RendezVousServiceImpl rdvService;
    private final EndpointService endpoint;

    /**
     * The name of this PeerView.
     * <p/>
     * FIXME 20040623 bondolo This should be a CodatID.
     */
    private final String name;

    /**
     * Delay in relative milliseconds to apply before contacting seeding rdvs.
     * 0 is supposed to be reserved by RdvConfig to mean "use the default".
     * However, it is in fact a valid value and also the one we want for the default.
     * The only problem with that is that it is not possible to configure this value
     * explicitly, should it one day not be the default. The issue is actually in RdvConfig.
     */
    private long seedingRdvConnDelay = 0;

    private final boolean useOnlySeeds;

    private final SeedingManager seedingManager;

    /**
     * If the peerview is smaller than this we will try harder to find
     * additional peerview members.
     */
    private int minHappyPeerView = 4;

    /**
     * A single timer is used to periodically kick each PeerView
     * into activity. For the Random PeerView, this activity consists
     * of selecting a PeerViewElement at random from its view and
     * sending it across to a randomly-selected peer from its view.
     * <p/>
     * FIXME 20021121 lomax
     * <p/>
     * The original idea of using a common timer in order to save threads IS a
     * very good idea. However, limitations, and actually, bugs, in java.util.Timer
     * creates the following problems when using a static Timer:
     * <p/>
     * <ul>
     * <li>Memory Leak: Canceling a TimerTask does not remove it from the
     * execution queue of the Timer until the Timer is canceled or the
     * TimerTask is fired. Since most of the TimerTasks are inner classes
     * this can mean that the PeerView is held around for a long time.</li>
     * <p/>
     * <li>java.util.Timer is not only not real-time (which is more or less fine
     * for the PeerView, but it sequentially invokes tasks (only one Thread
     * per Timer). As a result, tasks that takes a long time to run delays
     * other tasks.</li>
     * </ul>
     * <p/>
     * The PeerView would function better with a better Timer, but JDK does
     * not provide a standard timer that would fulfill the needs of the
     * PeerView. Maybe we should implement a JXTA Timer, since lots of the JXTA
     * services, by being very asynchronous, rely on the same kind of timer
     * semantics as the PeerView. Until then, creating a Timer per instance of
     * the PeerView (i.e. per instance of joined PeerGroup) is the best
     * workaround.
     */
    private final Timer timer;

    /**
     * A random number generator.
     */
    private final static Random random = new Random();

    /**
     * List of scheduled tasks
     */
    private final Set<TimerTask> scheduledTasks = Collections.synchronizedSet(new HashSet<TimerTask>());

    /**
     * Describes the frequency and amount of effort we will spend updating
     * the peerview.
     */
    private int bootLevel = MIN_BOOTLEVEL;

    /**
     * Earliest absolute time in milliseconds at which we will allow a reseed
     * to take place.
     */
    private long earliestReseed = 0L;

    private final String uniqueGroupId;

    /**
     * Listeners for PeerView Events.
     */
    private final Set<PeerViewListener> rpvListeners = Collections.synchronizedSet(new HashSet<PeerViewListener>());

    /**
     * Used for querying for pves.
     */
    private InputPipe wirePipeInputPipe = null;

    /**
     * Used for querying for pves.
     */
    private OutputPipe wirePipeOutputPipe = null;

    /**
     * Used for notifications about pve failures.
     */
    private InputPipe localGroupWirePipeInputPipe = null;

    /**
     * Used for notifications about pve failures.
     */
    private OutputPipe localGroupWirePipeOutputPipe = null;

    /**
     * A task which monitors the up and down peers in the peerview.
     */
    private WatchdogTask watchdogTask = null;

    /**
     * This is the accumulated view by an instance of this class.
     */
    private final SortedSet<PeerViewDestination> localView = Collections.synchronizedSortedSet(new TreeSet<PeerViewDestination>());

    /**
     * PVE for ourself.
     * <p/>
     * FIXME bondolo 20041015 This should be part of the local view.
     */
    private final PeerViewElement self;
    private PeerViewElement upPeer = null;
    private PeerViewElement downPeer = null;

    private final PeerViewStrategy replyStrategy;

    private final PeerViewStrategy kickRecipientStrategy;

    private final PeerViewStrategy kickAdvertisementStrategy;

    private final PeerViewStrategy refreshRecipientStrategy;

    // PeerAdv tracking.
    private PeerAdvertisement lastPeerAdv = null;
    private int lastModCount = -1;

    private final PipeAdvertisement localGroupWirePipeAdv;
    private final PipeAdvertisement advGroupPropPipeAdv;

    /**
     * If <code>true</code> then this Peer View instance is closed and is
     * shutting down.
     */
    private volatile boolean closed = false;

    /**
     * Get an instance of PeerView for the specified PeerGroup and Service.
     *
     * @param group            Peer Group in which this Peer View instance operates.
     * @param advertisingGroup Peer Group in which this Peer View instance will
     *                         advertise and broadcast its existence.
     * @param rdvService       The rdvService we are to use.
     * @param name             The identifying name for this Peer View instance.
     */
    public PeerView(PeerGroup group, PeerGroup advertisingGroup, RendezVousServiceImpl rdvService, String name) {
        this.group = group;
        this.advertisingGroup = advertisingGroup;
        this.rdvService = rdvService;
        this.name = name;

        this.endpoint = group.getEndpointService();

        this.uniqueGroupId = group.getPeerGroupID().getUniqueValue().toString();

        timer = new Timer("PeerView Timer for " + group.getPeerGroupID(), true);

        Advertisement adv = null;
        ConfigParams confAdv = group.getConfigAdvertisement();

        // Get the config. If we do not have a config, we're done; we just keep
        // the defaults (edge peer/no auto-rdv)
        if (confAdv != null) {
            try {
                XMLDocument configDoc = (XMLDocument) confAdv.getServiceParam(rdvService.getAssignedID());

                if (null != configDoc) {
                    adv = AdvertisementFactory.newAdvertisement(configDoc);
                }
            } catch (java.util.NoSuchElementException failed) {// ignored
            }
        }

        RdvConfigAdv rdvConfigAdv;

        if (!(adv instanceof RdvConfigAdv)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Creating new RdvConfigAdv for defaults.");
            }

            rdvConfigAdv = (RdvConfigAdv) AdvertisementFactory.newAdvertisement(RdvConfigAdv.getAdvertisementType());
        } else {
            rdvConfigAdv = (RdvConfigAdv) adv;
        }

        if (rdvConfigAdv.getSeedRendezvousConnectDelay() > 0) {
            seedingRdvConnDelay = rdvConfigAdv.getSeedRendezvousConnectDelay();
        }

        useOnlySeeds = rdvConfigAdv.getUseOnlySeeds();

        if (rdvConfigAdv.getMinHappyPeerView() > 0) {
            minHappyPeerView = rdvConfigAdv.getMinHappyPeerView();
        }

        URISeedingManager seedingManager;

        if ((null == advertisingGroup) && rdvConfigAdv.getProbeRelays()) {
            seedingManager = new RelayReferralSeedingManager(rdvConfigAdv.getAclUri(), useOnlySeeds, group, name);
        } else {
            seedingManager = new URISeedingManager(rdvConfigAdv.getAclUri(), useOnlySeeds, group, name);
        }

        for (URI aSeeder : Arrays.asList(rdvConfigAdv.getSeedingURIs())) {
            seedingManager.addSeedingURI(aSeeder);
        }

        for (URI aSeed : Arrays.asList(rdvConfigAdv.getSeedRendezvous())) {
            seedingManager.addSeed(aSeed);
        }

        this.seedingManager = seedingManager;

        lastPeerAdv = group.getPeerAdvertisement();
        lastModCount = lastPeerAdv.getModCount();

        // create a new local RdvAdvertisement and set it to self.
        RdvAdvertisement radv = createRdvAdvertisement(lastPeerAdv, name);

        self = new PeerViewElement(endpoint, radv);

        // addPeerViewElement( self );

        // setup endpoint listener
        endpoint.addIncomingMessageListener(this, SERVICE_NAME, uniqueGroupId);

        // add rendezvous listener
        rdvService.addListener(this);

        // initialize strategies
        replyStrategy = new PeerViewRandomWithReplaceStrategy(localView);

        kickRecipientStrategy = new PeerViewRandomStrategy(localView);

        kickAdvertisementStrategy = new PeerViewRandomWithReplaceStrategy(localView);

        refreshRecipientStrategy = new PeerViewSequentialStrategy(localView);

        localGroupWirePipeAdv = makeWirePipeAdvertisement(group, group, name);

        if (null != advertisingGroup) {
            advGroupPropPipeAdv = makeWirePipeAdvertisement(advertisingGroup, group, name);
        } else {
            advGroupPropPipeAdv = null;
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info( "PeerView created for group \"" + group.getPeerGroupName() +
                    "\" [" + group.getPeerGroupID() + "] name \"" + name + "\"");
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Listener for "PeerView"/&lt;peergroup-unique-id> and propagate pipes.
     */
    public void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        // check what kind of message this is (response or not).
        boolean isResponse = false;
        MessageElement me = msg.getMessageElement(MESSAGE_NAMESPACE, MESSAGE_ELEMENT_NAME);

        if (me == null) {
            me = msg.getMessageElement(MESSAGE_NAMESPACE, RESPONSE_ELEMENT_NAME);
            if (me == null) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Discarding damaged " + msg + ".");
                }
                return;
            } else {
                isResponse = true;
            }
        }

        Advertisement adv;

        try {
            XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(me);

            adv = AdvertisementFactory.newAdvertisement(asDoc);
        } catch (RuntimeException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed building rdv advertisement from message element", failed);
            }
            return;
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed building rdv advertisement from message element", failed);
            }
            return;
        }

        if (!(adv instanceof RdvAdvertisement)) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Response does not contain radv (" + adv.getAdvertisementType() + ")");
            }
            return;
        }

        RdvAdvertisement radv = (RdvAdvertisement) adv;

        if (null == radv.getRouteAdv()) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Rdv Advertisement does not contain route.");
            }
            return;
        }

        // See if we can find a src route adv in the message.
        me = msg.getMessageElement(MESSAGE_NAMESPACE, SRCROUTEADV_ELEMENT_NAME);
        if (me != null) {
            try {
                XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(me);
                Advertisement routeAdv = AdvertisementFactory.newAdvertisement(asDoc);

                if (!(routeAdv instanceof RouteAdvertisement)) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Advertisement is not a RouteAdvertisement");
                    }
                } else {
                    RouteAdvertisement rdvRouteAdv = radv.getRouteAdv().clone();

                    // XXX we stich them together even if in the end it gets optimized away
                    RouteAdvertisement.stichRoute(rdvRouteAdv, (RouteAdvertisement) routeAdv);
                    radv.setRouteAdv(rdvRouteAdv);
                }
            } catch (RuntimeException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed building route adv from message element", failed);
                }
            } catch (IOException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed building route adv from message element", failed);
                }
            }
        }
        me = null;

        // Is this a message about ourself?
        if (group.getPeerID().equals(radv.getPeerID())) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Received a PeerView message about self. Discard.");
            }

            return;
        }

        // Collect the various flags.

        boolean isFailure = (msg.getMessageElement(MESSAGE_NAMESPACE, FAILURE_ELEMENT_NAME) != null);
        boolean isCached = (msg.getMessageElement(MESSAGE_NAMESPACE, CACHED_RADV_ELEMENT_NAME) != null);
        boolean isFromEdge = (msg.getMessageElement(MESSAGE_NAMESPACE, EDGE_ELEMENT_NAME) != null);
        boolean isTrusted = isFromEdge || seedingManager.isAcceptablePeer(radv.getRouteAdv());

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            String srcPeer = srcAddr.toString();

            if ("jxta".equals(srcAddr.getProtocolName())) {
                try {
                    String idstr = ID.URIEncodingName + ":" + ID.URNNamespace + ":" + srcAddr.getProtocolAddress();

                    ID asID = IDFactory.fromURI(new URI(idstr));

                    PeerViewElement pve = getPeerViewElement(asID);

                    if (null != pve) {
                        srcPeer = "\"" + pve.getRdvAdvertisement().getName() + "\"";
                    }
                } catch (URISyntaxException failed) {// ignored
                }
            }

            LOG.fine(
                    "[" + group.getPeerGroupID() + "] Received a" + (isCached ? " cached" : "") + (isResponse ? " response" : "")
                    + (isFailure ? " failure" : "") + " message (" + msg.toString() + ")" + (isFromEdge ? " from edge" : "")
                    + " regarding \"" + radv.getName() + "\" from " + srcPeer);
        }

        if (!isTrusted) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Rejecting peerview message from " + radv.getPeerID());
            }
            return;
        }

        // if this is a notification failure. All we have to do is locally
        // process the failure
        if (isFailure) {
            notifyFailure(radv.getPeerID(), false);
            return;
        }

        handlePeerViewMessage(isResponse, isCached, isFromEdge, isTrusted, radv);
    }

    /**
     * Following the extraction of a peerview message from a
     */
    private void handlePeerViewMessage(boolean isResponse, boolean isCached, boolean isFromEdge, boolean isTrusted, RdvAdvertisement radv) {

        // Figure out if we know that peer already. If we do, reuse the pve
        // that we have.
        boolean isNewbie = false;
        boolean added = false;
        PeerViewElement pve;

        synchronized (localView) {
            PeerViewElement newbie = new PeerViewElement(endpoint, radv);

            pve = getPeerViewElement(newbie);

            if (null == pve) {
                pve = newbie;
                isNewbie = true;
            }

            if (!isFromEdge && !isCached && isTrusted) {
                if (isNewbie) {
                    added = addPeerViewElement(pve);
                } else {
                    pve.setRdvAdvertisement(radv);
                }
            }
        }

        if (!isNewbie && isFromEdge && !isCached) {
            // The message stated that it is from an edge we believed was a 
            // peerview member. Best thing to do is tell everyone that it's no 
            // longer in peerview.
            notifyFailure(pve, true);
            // we continue processing because it's not the other peer's fault we had the wrong idea.
        }

        // Do the rest of the add related tasks out of synch.
        // We must not nest any possibly synchronized ops in
        // the LocalView lock; it's the lowest level.

        if (added) {
            // Notify local listeners
            generateEvent(PeerViewEvent.ADD, pve);
        }

        /*
         * Now, see what if any message we have to send as a result.
         * There are three kinds of messages we can send:
         *
         * - A response with ourselves, if we're being probed and we're
         * a rdv.
         *
         * - A probe to the peer whose adv we received, because we want
         * confirmation that it's alive.
         *
         * - A response with a random adv from our cache if we're being probed
         *
         * We may send more than one message.
         */

        boolean status;

        if (!isCached) {
            if (!isResponse) {
                // Type 1: Respond to probe
                //
                // We are being probed by an edge peer or peerview member. We respond
                // with our own advertisement.
                status = send(pve, self, true, false);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Type 1 (Respond with self PVE) : Sent to " + pve + " result =" + status);
                }

                // Type 3: Respond with random entry from our PV when we are probed.
                //
                // Respond with a strategized adv from our view.
                PeerViewElement sendpve = replyStrategy.next();

                if ((sendpve != null) && !pve.equals(sendpve) && !self.equals(sendpve)) {
                    status = send(pve, sendpve, true, false);
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Type 3 (Respond with random PVE) : Sent " + sendpve + " to " + pve + " result=" + status);
                    }
                }
            } else {
                // Heartbeat: do nothing.
            }
        } else if (isResponse) {
            if (isNewbie && !useOnlySeeds && !isFromEdge) {
                // Type 2: Probe a peer we have just learned about from a referral.
                //
                // If useOnlySeeds, we're not allowed to talk to peers other than our 
                // seeds, so do not probe anything we learn from 3rd party. (Probing of
                // seeds happens as part of the "kick" strategy).
                status = send(pve, self, false, false);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Type 2 (Probe PVE) : Probed " + pve + " result=" + status);
                }
            } else {
                // Already known or ignoring: do nothing.
            }
        } else {
            // Invalid : do nothing.
        }
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("fallsthrough")
    public void rendezvousEvent(RendezvousEvent event) {

        if (closed) {
            return;
        }

        boolean notifyFailure = false;

        synchronized (this) {

            int theEventType = event.getType();

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("[" + group.getPeerGroupName() + "] Processing  " + event);
            }

            refreshSelf();

            if ((RendezvousEvent.BECAMERDV == theEventType) || (RendezvousEvent.BECAMEEDGE == theEventType)) {
                // kill any existing watchdog task
                if (null != watchdogTask) {
                    removeTask(watchdogTask);
                    watchdogTask.cancel();
                    watchdogTask = null;
                }
            }

            switch (theEventType) {
            case RendezvousEvent.RDVCONNECT:
            case RendezvousEvent.RDVRECONNECT:
            case RendezvousEvent.CLIENTCONNECT:
            case RendezvousEvent.CLIENTRECONNECT:
            case RendezvousEvent.RDVFAILED:
            case RendezvousEvent.RDVDISCONNECT:
            case RendezvousEvent.CLIENTFAILED:
            case RendezvousEvent.CLIENTDISCONNECT:
                break;

            case RendezvousEvent.BECAMERDV:
                openWirePipes();
                watchdogTask = new WatchdogTask();
                addTask(watchdogTask, WATCHDOG_PERIOD, WATCHDOG_PERIOD);
                rescheduleKick(true);
                break;

            case RendezvousEvent.BECAMEEDGE:
                openWirePipes();
                if (!localView.isEmpty()) {
                    // FIXME bondolo 20040229 since we likely don't have a
                    // rendezvous connection, it is kind of silly to be sending
                    // this now. probably should wait until we get a rendezvous
                    // connection.
                    notifyFailure = true;
                }
                rescheduleKick(true);
                break;

            default:
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("[" + group.getPeerGroupName() + "] Unexpected RDV event : " + event);
                }
                break;
            }
        }

        // we can't do the notification under synchronization.
        if (notifyFailure) {
            notifyFailure(self, true);
        }
    }

    public void start() {// do nothing for now... all the good stuff happens as a result of
        // rendezvous events.
    }

    public void stop() {

        synchronized (this) {
            // Only one thread gets to perform the shutdown.
            if (closed) {
                return;
            }
            closed = true;
        }

        // notify other rendezvous peers that we are going down
        notifyFailure(self, true);

        // From now on we can nullify everything we want. Other threads check
        // the closed flag where it matters.
        synchronized (this) {
            if (watchdogTask != null) {
                removeTask(watchdogTask);
                watchdogTask.cancel();
                watchdogTask = null;
            }

            // Remove message listener.
            endpoint.removeIncomingMessageListener(SERVICE_NAME, uniqueGroupId);

            // Remove rendezvous listener.
            rdvService.removeListener(this);

            // Remove all our pending scheduled tasks
            // Carefull with the indices while removing: do it backwards, it's
            // cheaper and simpler.

            synchronized (scheduledTasks) {
                Iterator<TimerTask> eachTask = scheduledTasks.iterator();

                while (eachTask.hasNext()) {
                    try {
                        TimerTask task = eachTask.next();

                        task.cancel();
                        eachTask.remove();
                    } catch (Exception ez1) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Cannot cancel task: ", ez1);
                        }
                    }
                }
            }

            // Make sure that we close our WirePipes
            closeWirePipes();

            // Let go of the up and down peers.
            downPeer = null;
            upPeer = null;
            localView.clear();

            timer.cancel();

            rpvListeners.clear();
        }
    }

    protected void addTask(TimerTask task, long delay, long interval) {

        synchronized (scheduledTasks) {
            if (scheduledTasks.contains(task)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Task list already contains specified task.");
                }
            }
            scheduledTasks.add(task);
        }

        if (interval >= 1) {
            timer.schedule(task, delay, interval);
        } else {
            timer.schedule(task, delay);
        }
    }

    protected void removeTask(TimerTask task) {
        scheduledTasks.remove(task);
    }

    /**
     * Adds the specified URI to the list of seeds. Even if useOnlySeeds is in
     * effect, this seed may now be used, as if it was part of the initial
     * configuration.
     *
     * @param seed the URI of the seed rendezvous.
     */
    public void addSeed(URI seed) {
        if (seedingManager instanceof URISeedingManager) {
            ((URISeedingManager) seedingManager).addSeed(seed);
        }
    }

    /**
     * Probe the specified peer immediately.
     * <p/>
     * Note: If "useOnlySeeds" is in effect and the peer is not a seed, any response to this probe will be ignored.
     */
    public boolean probeAddress(EndpointAddress address, RouteAdvertisement hint) {

        PeerViewElement holdIt;

        synchronized (localView) {
            holdIt = self;
        }

        return send(address, hint, holdIt, false, false);
    }

    /**
     * Send our own advertisement to all of the seed rendezvous.
     */
    public void seed() {
        long reseedRemaining = earliestReseed - TimeUtils.timeNow();

        if (reseedRemaining > 0) {
            // Too early; the previous round is not even done.
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Still Seeding for " + reseedRemaining + "ms.");
            }
            return;
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("New Seeding...");
        }

        // Schedule sending propagated query to our local network neighbors.
        send(null, null, self, false, false);

        long iterations = 0;

        if (localView.size() < minHappyPeerView) {
            // We only do these things if we don't have a "happy" Peer View.
            // If the Peer View is already "happy" then we will use only
            // Peer View referrals for learning of new entires.

            List<RouteAdvertisement> seedRdvs = new ArrayList<RouteAdvertisement>(
                    Arrays.asList(seedingManager.getActiveSeedRoutes()));

            while (!seedRdvs.isEmpty()) {
                RouteAdvertisement aSeed = seedRdvs.remove(0);

                if (null == aSeed.getDestPeerID()) {
                    // It is an incomplete route advertisement. We are going to assume that it is only a wrapper for a single ea.
                    Vector<String> seed_eas = aSeed.getDest().getVectorEndpointAddresses();

                    if (!seed_eas.isEmpty()) {
                        EndpointAddress aSeedHost = new EndpointAddress(seed_eas.get(0));

                        // XXX 20061220 bondolo We could check all of our current PVEs to make sure that this address is not already known.

                        send(aSeedHost, null, self, false, false);
                    }
                } else {
                    // We have a full route, send it to the virtual address of the route!
                    // FIXME malveaux 20070816 Second part of conjunct can be removed once 'self' is included in the peerview
                    if ((null == getPeerViewElement(aSeed.getDestPeerID())) && !group.getPeerID().equals(aSeed.getDestPeerID())) {
                        EndpointAddress aSeedHost = new EndpointAddress("jxta", aSeed.getDestPeerID().getUniqueValue().toString(),
                                null, null);

                        send(aSeedHost, aSeed, self, false, false);
                    }
                }
            }

            if (!useOnlySeeds) {
                // If use only seeds, we're not allowed to put in the peerview
                // anything but our seed rdvs. So, we've done everything that
                // was required.

                // Schedule sending propagated query to our advertising group
                if (advertisingGroup != null) {
                    // send it, but not immediately.
                    scheduleAdvertisingGroupQuery(DEFAULT_SEEDING_PERIOD * 2);
                }
            }
        }

        earliestReseed = TimeUtils.toAbsoluteTimeMillis(seedingRdvConnDelay + (DEFAULT_SEEDING_PERIOD * iterations));
    }

    /**
     * Make sure that the PeerView properly changes behavior, when switching
     * from edge mode to rdv mode, and vice-versa.
     * Since openWirePipes() requires some other services such as the Pipe
     * Service, and since updateStatus is invoked this work must happen in
     * background, giving a chance to other services to be started.
     */
    private class OpenPipesTask extends TimerTask {

        /**
         * {@inheritDoc}
         */
        @Override
        public void run() {
            try {
                if (closed) {
                    return;
                }

                openWirePipes();
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread: " + Thread.currentThread().getName(), all);
                }
            } finally {
                removeTask(this);
            }
        }
    }

    private void scheduleOpenPipes(long delay) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Scheduling open pipes attempt in " + delay + "ms.");
        }

        addTask(new OpenPipesTask(), delay, -1);
    }

    /**
     * Send a PeerView Message to the specified peer.
     *
     * @param response indicates whether this is a response. Otherwise
     *                 we may create a distributed loop where peers keep perpetually
     *                 responding to each-other.
     * @param failure  Construct the message as a failure notification.
     */
    private boolean send(PeerViewElement dest, PeerViewElement pve, boolean response, boolean failure) {

        Message msg = makeMessage(pve, response, failure);

        boolean result = dest.sendMessage(msg, SERVICE_NAME, uniqueGroupId);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending " + msg + " to " + dest + " success = " + result);
        }

        return result;
    }

    /**
     * Send a PeerView Message to the specified peer.
     *
     * @param response indicates whether this is a response. Otherwise
     *                 we may create a distributed loop where peers keep perpetually
     *                 responding to each-other.
     * @param failure  Construct the message as a failure notification.
     */
    private boolean send(EndpointAddress dest, RouteAdvertisement hint, PeerViewElement pve, boolean response, boolean failure) {

        Message msg = makeMessage(pve, response, failure);

        if (null != dest) {
            EndpointAddress realAddr = new EndpointAddress(dest, SERVICE_NAME, uniqueGroupId);

            Messenger messenger = rdvService.endpoint.getMessengerImmediate(realAddr, hint);

            if (null != messenger) {
                try {
                    boolean result = messenger.sendMessage(msg);

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Sending " + msg + " to " + dest + " success = " + result);
                    }

                    return result;
                } catch (IOException failed) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Could not send " + msg + " to " + dest, failed);
                    }
                    return false;
                }
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Could not get messenger for " + dest);
                }

                return false;
            }
        } else {
            // Else, propagate the message.
            try {
                endpoint.propagate(msg, SERVICE_NAME, uniqueGroupId);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Sent " + msg + " via propagate");
                }
                return true;
            } catch (IOException ez) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    // Pretty strange. This has little basis for failure...
                    LOG.log(Level.WARNING, "Could not propagate " + msg, ez);
                }
                return false;
            }
        }
    }

    /**
     * Send a PeerView Message to the specified peer.
     *
     * @param response indicates whether this is a response. Otherwise
     *                 we may create a distributed loop where peers keep perpetually
     *                 responding to each-other.
     * @param failure  Construct the message as a failure notification.
     * @param dest destination output pipe
     * @param pve the peer view element
     * @return  true if successful
     */
    private boolean send(OutputPipe dest, PeerViewElement pve, boolean response, boolean failure) {

        Message msg = makeMessage(pve, response, failure);

        try {
            return dest.send(msg);
        } catch (IOException ez) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not send " + msg, ez);
            }
            return false;
        }
    }

    /**
     * Make a PeerView Message
     *
     * @param content the peer view element
     * @param response the response
     * @param failure whether to create a message based on a failure
     * @return the message
     */
    private Message makeMessage(PeerViewElement content, boolean response, boolean failure) {

        Message msg = new Message();

        // // edge peers add an identifying element, RDV peers do not
        // if (!rdvService.isRendezVous()) {
        // msg.addMessageElement(MESSAGE_NAMESPACE, EDGE_ELEMENT);
        // }
        //
        if (failure) {
            // This is a failure notification.
            msg.addMessageElement(MESSAGE_NAMESPACE, FAILURE_ELEMENT);
        }

        refreshSelf();

        RdvAdvertisement radv = content.getRdvAdvertisement();

        XMLDocument doc = (XMLDocument) radv.getDocument(MimeMediaType.XMLUTF8);
        String msgName = response ? RESPONSE_ELEMENT_NAME : MESSAGE_ELEMENT_NAME;

        MessageElement msge = new TextDocumentMessageElement(msgName, doc, null);

        msg.addMessageElement(MESSAGE_NAMESPACE, msge);

        if (!content.equals(self)) {
            // This is a cached RdvAdvertisement
            msg.addMessageElement(MESSAGE_NAMESPACE, CACHED_RADV_ELEMENT);

            // This message contains an RdvAdvertisement which is not ourself. In that
            // case, it is wise to also send the local route advertisement (as the optional
            // SrcRdvAdv) so the destination might have a better change to access the "content"
            // RendezvousAdv (this peer will then act as a hop).

            RouteAdvertisement localra = EndpointUtils.extractRouteAdv(lastPeerAdv);

            if (localra != null) {
                try {
                    XMLDocument radoc = (XMLDocument) localra.getDocument(MimeMediaType.XMLUTF8);

                    msge = new TextDocumentMessageElement(SRCROUTEADV_ELEMENT_NAME, radoc, null);
                    msg.addMessageElement(MESSAGE_NAMESPACE, msge);
                } catch (Exception ez1) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Could not create optional src route adv for " + content, ez1);
                    }
                }
            }
        }

        return msg;
    }

    /**
     * Invoked by anyone in order to inform the PeerView of a failure
     * of one of the member peers.
     *
     * @param pid              ID of the peer which failed.
     * @param propagateFailure If <tt>true</tt>then broadcast the failure to
     *                         other peers otherwise only update the local peerview.
     */
    public void notifyFailure(PeerID pid, boolean propagateFailure) {

        PeerViewElement pve = getPeerViewElement(pid);

        if (null != pve) {
            notifyFailure(pve, propagateFailure);
        }
    }

    /**
     * Invoked when a peerview member peer becomes unreachable.
     *
     * @param pve              The peer which failed.
     * @param propagateFailure If {@code true} then broadcast the failure to
     *                         other peers otherwise only update the local peerview.
     */
    void notifyFailure(PeerViewElement pve, boolean propagateFailure) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Notifying failure of " + pve);
        }

        try {
            boolean removedFromPeerView = removePeerViewElement(pve);

            // only propagate if we actually knew of the peer
            propagateFailure &= (removedFromPeerView || (self == pve));

            // Notify local listeners
            if (removedFromPeerView) {
                generateEvent(PeerViewEvent.FAIL, pve);
            }

            boolean emptyPeerView = localView.isEmpty();

            // If the local view has become empty, reset the kicker into
            // a seeding mode.
            if (emptyPeerView && removedFromPeerView) {
                rescheduleKick(true);
            }

            if (propagateFailure) {
                // Notify other rendezvous peers that there has been a failure.
                OutputPipe op = localGroupWirePipeOutputPipe;

                if (null != op) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Propagating failure of " + pve);
                    }

                    send(op, pve, true, true);
                }
            }
        } catch (Exception ez) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure while generating noficiation of failure of PeerView : " + pve, ez);
            }
        }
    }

    /**
     * Invoked by the Timer thread to cause each PeerView to initiate
     * a Peer Advertisement exchange.
     */
    private void kick() {

        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Begun kick() in " + group.getPeerGroupID());
            }

            // Use seed strategy. (it has its own throttling and resource limiting).
            seed();

            // refresh ourself to a peer in our view
            PeerViewElement refreshee = refreshRecipientStrategy.next();

            if ((refreshee != null) && (self != refreshee)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Refresh " + refreshee);
                }
                send(refreshee, self, false, false);
            }

            // now share an adv from our local view to another peer from our
            // local view.

            PeerViewElement recipient = kickRecipientStrategy.next();

            if (recipient == null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("No recipient to send adv ");
                }
                return;
            }

            PeerViewElement rpve = kickAdvertisementStrategy.next();

            if (rpve == null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("No adv to send");
                }
                return;
            }

            if (rpve.equals(recipient) || self.equals(recipient)) {
                // give up: no point in sending a peer its own adv
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("adv to send is same as recipient: Nothing to do.");
                }
                return;
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sending adv " + rpve + " to " + recipient);
            }

            send(recipient, rpve, true, false);
        } finally {
            rescheduleKick(false);
        }
    }

    /**
     * Choose a boot level appropriate for the current configuration and state.
     *
     * @return the new boot level.
     */
    private int adjustBootLevel() {

        boolean areWeHappy = localView.size() >= minHappyPeerView;

        // increment boot level faster if we have a reasonable peerview.
        int increment = areWeHappy ? BOOTLEVEL_INCREMENT : BOOTLEVEL_INCREMENT * 2;

        // if we don't have a reasonable peerview, we continue to try harder.
        int maxbootlevel = MAX_BOOTLEVEL - (areWeHappy ? 0 : BOOTLEVEL_INCREMENT);

        bootLevel = Math.min(maxbootlevel, bootLevel + increment);

        return bootLevel;
    }

    private synchronized void rescheduleKick(boolean now) {

        if (closed) {
            return;
        }

        // Set the next iteration
        try {
            if (now) {
                bootLevel = MIN_BOOTLEVEL;
            } else {
                adjustBootLevel();
            }

            long tilNextKick = DEFAULT_BOOTSTRAP_KICK_INTERVAL * ((1L << bootLevel) - 1);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        "Scheduling kick in " + (tilNextKick / TimeUtils.ASECOND) + " seconds at bootLevel " + bootLevel
                        + " in group " + group.getPeerGroupID());
            }

            KickerTask task = new KickerTask();

            addTask(task, tilNextKick, -1);
        } catch (Exception ez1) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Cannot set timer. RPV will not work.", ez1);
            }
        }
    }

    /**
     * Refresh the local copy of the peer advertisement and the rendezvous
     * advertisement.
     */
    private void refreshSelf() {

        RdvAdvertisement radv;

        synchronized (this) {
            PeerAdvertisement newPadv = group.getPeerAdvertisement();
            int newModCount = newPadv.getModCount();

            if ((lastPeerAdv != newPadv) || (lastModCount != newModCount)) {
                lastPeerAdv = newPadv;
                lastModCount = newModCount;

                // create a new local RdvAdvertisement and set it to self.
                radv = createRdvAdvertisement(lastPeerAdv, name);

                if (radv != null) {
                    self.setRdvAdvertisement(radv);
                }
            }
        }
    }

    static RdvAdvertisement createRdvAdvertisement(PeerAdvertisement padv, String serviceName) {

        try {
            // FIX ME: 10/19/2002 lomax@jxta.org. We need to properly set up the service ID. Unfortunately
            // this current implementation of the PeerView takes a String as a service name and not its ID.
            // Since currently, there is only PeerView per group (all peerviews share the same "service", this
            // is not a problem, but that will have to be fixed eventually.

            // create a new RdvAdvertisement
            RdvAdvertisement rdv = (RdvAdvertisement) AdvertisementFactory.newAdvertisement(
                    RdvAdvertisement.getAdvertisementType());

            rdv.setPeerID(padv.getPeerID());
            rdv.setGroupID(padv.getPeerGroupID());
            rdv.setServiceName(serviceName);
            rdv.setName(padv.getName());

            RouteAdvertisement ra = EndpointUtils.extractRouteAdv(padv);

            // Insert it into the RdvAdvertisement.
            rdv.setRouteAdv(ra);

            return rdv;
        } catch (Exception ez) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Cannot create Local RdvAdvertisement: ", ez);
            }
            return null;
        }
    }

    /**
     * Add a listener for PeerViewEvent
     *
     * @param listener An PeerViewListener to process the event.
     * @return  true if successful
     */
    public boolean addListener(PeerViewListener listener) {
        boolean added = rpvListeners.add(listener);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Registered PeerViewEvent Listener (" + listener.getClass().getName() + ")");
        }

        return added;
    }

    /**
     * Removes a PeerViewEvent Listener previously added with addListener.
     *
     * @param listener the PeerViewListener listener remove
     * @return whether successful or not
     */
    public boolean removeListener(PeerViewListener listener) {
        boolean removed = rpvListeners.remove(listener);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Removed PeerViewEvent Listener (" + listener.getClass().getName() + ")");
        }

        return removed;
    }

    /**
     * Generate a PeerView Event and notify all listeners.
     *
     * @param type    the Event Type.
     * @param element The peer having the event.
     */
    private void generateEvent(int type, PeerViewElement element) {

        PeerViewEvent newevent = new PeerViewEvent(this, type, element);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Calling listeners for " + newevent + " in group " + group.getPeerGroupID());
        }

        for (Object o : Arrays.asList(rpvListeners.toArray())) {
            PeerViewListener pvl = (PeerViewListener) o;

            try {
                pvl.peerViewEvent(newevent);
            } catch (Throwable ignored) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in PeerViewEvent listener : (" + pvl.getClass().getName() + ")"
                            ,
                            ignored);
                }
            }
        }
    }

    static PipeAdvertisement makeWirePipeAdvertisement(PeerGroup destGroup, PeerGroup group, String name) {

        PipeAdvertisement adv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(PipeAdvertisement.getAdvertisementType());

        // Create a pipe advertisement for this group.
        // Generate a well known but unique ID.
        // FIXME bondolo 20040507 The ID created is really poor, it has only
        // 2 unique bytes on average. it would be much better to hash something
        // also, since the the definition of how to use the seed bytes is not
        // fixed, it's not reliable.
        PipeID pipeId = IDFactory.newPipeID(destGroup.getPeerGroupID()
                ,
                (SERVICE_NAME + group.getPeerGroupID().getUniqueValue().toString() + name).getBytes());

        adv.setPipeID(pipeId);
        adv.setType(PipeService.PropagateType);
        adv.setName(SERVICE_NAME + " pipe for " + group.getPeerGroupID());

        return adv;
    }

    private synchronized void openWirePipes() {

        PipeService pipes = group.getPipeService();

        if (null == pipes) {
            scheduleOpenPipes(TimeUtils.ASECOND); // Try again in one second.
            return;
        }

        try {
            // First, listen to in our own PeerGroup
            if (null == localGroupWirePipeInputPipe) {
                localGroupWirePipeInputPipe = pipes.createInputPipe(localGroupWirePipeAdv, new WirePipeListener());
            }

            if (null == localGroupWirePipeOutputPipe) {
                // Creates the OutputPipe - note that timeout is irrelevant for
                // propagated pipe.

                localGroupWirePipeOutputPipe = pipes.createOutputPipe(localGroupWirePipeAdv, 1 * TimeUtils.ASECOND);
            }

            if (localGroupWirePipeOutputPipe == null) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Cannot get OutputPipe for current group");
                }
            }
        } catch (Exception failed) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("PipeService not ready yet. Trying again in 1 second.");
            }
            // Try again in one second.
            scheduleOpenPipes(TimeUtils.ASECOND);
            return;
        }

        if (advertisingGroup != null) {
            try {
                pipes = advertisingGroup.getPipeService();

                if (null == pipes) {
                    // Try again in one second.
                    scheduleOpenPipes(TimeUtils.ASECOND);
                    return;
                }

                if (null == wirePipeInputPipe) {
                    wirePipeInputPipe = pipes.createInputPipe(advGroupPropPipeAdv, new WirePipeListener());
                }

                if (null == wirePipeOutputPipe) {
                    wirePipeOutputPipe = pipes.createOutputPipe(advGroupPropPipeAdv, 1 * TimeUtils.ASECOND);
                }

                if (wirePipeOutputPipe == null) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Cannot get OutputPipe for current group");
                    }
                }
            } catch (Exception failed) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Could not open pipes in local group. Trying again in 1 second.");
                }
                // Try again in one second.
                scheduleOpenPipes(TimeUtils.ASECOND);
                return;
            }
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Propagate Pipes opened.");
        }
    }

    private synchronized void closeWirePipes() {

        if (localGroupWirePipeInputPipe != null) {
            localGroupWirePipeInputPipe.close();
            localGroupWirePipeInputPipe = null;
        }

        if (localGroupWirePipeOutputPipe != null) {
            localGroupWirePipeOutputPipe.close();
            localGroupWirePipeOutputPipe = null;
        }

        if (wirePipeInputPipe != null) {
            wirePipeInputPipe.close();
            wirePipeInputPipe = null;
        }

        if (wirePipeOutputPipe != null) {
            wirePipeOutputPipe.close();
            wirePipeOutputPipe = null;
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Propagate Pipes closed.");
        }
    }

    /**
     * Adapter class for receiving wire pipe messages
     */
    private class WirePipeListener implements PipeMsgListener {

        /**
         * {@inheritDoc}
         */
        public void pipeMsgEvent(PipeMsgEvent event) {

            Message msg = event.getMessage();

            boolean failure = (null != msg.getMessageElement(MESSAGE_NAMESPACE, FAILURE_ELEMENT_NAME));
            boolean response = (null != msg.getMessageElement(MESSAGE_NAMESPACE, RESPONSE_ELEMENT_NAME));

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        "Received a PeerView " + (failure ? "failure " : "") + (response ? "response " : "") + "message [" + msg
                        + "] on propagated pipe " + event.getPipeID());
            }

            if (!failure && !response) {

                // If this is not a failure message then decide if we will respond.
                //
                // We play a game that is tuned by the view size so that the expectation of number of responses is equal to
                // minHappyPeerView. The game is to draw a number between 0 and the pv size.  If the result is < minHappyPeerView,
                // then we win (respond) else we lose (stay silent). The probability of winning is HAPPY_SIZE/viewsize. If each of
                // the viewsize peers plays the same game, on average HAPPY_SIZE of them win (with a significant variance, but
                // that is good enough). If viewsize is <= HAPPY_SIZE, then all respond.  This is approximate, of course, since
                // the view size is not always consistent among peers.

                int viewsize = PeerView.this.localView.size();

                if (viewsize > minHappyPeerView) {
                    int randinview = random.nextInt(viewsize);

                    if (randinview >= minHappyPeerView) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Ignoring " + msg + " from pipe " + event.getPipeID());
                        }
                        // We "lose".
                        return;
                    }
                } // Else, we always win; don't bother playing.
            }

            // Fabricate dummy src and dst addrs so that we can call processIncoming. These are
            // only used for traces. The merit of using the pipeID is that it is recognizable
            // in these traces.
            EndpointAddress src = new EndpointAddress(event.getPipeID(), SERVICE_NAME, null);
            EndpointAddress dest = new EndpointAddress(event.getPipeID(), SERVICE_NAME, null);

            try {
                // call the peerview.
                PeerView.this.processIncomingMessage(msg, src, dest);
            } catch (Throwable ez) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed processing " + msg + " from pipe " + event.getPipeID(), ez);
                }
            }
        }
    }

    private synchronized void scheduleAdvertisingGroupQuery(long delay) {

        if (closed) {
            return;
        }

        TimerTask task = new AdvertisingGroupQueryTask();

        addTask(task, delay, -1);
    }

    /**
     * Class implementing the query request on the AdvertisingGroup
     */
    private final class AdvertisingGroupQueryTask extends TimerTask {

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean cancel() {
            boolean res = super.cancel();
            return res;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void run() {
            try {
                if (closed) {
                    return;
                }

                OutputPipe op = wirePipeOutputPipe;

                if (null != op) {
                    Message msg = makeMessage(self, false, false);

                    op.send(msg);
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
                }
            } finally {
                removeTask(this);
            }
        }
    }

    /**
     * Get a copy of the PeerView for this group.
     *
     * @return A SortedSet which is the current local view of the peerview
     */
    public SortedSet<PeerViewElement> getView() {
        synchronized (localView) {
            return new TreeSet<PeerViewElement>((SortedSet)localView);
        }
    }

    /**
     * Add the provided element to the local peerview.
     *
     * @param pve the <code>PeerViewElement</code> to add.
     * @return <code>true</true> if the element was not present and added
     *         otherwise <code>false</code>.
     */
    private boolean addPeerViewElement(PeerViewElement pve) {
        boolean added;

        if (null == pve.getRdvAdvertisement()) {
            throw new IllegalStateException("Cannot add a seed pve to local view");
        }

        synchronized (localView) {
            added = localView.add(pve);

            if (added) {
                // Refresh, if necessary, our up and down peers.
                updateUpAndDownPeers();
            }
        }

        if (added) {
            pve.setPeerView(this);
        }

        return added;
    }

    /**
     * Remove the provided element from the local peerview.
     *
     * @param pve the <code>PeerViewElement</code> to remove.
     * @return <code>true</true> if the element was present and removed
     *         otherwise <code>false</code>.
     */
    private boolean removePeerViewElement(PeerViewElement pve) {
        boolean removed;

        synchronized (localView) {
            removed = localView.remove(pve);

            if (removed) {
                // Refresh, if necessary, our up and down peers.
                updateUpAndDownPeers();
            }
        }

        if (removed) {
            pve.setPeerView(null);
        }

        return removed;
    }

    /**
     * Return from the local view, the PeerViewElement that is equal to the
     * given PeerViewDestination, if one exists or <code>null</code> if it is
     * not present. Identity is defined by {@link PeerViewDestination#equals}
     * which only looks at the destination address. Thus a PeerViewDestination
     * is enough. A full PeerViewElement may be passed as well.  This method
     * does not require external synchronization.
     *
     * @param wanted PeerViewDestination matching the desired one.
     * @return the matching PeerViewElement or <code>null</code> if it could not
     *         be found.
     */
    public PeerViewElement getPeerViewElement(PeerViewDestination wanted) {

        try {
            PeerViewElement found = (PeerViewElement) localView.tailSet(wanted).first();

            if (wanted.equals(found)) {
                return found;
            }
        } catch (NoSuchElementException nse) {// This can happen if the tailset is empty. We could test for it,
            // but it could still become empty after the test, since it reflects
            // concurrent changes to localView. Not worth synchronizing for that
            // rare occurence. The end-result is still correct.
        }

        return null;
    }

    /**
     * Get from the local view, the PeerViewElement for the given PeerID, if one
     * exists. Null otherwise. This method does not require external
     * synchronization.
     *
     * @param pid the PeerID of the desired element.
     * @return the matching PeerViewElement null if it could not be found.
     */
    public PeerViewElement getPeerViewElement(ID pid) {

        return getPeerViewElement(new PeerViewDestination(pid));
    }

    /**
     * Get the down peer from the local peer.
     *
     * @return the down PeerViewElement or null if there is no such peer.
     */
    public PeerViewElement getDownPeer() {
        return downPeer;
    }

    /**
     * Get the local peer.
     *
     * @return the local PeerViewElement
     */
    public PeerViewElement getSelf() {
        return self;
    }

    /**
     * Get the up peer from the local peer.
     *
     * @return the up PeerViewElement or null if there is no such peer.
     */
    public PeerViewElement getUpPeer() {
        return upPeer;
    }

    /**
     * update Up and Down Peers
     */
    private void updateUpAndDownPeers() {

        synchronized (localView) {
            final PeerViewElement oldDown = downPeer;
            final PeerViewElement oldUp = upPeer;

            SortedSet<PeerViewDestination> headSet = localView.headSet(self);

            if (!headSet.isEmpty()) {
                downPeer = (PeerViewElement) headSet.last();
            } else {
                downPeer = null;
            }

            SortedSet<PeerViewDestination> tailSet = localView.tailSet(self);

            if (!tailSet.isEmpty()) {
                if (self.equals(tailSet.first())) {
                    Iterator eachTail = tailSet.iterator();

                    eachTail.next(); // self

                    if (eachTail.hasNext()) {
                        upPeer = (PeerViewElement) eachTail.next();
                    } else {
                        upPeer = null;
                    }
                } else {
                    upPeer = (PeerViewElement) tailSet.first();
                }
            } else {
                upPeer = null;
            }

            if ((oldDown != downPeer) && (downPeer != null)) {
                downPeer.setLastUpdateTime(TimeUtils.timeNow());
            }

            if ((oldUp != upPeer) && (upPeer != null)) {
                upPeer.setLastUpdateTime(TimeUtils.timeNow());
            }
        }
    }

    /**
     * A task that checks on upPeer and downPeer.
     */
    private final class WatchdogTask extends TimerTask {

        /**
         *  The number of iterations that the watchdog task has executed.
         */
        int iterations = 0;
        
        WatchdogTask() {}

        /**
         * {@inheritDoc}
         */
        @Override
        public void run() {
            try {
                if (closed) {
                    return;
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Watchdog task executing for group " + PeerView.this.group.getPeerGroupID());
                }

                refreshSelf();
                
                if(0 == (iterations % 5)) {
                    DiscoveryService discovery = group.getDiscoveryService();
                    if(null != discovery) {
                        discovery.publish(self.getRdvAdvertisement(), WATCHDOG_PERIOD * 10, WATCHDOG_PERIOD * 5);
                    }
                }
                
                PeerViewElement up = PeerView.this.getUpPeer();

                if (up != null) {
                    if (TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), up.getLastUpdateTime()) > WATCHDOG_GRACE_DELAY) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("UP peer has gone MIA : " + up);
                        }

                        notifyFailure(up, true);

                    } else {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Checking on UP peer : " + up);
                        }

                        PeerView.this.send(up, PeerView.this.getSelf(), false, false);
                    }
                }

                PeerViewElement down = PeerView.this.getDownPeer();

                if (down != null) {
                    if (TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), down.getLastUpdateTime()) > WATCHDOG_GRACE_DELAY) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("DOWN peer has gone MIA : " + down);
                        }

                        notifyFailure(down, true);

                    } else {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Checking on DOWN peer : " + down);
                        }

                        PeerView.this.send(down, PeerView.this.getSelf(), false, false);
                    }
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
                }
            }
            
            iterations++;
        }
    }


    /**
     * Class implementing the kicker
     */
    private final class KickerTask extends TimerTask {

        /**
         * {@inheritDoc}
         */
        @Override
        public void run() {
            try {
                if (closed) {
                    return;
                }

                PeerView.this.kick();
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread : " + Thread.currentThread().getName(), all);
                }
            } finally {
                removeTask(this);
            }
        }
    }
}
