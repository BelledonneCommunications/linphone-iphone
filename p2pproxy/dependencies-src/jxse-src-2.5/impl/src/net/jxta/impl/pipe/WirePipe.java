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
package net.jxta.impl.pipe;

import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.*;
import net.jxta.id.ID;
import net.jxta.impl.cm.SrdiIndex;
import net.jxta.impl.id.UUID.UUID;
import net.jxta.impl.id.UUID.UUIDFactory;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.rendezvous.RendezVousService;

import java.io.IOException;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * WirePipe (aka Propagated pipe) is very similar to IGMP, where a creation of an
 * input pipe results in a propagated pipe membership registration with the peer's
 * rendezvous peer, and a closure results in a propagated pipe group resignation,
 * these group registration/resignation are simply PipeService SRDI messages.
 */
public class WirePipe implements EndpointListener, InputPipe, PipeRegistrar {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(WirePipe.class.getName());

    /**
     * The number of message ID we track to eliminate duplicate messages.
     */
    private static final int MAX_RECORDED_MSGIDS = 250;

    private volatile boolean closed = false;

    private final PeerGroup peerGroup;
    private final PipeResolver pipeResolver;
    private final WirePipeImpl wireService;
    private final PipeAdvertisement pipeAdv;

    private final RendezVousService rendezvous;
    private final PeerID localPeerId;
    private NonBlockingWireOutputPipe repropagater;
    int messagesReceived = 0;

    /**
     * Table of local input pipes listening on this pipe. Weak map (used as a
     * Set) so that we don't keep pipes unnaturally alive and consuming
     * resources.
     */
    private final Map<InputPipe, Object> wireinputpipes = new WeakHashMap<InputPipe, Object>();

    /**
     * The list of message ids we have already seen. The most recently seen
     * messages are at the end of the list.
     * <p/>
     * XXX 20031102 bondolo@jxta.org: We might want to consider three
     * performance enhancements:
     * <p/>
     * <ul>
     * <li>Reverse the order of elements in the list. This would result
     * in faster searching since the default strategy for ArrayList to
     * search in index order. We are most likely to see duplicate messages
     * amongst the messages we have seen recently. This would make
     * additions more costly.</li>
     * <p/>
     * <li>When we do find a duplicate in the list, exchange it's location
     * with the newest message in the list. This will keep annoying dupes
     * close to the start of the list.</li>
     * <p/>
     * <li>When the array reaches MaxNbOfStoredIds treat it as a ring.</li>
     * </ul>
     */
    private final List<UUID> msgIds = new ArrayList<UUID>(MAX_RECORDED_MSGIDS);

    /**
     * Constructor
     *
     * @param group            The Group associated with this service
     * @param pipeResolver the associated pipe resolver
     * @param wireService  The pipe service associated with this pipe
     * @param adv          pipe advertisement
     */
    public WirePipe(PeerGroup group, PipeResolver pipeResolver, WirePipeImpl wireService, PipeAdvertisement adv) {
        this.peerGroup = group;
        this.pipeResolver = pipeResolver;
        this.wireService = wireService;
        this.pipeAdv = adv;
        localPeerId = peerGroup.getPeerID();
        rendezvous = group.getRendezVousService();
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Closes the pipe.
     */
    @Override
    protected synchronized void finalize() throws Throwable {

        if (!closed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Pipe is being finalized without being previously closed. This is likely a bug.");
            }
        }
        close();
        super.finalize();
    }

