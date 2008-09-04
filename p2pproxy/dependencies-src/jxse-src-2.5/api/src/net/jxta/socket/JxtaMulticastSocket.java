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
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.id.IDFactory;
import net.jxta.impl.util.ProducerBiasedQueue;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PipeAdvertisement;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MulticastSocket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.URI;
import java.util.Collections;
import java.util.Enumeration;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * The JxtaMulticastSocket class is useful for sending and receiving
 * JXTA multicast packets. A JxtaMulticastSocket is a (UDP) DatagramSocket,
 * with additional capabilities for joining "groups" of other multicast hosts
 * on the internet.
 * A multicast group is specified within the context of PeerGroup and a propagate
 * pipe advertisement.
 * One would join a multicast group by first creating a MulticastSocket
 * with the desired peer group and pipe advertisement :
 * <p/>
 * <pre>
 *  // join a Multicast group and send the group salutations
 *  ...
 *  String msg = "Hello";
 *  MulticastSocket s = new JxtaMulticastSocket(peergroup, propPipeAdv);
 *  //We are joined at this point
 *  DatagramPacket hi = new DatagramPacket(msg.getBytes(), msg.length());
 *  s.send(hi);
 *  // get their responses!
 *  byte[] buf = new byte[1000];
 *  DatagramPacket recv = new DatagramPacket(buf, buf.length);
 *  s.receive(recv);
 *  ...
 *  // OK, I'm done talking - leave the group...
 *  s.close();
 * </pre>
 * <p/>
 * One can also respond only to the sender of the datagram as follows :
 * <pre>
 *  DatagramPacket res = new DatagramPacket(response.getBytes(), response.length());
 *  res.setAddress(recv.getAddress());
 *  s.send(res);
 * </pre>
 * <p/>
 * When one sends a message to a multicast group, all subscribing recipients to
 * that peergroup and pipe receive the message (including themselves)
 * When a socket subscribes to a multicast group/port, it receives datagrams
 * sent by other hosts to the group/pipe, as do all other members of the group
 * and pipe. A socket relinquishes membership in a group by the
 * close() method. Multiple MulticastSocket's may
 * subscribe to a multicast group and pipe concurrently, and they will all receive
 * group datagrams.
 * <p/>
 * When a datagram is sent it carries along with the peerid of the sender.
 * The PeerID is represented as a InetAddress in the form of host/ipadress
 * where host name is the peerid, and ip address is always represented as 0.0.0.0
 * since it is meaningless in the context of JXTA.
 * e.g of InetAddress resembles the following:
 * <p/>
 * <pre>
 * uuid-59616261646162614A787461503250339C6014B0F21A49DBBDF2ADBDDBCB314703/0.0.0.0
 * </pre>
 */

public class JxtaMulticastSocket extends MulticastSocket implements PipeMsgListener {
    private final static Logger LOG = Logger.getLogger(JxtaMulticastSocket.class.getName());
    public static final String NAMESPACE = "JXTAMCAST";
    public static final String DATATAG = "DATAGRAM";
    public static final String SRCIDTAG = "SRCID";
    protected PipeAdvertisement pipeAdv;
    protected PipeService pipeSvc;
    protected InputPipe in;
    protected PeerGroup group;
    protected SocketAddress socketAddress;
    protected InetAddress localAddress;
    protected OutputPipe outputPipe;
    protected boolean closed = false;
    protected boolean bound = false;
    protected ProducerBiasedQueue queue = new ProducerBiasedQueue();
    protected Credential credential = null;
    protected StructuredDocument credentialDoc = null;
    private int timeout = 60000;
    private byte[] fauxip = new byte[4];
    private boolean jxtamode = false;
    private MessageElement srcElement = null;

    /**
     * Create a multicast socket and bind it to a specific pipe within specified
     * peer group
     *
     * @param group  group context
     * @param pipeAd PipeAdvertisement
     * @throws IOException if an io error occurs
     */
    public JxtaMulticastSocket(PeerGroup group, PipeAdvertisement pipeAd) throws IOException {
        super();
        joinGroup(group, pipeAd);
    }

    /**
     * joins MutlicastSocket to specified pipe within the context of group
     *
     * @param group  group context
     * @param pipeAd PipeAdvertisement
     * @throws IOException if an io error occurs
     */
    public void joinGroup(PeerGroup group, PipeAdvertisement pipeAd) throws IOException {

        if (pipeAd.getType() != null && !pipeAd.getType().equals(PipeService.PropagateType)) {
            throw new IOException("Only propagate pipe advertisements are supported");
        }
        if (pipeAd.getPipeID() == null) {
            throw new IOException("Invalid pipe advertisement");
        }

        this.group = group;
        this.pipeAdv = pipeAd;
        pipeSvc = group.getPipeService();
        this.in = pipeSvc.createInputPipe(pipeAd, this);
        this.credentialDoc = getCredDoc(group);
        outputPipe = pipeSvc.createOutputPipe(pipeAd, 1);
        String id = group.getPeerID().toString();

        srcElement = new StringMessageElement(SRCIDTAG, id, null);
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Starting JxtaMulticastSocket on pipe id :" + pipeAdv.getID());
        }
        String pipeStr = pipeAd.getPipeID().getUniqueValue().toString();

