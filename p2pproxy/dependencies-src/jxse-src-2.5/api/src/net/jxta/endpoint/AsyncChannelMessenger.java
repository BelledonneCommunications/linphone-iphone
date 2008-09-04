/*
 * Copyright (c) 2004-2007 Sun Microsystems, Inc.  All rights reserved.
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
package net.jxta.endpoint;

import net.jxta.peergroup.PeerGroupID;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

/**
 * Extends Channel Messenger behaviour to provide asynchronous message sending
 * via queuing.
 */
public abstract class AsyncChannelMessenger extends ChannelMessenger {

    /*
     *  Logger
     * private final static transient Logger LOG = Logger.getLogger(AsyncChannelMessenger.class.getName());
     */

    /**
     * {@code true} if we have deliberately closed our one message input queue.
     */
    private boolean inputClosed = false;

    /**
     * {@code true} if we have deliberately stopped sending.
     */
    private boolean outputClosed = false;

    /**
     * Actions that we defer to after returning from event methods. In other
     * words, they cannot be done with the lock held, or they require calling
     * more event methods.
     */
    private enum DeferredAction {

        /**
         * No action deferred.
         */
        ACTION_NONE,
        /**
         * Must send the current message.
         */
        ACTION_SEND,
        /**
         * Must report failure to connect.
         */
        ACTION_CONNECT
    }

    /**
     * The current deferred action.
     */
    private DeferredAction deferredAction = DeferredAction.ACTION_NONE;

    /**
     * The messages queue.
     */
    private final BlockingQueue<PendingMessage> queue;

    /**
     * State lock and engine.
     */
    private final AsyncChannelMessengerState stateMachine;

    /**
     * Our statemachine implementation; just connects the standard MessengerState action methods to
     * this object.
     */
    private class AsyncChannelMessengerState extends MessengerState {

        protected AsyncChannelMessengerState(boolean connected) {
            super(connected);
        }

        /*
         * The required action methods.
         */

