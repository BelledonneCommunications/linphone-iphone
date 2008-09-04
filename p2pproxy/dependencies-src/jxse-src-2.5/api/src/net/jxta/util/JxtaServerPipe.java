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
package net.jxta.util;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.IDFactory;
import net.jxta.impl.endpoint.tcp.TcpMessenger;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;

import java.io.IOException;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * The server side of a JxtaBiDiPipe. The intent of this object is accept connection requests.
 * JxtaServerPipe follows the same pattern as java.net.ServerSocket, without it no connection can be
 * established.
 *
 */
public class JxtaServerPipe implements PipeMsgListener {

    private static final Logger LOG = Logger.getLogger(JxtaServerPipe.class.getName());
    protected static final String nameSpace = "JXTABIP";
    protected static final String credTag = "Cred";
    protected static final String reqPipeTag = "reqPipe";
    protected static final String remPeerTag = "remPeer";
    protected static final String remPipeTag = "remPipe";
    protected static final String closeTag = "close";
    protected static final String reliableTag = "reliable";
    protected static final String directSupportedTag = "direct";
    private PeerGroup group;
    private InputPipe serverPipe;
    private PipeAdvertisement pipeadv;
    private int backlog = 50;
    private long timeout = 30 * 1000L;
    private final Object closeLock = new Object();
    protected BlockingQueue<JxtaBiDiPipe> connectionQueue = null;
    private boolean bound = false;
    private boolean closed = false;
    protected StructuredDocument myCredentialDoc = null;
    /**
     * The exceutor service.
     */
    private final ExecutorService executor;

    /**
     * Default constructor for the JxtaServerPipe
     * <p/>
     * backlog default of 50
     * <p> call to accept() for this ServerPipe will
     * block for only this amount of time. If the timeout expires,
     * a java.net.SocketTimeoutException is raised, though the ServerPipe is still valid.
     * <p/>
     *
     * @param group   JXTA PeerGroup
     * @param pipeadv PipeAdvertisement on which pipe requests are accepted
     * @throws IOException if an I/O error occurs
     */
    public JxtaServerPipe(PeerGroup group, PipeAdvertisement pipeadv) throws IOException {
        this(group, pipeadv, 50);
    }

    /**
     * Constructor for the JxtaServerPipe
     *
     * @param group   JXTA PeerGroup
     * @param pipeadv PipeAdvertisement on which pipe requests are accepted
     * @param backlog the maximum length of the queue.
     * @param timeout call to accept() for this ServerPipe will
     *                block for only this amount of time. If the timeout expires,
     *                a java.net.SocketTimeoutException is raised, though the ServerPipe is still valid.
     * @throws IOException if an I/O error occurs
     */
    public JxtaServerPipe(PeerGroup group, PipeAdvertisement pipeadv, int backlog, int timeout) throws IOException {
        this(group, pipeadv, backlog);
        this.timeout = timeout;
    }

    /**
     * Constructor for the JxtaServerPipe object
     *
     * @param group   JXTA PeerGroup
     * @param pipeadv PipeAdvertisement on which pipe requests are accepted
     * @param backlog the maximum length of the queue.
     *                * @exception  IOException  if an I/O error occurs
     * @throws IOException if an I/O error occurs
     */
    public JxtaServerPipe(PeerGroup group, PipeAdvertisement pipeadv, int backlog) throws IOException {
        this.group = group;
        this.executor = Executors.newFixedThreadPool(3);
        this.pipeadv = pipeadv;
        this.backlog = backlog;
        connectionQueue = new ArrayBlockingQueue<JxtaBiDiPipe>(backlog);
        PipeService pipeSvc = group.getPipeService();
        serverPipe = pipeSvc.createInputPipe(pipeadv, this);
        setBound();
    }

    /**
     * Binds the <code>JxtaServerPipe</code> to a specific pipe advertisement
     *
     * @param group   JXTA PeerGroup
     * @param pipeadv PipeAdvertisement on which pipe requests are accepted
     * @throws IOException if an I/O error occurs
     */
    public void bind(PeerGroup group, PipeAdvertisement pipeadv) throws IOException {
        bind(group, pipeadv, backlog);
    }

    /**
     * Binds the <code>JxtaServerPipe</code> to a specific pipe advertisement
     *
     * @param group   JXTA PeerGroup
     * @param pipeadv PipeAdvertisement on which pipe requests are accepted
     * @param backlog the maximum length of the queue.
     * @throws IOException if an I/O error occurs
     */
    public void bind(PeerGroup group, PipeAdvertisement pipeadv, int backlog) throws IOException {
        this.backlog = backlog;
        connectionQueue = new ArrayBlockingQueue<JxtaBiDiPipe>(backlog);
        this.group = group;
        this.pipeadv = pipeadv;
        PipeService pipeSvc = group.getPipeService();
        serverPipe = pipeSvc.createInputPipe(pipeadv, this);
        setBound();
    }

