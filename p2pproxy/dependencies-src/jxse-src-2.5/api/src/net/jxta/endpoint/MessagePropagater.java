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


/**
 * A MessagePropagater is a MessageTransport that can send propagated
 * (multicast) messages.
 *
 * @see net.jxta.endpoint.MessageTransport
 * @see net.jxta.endpoint.Message
 * @see net.jxta.endpoint.Messenger
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.endpoint.MessageSender
 * @see net.jxta.endpoint.MessageReceiver
 */
public interface MessagePropagater extends MessageTransport {

    /**
     * Returns the {@link EndpointAddress} which will be used as the source
     * address for all messages sent by this message sender. This is the
     * "preferred" address to which replies should be sent. This address is not
     * necessarily the best or only address by which the peer may be reached.
     * <p/>
     * The public address may also be for a different message transport.
     *
     * @return an EndpointAddress containing the public address for this
     *         message receiver.
     */
    public EndpointAddress getPublicAddress();

    /**
     * Propagates a Message via this Message Transport.
     *
     * @param msg           The Message to be propagated.
     * @param serviceName   Contains the name of the destination service, if any.
     *                      This will be integrated into the destination address.
     * @param serviceParams Contains the parameters associated with the service,
     *                      if any. This will be integrated into the destination address.
     * @param initialTTL    The requested initial TTL for this message. This value
     *                      may be reduced by the implementation.
     * @return {@code true} if the Message was propagated otherwise
     *         {@code false}.
     */
    public boolean propagate(Message msg, String serviceName, String serviceParams, int initialTTL);
}
