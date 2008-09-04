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

package net.jxta.endpoint;


import net.jxta.peergroup.PeerGroup;
import net.jxta.service.Service;

import java.io.IOException;
import java.util.Iterator;


/**
 * The EndpointService provides the API for sending and receiving messages
 * between peers. In general, applications and services use the
 * {@link net.jxta.pipe.PipeService}, or {@link net.jxta.socket.JxtaSocket}
 * rather than using this API directly.
 */
public interface EndpointService extends Service, EndpointListener {

    /**
     * Low Priority Messenger Event Listener.
     */
    public static final int LowPrecedence = 0;

    /**
     * Medium Priority Messenger Event Listener.
     */
    public static final int MediumPrecedence = 1;

    /**
     * High Priority Messenger Event Listener.
     */
    public static final int HighPrecedence = 2;

    /**
     * Returns the group to which this EndpointService is attached.
     *
     * @return the group.
     */
    public PeerGroup getGroup();

    /**
     * Returns a messenger to the specified destination.
     * <p/>
     * The canonical messenger is shared between all channels who's
     * destination contain the same protocol name and protocol address, in all
     * groups that have access to the same transport. The ChannelMessenger
     * returned is configured to send messages to the specified service name and
     * service param when these are not specified at the time of sending.
     * <p/>
     * The channel will also ensure delivery to this EndpointService's group
     * on arrival. The channel is not shared with any other module. That is,
     * each endpoint service interface object (as returned by {@link
     * net.jxta.peergroup.PeerGroup#getEndpointService()}) will return a
     * different channel messenger for the same destination. However, there is
     * no guarantee that two invocations of the same endpoint service interface
     * object for the same destination will return different channel objects.
     * Notably, if the result of a previous invocation is still strongly
     * referenced and in a {@link Messenger#USABLE} state, then that is what
     * this method will return.
     * <p/>
     * This method returns immediately. The messenger is not necessarily
     * resolved (the required underlying connection established, for example),
     * and it might never resolve at all.  Changes in the state of a messenger
     * may monitored with {@link Messenger#getState} and
     * {@link Messenger#waitState}. One may monitor multiple
     * {@link Messenger messengers} (and {@link Message Messages}) at a time by
     * using a {@link net.jxta.util.SimpleSelector}.  One may also arrange to
     * have a listener invoked when resolution is complete by using
     * {@link ListenerAdaptor}.
     * <p/>
     * The {@code hint} is interpreted by the transport. The only transport
     * known to consider hints is the endpoint router, and the hint is a route.
     * As a result, if addr is in the form: jxta://uniqueID, then hint may be a
     * RouteAdvertisement.  If that route is valid the router will add it to
     * it's cache of route and may then use it to successfully create a messenger
     * to the given destination.  There is no guarantee at this time that the
     * route will end up being the one specified, nor that this route will be
     * used only for this messenger (likely the opposite), nor that it will
     * remain in use in the future, nor that it will be used at all. However, if
     * there is no other route, and if the specified route is valid, it will be
     * used rather than seeking an alternative.
     *
     * @param addr The complete destination address.
     * @param hint A optional hint to be supplied to whichever transport ends-up
     *             making the real messenger. May be {@code null}, when no hint applies.
     * @return A messenger for the specified destination address or {@code null}
     *         if the address is not handled by any of the available Message Transports.
     *         The messenger, if returned, is not necessarily functional, nor resolved.
     * @see net.jxta.endpoint.ChannelMessenger
     */
    public Messenger getMessengerImmediate(EndpointAddress addr, Object hint);

    /**
     * Returns a messenger for the specified destination address.
     * <p/>
     * Behaves like {@link #getMessengerImmediate(EndpointAddress,Object)},
     * except that the invoker is blocked until the Messenger either resolves or
     * it is determined that no usable messenger can be created.
     *
     * @param addr The destination address.
     * @param hint A optional hint to be supplied to whichever transport ends-up
     *             making the real messenger. May be {@code null}, when no hint applies.
     * @return A messenger for the specified destination address or {@code null}
     *         if the destination address is not reachable.
     */
    public Messenger getMessenger(EndpointAddress addr, Object hint);

