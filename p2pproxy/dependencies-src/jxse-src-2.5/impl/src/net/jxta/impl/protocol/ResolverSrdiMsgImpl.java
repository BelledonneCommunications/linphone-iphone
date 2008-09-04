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

import net.jxta.credential.Credential;
import net.jxta.document.*;
import net.jxta.membership.MembershipService;
import net.jxta.protocol.ResolverSrdiMsg;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 * ResolverQuery provides the binding for the message to query other nodes
 */
public class ResolverSrdiMsgImpl extends ResolverSrdiMsg {

    private final static Logger LOG = Logger.getLogger(ResolverSrdiMsgImpl.class.getName());

    /**
     * Description of the Field
     */
    public final static String handlernameTag = "HandlerName";

    /**
     * Description of the Field
     */
    public final static String credentialTag = "jxta:Cred";

    /**
     * Description of the Field
     */
    public final static String payloadTag = "Payload";

    /**
     * Constructor for the ResolverSrdiMsgImpl object
     */
    public ResolverSrdiMsgImpl() {}

    /**
     * Construct from a StructuredDocument
     *
     * @param root       the underlying document
     * @param membership membership service used to verify credentials
     */
    public ResolverSrdiMsgImpl(Element root, MembershipService membership) {
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;
        String doctype = doc.getName();

        if (!getMessageType().equals(doctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doctype);
        }
        readIt(doc, membership);
    }

    /**
     * Creates this object with a specific handler name, credential and payload
     *
     * @param handlerName the
     * @param cred        the credntial
     * @param payload     the payload
     */
    public ResolverSrdiMsgImpl(String handlerName, Credential cred, String payload) {

        setHandlerName(handlerName);
        setPayload(payload);
        setCredential(cred);
    }

    /**
     * return the string representaion of this doc
     *
     * @return string representation of the message
     */
    @Override
    public String toString() {

        StructuredTextDocument doc = (StructuredTextDocument) getDocument(MimeMediaType.XMLUTF8);

        return doc.toString();
    }

    /**
     * return a Document represetation of this object
     *
     * @param asMimeType type of message
     * @return document
     */
    @Override
    public Document getDocument(MimeMediaType asMimeType) {
        StructuredTextDocument adv = (StructuredTextDocument) StructuredDocumentFactory.newStructuredDocument(asMimeType
                ,
                getMessageType());

        if (adv instanceof XMLElement) {
            ((XMLElement) adv).addAttribute("xmlns:jxta", "http://jxta.org");
        }

        Element e;

        e = adv.createElement(handlernameTag, getHandlerName());
        adv.appendChild(e);
        if (getCredential() != null) {
            try {
                StructuredDocumentUtils.copyElements(adv, adv, (getCredential()).getDocument(asMimeType));
            } catch (Exception ce) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Got an Exception during credential creation ", ce);
                }
            }
        }
        e = adv.createElement(payloadTag, getPayload());
        adv.appendChild(e);
        return adv;
    }

    /**
     * Parses an XML document into this object
     *
     * @param doc        the underlying document
     * @param membership used to parse credentails if any
     */
    private void readIt(XMLElement doc, MembershipService membership) {
        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (elem.getName().equals(handlernameTag)) {
                setHandlerName(elem.getTextValue());
                continue;
            }
            // Set credential
            if (elem.getName().equals(credentialTag)) {
                Credential credential;

                if (elem.getTextValue() != null) {
                    try {
                        credential = membership.makeCredential(elem);
                        setCredential(credential);
                    } catch (Exception ce) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Credential creation failed", ce);
                        }
                    }
                }
                continue;
            }

            // Set payload
            if (elem.getName().equals(payloadTag)) {
                setPayload(elem.getTextValue());
            }
        }
    }
}
