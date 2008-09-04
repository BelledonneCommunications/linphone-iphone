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

/**
 * The complete standard messenger life cycle state machine that all messengers
 * must obey. Compliant messengers can be built by implementing and using this
 * class as an engine to orchestrate discrete operations.
 * <p/>
 * In order to use this class, one must implement the various abstract Action
 * methods, so that they trigger the required operations.
 * <p/>
 * Synchronization has to be externally provided and usually needs to extend
 * around sections wider than just the invocation of this class' methods. For
 * example, if the user of this class maintains a queue, the state of the queue
 * must be kept consistent with the invocation of {@link #msgsEvent},
 * {@link #saturatedEvent}, and {@link #idleEvent}, which all denote different
 * states of that queue. It is suggested to use the instance of this class as
 * the synchronization object.
 */
public abstract class MessengerState {

    // All the transition map setup is rather terse because java tends to make 
    // it extremely verbose. We do not want to end up with 1000 lines of code 
    // for what amounts to initializing a table.

    // Below is a method reference. It permits putting "what to do" in a field.
    // The {@code doIt()} method is given the target object because we want our
    // entire transition table to be a static singleton. Otherwise it would cost
    // too much initializing each instance of this class.

    private interface Action {
        public void doIt(MessengerState messengerState);
    }

    // Action method "pointers".
    // The transition table is static. Otherwise it would cost too much initializing each instance of this class.

    private final static Action Connect = new Action() {
        public void doIt(MessengerState messengerState) {
            messengerState.connectAction();
        }
    };
    private final static Action Closein = new Action() {
        public void doIt(MessengerState s) {
            s.closeInputAction();
        }
    };
    private final static Action Start = new Action() {
        public void doIt(MessengerState s) {
            s.startAction();
        }
    };
    private final static Action Closeout = new Action() {
        public void doIt(MessengerState messengerState) {
            messengerState.closeOutputAction();
        }
    };
    private final static Action Failall = new Action() {
        public void doIt(MessengerState messengerState) {
            messengerState.failAllAction();
        }
    };
    private final static Action Closeio = new Action() {
        public void doIt(MessengerState s) {
            s.closeInputAction();
            s.closeOutputAction();
        }
    };
    private final static Action Closefail = new Action() {
        public void doIt(MessengerState messengerState) {
            messengerState.closeInputAction();
            messengerState.failAllAction();
        }
    };
    private final static Action Nop = new Action() {
        public void doIt(MessengerState messengerState) {
        }
    };

    /**
     * The transition each event causes when in that state.
     */
    private static class State {
        private final int number;

        private State stResolve;
        private Action acResolve;

        private State stMsgs;
        private Action acMsgs;

        private State stSaturated;
        private Action acSaturated;

        private State stClose;
        private Action acClose;

        private State stShutdown;
        private Action acShutdown;

        private State stUp;
        private Action acUp;

        private State stDown;
        private Action acDown;

        private State stIdle;
        private Action acIdle;

        State(int stateNum) {
            number = stateNum;
        }

        void init(Object[] data) {
            stResolve = (State) data[0];
            acResolve = (Action) data[1];
            stMsgs = (State) data[2];
            acMsgs = (Action) data[3];
            stSaturated = (State) data[4];
            acSaturated = (Action) data[5];
            stClose = (State) data[6];
            acClose = (Action) data[7];
            stShutdown = (State) data[8];
            acShutdown = (Action) data[9];
            stUp = (State) data[10];
            acUp = (Action) data[11];
            stDown = (State) data[12];
            acDown = (Action) data[13];
            stIdle = (State) data[14];
            acIdle = (Action) data[15];
        }
    }


    // All the states. (We put them together in a class essentially to simplify initialization).
    private static class TransitionMap {

        private final static State Unresolved = new State(Messenger.UNRESOLVED);
        private final static State ResPending = new State(Messenger.RESOLPENDING);
        private final static State Resolving = new State(Messenger.RESOLVING);
        private final static State ResolSat = new State(Messenger.RESOLSATURATED);
        private final static State Connected = new State(Messenger.CONNECTED);
        private final static State Disconned = new State(Messenger.DISCONNECTED);
        private final static State Reconning = new State(Messenger.RECONNECTING);
        private final static State ReconSat = new State(Messenger.RECONSATURATED);
        private final static State Sending = new State(Messenger.SENDING);
        private final static State SendingSat = new State(Messenger.SENDINGSATURATED);
        private final static State ResClosing = new State(Messenger.RESOLCLOSING);
        private final static State ReconClosing = new State(Messenger.RECONCLOSING);
        private final static State Closing = new State(Messenger.CLOSING);
        private final static State Disconning = new State(Messenger.DISCONNECTING);
        private final static State Unresable = new State(Messenger.UNRESOLVABLE);
        private final static State Closed = new State(Messenger.CLOSED);
        private final static State Broken = new State(Messenger.BROKEN);

