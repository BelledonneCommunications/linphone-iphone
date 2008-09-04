/*
 * Copyright (c) 2006-2007 Sun Microsystems, Inc.  All rights reserved.
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

import net.jxta.credential.Credential;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.id.ID;
import net.jxta.impl.endpoint.tcp.TcpMessenger;
import net.jxta.impl.util.pipe.reliable.Defs;
import net.jxta.impl.util.pipe.reliable.FixedFlowControl;
import net.jxta.impl.util.pipe.reliable.OutgoingMsgrAdaptor;
import net.jxta.impl.util.pipe.reliable.ReliableInputStream;
import net.jxta.impl.util.pipe.reliable.ReliableOutputStream;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.OutputPipeEvent;
import net.jxta.pipe.OutputPipeListener;
import net.jxta.pipe.PipeID;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;

import java.io.IOException;
import java.io.InputStream;
import java.net.SocketTimeoutException;
import java.util.Collections;
import java.util.Iterator;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * JxtaBiDiPipe is a pair of UnicastPipe channels that implements a bidirectional pipe.
 * By default, JxtaBiDiPipe operates in reliable mode, unless otherwise specified,
 * in addition, messages must not exceed the Endpoint MTU size of 64K, exceed the
 * MTU will lead to unexpected behavior.
 * <p/>
 * It highly recommended that an application message listener is specified, not doing so, may
 * lead to message loss in the event the internal queue is overflowed.
 * <p/>
 * Sending messages vis {@link #sendMessage(Message)} from within a 
 * {@code PipeMsgListener} may result in a deadlock due to contention
 * between the sending and receiving portions of BiDi pipes. 
 * <p/>
 * JxtaBiDiPipe, whenever possible, will attempt to utilize direct tcp messengers,
 * which leads to improved performance.
 */
public class JxtaBiDiPipe implements PipeMsgListener, OutputPipeListener, ReliableInputStream.MsgListener {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(JxtaBiDiPipe.class.getName());

    private final static int MAXRETRYTIMEOUT = 120 * 1000;
    private PipeAdvertisement remotePipeAdv;
    private PeerAdvertisement remotePeerAdv;
    protected int timeout = 15 * 1000;
    protected int retryTimeout = 60 * 1000;
    protected int maxRetryTimeout = MAXRETRYTIMEOUT;
    protected int windowSize = 50;

    private ArrayBlockingQueue<PipeMsgEvent> queue = new ArrayBlockingQueue<PipeMsgEvent>(windowSize);
    protected PeerGroup group;
    protected PipeAdvertisement pipeAdv;
    protected PipeAdvertisement myPipeAdv;
    protected PipeService pipeSvc;
    protected InputPipe inputPipe;
    protected OutputPipe connectOutpipe;
    protected Messenger msgr;
    protected InputStream stream;
    protected final Object closeLock = new Object();
    protected final Object acceptLock = new Object();
    protected final Object finalLock = new Object();
    protected boolean closed = false;
    protected boolean bound = false;
    protected boolean dequeued = false;
    protected PipeMsgListener msgListener;
    protected PipeEventListener eventListener;
    protected PipeStateListener stateListener;
    protected Credential credential = null;
    protected boolean waiting;

    /**
     * If {@code true} then we are using the underlying end-to-end ACK reliable
     * layer to ensure that messages are received by the remote peer.
     */
    protected boolean isReliable = false;

    protected ReliableInputStream ris = null;
    protected ReliableOutputStream ros = null;

    /**
     * If {@code true} then we are using a reliable direct messenger to the
     * remote peer. We will assume that messages which are sent successfully
     * will be received successfully.
     */
    protected volatile boolean direct = false;
    protected OutgoingMsgrAdaptor outgoing = null;
    protected StructuredDocument credentialDoc = null;

    /**
     * Pipe close Event
     */
    public static final int PIPE_CLOSED_EVENT = 1;

    /**
     * Creates a bidirectional pipe
     *
     * @param group      group context
     * @param msgr       lightweight output pipe
     * @param pipe       PipeAdvertisement
     * @param isReliable Whether the connection is reliable or not
     * @param credDoc    Credential StructuredDocument
     * @param direct     indicates a direct messenger pipe
     * @throws IOException if an io error occurs
     */
    protected JxtaBiDiPipe(PeerGroup group, Messenger msgr, PipeAdvertisement pipe, StructuredDocument credDoc, boolean isReliable, boolean direct) throws IOException {
        if (msgr == null) {
            throw new IOException("Null Messenger");
        }
        this.direct = direct;
        this.group = group;
        this.pipeAdv = pipe;
        this.credentialDoc = credDoc != null ? credDoc : getCredDoc(group);
        this.pipeSvc = group.getPipeService();
        this.inputPipe = pipeSvc.createInputPipe(pipe, this);
        this.msgr = msgr;
        this.isReliable = isReliable;
        if (!direct) {
            createRLib();
        }
        setBound();
    }

