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

import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.Messenger;
import net.jxta.id.ID;
import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.util.UnbiasedQueue;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeID;
import net.jxta.protocol.PipeAdvertisement;

import java.io.IOException;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * An implementation of Ouput Pipe which sends messages on the pipe
 * asynchronously. The <code>send()</code> method for this implementation will
 * never block.
 */
class NonBlockingOutputPipe implements PipeResolver.Listener, OutputPipe, Runnable {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(NonBlockingOutputPipe.class.getName());

    /**
     * Amount of time an idle worker thread will linger
     */
    private static final long IDLEWORKERLINGER = 10 * TimeUtils.ASECOND;

    /**
     * Minimum Query interval. Queries will not be sent more frequently than
     * this interval.
     */
    private static final long QUERYINTERVALMIN = 15 * TimeUtils.ASECOND;

    /**
     * Query timeout minimum. Waits for query response will not be shorter than
     * this interval.
     */
    private static final long QUERYTIMEOUTMIN = 1 * TimeUtils.AMINUTE;

    /**
     * If true then the pipe has been closed and will no longer accept messages.
     */
    private volatile boolean closed = false;

    /**
     * If true then this pipe has just migrated. Used to prevent re-entering
     * migration from an unfinished migration.
     */
    private boolean migrated = false;

    /**
     * Group in which we are working.
     */
    private PeerGroup peerGroup = null;

    /**
     * The endpoint of our group.
     */
    private EndpointService endpoint = null;

    /**
     * The pipe resolver we will use for migrate and verify.
     */
    private PipeResolver pipeResolver = null;

    /**
     * The advertisement we were created from.
     */
    private PipeAdvertisement pAdv = null;

    /**
     * The current peer the pipe is resolved to.
     */
    private ID destPeer = null;

    /**
     * The set of peers to which the pipe can be resolved.
     */
    private Set<? extends ID> resolvablePeers = null;

    /**
     * The endpoint destination address for the remote peer we are resolved to.
     */
    private EndpointAddress destAddress = null;
    private Messenger destMessenger = null;

    /**
     * The worker thread which actually sends messages on the pipe
     */
    private volatile Thread serviceThread = null;

    /**
     * Absolute time in milliseconds at which we will send the next verify
     * request.
     */
    private long nextVerifyAt = 0;

    /**
     * Queue of messages waiting to be sent.
     */
    private final UnbiasedQueue queue = UnbiasedQueue.synchronizedQueue(new UnbiasedQueue(50, false));

    /**
     * Tracks the state of our worker thread.
     */
    enum WorkerState {
        /**
         * Find a new eligible destination peer which is listening on the pipe.
         */
        STARTMIGRATE, 
        
        /**
         * Issue resolution queries and wait for responses
         */ 
        PENDINGMIGRATE, 
         
         /**
         * Determine if the destination peer is still listening on the pipe.
         */ 
        STARTVERIFY, 
        
        /**
         * Issue verify queries and wait for responses
         */ 
        PENDINGVERIFY, 
        
        /**
         * Acquire a messenger to the destination peer.
         */ 
        ACQUIREMESSENGER, 
        
        /**
         * Send messages via the messenger to the destination peer.
         */ 
        SENDMESSAGES, 
        
        /**
         * Exit.
         */ 
        CLOSED
    }

    /**
     * The current state of the worker thread
     */
    private WorkerState workerstate;

    /**
     * The query id we are currently operating under.
     */
    private int queryID = -1;

