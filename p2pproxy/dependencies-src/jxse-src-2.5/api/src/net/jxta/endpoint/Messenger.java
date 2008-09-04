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
import net.jxta.util.SimpleSelectable;

import java.io.IOException;


/**
 * A Messenger is used to send messages to a destination.
 * <p/>
 * This interface specifies the allowed observable states for a messenger. (fine grain).  This serves to normalize
 * the state machines of the various messenger implementations and allows for more meaningful diagnostics. Implementations may
 * use substates by adding high order bits, but these should never be reported by the public state observation methods. Most
 * implementations will not use all these states.
 * <p/>
 * <p/>Each valid state is represented by a integer that is a power of 2.
 * <p/>
 * <p/>The (coarse grain) constants: <code>USABLE, RESOLVED, TERMINAL, IDLE, SATURATED</code> represent meaningful partitions of the space of
 * states.
 * <p/>
 * <p/>The value of each constant is the bitwise <code>OR</code> of the states for which a given predicate is true: usable or not,
 * confirmed or not, etc.  Therefore the value of predicate <code>p</code> in state <code>s</code> is <code>(s & p)!=0</code>.
 * <p/>
 * <p/>These particular predicates are chosen so that they have a relevant truth value for all states. Therefore the bitwise negation
 * of the corresponding constants represents the obvious: <code>~USABLE</code> really lists all states that mean "not USABLE".
 * <p/>
 * <p/>These constants may be combined by bit logic operators to represent more predicates.  {@link #waitState} accepts such values as
 * a parameter.
 * <p/>
 * <p/>Applications should depend on the coarse grain constants, rather than those denoting discrete states.
 *
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.util.SimpleSelector
 * @see net.jxta.endpoint.EndpointAddress
 * @see net.jxta.endpoint.Message
 * @see MessengerState
 */
public interface Messenger extends SimpleSelectable {

    /**
     * No message was ever submitted for sending. No connection has ever been attempted.
     */
    public static final int UNRESOLVED = 0x1;

    /**
     * Initial connection is being attempted. No message is pending.
     */
    public static final int RESOLVING = 0x2;

    /**
     * Currently connected. No message is pending (being sent implies pending).
     */
    public static final int CONNECTED = 0x4;

    /**
     * Currently not connected. No message is pending.
     */
    public static final int DISCONNECTED = 0x8;

    /**
     * Initial connection is being attempted. Messages are pending.
     */
    public static final int RESOLPENDING = 0x10;

    /**
     * Initial connection is being attempted. Messages are pending. New messages may not be submitted at this time.
     */
    public static final int RESOLSATURATED = 0x20;

    /**
     * Currently connected and sending messages.
     */
    public static final int SENDING = 0x40;

    /**
     * Currently sending messages.New messages may not be submitted at this time.
     */
    public static final int SENDINGSATURATED = 0x80;

    /**
     * Currently trying to re-establish connection. Messages are pending.
     */
    public static final int RECONNECTING = 0x100;

    /**
     * Currently trying to re-establish connection. New messages may not be submitted at this time.
     */
    public static final int RECONSATURATED = 0x200;

    /**
     * Attempting initial connection. Close has been requested. Messages are pending.
     * New messages may no longer be submitted.
     */
    public static final int RESOLCLOSING = 0x400;

    /**
     * Currently sending messages. Close has been requested. New messages may no longer be submitted.
     */
    public static final int CLOSING = 0x800;

    /**
     * Trying to re-establish connection. Close has been requested. Messages are pending.
     * New messages may no longer be submitted.
     */
    public static final int RECONCLOSING = 0x1000;

    /**
     * Failed to establish initial connection. Pending messages are being rejected. New messages may no longer be submitted.
     */
    public static final int UNRESOLVING = 0x2000;

    /**
     * Failed to re-establish connection.  Pending messages are being rejected. New messages may no longer be submitted.
     */
    public static final int BREAKING = 0x4000;

    /**
     * Breaking established connection for expedite closure. Pending messages are being rejected.
     * New messages may no longer be submitted.
     */
    public static final int DISCONNECTING = 0x8000;

