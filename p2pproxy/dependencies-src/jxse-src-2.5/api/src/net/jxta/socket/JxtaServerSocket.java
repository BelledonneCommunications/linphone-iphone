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
package net.jxta.socket;

import net.jxta.credential.Credential;
import net.jxta.credential.CredentialValidator;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * JxtaServerSocket is a bi-directional Pipe that behaves very much like
 * ServerSocket.  It creates an inputpipe and listens for pipe connection
 * requests. JxtaServerSocket also defines it own protocol. Requests arrive as
 * a JXTA Message with the following elements:
 * <p/>
 * &lt;Cred> Credentials which can be used to determine trust &lt;/Cred>
 * <p/>
 * &lt;reqPipe> requestor's pipe advertisement &lt;/reqPipe>
 * <p/>
 * &lt;remPipe> Remote pipe advertisement &lt;/remPipe>
 * <p/>
 * &lt;reqPeer> Remote peer advertisement  &lt;/reqPeer>
 * <p/>
 * &lt;stream> determine whether the connection is reliable, or not &lt;/stream>
 * <p/>
 * &lt;close> close request &lt;/close>
 * <p/>
 * &lt;data> Data &lt;/data>
 * <p/>
 * JxtaServerSocket then creates a new private pipe, listens for messages on that pipe,
 * resolves the requestor's pipe, and sends a &lt;remPipe> private pipe created &lt;/remotePipe>
 * advertisement back, where the remote side is resolved.
 * <p/>
 * The {@code accept()} backlog defaults to 50 requests.
 * <p/>
 * The timeout default to 60 seconds, i.e. blocking.
 */
public class JxtaServerSocket extends ServerSocket implements PipeMsgListener {

    private static final Logger LOG = Logger.getLogger(JxtaServerSocket.class.getName());

    protected static final String MSG_ELEMENT_NAMESPACE = "JXTASOC";
    protected static final String credTag = "Cred";
    protected static final String reqPipeTag = "reqPipe";
    protected static final String remPeerTag = "remPeer";
    protected static final String remPipeTag = "remPipe";
    protected static final String dataTag = "data";
    protected static final String closeTag = "close";
    protected final static String closeReqValue = "close";
    protected final static String closeAckValue = "closeACK";
    protected static final String streamTag = "stream";

    private final static int DEFAULT_BACKLOG = 50;
    private final static long DEFAULT_TIMEOUT = 60 * 1000L;

    /**
     * QUEUE_END_MESSAGE is used to signal that the queue has been closed.
     */
    protected static final Message QUEUE_END_MESSAGE = new Message();

    /**
     * The PeerGroup
     */
    protected PeerGroup group;

    /**
     * The pipe advertisement we are serving.
     */
    protected PipeAdvertisement pipeAdv;

    /**
     * The input pipe on which we listen for connect requests.
     */
    protected InputPipe serverPipe;

    /**
     * The credential we will present to connect requests.
     */
    protected Credential localCredential = null;

    /**
     * The number of connect requests we will allow to become backlogged.
     */
    protected int backlog = DEFAULT_BACKLOG;

    /**
     * The timeout for accept operations.
     */
    protected long timeout = DEFAULT_TIMEOUT;

    protected BlockingQueue<Message> queue = null;
    protected volatile boolean bound = false;
    protected volatile boolean closed = false;
    private CredentialValidator credValidator = null;

    /**
     * Default Constructor
     * <p/>
     * A call to {@code bind()} is needed to finish initializing this object.
     *
     * @throws IOException if an io error occurs
     */
    public JxtaServerSocket() throws IOException {}

    /**
     * Constructs and binds a JxtaServerSocket using a JxtaSocketAddress as
     * the address.
     *
     * @param address an instance of JxtaSocketAddress
     * @throws IOException if an io error occurs
     * @see net.jxta.socket.JxtaSocketAddress
     */
    public JxtaServerSocket(SocketAddress address) throws IOException {
        this(address, DEFAULT_BACKLOG);
    }

    /**
     * Constructs and binds a JxtaServerSocket to the specified pipe.
     *
     * @param group   JXTA PeerGroup
     * @param pipeAdv PipeAdvertisement on which pipe requests are accepted
     * @throws IOException if an I/O error occurs
     */
    public JxtaServerSocket(PeerGroup group, PipeAdvertisement pipeAdv) throws IOException {
        this(group, pipeAdv, DEFAULT_BACKLOG);
    }

