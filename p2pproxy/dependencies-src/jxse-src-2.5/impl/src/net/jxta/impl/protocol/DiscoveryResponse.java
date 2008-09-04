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


import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.UndeclaredThrowableException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Vector;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.TextElement;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.protocol.DiscoveryResponseMsg;
import net.jxta.protocol.PeerAdvertisement;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 *  DiscoveryResponse.
 *
 *  <p/>This message is part of the standard JXTA Peer Discovery Protocol (PDP).
 *
 *  <pre>
 * &lt;xs:element name="DiscoveryResponse" type="jxta:DiscoveryResponse"/>
 *
 * &lt;xs:complexType name="DiscoveryResponse">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="Type" type="jxta:DiscoveryQueryType"/>
 *     &lt;xs:element name="Count" type="xs:unsignedInt" minOccurs="0"/>
 *     &lt;xs:element name="Attr" type="xs:string" minOccurs="0"/>
 *     &lt;xs:element name="Value" type="xs:string" minOccurs="0"/>
 *     &lt;!-- The following should refer to a peer adv, but is instead a whole doc for historical reasons -->
 *     &lt;xs:element name="PeerAdv" minOccurs="0">
 *     &lt;xs:complexType>
 *       &lt;xs:simpleContent>
 *         &lt;xs:extension base="xs:string">
 *           &lt;xs:attribute name="Expiration" type="xs:unsignedLong"/>
 *         &lt;/xs:extension>
 *       &lt;/xs:simpleContent>
 *     &lt;/xs:complexType>
 *     &lt;/xs:element>
 *     &lt;xs:element name="Response" maxOccurs="unbounded">
 *     &lt;xs:complexType>
 *       &lt;xs:simpleContent>
 *         &lt;xs:extension base="xs:string">
 *           &lt;xs:attribute name="Expiration" type="xs:unsignedLong"/>
 *         &lt;/xs:extension>
 *       &lt;/xs:simpleContent>
 *     &lt;/xs:complexType>
 *     &lt;/xs:element>
 *   &lt;/xs:sequence>
 * &lt;/xs:complexType>
 * </pre>
 *
 *@see    net.jxta.discovery.DiscoveryService
 *@see    net.jxta.impl.discovery.DiscoveryServiceImpl
 *@see    <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pdp" 
 *        target="_blank">JXTA Protocols Specification : Peer Discovery Protocol</a>
 */
public class DiscoveryResponse extends DiscoveryResponseMsg {

    private final static transient Logger LOG = Logger.getLogger(DiscoveryResponse.class.getName());

    private final static String countTag = "Count";
    private final static String expirationTag = "Expiration";
    private final static String peerAdvTag = "PeerAdv";
    private final static String queryAttrTag = "Attr";
    private final static String queryValueTag = "Value";
    private final static String responsesTag = "Response";
    private final static String typeTag = "Type";

    /**
     *  Constructor for new instances.
     */
    public DiscoveryResponse() {}

    /**
     *  Construct from a StructuredDocument
     *
     *@param  root  Description of the Parameter
     */
    public DiscoveryResponse(Element root) {

        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
        }
        XMLElement doc = (XMLElement) root;
        String docName = doc.getName();

