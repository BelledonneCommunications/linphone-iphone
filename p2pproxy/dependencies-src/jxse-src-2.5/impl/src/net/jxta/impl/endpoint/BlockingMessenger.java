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
package net.jxta.impl.endpoint;

import java.util.Timer;
import java.util.TimerTask;

import java.io.IOException;
import java.io.InterruptedIOException;

import java.util.logging.Level;

import net.jxta.logging.Logging;

import java.util.logging.Logger;

import net.jxta.endpoint.AbstractMessenger;
import net.jxta.endpoint.ChannelMessenger;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.MessengerState;
import net.jxta.endpoint.OutgoingMessageEvent;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.util.SimpleSelectable;

import net.jxta.impl.util.TimeUtils;

/**
 * This class is a near-drop-in replacement for the previous BlockingMessenger class.
 * To subclassers (that is, currently, transports) the only difference is that some
 * overloaded methods have a different name (class hierarchy reasons made it impossible
 * to preserve the names without forcing an API change for applications).
 *
 * The other difference which is not API visible, is that it implements the
 * standard MessengerState behaviour and semantics required by the changes in the endpoint framework.
 *
 * This the only base messenger class meant to be extended by outside code that is in the impl tree. The
 * reason being that what it replaces was there already and that new code should not become dependant upon it.
 */
public abstract class BlockingMessenger extends AbstractMessenger {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(BlockingMessenger.class.getName());

    /**
     * The self destruct timer.
     * <p/>
     * When a messenger has become idle, it is closed. As a side effect, it
     * makes the owning canonical messenger, if any, subject to removal if it is
     * otherwise unreferenced.
     */
    private final static transient Timer timer = new Timer("BlockingMessenger self destruct timer", true);

    /*
     * Actions that we defer to after returning from event methods. In other 
     * words, they cannot be done with the lock held, or they require calling 
     * more event methods.  Because this messenger can take only one message at 
     * a time (saturated while sending), actions do not cascade much. Start can 
     * lead to connect if the sending fails, but, because we always fail to
     * connect, connect will not lead to start. As a result we can get away with 
     * performing deferred actions recursively. That simplifies the code.
     */
    private enum DeferredAction {
        /**
         * No deferred action.
         */
        ACTION_NONE,

        /**
         * Must send message.
         */
        ACTION_SEND,

        /**
         * Must report failure to connect.
         */
        ACTION_CONNECT
    }

    /**
     * The outstanding message.
     */
    private Message currentMessage = null;

    /**
     * The serviceName override for that message.
     */
    private String currentService = null;

    /**
     * The serviceParam override for that message.
     */
    private String currentParam = null;

    /**
     * The exception that caused that message to not be sent.
     */
    private Throwable currentThrowable = null;

    /**
     * true if we have deliberately closed our one message input queue.
     */
    private boolean inputClosed = false;

    /**
     * Need to know which group this transport lives in, so that we can suppress
     * channel redirection when in the same group. This is currently the norm.
     */
    private final PeerGroupID homeGroupID;

    /**
     * The current deferred action.
     */
    private DeferredAction deferredAction = DeferredAction.ACTION_NONE;

    /**
     * Reference to owning object. This is there so that the owning object is not subject to garbage collection
     * unless this object here becomes itself unreferenced. That happens when the self destruct timer closed it.
     */
    private Object owner = null;

    /**
     * The timer task watching over our self destruction requirement.
     */
    private final TimerTask selfDestructTask;

    /**
     * State lock and engine.
     */
    private final BlockingMessengerState stateMachine = new BlockingMessengerState();

    /**
     * legacy artefact: transports need to believe the messenger is not yet closed in order to actually close it.
     * So we lie to them just while we run their closeImpl method so that they do not see that the messenger is
     * officially closed.
     */
    private boolean lieToOldTransports = false;

    /**
     * Our statemachine implementation; just connects the standard AbstractMessengerState action methods to
     * this object.
     */
    private class BlockingMessengerState extends MessengerState {

