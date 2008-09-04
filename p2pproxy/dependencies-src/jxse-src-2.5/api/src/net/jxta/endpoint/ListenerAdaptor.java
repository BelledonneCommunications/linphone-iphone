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

import net.jxta.logging.Logging;
import net.jxta.util.SimpleSelectable;
import net.jxta.util.SimpleSelectable.IdentityReference;
import net.jxta.util.SimpleSelector;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executor;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * The legacy getMessenger asynchronous API never returns any object to the invoker until a messenger could actually be made,
 * allowing the application to supply a listener to be invoked when the operation completes. The legacy Messenger API also
 * provides a method to send messages that calls a listener to report the outcome of the operation.  <p/>
 * <p/>
 * The model has changed, so that an asynchronous messenger is made unresolved and returned immediately to the invoker, which can
 * then request opening or even just send a message to force the opening. Subsequently, the messenger can be used as a control
 * block to monitor progress with {@link Messenger#register} and {@link Messenger#waitState}.<p/>
 * <p/>
 * Likewise, the outcome of sending a message is a property of that message. Messages can be selected to monitor property changes
 * with {@link Message#register} and {@link net.jxta.endpoint.Message#getMessageProperty(Object)} (the outcome property key is
 * <code>Messenger.class</code>).
 * <p/>
 * This class here provides the legacy listener model on top of the new model for applications that prefer listeners. This class
 * is used internally to emulate the legacy listener behaviour, so that applications do not need to be adapted.<p/>
 * <p/>
 * Note: one instance of this class gets instantiated by each EndpointService interface. However, it does not start using any
 * resources until it first gets used.<p/>
 */
public class ListenerAdaptor implements Runnable {

    // FIXME - jice 20040413: Eventhough it is not as critical as it used to be we should get rid of old, never resolved entries.
    // Attempts are supposed to always fail or succeed rather soon. Here, we trust transports in that matter. Is it safe ?

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(ListenerAdaptor.class.getName());

    /**
     * The in progress messages.
     */
    private final Map<IdentityReference, ListenerContainer> inprogress = new HashMap<IdentityReference, ListenerContainer>(32);

    /**
     * The thread that does the work.
     */
    private Thread bgThread = null;

    /**
     * The selector that we use to watch messengers progress.
     */
    private final SimpleSelector selector = new SimpleSelector();

    /**
     * Have we been shutdown?
     */
    private volatile boolean shutdown = false;

    /**
     * The exceutor service.
     */
    private final Executor executor;

    /**
     * The ThreadGroup in which this adaptor will run.
     */
    private final ThreadGroup threadGroup;

    /**
     * Standard Constructor
     *
     * @param threadGroup The ThreadGroup in which this adaptor will run.
     */
    public ListenerAdaptor(ThreadGroup threadGroup) {
        this(threadGroup, null);
    }
    /**
     * Creates a ListenerAdaptor with a threadpool for notification callback.
     *
     * @param threadGroup The ThreadGroup in which this adaptor will run.
     * @param executor the excutor to use for notification callback
     */
    public ListenerAdaptor(ThreadGroup threadGroup, Executor executor) {
        this.executor = executor;
        this.threadGroup = threadGroup;
    }

    /**
     * Cannot be re-started. Do not call once shutdown.
     */
    private synchronized void init() {
        assert !shutdown;

        if (bgThread != null) {
            return;
        }

        bgThread = new Thread(threadGroup, this, "Listener Adaptor");
        bgThread.setDaemon(true);
        bgThread.start();
    }

    public synchronized void shutdown() {
        shutdown = true;

        // Stop the thread if it was ever created.
        Thread bg = bgThread;
        if (bg != null) {
            bg.interrupt();
        }
    }

    /**
     * Stop watching a given selectable.
     *
     * @param ts the selectable
     */
    private void forgetSelectable(SimpleSelectable ts) {
        // Either way, we're done with this one.
        ts.unregister(selector);

        synchronized (this) {
            inprogress.remove(ts.getIdentityReference());
        }
    }

    /**
     * Select the given message and invoke the given listener when the message sending is complete.
     *
     * @param listener The listener to invoke. If null the resolution will take place, but obviously no listener will be invoked.
     * @param message  The message being sent.
     * @return true if the message was registered successfully or the listener is null. If true it is guaranteed that the listener
     *         will be invoked unless null. If false, it is guaranteed that the listener will not be invoked.
     */
    public boolean watchMessage(OutgoingMessageEventListener listener, Message message) {
        synchronized (this) {
            if (shutdown) {
                return false;
            }

            if (listener == null) {
                // We're done, then. The invoker does not really care.
                return true;
            }

            // Init if needed.
            init();

            // First we must ensure that if the state changes we'll get to handle it.
            MessageListenerContainer allListeners = (MessageListenerContainer) inprogress.get(message.getIdentityReference());

            if (allListeners == null) {
                allListeners = new MessageListenerContainer();
                inprogress.put(message.getIdentityReference(), allListeners);
            }
            allListeners.add(listener);
        }

        // When we do that, the selector gets notified. Therefore always check the initial state automatically. If the
        // selectable is already done with, the listener will be called by the selector's handler.
        message.register(selector);

        return true;
    }

    /**
     * Select the given messenger and invoke the given listener when the messenger is resolved.
     *
     * @param listener  The listener to invoke. If null the resolution will take place, but obviously no listener will be invoked.
     * @param messenger The messenger being resolved.
     * @return true if the messenger was registered successfully or the listener is null. If true it is guaranteed that the listener
     *         will be invoked unless null. If false, it is guaranteed that the listener will not be invoked.
     */
    public boolean watchMessenger(MessengerEventListener listener, Messenger messenger) {
        synchronized (this) {

            if (shutdown) {
                return false;
            }

            if (listener == null) {
                // We're done, then. The invoker does not really care.
                return true;
            }

            // Init if needed.
            init();

            // First we must ensure that if the state changes we'll get to handle it.
            MessengerListenerContainer allListeners = (MessengerListenerContainer) inprogress.get(messenger.getIdentityReference());

            if (allListeners == null) {
                // Use ArrayList. The code is optimized for that.
                allListeners = new MessengerListenerContainer();
                inprogress.put(messenger.getIdentityReference(), allListeners);
            }
            allListeners.add(listener);
        }

        // When we do that, the selector get notified. Therefore we will always check the initial state automatically. If the
        // selectable is already done with, the listener will be called by the selector's handler.
        messenger.register(selector);

        return true;
    }

    /*
     * Any sort of listener type.
     */
    static abstract class ListenerContainer<S extends SimpleSelectable, L extends java.util.EventListener> extends ArrayList<L> {

        public ListenerContainer() {
            super(1);
        }

        protected abstract void giveUp(S what, Throwable how);

        protected abstract void process(S what);
    }


    /**
     * For messages
     */
    @SuppressWarnings("serial")
    class MessageListenerContainer extends ListenerContainer<Message, OutgoingMessageEventListener> {

        private void messageDone(Message message, OutgoingMessageEvent event) {
            // Note: synchronization is externally provided. When this method is invoked, this
            // object has already been removed from the map, so the list of listener cannot change.

            if (event == OutgoingMessageEvent.SUCCESS) {
                // Replace it with a msg-specific one.
                event = new OutgoingMessageEvent(message, null);

                for (OutgoingMessageEventListener eachListener : this) {
                    try {
                        eachListener.messageSendSucceeded(event);
                    } catch (Throwable any) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Uncaught throwable from listener", any);
                        }
                    }
                }
            } else {
                if (event == OutgoingMessageEvent.OVERFLOW) {
                    // Replace it with a msg-specific one.
                    event = new OutgoingMessageEvent(message, null);
                }

                for (OutgoingMessageEventListener eachListener : this) {
                    try {
                        eachListener.messageSendFailed(event);
                    } catch (Throwable any) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Uncaught throwable in listener", any);
                        }
                    }
                }
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void process(Message message) {
            OutgoingMessageEvent event = (OutgoingMessageEvent) message.getMessageProperty(Messenger.class);

            if (event == null) {
                return;
            }

            // Remove this container-selectable binding
            forgetSelectable(message);

            // Invoke app listeners
            messageDone(message, event);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void giveUp(Message m, Throwable how) {
            messageDone(m, new OutgoingMessageEvent(m, how));
        }
    }

    /**
     * For messengers
     */
    @SuppressWarnings("serial")
    class MessengerListenerContainer extends ListenerContainer<Messenger, MessengerEventListener> {

        private void messengerDone(Messenger messenger) {

            // Note: synchronization is externally provided. When this method is invoked, this
            // object has already been removed from the map, so the list of listener cannot change.

            MessengerEvent event = new MessengerEvent(ListenerAdaptor.this, messenger, null);

            for (MessengerEventListener eachListener : this) {
                try {
                    eachListener.messengerReady(event);
                } catch (Throwable any) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Uncaught throwable in listener", any);
                    }
                }
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void process(Messenger messenger) {
            if ((messenger.getState() & (Messenger.RESOLVED | Messenger.TERMINAL)) == 0) {
                return;
            }

            // Remove this container-selectable binding
            forgetSelectable(messenger);

            if ((messenger.getState() & Messenger.USABLE) == 0) {
                messenger = null;
            }

            // Invoke app listeners
            messengerDone(messenger);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        protected void giveUp(Messenger m, Throwable how) {
            messengerDone(null);
        }
    }

    /**
     * {@inheritDoc}
     */
    public void run() {
        try {
            while (!shutdown) {
                try {
                    Collection<SimpleSelectable> changed = selector.select();
                    for (SimpleSelectable simpleSelectable : changed) {
                        ListenerContainer listeners;
                        synchronized (this) {
                            listeners = inprogress.get(simpleSelectable.getIdentityReference());
                        }
                        if (listeners == null) {
                            simpleSelectable.unregister(selector);
                            continue;
                        }
                        if (executor == null) {
                            listeners.process(simpleSelectable);
                        } else {
                            executor.execute(new ListenerProcessor(listeners, simpleSelectable));
                        }
                    }
                } catch (InterruptedException ie) {
                    Thread.interrupted();
                }
            }
        } catch (Throwable anyOther) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in background thread", anyOther);
            }

            // There won't be any other thread. This thing is dead if that
            // happens. And it really shouldn't.
            synchronized (this) {
                shutdown = true;
            }
        } finally {
            try {
                // It's only us now. Stopped is true.
                IOException failed = new IOException("Endpoint interface terminated");
                for (Map.Entry<IdentityReference, ListenerContainer> entry : inprogress.entrySet()) {
                    SimpleSelectable simpleSelectable = entry.getKey().getObject();
                    ListenerContainer listeners = entry.getValue();
                    simpleSelectable.unregister(selector);

                    if (listeners != null) {
                        listeners.giveUp(simpleSelectable, failed);
                    }
                }
                inprogress.clear();
            } catch (Throwable anyOther) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable while shutting down background thread", anyOther);
                }
            }
            bgThread = null;
        }
    }

    /**
     * A small class for processing individual messages.
     */
    private class ListenerProcessor implements Runnable {

        private SimpleSelectable simpleSelectable;
        private ListenerContainer listeners;
        ListenerProcessor(ListenerContainer listeners, SimpleSelectable simpleSelectable) {
            this.listeners = listeners;
            this.simpleSelectable = simpleSelectable;
        }

        public void run() {
            listeners.process(simpleSelectable);
        }
    }
}