        if (!getAdvertisementType().equals(docName)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + " from doc containing a " + docName);
        }
        readIt(doc);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType asMimeType) {

        StructuredTextDocument adv = (StructuredTextDocument)
                StructuredDocumentFactory.newStructuredDocument(asMimeType, getAdvertisementType());

        if (adv instanceof XMLDocument) {
            ((XMLDocument) adv).addAttribute("xmlns:jxta", "http://jxta.org");
        }

        Element e;

        e = adv.createElement(countTag, Integer.toString(responses.size()));
        adv.appendChild(e);
        e = adv.createElement(typeTag, Integer.toString(type));
        adv.appendChild(e);

        PeerAdvertisement myPeerAdv = getPeerAdvertisement();

        if (null != myPeerAdv) {
            e = adv.createElement(peerAdvTag, myPeerAdv.toString());
            adv.appendChild(e);
        }

        if ((attr != null) && (attr.length() > 0)) {
            e = adv.createElement(queryAttrTag, getQueryAttr());
            adv.appendChild(e);
            if ((value != null) && (value.length() > 0)) {
                e = adv.createElement(queryValueTag, value);
                adv.appendChild(e);
            }
        }

        Enumeration<String> advs = getResponses();
        Enumeration exps = getExpirations();

        try {
            while (advs.hasMoreElements()) {
                Long l = (Long) exps.nextElement();
                String response = advs.nextElement();

                e = adv.createElement(responsesTag, response);

                adv.appendChild(e);
                if (adv instanceof Attributable) {
                    ((Attributable) e).addAttribute(expirationTag, l.toString());
                }
            }
        } catch (Exception failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Got an Exception during doc creation", failed);
            }
            IllegalStateException failure = new IllegalStateException("Got an Exception during doc creation");

            failure.initCause(failed);
            throw failure;
        }
        return adv;
    }

    /**
     *  Parses a document into this object
     *
     *@param  doc  Document
     */
    private void readIt(XMLElement doc) {
        Vector<String> res = new Vector<String>();
        Vector<Long> exps = new Vector<Long>();

        try {
            Enumeration elements = doc.getChildren();

            while (elements.hasMoreElements()) {
                XMLElement elem = (XMLElement) elements.nextElement();

                if (elem.getName().equals(typeTag)) {
                    type = Integer.parseInt(elem.getTextValue());
                    continue;
                }

                if (elem.getName().equals(peerAdvTag)) {
                    String peerString = elem.getTextValue();

                    if (null == peerString) {
                        continue;
                    }

                    peerString = peerString.trim();
                    if (peerString.length() > 0) {
                        XMLDocument xmlPeerAdv = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(
                                MimeMediaType.XMLUTF8, new StringReader(peerString));

                        setPeerAdvertisement((PeerAdvertisement) AdvertisementFactory.newAdvertisement(xmlPeerAdv));
                    }
                    continue;
                }

                if (elem.getName().equals(queryAttrTag)) {
                    setQueryAttr(elem.getTextValue());
                    continue;
                }

                if (elem.getName().equals(queryValueTag)) {
                    setQueryValue(elem.getTextValue());
                    continue;
                }

                if (elem.getName().equals(responsesTag)) {
                    // get the response
                    String aResponse = elem.getTextValue();

                    if (null == aResponse) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Discarding an empty response tag");
                        }
                        continue;
                    }
                    res.add(aResponse);

                    long exp;
                    // get expiration associated with this response
                    Attribute attr = (elem).getAttribute(expirationTag);

                    if (null != attr) {
                        exp = Long.parseLong(attr.getValue());
                    } else {
                        // if there are no attribute use DEFAULT_EXPIRATION
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(
                                    "Received an old-style DiscoveryResponse.\n You received a response from a peer that does \nnot support advertisement aging. \nSetting expiration to DiscoveryService.DEFAULT_EXPIRATION ");
                        }
                        exp = DiscoveryService.DEFAULT_EXPIRATION;
                    }

                    exps.add(exp);
                }
            }
        } catch (Exception failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Got an Exception during Parse ", failed);
            }
            IllegalArgumentException failure = new IllegalArgumentException("Got an Exception during parse");

            failure.initCause(failed);
            throw failure;
        }
        
        setResponses(res);
        setExpirations(exps);
    }

    /**
     * Return a string representation of this message. The string will
     * contain the message formated as a UTF-8 encoded XML Document.
     *
     * @return String  a String containing the message.
     */
    @Override
    public String toString() {
        
        try {
            XMLDocument doc = (XMLDocument) getDocument(MimeMediaType.XMLUTF8);
            
            return doc.toString();
        } catch (Throwable e) {
            if (e instanceof Error) {
                throw (Error) e;
            } else if (e instanceof RuntimeException) {
                throw (RuntimeException) e;
            } else {
                throw new UndeclaredThrowableException(e);
            }
        }
    }
}