    /**
     * Failed to establish initial connection. New messages may no longer be submitted. State will never change again.
     */
    public static final int UNRESOLVABLE = 0x10000;

    /**
     * Failed to re-establish connection.  New messages may no longer be submitted. State will never change again.
     */
    public static final int BROKEN = 0x20000;

    /**
     * Closed as requested. All pending messages could be sent. New messages may no longer be submitted.
     * State will never change again.
     */
    public static final int CLOSED = 0x40000;

    /**
     * The bitwise OR of all valid states.
     */
    public static final int ANYSTATE = 0x7FFFF;

    /* Predicates. */

    /**
     * Composite state.<p/>
     * <p/>
     * Not known to be broken.
     * Messenger may be used to send messages. Viability has not been evaluated yet.
     * This is the most useful predicate to applications. USABLE means that
     * it is reasonable to try and send a message.
     */
    public static final int USABLE = (UNRESOLVED | RESOLVING | CONNECTED | DISCONNECTED | RESOLPENDING | RESOLSATURATED | SENDING
            | SENDINGSATURATED | RECONNECTING | RECONSATURATED);

    /**
     * Composite state.
     * <p/>
     * <ul>
     * <li>Messenger was once resolved.</p>
     * <li>Messenger was at least once proven viable. Current usability is
     * not asserted. For example a messenger may be found to be in a
     * TERMINAL state, but also be in a RESOLVED state. Thus proving that
     * the destination of the messenger is sometimes valid.</li>
     * </ul>
     */
    public static final int RESOLVED = (CONNECTED | SENDING | SENDINGSATURATED | CLOSING | CLOSED | DISCONNECTED | RECONNECTING
            | RECONSATURATED | RECONCLOSING | BREAKING | DISCONNECTING | BROKEN);

    /**
     * Composite state.
     * <p/>
     * Messenger has terminated its useful life. State will never change any more.
     */
    public static final int TERMINAL = (UNRESOLVABLE | CLOSED | BROKEN);

    /**
     * Composite state.
     * <p/>
     * Any message that may have been submitted in the past has been either sent or failed already.
     * If a messenger is in a state <code>IDLE & RESOLVED & USABLE</code>, then the expected delay in sending a message
     * is minimal.
     */
    public static final int IDLE = (UNRESOLVED | RESOLVING | CONNECTED | DISCONNECTED | UNRESOLVABLE | CLOSED | BROKEN);

    /**
     * Composite state.
     * <p/>
     * This messenger cannot take new messages for now. All available buffering space is occupied.
     * Note that only the usable states can be saturated. Once a messenger is in the process of terminating
     * and thus takes no new messages anyway, it no-longer shows as saturated.
     */
    public static final int SATURATED = (RESOLSATURATED | SENDINGSATURATED | RECONSATURATED);

    /**
     * Returns the current state.
     *
     * @return one of the legal discrete state values.
     */
    public int getState();

    /**
     * Blocks unless and until the current state is or has become one of the desired values.  The observable states are guaranteed
     * to be represented by a single bit. Multiple desired values may be specified by passing them <code>OR</code>ed together.
     * <p/>
     * This class defines the list of constants that may be used and how these may be combined.
     * <p/>
     * Note that the state can change as soon as this method returns, so any observation is only an indication of the
     * past. Synchronizing on the object itself has no other effect than interfering with other threads doing the same. Obviously,
     * certain transitions cannot happen unless a new message is submitted. So unless another thread is using a messenger, it is
     * safe to assume that a non-saturated messenger will not become saturated spontaneously. Note that messengers returned by
     * different endpoint service interface objects (what {@link net.jxta.peergroup.PeerGroup#getEndpointService()} returns) are
     * different. However a given endpoint interface object will return an existing messenger to the same exact destination if
     * there is a {@link Messenger#USABLE} one.
     * <p/>
     * With an unshared messenger, one can wait for any change with {@code waitState(~getState(), 0);}.
     * <p/>
     * Note that it is advisable to always include {@link #TERMINAL} as part
     * of the desired states with unless being blocked past the messenger
     * termination is an acceptable behaviour.
     * <p/>
     * Examples:<p/>
     * <p/>
     * Ensure that the messenger can take more messages (or is {@code UNUSABLE}): {@code waitState(~SATURATED)}<p/>
     * <p/>
     * Ensure that all submitted messages have been sent: {@code waitState(TERMINAL | IDLE)}
     * <p/>
     * Ensure that the messenger is already resolved and can take more messages: {@code waitState(TERMINAL | (RESOLVED & ~SATURATED))}
     *
     * @param wantedStates The binary OR of the desired states.
     * @param timeout      The maximum number of milliseconds to wait. A timeout of 0 means no time limit.
     * @return The desired state that was reached or the current state when time ran out.
     * @throws InterruptedException If the invoking thread was interrupted before the condition was realized.
     */
    int waitState(int wantedStates, long timeout) throws InterruptedException;