        localAddress = InetAddress.getByAddress(pipeStr, fauxip);
        socketAddress = new InetSocketAddress(localAddress, 0);
        bound = true;
    }

    /**
     * Obtain the credential doc from the group object
     *
     * @param group group context
     * @return The credDoc value
     */
    protected static StructuredDocument getCredDoc(PeerGroup group) {
        try {
            MembershipService membership = group.getMembershipService();
            Enumeration each = membership.getCurrentCredentials();

            if (each.hasMoreElements()) {
                // get the only credential "nobody"
                Credential credential = (Credential) each.nextElement();

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
     * Returns the binding state of the MutlicastSocket.
     *
     * @return true if the MutlicastSocket successfully bound to an address
     */
    @Override
    public boolean isBound() {
        return bound;
    }

    /**
     * Closes this MutlicastSocket.
     */
    @Override
    public synchronized void close() {
        if (closed) {
            return;
        }
        bound = false;
        closed = true;
        in.close();
        outputPipe.close();
        queue.close();
        in = null;
    }

    /**
     * {@inheritDoc}
     */
    public void pipeMsgEvent(PipeMsgEvent event) {

        Message message = event.getMessage();

        if (message == null) {
            return;
        }

        MessageElement element = null;

        // does the message contain any data
        element = message.getMessageElement(NAMESPACE, DATATAG);
        if (element == null) {
            return;
        }
        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Pushing a message onto queue");
            }
            queue.push(message, -1);
        } catch (InterruptedException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Interrupted", e);
            }
        }
    }

    /**
     * Gets the Timeout attribute of the JxtaMulticastSocket
     *
     * @return The soTimeout value
     */
    @Override
    public synchronized int getSoTimeout() {
        return timeout;
    }

    /**
     * Sets the Timeout attribute of the JxtaMulticastSocket
     * a timeout of 0 blocks forever, by default this Socket's
     * timeout is set to 0
     *
     * @param timeout The new soTimeout value
     */
    @Override
    public synchronized void setSoTimeout(int timeout) throws SocketException {
        checkState();
        this.timeout = timeout;
    }

    /**
     * Returns the closed state of the JxtaMulticastSocket.
     *
     * @return true if the socket has been closed
     */
    @Override
    public synchronized boolean isClosed() {
        return closed;
    }

    /**
     * Throws a SocketException if closed or not bound
     *
     * @throws SocketException if closed
     */
    private void checkState() throws SocketException {
        if (isClosed()) {
            throw new SocketException("MulticastSocket is closed");
        } else if (!isBound()) {
            throw new SocketException("MulticastSocket not bound");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void send(DatagramPacket packet) throws IOException {
        checkState();
        byte[] data = new byte[packet.getLength()];

        System.arraycopy(packet.getData(), packet.getOffset(), data, 0, packet.getLength());

        Message msg = new Message();

        msg.addMessageElement(NAMESPACE, srcElement);
        msg.addMessageElement(NAMESPACE, new ByteArrayMessageElement(DATATAG, MimeMediaType.AOS, data, null));
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending a data packet");
        }
        InetAddress address = packet.getAddress();
        PeerID pid = null;

        if (address != null) {
            String pidStr = address.getHostName();

            try {
                pid = (PeerID) IDFactory.fromURI(new URI(pidStr));
            } catch (Exception ex) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Invalid source PeerID multicasting instead");
                }
            }
        }
        if (pid != null) {
            // Unicast datagram
            // create a op pipe to the destination peer
            OutputPipe op = pipeSvc.createOutputPipe(pipeAdv, Collections.singleton(pid), 1000);

            op.send(msg);
            op.close();
        } else {
            // multicast
            outputPipe.send(msg);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void receive(DatagramPacket packet) throws IOException {
        checkState();
        Message msg = null;
        // data
        MessageElement del = null;
        // src
        MessageElement sel = null;

        try {
            msg = (Message) queue.pop(timeout);
            if (msg == null) {
                if (timeout > 0) {
                    throw new SocketTimeoutException("Socket timeout reached");
                } else {
                    return;
                }
            }
            del = msg.getMessageElement(NAMESPACE, DATATAG);
            sel = msg.getMessageElement(NAMESPACE, SRCIDTAG);
            if (del == null || sel == null) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Message contains no data element, returning");
                }
                return;
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Popped a message off the queue");
                }
            }
        } catch (InterruptedException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Exception occured", e);
            }
            throw new IOException(e.toString());
        }
        if (del.getByteLength() > packet.getLength()) {
            throw new IOException("Datagram can not accomodate message of size :" + del.getByteLength());
        }
        String addrStr = new String(sel.getBytes(false), 0, (int) sel.getByteLength(), "UTF8");

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Src Address :" + addrStr);
        }
        InetAddress address = InetAddress.getByAddress(addrStr, fauxip);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Setting Data, and Src Address :" + address);
        }
        packet.setAddress(address);
        packet.setData(del.getBytes(false));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public InetAddress getLocalAddress() {
        if (isClosed()) {
            return null;
        }
        return localAddress;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public SocketAddress getLocalSocketAddress() {
        if (isClosed()) {
            return null;
        }
        return socketAddress;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void bind(SocketAddress addr) throws SocketException {
        if (isBound()) {
            throw new SocketException("Already bound");
        }
    }
}