    /**
     * Creates and maps a canonical messenger to the specified destination.
     * <p/>
     * Behaves like {@link #getMessengerImmediate(EndpointAddress,Object)}
     * except that it returns a canonical messenger.
     * <p/>
     * The messenger is said to be canonical, because there is only one such
     * <em>live</em> object for any given destination address. The term "live",
     * here means that the messenger is not in any of the
     * {@link Messenger#TERMINAL} states as defined by {@link MessengerState}.
     * Therefore, for a given destination there may be any number of messengers
     * in a {@link Messenger#TERMINAL} state, but at most one in any other state.
     * As long as such an object exists, all calls to
     * {@code getCanonicalMessenger()} for the same address return this very
     * object.
     * <p/>
     * When first created, a canonical messenger is usually in the
     * {@link Messenger#UNRESOLVED} state. It becomes resolved by obtaining an
     * actual transport messenger to the destination upon the first attempt at
     * using it or when first forced to attempt resolution. Should resolution
     * fail at that point, it becomes {@link Messenger#UNRESOLVABLE}. Otherwise,
     * subsequent, failures are repaired automatically by obtaining a new
     * transport messenger when needed. If a failure cannot be repaired, the
     * messenger becomes {@link Messenger#BROKEN}.
     * <p/>
     * {@code getCanonicalMessenger()} is a recursive function.
     * Exploration of the parent endpoint is done automatically.
     * <p/>
     * <b>Note 1:</b> This method is the most fundamental messenger
     * instantiation method. It creates a different messenger for each variant
     * of destination address passed to the constructor. In general invokers
     * should use plain addresses; stripped of any service-specific destination.
     * <p/>
     * <b>Note 2:</b> The messengers that this method returns, are not
     * generally meant to be used directly. They provide a single queue for all
     * invokers, and do not perform group redirection and only support only a
     * subset of the {@code sendMessage()} methods.  One must get a properly
     * configured channel in order to send messages.
     * <p/>
     * If one of the other {@code getMessenger()} methods fits the
     * application needs, it should be preferred.
     *
     * @param addr The destination address. It is recommended, though not
     *             mandatory, that the address be stripped of its service name and service
     *             param elements.
     * @param hint An object, of a type specific to the protocol of the address,
     *             that may be provide additional information to the transport in
     *             establishing the connection. Typically but not necessarily, this is a
     *             route advertisement. If the transport cannot use the hint, or if it is
     *             {@code null}, it will be ignored.
     * @return A Canonical messenger that obtains transport messengers to the
     *         specified address, from LOCAL transports. Returns {@code null} if no
     *         local transport handles this type address.
     */
    public Messenger getCanonicalMessenger(EndpointAddress addr, Object hint);

    /**
     * Removes the specified listener.
     *
     * @param listener The listener that would have been called.
     * @param priority Priority set from which to remove this listener.
     * @return true if the listener was removed, otherwise false.
     */
    public boolean removeMessengerEventListener(MessengerEventListener listener, int priority);

    /**
     * Adds the specified listener for all messenger creation.
     *
     * @param listener The listener that will be called.
     * @param priority Order of precedence requested (from 0 to 2). 2 has the
     * highest precedence. Listeners are called in decreasing order of 
     * precedence. Listeners with equal precedence are called in an unspecified
     * order. There cannot be more than one listener object for a given 
     * precedence. Redundant calls have no effect.
     * @return true if the listener was added, otherwise false.
     */
    public boolean addMessengerEventListener(MessengerEventListener listener, int priority);