    /**
     * Returns {@code true} if this messenger is closed and no longer
     * accepting messages to be sent. This is a shortcut for 
     * {@code (getState() & USABLE == 0)}. Once closed, a messenger should be 
     * discarded. 
     *
     * @return {@code true} if this messenger is closed, otherwise {@code false}.
     */
    boolean isClosed();

    /**
     * Returns an indication of whether this messenger may be worth closing.
     *
     * @return {@code true} if the messenger is idle otherwise {@code false}.
     * @deprecated no longer relevant and always false. This notably is <bold>not</bold> equivalent to the {@link #IDLE} state.
     */
    @Deprecated
    boolean isIdle();

    /**
     * Returns {@code true} if the <code>sendMessage</code> methods of
     * this messenger are fully synchronous.
     *
     * @deprecated all messengers are asynchronous, and the {@link #sendMessageB} method is always blocking.
     *
     * @return true if synchronous
     */
    @Deprecated
    boolean isSynchronous();

    /**
     * Returns the destination of this messenger.
     *
     * @return The destination address of this messenger
     * @see Messenger#getLogicalDestinationAddress()
     */
    EndpointAddress getDestinationAddress();

    /**
     * Returns the internal EndpointAddress object of the destination of the user. 
     * This method is intended to be used for applications that require a weak
     * or soft reference to an EndpointMessenger: the returned Endpoint Address 
     * object will be unreferenced when this messenger will finalize.
     *
     * @deprecated EndpointAddress is now immutable which means that this result
     * is the same as {@link #getDestinationAddress()}. This method will be
     * eventually removed.
     *
     * @return EndpointAddress the destination address of this messenger
     * @see #getDestinationAddress()
     */
    @Deprecated
    EndpointAddress getDestinationAddressObject();

    /**
     * Returns the logical destination of this messenger. This may be a
     * different address than is returned by
     * {@link #getDestinationAddress() getDestinationAddress} and refers to
     * the entity which is located at the destination address.
     * <p/>
     * By analogy, a telephone number would be the destination address, and
     * the owner of that telephone number would be the logical destination.
     * Each logical destination may be known by one or more destination
     * addresses.
     *
     * @return EndpointAddress the logical destination address of this messenger.
     * @see #getDestinationAddress()
     */
    EndpointAddress getLogicalDestinationAddress();

    /**
     * Returns the maximum message size that this messenger can be used to send.
     * The limit refers to the cumulative size of application level elements.
     * The various {@code sendMessage()} variants will refuse to send messages
     * that exceed this limit.
     *
     * @return the limit.
     */
    long getMTU();

    /**
     * If applicable, returns another messenger that will send messages to the same destination address than this one, but with
     * the specified default service and serviceParam, possibly rewriting addresses to ensure delivery through the specified
     * redirection. This is not generally useful to applications and most messengers will return null. This method is needed
     * by the EndpointService when interacting with Messengers provided by Transport modules. If you are not implementing a
     * Transport module, then you can ignore this method.
     * <p/>
     * <b>Important:</b> The channel so obtained is not configured to support the {@link #sendMessage(Message,String,String,
     *OutgoingMessageEventListener)} legacy method. If use of this method is desired, {@link ChannelMessenger#setMessageWatcher}
     * must be used first.
     *
     * @param redirection  The requested redirection. The resulting channel messenger will use this to force
     *                     delivery of the message only in the specified group (or possibly descendents, but not parents). If null the local
     *                     group is assumed. This redirection is applied only to messages that are sent to a service name and service param that
     *                     do not imply a group redirection.
     * @param service      The service to which the resulting channel will send messages, when they are not sent to a
     *                     specified service.
     * @param serviceParam The service parameter that the resulting channel will use to send messages, when no parameter is
     *                     specified.
     * @return a channelMessenger as specified.
     * @see MessageSender#getMessenger(EndpointAddress,Object)
     */
    Messenger getChannelMessenger(PeerGroupID redirection, String service, String serviceParam);