        // The states need to exist before init, because they refer to each other.
        // We overwrite them in-place with the complete data.

        private final static Object[][] INIT_TRANSITION_MAP = {

                /* STATE resolve, msgs, saturated, close, shutdown, up, down, idle */

                /* UNRESOLVED      */
                {Resolving, Connect, ResPending, Connect, ResolSat, Connect, Closed, Closein, Broken, Closein, Connected, Nop, Unresolved, Nop, Unresolved, Nop},

                /* RESOLPENDING    */
                {ResPending, Nop, ResPending, Nop, ResolSat, Nop, ResClosing, Closein, Broken, Closefail, Sending, Start, Unresable, Closefail, Resolving, Nop},

                /* RESOLVING       */
                {Resolving, Nop, ResPending, Nop, ResolSat, Nop, Closed, Closein, Broken, Closein, Connected, Nop, Unresable, Closein, Resolving, Nop},

                /* RESOLSATURATED  */
                {ResolSat, Nop, ResPending, Nop, ResolSat, Nop, ResClosing, Closein, Broken, Closefail, SendingSat, Start, Unresable, Closefail, Resolving, Nop},

                /* CONNECTED       */
                {Connected, Nop, Sending, Start, SendingSat, Start, Closed, Closeio, Broken, Closeio, Connected, Nop, Disconned, Nop, Connected, Nop},

                /* DISCONNECTED    */
                {Disconned, Nop, Reconning, Connect, ReconSat, Connect, Closed, Closein, Broken, Closein, Connected, Nop, Disconned, Nop, Disconned, Nop},

                /* RECONNECTING    */
                {Reconning, Nop, Reconning, Nop, ReconSat, Nop, ReconClosing, Closein, Broken, Closefail, Sending, Start, Broken, Closefail, Disconned, Nop},

                /* RECONSATURATED  */
                {ReconSat, Nop, Reconning, Nop, ReconSat, Nop, ReconClosing, Closein, Broken, Closefail, SendingSat, Start, Broken, Closefail, Disconned, Nop},

                /* SENDING         */
                {Sending, Nop, Sending, Nop, SendingSat, Nop, Closing, Closein, Disconning, Closeio, Sending, Nop, Reconning, Connect, Connected, Nop},

                /* SENDINGSATURATED*/
                {SendingSat, Nop, Sending, Nop, SendingSat, Nop, Closing, Closein, Disconning, Closeio, SendingSat, Nop, ReconSat, Connect, Connected, Nop},

                /* RESOLCLOSING    */
                {ResClosing, Nop, ResClosing, Nop, ResClosing, Nop, ResClosing, Nop, Broken, Failall, Closing, Start, Unresable, Failall, ResClosing, Nop},

                /* RECONCLOSING    */
                {ReconClosing, Nop, ReconClosing, Nop, ReconClosing, Nop, ReconClosing, Nop, Broken, Failall, Closing, Start, Broken, Failall, ReconClosing, Nop},

                /* CLOSING         */
                {Closing, Nop, Closing, Nop, Closing, Nop, Closing, Nop, Disconning, Closeout, Closing, Nop, ReconClosing, Connect, Closed, Closeout},

                /* DISCONNECTING   */
                {Disconning, Nop, Disconning, Nop, Disconning, Nop, Disconning, Nop, Disconning, Nop, Disconning, Nop, Broken, Failall, Broken, Nop},

                /* UNRESOLVABLE    */
                {Unresable, Nop, Unresable, Nop, Unresable, Nop, Unresable, Nop, Unresable, Nop, Unresable, Closeout, Unresable, Nop, Unresable, Nop},

                /* CLOSED          */
                {Closed, Nop, Closed, Nop, Closed, Nop, Closed, Nop, Closed, Nop, Closed, Closeout, Closed, Nop, Closed, Nop},

                /* BROKEN          */
                {Broken, Nop, Broken, Nop, Broken, Nop, Broken, Nop, Broken, Nop, Broken, Closeout, Broken, Nop, Broken, Nop}
        };

