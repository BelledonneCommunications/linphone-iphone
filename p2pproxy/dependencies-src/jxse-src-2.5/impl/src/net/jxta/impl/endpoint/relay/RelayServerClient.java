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
package net.jxta.impl.endpoint.relay;

import java.io.IOException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.logging.Logger;
import java.util.logging.Level;

import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.logging.Logging;

import net.jxta.impl.endpoint.BlockingMessenger;
import net.jxta.impl.endpoint.EndpointServiceImpl;
import net.jxta.impl.util.TimeUtils;

/**
 * This class abstracts a client of the Relay Server
 */
class RelayServerClient implements Runnable {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(RelayServerClient.class.getName());

    /**
     * The lease length when there are messages pending and we can't send them.
     */
    private static long stallTimeout = 0;

    /**
     * the Relay Server of this client
     */
    private final RelayServer server;

    /**
     * the peerId of this client
     */
    private final String clientPeerId;

    /**
     * The length of the lease in milliseconds
     */
    private long leaseLength = 0;

    /**
     * the Endpoint Address of the client of the queue expressed as jxta://'peerid'
     */
    private final EndpointAddress clientAddr;

    /**
     * the time at which the message queue expires
     */
    private volatile long expireTime = 0;

    /**
     * indicates whether this client handler is valid or expired
     */
    private boolean isClosed = false;

    /**
     * a queue of message for this client
     */
    private final BlockingQueue<Message> messageList;

    /**
     * endpoint service for this client
     */
    private final EndpointService endpoint;

    private Messenger messenger = null;
    private Thread thread = null;
    private boolean thread_idle = false;

    private Message outOfBandMessage = null;