    /**
     * Close this messenger after processing any pending messages. This method 
     * is not blocking. Upon return, the messenger will be in one of the non 
     * {@link #USABLE} states, which means that no message may be sent through 
     * it. Any other effect of this method, such as an underlying connection 
     * being closed, or all pending messages being processed, may be deferred
     * indefinitely.  When the messenger has completely processed the closure 
     * request, it will be in one of the {@link #TERMINAL} states (which are 
     * also {@link #IDLE} states). Therefore, if one is interested in the 
     * outcome of the closure, one may wait for the messenger to be in a 
     * {@link #TERMINAL} or {@link #IDLE} state, and check which it is. 
     * {@link #CLOSED} denotes success (all outstanding messages have been 
     * sent), as opposed to {@link #UNRESOLVABLE} or {@link #BROKEN}.
     */
    void close();

    /**
     * Makes sure that all outstanding messages have been processed; 
     * successfully or not. This method waits unless and until the state of the 
     * messenger is an {@link #IDLE} state. If the reached state is neither 
     * {@link #CLOSED} or any {@link #USABLE} state, then it throws an 
     * IOException. Else it returns silently.<p/>
     * 
     * <p/><b>If another thread keeps sending messages, this method may never 
     * return.</b>
     * 
     * <p/>This method is deliberately simple. If a timeout needs to be 
     * provided, or if more detailed conditions are required, the 
     * {@link #waitState(int,long)} and {@link #getState()} methods should be 
     * used. For example : 
     * 
     * <p/><code><pre>
     * int myFlush(long notTooLong) {
     *   messenger.waitState(IDLE, notTooLong);
     *   if ((messenger.getState() & IDLE) == 0) return TOOLONG;
     *   if ((messenger.getState() & USABLE) != 0) return FLUSHED;
     *   if (messenger.getState() == CLOSED) return CLOSEDANDFLUSHED;
     *   return BROKEN;
     * }
     * </pre></code>
     * 
     * <p/>Note: {@link #close()} being asynchronous, it is valid to invoke 
     * <code>flush()</code> after <code>close()</code> as a form of synchronous 
     * variant of <code>close()</code>. If this messenger is not shared with any 
     * other thread, then invoking <code>flush()</code> before 
     * <code>close</code> is a more familiar means of achieving the same effect.
     *
     * @throws IOException This messenger failed before processing all 
     * outstanding messages successfully.
     */
    void flush() throws IOException;

    /**
     * Force the messenger to start resolving if it is not resolved yet. Any 
     * attempt at sending a message has the same effect, but the message may 
     * fail as a result, depending upon the method used.
     */
    void resolve();

