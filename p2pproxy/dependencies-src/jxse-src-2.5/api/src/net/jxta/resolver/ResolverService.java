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

package net.jxta.resolver;


import net.jxta.service.Service;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.ResolverSrdiMsg;


/**
 * Provides a generic mechanism for JXTA Services to send "Queries", and receive 
 * "Responses". It removes the burden for registered handlers in deal with :
 *
 *<ul type-disc>
 *    <li>Setting message tags, to ensure uniqueness of tags and
 *     ensures that messages are sent to the correct address, and group.</p>
 *    <li>Authentication, and verification of credentials.</p>
 *    <li>Query routing.</p>
 *    <li>drop rogue messages.</p>
 *</ul>
 *
 * <p/>The ResolverService does not proccess the queries, nor does it not
 * compose reponses. Processing of queries, and composition of responses is left
 * up to the registered handlers. Services that wish to handle queries and
 * generate reponses must implement {@link net.jxta.resolver.QueryHandler}.
 *
 * @see net.jxta.service.Service
 * @see net.jxta.resolver.QueryHandler
 * @see net.jxta.protocol.ResolverQueryMsg
 * @see net.jxta.protocol.ResolverResponseMsg
 */
public interface ResolverService extends Service, GenericResolver {
    
    /**
     *  Returned by query handlers to indicate that the query should be
     *  forwarded to the rest of the network.
     */
    public final static int Repropagate = -1;
    
    /**
     *  Returned by query handlers to indicate that the query has been resolved
     *  and a response has been sent.
     */
    public final static int OK = 0;
    
    /**
     *  Registers a given QueryHandler, returns the previous handler registered
     *  under this name.
     *
     *  @param name The name under which this handler is to be registered.
     *  @param handler The handler.
     *  @return The previous handler registered under this name.
     */
    public QueryHandler registerHandler(String name, QueryHandler handler);
    
    /**
     *  Unregisters a given QueryHandler, returns the previous handler
     *  registered under this name.
     *
     *  @param name The name of the handler to unregister.
     *  @return The previous handler registered under this name.
     */
    public QueryHandler unregisterHandler(String name);
    
    /**
     *  Registers a given SrdiHandler, returns the previous handler registered
     *  under this name.
     *
     *  @param name The name under which this handler is to be registered.
     *  @param handler The handler.
     *  @return The previous handler registered under this name.
     *
     */
    public SrdiHandler registerSrdiHandler(String name, SrdiHandler handler);
    
    /**
     *  Unregisters a given SrdiHandler, returns the previous handler registered
     *  under this name.
     *
     *  @param name The name of the handler to unregister.
     *  @return The previous handler registered under this name
     *
     */
    public SrdiHandler unregisterSrdiHandler(String name);
    
    /**
     *  Sends a resolver query. If <tt>destPeer</tt> is <tt>null</tt> the 
     *  message is propagated.
     *
     *  @param destPeer The destination peer of the query or <tt>null</tt> if
     *  the query is to be propagated.
     *  @param query The query to match.
     */
    public void sendQuery(String destPeer, ResolverQueryMsg query);
    
    /**
     *  Send a resolver response. If <tt>destPeer</tt> is <tt>null</tt> then the
     *  response is propagated. Propagated responses are generally announcements
     *  and not responses to active queries.
     *
     *  @param destPeer The destination peer of the response or <tt>null</tt> if
     *  the response is to be propagated. 
     *  @param response The response to be sent.
     */
    public void sendResponse(String destPeer, ResolverResponseMsg response);
    
    /**
     *  Send an SRDI message.
     *
     *  <p/>If <tt>destPeer</tt> is <tt>null</tt> the message is walked.
     *
     *  @param destPeer is the destination of the SRDI message.
     *  @param srdi is the SRDI message to be sent.
     */
    public void sendSrdi(String destPeer, ResolverSrdiMsg srdi);
}