    protected RelayServerClient(RelayServer server, String clientPeerId, long leaseLength, long stallTimeout, int clientQueueSize) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("new Client peerId=" + clientPeerId + " lease=" + leaseLength);
        }

        this.server = server;
        this.clientPeerId = clientPeerId;
        this.leaseLength = leaseLength;
        RelayServerClient.stallTimeout = stallTimeout;

        clientAddr = new EndpointAddress("jxta", clientPeerId, null, null);
        endpoint = server.getEndpointService();
        messageList = new ArrayBlockingQueue<Message>(clientQueueSize);

        // initialize the lease
        renewLease();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void finalize() throws Throwable {
        closeClient();
        super.finalize();
    }

    /**
     * {@inheritDoc}
     *
     * <p/>Send all of the queued messages to the client.
     */
    public void run() {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("send queued messages to " + clientAddr);
        }

        try {
            Message message;
            int failedInARow = 0;

            while (true) {
                message = null;
                Messenger holdIt;
                synchronized (this) {
                    // Messenger + message is the condition to continue running
                    // We do not want to dequeue messages for sending before knowing if
                    // we have a messenger because re-queing is clumsy, so we
                    // check the messenger first. However, if we fail to get
                    // a messenger, we are forced to check the queue so that we
                    // can update the lease accordingly. It is possible to be here
                    // with neither messenger nor messages and then we must let
                    // the lease be long.

                    if (messenger == null || messenger.isClosed()) {
                        messenger = null;
                        if (outOfBandMessage != null || !messageList.isEmpty()) {

                            // If we cannot send a message by lack of messenger.
                            // The client is suspect of being dead. The clock starts
                            // ticking faster until we manage to send again.
                            // In two minutes we declare it dead.

                            long newExpireTime = TimeUtils.toAbsoluteTimeMillis(stallTimeout);

                            // If we're closed, we won't touch expireTime since it is 0.
                            if (expireTime > newExpireTime) {
                                expireTime = newExpireTime;
                            }

                        }
                        thread = null; // Make sure a thread will be created if
                        break; // it is needed after we release the synch.
                    }

                    if (outOfBandMessage != null) {
                        message = outOfBandMessage;
                        outOfBandMessage = null;
                    } else {
                        message = messageList.poll(0, TimeUnit.MILLISECONDS);
                        if (message == null) {
                            try {
                                thread_idle = true;
                                wait(4 * TimeUtils.ASECOND);
                                if (outOfBandMessage != null) {
                                    message = outOfBandMessage;
                                    outOfBandMessage = null;
                                } else {
                                    message = messageList.poll(0, TimeUnit.MILLISECONDS);
                                }
                            } catch (InterruptedException ie) {
                                //ignored
                            }
                            if (message == null) {
                                thread = null; // Make sure a thread will be created if
                                break; // it is needed after we release the synch.
                            }
                        }
                    }

                    holdIt = messenger; // Avoid NPE once out of synch.
                    thread_idle = false;
                }

                // get the final service name and parameter that was loaded before queueing
                MessageElement dstAddressElement = message.getMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NS,
                        EndpointServiceImpl.MESSAGE_DESTINATION_NAME);

                if (null == dstAddressElement) {
                    // No destination address... Just discard
                    // this should really not happen
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("message destination was not set");
                    }

                    continue;
                }
                EndpointAddress destAddr = new EndpointAddress(dstAddressElement.toString());

                // send the message
                try {
                    holdIt.sendMessageB(message, destAddr.getServiceName(), destAddr.getServiceParameter());

                    // The client is off the hook for now. One message was sent.
                    // Lease will stay long until the next messenger failure.
                    synchronized (this) {
                        failedInARow = 0;

                        // Do not touch expireTime if we've been closed.
                        if (!isClosed) {
                            expireTime = TimeUtils.toAbsoluteTimeMillis(leaseLength);
                        }
                    }
                } catch (Exception e) {
                    // Check that the exception is not due to the message
                    // rather than the messenger, and then drop the message. In that case
                    // we give the messenger the benefit of the doubt and keep
                    // it open, renewing the lease as above. (this could be the last
                    // message). For now the transports do not tell the difference, so we
                    // count the nb of times we failed in a row. After three times,
                    // kill the message rather than the messenger.

                    // put the message back
                    synchronized (this) {
                        if (++failedInARow >= 3) {
                            failedInARow = 0;
                            if (!isClosed) {
                                expireTime = TimeUtils.toAbsoluteTimeMillis(leaseLength);
                            }
                            continue;
                        }

                        // Ok, if we cannot push back the message, below, we 
                        // should reset failedInARow, since we won't be retrying
                        // the same message. But it does not realy matter so 
                        // let's keep things simple.

                        if (outOfBandMessage == null) {
                            outOfBandMessage = message;
                        }
                    }

                    // If we're here, we decided to close the messenger. We do that
                    // out of sync.
                    if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                        LOG.log(Level.INFO, "closing messenger after exception :" + clientAddr, e);
                    }
                    holdIt.close(); // Next loop deal with it.
                    // (including shortening the lease if needed.)
                }
            }
        } catch (Throwable all) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            }
        } finally {
            thread = null;

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("stopped sending queued messages for " + clientAddr);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        return clientPeerId + "," + messageList.size() + ","
                + (messenger == null ? "-m" : "+m") + "," + TimeUtils.toRelativeTimeMillis(expireTime, TimeUtils.timeNow());
    }

    protected int getQueueSize() {
        return messageList.size();
    }

    public long getLeaseRemaining() {
        // May be shorter than lease length. Compute real value from expire
        // time.
        return TimeUtils.toRelativeTimeMillis(expireTime, TimeUtils.timeNow());
    }

    public void closeClient() {

        Messenger messengerToClose;

        synchronized (this) {
            if (isClosed) {
                return;
            }

            isClosed = true;

            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info( "Terminating client:" + "\n\tclient=" + clientAddr + "\tnbMessages=" + messageList.size()
                                + "\tmessenger=" + messenger + (messenger == null ? "" : "(c:" + messenger.isClosed() + ")")
                                + "\tlease-left=" + TimeUtils.toRelativeTimeMillis(expireTime, TimeUtils.timeNow()) + "\tt=" + (thread != null));
            }

            messengerToClose = messenger;

            expireTime = 0;
            messenger = null;

            // remove all queued messages if expired
            messageList.clear();
        }

        // We can do that out of sync. It avoids nesting locks.
        server.removeClient(clientPeerId, this);
        if (messengerToClose != null) {
            messengerToClose.close();
        }
    }

    /**
     * remove all queued messages.
     */
    synchronized void flushQueue() {
        messageList.clear();
    }

    public boolean addMessenger(Messenger newMessenger) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("addMessenger() " + newMessenger);
        }

        // make sure we are being passed a valid messenger
        if (newMessenger == null || newMessenger.isClosed()) {
            return false;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("messenger (" + messenger + ") thread=" + thread);
        }

        // Unless we change our mind, we'll close the new messenger.
        // If we do not keep it, we must close it. Otherwise
        // the client on the other end will never know what happened.
        // Its connection will be left hanging for a long time.

        Messenger messengerToClose = newMessenger;

        synchronized (this) {
            // Do not use isExpired() here. IsExpired is not supposed to be called
            // synchronized. Also  isClosed() is good enough. The handler being
            // expired is not a problem; we'll figure it out soon enough and do the
            // right thing.

            if (!isClosed) {
                // use this messenger instead of the old one.

                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    if (messenger != null) {
                        LOG.info("closing messenger replaced by a new one : " + clientAddr);
                    }
                }

                // Swap messengers; we'll close the old one if there was one.

                messengerToClose = messenger;
                messenger = newMessenger;

                if ((thread == null || thread_idle) && ((!messageList.isEmpty()) || (outOfBandMessage != null))) {

                    // if we do not already have a thread, start one

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("messageList.size() = " + messageList.size() + " client=" + clientAddr);
                    }

                    if (thread != null) {
                        notify();
                    } else {
                        thread = new Thread(server.group.getHomeThreadGroup(), this, "Draining queue to " + clientAddr);
                        thread.setDaemon(true);
                        thread.start();
                    }
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("messenger (" + messenger + ") thread=" + thread);
                }
            }
        }

        // Close whichever messenger out of sync.
        // In either case, we claim that we kept the new one.

        if (messengerToClose != null) {
            messengerToClose.close();
        }

        return true;
    }

    public boolean isExpired() {
        boolean isExpired = TimeUtils.timeNow() > expireTime;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("isExpired() = " + isExpired + " client=" + clientAddr);
        }

        if (isExpired) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Closing expired client : " + clientAddr);
            }
            closeClient();
        }

        return isExpired;
    }

    public synchronized boolean renewLease() {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("renewLease() old expireTime = " + expireTime);
        }

        // It is ok to renew a lease past the expiration time, as long
        // as the handler has not been closed yet. So, we do not use
        // isExpired().

        if (isClosed) {
            return false;
        }

        // As long as there are messages to send, the lease is controlled
        // by our ability to send them, not by client renewal.

        if (!messageList.isEmpty()) {
            return true;
        }

        expireTime = TimeUtils.toAbsoluteTimeMillis(leaseLength);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("renewLease() new expireTime = " + expireTime);
        }
        return true;
    }

    /**
     * add a message to the tail of the list
     *
     * @param message  the message
     * @param outOfBand if true, indicates outbound
     * @throws IOException if an io error occurs
     */
    private void queueMessage(Message message, boolean outOfBand) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("queueMessage (" + messageList.size() + ") client=" + clientAddr);
        }

        synchronized (this) {
            if (isClosed) {
                throw new IOException("Client has been disconnected");
            }

            if (outOfBand) {
                // We have a single oob message pending.
                outOfBandMessage = message;
            } else {
                // We will simply discard the latest msg when the queue is full
                // to avoid penalty of dropping earlier reliable message
                if (!messageList.offer(message)) {
                    if (Logging.SHOW_WARNING) {
                        LOG.warning("Dropping relayed message " + message.toString() + " for peer " + clientPeerId);
                    }
                }
            }

            // check if a new thread needs to be started.
            if ((thread == null) || thread_idle) {
                // Normally, if messenger is null we knew it already:
                // it becomes null only when we detect that it breaks while
                // trying to send. However, let's imagine it's possible that
                // we never had one so far. Be carefull that this is not a
                // one-time event; we must not keep renewing the short lease;
                // that would ruin it's purpose.

                if (messenger == null) {
                    long newExpireTime = TimeUtils.toAbsoluteTimeMillis(stallTimeout);

                    if (expireTime > newExpireTime) {
                        expireTime = newExpireTime;
                    }

                } else {
                    // Messenger good.
                    // if we do not already have a thread, start one
                    if (thread != null) {
                        notify();
                    } else {
                        thread = new Thread(server.group.getHomeThreadGroup(), this, "Draining queue to " + clientAddr);
                        thread.setDaemon(true);
                        thread.start();
                    }
                }
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("done queueMessage (" + messageList.size() + ") client=" + clientAddr);
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("messenger (" + messenger + ") thread=" + thread);
        }
    }

    protected EndpointAddress getClientAddress() {
        return clientAddr;
    }

    protected Messenger getMessenger(EndpointAddress srcAddr, EndpointAddress destAddr, boolean outOfBand) {
        return new RelayMessenger(srcAddr, destAddr, this, outOfBand);
    }

    protected static class RelayMessenger extends BlockingMessenger {
        private final MessageElement srcAddressElement;
        private final RelayServerClient handler;
        private boolean outOfBand = false;

        // Since we send messages through other messengers that do not necessarily have the
        // same destination service and param (usually none), we have to pass these along explicitly
        // in all cases. If we just build a destination element for the message it will be overwritten
        // by messengers below.
        private final String defaultServiceName;
        private final String defaultServiceParam;

        public RelayMessenger(EndpointAddress srcAddress, EndpointAddress destAddress, RelayServerClient handler, boolean outOfBand) {

            // We do not use self destruction
            super(handler.server.group.getPeerGroupID(), destAddress, false);

            this.defaultServiceName = destAddress.getServiceName();
            this.defaultServiceParam = destAddress.getServiceParameter();
            this.handler = handler;
            this.outOfBand = outOfBand;

            this.srcAddressElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NAME, srcAddress.toString(), null);
        }

        /*
        * The cost of just having a finalize routine is high. The finalizer is
        * a bottleneck and can delay garbage collection all the way to heap
        * exhaustion. Leave this comment as a reminder to future maintainers.
        * Below is the reason why finalize is not needed here.
        *
        * This is never given to application layers directly. No need
        * to close-on-finalize.
        *

        protected void finalize() {
        }

        */

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean isIdleImpl() {
            // We do not use self destruction
            return false;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void closeImpl() {
            // Nothing to do. The underlying connection is not affected.
            // The messenger will be marked closed by the state machine once completely down; that's it.
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public EndpointAddress getLogicalDestinationImpl() {
            // getClientAddress returns a clone of the client's jxta: address.
            return handler.getClientAddress();
        }

        /*
        *   {@inheritDoc}
        *
        * <p/>Send messages. Messages are queued and then processed when there is a transport messenger.
        */
        @Override
        public void sendMessageBImpl(Message message, String serviceName, String serviceParam) throws IOException {

            // Set the message with the appropriate src address
            message.replaceMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NS, srcAddressElement);

            // load the final destination into the message
            EndpointAddress destAddressToUse = getDestAddressToUse(serviceName, serviceParam);

            MessageElement dstAddressElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NAME,
                    destAddressToUse.toString(), null);

            message.replaceMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NS, dstAddressElement);

            // simply enqueue the message.
            // We clone it, since we pretend it's been sent synchronously.
            handler.queueMessage(message.clone(), outOfBand);
        }
    }
}