    /**
     * Simple sending: blocks until the message was accepted for sending or the messenger is not {@link #USABLE}; whichever occurs
     * first.  If a service name and service param are specified, they will replace those specified at construction for the
     * purpose of sending this message only.<p/>
     * <p/>
     * Error Handling:
     * <p/>
     * <ul>
     * <li>An {@link java.io.IOException} means that this message is invalid or that this messenger is now in one of the non {@link
     * #USABLE} states and may no longer send new messages, and means that the message was not sent. The exact state of the
     * messenger may be obtained from the {@link #getState()} method. If no exception is thrown, the message is accepted for
     * sending and may or may not be fully processed.</li>
     * <p/>
     * <li>The invoker may have confirmation of completion by observing the message's properties. When the message has been
     * fully processed, {@link Message#getMessageProperty(Object) Message#getMessageProperty(Messenger.class)} will return an
     * object of class {@link OutgoingMessageEvent}. Changes in a message's set of properties may be tracked by selecting the
     * message. If an exception was thrown, the message's properties will not be modified
     * <p/>
     * <li>There is no guarantee that the process of sending the message will not fail after that method returned. If this messenger
     * subsequently reaches an {@link #IDLE} state that is either {@link #CLOSED} or a {@link #USABLE} state, then it may be inferred
     * that all outstanding messages have been processed without this messenger detecting an error.</li>
     * </ul>
     * <p/>
     * <p/><b>WARNING:</b> The Message object should not be reused or modified until completely processed. Concurrent modification
     * of a message while a messenger is sending the message will produce incorrect and unpredictable results.  If completion is
     * not monitored, the message should <b>never</b> be reused. If necessary, a clone of the message may be provided to
     * {@link #sendMessageB}:
     * <p/>
     * <p/><code><pre>
     *     messenger.sendMessageB( (Message) myMessage.clone(), theService, theParam );
     * </pre></code>
     * <p/>
     * There is no guarantee that a message successfully sent will actually be received.<p/>
     *
     * @param msg          the message
     * @param service      Optionally replaces the service in the destination
     *                     address. If {@code null} then the destination address's default
     *                     service will be used.  If the empty string ("") is used then
     *                     no service is included in the destination address.
     * @param serviceParam Optionally replaces the service param in the
     *                     destination address. If {@code null} then the destination address's
     *                     default service parameter will be used. If the empty string ("") is used
     *                     then no service param is included in the destination address.
     * @throws IOException Thrown if the message cannot be sent.
     */
    void sendMessageB(Message msg, String service, String serviceParam) throws IOException;

    /**
     * Sends a message to the destination specified at construction. If a service name and service param are specified, they will
     * replace those specified at construction for the purpose of sending this message only.
     * <p/>
     * This method is identical to {@link #sendMessage(Message,String,String)}, except that it does not throw an exception. The invoker
     * has to retrieve a detailed status from the message if needed.
     * <p/>
     * Error Handling:  <ul>
     * <li>A return result of {@code false} indicates that the message was not accepted to be sent. This may be due to
     * local resource limits being reached or to the messenger being in a state that is not {@link #USABLE} or to the message
     * being invalid. The exact cause of the failure can be retrieved from the message by using
     * {@link Message#getMessageProperty(Object) <code>Message.getMessageProperty(Messenger.class)</code>}.  If appropriate,
     * another attempt at sending the message, may be made after waiting for the congestion to clear (for example by using
     * {@link #waitState(int,long)}).</li>
     * <p/>
     * <li>A return result of {@code true} indicates that the message was accepted for sending. <b>It does not imply that
     * the message will be sent or that the destination will receive the message.</b> There will be no indication by this method
     * of any errors in sending the message. If this messenger subsequently reaches an {@link #IDLE} state that is either {@link
     * #CLOSED} or a {@link #USABLE} state, then it may be inferred that all outstanding messages have been processed without this
     * messenger detecting an error.</li>
     * <p/>
     * <li>The invoker may have confirmation of completion (successful or not) by observing the message's properties. When the
     * message has been fully processed, {@link Message#getMessageProperty(Object) <code>Message.getMessageProperty(Messenger.class)</code>}
     * will return an object of class {@link OutgoingMessageEvent}. Changes in a message's set of properties may be tracked by
     * selecting the message.</li>
     * </ul>
     * <p/>
     * <p/><b>WARNING:</b> The Message object should not be reused or modified until completely processed. Concurrent modification
     * of a message while a messenger is sending the message will produce incorrect and unpredictable results.  If completion is
     * not monitored, the message should <b>never</b> be reused. If necessary, a clone of the message may be provided to
     * {@link #sendMessageN}:
     * <p/>
     * <p/><code><pre>
     *     messenger.sendMessageN( (Message) myMessage.clone(), theService, theParam );
     * </pre></code>
     * <p/>
     * There is no guarantee that a message successfully sent will actually be received.<p/>
     *
     * @param msg          The message to send.
     * @param service      Optionally replaces the service in the destination
     *                     address. If {@code null} then the destination address's default
     *                     service will be used.  If the empty string ("") is used then
     *                     no service is included in the destination address.
     * @param serviceParam Optionally replaces the service param in the
     *                     destination address. If {@code null} then the destination address's
     *                     default service parameter will be used. If the empty string ("") is used
     *                     then no service param is included in the destination address.
     * @return boolean {@code true} if the message has been accepted for sending, otherwise {@code false}.
     */
    boolean sendMessageN(Message msg, String service, String serviceParam);

