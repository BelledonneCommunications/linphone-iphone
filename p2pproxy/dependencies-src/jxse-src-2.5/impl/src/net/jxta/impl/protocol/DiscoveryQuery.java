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
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.*;
import net.jxta.protocol.DiscoveryQueryMsg;
import net.jxta.protocol.PeerAdvertisement;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.StringReader;
import java.lang.reflect.UndeclaredThrowableException;
import java.util.Enumeration;


/**
 * Implements the Discovery Query Message according to the schema defined by the
 * standard JXTA Peer Discovery Protocol (PDP).
 * <p/>
 * <p/><pre>
 * &lt;xs:element name="DiscoveryQuery" type="jxta:DiscoveryQuery"/>
 * <p/>
 * &lt;xsd:simpleType name="DiscoveryQueryType">
 *   &lt;xsd:restriction base="xsd:string">
 *     &lt;!-- peer -->
 *     &lt;xsd:enumeration value="0"/>
 *     &lt;!-- group -->
 *     &lt;xsd:enumeration value="1"/>
 *     &lt;!-- adv -->
 *     &lt;xsd:enumeration value="2"/>
 *   &lt;/xsd:restriction>
 * &lt;/xsd:simpleType>
 * <p/>
 * &lt;xs:complexType name="DiscoveryQuery">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="Type" type="jxta:DiscoveryQueryType"/>
 *     &lt;xs:element name="Threshold" type="xs:unsignedInt" minOccurs="0"/>
 *     &lt;xs:element name="Attr" type="xs:string" minOccurs="0"/>
 *     &lt;xs:element name="Value" type="xs:string" minOccurs="0"/>
 *     &lt;!-- The following should refer to a peer adv, but is instead a whole doc for historical reasons -->
 *     &lt;xs:element name="PeerAdv" type="xs:string" minOccurs="0"/>
 *   &lt;/xs:sequence>
 * &lt;/xs:complexType>
 * </pre>
 *
 * @see net.jxta.discovery.DiscoveryService
 * @see net.jxta.impl.discovery.DiscoveryServiceImpl
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pdp" target="_blank">JXTA Protocols Specification : Peer Discovery Protocol</a>
 */
public class DiscoveryQuery extends DiscoveryQueryMsg {

    private static final Logger LOG = Logger.getLogger(DiscoveryQuery.class.getName());

    private static final String typeTag = "Type";
    private static final String peerAdvTag = "PeerAdv";
    private static final String thresholdTag = "Threshold";
    private static final String queryAttrTag = "Attr";
    private static final String queryValueTag = "Value";

    /**
     * Default constructor
     */
    public DiscoveryQuery() {}

    /**
     * Construct from a StructuredDocument
     *
     * @param doc the element
     */
    public DiscoveryQuery(Element doc) {
        initialize(doc);
    }

    /**
     * Process an individual element from the document during parse. Normally,
     * implementations will allow the base advertisments a chance to handle the
     * element before attempting ot handle the element themselves. ie.
     * <p/>
     * <p/><pre><code>
     *  protected boolean handleElement(Element elem) {
     * <p/>
     *      if (super.handleElement()) {
     *           // it's been handled.
     *           return true;
     *           }
     * <p/>
     *      <i>... handle elements here ...</i>
     * <p/>
     *      // we don't know how to handle the element
     *      return false;
     *      }
     *  </code></pre>
     *
     * @param elem the element to be processed.
     * @return true if the element was recognized, otherwise false.
     */
    protected boolean handleElement(XMLElement elem) {
        
        String value = elem.getTextValue();
        
        if(null == value) {
            return false;
        }
        
        value = value.trim();
        
        if(0 == value.length()) {
            return false;
        }

        if (elem.getName().equals(typeTag)) {
            setDiscoveryType(Integer.parseInt(value));
            return true;
        }
        if (elem.getName().equals(thresholdTag)) {
            setThreshold(Integer.parseInt(value));
            return true;
        }
        if (elem.getName().equals(peerAdvTag)) {
            try {
                XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, new StringReader(value));
                PeerAdvertisement adv = (PeerAdvertisement) AdvertisementFactory.newAdvertisement(asDoc);
                setPeerAdvertisement(adv);
                return true;
            } catch(IOException failed) {
                IllegalArgumentException failure = new IllegalArgumentException("Bad Peer Advertisement");
                failure.initCause(failed);
                
                throw failure;
            }
        }
        if (elem.getName().equals(queryAttrTag)) {
            setAttr(value);
            return true;
        }
        if (elem.getName().equals(queryValueTag)) {
            setValue(value);
            return true;
        }