        protected BlockingMessengerState() {
            super(true);
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
            // we're synchonized here. (invoked from stateMachine).
            inputClosed = true;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void closeOutputAction() {
            // This will break the cnx; thereby causing a down event if we have a send in progress.
            // If the cnx does not break before the current message is sent, then the message will be sent successfully,
            // resulting in an idle event. Either of these events is enough to complete the shutdown process.
            lieToOldTransports = true;
            closeImpl();
            lieToOldTransports = false;

            // Disconnect from the timer.
            if (selfDestructTask != null) {
                selfDestructTask.cancel();
            }
        }

        // This is a synchronous action. No synchronization needed: we're already synchronized, here.
        // There's a subtlety here: we do not clear the current message. We let sendMessageB or sendMessageN
        // deal with it, so that they can handle the status reporting each in their own way. So basically, all we
        // do is to set a reason for that message to fail in case we are shutdown from the outside and that message
        // is not sent yet. As long as there is a current message, it is guaranteed that there is a thread
        // in charge of reporting its status. It is also guaranteed that when failAll is called, the input is
        // already closed, and so, we have no obligation of making room for future messages immediately.
        // All this aggravation is so that we do not have to create one context wrapper for each message just so
        // that we can associate it with its result. Instead we use our single msg and single status model
        // throughout.
        @Override
        protected void failAllAction() {

            if (currentMessage == null) {
                return;
            }

            if (currentThrowable == null) {
                currentThrowable = new IOException("Messenger unexpectedly closed");
            }
        }
    }


    /**
     * The implementation of channel messenger that getChannelMessenger returns:
     * All it does is address rewriting. Even close() is forwarded to the shared messenger.
     * The reason is that BlockingMessengers are not really shared; they're transitional
     * entities used directly by CanonicalMessenger. GetChannel is used only to provide address
     * rewriting when we pass a blocking messenger directly to incoming messenger listeners...this
     * practice is to be removed in the future, in favor of making incoming messengers full-featured
     * async messengers that can be shared.
     */
    private final class BlockingMessengerChannel extends ChannelMessenger {

        public BlockingMessengerChannel(EndpointAddress baseAddress, PeerGroupID redirection, String origService, String origServiceParam) {

            super(baseAddress, redirection, origService, origServiceParam);

            // We tell our super class that we synchronize on the stateMachine object. Althoug it is not obvious, our getState()
            // method calls the shared messenger getState() method, which synchronizes on the shared messenger's state machine
            // object. So, that's what we must specify.  Logic would dictate that we pass it to super(), but it is not itself
            // constructed until super() returns. No way around it.

            setStateLock(stateMachine);
        }

        /**
         * {@inheritDoc}
         */
        public int getState() {
            return BlockingMessenger.this.getState();
        }

        /**
         * {@inheritDoc}
         */
        public void resolve() {
            BlockingMessenger.this.resolve();
        }

        /**
         * {@inheritDoc}
         */
        public void close() {
            BlockingMessenger.this.close();
        }

        /**
         * {@inheritDoc}
         *
         * <p/>
         * Address rewriting done here.
         */
        public boolean sendMessageN(Message msg, String service, String serviceParam) {
            return BlockingMessenger.this.sendMessageN(msg, effectiveService(service), effectiveParam(service, serviceParam));
        }

        /**
         * {@inheritDoc}
         *
         * <p/>
         * Address rewriting done here.
         */
        public void sendMessageB(Message msg, String service, String serviceParam) throws IOException {
            BlockingMessenger.this.sendMessageB(msg, effectiveService(service), effectiveParam(service, serviceParam));
        }

        /**
         * {@inheritDoc}
         *
         * <p/>
         * We're supposed to return the complete destination, including
         * service and param specific to that channel. It is not clear, whether
         * this should include the cross-group mangling, though. For now, let's
         * say it does not.
         */
        public EndpointAddress getLogicalDestinationAddress() {
            EndpointAddress rawLogical = getLogicalDestinationImpl();

            if (rawLogical == null) {
                return null;
            }
            return new EndpointAddress(rawLogical, origService, origServiceParam);
        }

        // Check if it is worth staying registered
        public void itemChanged(Object changedObject) {

            if (!notifyChange()) {
                if (haveListeners()) {
                    return;
                }

                BlockingMessenger.this.unregisterListener(this);

                if (!haveListeners()) {
                    return;
                }

                // Ooops collision. We should not have unregistered. Next time, then. In case of collision, the end result
                // is always to stay registered. There's no harm in staying registered.
                BlockingMessenger.this.registerListener(this);
            }
        }