    /**
     * Sends a message to the destination specified at construction as if by 
     * invoking {@link #sendMessage(Message,String,String) sendMessage(msg, null, null)}
     * 
     * @param msg The message to send.
     * @return boolean {@code true} if the message has been accepted for sending, otherwise {@code false}.
     * @throws IOException Thrown if the message cannot be sent.
     */
    boolean sendMessage(Message msg) throws IOException;

    /**
     * Sends a message to the destination specified at construction. If a service name and service param are specified, they will
     * replace those specified at construction for the purpose of sending this message only.
     * <p/>
     * Error Handling:
     * <p/>
     * <ul>
     * <p/>
     * <li>An {@link java.io.IOException} means that this message is invalid or that this messenger is now in one of the
     * non {@link #USABLE} states and may no longer send new messages, and that the message was not sent. The exact state of
     * the messenger may be obtained from the {@link #getState()} method.</li>
     * <p/>
     * <li>A return result of {@code false} indicates that the message was not accepted to be sent. Usually this is due to
     * local resource limits being reached. If needed, another attempt at sending the message, may be made after waiting for the
     * congestion to clear (for example by using {@link #waitState(int,long)}).</li>
     * <p/>
     * <li>A return result of {@code true} indicates that the message was accepted for sending. <b>It does not imply that
     * the message will be sent or that the destination will receive the message.</b> There will be no immediate indication of any
     * errors in sending the message. If this messenger subsequently reaches an {@link #IDLE} state that is either {@link #CLOSED}
     * or a {@link #USABLE} state, then it may be inferred that all outstanding messages have been processed without this
     * messenger detecting an error.</li>
     * <p/>
     * <li>The invoker may have confirmation of completion by observing the message's properties. When the message has been fully
     * processed, {@link Message#getMessageProperty(Object) <code>Message.getMessageProperty(Messenger.class)</code>} will return
     * an object of class {@link OutgoingMessageEvent}. Changes in a message's set of properties may be tracked by selecting
     * the message.
     * <p/>
     * </ul>
     * <p/>
     * <p/><b>WARNING:</b> The Message object should not be reused or modified until completely processed. Concurrent modification
     * of a message while a messenger is sending the message will produce incorrect and unpredictable results.  If completion is
     * not monitored, the message should <b>never</b> be reused. If necessary, a clone of the message may be provided to
     * <code>sendMessage</code>:
     * <p/>
     * <code><pre>
     *     messenger.sendMessage( (Message) myMessage.clone(), theService, theParam );
     * </pre></code>
     * <p/>
     * There is no guarantee that a message successfully sent will actually be received.<p/>
     * <p/>
     * <b>Limitation:</b> using this method along with {@link net.jxta.util.SimpleSelector#select} on the same message may occasionally
     * cause some errors to not be thrown. Prefer {@link #sendMessageN} when using {@link net.jxta.util.SimpleSelector#select}.
     * <p/>
     * This is a legacy method. Modern code should prefer the other methods and select messages.
     *
     * @param msg          The message to send.
     * @param service      Optionally replaces the service in the destination
     *                     address. If {@code null} then the destination address's default
     *                     service will be used.  If the empty string ("") is used then
     *                     no service is included in the destination address.
     * @param serviceParam Optionally replaces the service param in the
     *                     destination address. If {@code null} then the destination address's
     *                     default service parameter will be used. If the empty string ("") is used
     *                     then no service param is included in the destination address.
     * @return boolean {@code true} if the message has been accepted for sending, otherwise {@code false}.
     * @throws IOException Thrown if the message cannot be sent.
     */
    boolean sendMessage(Message msg, String service, String serviceParam) throws IOException;

