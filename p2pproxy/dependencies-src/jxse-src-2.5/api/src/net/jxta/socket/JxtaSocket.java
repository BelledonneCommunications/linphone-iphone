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
package net.jxta.socket;

import net.jxta.credential.Credential;
import net.jxta.document.*;
import net.jxta.endpoint.*;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.util.pipe.reliable.FixedFlowControl;
import net.jxta.impl.util.pipe.reliable.Outgoing;
import net.jxta.impl.util.pipe.reliable.OutgoingMsgrAdaptor;
import net.jxta.impl.util.pipe.reliable.ReliableInputStream;
import net.jxta.impl.util.pipe.reliable.ReliableOutputStream;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.pipe.*;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.util.Collections;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * JxtaSocket is a sub-class of java.net.socket, and should be used like a java.net.Socket.
 * Key differences to keep in mind are the following :
 * </p>
 * - JxtaSocket does not implement Nagle's algorithm, therefore at end of a data frame a flush must invoked to enure all
 * buffered data is packaged and transmitted.
 * - JxtaSocket does not implement keep-alive, therefore it is possible the underlaying messengers to be closed due to
 * lack of inactivity, which manifests in a short latency, while the messenger are recreated. This limitation should cease
 * to exist as soon the inactivity logic is removed.
 *
 */
public class JxtaSocket extends Socket implements PipeMsgListener, OutputPipeListener {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(JxtaSocket.class.getName());
    private final static int MAXRETRYTIMEOUT = 120 * 1000;
    private final static int DEFAULT_TIMEOUT = 15 * 1000;

    /**
     * Default size for output buffers. Only used when we do not know the MTU
     * size for messengers sending to the remote peer and as an upper bounds
     * should the MTU size be really huge.
     */
    private final static int DEFAULT_OUTPUT_BUFFER_SIZE = 256 * 1024;

    /**
     * If true then this peer initiated the connection.
     */
    private boolean initiator = false;

    /**
     * The PeerGroup
     */
    protected PeerGroup group;

    /**
     * Pipe Advertisement of the well known pipe.
     */
    protected PipeAdvertisement pipeAdv;

    /**
     * Pipe Advertisement of local ephemeral pipe.
     */
    protected PipeAdvertisement localEphemeralPipeAdv;

    /**
     * The input pipe for our ephemeral pipe. We will receive all messages on this pipe.
     */
    protected InputPipe localEphemeralPipeIn;

    /**
     * Pipe Advertisement of it's ephemeral pipe.
     */
    protected PipeAdvertisement remoteEphemeralPipeAdv;

    /**
     * The Messenger we use to
     */
    protected Messenger remoteEphemeralPipeMsgr;

    protected PipeService pipeSvc;

    /**
     * The peer id of the peer we are connecting to or {@code null} if we are
     * willing to connect to any peer.
     */
    protected PeerID remotePeerID;

    /**
     * Used to negotiate connection parameters
     */
    protected OutputPipe connectOutpipe;

    /**
     * The timeout of the read() of this socket's input stream
     */
    private int soTimeout = 0;

    /**
     * timeout for connect and close
     */
    protected long timeout = 60 * 1000;

    /**
     * retry timeout in millisecods
     */
    protected int retryTimeout = 60 * 1000;

    /**
     * maximum retry timeout allowed
     */
    protected int maxRetryTimeout = MAXRETRYTIMEOUT;

    /**
     * retry window size
     */
    protected int windowSize = 20;

    /**
     * Lock for output pipe resolution.
     */
    protected final Object pipeResolveLock = new Object();

    /**
     * Lock for ephemeral pipe connect states.
     */
    protected final Object socketConnectLock = new Object();

    /**
     * Lock for closing states.
     */
    protected final Object closeLock = new Object();

    /*
     *used to determine whether to wait for an ack
     */
    private boolean closeAckReceived = false;

    /**
     * If {@code true} then this socket has been closed and can no longer be used.
     */
    protected volatile boolean closed = false;

    /**
     * If {@code true} then we believer our end of the connection is open.
     */
    protected boolean bound = false;

    /**
     * If {@code true} then we believe the remote peer currently has this socket open.
     */
    protected boolean connected = false;

    /**
     * Credential of the remote peer.
     */
    protected Credential remoteCredential = null;

    /**
     * Our credential that we provide to the remote peer.
     */
    protected Credential localCredential = null;

    /**
     * The remote peer advertisement.
     */
    private PeerAdvertisement remotePeerAdv = null;

    /**
     * If {@code true} then the socket is a stream socket otherwise it is a datagram socket.
     */
    protected boolean isReliable = true;

    /**
     *  If {@code true} then the output stream has been shutdown. All attempts 
     *  to write to the socket will fail. This socket can no longer be used to 
     *  send data though it may remain capable of receiving data.
     */
    private boolean outputShutdown = false;
    
    /**
     *  If {@code true} then the input stream has been shutdown. All attempts 
     *  to read from the socket will fail. This socket can no longer be used to 
     *  receive data though it may remain capable of sending data.
     */
    private boolean inputShutdown = false;

    /**
     *  Used for sending all messages by the reliable output and input streams.
     */
    protected Outgoing outgoing = null;
    
    /**
     *  The reliable input stream we use for receiving data if 
     *  {@link #isReliable} is {@code true}.
     */
    protected ReliableInputStream ris = null;

    /**
     *  The reliable output stream we use for sending data if 
     *  {@link #isReliable} is {@code true}.
     */
    protected ReliableOutputStream ros = null;

    /**
     *  The unreliable input stream we use for receiving data if 
     *  {@link #isReliable} is {@code false}.
     */
    protected JxtaSocketInputStream nonReliableInputStream = null;

    /**
     *  The unreliable output stream we use for sending data if 
     *  {@link #isReliable} is {@code false}.
     */
    protected JxtaSocketOutputStream nonReliableOutputStream = null;

    /**
     * The size of the output buffers to use. If not set this defaults to the
     * MTU size of the messenger to the remote peer.
     */
    private int outputBufferSize = -1;