    /**
     * Creates a new object with a default timeout of #timeout, and no reliability.
     *
     */
    public JxtaBiDiPipe() {
    }

    /**
     * Creates a bidirectional pipe.
     *
     * Attempts to create a bidirectional connection to remote peer within default
     * timeout of #timeout.
     *
     * @param group       group context
     * @param pipeAd      PipeAdvertisement
     * @param msgListener application PipeMsgListener
     * @throws IOException if an io error occurs
     */
    public JxtaBiDiPipe(PeerGroup group, PipeAdvertisement pipeAd, PipeMsgListener msgListener) throws IOException {
        connect(group, null, pipeAd, timeout, msgListener);
    }

    /**
     * Creates a bidirectional pipe.
     *
     * Attempts to create a bidirectional connection to remote peer within specified
     * timeout of #timeout.
     *
     * @param group       group context
     * @param timeout     The number of milliseconds within which the JxtaBiDiPipe must
     *                    be successfully created. An exception will be thrown if the pipe
     *                    cannot be created in the alotted time. A timeout value of {@code 0}
     *                    (zero) specifies an infinite timeout.
     * @param pipeAd      PipeAdvertisement
     * @param msgListener application PipeMsgListener
     * @throws IOException if an io error occurs
     */
    public JxtaBiDiPipe(PeerGroup group, PipeAdvertisement pipeAd, int timeout, PipeMsgListener msgListener) throws IOException {
        connect(group, null, pipeAd, timeout, msgListener);
    }

    /**
     * attempts to create a bidirectional connection to remote peer
     *
     * @param group       group context
     * @param pipeAd      PipeAdvertisement
     * @param timeout     The number of milliseconds within which the JxtaBiDiPipe must
     *                    be successfully created. An exception will be thrown if the pipe
     *                    cannot be created in the allotted time. A timeout value of {@code 0}
     *                    (zero) specifies an infinite timeout.
     * @param msgListener application PipeMsgListener
     * @param reliable    if true, the reliability is assumed
     * @throws IOException if an io error occurs
     */
    public JxtaBiDiPipe(PeerGroup group, PipeAdvertisement pipeAd, int timeout, PipeMsgListener msgListener, boolean reliable) throws IOException {
        connect(group, null, pipeAd, timeout, msgListener, reliable);
    }

    /**
     * Connect to a JxtaServerPipe with default timeout
     *
     * @param group  group context
     * @param pipeAd PipeAdvertisement
     * @throws IOException if an io error occurs
     */
    public void connect(PeerGroup group, PipeAdvertisement pipeAd) throws IOException {
        connect(group, pipeAd, timeout);
    }

    /**
     * Connects to a remote JxtaBiDiPipe
     *
     * @param group   group context
     * @param pipeAd  PipeAdvertisement
     * @param timeout timeout in ms, also reset object default timeout
     *                to that of timeout
     * @throws IOException if an io error occurs
     */
    public void connect(PeerGroup group, PipeAdvertisement pipeAd, int timeout) throws IOException {
        connect(group, null, pipeAd, timeout, null);
    }

    /**
     * Connects to a remote JxtaServerPipe
     *
     * @param group       group context
     * @param peerid      peer to connect to
     * @param pipeAd      PipeAdvertisement
     * @param timeout     timeout in ms, also reset object default timeout to that of timeout
     * @param msgListener application PipeMsgListener
     * @throws IOException if an io error occurs
     */
    public void connect(PeerGroup group, PeerID peerid, PipeAdvertisement pipeAd, int timeout, PipeMsgListener msgListener) throws IOException {
        connect(group, peerid, pipeAd, timeout, msgListener, isReliable);
    }