    /**
     * Create a new output pipe
     *
     * @param peerGroup        peergroup we are working in.
     * @param pipeResolver        the piperesolver this pipe is bound to.
     * @param pAdv     advertisement for the pipe we are supporting.
     * @param destPeer the peer this pipe is currently bound to.
     * @param peers    the set of peers we allow this pipe to be bound to.
     */
    public NonBlockingOutputPipe(PeerGroup peerGroup, PipeResolver pipeResolver, PipeAdvertisement pAdv, ID destPeer, Set<? extends ID> peers) {

        this.peerGroup = peerGroup;
        endpoint = peerGroup.getEndpointService();
        this.pipeResolver = pipeResolver;

        this.pAdv = pAdv;
        this.destPeer = destPeer;
        this.resolvablePeers = new HashSet<ID>(peers);

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Constructing for " + getPipeID());
        }
        workerstate = WorkerState.ACQUIREMESSENGER;
        startServiceThread();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void finalize() throws Throwable {
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
    public synchronized void close() {

        // Close the queue so that no more messages are accepted
        if (!closed) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Closing for " + getPipeID());
            }
            queue.close();
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
    public boolean send(Message msg) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Queuing " + msg + " for pipe " + getPipeID());
        }

        boolean pushed = false;
        while (!queue.isClosed()) {
            try {
                pushed = queue.push(msg, 250 * TimeUtils.AMILLISECOND);
                break;
            } catch (InterruptedException woken) {
                Thread.interrupted();
            }
        }