    /**
     * This constructor does not establish a connection. Use this constructor
     * when altering the default parameters, and options of the socket.
     * <p/>
     * By default connections are reliable, and the default timeout is 60
     * seconds. To alter a connection a call to create(false) changes the
     * connection to an unreliable one.
     */
    public JxtaSocket() {}

    /**
     * This constructor is used by JxtaServer socket for creating JxtaSocket
     * instances in response to incoming connections.
     *
     * @param group               group context
     * @param pipeAdv             The original PipeAdvertisement
     * @param localCredential        Our credential.
     * @param remoteEphemeralPipeAdv the phemeral pipe advertisement
     * @param  remotePeerAdv          remote peer advertisement
     * @param remoteCredential       The remote peer's credential.
     * @param isReliable          {@code true} for reliable stream connection or
     *                            {@code false} for unreliable stream connection.
     * @throws IOException if an io error occurs
     */
    protected JxtaSocket(PeerGroup group, PipeAdvertisement pipeAdv, PipeAdvertisement remoteEphemeralPipeAdv, PeerAdvertisement remotePeerAdv, Credential localCredential, Credential remoteCredential, boolean isReliable) throws IOException {

        this.initiator = false;
        this.group = group;
        this.pipeAdv = pipeAdv;
        this.remoteEphemeralPipeAdv = remoteEphemeralPipeAdv;
        this.localEphemeralPipeAdv = newEphemeralPipeAdv(pipeAdv);
        this.remotePeerAdv = remotePeerAdv;
        this.remotePeerID = remotePeerAdv.getPeerID();
        this.localCredential = localCredential;
        this.remoteCredential = remoteCredential;
        this.isReliable = isReliable;

        pipeSvc = group.getPipeService();
        bind();
        connect();

        Message connectResponse = createConnectMessage(group, localEphemeralPipeAdv, localCredential, isReliable, initiator);

        remoteEphemeralPipeMsgr.sendMessage(connectResponse);
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("New socket : " + this);
        }
    }

    /**
     * Create a JxtaSocket connected to the give JxtaSocketAddress.
     *
     * @param address JxtaSocketAddress to connect to
     * @throws IOException if an io error occurs
     */
    public JxtaSocket(SocketAddress address) throws IOException {
        connect(address, DEFAULT_TIMEOUT);
    }

    /**
     * Create a JxtaSocket to any node listening on pipeAdv
     *
     * @param group   group context
     * @param pipeAdv PipeAdvertisement
     * @throws IOException if an io error occurs
     */
    public JxtaSocket(PeerGroup group, PipeAdvertisement pipeAdv) throws IOException {
        connect(group, pipeAdv);
    }

    /**
     * Create a JxtaSocket to the given JxtaSocketAddress, within the timeout
     * specified in milliseconds.
     *
     * @param address JxtaSocket address to connect to
     * @param timeout The number of milliseconds within which the socket must
     *                be successfully created. An exception will be thrown if the socket
     *                cannot be created in the allotted time. A timeout value of {@code 0}
     *                (zero) specifies an infinite timeout.
     * @throws IOException            For failures in creating the socket.
     * @throws SocketTimeoutException If the socket cannot be created before
     *                                the timeout occurs.
     */
    public JxtaSocket(SocketAddress address, int timeout) throws IOException {
        connect(address, timeout);
    }

    /**
     * Create a JxtaSocket to any peer listening on pipeAdv this attempts
     * establish a connection to specified pipe within the context of the
     * specified group within timeout specified in milliseconds.
     *
     * @param group   group context
     * @param pipeAdv PipeAdvertisement
     * @param timeout The number of milliseconds within which the socket must
     *                be successfully created. An exception will be thrown if the socket
     *                cannot be created in the allotted time. A timeout value of {@code 0}
     *                (zero) specifies an infinite timeout.
     * @throws IOException            if an io error occurs
     * @throws SocketTimeoutException If the socket cannot be created before
     *                                the timeout occurs.
     */
    public JxtaSocket(PeerGroup group, PipeAdvertisement pipeAdv, int timeout) throws IOException {
        connect(group, pipeAdv, timeout);
    }

    /**
     * Create a JxtaSocket to any peer listening on pipeAdv
     * this attempts establish a connection to specified
     * pipe within a context of <code>group</code> and within the timeout specified in milliseconds
     *
     * @param group   group context
     * @param peerid  node to connect to
     * @param pipeAdv PipeAdvertisement
     * @param timeout The number of milliseconds within which the socket must
     *                be successfully created. An exception will be thrown if the socket
     *                cannot be created in the allotted time. A timeout value of {@code 0}
     *                (zero) specifies an infinite timeout.
     * @throws IOException            For failures in creating the socket.
     * @throws SocketTimeoutException If the socket cannot be created before
     *                                the timeout occurs.
     */
    public JxtaSocket(PeerGroup group, PeerID peerid, PipeAdvertisement pipeAdv, int timeout) throws IOException {
        connect(group, peerid, pipeAdv, timeout);
    }

    /**
     * Create a JxtaSocket to the given JxtaSocketAddress, within the timeout
     * specified in milliseconds. The JxtaSocket can be reliable (stream) or
     * not (datagram). If you want to use a SocketAddress in the constructor,
     * this is the preferred method. Either that, or use JxtaSocket(), followed
     * by create(boolean) to turn on reliability, followed by
     * connect(SocketAddress, int) or connect(SocketAddress) to make the
     * connection.
     *
     * @param address  JxtaSocket address to connect to
     * @param timeout  The number of milliseconds within which the socket must
     *                 be successfully created. An exception will be thrown if the socket
     *                 cannot be created in the allotted time. A timeout value of {@code 0}
     *                 (zero) specifies an infinite timeout.
     * @param reliable {@code true} for reliable stream connection or
     *                 {@code false} for unreliable stream connection.
     * @throws IOException            For failures in creating the socket.
     * @throws SocketTimeoutException If the socket cannot be created before
     *                                the timeout occurs.
     */
    public JxtaSocket(SocketAddress address, int timeout, boolean reliable) throws IOException {
        this.isReliable = reliable;
        connect(address, timeout);
    }

    /**
     * Create a JxtaSocket to any peer listening on pipeAdv
     * this attempts establish a connection to specified
     * pipe within a context of <code>group</code> and within the timeout specified in milliseconds
     *
     * @param group    group context
     * @param peerid   node to connect to
     * @param pipeAdv  PipeAdvertisement
     * @param timeout  The number of milliseconds within which the socket must
     *                 be successfully created. An exception will be thrown if the socket
     *                 cannot be created in the allotted time. A timeout value of {@code 0}
     *                 (zero) specifies an infinite timeout.
     * @param reliable {@code true} for reliable stream connection or
     *                 {@code false} for unreliable stream connection.
     * @throws IOException            For failures in creating the socket.
     * @throws SocketTimeoutException If the socket cannot be created before
     *                                the timeout occurs.
     */
    public JxtaSocket(PeerGroup group, PeerID peerid, PipeAdvertisement pipeAdv, int timeout, boolean reliable) throws IOException {

        this.isReliable = reliable;
        connect(group, peerid, pipeAdv, timeout);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void finalize() throws Throwable {
        if (!closed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("JxtaSocket is being finalized without being previously closed. This is likely a users bug.");
            }
        }
        close();
        super.finalize();
    }

    /**
     * Creates either a stream or a datagram socket.
     *
     * @param reliable {@code true} for reliable stream connection or {@code false} for unreliable stream connection.
     * @throws IOException if an I/O error occurs while creating the socket.
     * @deprecated Unreliable mode is being removed. Use JxtaBiDiPipe instead.
     */
    @Deprecated
    public void create(boolean reliable) throws IOException {
        if (isBound()) {
            throw new IOException("Socket already bound, it is not possible to change connection type");
        }
        this.isReliable = reliable;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Unsupported operation, an IOException will be thrown.
     *
     * @throws IOException Thrown in all cases as this operation is not supported.
     */
    @Override
    public void bind(SocketAddress address) throws IOException {
        throw new IOException("Unsupported operation, use java.net.Socket instead");
    }

    /**
     * {@inheritDoc}
     * <p/>
     * The default connect timeout of 60 seconds is used If SocketAddress is not an instance of JxtaSocketAddress, an
     * IOException will be thrown.
     */
    @Override
    public void connect(SocketAddress address) throws IOException {
        connect(address, DEFAULT_TIMEOUT);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * If SocketAddress is not an instance of JxtaSocketAddress, an IOException will be thrown.
     */
    @Override
    public void connect(SocketAddress address, int timeout) throws IOException {
        if (!(address instanceof JxtaSocketAddress)) {
            throw new IOException("Subclass of SocketAddress not supported. Use JxtaSocketAddress instead.");
        }
        JxtaSocketAddress socketAddress = (JxtaSocketAddress) address;
        PeerGroup pg = PeerGroup.globalRegistry.lookupInstance(socketAddress.getPeerGroupId());

        if (pg == null) {
            throw new IOException("Can't connect socket in PeerGroup with id " + socketAddress.getPeerGroupId()
                    + ". No running instance of the group is registered.");
        }
        connect(pg.getWeakInterface(), socketAddress.getPeerId(), socketAddress.getPipeAdv(), timeout);
        pg.unref();
    }

    /**
     * Connects to a JxtaServerSocket on any peer within the default timeout of 60 seconds
     *
     * @param group   group context
     * @param pipeAdv PipeAdvertisement
     * @throws IOException if an io error occurs
     */
    public void connect(PeerGroup group, PipeAdvertisement pipeAdv) throws IOException {
        connect(group, pipeAdv, DEFAULT_TIMEOUT);
    }

    /**
     * Connects to a JxtaServerSocket on any peer within a timeout specified in milliseconds
     *
     * @param group   group context
     * @param pipeAdv PipeAdvertisement
     * @param timeout in milliseconds
     * @throws IOException if an io error occurs
     */
    public void connect(PeerGroup group, PipeAdvertisement pipeAdv, int timeout) throws IOException {
        connect(group, null, pipeAdv, timeout);
    }

    /**
     * Connects to a JxtaServerSocket on a specific peer within a timeout specified in milliseconds
     *
     * @param group   group context
     * @param peerid  peer to connect to
     * @param pipeAdv PipeAdvertisement
     * @param timeout timeout in milliseconds
     * @throws IOException if an io error occurs
     */
    public void connect(PeerGroup group, PeerID peerid, PipeAdvertisement pipeAdv, int timeout) throws IOException {
        if (PipeService.PropagateType.equals(pipeAdv.getType())) {
            throw new IOException("Propagate pipe advertisements are not supported");
        }

        if (timeout < 0) {
            throw new IllegalArgumentException("timeout may not be negative");
        }

        this.initiator = true;
        this.group = group;
        this.remotePeerID = peerid;
        this.pipeAdv = pipeAdv;
        this.localEphemeralPipeAdv = newEphemeralPipeAdv(pipeAdv);
        this.timeout = (timeout == 0) ? Long.MAX_VALUE : timeout;

        pipeSvc = group.getPipeService();
        bind();
        Message openMsg = createConnectMessage(group, localEphemeralPipeAdv, localCredential, isReliable, initiator);
        long connectTimeoutAt = System.currentTimeMillis() + timeout;

        if (connectTimeoutAt < timeout) {
            // ensure no overflow
            connectTimeoutAt = Long.MAX_VALUE;
        }

        // Create the output pipe and send this message. Need to retry the call
        // to createOutputPipe. If there is no rendezvous yet and the
        // destination is not reachable by mcast, then createOutputPipe has no
        // effect.  We repeat it with exponential delays.
        if (peerid == null) {
            pipeSvc.createOutputPipe(pipeAdv, this);
        } else {
            pipeSvc.createOutputPipe(pipeAdv, Collections.singleton(peerid), this);
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.log(Level.FINE, "Beginning Output Pipe Resolution. " + this);
        }

        // Wait for the pipe resolution.
        synchronized (pipeResolveLock) {
            while (connectOutpipe == null) {
                try {
                    long waitFor = connectTimeoutAt - System.currentTimeMillis();

                    if (waitFor <= 0) {
                        // too late
                        break;
                    }
                    if (connectOutpipe == null) {
                        // in case the pipe is resolved
                        pipeResolveLock.wait(waitFor);
                    }
                } catch (InterruptedException ie) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Interrupted", ie);
                    }
                    Thread.interrupted();
                    SocketException exp = new SocketException("Connect Interrupted");
                    exp.initCause(ie);
                    throw exp;
                }
            }
        }

        if (connectOutpipe == null) {
            throw new SocketTimeoutException("Connection (resolution) timeout");
        }

        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Sending connect message. " + this);
            }

            // send connect message
            connectOutpipe.send(openMsg);

            // wait for the connect response.
            synchronized (socketConnectLock) {
                while (!isConnected()) {
                    try {
                        long waitFor = connectTimeoutAt - System.currentTimeMillis();
                        if (waitFor <= 0) {
                            // too late
                            break;
                        }
                        socketConnectLock.wait(waitFor);
                    } catch (InterruptedException ie) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Interrupted", ie);
                        }
                        Thread.interrupted();
                        SocketException exp = new SocketException("Connect Interrupted");
                        exp.initCause(ie);
                        throw exp;
                    }
                }
            }
        } finally {
            connectOutpipe.close();
            connectOutpipe = null;
        }

        if (!isConnected()) {
            throw new SocketTimeoutException("Connection timeout (connect)");
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("New socket connection : " + this);
        }
    }

    /**
     * obtain the cred doc from the group object
     *
     * @param group the group context
     * @return The <code>Credential</code> value
     */
    protected static Credential getDefaultCredential(PeerGroup group) {
        try {
            MembershipService membership = group.getMembershipService();
            return membership.getDefaultCredential();
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
    public Credential getCredentialDoc() {
        try {
            return remoteCredential;
        } catch (Exception failure) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to generate credential document ", failure);
            }
            return null;
        }
    }

    /**
     * Sets our credential to be used by this socket connection. If no
     * credentials are set, the default group credential will be used.
     *
     * @param localCredential The credential to be used for connection responses
     *                     or <tt>null</tt> if the default credential is to be used.
     */
    public void setCredential(Credential localCredential) {
        if (localCredential == null) {
            this.localCredential = localCredential;
        } else {
            try {
                MembershipService membership = group.getMembershipService();
                this.localCredential = membership.getDefaultCredential();
            } catch (Exception failed) {
                this.localCredential = null;
            }
        }
    }

    /**
     * Create a connection request/response message
     *
     * @param group      The group in which the socket is being used.
     * @param pipeAdv    Advertisement for our ephemeral pipe.
     * @param credential Our credential or null if default credential is to
     *                   be used.
     * @param isReliable The socket is to be reliable (stream).
     * @param initiator  indicates initiator
     * @return The message.
     * @throws IOException if an I/O error occurs
     */
    protected Message createConnectMessage(PeerGroup group, PipeAdvertisement pipeAdv, Credential credential, boolean isReliable, boolean initiator) throws IOException {

        Message msg = new Message();

        if (credential == null) {
            credential = getDefaultCredential(group);
        }

        if ((credential == null) && PipeService.UnicastSecureType.equals(pipeAdv.getType())) {
            throw new IOException("Credentials must be established to initiate a secure connection.");
        }

        if (credential != null) {
            try {
                XMLDocument credDoc = (XMLDocument) credential.getDocument(MimeMediaType.XMLUTF8);

                msg.addMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE,
                        new TextDocumentMessageElement(JxtaServerSocket.credTag, credDoc, null));
            } catch (Exception failed) {
                IOException failure = new IOException("Could not generate credential element.");
                failure.initCause(failed);
                throw failure;
            }
        }

        msg.addMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE,
                new TextDocumentMessageElement(initiator ? JxtaServerSocket.reqPipeTag : JxtaServerSocket.remPipeTag,
                (XMLDocument) pipeAdv.getDocument(MimeMediaType.XMLUTF8), null));

        msg.addMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE,
                new TextDocumentMessageElement(JxtaServerSocket.remPeerTag,
                (XMLDocument) group.getPeerAdvertisement().getDocument(MimeMediaType.XMLUTF8), null));

        msg.addMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE,
                new StringMessageElement(JxtaServerSocket.streamTag, Boolean.toString(isReliable), null));

        return msg;
    }

    /**
     * Create a pipe advertisement for an ephemeral pipe (w/random pipe ID) from an existing pipe advertisement.
     * The specified pipe adveritsement is only used for the name and type
     *
     * @param pipeAdv to get the basename and type from
     * @return A new pipe advertisement for an ephemeral pipe.
     */
    protected static PipeAdvertisement newEphemeralPipeAdv(PipeAdvertisement pipeAdv) {
        PipeAdvertisement adv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(PipeAdvertisement.getAdvertisementType());
        PeerGroupID gid = (PeerGroupID) ((PipeID) pipeAdv.getPipeID()).getPeerGroupID();
        adv.setPipeID(IDFactory.newPipeID(gid));
        adv.setName(pipeAdv.getName() + ".remote");
        adv.setType(pipeAdv.getType());
        return adv;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isBound() {
        return bound;
    }

    /**
     * Sets whether this socket is currently bound or not. A socket is considered bound if the local resources required
     * in order to interact with a remote peer are allocated and open.
     *
     * @param boundState The new bound state.
     */
    private void setBound(boolean boundState) {
        bound = boundState;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isConnected() {
        return connected;
    }

    /**
     * Sets whether this socket is currently connected or not. A socket is
     * considered connected if is believed that the Socket's remote peer has
     * the resources required in order to interact with a local peer (us)
     * allocated and open.
     *
     * @param connectedState The new connected state.
     */
    private void setConnected(boolean connectedState) {
        connected = connectedState;
    }

    /**
     * Opens our ephemeral input pipe enabling us to receive messages.
     *
     * @throws IOException Thrown for errors in creating the input pipe.
     */
    private void bind() throws IOException {
        this.localEphemeralPipeIn = pipeSvc.createInputPipe(localEphemeralPipeAdv, this);
        // The socket is bound now.
        setBound(true);
    }

    /**
     *  Create an appropriate Outgoing Adaptor. This method exists primarily
     *  so that sub-classes can substitute a different Outgoing sub-class.
     *
     *  @param msgr The messenger to be wrapped.
     *  @param timeout The timeout value;
     *  @return Outgoing The messenger wrapped in an appropriate adaptor.
     */
    protected Outgoing makeOutgoing(Messenger msgr, long timeout) {
        return new OutgoingMsgrAdaptor(msgr, (int) timeout);
    }
            
    /**
     * Opens the ephemeral output pipe for the remote peer. Also opens the
     * input and output streams. (delaying adds complexity).
     *
     * @throws IOException Thrown for errors in opening resources.
     */
    private void connect() throws IOException {

        remoteEphemeralPipeMsgr = lightweightOutputPipe(group, remoteEphemeralPipeAdv, remotePeerAdv);

        if (remoteEphemeralPipeMsgr == null) {
            throw new IOException("Could not create messenger back to connecting peer");
        }

        // Force the buffer size smaller if user set it too high.
        if (remoteEphemeralPipeMsgr.getMTU() < outputBufferSize) {
            outputBufferSize = Math.min((int) remoteEphemeralPipeMsgr.getMTU(), DEFAULT_OUTPUT_BUFFER_SIZE);
        }

        if (outputBufferSize == -1) {
            outputBufferSize = Math.min((int) remoteEphemeralPipeMsgr.getMTU(), DEFAULT_OUTPUT_BUFFER_SIZE);
        }

        // Force the creation of the inputStream now. Waiting until someone
        // calls getInputStream() would likely cause us to drop messages.
        if (isReliable) {
            outgoing = makeOutgoing(remoteEphemeralPipeMsgr, retryTimeout);
            ris = new ReliableInputStream(outgoing, soTimeout);
            ros = new ReliableOutputStream(outgoing, new FixedFlowControl(windowSize));
            try {
                ros.setSendBufferSize(outputBufferSize);
            } catch (IOException ignored) {// it's only a preference...
            }
        } else {
            nonReliableInputStream = new JxtaSocketInputStream(this, windowSize);
            nonReliableOutputStream = new JxtaSocketOutputStream(this, outputBufferSize);
        }

        // the socket is now connected!
        setConnected(true);
    }

    /**
     * Returns the internal output stream buffer size
     *
     * @return the internal buffer size.
     * @deprecated Use the standard {@link #getSendBufferSize()} method instead.
     */
    @Deprecated
    public int getOutputStreamBufferSize() {
        return (outputBufferSize == -1) ? DEFAULT_OUTPUT_BUFFER_SIZE : outputBufferSize;
    }

    /**
     * Sets the internal output stream buffer size.
     *
     * @param size The internal buffer size.
     * @throws IOException if an I/O error occurs
     * @deprecated Use the standard {@link #setSendBufferSize(int)} method instead.
     */
    @Deprecated
    public void setOutputStreamBufferSize(int size) throws IOException {
        setSendBufferSize(size);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public InputStream getInputStream() throws IOException {
        checkState();

        if (isInputShutdown()) {
            throw new SocketException("Input already shutdown.");
        }

        if (isReliable) {
            return ris;
        } else {
            return nonReliableInputStream;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public OutputStream getOutputStream() throws IOException {
        checkState();
        if (isOutputShutdown()) {
            throw new SocketException("Output already shutdown.");
        }
        return isReliable ? ros : nonReliableOutputStream;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * We hard-close both the input and output streams. Nothing can be
     * written or read to the socket after this method. Any queued incoming
     * data is discarded. Any additional incoming messages will be ACKed but
     * their content will be discarded. We will attempt to send any data which
     * has already been written to the OutputStream.
     * <p/>
     * Once the output queue is empty we will send a close message to tell
     * the remote side that no more data is coming.
     * <p/>
     * This is the only method in this class which is {@code synchronized}.
     * All others use internal synchronization.
     */
    @Override
    public synchronized void close() throws IOException {
        try {
            synchronized (closeLock) {
                long closeEndsAt = System.currentTimeMillis() + timeout;

                if (closeEndsAt < timeout) {
                    closeEndsAt = Long.MAX_VALUE;
                }
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Closing " + this + " timeout=" + timeout + "ms.");
                }
                if (closed) {
                    return;
                }

                closed = true;
                shutdownOutput();
                shutdownInput();
                while (isConnected()) {
                    long closingFor = closeEndsAt - System.currentTimeMillis();

                    if (closingFor <= 0) {
                        break;
                    }

                    if (isReliable) {
                        try {
                            if (ros.isQueueEmpty()) {
                                // Only send a close if the queue is empty.
                                sendClose();
                            } else {
                                // Reliable Output Stream not empty. Don't send close yet.
                                ros.waitQueueEmpty(1000);
                                continue;
                            }
                        } catch (InterruptedException woken) {
                            Thread.interrupted();
                            break;
                        }
                    } else {
                        sendClose();
                    }

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Sent close, awaiting ACK for " + this);
                    }

                    // Don't send our close too many times.
                    try {
                        long nextTry = Math.min(20000, closingFor);

                        if (nextTry > 0 && isConnected()) {
                            closeLock.wait(nextTry);
                        }
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                        break;
                    }
                }

                if (isConnected()) {
                    // Last ditch close attempt
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Still connected at end of timeout. Forcing closed." + this);
                    }
                    sendClose();
                    throw new SocketTimeoutException("Failed to receive close ack from remote connection.");
                }
            }
        } finally {
            // No matter what else happens at the end of close() we are no
            // longer connected and no longer bound.
            setConnected(false);
            unbind();
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Socket closed : " + this);
            }
        }
    }

    /**
     * This is called when closure is initiated on the remote side. By
     * convention we receive the close message only after we have ACKed the last
     * data segment.
     * <p/>
     * We soft-close the InputStream which allows us to read data already
     * received.
     * <p/>
     * We hard-close our output stream and discard all queued, unACKed data
     * as the remote side doesn't want to receive it (the remote side has
     * already unbound themselves, they just want our close ACK in order to clean
     * up.)
     *
     * @throws IOException if an I/O error occurs
     */
    protected void closeFromRemote() throws IOException {
        synchronized (closeLock) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.INFO)) {
                LOG.info("Received a remote close request." + this);
            }

            if (isConnected()) {
                setConnected(false);
                if (isReliable) {
                    ris.softClose();
                    ros.hardClose();
                } else {
                    nonReliableInputStream.softClose();
                    nonReliableOutputStream.hardClose();
                }
            }

            // If we are still bound then send them a close ACK.
            if (isBound() && (ros != null && ros.isQueueEmpty())) {
                // do not ack until the queue is empty
                sendCloseACK();
            }
            if (closeAckReceived) {
                closeLock.notifyAll();
            }
        }
    }

    /**
     * Closes the input pipe which we use to receive messages and the messenger
     * used for sending messages.
     */
    protected synchronized void unbind() {
        if (!isBound()) {
            return;
        }

        if (isReliable) {
            try {
                ris.close();
            } catch (IOException ignored) {// ignored
            }

            ros.hardClose();
        } else {
            nonReliableInputStream.close();
            nonReliableOutputStream.hardClose();
        }

        // We are no longer bound
        setBound(false);

        // close pipe and messenger
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Closing ephemeral input pipe");
        }

        localEphemeralPipeIn.close();

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Closing remote ephemeral pipe messenger");
        }

        if(null != outgoing) {
            outgoing.close();
        }
        remoteEphemeralPipeMsgr.close();
    }

    /**
     * {@inheritDoc}
     */
    public void pipeMsgEvent(PipeMsgEvent event) {
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.log(Level.FINER, "Pipe Message Event for " + this + "\n\t" + event.getMessage() + " for " + event.getPipeID());
        }

        Message message = event.getMessage();
        if (message == null) {
            return;
        }

        // look for close request/ack
        MessageElement element = message.getMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE, JxtaServerSocket.closeTag);

        if (element != null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Handling a close message " + this + " : " + element.toString());
            }
            if (JxtaServerSocket.closeReqValue.equals(element.toString())) {
                try {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Received a close request");
                    }
                    closeFromRemote();
                } catch (IOException ie) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "failed during closeFromRemote", ie);
                    }
                }
            } else if (JxtaServerSocket.closeAckValue.equals(element.toString())) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Received a close acknowledgement");
                }
                synchronized (closeLock) {
                    closeAckReceived = true;
                    setConnected(false);
                    closeLock.notifyAll();
                }
            }
            return;
        }

        if (!isConnected()) {
            // connect response
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Processing connect response : " + message);
            }

            // look for a remote pipe answer
            element = message.getMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE, JxtaServerSocket.remPipeTag);
            PipeAdvertisement incomingPipeAdv = null;

            if (element != null) {
                try {
                    XMLDocument pipeAdvDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(element);

                    incomingPipeAdv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(pipeAdvDoc);
                } catch (IOException badPipeAdv) {// ignored
                }
            }

            element = message.getMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE, JxtaServerSocket.remPeerTag);
            PeerAdvertisement incomingRemotePeerAdv = null;

            if (element != null) {
                try {
                    XMLDocument peerAdvDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(element);
                    incomingRemotePeerAdv = (PeerAdvertisement) AdvertisementFactory.newAdvertisement(peerAdvDoc);
                } catch (IOException badPeerAdv) {// ignored
                }
            }

            element = message.getMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE, JxtaServerSocket.credTag);
            Credential incomingCredential = null;

            if (element != null) {
                try {
                    StructuredDocument incomingCredentialDoc = StructuredDocumentFactory.newStructuredDocument(element);
                    incomingCredential = group.getMembershipService().makeCredential(incomingCredentialDoc);
                } catch (Exception failed) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Unable to generate credential for " + this, failed);
                    }
                }
            }

            element = message.getMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE, JxtaServerSocket.streamTag);
            boolean incomingIsReliable = isReliable;

            if (element != null) {
                incomingIsReliable = Boolean.valueOf(element.toString());
            }

            if ((null != incomingPipeAdv) && (null != incomingRemotePeerAdv)) {
                if ((null != remotePeerID) && (remotePeerID != incomingRemotePeerAdv.getPeerID())) {
                    // let the connection attempt timeout
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning(
                                "Connection response from wrong peer! " + remotePeerID + " != "
                                + incomingRemotePeerAdv.getPeerID());
                    }
                    return;
                }

                synchronized (socketConnectLock) {
                    if (!isConnected()) {
                        remoteCredential = incomingCredential;
                        remotePeerAdv = incomingRemotePeerAdv;
                        remotePeerID = incomingRemotePeerAdv.getPeerID();
                        remoteEphemeralPipeAdv = incomingPipeAdv;
                        isReliable = incomingIsReliable;

                        // Force the creation of the inputStream now. Waiting until someone
                        // calls getInputStream() would likely cause us to drop messages.

                        // FIXME: it would be even better if we could create the
                        // input stream BEFORE having the output pipe resolved, but
                        // that would force us to have the MsrgAdaptor block
                        // until we can give it the real pipe or msgr... later.
                        try {
                            connect();
                        } catch (IOException failed) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Connection failed : " + this, failed);
                            }
                            return;
                        }
                        socketConnectLock.notify();
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.log(Level.INFO, "New Socket Connection : " + this);
                        }
                    }
                }
                return;
            }
        }

        // Often we are called to handle data before the socket is connected.
        synchronized (socketConnectLock) {
            long timeoutAt = System.currentTimeMillis() + timeout;
            if (timeoutAt < timeout) {
                timeoutAt = Long.MAX_VALUE;
            }

            while (!isClosed() && !isConnected()) {
                long waitFor = timeoutAt - System.currentTimeMillis();

                if (waitFor <= 0) {
                    break;
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "Holding " + message + " for " + timeout);
                }

                try {
                    socketConnectLock.wait(timeout);
                } catch (InterruptedException woken) {
                    return;
                }
            }
        }

        if (!isReliable) {
            // is there data ?
            Iterator<MessageElement> dataElements = message.getMessageElements(JxtaServerSocket.MSG_ELEMENT_NAMESPACE,
                    JxtaServerSocket.dataTag);

            while (dataElements.hasNext()) {
                MessageElement anElement = dataElements.next();
                nonReliableInputStream.enqueue(anElement);
            }
        } else {
            // Give ACKs to the Reliable Output Stream
            if (ros != null) {
                ros.recv(message);
            }
            // Give data blocks to the Reliable Input Stream
            if (ris != null) {
                ris.recv(message);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void outputPipeEvent(OutputPipeEvent event) {
        OutputPipe op = event.getOutputPipe();

        if (op.getAdvertisement() == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("The output pipe has no internal pipe advertisement. discarding event");
            }
            return;
        }
        // name can be different, therefore check the id + type
        if (pipeAdv.getID().equals(op.getAdvertisement().getID()) && pipeAdv.getType().equals(op.getAdvertisement().getType())) {
            synchronized (pipeResolveLock) {
                // modify op within lock to prevent a race with the if.
                if (connectOutpipe == null) {
                    connectOutpipe = op;
                    // if not null, will be closed.
                    op = null;
                }
                pipeResolveLock.notify();
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
     * A lightweight output pipe constructor, note the return type
     * Since all the info needed is available, there's no need for to use the pipe service
     * to resolve the pipe we have all we need to construct a messenger.
     *
     * @param group   group context
     * @param pipeAdv Remote Pipe Advertisement
     * @param peerAdv Remote Peer Advertisement
     * @return Messenger
     */
    protected static Messenger lightweightOutputPipe(PeerGroup group, PipeAdvertisement pipeAdv, PeerAdvertisement peerAdv) {
        EndpointService endpoint = group.getEndpointService();
        ID opId = pipeAdv.getPipeID();
        String destPeer = peerAdv.getPeerID().getUniqueValue().toString();

        // Get an endpoint messenger to that address
        EndpointAddress addr;
        RouteAdvertisement routeHint = net.jxta.impl.endpoint.EndpointUtils.extractRouteAdv(peerAdv);
        if (pipeAdv.getType().equals(PipeService.UnicastType)) {
            addr = new EndpointAddress("jxta", destPeer, "PipeService", opId.toString());
        } else if (pipeAdv.getType().equals(PipeService.UnicastSecureType)) {
            addr = new EndpointAddress("jxtatls", destPeer, "PipeService", opId.toString());
        } else {
            // not a supported type
            throw new IllegalArgumentException(pipeAdv.getType() + " is not a supported pipe type");
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("New pipe lightweight messenger for " + addr);
        }
        return endpoint.getMessenger(addr, routeHint);
    }

    /**
     * Sends a close message
     * @throws IOException if an io error occurs
     */
    private void sendClose() throws IOException {
        Message msg = new Message();

        msg.addMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE
                ,
                new StringMessageElement(JxtaServerSocket.closeTag, JxtaServerSocket.closeReqValue, null));

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending a close request " + this + " : " + msg);
        }
        remoteEphemeralPipeMsgr.sendMessageN(msg, null, null);
    }

    /**
     * Sends a close ack message
     * @throws IOException if an io error occurs
     */
    private void sendCloseACK() throws IOException {
        Message msg = new Message();
        msg.addMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE,
                new StringMessageElement(JxtaServerSocket.closeTag, JxtaServerSocket.closeAckValue, null));
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending a close ACK " + this + " : " + msg);
        }
        remoteEphemeralPipeMsgr.sendMessageN(msg, null, null);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getSoTimeout() throws SocketException {
        if (isClosed()) {
            throw new SocketException("Socket is closed");
        }
        if (timeout > Integer.MAX_VALUE) {
            return 0;
        } else {
            return (int) timeout;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSoTimeout(int soTimeout) throws SocketException {
        if (soTimeout < 0) {
            throw new IllegalArgumentException("Invalid Socket timeout :" + soTimeout);
        }

        this.timeout = soTimeout;
        if (!isBound()) {
            return;
        }

        // If we are bound then set the timeout on the streams.
        // FIXME, ros does not define a timeout as it only relies on window saturation, it should take into account
        // the socket timeout
        if (isReliable) {
            if (ris != null) {
                ris.setTimeout(soTimeout);
            }
        } else {
            nonReliableInputStream.setTimeout((long) soTimeout);
        }
    }

    /**
     * Gets the Maximum Retry Timeout of the reliability layer
     *
     * @return The maximum retry Timeout value
     * @deprecated The reliability layer manages it's own maximum. This value is not useful.
     */
    @Deprecated
    public int getMaxRetryTimeout() {
        return maxRetryTimeout;
    }

    /**
     * Gets the Maximum Retry Timeout of the reliability layer
     *
     * @param maxRetryTimeout The new maximum retry timeout value
     * @throws IllegalArgumentException if maxRetryTimeout exceeds jxta platform maximum retry timeout
     * @deprecated The reliability layer manages it's own maximum. This value is not useful.
     */
    @Deprecated
    public void setMaxRetryTimeout(int maxRetryTimeout) {
        if (maxRetryTimeout <= 0 || maxRetryTimeout > MAXRETRYTIMEOUT) {
            throw new IllegalArgumentException("Invalid Maximum retry timeout :" + maxRetryTimeout + " Exceed Global maximum retry timeout :"
                    + MAXRETRYTIMEOUT);
        }
        this.maxRetryTimeout = maxRetryTimeout;
    }

    /**
     * Gets the Retry Timeout of the reliability layer
     *
     * @return The retry Timeout value
     */
    public int getRetryTimeout() {
        return retryTimeout;
    }

    /**
     * Sets the Retry Timeout of the underlying reliability layer.
     * In reliable mode it is possible for this call to block
     * trying to obtain a lock on reliable input stream
     *
     * @param retryTimeout The new retry timeout value
     * @throws SocketException if an I/O error occurs
     */
    public void setRetryTimeout(int retryTimeout) throws SocketException {
        if (retryTimeout <= 0 || retryTimeout > maxRetryTimeout) {
            throw new IllegalArgumentException("Invalid Retry Socket timeout :" + retryTimeout);
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
    public int getWindowSize() {
        return windowSize;
    }

    /**
     * When in reliable mode, sets the Reliable library window size
     *
     * @param windowSize The new window size value
     * @throws SocketException if an I/O error occurs
     */
    public void setWindowSize(int windowSize) throws SocketException {
        if (isBound()) {
            throw new SocketException("Socket bound. Can not change the window size");
        }
        this.windowSize = windowSize;
    }

    /**
     * Returns the closed state of the JxtaSocket.
     *
     * @return true if the socket has been closed
     */
    @Override
    public boolean isClosed() {
        return closed;
    }

    /**
     * Performs on behalf of JxtaSocketOutputStream.
     *
     * @param buf    the data.
     * @param offset the start offset in the data.
     * @param length the number of bytes to write.
     * @throws IOException if an I/O error occurs
     * @see java.io.OutputStream#write
     */
    protected void write(byte[] buf, int offset, int length) throws IOException {
        checkState();
        if (isReliable) {
            ros.write(buf, offset, length);
        } else {
            byte[] bufCopy = new byte[length];
            System.arraycopy(buf, offset, bufCopy, 0, length);

            Message msg = new Message();
            msg.addMessageElement(JxtaServerSocket.MSG_ELEMENT_NAMESPACE,
                    new ByteArrayMessageElement(JxtaServerSocket.dataTag, MimeMediaType.AOS, bufCopy, 0, length, null));
            remoteEphemeralPipeMsgr.sendMessageB(msg, null, null);
        }
    }

    /**
     * @throws SocketException if closed, not bound or not connected.
     */
    private void checkState() throws SocketException {
        if (isClosed()) {
            throw new SocketException("Socket is closed.");
        } else if (!isBound()) {
            throw new SocketException("Socket not bound.");
        } else if (!isConnected()) {
            throw new SocketException("Socket not connected.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getSendBufferSize() throws SocketException {
        if (isOutputShutdown()) {
            throw new SocketException("Socket is closed");
        }
        return (outputBufferSize == -1) ? DEFAULT_OUTPUT_BUFFER_SIZE : outputBufferSize;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSendBufferSize(int size) throws SocketException {
        if (isOutputShutdown()) {
            throw new SocketException("Socket is closed");
        }

        if (size < 1) {
            throw new IllegalArgumentException("negative/zero buffer size");
        }

        if ((null != remoteEphemeralPipeMsgr) && (size > remoteEphemeralPipeMsgr.getMTU())) {
            throw new IllegalArgumentException("Buffer size larger than limit : " + remoteEphemeralPipeMsgr.getMTU());
        }

        outputBufferSize = size;
        if (null != ros) {
            try {
                ros.setSendBufferSize(size);
            } catch (SocketException failure) {
                throw failure;
            } catch (IOException failed) {
                SocketException failure = new SocketException("Failed");

                failure.initCause(failed);
                throw failure;
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getReceiveBufferSize() throws SocketException {
        if (isInputShutdown()) {
            throw new SocketException("Socket is closed");
        }
        // this is just rough size
        return outputBufferSize * windowSize;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean getKeepAlive() throws SocketException {
        if (inputShutdown) {
            throw new SocketException("Socket is closed");
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getTrafficClass() throws SocketException {
        throw new SocketException("TrafficClass not yet defined");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTrafficClass(int tc) throws SocketException {
        // a place holder when and if we decide to add hints regarding
        // flow info hints such as (IPTOS_LOWCOST (0x02), IPTOS_RELIABILITY (0x04), etc
        throw new SocketException("TrafficClass not yet defined");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isInputShutdown() {
        return inputShutdown;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isOutputShutdown() {
        return outputShutdown;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendUrgentData(int data) throws IOException {
        throw new SocketException("Urgent data not supported");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setOOBInline(boolean state) throws SocketException {
        throw new SocketException("Enable/disable OOBINLINE supported");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setKeepAlive(boolean state) throws SocketException {
        if (isClosed()) {
            throw new SocketException("Socket is closed");
        }
        throw new SocketException("Operation not supported");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void shutdownInput() throws IOException {
        inputShutdown = true;
        if (isReliable) {
            // hard close (EOF on next read)
            ris.close();
        } else {
            // hard close (EOF on next read)
            nonReliableInputStream.close();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void shutdownOutput() throws IOException {
        outputShutdown = true;
        if (isReliable) {
            ros.setLingerDelay(timeout);
            // soft close (finish sending if you can)
            ros.close();
        } else {
            // soft close (finish sending if you can)
            nonReliableOutputStream.close();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public SocketAddress getLocalSocketAddress() {
        if (!isBound()) {
            return null;
        }
        return new JxtaSocketAddress(group, localEphemeralPipeAdv, group.getPeerAdvertisement());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public SocketAddress getRemoteSocketAddress() {
        if (!isConnected()) {
            return null;
        }
        return new JxtaSocketAddress(group, remoteEphemeralPipeAdv, remotePeerAdv);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This output is suitable for debugging but should not be parsed. All
     * of the information is available through other means.
     */
    @Override
    public String toString() {

        StringBuilder result = new StringBuilder();

        result.append(getClass().getName());
        result.append('@');
        result.append(System.identityHashCode(this));
        result.append('[');

        if (null != pipeAdv) {
            result.append(pipeAdv.getPipeID().getUniqueValue());
        }
        result.append('/');

        if (null != localEphemeralPipeAdv) {
            result.append(localEphemeralPipeAdv.getPipeID().getUniqueValue());
        }
        result.append(']');
        result.append(isClosed() ? " CLOSED :" : " OPEN :");
        result.append(initiator ? " I " : " i ");
        result.append(isReliable ? " R " : " r ");
        result.append(isBound() ? " B " : " b ");
        result.append(isConnected() ? " C " : " c ");

        return result.toString();
    }
}