        /**
         * {@inheritDoc}
         * <p/>
         * Always make sure we're registered with the shared messenger.
         */
        @Override
        protected void registerListener(SimpleSelectable l) {
            BlockingMessenger.this.registerListener(this);
            super.registerListener(l);
        }
    }

    private void storeCurrent(Message msg, String service, String param) {
        currentMessage = msg;
        currentService = service;
        currentParam = param;
        currentThrowable = null;
    }

    /**
     * Constructor.
     * <p/>
     * We start in the CONNECTED state, we pretend to have a queue of size 1, and we can never re-connect.  Although this
     * messenger fully respects the asynchronous semantics, it is saturated as soon as one msg is being send, and if not
     * saturated, send is actually performed by the invoker thread. So return is not completely immediate.  This is a barely
     * acceptable implementation, but this is also a transition adapter that is bound to disappear one release from now. The main
     * goal is to get things going until transports are adapted.
     *
     * @param homeGroupID  the group that this messenger works for. This is the group of the endpoint service or transport
     *                     that created this messenger.
     * @param dest         where messages should be addressed to
     * @param selfDestruct true if this messenger must self close destruct when idle. <b>Warning:</b> If selfDestruct is used,
     *                     this messenger will remained referenced for as long as isIdleImpl returns false.
     */

    public BlockingMessenger(PeerGroupID homeGroupID, EndpointAddress dest, boolean selfDestruct) {

        super(dest);

        this.homeGroupID = homeGroupID;

        // We tell our superclass that we synchronize our state on the stateMachine object.  Logic would dictate that we pass it
        // to super(), but it is not itself constructed until super() returns. No way around it.

        setStateLock(stateMachine);

        /*
         * Sets up a timer task that will close this messenger if it says to have become idle. It will keep it referenced
         * until then.
         * <p/>
         * As long as this timer task is scheduled, this messenger is not subject to GC. Therefore, its owner, if any, which is strongly
         * referenced, is not subject to GC either. This avoids prematurely closing open connections just because a destination is
         * not currently in use, which we would have to do if CanonicalMessengers could be GCed independantly (and which would
         * force us to use finalizers, too).<p/>
         *
         * Such a mechanism is usefull only if this blocking messenger is expensive to make or holds system resources that require
         * an explicit invocation of the close method. Else, it is better to let it be GC'ed along with any refering canonical
         * messenger when memory is tight.<p/>
         *
         */

        //
        // FIXME 20040413 jice : we trust transports to implement isIdle reasonably, which may be a leap of faith. We
        // should probably superimpose a time limit of our own.
        //
        if (selfDestruct) {
            selfDestructTask = new TimerTask() {

                /**
                 * {@inheritDoc}
                 */
                @Override
                public void run() {
                    try {
                        if (isIdleImpl()) {
                            close();
                        } else {
                            return;
                        }
                    } catch (Throwable uncaught) {
                        if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                            LOG.log(Level.SEVERE, "Uncaught Throwable in selfDescructTask. ", uncaught);
                        }
                    }
                    cancel();
                }
            };

            timer.schedule(selfDestructTask, TimeUtils.AMINUTE, TimeUtils.AMINUTE);
        } else {
            selfDestructTask = null;
        }
    }

    /**
     * Sets an owner for this blocking messenger. Owners are normally canonical messengers. The goal of registering the owner is
     * to keep that owner reachable as long as this blocking messenger is.  Canonical messengers are otherwise softly referenced,
     * and so, may be deleted whenever memory is tight.
     * <p/>
     * We do not want to use finalizers or the equivalent reference queue mechanism; so we have no idea when a blocking messenger
     * is no-longer referenced by any canonical. In addition it may be expensive to make and so we want to keep it for a while
     * anyway. As a result, instead of keeping a blocking messenger open as long as there is a canonical, we do the opposite: we
     * keep the canonical (owner, here) as long as the blocking messenger is open (and usually beyond, memory allowing). How long
     * a blocking messenger will stay around depends upon that messenger's implementation. That may even be left up to the GC, in
     * the end (if close is not needed AND the messenger is cheap to make). In that case, the owner is likely the only referrer,
     * and so both will have the same lifetime.
     *
     * @param owner The object that should be kept referenced at least as long as this one.
     */
    public void setOwner(Object owner) {
        this.owner = owner;
    }