    /**
     * Propagates (broadcasts) a message via all available Message Transports.
     * Each Message Transport that implements propagation will send the message
     * using it's broadcast functionality to a configured broadcast address. Any
     * peers in the same network scope listening on that broadcast address will
     * receive the propagated message.
     * <p/>
     * The message will be sent using the default TTL value (which is 
     * unspecified).
     * 
     * @param message The message to be propagated. The message will not be
     * modified by this method.
     * @param serviceName  The name of the destination service.
     * @param serviceParam An optional parameter for the destination service or
     * {@code null}.
     * @throws IOException Thrown if the message could not be propagated.
     */
    public void propagate(Message message, String serviceName, String serviceParam) throws IOException;

    /**
     * Propagates (broadcasts) a message via all available Message Transports.
     * Each Message Transport that implements propagation will send the message
     * using it's broadcast functionality to a configured broadcast address. Any
     * peers in the same network scope listening on that broadcast address will
     * receive the propagated message.
     * 
     * @param message The message to be propagated. The message will not be
     * modified by this method.
     * @param serviceName  The name of the destination service.
     * @param serviceParam An optional parameter for the destination service or
     * {@code null}.
     * @param initialTTL The requested initial TTL for this message. The actual
     * TTL value used may be lower than this value but will never be higher.
     */
    public void propagate(Message message, String serviceName, String serviceParam, int initialTTL);

    /**
     * Verifies that the given address can be reached. The method, and accuracy
     * of the verification depends upon each Message Transport. In some cases
     * the address may be contacted to determine connectivity but this is not
     * guaranteed.
     *
     * @param addr is the Endpoint Address to ping.
     * @return {@code true} if the address can be reached otherwise {@code false}.
     * @deprecated The cost of performing this operation is generally the same
     * as getting a Messenger for the destination. Using {@code getMessenger()}
     * is a better approach because the resulting Messenger is generally needed
     * soon after ping.
     */
    @Deprecated
    public boolean ping(EndpointAddress addr);

    /**
     * Add a listener for the specified address.
     * <p/>
     * A single registered listener will be called for incoming messages
     * when (in order of preference) : <ol>
     * <li>The service name and service parameter match exactly to the
     * service name and service parameter specified in the destination
     * address of the message.</li>
     * <p/>
     * <li>The service name matches exactly the service name from the
     * message destination address and service parameter is {@code null}.
     * </li>
     * </ol>
     *
     * @param listener The listener which will be called when messages are
     * received for the registered destination.
     * @param serviceName The name of the service destination which will be
     * matched against incoming message destination endpoint addresses.
     * @param serviceParam An optional service parameter value which will be 
     * matched against destination endpoint addresses. May be null.
     * @return true if the listener was registered, otherwise false.
     */
    public boolean addIncomingMessageListener(EndpointListener listener, String serviceName, String serviceParam);

    /**
     * Get the listener for the specified address.
     *
     * @param serviceName  The service name to which the listener is registered.
     * @param serviceParam The service parameter to which the listener is
     *                     registered. May be {@code null}.
     * @return The currently registered listener or {@code null} if there is no
     *         listener for the specified name and parameter.
     */
    public EndpointListener getIncomingMessageListener(String serviceName, String serviceParam);

    /**
     * Remove the listener for the specified address.
     *
     * @param serviceName  The service name to which the listener is registered.
     * @param serviceParam The service parameter to which the listener is
     *                     registered. May be {@code null}.
     * @return The listener which was removed or {@code null} if there was
     *         no listener for the specified name and parameter.
     */
    public EndpointListener removeIncomingMessageListener(String serviceName, String serviceParam);