    /**
     * Sends a message to the destination specified at construction. If a service name and service param are specified, they will
     * replace those specified at construction for the purpose of sending this message only.
     * <p/>
     * <b>WARNING:</b> The Message object should not be reused or modified until the message has been fully processed.
     * Concurrent modification of a message while a messenger is sending the message will produce incorrect and unpredictable
     * results. If a listener is provided it is invoked after the message is considered fully processed. However it is recommended
     * not to reuse or otherwise modify a messages after sending it. If necessary, a clone of the message may be provided to
     * <code>sendMessage</code>:
     * <p/>
     * <code><pre>
     *     messenger.sendMessage( (Message) myMessage.clone() );
     * </pre></code>
     * <p/>
     * Error Handling:
     * <p/>
     * <ul>
     * <p/>
     * <li>If a listener was provided, it will always be invoked. Depending upon the outcome either the
     * {@link OutgoingMessageEventListener#messageSendFailed(OutgoingMessageEvent)} or the {@link
     * OutgoingMessageEventListener#messageSendSucceeded(OutgoingMessageEvent)} method will be invoked:<p/>
     * <p/>
     * <ul>
     * <p/>
     * <li>If the message could not be accepted for sending due to temporary resource saturation,
     * <code>messageSendFailed</code> will be invoked. The {@link Throwable} object returned by the {@link
     * OutgoingMessageEvent#getFailure()} method of the passed event object will be null.</li>
     * <p/>
     * <li>If the message could not be accepted for sending due to the messenger not being {@link #USABLE},
     * <code>messageSendFailed</code> will be invoked. The {@link Throwable} object returned by the {@link
     * OutgoingMessageEvent#getFailure()} method of the passed event object will reflect the messenger's condition.</li>
     * <p/>
     * <li>If the message is accepted for sending but later fails, then <code>messageSendFailed</code> will be invoked.  The
     * {@link Throwable} object returned by the {@link OutgoingMessageEvent#getFailure()} method of the passed event object
     * will reflect the origin of the failure.</li>
     * <p/>
     * <li>If the message is accepted for sending and later succeeds, then <code>messageSendSucceeded</code> will be
     * invoked.</li>
     * <p/>
     * </ul></li>
     * <p/>
     * <li>If a listener was provided, it is always possible that it is invoked before this method returns.</li>
     * <p/>
     * <li>If no listener was provided, </b> There will be no notification of any errors nor success in sending the message. If
     * this messenger subsequently reaches an {@link #IDLE} state that is either {@link #CLOSED} or a {@link #USABLE} state, then it
     * may be inferred that all outstanding messages have been processed without this messenger detecting an error.</li>
     * <p/>
     * <li>This method does not throw exceptions. As a result, when used with a {@code null} listener, it provides very
     * little feedback to the invoker. A messenger should be abandoned once it is in one of the {@link #TERMINAL} states.</li>
     * <p/>
     * </ul>
     * <p/>
     * As with all <code>sendMessage</code> methods, success is not a guarantee that the message will actually be received.
     * <p/>
     * <p/>This is a legacy method. Modern code should prefer the other methods and select messages. If a listener API is preferred, it is possible to use a {@link ListenerAdaptor} object explicitly to have a listener called.
     *
     * @param msg          The message to send.
     * @param service      Optionally replaces the service in the destination
     *                     address. If {@code null} then the destination address's default
     *                     service will be used.  If the empty string ("") is used then
     *                     no service is included in the destination address.
     * @param serviceParam Optionally replaces the service param in the
     *                     destination address. If {@code null} then the destination address's
     *                     default service parameter will be used. If the empty string ("") is used
     *                     then no service param is included in the destination address.
     * @param listener     listener for events about this message or null if no notification is desired.
     * @throws UnsupportedOperationException If this messenger is not a channel or was not given a {@link ListenerAdaptor}.
     *                                       (all messengers obtained through {@link EndpointService#getMessenger} are configured properly.
     */
    void sendMessage(Message msg, String service, String serviceParam, OutgoingMessageEventListener listener);
}
