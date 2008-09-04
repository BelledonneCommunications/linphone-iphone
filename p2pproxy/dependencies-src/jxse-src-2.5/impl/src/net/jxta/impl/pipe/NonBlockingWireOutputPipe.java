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
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.OutputPipe;
import net.jxta.protocol.PipeAdvertisement;

import java.io.IOException;
import java.util.HashSet;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * An implementation of Ouput Pipe which sends messages on the pipe
 * asynchronously. The <code>send()</code> method for this implementation will
 * never block.
 */
class NonBlockingWireOutputPipe implements OutputPipe {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(NonBlockingWireOutputPipe.class.getName());

    /**
     * If true then the pipe has been closed and will no longer accept messages.
     */
    private volatile boolean closed = false;

    /**
     * Group in which we are working.
     */
    private final PeerGroup peerGroup;

    /**
     * The endpoint of our group.
     */
    private final WirePipe wire;

    /**
     * The advertisement we were created from.
     */
    private final PipeAdvertisement pAdv;

    /**
     * The set of peers to which messages on this pipe are sent. If empty then
     * the message is sent to all propagation targets.
     */
    private final Set<? extends ID> destPeers;

    /**
     * Create a new output pipe
     *
     * @param group     The peergroup we are working in.
     * @param wire  The propagate pipe service.
     * @param pAdv  advertisement for the pipe we are supporting.
     * @param peers the set of peers we allow this pipe to be bound to.
     */
    public NonBlockingWireOutputPipe(PeerGroup group, WirePipe wire, PipeAdvertisement pAdv, Set<? extends ID> peers) {

        peerGroup = group;
        this.wire = wire;
        this.destPeers = new HashSet<ID>(peers);
        this.pAdv = pAdv;

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Constructing for " + getPipeID());
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void close() {

        // Close the queue so that no more messages are accepted
        if (!closed) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Closing queue for " + getPipeID());
            }
        }
        closed = true;
    }

    /**
     * {@inheritDoc}
     */
    public boolean isClosed() {
        return closed;
    }

    /**
     * {@inheritDoc}
     */
    public final String getType() {
        return pAdv.getType();
    }

    /**
     * {@inheritDoc}
     */
    public final ID getPipeID() {
        return pAdv.getPipeID();
    }

    /**
     * {@inheritDoc}
     */
    public final String getName() {
        return pAdv.getName();
    }

    /**
     * {@inheritDoc}
     */
    public final PipeAdvertisement getAdvertisement() {
        return pAdv;
    }

    /**
     * {@inheritDoc}
     */
    public boolean send(Message message) throws IOException {
        if (closed) {
            // also throw it here to void the extra operations
            throw new IOException("Pipe closed");
        }

        WireHeader header = new WireHeader();
        header.setPipeID(getPipeID());
        header.setSrcPeer(peerGroup.getPeerID());
        header.setTTL(destPeers.isEmpty() ? 200 : 1);
        header.setMsgId(WirePipe.createMsgId());

        XMLDocument asDoc = (XMLDocument) header.getDocument(MimeMediaType.XMLUTF8);
        MessageElement elem = new TextDocumentMessageElement(WirePipeImpl.WIRE_HEADER_ELEMENT_NAME, asDoc, null);

        Message msg = message.clone();
        msg.replaceMessageElement(WirePipeImpl.WIRE_HEADER_ELEMENT_NAMESPACE, elem);
        return sendUnModified(msg, header);
    }

    /**
     * Sends a message unaltered
     *
     * @param msg the message to send
     * @return true if successful
     * @throws IOException if an io error occurs
     * @param header message header
     */
    boolean sendUnModified(Message msg, WireHeader header) throws IOException {
        if (closed) {
            throw new IOException("Pipe closed");
        }
        wire.sendMessage(msg, destPeers, header);
        // we are here, there are not io exception, we assume it succeeded
        return true;
    }
}
