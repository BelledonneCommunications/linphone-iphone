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


/**
 * A Channel Messenger provides an exclusive interface to the shared messenger.
 * <p/>
 * What is typically exclusive is the message queue, addressing parameters
 * that are not usefully shared (serviceName, serviceParam), and if needed
 * cross-group address rewriting parameters.
 * <p/>
 * This class is provided as a base for implementing such channel messengers,
 * which are typically what Messenger.getChannelMessenger() needs to return.
 *
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.endpoint.EndpointAddress
 * @see net.jxta.endpoint.Message
 */
public abstract class ChannelMessenger extends AbstractMessenger implements Messenger {

    /**
     * insertedServicePrefix This is how all valid inserted services start. This
     * lets us recognize if a message already has an inserted service. Then we
     * must not add another one. Only the top-most one counts.  Since insertion
     * is only done here, the constant is defined here. Even if historically it
     * was done within the endpoint implementation, it has become a protocol now.
     */
    public static final String InsertedServicePrefix = "EndpointService:";

    private String insertedService;

    /**
     * The worker that implements sendMessage-with-listener for this channel. If
     * there's none, sendMessage-with-listener will throw an exception. Channels
     * returned by getMessenger methods all have one already. It is up to the
     * invoker of getChannelMessenger to supply one or not.
     */
    private ListenerAdaptor messageWatcher;

    protected String origService;

    protected String origServiceParam;

    /**
     * Figure out what the service string will be after mangling (if required)
     * and applying relevant defaults.
     *
     * @param service The service name in the unmangled address.
     * @return String The service name in the mangled address.
     */
    protected String effectiveService(String service) {

        // No redirection required. Just apply the default service.
        if (insertedService == null) {
            return (service == null) ? origService : service;
        }

        // Check if redirection is applicable.
        return ((service != null) && service.startsWith(InsertedServicePrefix)) ? service : insertedService;
    }

    /**
     * Figure out what the param string will be after mangling (if required) and
     * applying relevant defaults.
     *
     * @param service      The service name in the unmangled address.
     * @param serviceParam The service parameter in the unmangled address.
     * @return String The service parameter in the mangled address.
     */
    protected String effectiveParam(String service, String serviceParam) {

        // No redirection required. Or not applicable. Just apply the default param.
        if ((insertedService == null) || ((service != null) && service.startsWith(InsertedServicePrefix))) {
            return (serviceParam == null) ? origServiceParam : serviceParam;
        }

        // Apply redirection. We need the effective service, now.
        if (service == null) {
            service = origService;
        }

        if (serviceParam == null) {
            serviceParam = origServiceParam;
        }

        return ((null != service) && (null != serviceParam)) ? (service + "/" + serviceParam) : service;
    }

    /**
     * Give this channel the watcher that it must use whenever sendMessage(...,listener) is used. If not set,
     * sendMessage(..., listener) will throw.
     *
     * @param messageWatcher the listener
     */
    public void setMessageWatcher(ListenerAdaptor messageWatcher) {
        this.messageWatcher = messageWatcher;
    }

    /**
     * Create a new ChannelMessenger
     *
     * @param baseAddress      The network address messages go to; regardless of service, param, or group.
     * @param groupRedirection Group to which the messages must be redirected. This is used to implement the automatic group
     *                         segregation which has become a de-facto standard. If not null, the unique portion of the specified groupID is
     *                         prepended with {@link #InsertedServicePrefix} and inserted in every message's destination address in place of the
     *                         the original service name, which gets shifted into the beginning of the service parameter. The opposite is done
     *                         on arrival to restore the original destination address before the message is delivered to the listener in the
     *                         the specified group. Messages that already bear a group redirection are not affected.
     * @param origService      The default destination service for messages sent without specifying a different service.
     * @param origServiceParam The default destination service parameter for messages sent without specifying a different service
     *                         parameter.
     */
    public ChannelMessenger(EndpointAddress baseAddress, PeerGroupID groupRedirection, String origService, String origServiceParam) {

        // FIXME: The inserted service business is really messy. Group seggregation does not have to be the endpoint service's
        // business. It should be specified by the app as part of the destination address. What we're doing here
        // is simply enforcing what could just be a convention.

        super(baseAddress);
        if (groupRedirection == null) {
            insertedService = null;
        } else {
            insertedService = InsertedServicePrefix + groupRedirection.getUniqueValue().toString();
        }
        this.origService = origService;
        this.origServiceParam = origServiceParam;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * By default a channel refuses to make a channel.
     */
    public Messenger getChannelMessenger(PeerGroupID redirection, String service, String serviceParam) {
        return null;
    }

    /**
     * {@inheritDoc}
     *
     */
    @Override
    public void sendMessage(Message msg, String service, String serviceParam, OutgoingMessageEventListener listener) {
        if (messageWatcher == null) {
            throw new UnsupportedOperationException("This channel was not configured to emulate this legacy method.");
        }

        // Cleanup the message from any existing result prop since we're going to use select.
        msg.setMessageProperty(Messenger.class, null);

        // Tell the watcher to select that message.
        messageWatcher.watchMessage(listener, msg);

        sendMessageN(msg, service, serviceParam);
    }
}