        static {
            // install the transitions map in its proper place.
            Unresolved.init(INIT_TRANSITION_MAP[0]);
            ResPending.init(INIT_TRANSITION_MAP[1]);
            Resolving.init(INIT_TRANSITION_MAP[2]);
            ResolSat.init(INIT_TRANSITION_MAP[3]);
            Connected.init(INIT_TRANSITION_MAP[4]);
            Disconned.init(INIT_TRANSITION_MAP[5]);
            Reconning.init(INIT_TRANSITION_MAP[6]);
            ReconSat.init(INIT_TRANSITION_MAP[7]);
            Sending.init(INIT_TRANSITION_MAP[8]);
            SendingSat.init(INIT_TRANSITION_MAP[9]);
            ResClosing.init(INIT_TRANSITION_MAP[10]);
            ReconClosing.init(INIT_TRANSITION_MAP[11]);
            Closing.init(INIT_TRANSITION_MAP[12]);
            Disconning.init(INIT_TRANSITION_MAP[13]);
            Unresable.init(INIT_TRANSITION_MAP[14]);
            Closed.init(INIT_TRANSITION_MAP[15]);
            Broken.init(INIT_TRANSITION_MAP[16]);
        }
    }

    /**
     * The current state!
     */
    private volatile State state = null;

    /**
     * Constructs a new messenger state machine.
     * <p/>
     * The transition map is static and we refer to it only to grab the first
     * state. After that, states refer to each other. The only reason they are
     * members in the map is so that we can make references during init.
     *
     * @param connected If <tt>true</tt>, the initial state is {@link Messenger#CONNECTED} otherwise {@link Messenger#UNRESOLVED}.
     */
    protected MessengerState(boolean connected) {

        state = connected ? TransitionMap.Connected : TransitionMap.Unresolved;
    }

    /**
     * @return The current state.
     */
    public int getState() {
        // getState is always just a peek. It needs no sync.
        return state.number;
    }

    /**
     * Event input.
     */
    public void resolveEvent() {
        Action a = state.acResolve;

        state = state.stResolve;
        a.doIt(this);
    }

    /**
     * Message event
     */
    public void msgsEvent() {
        Action a = state.acMsgs;

        state = state.stMsgs;
        a.doIt(this);
    }

    /**
     * Saturated Event
     */
    public void saturatedEvent() {
        Action a = state.acSaturated;

        state = state.stSaturated;
        a.doIt(this);
    }

    /**
     * Close Event
     */
    public void closeEvent() {
        Action a = state.acClose;

        state = state.stClose;
        a.doIt(this);
    }

    /**
     * Shutdown Event
     */
    public void shutdownEvent() {
        Action a = state.acShutdown;

        state = state.stShutdown;
        a.doIt(this);
    }

    /**
     * Up Event
     */
    public void upEvent() {
        Action a = state.acUp;

        state = state.stUp;
        a.doIt(this);
    }

    /**
     * Down Event
     */
    public void downEvent() {
        Action a = state.acDown;

        state = state.stDown;
        a.doIt(this);
    }

    /**
     * Idle Event
     */
    public void idleEvent() {
        Action a = state.acIdle;

        state = state.stIdle;
        a.doIt(this);
    }

    /**
     * Actions they're always called in sequence by event methods.
     *
     * Actions must not call event methods in sequence.
     **/

    /**
     * Try to make a connection. Called whenever transitioning from a state that neither needs nor has a connection to a state
     * that needs a connection and does not have it. Call upEvent when successful, or downEvent when failed.
     */
    protected abstract void connectAction();

    /**
     * Start sending. Called whenever transitioning to a state that has both a connection and messages to send from a state that
     * lacked either attributes. So, will be called after sending stopped due to broken cnx or idle condition.  Call downEvent
     * when stopping due to broken or closed connection, call {@link #idleEvent} when no pending message is left.
     */
    protected abstract void startAction();

    /**
     * Reject new messages from now on. Called whenever transitioning from a state that is {@link Messenger#USABLE} to a state
     * that is not. No event expected once done.
     */
    protected abstract void closeInputAction();

    /**
     * Drain pending messages, all failed. Called once output is down and there are still pending messages.
     * Call {@link #idleEvent} when done, as a normal result of no pending messages being left.
     */
    protected abstract void failAllAction();

    /**
     * Force close output. Called whenever the underlying connection is to be discarded and never to be needed again.  That is
     * either because orderly close has completed, or shutdown is in progress. No event expected once done, but this action
     * <b>must</b> cause any sending in progress to stop eventually. The fact that the sending has stopped must be reported as
     * usual: either with a {@link #downEvent}, if the output closure caused the sending process to fail, or with an {@link
     * #idleEvent} if the sending of the last message could be sent successfully despite the attempt at interrupting it.
     * <p/>
     * Sending is said to be in progress if, and only if, the last call to startAction is more recent than the last call to
     * {@link #idleEvent} or {@link #downEvent}.
     * <p/>
     * It is advisable to also cancel an ongoing connect action, but not mandatory. If a {@link #connectAction} later succeeds,
     * then {@link #upEvent} <b>must</b> be called as usual. This will result in another call to {@link #closeOutputAction}.
     */
    protected abstract void closeOutputAction();
}