    /**
     * Assemble a destination address for a message based upon the messenger
     * default destination address and the optional provided overrides.
     *
     * @param service The destination service or {@code null} to use default.
     * @param serviceParam The destination service parameter or {@code null} to 
     * use default.
     */
    protected EndpointAddress getDestAddressToUse(String service, String serviceParam) {
        EndpointAddress defaultAddress = getDestinationAddress();
        EndpointAddress result;
        
        if(null == service) {
            if(null == serviceParam) {
                // Use default service name and service params
                result = defaultAddress;
            } else {
                // use default service name, override service params
                result = new EndpointAddress(defaultAddress, defaultAddress.getServiceName(), serviceParam);
            }
        } else {
            if(null == serviceParam) {
                // override service name, use default service params (this one is pretty weird and probably not useful)
                result = new EndpointAddress(defaultAddress, service, defaultAddress.getServiceParameter());
            } else {
                // override service name, override service params
                result = new EndpointAddress(defaultAddress, service, serviceParam);
            }
        }
        
        return result;
    }

    /**
     * A transport may call this to cause an orderly closure of its messengers.
     */
    protected final void shutdown() {
        DeferredAction action;

        synchronized (stateMachine) {
            stateMachine.shutdownEvent();
            action = eventCalled();
        }

        // We called an event. State may have changed.
        notifyChange();

        performDeferredAction(action);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * We overload isClosed because many messengers still use super.isClosed()
     * as part of their own implementation or don't override it at all. They
     * expect it to be true only when all is shutdown; not while we're closing
     * gently.
     *
     * FIXME - jice@jxta.org 20040413: transports should get a deeper retrofit eventually.
     */
    @Override
    public boolean isClosed() {
        return (!lieToOldTransports) && super.isClosed();
    }

    /**
     * {@inheritDoc}
     * <p/>
     * getLogicalDestinationAddress() requires resolution (it's the address advertised by the other end).
     * For a blocking messenger it's easy. We're born resolved. So, just ask the implementor what it is.
     */
    public final EndpointAddress getLogicalDestinationAddress() {
        return getLogicalDestinationImpl();
    }

    /**
     * {@inheritDoc}
     *
     * <p/> Some transports historically overload the close method of BlockingMessenger.
     * The final is there to make sure we know about it. However, there should be no
     * harm done if the unchanged code is renamed to closeImpl; even if it calls super.close().
     * The real problem, however, is transports calling close (was their own, but now it means this one), when
     * they want to break. It will make things look like someone just called close, but it will not
     * actually break anything. However, that will cause the state machine to go through the close process.
     * this will end up calling closeImpl(). That will do.
     */
    public final void close() {
        DeferredAction action;

        synchronized (stateMachine) {
            stateMachine.closeEvent();

            action = eventCalled();
        }

        // We called an event. State may have changed.
        notifyChange();

        performDeferredAction(action);
    }

    /**
     * {@inheritDoc}
     */
    public void sendMessageB(Message msg, String service, String serviceParam) throws IOException {

        DeferredAction action;

        synchronized (stateMachine) {
            try {
                while ((currentMessage != null) && !inputClosed) {
                    stateMachine.wait();
                }
            } catch (InterruptedException ie) {
                throw new InterruptedIOException();
            }

            if (inputClosed) {
                throw new IOException("Messenger is closed. It cannot be used to send messages");
            }

            // We store the four elements of a pending msg separately. We do not want to pour millions of tmp objects on the GC for
            // nothing.

            storeCurrent(msg, service, serviceParam);
            stateMachine.saturatedEvent();
            action = eventCalled();
        }

        notifyChange(); // We called an event. State may have changed.
        performDeferredAction(action); // We called an event. There may be an action. (start, normally).

        // After deferred action, the message was either sent or failed. (done by this thread).
        // We can tell because, if failed, the currentMessage is still our msg.
        Throwable failure = null;

        synchronized (stateMachine) {
            if (currentMessage == msg) {
                failure = currentThrowable;
                if (failure == null) {
                    failure = new IOException("Unknown error");
                }
                // Ok, let it go, now.
                storeCurrent(null, null, null);
            } // Else, don't touch currentMsg; it's not our msg.
        }

        if (failure == null) {
            // No failure. Report ultimate succes.
            msg.setMessageProperty(Messenger.class, OutgoingMessageEvent.SUCCESS);
            return;
        }

        // Failure. See how we can manage to throw it.
        if (failure instanceof IOException) {
            throw (IOException) failure;
        }
        if (failure instanceof RuntimeException) {
            throw (RuntimeException) failure;
        }
        if (failure instanceof Error) {
            throw (Error) failure;
        }

        IOException failed = new IOException("Failure sending message");
        failed.initCause(failure);
        throw failed;
    }

    /**
     * {@inheritDoc}
     */
    public final boolean sendMessageN(Message msg, String service, String serviceParam) {

        boolean queued = false;
        DeferredAction action = DeferredAction.ACTION_NONE;
        boolean closed;

        synchronized (stateMachine) {
            closed = inputClosed;
            if ((!closed) && (currentMessage == null)) {
                // We copy the four elements of a pending msg right here. We do not want to pour millions of tmp objects on the GC.
                storeCurrent(msg, service, serviceParam);
                stateMachine.saturatedEvent();
                action = eventCalled();
                queued = true;
            }
        }

        if (queued) {
            notifyChange(); // We called an event. State may have changed.
            performDeferredAction(action); // We called an event. There may be an action. (start, normally).

            // After deferred action, the message was either sent or failed. (done by this thread).
            // We can tell because, if failed, the currentMessage is still our msg.
            synchronized (stateMachine) {
                if (currentMessage == msg) {
                    if (currentThrowable == null) {
                        currentThrowable = new IOException("Unknown error");
                    }
                    msg.setMessageProperty(Message.class, currentThrowable);
                    // Ok, let it go, now.
                    storeCurrent(null, null, null);
                } else {
                    msg.setMessageProperty(Message.class, OutgoingMessageEvent.SUCCESS);
                    // Don't touch the current msg; it's not our msg.
                }
            }
            // Yes, we return true in either case. sendMessageN is supposed to be async. If a message fails
            // after it was successfuly queued, the error is not reported by the return value, but only by
            // the message property (and select). Just making sure the behaviour is as normal as can be
            // even it means suppressing some information.

            return true;
        }

        // Not queued. Either closed, or currently sending. If inputClosed, that's what we report.
        msg.setMessageProperty(Messenger.class,
                closed ?
                        new OutgoingMessageEvent(msg, new IOException("This messenger is closed. " + "It cannot be used to send messages.")) :
                        OutgoingMessageEvent.OVERFLOW);
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public final void resolve() {// We're born resolved. Don't bother calling the event.
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
    public final Messenger getChannelMessenger(PeerGroupID redirection, String service, String serviceParam) {

        // Our transport is always in the same group. If the channel's target group is the same, no group
        // redirection is ever needed.

        return new BlockingMessengerChannel(getDestinationAddress(),
                homeGroupID.equals(redirection) ? null : redirection, service,
                serviceParam);
    }

    /**
     * Three exposed methods may need to inject new events in the system: sendMessageN, close, and shutdown. Since they can both
     * cause actions, and since connectAction and startAction are deferred, it seems possible that one of the
     * actions caused by send, close, or shutdown be called while connectAction or startAction are in progress.
     *
     * However, the state machine gives us a few guarantees: connectAction and startAction can never nest. We will not be
     * asked to perform one while still performing the other. Only the synchronous actions closeInput, closeOutput, or
     * failAll can possibly be requested in the interval. We could make more assumptions and simplify the code, but rather
     * keep at least some flexibility.
     */

    private void performDeferredAction(DeferredAction action) {
        switch (action) {
            case ACTION_SEND:
                sendIt();
                break;

            case ACTION_CONNECT:
                cantConnect();
                break;
        }
    }

    /**
     * A shortHand for a frequently used sequence. MUST be called while synchronized on stateMachine.
     *
     * @return the deferred action.
     */
    private DeferredAction eventCalled() {
        DeferredAction action = deferredAction;

        deferredAction = DeferredAction.ACTION_NONE;
        stateMachine.notifyAll();
        return action;
    }

    /**
     * Performs the ACTION_SEND deferred action: sends the one msg in our one msg queue.
     * This method *never* sets the outcome message property. This is left to sendMessageN and sendMessageB, because
     * sendMessageB does not want to set it in any other case than success, while sendMessageN does it in all cases.
     * The problem with that is: how do we communicate the outcome to sendMessage{NB} without having to keep
     * the 1 msg queue locked until then (which would be in contradiction with how we interact with the state machine).
     * To make it really inexpensive, here's the trick: when a message fails currentMessage and currentFailure remain.
     * So the sendMessageRoutine can check them and known that it is its message and not another one that caused the
     * failure. If all is well, currentMessage and currentFailure are nulled and if another message is send immediately
     * sendMessage is able to see that its own message was processed fully. (this is a small cheat regarding the
     * state of saturation after failall, but that's not actually detectable from the outside: input is closed
     * before failall anyway. See failall for that part.
     */
    private void sendIt() {

        if (currentMessage == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Internal error. Asked to send with no message.");
            }
            return;
        }

        DeferredAction action;

        try {
            sendMessageBImpl(currentMessage, currentService, currentParam);
        } catch (Throwable any) {
            // Did not work. We report the link down and let the state machine tell us when to fail the msg.  It is assumed that
            // when this happens, the cnx is already down.  FIXME - jice@jxta.org 20040413: check with the various kind of funky
            // exception. Some may not mean the link is down
            synchronized (stateMachine) {
                currentThrowable = any;
                stateMachine.downEvent();
                action = eventCalled();
            }
            notifyChange();
            performDeferredAction(action); // we expect connect but let the state machine decide.
            return;
        }

        // Worked.

        synchronized (stateMachine) {
            storeCurrent(null, null, null);
            stateMachine.idleEvent();
            action = eventCalled();
        }

        // We did go from non-idle to idle. Report it.
        notifyChange();

        performDeferredAction(action); // should be none but let the state machine decide.
    }

    /**
     * Performs the ACTION_CONNECT deferred action: generate a downEvent since we cannot reconnect.
     */
    private void cantConnect() {
        DeferredAction action;

        synchronized (stateMachine) {
            stateMachine.downEvent();
            action = eventCalled();
        }
        notifyChange();
        performDeferredAction(action); // should be none but let the state machine decide.
    }

    /*
     * Abstract methods to be provided by implementer (a transport for example).
     * To adapt legacy transport, keep extending BlockingMessenger and just rename your close, isIdle, sendMessage and
     * getLogicalDestinationAddress methods to closeImpl, isIdleImpl, sendMessageBImpl, and getLogicalDestinationImpl, respectively.
     */

    /**
     * Close connection. May fail current send.
     */
    protected abstract void closeImpl();

    /**
     * Send a message blocking as needed until the message is sent.
     *
     * @param message The message to send.
     * @param service The destination service.
     * @param param   The destination serivce param.
     * @throws IOException Thrown for errors encountered while sending the message.
     */
    protected abstract void sendMessageBImpl(Message message, String service, String param) throws IOException;

    /**
     * return true if this messenger has not been used for a long time. The definition of long time is: "sufficient such that closing it
     * is worth the cost of having to re-open". A messenger should self close if it thinks it meets the definition of
     * idle. BlockingMessenger leaves the evaluation to the transport but does the work automatically. <b>Important:</b> if
     * self destruction is used, this method must work; not just return false. See the constructor. In general, if closeImpl does
     * not need to do anything, then self destruction is not needed.
     *
     * @return {@code true} if theis messenger is, by it's own definition, idle.
     */
    protected abstract boolean isIdleImpl();

    /**
     * Obtain the logical destination address from the implementer (a transport for example).
     */
    protected abstract EndpointAddress getLogicalDestinationImpl();
}