    /**
     * Registers a message filter listener. Each message will be tested against
     * the list of filters as part of its sending or receiving.
     * <p/>
     * The listener is invoked for a message when:
     * <ul>
     *   <li>The message contains a message element which matches exactly the
     *   values specified by namespace and name.</li>
     *  
     *   <li>The message contains a message element who's namespace value 
     *   matches exactly the specified namespace value and the specified name is
     *   {@code null}.</li>
     *  
     *   <li>The message contains a message element who's names value matches
     *   exactly the specified name value and the specified namespace is
     *   {@code null}.</li>
     * 
     *   <li>The specified name value and the specified namespace are both
     *   {@code null}.</li>
     * </ul>
     *
     * @param listener  The filter which will be called.
     * @param namespace Only messages containing elements of this namespace
     *                  which also match the 'name' parameter will be processed. {@code null}
     *                  may be use to specify all namespaces.
     * @param name      only messages containing elements of this name which also
     *                  match the 'namespace' parameter will be processed. {@code null} may be
     *                  use to specify all names.
     */
    public void addIncomingMessageFilterListener(MessageFilterListener listener, String namespace, String name);

    /**
     * Registers a message filter listener. Each message will be tested against
     * the list of filters as part of its sending or receiving.
     * <p/>
     * The listener is invoked for a message when:
     * <ul>
     *   <li>The message contains a message element which matches exactly the
     *   values specified by namespace and name.</li>
     *  
     *   <li>The message contains a message element who's namespace value 
     *   matches exactly the specified namespace value and the specified name is
     *   {@code null}.</li>
     *  
     *   <li>The message contains a message element who's names value matches
     *   exactly the specified name value and the specified namespace is
     *   {@code null}.</li>
     * 
     *   <li>The specified name value and the specified namespace are both
     *   {@code null}.</li>
     * </ul>
     *
     * @param listener  The filter which will be called.
     * @param namespace Only messages containing elements of this namespace
     *                  which also match the 'name' parameter will be processed. {@code null}
     *                  may be used to specify all namespaces.
     * @param name      only messages containing elements of this name which also
     *                  match the 'namespace' parameter will be processed. {@code null} may be
     *                  use to specify all names.
     */
    public void addOutgoingMessageFilterListener(MessageFilterListener listener, String namespace, String name);

    /**
     * Removes the given listener previously registered under the given element 
     * name
     *
     * @param listener  the listener to remove
     * @param namespace the name space
     * @param name      the name
     * @return the removed listener
     */
    public MessageFilterListener removeIncomingMessageFilterListener(MessageFilterListener listener, String namespace, String name);

    /**
     * Removes the given listener previously registered under the given element
     * name.
     *
     * @param listener  the listener to remove
     * @param namespace the name space
     * @param name      the name
     * @return the removed listener
     */
    public MessageFilterListener removeOutgoingMessageFilterListener(MessageFilterListener listener, String namespace, String name);

    /**
     * Delivers the provided message to the correct listener as specified by
     * the message's destination address.
     * <p/>
     * Two additional common message elements are optionally used by Message
     * Transports in conjunction with the Endpoint Service. Message Transports
     * may typically provide received messages to the Endpoint Service
     * containing these elements and the Endpoint service will dispatch the
     * messages based upon their content. Message Transports may use alternate
     * mechanisms for determining message source and destination addresses and
     * need not use these elements.
     * <p/>
     * The {@code jxta:EndpointSourceAddress} Message Element contains an 
     * Endpoint Address for the source of this message. The source address has a
     * variety of meanings based upon the usage of the underlying Message 
     * Transport. For low level transports such as TCP or HTTP the source 
     * address is the return address of the peer from which the message was 
     * received, ie. the hop address. For higher level Message Transports such 
     * as the Endpoint Router Transport or the TLS transport the source address 
     * is the virtual Endpoint Address of the peer which originated the message 
     * regardless of any intervening hops the message may have made.
     * <p/>
     * The {@code jxta:EndpointDestinationAddress} Message Element contains an 
     * Endpoint Address which will be used by the Endpoint Service to dispatch a
     * received message to the recipient specified by the service name and 
     * service parameter. The protocol address is also provided to the recipient 
     * service and can be used in some protocols for determining how the message 
     * was received. For example a service may wish to handle messages which
     * were sent directly differently than messages which were sent via 
     * propagation.
     *
     * @param msg The message to be delivered.
     * @deprecated Please convert your code to use the
     * {@link EndpointListener#processIncomingMessage(Message,EndpointAddress,EndpointAddress)}
     * method instead. The addressing method used by demux() was never part of
     * the formal JXTA protocol specification but was a defacto part because
     * demux() depended upon it.
     */
    @Deprecated
    public void demux(Message msg);

