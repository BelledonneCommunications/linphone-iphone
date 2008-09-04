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

package net.jxta.impl.protocol;


import java.util.Enumeration;

import net.jxta.document.*;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.RouteAdvertisement;


/**
 * ResolverResponse provides an implementation for 
 * {@link net.jxta.protocol.ResolverResponseMsg} using the standard JXTA
 * Peer Resolver Protocol.
 *
 * <p/>The message is implemented with the following schema:
 *
 *<pre><tt>
 * &lt;xs:complexType name="ResolverResponse">
 *   &lt;xs:all>
 *     &lt;xs:element ref="jxta:Cred" minOccurs="0"/>
 *     &lt;xs:element name="HandlerName" type="xs:string"/>
 *     &lt;xs:element name="QueryID" type="xs:string"/>
 *     &lt;xs:element name="Response" type="xs:anyType"/>
 *   &lt;/xs:all>
 * &lt;/xs:complexType>
 *</tt></pre>
 *
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-prp" target="_blank">JXTA Protocols Specification : Peer Resolver Protocol</a>
 */
public class ResolverResponse extends ResolverResponseMsg {

    private static final String handlernameTag = "HandlerName";
    private static final String  credentialTag = "jxta:Cred";
    private static final String     queryIdTag = "QueryID";
    private static final String   responseTag = "Response";

    /**
     * optional route information to send the response
     */
    private RouteAdvertisement srcRoute = null;

    /**
     *
     *  Standard Constructor for new instances.
     */
    public ResolverResponse() {}

    /**
     * Construct a doc from strings
     *
     *  @deprecated use the individual accessor methods instead.
     * @param HandlerName the handler name
     * @param Credential the credential doc
     * @param QueryId    query ID
     * @param Response   the response
     *
     */
    @Deprecated
    public ResolverResponse(String HandlerName, StructuredDocument Credential, int QueryId, String Response) {
        setHandlerName(HandlerName);
        setCredential(Credential);
        setQueryId(QueryId);
        setResponse(Response);
    }

    /**
     * Construct from a StructuredDocument
     *
     * @param root the element
     */
    public ResolverResponse(Element root) {

        this();
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;
        String doctype = doc.getName();

        if (!getAdvertisementType().equals(doctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doctype);
        }

        readIt(doc);

        // sanity check
        if (null == getHandlerName()) {
            throw new IllegalArgumentException("Response message does not contain a handler name");
        }

        if (null == getResponse()) {
            throw new IllegalArgumentException("Response message does not contain a response");
        }
    }

    public void readIt(TextElement doc) {
        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            TextElement elem = (TextElement) elements.nextElement();

            if (elem.getName().equals(handlernameTag)) {
                setHandlerName(elem.getTextValue());
                continue;
            }
            // Set credential
            if (elem.getName().equals(credentialTag)) {
                setCredential(StructuredDocumentUtils.copyAsDocument(elem));
                continue;
            }
            // Set queryid
            if (elem.getName().equals(queryIdTag)) {
                queryid = Integer.parseInt(elem.getTextValue());
                continue;
            }
            // Set response
            if (elem.getName().equals(responseTag)) {
                setResponse(elem.getTextValue());
            }
        }
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType asMimeType) {

        StructuredTextDocument adv = (StructuredTextDocument)
                StructuredDocumentFactory.newStructuredDocument(asMimeType, getAdvertisementType());

        if (adv instanceof Attributable) {
            ((Attributable) adv).addAttribute("xmlns:jxta", "http://jxta.org");
        }

        Element e;

        e = adv.createElement(handlernameTag, getHandlerName());
        adv.appendChild(e);
        if (getCredential() != null) {
            StructuredDocumentUtils.copyElements(adv, adv, getCredential());
        }
        e = adv.createElement(queryIdTag, Integer.toString(queryid));
        adv.appendChild(e);
        e = adv.createElement(responseTag, getResponse());
        adv.appendChild(e);
        return adv;
    }

    /**
     *  {@inheritDoc}
     *
     *  <p/>Result is the response as an XML string.
     */
    @Override
    public String toString() {
        return getDocument(MimeMediaType.XMLUTF8).toString();
    }

    /**
     * Set optional route information as part of the response.
     * This information is just attached to the response and
     * will not be sent as part of the response
     *
     * @param route RouteAdvertisement to send the response
     */

    @Override
    public void setSrcPeerRoute(RouteAdvertisement route) {
        srcRoute = route;
    }

    /**
     * Get optional route information that may be attached to the
     * response. This information is just attached to the response and will
     * not be sent as part of the response
     *
     * @return RouteAdvertisement to send the response
     */

    @Override
    public RouteAdvertisement  getSrcPeerRoute() {
        if (srcRoute != null) {
            return srcRoute.clone();
        } else {
            return null;
        }
    }
}
