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
package net.jxta.protocol;


import net.jxta.document.Document;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;


/**
 * Generic Resolver Service message "Response".
 *
 * @see net.jxta.resolver.ResolverService
 *@see    net.jxta.protocol.ResolverQueryMsg
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-prp" target="_blank">JXTA Protocols Specification : Peer Resolver Protocol</a>
 **/
public abstract class ResolverResponseMsg {
    private StructuredDocument credential = null;

    private String handlername = null;
    
    /**
     *  Description of the Field
     */
    public int queryid = 0;
    
    private String response = null;

    /**
     *  returns the credential
     *
     *@return    StructuredDocument credential
     */
    public StructuredDocument getCredential() {
        return credential;
    }

    /**
     *  Write advertisement into a document. asMimeType is a mime media-type
     *  specification and provides the form of the document which is being
     *  requested. Two standard document forms are defined. "text/text" encodes
     *  the document in a form nice for printing out and "text/xml" which
     *  provides an XML format.
     *
     *@param  asMimeType  mime-type representation requested for that document
     *@return             Document document representing the advertisement
     */
    public abstract Document getDocument(MimeMediaType asMimeType);

    /**
     * Set optional route information as part of the response.  This
     * information is just attached to the response and is ONLY used by the
     * resolver service. This information will not be sent as part of the
     * response msg and is not part of the resolver response wire format
     * protocol.
     *
     * @param route RouteAdvertisement to send the response
     */
    public abstract void setSrcPeerRoute(RouteAdvertisement route);

    /**
     * Get optional route information that may be attached to the
     * response. This information is just attached to the response and 
     * is only used internally by the resolver service. This information will
     * not be sent as part of the response msg and is not part of the
     * resolver response wire format.
     *
     * @return RouteAdvertisement to send the response
     */
    public abstract RouteAdvertisement  getSrcPeerRoute();

    /**
     *  returns the handlername
     *
     *@return    String handlername name
     */
    public String getHandlerName() {
        return handlername;
    }

    /**
     *  returns queryid value
     *
     *@return    int queryid value
     */
    public int getQueryId() {
        return queryid;
    }

    /**
     *  returns the response body for this message.
     *
     *@return    the response body represented as a string.
     */
    public String getResponse() {
        return response;
    }

    /**
     *  set the credential for this response.
     *
     *@param  cred  string credential
     */

    public void setCredential(StructuredDocument cred) {
        this.credential = cred;
    }

    /**
     *  set the handlername
     *
     *@param  name  string handlername
     */
    public void setHandlerName(String name) {
        this.handlername = name;
    }

    /**
     *  set the query id to which this message is a response.
     *
     *@param  id  the query id for this response.
     */
    public void setQueryId(int id) {
        this.queryid = id;
    }

    /**
     *  Set the response body for this message.
     *
     *@param  response  response body as a string.
     */
    public void setResponse(String response) {
        this.response = response;
    }

    /**
     *  All messages have a type (in xml this is !doctype) which identifies the
     *  message
     *
     *@return    String type of the advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:ResolverResponse";
    }
}