    /**
     * Constructs and binds a JxtaServerSocket using a JxtaSocketAddress as
     * the address.
     *
     * @param address an instance of JxtaSocketAddress
     * @param backlog the size of the backlog queue
     * @throws IOException if an I/O error occurs
     * @see net.jxta.socket.JxtaSocketAddress
     */
    public JxtaServerSocket(SocketAddress address, int backlog) throws IOException {
        this(address, backlog, (int) DEFAULT_TIMEOUT);
    }

    /**
     * Constructor for the JxtaServerSocket object
     *
     * @param group   JXTA PeerGroup
     * @param pipeAdv PipeAdvertisement on which pipe requests are accepted
     * @param backlog the maximum length of the queue.
     * @throws IOException if an I/O error occurs
     */
    public JxtaServerSocket(PeerGroup group, PipeAdvertisement pipeAdv, int backlog) throws IOException {
        this(group, pipeAdv, backlog, (int) DEFAULT_TIMEOUT);
    }

    /**
     * Constructs and binds a JxtaServerSocket using a JxtaSocketAddress as
     * the address.
     *
     * @param address an instance of JxtaSocketAddress
     * @param backlog the size of the backlog queue
     * @param timeout connection timeout in milliseconds
     * @throws IOException if an I/O error occurs
     * @see net.jxta.socket.JxtaSocketAddress
     */
    public JxtaServerSocket(SocketAddress address, int backlog, int timeout) throws IOException {
        setSoTimeout(timeout);
        bind(address, backlog);
    }

    /**
     * Constructor for the JxtaServerSocket object.
     *
     * @param group   JXTA PeerGroup
     * @param pipeAdv PipeAdvertisement on which pipe requests are accepted
     * @param backlog the maximum length of the queue.
     * @param timeout the specified timeout, in milliseconds
     * @throws IOException if an I/O error occurs
     */
    public JxtaServerSocket(PeerGroup group, PipeAdvertisement pipeAdv, int backlog, int timeout) throws IOException {
        this(group, pipeAdv, backlog, timeout, null);
    }

    /**
     * Constructor for the JxtaServerSocket object.
     *
     * @param group   JXTA PeerGroup
     * @param pipeAdv PipeAdvertisement on which pipe requests are accepted
     * @param backlog the maximum length of the queue.
     * @param timeout the specified timeout, in milliseconds
     * @param credValidator the CredentialValidator
     * @throws IOException if an I/O error occurs
     */
    public JxtaServerSocket(PeerGroup group, PipeAdvertisement pipeAdv, int backlog, int timeout, CredentialValidator credValidator) throws IOException {
        setSoTimeout(timeout);
        this.credValidator = credValidator;
        bind(group, pipeAdv, backlog);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Closes the JxtaServerPipe.
     */
    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (!closed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("JxtaServerSocket is being finalized without being previously closed. This is likely an application level bug.");
            }
        }
        close();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Socket accept() throws IOException {
        if (!isBound()) {
            throw new SocketException("Socket is not bound yet");
        }

        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Waiting for a connection");
            }

