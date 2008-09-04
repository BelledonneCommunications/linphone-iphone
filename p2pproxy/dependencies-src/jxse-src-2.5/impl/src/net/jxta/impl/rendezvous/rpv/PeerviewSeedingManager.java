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
package net.jxta.impl.rendezvous.rpv;

import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import java.util.logging.Logger;
import java.util.logging.Level;

import net.jxta.logging.Logging;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;

import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.RouteAdvertisement;

import net.jxta.protocol.RdvAdvertisement;
import net.jxta.impl.util.ACLSeedingManager;
import net.jxta.impl.util.TimeUtils;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PipeAdvertisement;

/**
 * A Seeding Manager which uses the peerview advertisement pipes in order to
 * locate seed peers for a given peer group.
 */
public class PeerviewSeedingManager extends ACLSeedingManager implements EndpointListener {

    /**
     * Logger
     */
    private static final transient Logger LOG = Logger.getLogger(PeerviewSeedingManager.class.getName());

    /**
     * This is the minimum interval at which we will refresh our "peerview"
     */
    protected final static long MINIMUM_PEERVIEW_REFRESH_INTERVAL = 1 * TimeUtils.AMINUTE;

    /**
     * The standard time interval after which we will refresh our "peerview"
     */
    protected final static long STANDARD_PEERVIEW_REFRESH_INTERVAL = 20 * TimeUtils.AMINUTE;

    /**
     * Our mock peerview we use to keep responses.
     */
    protected Set<RouteAdvertisement> peerview = new HashSet<RouteAdvertisement>();

    /**
     * The absolute time at which we will refresh our "PeerView"
     */
    protected long nextPeerViewRefresh = 0;

    /**
     * The absolute time at which we will refresh our "PeerView"
     */
    protected int unsuccessfulProbes = 0;

    protected final PeerGroup advGroup;

    protected final PeerGroup group;

    protected final String name;

    protected final PipeAdvertisement advPipeAdv;

    /**
     * Creates a new instance of PeerviewSeedingManager
     *
     * @param aclLocation ACL uri
     * @param group       the group context
     * @param advGroup    the advertising group
     * @param name        service name
     */
    public PeerviewSeedingManager(URI aclLocation, PeerGroup group, PeerGroup advGroup, String name) {
        super(aclLocation);

        this.group = group;
        this.advGroup = advGroup;
        this.name = name;

        advPipeAdv = PeerView.makeWirePipeAdvertisement(advGroup, group, name);

        group.getEndpointService().addIncomingMessageListener(this, PeerView.SERVICE_NAME, group.getPeerGroupID().getUniqueValue().toString());
    }

    /**
     * {@inheritDoc}
     */
    public void stop() {
        group.getEndpointService().removeIncomingMessageListener(PeerView.SERVICE_NAME, group.getPeerGroupID().getUniqueValue().toString());
    }

    /**
     * Adds a rpv seed
     *
     * @param seed the seed
     */
    public void addSeed(RouteAdvertisement seed) {
        peerview.add(seed);
    }

    /**
     * {@inheritDoc}
     */
    public URI[] getActiveSeedURIs() {
        throw new UnsupportedOperationException("Not supported.");
    }

    /**
     * {@inheritDoc}
     */
    public synchronized RouteAdvertisement[] getActiveSeedRoutes() {
        refreshActiveSeeds();

        List<RouteAdvertisement> result = new ArrayList<RouteAdvertisement>();

        for (RouteAdvertisement anRA : peerview) {
            result.add(anRA.clone());
        }

        return result.toArray(new RouteAdvertisement[result.size()]);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized boolean isAcceptablePeer(RouteAdvertisement radv) {
        boolean acceptable = peerview.contains(radv);

        return acceptable && super.isAcceptablePeer(radv);
    }

    protected void refreshActiveSeeds() {
        if (TimeUtils.timeNow() < nextPeerViewRefresh) {
            return;
        }

        // remove the old stale entries.
        peerview.clear();

        Message message = makeMessage();

        try {
            PipeService pipes = advGroup.getPipeService();

            OutputPipe output = pipes.createOutputPipe(advPipeAdv, 30 * TimeUtils.ASECOND);

            output.send(message);

            output.close();

            // Only update the refresh if we we able to send.

            long untilNextRefresh;

            if (peerview.isEmpty()) {
                unsuccessfulProbes++;
                untilNextRefresh = Math.min(unsuccessfulProbes * MINIMUM_PEERVIEW_REFRESH_INTERVAL,
                        STANDARD_PEERVIEW_REFRESH_INTERVAL);
            } else {
                untilNextRefresh = STANDARD_PEERVIEW_REFRESH_INTERVAL;
            }

            nextPeerViewRefresh = TimeUtils.toAbsoluteTimeMillis(untilNextRefresh);
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed sending " + message + ".", failed);
            }
        }
    }

    /**
     * Make a PeerView Message.
     *
     * @return a peer view message
     */
    private Message makeMessage() {

        Message msg = new Message();

        msg.addMessageElement(PeerView.MESSAGE_NAMESPACE, PeerView.EDGE_ELEMENT);

        RdvAdvertisement radv = PeerView.createRdvAdvertisement(group.getPeerAdvertisement(), name);

        XMLDocument doc = (XMLDocument) radv.getDocument(MimeMediaType.XMLUTF8);

        MessageElement msge = new TextDocumentMessageElement(PeerView.MESSAGE_ELEMENT_NAME, doc, null);

        msg.addMessageElement(PeerView.MESSAGE_NAMESPACE, msge);

        return msg;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>Listener for "PeerView"/&lt;peergroup-unique-id> and propagate pipes.
     */
    public synchronized void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        // check what kind of message this is (response or not).
        boolean isResponse = false;
        MessageElement me = msg.getMessageElement(PeerView.MESSAGE_NAMESPACE, PeerView.MESSAGE_ELEMENT_NAME);

        if (me == null) {
            me = msg.getMessageElement(PeerView.MESSAGE_NAMESPACE, PeerView.RESPONSE_ELEMENT_NAME);
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
                LOG.warning("Radv does not contain route");
            }
            return;
        }

        // See if we can find a src route adv in the message.s
        me = msg.getMessageElement(PeerView.MESSAGE_NAMESPACE, PeerView.SRCROUTEADV_ELEMENT_NAME);
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

        boolean isFailure = (msg.getMessageElement(PeerView.MESSAGE_NAMESPACE, PeerView.FAILURE_ELEMENT_NAME) != null);
        boolean isCached = (msg.getMessageElement(PeerView.MESSAGE_NAMESPACE, PeerView.CACHED_RADV_ELEMENT_NAME) != null);
        boolean isFromEdge = (msg.getMessageElement(PeerView.MESSAGE_NAMESPACE, PeerView.EDGE_ELEMENT_NAME) != null);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            String srcPeer = srcAddr.toString();
            LOG.fine("[" + group.getPeerGroupID() + "] Received a" + (isCached ? " cached" : "") + (isResponse ? " response" : "")
                    + (isFailure ? " failure" : "") + " message (" + msg.toString() + ")" + (isFromEdge ? " from edge" : "")
                    + " regarding \"" + radv.getName() + "\" from " + srcPeer);
        }

        if (!isResponse || isFailure || isCached || isFromEdge) {
            // We don't care about anything except responses from active rdvs.
            return;
        }

        peerview.add(radv.getRouteAdv());
    }
}