        if (!pushed && queue.isClosed()) {
            IOException failed = new IOException("Could not enqueue " + msg + " for sending. Pipe is closed.");
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, failed.getMessage(), failed);
            }
            throw failed;
        }
        startServiceThread();
        return pushed;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Sends the messages.
     * <p/>
     * This method does a lot of things. It has several distinct states:
     * <p/>
     * <table border="1">
     * <thead>
     * <tr>
     * <th>STATE</th>
     * <th>Activity</th>
     * <tr>
     * </thead>
     * <p/>
     * <tbody>
     * <tr>
     * <th>ACQUIREMESSENGER</th
     * <td>Acquire a messenger to the specified destination peer. If a
     * messenger is acquired, then go to <b>SENDMESSAGES</b> state
     * otherwise go to <b>STARTMIGRATE</b>.</td>
     * </tr>
     * <p/>
     * <tr>
     * <th>SENDMESSAGES</th>
     * <td>Send messages until queue is closed and all messages have
     * been sent. Go to state <b>CLOSED</b> when done. If the messenger
     * becomes closed then go to <b>ACQUIREMESSENGER</b>. <emphasis>If
     * there are no messages to send for <code>IDLEWORKERLINGER</code>
     * milliseconds then the worker thread will exit. It will only be
     * restarted if another message is eventually enqueued.</emphasis>
     * </td>
     * </tr>
     * <p/>
     * <tr>
     * <th>STARTVERIFY</th>
     * <td>Starts a verification query(s) to the destination peer. This
     * state is activated after
     * <code>PipeServiceImpl.VERIFYINTERVAL</code> milliseconds of
     * sending messages. The query responses will be tracked in the
     * <b>PENDINGVERIFY</b> state.</td>
     * </tr>
     * <p/>
     * <tr>
     * <th>STARTMIGRATE</th>
     * <td>Starts a query(s) for peers listening on this pipe. The
     * query responses will be tracked in the <b>PENDINGMIGRATE</b>
     * state.</td>
     * </tr>
     * <p/>
     * <tr>
     * <th>PENDINGVERIFY</th>
     * <td>Issues query messages to verify that the destination peer is
     * still listening on the pipe. Queries are issued every
     * <code>QUERYINTERVAL</code> milliseconds. If a positive response
     * is received, go to state <b>ACQUIREMESSENGER</b>. If no response
     * is received within <b>QUERYTIMEOUT</b> milliseconds or a
     * negative response is received then go to state
     * <b>STARTMIGRATE</b>.</td>
     * </tr>
     * <p/>
     * <tr>
     * <th>PENDINGMIGRATE</th>
     * <td>Issues query messages to find a new destination peer.
     * Queries are issued every <code>QUERYINTERVAL</code> milliseconds.
     * If a positive response is received, go to state
     * <b>ACQUIREMESSENGER</b>. If no positive response from an
     * eligible peer is received within <b>QUERYTIMEOUT</b>
     * milliseconds go to state <b>CLOSED</b>.</td>
     * </tr>
     * <p/>
     * <tr>
     * <th>CLOSED</th>
     * <td>Exit the worker thread.</td>
     * </tr>
     * </tbody>
     * </table>
     */
    public void run() {
        long absoluteTimeoutAt = -1;
        long nextQueryAt = -1;

        try {
            // state loop
            while (WorkerState.CLOSED != workerstate) {
                synchronized (this) {
                    LOG.fine("NON-BLOCKING WORKER AT STATE : " + workerstate
                            + ((WorkerState.SENDMESSAGES == workerstate)
                                    ? "\n\t" + TimeUtils.toRelativeTimeMillis(nextVerifyAt, TimeUtils.timeNow())
                                    + " until verify."
                                    : ""));

                    // switch() emulation
                    if ((WorkerState.STARTVERIFY == workerstate) || (WorkerState.STARTMIGRATE == workerstate)) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            if (null == destPeer) {
                                LOG.fine("Starting re-resolve for \'" + getPipeID());
                            } else {
                                LOG.fine("Starting verify for \'" + getPipeID() + "\' to : " + destPeer);
                            }
                        }

                        queryID = PipeResolver.getNextQueryID();
                        pipeResolver.addListener(getPipeID(), this, queryID);
                        absoluteTimeoutAt = TimeUtils.toAbsoluteTimeMillis(
                                Math.max(QUERYTIMEOUTMIN, (PipeServiceImpl.VERIFYINTERVAL / 20)));
                        nextQueryAt = TimeUtils.timeNow();

                        if (WorkerState.STARTVERIFY == workerstate) {
                            workerstate = WorkerState.PENDINGVERIFY;
                        } else if (WorkerState.STARTMIGRATE == workerstate) {
                            workerstate = WorkerState.PENDINGMIGRATE;
                        }

                        // move on to the next state.
                    } else if ((WorkerState.PENDINGVERIFY == workerstate) || (WorkerState.PENDINGMIGRATE == workerstate)) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(
                                    "Pipe " + ((WorkerState.PENDINGVERIFY == workerstate) ? "verify" : "migrate")
                                    + "in progress. Continues for "
                                    + TimeUtils.toRelativeTimeMillis(absoluteTimeoutAt, TimeUtils.timeNow())
                                    + "ms. Next query in " + TimeUtils.toRelativeTimeMillis(nextQueryAt, TimeUtils.timeNow())
                                    + "ms.");
                        }

                        // check to see if we are completely done.
                        if (TimeUtils.toRelativeTimeMillis(absoluteTimeoutAt, TimeUtils.timeNow()) <= 0) {
                            pipeResolver.removeListener(getPipeID(), queryID);

                            if (WorkerState.PENDINGVERIFY == workerstate) {
                                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                                    LOG.info("Pipe \'" + getPipeID() + "\' has migrated from " + destPeer);
                                }
                                workerstate = WorkerState.STARTMIGRATE;
                                // move on to the next state.
                                continue;
                            } else {
                                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                    LOG.warning("Pipe \'" + getPipeID() + "\' cannot be migrated and is being closed");
                                }
                                workerstate = WorkerState.CLOSED;
                                close();
                                // move on to the next state.
                                continue;
                            }
                        }

                        // check if its time ot send another copy of the query.
                        if (TimeUtils.toRelativeTimeMillis(nextQueryAt, TimeUtils.timeNow()) <= 0) {
                            if (null != destPeer) {
                                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                    LOG.fine(
                                            "Sending out verify query (" + queryID + ") for \'" + getPipeID() + "\' to : "
                                            + destPeer);
                                }
                                pipeResolver.sendPipeQuery(pAdv, Collections.singleton(destPeer), queryID);
                            } else {
                                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                    LOG.fine("Sending out resolve query (" + queryID + ") for " + getPipeID());
                                }
                                pipeResolver.sendPipeQuery(pAdv, resolvablePeers, queryID);
                            }
                            nextQueryAt = TimeUtils.toAbsoluteTimeMillis(
                                    Math.max(QUERYINTERVALMIN, (PipeServiceImpl.VERIFYINTERVAL / 50)));
                        }

                        long sleep = TimeUtils.toRelativeTimeMillis(Math.min(nextQueryAt, absoluteTimeoutAt), TimeUtils.timeNow());

                        if (sleep >= 0) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Waiting " + sleep + "ms for response for (" + queryID + ") for " + getPipeID());
                            }
                            try {
                                wait(sleep);
                            } catch (InterruptedException woken) {
                                Thread.interrupted();
                            }
                        }

                        // move on to the next state.
                    } else if (WorkerState.ACQUIREMESSENGER == workerstate) {
                        if ((null == destMessenger) || destMessenger.isClosed()) {
                            destMessenger = null;
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Getting messenger to \'" + destPeer + "\' for pipe " + getPipeID());
                            }

                            destAddress = mkAddress(destPeer, getPipeID());
                            // todo 20031011 bondolo@jxta.org This should not be done under sync
                            destMessenger = endpoint.getMessenger(destAddress);
                            if (destMessenger == null) {
                                // We could not get a messenger to the peer, forget it and try again.
                                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                    LOG.warning("Could not get messenger to : " + destPeer + ". ");
                                }

                                if (migrated) {
                                    // we can't migrate again, we never finished.
                                    // the last migrate!
                                    workerstate = WorkerState.CLOSED;
                                    close();
                                } else {
                                    workerstate = WorkerState.STARTMIGRATE;
                                }
                                pipeResolver.removeListener((PipeID) getPipeID(), queryID);
                                queryID = -1;
                                destPeer = null;
                                destAddress = null;

                                // move on to the next state.
                                continue;
                            } else {
                                // migration completed.
                                migrated = false;
                            }
                        } else {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Using existing messenger to : " + destPeer);
                            }
                        }

                        workerstate = WorkerState.SENDMESSAGES;
                        nextVerifyAt = TimeUtils.toAbsoluteTimeMillis(PipeServiceImpl.VERIFYINTERVAL);

                        // move on to the next state.
                        continue; // can't just fall through because we would start sending messages immediately.
                    } else if (WorkerState.SENDMESSAGES == workerstate) {
                        // is it time to do verification again?
                        if (TimeUtils.toRelativeTimeMillis(nextVerifyAt, TimeUtils.timeNow()) <= 0) {
                            workerstate = WorkerState.STARTVERIFY;
                            pipeResolver.removeListener(getPipeID(), queryID);
                            queryID = -1;
                        }
                        // move on to the next state.
                    } else if (WorkerState.CLOSED == workerstate) {
                        queue.clear(); // they aren't going to be sent
                        if (null != destMessenger) {
                            destMessenger.close();
                            destMessenger = null;
                        }
                        serviceThread = null;
                        break;
                    } else {
                        LOG.warning("Unrecognized state in worker thread : " + workerstate);
                    }
                }

                // now actually send messages. We don't do this under the global sync.
                if (WorkerState.SENDMESSAGES == workerstate) {
                    Message msg = null;

                    try {
                        msg = (Message) queue.pop(IDLEWORKERLINGER);
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                        continue;
                    }

                    if (null == msg) {
                        synchronized (this) {
                            // before deciding to die, we need to make sure that
                            // nobody snuck something into the queue. If there
                            // is, then we have to be the one to service the
                            // queue.
                            if (null == queue.peek()) {
                                if (closed) {
                                    workerstate = WorkerState.CLOSED;
                                    continue;
                                } else {
                                    serviceThread = null;
                                    break;
                                }
                            } else {
                                continue;
                            }
                        }
                    }

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Sending " + msg + " on " + getPipeID());
                    }

                    if (!destMessenger.isClosed()) {
                        try {
                            destMessenger.sendMessageB(msg, null, null);
                        } catch (IOException failed) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Failure sending " + msg + " on " + getPipeID(), failed);
                            }
                        }
                    }

                    // May be now closed due to failing to send.
                    if (destMessenger.isClosed()) {
                        synchronized (this) {
                            workerstate = WorkerState.ACQUIREMESSENGER;
                            destMessenger = null;
                        }
                    }
                }
            }
        } catch (Throwable all) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            }

            // give another thread the chance to start unless one already has.
            // If the exception was caused by damaged state on this object then
            // starting a new Thread may just cause the same exception again.
            // Unfortunate tradeoff.
            synchronized (this) {
                if (serviceThread == Thread.currentThread()) {
                    serviceThread = null;
                }
            }
        } finally {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info(
                        "Thread exit : " + Thread.currentThread().getName() + "\n\tworker state : " + workerstate
                        + "\tqueue closed : " + queue.isClosed() + "\tnumber in queue : " + queue.getCurrentInQueue()
                        + "\tnumber queued : " + queue.getNumEnqueued() + "\tnumber dequeued : " + queue.getNumDequeued());
            }
        }
    }

    /**
     * Starts the worker thread if it is not already running.
     */
    private synchronized void startServiceThread() {
        // if there is no service thread, start one.
        if ((null == serviceThread) && !closed) {
            serviceThread = new Thread(peerGroup.getHomeThreadGroup(), this
                    ,
                    "Worker Thread for NonBlockingOutputPipe : " + getPipeID());
            serviceThread.setDaemon(true);
            serviceThread.start();
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info(
                        "Thread start : " + serviceThread.getName() + "\n\tworker state : " + workerstate + "\tqueue closed : "
                        + queue.isClosed() + "\tnumber in queue : " + queue.getCurrentInQueue() + "\tnumber queued : "
                        + queue.getNumEnqueued() + "\tnumber dequeued : " + queue.getNumDequeued());
            }
        }
    }

    /**
     * Convenience method for constructing a peer endpoint address from its
     * peer id
     *
     * @param destPeer the desitnation peer
     * @param pipeID   the pipe to put in the param field.
     * @return the pipe endpoint address.
     */
    protected EndpointAddress mkAddress(ID destPeer, ID pipeID) {
        return new EndpointAddress("jxta", destPeer.getUniqueValue().toString(), "PipeService", pipeID.toString());
    }

    /**
     * {@inheritDoc}
     */
    public synchronized boolean pipeNAKEvent(PipeResolver.Event event) {

        if (((workerstate == WorkerState.PENDINGVERIFY) || (workerstate == WorkerState.ACQUIREMESSENGER)
                || (workerstate == WorkerState.SENDMESSAGES))
                && (event.getPeerID().equals(destPeer) && (event.getQueryID() == queryID))) {
            // we have been told that the destination peer no longer wants
            // to talk with us. We will try to migrate to another peer.
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Pipe \'" + getPipeID() + "\' is closed at " + event.getPeerID());
            }

            workerstate = WorkerState.STARTMIGRATE;
            pipeResolver.removeListener(getPipeID(), queryID);
            queryID = -1;
            destPeer = null;
            destAddress = null;
            if (null != destMessenger) {
                destMessenger.close();
                destMessenger = null;
            }
            notify();
            return true;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Ignoring NAK from " + event.getPeerID());
        }

        // didn't refer to us or we don't care.
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized boolean pipeResolveEvent(PipeResolver.Event event) {

        if (((workerstate == WorkerState.PENDINGVERIFY) || (workerstate == WorkerState.PENDINGMIGRATE))
                && (event.getQueryID() == queryID)) {
            if ((workerstate == WorkerState.PENDINGVERIFY) && !event.getPeerID().equals(destPeer)) {
                // not from the right peer so ignore it.

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Ignoring response from " + event.getPeerID());
                }
                return false;
            } else {
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Pipe \'" + getPipeID() + "\' is verified for " + destPeer);
                }
            }

            workerstate = WorkerState.ACQUIREMESSENGER;
            migrated = true;
            destPeer = event.getPeerID();

            if ((workerstate == WorkerState.PENDINGMIGRATE) && Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Pipe \'" + getPipeID() + "\' has migrated to " + destPeer);
            }
            notify();
            return true;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Ignoring resolve from " + event.getPeerID());
        }
        // didn't refer to us or we don't care.
        return false;
    }
}