            while (true) {
                if (isClosed()) {
                    throw new SocketException("Socket is closed");
                }
                Message msg = queue.poll(timeout, TimeUnit.MILLISECONDS);

                if (isClosed()) {
                    throw new SocketException("Socket is closed");
                }
                if (msg == null) {
                    throw new SocketTimeoutException("Timeout reached");
                }

                if (QUEUE_END_MESSAGE == msg) {
                    throw new SocketException("Socket is closed.");
                }

                JxtaSocket socket = processMessage(msg);

                // make sure we have a socket returning
                if (socket != null) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("New socket connection " + socket);
                    }
                    return socket;
                } else if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("No connection.");
                }
            }
        } catch (InterruptedException ie) {
            SocketException interrupted = new SocketException("interrupted");

            interrupted.initCause(ie);
            throw interrupted;
        }
    }

    /**
     * Binds the <code>JxtaServerSocket</code> to a specific pipe advertisement
     *
     * @param group   JXTA PeerGroup
     * @param pipeAdv PipeAdvertisement on which pipe requests are accepted
     * @throws IOException if an I/O error occurs
     */
    public void bind(PeerGroup group, PipeAdvertisement pipeAdv) throws IOException {
        bind(group, pipeAdv, DEFAULT_BACKLOG);
    }

    /**
     * Binds the <code>JxtaServerSocket</code> to a specific pipe advertisement
     *
     * @param group   JXTA PeerGroup
     * @param pipeadv PipeAdvertisement on which pipe requests are accepted
     * @param backlog the maximum length of the queue.
     * @throws IOException if an I/O error occurs
     */
    public void bind(PeerGroup group, PipeAdvertisement pipeadv, int backlog) throws IOException {
        if (PipeService.PropagateType.equals(pipeadv.getType())) {
            throw new IOException("Propagate pipe advertisements are not supported");
        }

        if (backlog <= 0) {
            throw new IllegalArgumentException("backlog must be > 0");
        }

        this.backlog = backlog;
        queue = new ArrayBlockingQueue<Message>(backlog);
        this.group = group;
        this.pipeAdv = pipeadv;
        PipeService pipeSvc = group.getPipeService();

        serverPipe = pipeSvc.createInputPipe(pipeadv, this);
        setBound(true);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Used to bind a  JxtaServerSocket created with the no-arg constructor.
     */
    @Override
    public void bind(SocketAddress endpoint) throws IOException {
        bind(endpoint, backlog);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Used to bind a  JxtaServerSocket created with the no-arg constructor.
     */
    @Override
    public void bind(SocketAddress endpoint, int backlog) throws IOException {
        if (endpoint instanceof JxtaSocketAddress) {
            JxtaSocketAddress socketAddress = (JxtaSocketAddress) endpoint;
            PeerGroup pg = PeerGroup.globalRegistry.lookupInstance(socketAddress.getPeerGroupId());

            if (pg == null) {
                throw new IOException(
                        "Can't connect socket in PeerGroup with id " + socketAddress.getPeerGroupId()
                        + ". No running instance of the group is registered.");
            }
            bind(pg.getWeakInterface(), socketAddress.getPipeAdv(), backlog);
            pg.unref();
        } else {
            throw new IllegalArgumentException("Unsupported subclass of SocketAddress; " + "use JxtaSocketAddress instead.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void close() throws IOException {

        if (closed) {
            return;
        }
        closed = true;

        if (isBound()) {
            // close all the pipe
            serverPipe.close();
            setBound(false);
        }

        queue.clear();
        while (true) {
            try {
                queue.put(QUEUE_END_MESSAGE);
                // end queue message is now on the queue, we are done.
                break;
            } catch (InterruptedException woken) {
                // We MUST put the terminal message onto the queue before
                // finishing. We won't have a second chance.
                Thread.interrupted();
            }
        }
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Closed : " + this);
        }
    }

    /**
     * @return the server socket's JxtaSocketAddress
     * @see java.net.ServerSocket#getLocalSocketAddress()
     */
    @Override
    public SocketAddress getLocalSocketAddress() {
        return new JxtaSocketAddress(getGroup(), getPipeAdv());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getSoTimeout() throws IOException {
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
    public void setSoTimeout(int timeout) throws SocketException {
        if (isClosed()) {
            throw new SocketException("Socket is closed");
        }

        if (timeout < 0) {
            throw new IllegalArgumentException("timeout must be >= 0");
        }

        if (0 == timeout) {
            this.timeout = Long.MAX_VALUE;
        } else {
            this.timeout = (long) timeout;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isBound() {
        return bound;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isClosed() {
        return closed;
    }

    /**
     * Sets whether this socket is currently bound or not. A socket is
     * considered bound if the local resources required in order to interact
     * with a remote peer are allocated and open.
     *
     * @param boundState The new bound state.
     */
    private synchronized void setBound(boolean boundState) {
        this.bound = boundState;
    }

    /**
     * Gets the group associated with this JxtaServerSocket object
     *
     * @return The group value
     */
    public PeerGroup getGroup() {
        return group;
    }

    /**
     * Gets the PipeAdvertisement associated with this JxtaServerSocket object
     *
     * @return The pipeAdv value
     */
    public PipeAdvertisement getPipeAdv() {
        return pipeAdv;
    }

    /**
     * {@inheritDoc}
     */
    public void pipeMsgEvent(PipeMsgEvent event) {

        // deal with messages as they come in
        Message message = event.getMessage();

        if (message == null) {
            return;
        }

        boolean pushed = false;
        try {
            pushed = queue.offer(message, timeout, TimeUnit.MILLISECONDS);
        } catch (InterruptedException woken) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Interrupted", woken);
            }
        }

        if (!pushed && Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
            LOG.warning("backlog queue full, connect request dropped");
        }
    }

    /**
     * processMessage is the main mechanism in establishing bi-directional connections
     * <p/>
     * It accepts connection messages and constructs a JxtaSocket with a ephemeral
     * InputPipe and a messenger.
     *
     * @param msg The client connection request (assumed not null)
     * @return JxtaSocket Which may be null if an error occurs.
     */
    private JxtaSocket processMessage(Message msg) {

        PipeAdvertisement remoteEphemeralPipeAdv = null;
        PeerAdvertisement remotePeerAdv = null;
        Credential credential = null;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Processing a connection message : " + msg);
        }

        try {
            MessageElement el = msg.getMessageElement(MSG_ELEMENT_NAMESPACE, reqPipeTag);
            if (el != null) {
                XMLDocument pipeAdvDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(el);
                remoteEphemeralPipeAdv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(pipeAdvDoc);
            }

            el = msg.getMessageElement(MSG_ELEMENT_NAMESPACE, remPeerTag);
            if (el != null) {
                XMLDocument peerAdvDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(el);
                remotePeerAdv = (PeerAdvertisement) AdvertisementFactory.newAdvertisement(peerAdvDoc);
            }

            el = msg.getMessageElement(MSG_ELEMENT_NAMESPACE, credTag);
            if (el != null) {
                try {
                    XMLDocument credDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(el);
                    credential = group.getMembershipService().makeCredential(credDoc);
                    if (!checkCred(credential)) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Invalid credential");
                        }
                        return null;
                    }
                } catch (Exception ignored) {
                    // ignored
                }
            }

            boolean isReliable = false;

            el = msg.getMessageElement(MSG_ELEMENT_NAMESPACE, streamTag);
            if (el != null) {
                isReliable = Boolean.valueOf(el.toString());
            }

            if ((null != remoteEphemeralPipeAdv) && (null != remotePeerAdv)) {
                return createEphemeralSocket(group, pipeAdv, remoteEphemeralPipeAdv, remotePeerAdv, localCredential, credential, isReliable);
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Connection message did not contain valid connection information.");
                }
                return null;
            }
        } catch (IOException e) {
            // deal with the error
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "IOException occured", e);
            }
        } catch (RuntimeException e) {
            // deal with the error
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception occured", e);
            }
        }
        return null;
    }

    /**
     * Invokes the specified CredentialValidator to very a credential
     * @param cred the credential
     * @return <code>true</code> if valid, or if no validator is specified
     */
    private boolean checkCred(Credential cred) {
        return credValidator == null || credValidator.checkCred(cred);
    }

    /**
     * Construct the emphemeral socket result from accept. This method exists
     * primarily so that sub-classes can substitute a different JxtaSocket
     * sub-class.
     *
     * @param group               The peer group for the socket.
     * @param pipeAdv             The public pipe advertisement.
     * @param remoteEphemeralPipeAdv The pipe advertisement of the remote peer's
     *                            ephemeral pipe.
     * @param remotePeerAdv          The peer advertisement of the remote peer.
     * @param localCredential        Our credential.
     * @param credential          The credential of the remote peer.
     * @param isReliable          if true, uses the reliability library in non-direct mode
     * @return The new JxtaSocket instance.
     * @throws IOException if an io error occurs
     */
    protected JxtaSocket createEphemeralSocket(PeerGroup group, PipeAdvertisement pipeAdv, PipeAdvertisement remoteEphemeralPipeAdv, PeerAdvertisement remotePeerAdv, Credential localCredential, Credential credential, boolean isReliable) throws IOException {
        return new JxtaSocket(group, pipeAdv, remoteEphemeralPipeAdv, remotePeerAdv, localCredential, credential, isReliable);
    }

    /**
     * Sets the credential to be used by this socket connection. If no
     * credentials are set, the default group credential will be used.
     *
     * @param localCredential The credential to be used for connection responses
     *                     or <tt>null</tt> if the default credential is to be used.
     */
    public void setCredential(Credential localCredential) {
        this.localCredential = localCredential;
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
        result.append(pipeAdv.getPipeID());
        result.append(']');

        result.append(isClosed() ? " CLOSED :" : " OPEN :");
        result.append(isBound() ? " BOUND " : " UNBOUND ");
        return result.toString();
    }
}