    /**
     * Adds the specified MessageTransport to this endpoint. A MessageTransport
     * may only be added if there are no other equivalent MessageTransports
     * available (as determined by {@link Object#equals(Object) equals()}).
     * <p/>
     * The MessageTransport becomes usable by the endpoint service to send
     * unicast messages and optionally propagation and ping messages if it is a
     * {@link net.jxta.endpoint.MessageSender}. The endpoint service becomes
     * usable by this MessageTransport to handle incoming messages if it is a
     * {@link MessageReceiver}.
     *
     * @param transport the MessageTransport to be installed.
     * @return A messenger event listener to invoke when incoming messengers are
     *         created or {@code null} if the MessageTransport was not installed.
     */
    public MessengerEventListener addMessageTransport(MessageTransport transport);

    /**
     * Removes the given MessageTransport protocol from this endpoint service.
     * <p/>
     * Transports remove themselves from the list when stopped. This method
     * is normally only called from the stoppApp method of the transport. To
     * cleanly remove a transport, call the transport's
     * {@link net.jxta.platform.Module#stopApp() stopApp()}and allow it to call
     * this method.
     *
     * @param transpt the MessageTransport to be removed.
     * @return {@code true} if the Message Transport was removed, otherwise
     * {@code false}.
     */
    public boolean removeMessageTransport(MessageTransport transpt);

    /**
     * Get an iterator of the MessageTransports available to this
     * EndpointService.
     *
     * @return the iterator of all message transports.
     */
    public Iterator<MessageTransport> getAllMessageTransports();

    /**
     * Get a Message Transport by protocol name.
     *
     * @param name The protocol name of the MessageTransport.
     * @return The Message Transport for the specified protocol name or
     * {@code null} if there is no matching Message Transport
     */
    public MessageTransport getMessageTransport(String name);

    /**
     * Returns a Messenger that may be used to send messages via  this endpoint
     * to the specified destination.
     *
     * @param addr the destination address.
     * @return The messenger or {@code null} is returned if the destination
     * address is not reachable.
     */
    public Messenger getMessenger(EndpointAddress addr);

    /**
     * Asynchronously acquire a messenger for the specified address. The
     * listener will be called when the messenger has been constructed.
     *
     * @param listener the listener to call when the messenger is ready.
     * @param addr     the destination for the messenger.
     * @param hint     the messenger hint, if any, otherwise null.
     * @return {@code true} if the messenger is queued for construction
     * otherwise {@code false}.
     * @deprecated This method is being phased out. Prefer one of the other
     * non-blocking variants. If a listener style paradigm is required, use
     * {@link ListenerAdaptor} which emulates this functionality.
     */
    @Deprecated
    public boolean getMessenger(MessengerEventListener listener, EndpointAddress addr, Object hint);

    /**
     * Returns a Direct Messenger that may be used to send messages via  this endpoint to the specified destination.
     * </p>
     * Direct messengers are non self destructive, they must be explicilty closed.
     *
     * @param addr the destination address.
     * @param hint the messenger hint, if any, otherwise null.
     * @param exclusive if true avoids caching the messenger
     * @return The messenger or {@code null} is returned if the destination address is not reachable.
     * @throws IllegalArgumentException if hint is not of RouteAdvertisement, or PeerAdvertisement type.
     */
    public Messenger getDirectMessenger(EndpointAddress addr, Object hint, boolean exclusive);
}