        // element was not handled
        return false;
    }

    /**
     * Intialize a Discovery Query from a portion of a structured document.
     *
     * @param root document to intialize from
     */
    protected void initialize(Element root) {

        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
        }

        XMLElement doc = (XMLElement) root;

        if (!doc.getName().equals(getAdvertisementType())) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        setDiscoveryType(-1); // force illegal value;

        Enumeration<XMLElement> elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element : " + elem.toString());
                }
            }
        }

        // sanity check time!

        if ((DiscoveryService.PEER != getDiscoveryType()) && (DiscoveryService.GROUP != getDiscoveryType())
                && (DiscoveryService.ADV != getDiscoveryType())) {
            throw new IllegalArgumentException("Type is not one of the required values.");
        }
        if (getThreshold() < 0) {
            throw new IllegalArgumentException("Threshold must not be less than zero.");
        }
        if ((getDiscoveryType() != DiscoveryService.PEER) && (getThreshold() == 0)) {
            throw new IllegalArgumentException("Threshold may not be zero.");
        }
        if ((null == getAttr()) && (null != getValue())) {
            throw new IllegalArgumentException("Value specified without attribute.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType asMimeType) {
        StructuredDocument adv = StructuredDocumentFactory.newStructuredDocument(asMimeType, getAdvertisementType());

        if (adv instanceof XMLDocument) {
            XMLDocument xmlDoc = (XMLDocument) adv;

            xmlDoc.addAttribute("xmlns:jxta", "http://jxta.org");
            xmlDoc.addAttribute("xml:space", "preserve");
        }

        Element e;

        e = adv.createElement(typeTag, Integer.toString(getDiscoveryType()));
        adv.appendChild(e);

        int threshold = getThreshold();

        if (threshold < 0) {
            throw new IllegalStateException("threshold must be >= 0");
        }
        e = adv.createElement(thresholdTag, Integer.toString(threshold));
        adv.appendChild(e);

        PeerAdvertisement peerAdv = getPeerAdvertisement();

        if ((peerAdv != null)) {
            e = adv.createElement(peerAdvTag, peerAdv.toString());
            adv.appendChild(e);
        }

        String attr = getAttr();

        if ((attr != null) && (attr.length() > 0)) {
            e = adv.createElement(queryAttrTag, attr.trim());
            adv.appendChild(e);

            String value = getValue();

            if ((value != null) && (value.length() > 0)) {
                e = adv.createElement(queryValueTag, value.trim());
                adv.appendChild(e);
            } else {
                if (threshold < 0) {
                    throw new IllegalStateException("Attribute specified, but no value was specified.");
                }
            }
        }
        return adv;
    }

    /**
     * return the string representaion of this doc
     *
     * @deprecated should not be used. use getDocument().toString() instead.
     */
    @Override
    @Deprecated
    public String toString() {

        try {
            StructuredTextDocument doc = (StructuredTextDocument) getDocument(MimeMediaType.XMLUTF8);

            return doc.toString();
        } catch (Exception e) {
            if (e instanceof RuntimeException) {
                throw (RuntimeException) e;
            } else {
                throw new UndeclaredThrowableException(e);
            }
        }
    }
}
