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
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.impl.endpoint.tcp.TcpMessenger;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.OutputPipe;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * This Object is created when a call to PipeService.createOutputPipe(propgateAdv) with a Set containing a single
 * PeerID.  This pipe blocks until a valid messeger is created (i.e. resolved and useable). With this object it is
 * possible to detect connection failures during the messenger resolution. Notge, this pipe also avoids utilitizing the
 * rendezvous for propagation, effectively reducing message overhead, resulting in improved performance.
 * <p/>
 * #send is remains asynchronous.
 */
public class BlockingWireOutputPipe implements OutputPipe {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(NonBlockingWireOutputPipe.class.getName());

    /**
     * If true then the pipe has been closed and will no longer accept messages.
     */
    private volatile boolean closed = false;

    /**
     * The advertisement we were created from.
     */
    private final PipeAdvertisement pAdv;

    private final PeerGroup group;
    private Messenger destMessenger = null;
    private EndpointAddress destination;
    private EndpointService endpoint = null;
    private RouteAdvertisement route = null;

    /**
     * Create a new blocking output pipe
     *
     * @param group  The peergroup context.
     * @param pAdv   advertisement for the pipe we are supporting.
     * @param peerID the destination <code>PeerID</code>.
     */
    public BlockingWireOutputPipe(PeerGroup group, PipeAdvertisement pAdv, PeerID peerID) {

        this.pAdv = pAdv;
        this.group = group;
        this.endpoint = group.getEndpointService();
        destination = new EndpointAddress("jxta", peerID.getUniqueValue().toString(), "PipeService", pAdv.getID().toString());
        destMessenger = endpoint.getMessenger(destination);

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Created output pipe for " + getPipeID());
        }
    }
    /**
     * Create a new blocking output pipe
     *
     * @param group  The peergroup context.
     * @param pAdv   advertisement for the pipe we are supporting.
     * @param peerID the destination <code>PeerID</code>.
     * @param route the destination route.
     */
    public BlockingWireOutputPipe(PeerGroup group, PipeAdvertisement pAdv, PeerID peerID, RouteAdvertisement route) {
        this.route = route;
        this.pAdv = pAdv;
        this.group = group;
        this.endpoint = group.getEndpointService();
        destination = new EndpointAddress("jxta", peerID.getUniqueValue().toString(), "PipeService", pAdv.getID().toString());
        if (route != null) {
            destMessenger = endpoint.getDirectMessenger(destination, route, true);
        }
        if (destMessenger == null) {
            destMessenger = endpoint.getMessenger(destination);
        }
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Created output pipe for " + getPipeID());
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void close() {

        if (closed) {
            return;
        }
        // Close the queue so that no more messages are accepted
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Closing queue for " + getPipeID());
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

    private void checkMessenger() throws IOException {
        if (!(destMessenger instanceof TcpMessenger) && destMessenger != null && (destMessenger.getState() & Messenger.USABLE) != 0) {
            return;
        }
        if (destMessenger != null && !destMessenger.isClosed()) {
                return;
        }
        synchronized (this) {
            if (route != null) {
                destMessenger = endpoint.getDirectMessenger(destination, route, true);
            }
            destMessenger = endpoint.getMessenger(destination);
            if (!(destMessenger instanceof TcpMessenger) && destMessenger == null || (destMessenger.getState() & Messenger.TERMINAL) != 0) {
                if (destMessenger != null) {
                    destMessenger.close();
                    destMessenger = null;
                }
                throw new IOException("Unable to create a messenger to " + destination.toString());
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public boolean send(Message message) throws IOException {
        if (closed) {
            throw new IOException("Pipe closed");
        }
        WireHeader header = new WireHeader();

        header.setPipeID(getPipeID());
        header.setSrcPeer(group.getPeerID());
        header.setTTL(1);
        header.setMsgId(WirePipe.createMsgId());

        XMLDocument asDoc = (XMLDocument) header.getDocument(MimeMediaType.XMLUTF8);
        MessageElement elem = new TextDocumentMessageElement(WirePipeImpl.WIRE_HEADER_ELEMENT_NAME, asDoc, null);
        Message msg = message.clone();

        msg.replaceMessageElement(WirePipeImpl.WIRE_HEADER_ELEMENT_NAMESPACE, elem);
        checkMessenger();
        try {
            if (destMessenger instanceof TcpMessenger) {
                ((TcpMessenger) destMessenger).sendMessageDirect(msg, null, null, true);
            } else  if (!destMessenger.sendMessage(msg, null, null)) {
                throw new IOException("Pipe closed");
            }
        } catch (IOException io) {
            checkMessenger();
            if (!destMessenger.sendMessage(msg, null, null)) {
                throw new IOException("Pipe closed");
            }
        }
        return true;
    }
}