        /**
         * {@inheritDoc}
         */
        @Override
        protected void connectAction() {
            deferredAction = DeferredAction.ACTION_CONNECT;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void startAction() {
            deferredAction = DeferredAction.ACTION_SEND;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void closeInputAction() {
            // We're synchronized here. (invoked from stateMachine)
            inputClosed = true;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void closeOutputAction() {
            // We're synchronized here. (invoked from stateMachine)
            outputClosed = true;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void failAllAction() {

            // The queue is now closed, so we can rest assured that the last
            // message is really the last one. This is a synchronous action. The
            // state machine assumes that it is done when we return. There is no
            // need to signal completion with an idleEvent.
            PendingMessage theMsg;

            while (true) {
                theMsg = null;

                synchronized (stateMachine) {
                    theMsg = queue.poll();
                }

                if (theMsg == null) {
                    return;
                }

                Message currentMsg = theMsg.msg;
                Throwable currentFailure = theMsg.failure;

                if (currentFailure == null) {
                    currentFailure = new IOException("Messenger unexpectedly closed");
                }

                OutgoingMessageEvent event = new OutgoingMessageEvent(currentMsg, currentFailure);

                currentMsg.setMessageProperty(Messenger.class, event);
            }
        }
    }


    /**
     * The representation of a queued message. It is shared between this
     * abstract class and any implementation.
     */
    protected static class PendingMessage {
        final Message msg;
        final String service;
        final String param;
        Throwable failure;

        PendingMessage(Message msg, String service, String param) {
            this.msg = msg;
            this.service = service;
            this.param = param;
            this.failure = null;
        }
    }

    /**
     * Create a new AsyncChannelMessenger.
     *
     * @param baseAddress      The network address messages go to; regardless of
     *                         service, param, or group.
     * @param redirection      Group to which the messages must be redirected. This
     *                         is used to implement the automatic group segregation which has become a
     *                         de-facto standard. If not null, the unique portion of the specified
     *                         groupID is prepended with {@link #InsertedServicePrefix} and inserted in
     *                         every message's destination address in place of the the original service
     *                         name, which gets shifted into the beginning of the service parameter. The
     *                         opposite is done on arrival to restore the original destination address
     *                         before the message is delivered to the listener in the the specified
     *                         group. Messages that already bear a group redirection are not affected.
     * @param origService      The default destination service for messages sent
     *                         without specifying a different service.
     * @param origServiceParam The default destination service parameter for
     *                         messages sent without specifying a different service parameter.
     * @param queueSize        the queue size that channels should have.
     * @param connected        true if the channel is created in the connected state.
     */
    public AsyncChannelMessenger(EndpointAddress baseAddress, PeerGroupID redirection, String origService, String origServiceParam, int queueSize, boolean connected) {

        super(baseAddress, redirection, origService, origServiceParam);

        stateMachine = new AsyncChannelMessengerState(connected);

        queue = new ArrayBlockingQueue<PendingMessage>(queueSize);

        // We synchronize our state with the sharedMessenger's stateMachine.
        // Logic would dictate that we pass it to super(), but it is not itself
        // constructed until super() returns. No way around it.

        setStateLock(stateMachine);
    }

    /**
     * {@inheritDoc}
     */
    public final void close() {
        DeferredAction action;

        synchronized (stateMachine) {
            stateMachine.closeEvent();
            action = eventCalled(true);
        }

        // We called an event. State may have changed.
        notifyChange();

        performDeferredAction(action);
    }

    /**
     * This internal method does the common part of sending the message on
     * behalf of both sendMessageN and sendMessageB.
     * <p/>It is not quite possible to implement sendMessageB as a wrapper
     * around sendMessageN without some internal cooperation. At least not in
     * an efficient manner. sendMessageB must not set the message property:
     * either it fails and throws, or it returns successfully and the property
     * is set later. This is required so that messages can be retried when
     * failing synchronously (through a blocking messenger typically, but the
     * semantic has to be uniform).
     * <p/>Each of sendMessageB and sendMessageN takes care of status reporting
     * on its own terms.
     *
     * @param msg           the message to send
     * @param rService      destination service
     * @param rServiceParam destination param
     * @return The outcome from that one attempt. {@code true} means done.
     *         {@code false} means saturated.  When {@code true} is returned, it means
     *         that the fate of the message will be decided asynchronously, so we do
     *         not have any details, yet.
     * @throws IOException          is thrown if this messenger is closed.
     * @throws InterruptedException if interrupted
     */
    private boolean sendMessageCommon(Message msg, String rService, String rServiceParam) throws IOException, InterruptedException {

        String service = effectiveService(rService);
        String serviceParam = effectiveParam(rService, rServiceParam);
        boolean queued = true;
        boolean change = false;
        DeferredAction action = DeferredAction.ACTION_NONE;

        synchronized (stateMachine) {
            if (inputClosed) {
                throw new IOException("This messenger is closed. It cannot be used to send messages.");
            }

            boolean wasEmpty = queue.isEmpty();

            if (queue.remainingCapacity() > 1) {
                queue.put(new PendingMessage(msg, service, serviceParam));

                // Still not saturated. If we weren't idle either, then nothing worth mentionning.
                if (wasEmpty) {
                    change = true;
                    stateMachine.msgsEvent();
                    action = eventCalled(false);
                }
            } else if (1 == queue.remainingCapacity()) {
                queue.put(new PendingMessage(msg, service, serviceParam));

                // Now saturated.
                stateMachine.saturatedEvent();
                action = eventCalled(false);
                change = true;
            } else {
                // Was already saturated.
                queued = false;
            }
        }

        if (queued && change) {
            // If not queued, there was no change of condition as far as
            // outsiders are concerned. (redundant saturatedEvent, only
            // defensive; to guarantee statemachine in sync). else, if the
            // saturation state did not change, we have no state change to
            // notify.
            notifyChange();
        }

        performDeferredAction(action);

        // Before we return, make sure that this channel remains referenced if
        // it has messages. It could become unreferenced if it is not yet
        // resolved and the application lets go of it after sending messages.
        // This means that we may need to do something only in the resolpending
        // and resolsaturated cases. The way we do this test, there can be false
        // positives. They're dealt with as part of the action that is carried
        // out.
        if ((stateMachine.getState() & (Messenger.RESOLPENDING | Messenger.RESOLSATURATED)) != 0) {
            resolPendingImpl();
        }

        return queued;
    }

    /**
     * {@inheritDoc}
     */
    public final boolean sendMessageN(Message msg, String rService, String rServiceParam) {

        try {
            if (sendMessageCommon(msg, rService, rServiceParam)) {
                // If it worked the message is queued; the outcome will be notified later.
                return true;
            }
            // Non-blocking and queue full: report overflow.
            msg.setMessageProperty(Messenger.class, OutgoingMessageEvent.OVERFLOW);
        } catch (IOException oie) {
            msg.setMessageProperty(Messenger.class, new OutgoingMessageEvent(msg, oie));
        } catch (InterruptedException interrupted) {
            msg.setMessageProperty(Messenger.class, new OutgoingMessageEvent(msg, interrupted));
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public final void sendMessageB(Message msg, String rService, String rServiceParam) throws IOException {

        try {
            while (true) {
                // if sendMessageCommon says "true" it worked.
                if (sendMessageCommon(msg, rService, rServiceParam)) {
                    return;
                }
                // Do a shallow check on the queue. If it seems empty (without getting into a critical section to
                // verify it), then yielding is good bet. It is a lot cheaper and smoother than waiting.
                // Note the message should be enqueued now. yielding makes sense now if the queue is empty
                if (queue.isEmpty()) {
                    Thread.yield();
                }

                // If we reached this far, it is neither closed, nor ok. So it was saturated.
                synchronized (stateMachine) {
                    // Cheaper than waitState. sendMessageCommon already does the relevant state checks.
                    stateMachine.wait();
                }
            }
        } catch (InterruptedException ie) {
            InterruptedIOException iie = new InterruptedIOException("Message send interrupted");

            iie.initCause(ie);
            throw iie;
        }
    }

    /**
     * {@inheritDoc}
     */
    public final void resolve() {
        DeferredAction action;

        synchronized (stateMachine) {
            stateMachine.resolveEvent();
            action = eventCalled(true);
        }
        notifyChange();
        performDeferredAction(action); // we expect connect but let the state machine decide.
    }

    /**
     * {@inheritDoc}
     */
    public final int getState() {
        return stateMachine.getState();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final Messenger getChannelMessenger(PeerGroupID redirection, String service, String serviceParam) {
        // Channels don't make channels.
        return null;
    }

    /**
     * Three exposed methods may need to inject new events in the system:
     * sendMessageN, close, and shutdown.
     * Since they can all cause actions, and since connectAction and
     * startAction are deferred, it seems possible that one of the actions
     * caused by send, close, or shutdown be called while connectAction or
     * startAction are in progress.
     * <p/>However, the state machine gives us a few guarantees: connectAction
     * and startAction can never nest. We will not be asked to perform one while
     * still performing the other. Only the synchronous actions closeInput,
     * closeOutput, or failAll can possibly be requested in the interval. We
     * could make more assumptions and simplify the code, but rather keep at
     * least some flexibility.
     * <p/>
     * DEAD LOCK WARNING: the implementor's method invoke some of our call backs
     * while synchronized. Then our call backs synchronize on the state machine
     * in here. This nesting order must always be respected. As a result, we can
     * never invoke implementors methods while synchronized. Hence the
     * deferredAction processing.
     *
     * @param action the action
     */
    private void performDeferredAction(DeferredAction action) {
        switch (action) {
        case ACTION_SEND:
            startImpl();
            break;

        case ACTION_CONNECT:
            connectImpl();
            break;
        }
    }

    /**
     * A shortHand for a frequently used sequence. MUST be called while
     * synchronized on stateMachine.
     *
     * @param notifyAll If {@code true} then this is a life-cycle event and all
     *                  waiters on the stateMachine should be notified. If {@code false} then
     *                  only a single waiter will be notified for simple activity events.
     * @return the deferred action.
     */
    private DeferredAction eventCalled(boolean notifyAll) {
        DeferredAction action = deferredAction;

        deferredAction = DeferredAction.ACTION_NONE;
        if (notifyAll) {
            stateMachine.notifyAll();
        } else {
            stateMachine.notify();
        }
        return action;
    }

    /*
     * Implement the methods that our shared messenger will use to report progress.
     */

    /**
     * The implementation will invoke this method when it becomes resolved,
     * after connectImpl was invoked.
     */
    protected void up() {
        DeferredAction action;

        synchronized (stateMachine) {
            stateMachine.upEvent();
            action = eventCalled(true);
        }
        notifyChange();
        performDeferredAction(action); // we expect start but let the state machine decide.
    }

    /**
     * The implementation invokes this method when it becomes broken.
     */
    protected void down() {
        DeferredAction action;

        synchronized (stateMachine) {
            stateMachine.downEvent();
            action = eventCalled(true);
        }
        notifyChange();
        performDeferredAction(action); // we expect connect but let the state machine decide.
    }

    /**
     * Here, we behave like a queue to the shared messenger. When we report
     * being empty, though, we're automatically removed from the active queues
     * list. We'll go back there the next time we have something to send by
     * calling startImpl.
     *
     * @return pending message
     */
    protected PendingMessage peek() {

        PendingMessage theMsg;
        DeferredAction action = DeferredAction.ACTION_NONE;

        synchronized (stateMachine) {
            // We like the msg to keep occupying space in the queue until it's
            // out the door. That way, idleness (that is, not currently working
            // on a message), is always consistent with queue emptyness.

            theMsg = queue.peek();
            if (theMsg == null) {
                stateMachine.idleEvent();
                action = eventCalled(false);

                // We do not notifyChange, here, because, if the queue is empty,
                // it was already notified when the last message was popped. The
                // call to idleEvent is only defensive programming to make extra
                // sure the state machine is in sync.

                return null;
            }

            if (outputClosed) {
                // We've been asked to stop sending. Which, if we were sending,
                // must be notified by either an idle event or a down
                // event. Nothing needs to happen to the shared messenger. We're
                // just a channel.
                stateMachine.downEvent();
                action = eventCalled(true);
                theMsg = null;
            }
        }

        notifyChange();
        performDeferredAction(action); // we expect none but let the state machine decide.
        return theMsg;
    }

    /**
     * Returns the number of elements in this collection.  If this collection
     * contains more than <tt>Integer.MAX_VALUE</tt> elements, returns
     * <tt>Integer.MAX_VALUE</tt>.
     *
     * @return the number of elements in this collection
     */
    protected int size() {
        return queue.size();
    }
    
    /**
     * One message done. Update the saturated/etc state accordingly.
     *
     * @return true if there are more messages after the one we removed.
     */
    protected boolean poll() {

        boolean result;
        DeferredAction action;

        synchronized (stateMachine) {
            queue.poll();

            if (queue.peek() == null) {
                stateMachine.idleEvent();
                action = eventCalled(false);
                result = false;
            } else {
                stateMachine.msgsEvent();
                action = eventCalled(false);
                result = true;
            }
        }

        notifyChange();
        performDeferredAction(action); // we expect none but let the state machine decide.

        return result;
    }

    /**
     * We invoke this method to be placed on the list of channels that have
     * message to send.
     * <p/>
     * NOTE that it is the shared messenger responsibility to synchronize so
     * that we cannot be added to the active list just before we get removed
     * due to reporting an empty queue in parallel. So, if we report an empty
     * queue and have a new message to send before the shared messenger removes
     * us form the active list, startImpl will block until the removal is done.
     * Then we'll be added back.
     * <p/>
     * If it cannot be done, it means that the shared messenger is no longer
     * usable. It may call down() in sequence. Out of defensiveness, it should
     * do so without holding its lock.
     */
    protected abstract void startImpl();

    /**
     * We invoke this method to be placed on the list of channels that are
     * waiting for resolution.
     * <p/>
     * If it cannot be done, it means that the shared messenger is no longer
     * usable. It may call down() in sequence. Out of defensiveness, it should
     * do so without holding its lock. If the messenger is already resolved it
     * may call up() in sequence. Same wisdom applies. It is a good idea to
     * create channels in the resolved state if the shared messenger is already
     * resolved. That avoids this extra contortion.
     */
    protected abstract void connectImpl();

    /**
     * This is invoked to inform the implementation that this channel is now in
     * the resolPending or resolSaturated state. This is specific to this type
     * of channels. The shared messenger must make sure that this channel
     * remains strongly referenced, even though it is not resolved, because
     * there are messages in it. It is valid for an application to let go of a
     * channel after sending a message, even if the channel is not yet
     * resolved. The message will go if/when the channel resolves. This method
     * may be invoked redundantly and even once the channel is no longer among
     * the one awaiting resolution. The implementation must be careful to
     * ignore such calls.
     */
    protected abstract void resolPendingImpl();
}