    /**
     * Listens for a connection to be made to this socket and accepts
     * it. The method blocks until a connection is made.
     *
     * @return the connection accepted, null otherwise
     * @throws IOException if an I/O error occurs
     */
    public JxtaBiDiPipe accept() throws IOException {
        if (isClosed()) {
            throw new SocketException("JxtaServerPipe is closed");
        }
        if (!isBound()) {
            throw new SocketException("JxtaServerPipe is not bound yet");
        }
        try {
            JxtaBiDiPipe bidi = connectionQueue.poll(timeout, TimeUnit.MILLISECONDS);
            if (bidi == null) {
                throw new SocketTimeoutException("Timeout reached");
            }
            return bidi;
        } catch (InterruptedException ie) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Interrupted", ie);
            }
            throw new SocketException("interrupted");
        }
    }

    /**
     * Gets the group associated with this JxtaServerPipe
     *
     * @return The group value
     */
    public PeerGroup getGroup() {
        return group;
    }

    /**
     * Gets the PipeAdvertisement associated with this JxtaServerPipe
     *
     * @return The pipeAdv value
     */
    public PipeAdvertisement getPipeAdv() {
        return pipeadv;
    }

    /**
     * Closes this JxtaServerPipe (closes the underlying input pipe).
     *
     * @throws IOException if an I/O error occurs
     */
    public void close() throws IOException {
        synchronized (closeLock) {
            if (isClosed()) {
                return;
            }
            if (bound) {
                // close all the pipe
                serverPipe.close();
                connectionQueue.clear();
                executor.shutdownNow();
                bound = false;
            }
            closed = true;
        }
    }

    /**
     * Sets the bound attribute of the JxtaServerPipe
     */
    void setBound() {
        bound = true;
    }

    /**
     * Gets the Timeout attribute of the JxtaServerPipe
     *
     * @return The soTimeout value
     * @throws IOException if an I/O error occurs
     */

    public synchronized int getPipeTimeout() throws IOException {
        if (isClosed()) {
            throw new SocketException("Server Pipe is closed");
        }
        if (timeout > Integer.MAX_VALUE) {
            return 0;
        } else {
            return (int) timeout;
        }
    }

    /**
     * Sets the Timeout attribute of the JxtaServerPipe a timeout of 0 blocks forever.
     *
     * @param timeout The new soTimeout value
     * @throws SocketException if an I/O error occurs
     */
    public synchronized void setPipeTimeout(int timeout) throws SocketException {
        if (isClosed()) {
            throw new SocketException("Server Pipe is closed");
        }

        if (timeout < 0) {
            throw new IllegalArgumentException("Negative timeout values are not allowed.");
        }

        if (0 == timeout) {
            this.timeout = Long.MAX_VALUE;
        } else {
            this.timeout = (long) timeout;
        }
    }

    /**
     * Returns the closed state of the JxtaServerPipe.
     *
     * @return true if the socket has been closed
     */
    public boolean isClosed() {
        synchronized (closeLock) {
            return closed;
        }
    }

    /**
     * Returns the binding state of the JxtaServerPipe.
     *
     * @return true if the ServerSocket successfully bound to an address
     */
    public boolean isBound() {
        return bound;
    }

    /**
     * {@inheritDoc}
     */
    public void pipeMsgEvent(PipeMsgEvent event) {
        Message message = event.getMessage();
        if (message == null) {
            return;
        }
        ConnectionProcessor processor = new ConnectionProcessor(message);
        executor.execute(processor);
    }

    /**
     * Method processMessage is the heart of this class.
     * <p/>
     * This takes new incoming connect messages and constructs the JxtaBiDiPipe
     * to talk to the new client.
     * <p/>
     * The ResponseMessage is created and sent.
     *
     * @param msg The client connection request (assumed not null)
     * @return JxtaBiDiPipe Which may be null if an error occurs.
     */
    private JxtaBiDiPipe processMessage(Message msg) {

        PipeAdvertisement outputPipeAdv = null;
        PeerAdvertisement peerAdv = null;
        StructuredDocument credDoc = null;
        try {
            MessageElement el = msg.getMessageElement(nameSpace, credTag);

            if (el != null) {
                credDoc = StructuredDocumentFactory.newStructuredDocument(el);
            }

            el = msg.getMessageElement(nameSpace, reqPipeTag);
            if (el != null) {
                XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(el);
                outputPipeAdv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(asDoc);
            }

            el = msg.getMessageElement(nameSpace, remPeerTag);
            if (el != null) {
                XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(el);
                peerAdv = (PeerAdvertisement) AdvertisementFactory.newAdvertisement(asDoc);
            }

            el = msg.getMessageElement(nameSpace, reliableTag);
            boolean isReliable = false;
            if (el != null) {
                isReliable = Boolean.valueOf((el.toString()));
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Connection request [isReliable] :" + isReliable);
                }
            }

            el = msg.getMessageElement(nameSpace, directSupportedTag);
            boolean directSupported = false;
            if (el != null) {
                directSupported = Boolean.valueOf((el.toString()));
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Connection request [directSupported] :" + directSupported);
                }
            }

            Messenger msgr;
            boolean direct = false;
            if (directSupported) {
                msgr = JxtaBiDiPipe.getDirectMessenger(group, outputPipeAdv, peerAdv);
                if (msgr == null) {
                    msgr = JxtaBiDiPipe.lightweightOutputPipe(group, outputPipeAdv, peerAdv);
                } else {
                    direct = true;
                }
            } else {
                msgr = JxtaBiDiPipe.lightweightOutputPipe(group, outputPipeAdv, peerAdv);
            }

            if (msgr != null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Reliability set to :" + isReliable);
                }
                PipeAdvertisement newpipe = newInputPipe(group, outputPipeAdv);
                JxtaBiDiPipe pipe = new JxtaBiDiPipe(group, msgr, newpipe, credDoc, isReliable, direct);

                pipe.setRemotePeerAdvertisement(peerAdv);
                pipe.setRemotePipeAdvertisement(outputPipeAdv);
                sendResponseMessage(group, msgr, newpipe);
                return pipe;
            }
        } catch (IOException e) {
            // deal with the error
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "IOException occured", e);
            }
        }
        return null;
    }

    /**
     * Method sendResponseMessage get the createResponseMessage and sends it.
     *
     * @param group  the peer group
     * @param msgr   the remote node messenger
     * @param pipeAd the pipe advertisement
     * @throws IOException for failures sending the response message.
     */
    protected void sendResponseMessage(PeerGroup group, Messenger msgr, PipeAdvertisement pipeAd) throws IOException {

        Message msg = new Message();
        PeerAdvertisement peerAdv = group.getPeerAdvertisement();

        if (myCredentialDoc == null) {
            myCredentialDoc = JxtaBiDiPipe.getCredDoc(group);
        }

        if (myCredentialDoc != null) {
            msg.addMessageElement(JxtaServerPipe.nameSpace,
                    new TextDocumentMessageElement(credTag, (XMLDocument) myCredentialDoc, null));
        }

        msg.addMessageElement(JxtaServerPipe.nameSpace,
                new StringMessageElement(JxtaServerPipe.directSupportedTag, Boolean.toString(true), null));

        msg.addMessageElement(JxtaServerPipe.nameSpace,
                new TextDocumentMessageElement(remPipeTag, (XMLDocument) pipeAd.getDocument(MimeMediaType.XMLUTF8), null));

        msg.addMessageElement(nameSpace,
                new TextDocumentMessageElement(remPeerTag, (XMLDocument) peerAdv.getDocument(MimeMediaType.XMLUTF8), null));
        if (msgr instanceof TcpMessenger) {
            ((TcpMessenger) msgr).sendMessageDirect(msg, null, null, true);
        } else {
            msgr.sendMessage(msg);
        }
    }

    /**
     * Utility method newInputPipe is used to get new pipe advertisement (w/random pipe ID) from old one.
     * <p/>
     * Called by JxtaSocket to make pipe (name -> name.remote) for open message
     * <p/>
     * Called by JxtaServerSocket to make pipe (name.remote -> name.remote.remote) for response message
     *
     * @param group   the peer group
     * @param pipeadv to get the basename and type from
     * @return PipeAdvertisement a new pipe advertisement
     */
    protected static PipeAdvertisement newInputPipe(PeerGroup group, PipeAdvertisement pipeadv) {
        PipeAdvertisement adv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(PipeAdvertisement.getAdvertisementType());
        adv.setPipeID(IDFactory.newPipeID(group.getPeerGroupID()));
        adv.setName(pipeadv.getName());
        adv.setType(pipeadv.getType());
        return adv;
    }

    /**
     * get the credential doc
     *
     * @return Credential StructuredDocument
     */
    public StructuredDocument getCredentialDoc() {
        return myCredentialDoc;
    }

    /**
     * Sets the connection credential doc
     * If no credentials are set, the default group credential will be used
     *
     * @param doc Credential StructuredDocument
     */
    public void setCredentialDoc(StructuredDocument doc) {
        this.myCredentialDoc = doc;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Closes the JxtaServerPipe.
     */
    @Override
    protected synchronized void finalize() throws Throwable {
        if (!closed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("JxtaServerPipe is being finalized without being previously closed. This is likely a user's bug.");
            }
        }
        close();
        super.finalize();
    }
    /**
     * A small class for processing individual messages.
     */
    private class ConnectionProcessor implements Runnable {

        private Message message;
        ConnectionProcessor(Message message) {
            this.message = message;
        }

        public void run() {
            JxtaBiDiPipe bidi = processMessage(message);
            // make sure we have a socket returning
            if (bidi != null) {
                try {
                    connectionQueue.offer(bidi, timeout, TimeUnit.MILLISECONDS);
                } catch (InterruptedException e) {
                    Thread.interrupted();
                }
            }
        }
    }
}