    /**
     * {@inheritDoc}
     */
    public synchronized boolean register(InputPipe wireinputpipe) {
        if (closed) {
            return false;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Registering input pipe for " + pipeAdv.getPipeID());
        }

        wireinputpipes.put(wireinputpipe, null);
        boolean registered;
        if (1 == wireinputpipes.size()) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Registering " + pipeAdv.getPipeID() + " with pipe resolver.");
            }
            registered = pipeResolver.register(this);
        } else {
            registered = true;
        }
        return registered;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized boolean forget(InputPipe wireinputpipe) {
        boolean removed = null != wireinputpipes.remove(wireinputpipe);

        if (wireinputpipes.isEmpty()) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Deregistering wire pipe with pipe resolver");
            }
            pipeResolver.forget(this);
        }

        if (removed && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Removed input pipe for " + pipeAdv.getPipeID());
        }
        return removed;
    }

    // This is the InputPipe API implementation.
    // This is needed only to be able to register an InputPipe to the PipeResolver.
    // Not everything has to be implemented.

    /**
     * {@inheritDoc}
     */
    public Message waitForMessage() throws InterruptedException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("This method is not really supported.");
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    public Message poll(int timeout) throws InterruptedException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("This method is not really supported.");
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void close() {
        if (closed) {
            return;
        }

        closed = true;

        if (null != repropagater) {
            repropagater.close();
            repropagater = null;
        }

        List<InputPipe> inputPipes = new ArrayList<InputPipe>(wireinputpipes.keySet());

        for (InputPipe anInputPipe : inputPipes) {
            anInputPipe.close();
        }

        wireinputpipes.clear();
        msgIds.clear();
        wireService.forgetWirePipe(pipeAdv.getPipeID());
    }

    /**
     * {@inheritDoc}
     */
    public String getType() {
        return pipeAdv.getType();
    }

    /**
     * {@inheritDoc}
     */
    public ID getPipeID() {
        return pipeAdv.getPipeID();
    }

    /**
     * {@inheritDoc}
     */
    public String getName() {
        return pipeAdv.getName();
    }

    /**
     * {@inheritDoc}
     */
    public PipeAdvertisement getAdvertisement() {
        return pipeAdv;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Handler for messages received through the normal pipe endpoint
     * listener.
     * <p/>
     * "PipeService" / &lt;PipeID&gt;
     */
    public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        // Check if there is a JXTA-WIRE header
        MessageElement elem = message.getMessageElement(WirePipeImpl.WIRE_HEADER_ELEMENT_NAMESPACE,
                WirePipeImpl.WIRE_HEADER_ELEMENT_NAME);

        if (null == elem) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No JxtaWireHeader element. Discarding " + message);
            }
            return;
        }

        WireHeader header;
        try {
            XMLDocument doc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(elem);
            header = new WireHeader(doc);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "bad wire header", e);
            }
            return;
        }
        processIncomingMessage(message, header, srcAddr, dstAddr);
    }

    /**
     * local version with the wire header already parsed. There are two paths
     * to this point; via the local endpoint listener or via the general
     * propagation listener in WirePipeImpl.
     *
     * @param message the message
     * @param header  the wire header
     * @param srcAddr source
     * @param dstAddr destination
     */
    void processIncomingMessage(Message message, WireHeader header, EndpointAddress srcAddr, EndpointAddress dstAddr) {
        if (recordSeenMessage(header.getMsgId())) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Discarding duplicate " + message);
            }
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Processing " + message + " from " + srcAddr + " on " + pipeAdv.getPipeID());
        }
        callLocalListeners(message, srcAddr, dstAddr);
        if (peerGroup.isRendezvous()) {
            repropagate(message, header);
        }
    }

    /**
     * Calls the local listeners for a given pipe.
     *
     * @param message the message
     * @param srcAddr source The peer which sent us the message (last hop).
     * @param dstAddr dest The wire pipe the message was sent to.
     */
    private void callLocalListeners(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        List<InputPipeImpl> listeners = new ArrayList(wireinputpipes.keySet());
        if (listeners.isEmpty()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No local listeners for " + pipeAdv.getPipeID());
            }
        } else {
            int listenersCalled = 0;
            for (InputPipeImpl anInputPipe : listeners) {
                try {
                    anInputPipe.processIncomingMessage(message.clone(), srcAddr, dstAddr);
                    listenersCalled++;
                } catch (Throwable ignored) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "Uncaught Throwable during callback (" + anInputPipe + ") for " + anInputPipe.getPipeID(), ignored);
                    }
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Called " + listenersCalled + " of " + listeners.size() + " local listeners for " + pipeAdv.getPipeID());
            }
        }
    }

    /**
     * Repropagate a message.
     *
     * @param message the message
     * @param header  the header
     */
    void repropagate(Message message, WireHeader header) {

        if (closed) {
            return;
        }

        if ((header.getTTL() <= 1)) {
            // This message has run out of fuel.
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No TTL remaining - discarding " + message + " on " + header.getPipeID());
            }
            return;
        }

        Message msg = message.clone();
        header.setTTL(header.getTTL() - 1);
        XMLDocument headerDoc = (XMLDocument) header.getDocument(MimeMediaType.XMLUTF8);
        MessageElement elem = new TextDocumentMessageElement(WirePipeImpl.WIRE_HEADER_ELEMENT_NAME, headerDoc, null);

        msg.replaceMessageElement(WirePipeImpl.WIRE_HEADER_ELEMENT_NAMESPACE, elem);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Repropagating " + msg + " on " + header.getPipeID());
        }

        synchronized (this) {
            if (closed) {
                return;
            }
            if (null == repropagater) {
                repropagater = wireService.createOutputPipe(pipeAdv, Collections.EMPTY_SET);
            }
        }

        try {
            if (!repropagater.sendUnModified(msg, header)) {
                // XXX bondolo@jxta.org we don't make any attempt to retry.
                // There is a potential problem in that we have accepted the
                // message locally but didn't resend it. If we get another copy
                // of this message then we will NOT attempt to repropagate it
                // even if we should.
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Failure repropagating " + msg + " on " + header.getPipeID() + ". Could not queue message.");
                }
            }
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure repropagating " + msg + " on " + header.getPipeID(), failed);
            }
        }
    }

    /**
     * Sends a message on the propagated pipe.  if set is not empty, then the
     * message is sent to set of peers.
     *
     * @param message    The message to send.
     * @param peers  The peers to which the message will be sent. If the
     *               set is empty then the message is sent to all members of the pipe that are
     *               connected to the rendezvous, as well as walk the message through the network
     * @param header message header
     * @throws java.io.IOException if an io error occurs
     */
    void sendMessage(Message message, Set<? extends ID> peers, WireHeader header) throws IOException {
        message = message.clone();

        // do local listeners if we are to be one of the destinations
        if (peers.isEmpty() || peers.contains(localPeerId)) {
            if (!recordSeenMessage(header.getMsgId())) {
                callLocalListeners(message, new EndpointAddress(localPeerId, null, null),
                        new EndpointAddress(pipeAdv.getPipeID(), null, null));
            }
        }

        if (peers.isEmpty()) {
            if (peerGroup.isRendezvous()) {
                // propagate to my clients
                SrdiIndex srdiIndex = pipeResolver.getSrdiIndex();
                List<PeerID> peerids = srdiIndex.query(PipeService.PropagateType, PipeAdvertisement.IdTag, getPipeID().toString(),
                        Integer.MAX_VALUE);

                peerids.retainAll(rendezvous.getConnectedPeerIDs());
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Propagating " + message + " to " + peerids.size() + " subscriber peers.");
                }
                rendezvous.propagate(Collections.enumeration(peerids), message, WirePipeImpl.WIRE_SERVICE_NAME,
                        wireService.getServiceParameter(), 1);
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Propagating " + message + " to whole network.");
                }
                // propagate to local sub-net
                rendezvous.propagateToNeighbors(message, WirePipeImpl.WIRE_SERVICE_NAME, wireService.getServiceParameter(),
                        RendezVousService.DEFAULT_TTL);
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Walking " + message + " through peerview.");
            }
            // walk the message through rdv network (edge, or rendezvous)
            rendezvous.walk(message, WirePipeImpl.WIRE_SERVICE_NAME, wireService.getServiceParameter(),
                    RendezVousService.DEFAULT_TTL);
        } else {
            // Send to specific peers
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Propagating " + message + " to " + peers.size() + " peers.");
            }
            rendezvous.propagate(Collections.enumeration(peers), message, WirePipeImpl.WIRE_SERVICE_NAME,
                    wireService.getServiceParameter(), 1);
        }
    }

    /**
     * Create a unique (mostly) identifier for this message
     *
     * @return a message sequence uuid
     */
    static String createMsgId() {
        return UUIDFactory.newSeqUUID().toString();
    }

    /**
     * Adds a message ID to our table or stored IDs.
     *
     * @param id to add.
     * @return false if ID was added, true if it was a duplicate.
     */
    private boolean recordSeenMessage(String id) {

        UUID msgid;
        try {
            msgid = new UUID(id);
        } catch (IllegalArgumentException notauuid) {
            // XXX 20031024 bondolo@jxta.org these two conversions are provided
            // for backwards compatibility and should eventually be removed.
            try {
                msgid = UUIDFactory.newHashUUID(Long.parseLong(id), 0);
            } catch (NumberFormatException notanumber) {
                msgid = UUIDFactory.newHashUUID(id.hashCode(), 0);
            }
        }
        synchronized (msgIds) {
            if (msgIds.contains(msgid)) {
                // Already there. Nothing to do
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("duplicate " + msgid);
                }
                return true;
            }
            if (msgIds.size() < MAX_RECORDED_MSGIDS) {
                msgIds.add(msgid);
            } else {
                msgIds.set((messagesReceived % MAX_RECORDED_MSGIDS), msgid);
            }
            messagesReceived++;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("added " + msgid);
        }
        return false;
    }
}