    /**
     * Connects to a remote JxtaServerPipe
     *
     * @param group       group context
     * @param peerid      peer to connect to
     * @param pipeAd      PipeAdvertisement
     * @param timeout     timeout in ms, also reset object default timeout to that of timeout
     * @param msgListener application PipeMsgListener
     * @param reliable    Reliable connection
     * @throws IOException if an io error occurs
     */
    public void connect(PeerGroup group, PeerID peerid, PipeAdvertisement pipeAd, int timeout, PipeMsgListener msgListener, boolean reliable) throws IOException {
        if (isBound()) {
            throw new IOException("Pipe already bound");
        }
        if (timeout <= 0) {
            throw new IllegalArgumentException("Invalid timeout :" + timeout);
        }

        this.pipeAdv = pipeAd;
        this.group = group;
        this.msgListener = msgListener;
        if (msgListener != null) {
            dequeued = true;
        }
        this.isReliable = reliable;
        pipeSvc = group.getPipeService();
        this.timeout = timeout;
        myPipeAdv = JxtaServerPipe.newInputPipe(group, pipeAd);
        this.inputPipe = pipeSvc.createInputPipe(myPipeAdv, this);
        this.credentialDoc = credentialDoc != null ? credentialDoc : getCredDoc(group);
        Message openMsg = createOpenMessage(group, myPipeAdv);

        // create the output pipe and send this message
        if (peerid == null) {
            pipeSvc.createOutputPipe(pipeAd, this);
        } else {
            pipeSvc.createOutputPipe(pipeAd, Collections.singleton(peerid), this);
        }
        try {
            synchronized (acceptLock) {
                // check connectOutpipe within lock to prevent a race with modification.
                if (connectOutpipe == null) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Waiting for " + timeout + " msec");
                    }
                    acceptLock.wait(timeout);
                }
            }
        } catch (InterruptedException ie) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Interrupted", ie);
            }
            Thread.interrupted();
            IOException exp = new IOException("Interrupted");
            exp.initCause(ie);
            throw exp;
        }
        if (connectOutpipe == null) {
            throw new IOException("connection timeout");
        }
        // send connect message
        waiting = true;
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending a backchannel message");
        }
        connectOutpipe.send(openMsg);
        // wait for the second op
        try {
            synchronized (finalLock) {
                if (waiting) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Waiting for " + timeout + " msec for back channel to be established");
                    }
                    finalLock.wait(timeout);
                    // Need to check for creation
                    if (msgr == null) {
                        throw new IOException("connection timeout");
                    }
                }
            }
        } catch (InterruptedException ie) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Interrupted", ie);
            }
            Thread.interrupted();
            IOException exp = new IOException("Interrupted");
            exp.initCause(ie);
            throw exp;
        }
        setBound();
        notifyListeners(PipeStateListener.PIPE_OPENED_EVENT);
    }

    /**
     * creates all the reliability objects
     */
    private void createRLib() {
        if (isReliable) {
            if (outgoing == null) {
                outgoing = new OutgoingMsgrAdaptor(msgr, retryTimeout);
            }
            if (ros == null) {
                ros = new ReliableOutputStream(outgoing, new FixedFlowControl(windowSize));
            }
            if (ris == null) {
                ris = new ReliableInputStream(outgoing, retryTimeout, this);
            }
        }
    }

    /**
     * Toggles reliability
     *
     * @param reliable Toggles reliability to reliable
     * @throws IOException if pipe is bound
     */
    public void setReliable(boolean reliable) throws IOException {
        if (isBound()) {
            throw new IOException("Can not set reliability after pipe is bound");
        }
        this.isReliable = reliable;
    }

    /**
     * Obtain the cred doc from the group object.
     *
     * @param group group context
     * @return The credDoc value
     */
    protected static StructuredDocument getCredDoc(PeerGroup group) {
        try {
            MembershipService membership = group.getMembershipService();
            Credential credential = membership.getDefaultCredential();

            if (credential != null) {
                return credential.getDocument(MimeMediaType.XMLUTF8);
            }
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to get credential", e);
            }
        }
        return null;
    }

    /**
     * get the remote credential doc
     *
     * @return Credential StructuredDocument
     */
    public StructuredDocument getCredentialDoc() {
        return credentialDoc;
    }

    /**
     * Sets the connection credential doc.
     * If no credentials are set, the default group credential are used.
     *
     * @param doc Credential StructuredDocument
     */
    public void setCredentialDoc(StructuredDocument doc) {
        this.credentialDoc = doc;
    }

    /**
     * Creates a connection request message
     *
     * @param group  group context
     * @param pipeAd pipe advertisement
     * @return the Message  object
     * @throws IOException if an io error occurs
     */
    protected Message createOpenMessage(PeerGroup group, PipeAdvertisement pipeAd) throws IOException {
        Message msg = new Message();
        PeerAdvertisement peerAdv = group.getPeerAdvertisement();

        if (credentialDoc == null) {
            credentialDoc = getCredDoc(group);
        }
        if (credentialDoc == null && pipeAd.getType().equals(PipeService.UnicastSecureType)) {
            throw new IOException("No credentials established to initiate a secure connection");
        }
        try {
            if (credentialDoc != null) {
                msg.addMessageElement(JxtaServerPipe.nameSpace,
                        new TextDocumentMessageElement(JxtaServerPipe.credTag, (XMLDocument) credentialDoc, null));
            }
            msg.addMessageElement(JxtaServerPipe.nameSpace,
                    new TextDocumentMessageElement(JxtaServerPipe.reqPipeTag,
                            (XMLDocument) pipeAd.getDocument(MimeMediaType.XMLUTF8), null));

            msg.addMessageElement(JxtaServerPipe.nameSpace,
                    new StringMessageElement(JxtaServerPipe.reliableTag, Boolean.toString(isReliable), null));

            msg.addMessageElement(JxtaServerPipe.nameSpace,
                    new StringMessageElement(JxtaServerPipe.directSupportedTag, Boolean.toString(true), null));

            msg.addMessageElement(JxtaServerPipe.nameSpace,
                    new TextDocumentMessageElement(JxtaServerPipe.remPeerTag,
                            (XMLDocument) peerAdv.getDocument(MimeMediaType.XMLUTF8), null));
            return msg;
        } catch (Throwable t) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "error getting element stream", t);
            }
            return null;
        }
    }

    /**
     * Sets the bound attribute of the JxtaServerPipe object
     */
    void setBound() {
        bound = true;
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Pipe Bound :true");
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
     * Returns an input stream for this socket.
     *
     * @return a stream for reading from this socket.
     * @throws IOException if an I/O error occurs when creating the
     *                     input stream.
     */
    public InputPipe getInputPipe() throws IOException {
        return inputPipe;
    }

    /**
     * Returns remote PeerAdvertisement
     *
     * @return remote PeerAdvertisement
     */
    public PeerAdvertisement getRemotePeerAdvertisement() {
        return remotePeerAdv;
    }

    /**
     * Returns remote PipeAdvertisement
     *
     * @return remote PipeAdvertisement
     */
    public PipeAdvertisement getRemotePipeAdvertisement() {
        return remotePipeAdv;
    }

    /**
     * Sets the remote PeerAdvertisement
     *
     * @param peer Remote PeerAdvertisement
     */
    protected void setRemotePeerAdvertisement(PeerAdvertisement peer) {
        this.remotePeerAdv = peer;
    }

    /**
     * Sets the remote PipeAdvertisement
     *
     * @param pipe PipeAdvertisement
     */
    protected void setRemotePipeAdvertisement(PipeAdvertisement pipe) {
        this.remotePipeAdv = pipe;
    }

    /**
     * Closes this pipe.
     *
     * @throws IOException if an I/O error occurs when closing this
     *                     socket.
     */
    public void close() throws IOException {
        sendClose();
        closePipe(false);
        bound = false;
    }

    protected void closePipe(boolean fastClose) throws IOException {
        // close both pipes
        synchronized (closeLock) {
            if (closed) {
                return;
            }
            closed = true;
            bound = false;
        }

        if (!fastClose && isReliable && !direct) {
            /*
             *  This implements linger!
             */
            long quitAt = System.currentTimeMillis() + timeout;
            while (true) {
                //FIXME hamada this does not loop
                if (ros == null || ros.getMaxAck() == ros.getSeqNumber()) {
                    // Nothing to worry about.
                    break;
                }

                // By default wait forever.
                long left = 0;

                // If timeout is not zero. Then compute the waiting time
                // left.
                if (timeout != 0) {
                    left = quitAt - System.currentTimeMillis();
                    if (left < 0) {
                        // Too late
                        sendClose();
                        throw new IOException("Close timeout");
                    }
                }

                try {
                    if (!ros.isQueueEmpty()) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Waiting for Output stream queue event");
                        }
                        ros.waitQueueEvent(left);
                    }
                    break;
                } catch (InterruptedException ie) {
                    // give up, then.
                    throw new IOException("Close interrupted");
                }
            }

            // We are initiating the close. We do not want to receive
            // anything more. So we can close the ris right away.
            ris.close();
        }

        if (isReliable && ros != null) {
            ros.close();
        }

        // close the pipe
        inputPipe.close();
        msgr.close();
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Pipe close complete");
        }
        notifyListeners(PIPE_CLOSED_EVENT);
    }

    private void notifyListeners(int event) {
        try {
            if (eventListener != null) {
                eventListener.pipeEvent(event);
            } else if (stateListener != null) {
                stateListener.stateEvent(this, event);
            }
        } catch (Throwable th) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "error during pipe event callback", th);
            }
        }
    }

    /**
     * Sets the inputPipe attribute of the JxtaBiDiPipe object
     *
     * @param inputPipe The new inputPipe value
     */
    protected void setInputPipe(InputPipe inputPipe) {
        this.inputPipe = inputPipe;
    }

    /**
     * {@inheritDoc}
     */
    public void pipeMsgEvent(PipeMsgEvent event) {
        Message message = event.getMessage();
        if (message == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Empty event");
            }
            return;
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Pipe message arrived");
        }

        MessageElement element;
        if (!bound) {
            // look for a remote pipe answer
            element = message.getMessageElement(JxtaServerPipe.nameSpace, JxtaServerPipe.remPipeTag);
            if (element != null) {
                // connect response
                try {
                    XMLDocument CredDoc = null;
                    XMLDocument remotePipeDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(element);

                    remotePipeAdv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(remotePipeDoc);
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Recevied a pipe Advertisement :" + remotePipeAdv.getName());
                    }

                    element = message.getMessageElement(JxtaServerPipe.nameSpace, JxtaServerPipe.remPeerTag);
                    if (element != null) {
                        XMLDocument remotePeerDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(element);

                        remotePeerAdv = (PeerAdvertisement) AdvertisementFactory.newAdvertisement(remotePeerDoc);
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Recevied an Peer Advertisement :" + remotePeerAdv.getName());
                        }
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning(" BAD connect response");
                        }
                        return;
                    }

                    element = message.getMessageElement(JxtaServerPipe.nameSpace, JxtaServerPipe.credTag);
                    if (element != null) {
                        CredDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(element);
                    }
                    if (pipeAdv.getType().equals(PipeService.UnicastSecureType) && (CredDoc == null || !checkCred(CredDoc))) {
                        // we're done here
                        if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                            LOG.severe("Missing remote credential doc");
                        }
                        return;
                    }

                    element = message.getMessageElement(JxtaServerPipe.nameSpace, JxtaServerPipe.reliableTag);
                    if (element != null) {
                        isReliable = Boolean.valueOf(element.toString());
                    }

                    boolean directSupported = false;
                    element = message.getMessageElement(JxtaServerPipe.nameSpace, JxtaServerPipe.directSupportedTag);
                    if (element != null) {
                        directSupported = Boolean.valueOf(element.toString());
                    }

                    if (directSupported) {
                        msgr = getDirectMessenger(group, remotePipeAdv, remotePeerAdv);
                        if (msgr != null) {
                            this.direct = true;
                        } else {
                            msgr = lightweightOutputPipe(group, remotePipeAdv, remotePeerAdv);
                        }
                    } else {
                        msgr = lightweightOutputPipe(group, remotePipeAdv, remotePeerAdv);
                    }

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Reliability set to :" + isReliable);
                    }
                    if (isReliable && !direct) {
                        createRLib();
                    }
                    synchronized (finalLock) {
                        waiting = false;
                        finalLock.notifyAll();
                    }
                } catch (IOException e) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "failed to process response message", e);
                    }
                }
                return;
            }
        }

        if (isReliable && !direct) {
            // let reliabilty deal with the message
            receiveMessage(message);
            return;
        }
        if (!hasClose(message)) {
            push(event);
        }
    }

    private boolean hasClose(Message message) {
        // look for close request
        MessageElement element = message.getMessageElement(JxtaServerPipe.nameSpace, JxtaServerPipe.closeTag);
        if (element != null) {
            try {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Recevied a pipe close request, closing pipes");
                }
                if (ros != null) {
                    ros.hardClose();
                }
                closePipe(false);
            } catch (IOException ie) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "failed during close", ie);
                }
            }
            return true;
        }
        return false;
    }

    private void receiveMessage(Message message) {
        Iterator<MessageElement> i = message.getMessageElements(Defs.NAMESPACE, Defs.MIME_TYPE_ACK);

        if (i.hasNext()) {
            if (ros != null) {
                ros.recv(message);
            }
            return;
        }

        i = message.getMessageElements(Defs.NAMESPACE, Defs.MIME_TYPE_BLOCK);
        if (i.hasNext()) {

            // It can happen that we receive messages for the input stream
            // while we have not finished creating it.
            try {
                synchronized (finalLock) {
                    while (waiting) {
                        finalLock.wait(timeout);
                    }
                }
            } catch (InterruptedException ie) {// ignored
            }

            if (ris != null) {
                ris.recv(message);
            }
        }
    }

    /**
     * Gets the Maximum Retry Timeout of the reliability layer
     *
     * @return The maximum retry Timeout value
     */
    public synchronized int getMaxRetryTimeout() {
        return maxRetryTimeout;
    }

    /**
     * Gets the Maximum Retry Timeout of the reliability layer
     *
     * @param maxRetryTimeout The new maximum retry timeout value
     * @throws IllegalArgumentException if maxRetryTimeout exceeds jxta platform maximum retry timeout
     */
    public synchronized void setMaxRetryTimeout(int maxRetryTimeout) {
        if (maxRetryTimeout <= 0 || maxRetryTimeout > MAXRETRYTIMEOUT) {
            throw new IllegalArgumentException(
                    "Invalid Maximum retry timeout :" + maxRetryTimeout + " Exceed Global maximum retry timeout :"
                            + MAXRETRYTIMEOUT);
        }
        this.maxRetryTimeout = maxRetryTimeout;
    }

    /**
     * Gets the Retry Timeout of the reliability layer
     *
     * @return The retry Timeout value
     */
    public synchronized int getRetryTimeout() {
        return retryTimeout;
    }

    /**
     * Sets the Retry Timeout of the underlying reliability layer
     * .
     * In reliable mode it is possible for this call to block
     * trying to obtain a lock on reliable input stream
     *
     * @param retryTimeout The new retry timeout value
     * @throws IOException if an I/O error occurs
     */
    public synchronized void setRetryTimeout(int retryTimeout) throws IOException {
        if (timeout <= 0) {
            throw new IllegalArgumentException("Invalid Socket timeout :" + retryTimeout);
        }
        this.retryTimeout = retryTimeout;
        if (outgoing != null) {
            outgoing.setTimeout(retryTimeout);
        }
    }

    /**
     * When in reliable mode, gets the Reliable library window size
     *
     * @return The windowSize value
     */
    public synchronized int getWindowSize() {
        return windowSize;
    }

    /**
     * When in reliable mode, sets the Reliable library window size
     *
     * @param windowSize The new window size value
     * @throws IOException if an I/O error occurs
     */
    public synchronized void setWindowSize(int windowSize) throws IOException {
        if (isBound()) {
            throw new IOException("Socket bound. Can not change the window size");
        }
        this.windowSize = windowSize;
    }

    /**
     * This method is invoked by the Reliablity library for each incoming data message
     *
     * @param message Incoming message
     */
    public void processIncomingMessage(Message message) {
        if (!hasClose(message)) {
            PipeMsgEvent event = new PipeMsgEvent(this, message, (PipeID) inputPipe.getPipeID());
            push(event);
        }
    }

    private void push(PipeMsgEvent event) {
        if (msgListener == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("push message onto queue");
            }
            queue.offer(event);
        } else {
            dequeue();
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("calling message listener");
            }
            msgListener.pipeMsgEvent(event);
        }

    }

    /**
     * Send a message
     * <p/>
     * <code>Messenger</code>
     *
     * @param msg Message to send to the remote side
     * @return true if message was successfully enqueued
     * @throws IOException if the underlying messenger breaks, either due to
     *                     a physical address change, reliability issue.
     * @see net.jxta.endpoint.Message
     */
    public boolean sendMessage(Message msg) throws IOException {
        if (isReliable && !direct) {
            int seqn = ros.send(msg);
            return (seqn > 0);
        } else {
            try {
                if (msgr instanceof TcpMessenger) {
                    ((TcpMessenger) msgr).sendMessageDirect(msg, null, null, true);
                    return true;
                } else {
                    return msgr.sendMessage(msg, null, null);
                }
            } catch (SocketTimeoutException io) {
                if (msgr instanceof TcpMessenger) {
                    ((TcpMessenger) msgr).sendMessageDirect(msg, null, null, true);
                    return true;
                } else {
                    return msgr.sendMessage(msg, null, null);
                }
            } catch (IOException io) {
                closePipe(true);
                IOException exp = new IOException("IO error occured during sendMessage()");
                exp.initCause(io);
                throw exp;

            }
        }
    }

    private void dequeue() {
        if (!dequeued && (null != msgListener)) {
            while (queue != null && !queue.isEmpty()) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("dequeing messages onto message listener");
                }
                try {
                    msgListener.pipeMsgEvent(queue.take());
                } catch (InterruptedException e) {
                    //ignored
                }
            }
            dequeued = false;
        }
    }

    /**
     * {@inheritDoc}
     */
    public void outputPipeEvent(OutputPipeEvent event) {
        OutputPipe op = event.getOutputPipe();

        if (op.getAdvertisement() == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("The output pipe has no internal pipe advertisement. Continueing anyway.");
            }
        }
        if (op.getAdvertisement() == null || pipeAdv.equals(op.getAdvertisement())) {
            synchronized (acceptLock) {
                // modify op within lock to prevent a race with the if.
                if (connectOutpipe == null) {
                    connectOutpipe = op;
                    // set to null to avoid closure
                    op = null;
                }
                acceptLock.notifyAll();
            }
            // Ooops one too many, we were too fast re-trying.
            if (op != null) {
                op.close();
            }

        } else {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Unexpected OutputPipe :" + op);
            }
        }
    }

    /**
     * A lightweight direct messenger output pipe constructor, note the return type
     * Since all the info needed is available, there's no need for to
     * use the pipe service to resolve the pipe we have all we need
     * to construct a messenger.
     *
     * @param group   group context
     * @param pipeAdv Remote Pipe Advertisement
     * @param peer    Remote Peer advertisement
     * @return Messenger
     */
    protected static Messenger getDirectMessenger(PeerGroup group, PipeAdvertisement pipeAdv, PeerAdvertisement peer) {
        // Get an endpoint messenger to that address
        if (pipeAdv.getType().equals(PipeService.PropagateType)) {
            throw new IllegalArgumentException("Invalid pipe type " + pipeAdv.getType());
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Creating a Direct Messenger");
        }

        if (pipeAdv.getType().equals(PipeService.UnicastType)) {
            EndpointService endpoint = group.getEndpointService();
            EndpointAddress pipeEndpoint = new EndpointAddress("jxta",
                                                       (peer.getPeerID().getUniqueValue()).toString(),
                                                       "PipeService",
                                                       pipeAdv.getPipeID().toString());
            return endpoint.getDirectMessenger(pipeEndpoint, peer, true);
        }
        return null;
    }

    /**
     * A lightweight output pipe constructor, note the return type
     * Since all the info needed is available, there's no need for to
     * use the pipe service to resolve the pipe we have all we need
     * to construct a messenger.
     *
     * @param group   group context
     * @param pipeAdv Remote Pipe Advertisement
     * @param peer    Remote Peer advertisement
     * @return Messenger
     */
    protected static Messenger lightweightOutputPipe(PeerGroup group, PipeAdvertisement pipeAdv, PeerAdvertisement peer) {

        EndpointService endpoint = group.getEndpointService();
        ID opId = pipeAdv.getPipeID();
        String destPeer = (peer.getPeerID().getUniqueValue()).toString();

        // Get an endpoint messenger to that address
        EndpointAddress addr;
        if (pipeAdv.getType().equals(PipeService.UnicastType)) {
            addr = new EndpointAddress("jxta", destPeer, "PipeService", opId.toString());
        } else if (pipeAdv.getType().equals(PipeService.UnicastSecureType)) {
            addr = new EndpointAddress("jxtatls", destPeer, "PipeService", opId.toString());
        } else {
            // not a supported type
            return null;
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Creating a lightweightOutputPipe()");
        }
        return endpoint.getMessenger(addr);
    }

    /**
     * Not implemented yet
     *
     * @param cred the credential document
     * @return always returns true
     */
    protected boolean checkCred(StructuredDocument cred) {
        // FIXME need to check credentials
        return true;
    }

    /**
     * Send a close message to the remote side
     */
    private void sendClose() {
        if (!direct && isReliable && ros.isClosed()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("ReliableOutputStream is already closed. Skipping close message");
            }
            return;
        }

        Message msg = new Message();
        msg.addMessageElement(JxtaServerPipe.nameSpace, new StringMessageElement(JxtaServerPipe.closeTag, "close", null));
        try {
            sendMessage(msg);
            // ros will not take any new message, now.
            if (!direct && ros != null) {
                ros.close();
            }
        } catch (IOException ie) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.SEVERE, "failed during close", ie);
            }
        }
    }

    /**
     * Returns the message listener for this pipe
     *
     * @return PipeMsgListener
     * @deprecated use getMessageListener instead
     */
    @Deprecated
    public PipeMsgListener getListener() {
        return getMessageListener();
    }

    /**
     * Returns the message listener for this pipe
     *
     * @return PipeMsgListener
     */
    public PipeMsgListener getMessageListener() {
        return msgListener;
    }

    /**
     * Sets message listener for a pipe spawned by the JxtaServerPipe.
     * There is a window where a message could arrive prior to listener being
     * registered therefore a message queue is created to queue messages, once
     * a listener is registered these messages will be dequeued by calling the
     * listener until the queue is empty
     *
     * @param msgListener New value of property listener.
     * @deprecated use setMessageListener instead
     */
    @Deprecated
    public void setListener(PipeMsgListener msgListener) {
        setMessageListener(msgListener);
    }

    /**
     * Sets message listener for a pipe spawned by the JxtaServerPipe.
     * There is a window where a message could arrive prior to listener being
     * registered therefore a message queue is created to queue messages, once
     * a listener is registered these messages will be dequeued by calling the
     * listener until the queue is empty.
     * <p/>
     * Sending messages vis {@link #sendMessage(Message)} from within a 
     * {@code PipeMsgListener} may result in a deadlock due to contention
     * between the sending and receiving portions of BiDi pipes. 
     *
     * @param msgListener New value of property listener.
     */
    public void setMessageListener(PipeMsgListener msgListener) {
        this.msgListener = msgListener;
        // if there are messages enqueued then dequeue them onto the msgListener
        dequeue();
    }

    /**
     * Sets a Pipe event listener, set listener to null to unset the listener
     *
     * @param eventListener New value of property listener.
     * @deprecated use setPipeEventListener instead
     */
    @Deprecated
    public void setListener(PipeEventListener eventListener) {
        setPipeEventListener(eventListener);
    }

    /**
     * Sets a Pipe event listener, set listener to null to unset the listener
     *
     * @param eventListener New value of property listener.
     */
    public void setPipeEventListener(PipeEventListener eventListener) {
        this.eventListener = eventListener;
    }

    /**
     * Returns the Pipe event listener for this pipe
     *
     * @return PipeMsgListener
     */
    public PipeEventListener getPipeEventListener() {
        return eventListener;
    }

    /**
     * Sets a Pipe state listener, set listener to null to unset the listener
     *
     * @param stateListener New value of property listener.
     */
    public void setPipeStateListener(PipeStateListener stateListener) {
        this.stateListener = stateListener;
    }

    /**
     * Returns the Pipe state listener for this pipe
     *
     * @return PipeMsgListener
     */
    public PipeStateListener getPipeStateListener() {
        return stateListener;
    }

    /**
     * Gets a message from the queue. If no Object is immediately available,
     * then wait the specified amount of time for a message to be inserted.
     *
     * @param timeout Amount of time to wait in milliseconds for an object to
     *                be available. Per Java convention, a timeout of zero (0) means wait an
     *                infinite amount of time. Negative values mean do not wait at all.
     * @return The next message in the queue. if a listener is registered calls
     *         to this method will return null
     * @throws InterruptedException if the operation is interrupted before
     *                              the timeout interval is completed.
     */
    public Message getMessage(int timeout) throws InterruptedException {
        if (queue == null || msgListener != null) {
            return null;
        } else {
            PipeMsgEvent ev = queue.poll(timeout, TimeUnit.MILLISECONDS);
            if (ev != null) {
                return ev.getMessage();
            } else {
                return null;
            }
        }
    }

    /**
     * Returns the Assigned PipeAdvertisement
     *
     * @return the Assigned PipeAdvertisement
     */
    public PipeAdvertisement getPipeAdvertisement() {
        return pipeAdv;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Closes the JxtaBiDiPipe.
     */
    @Override
    protected synchronized void finalize() throws Throwable {
        if (!closed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("JxtaBiDiPipe is being finalized without being previously closed. This is likely a users bug.");
            }
            close();
        }
        super.finalize();
    }
}
