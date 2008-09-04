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


import net.jxta.document.*;
import net.jxta.protocol.RouteAdvertisement;
import net.jxta.protocol.RouteResponseMsg;

import java.lang.reflect.UndeclaredThrowableException;
import java.util.Enumeration;


/**
 * Used by the Endpoint Routing protocol in response to Route Query Messages.
 * The Route Response Message contains a route advertisement for the destination
 * peer.
 * <p/>
 * <p/><pre>
 * &lt;xs:complexType name="ERR">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="Dst">
 *       &lt;xs:complexType>
 *         &lt;xs:sequence>
 *           &lt;xs:element ref="jxta:RA" />
 *         &lt;/xs:sequence>
 *       &lt;/xs:complexType>
 *     &lt;/xs:element>
 *     &lt;xs:element name="Src">
 *       &lt;xs:complexType>
 *         &lt;xs:sequence>
 *           &lt;xs:element ref="jxta:RA" />
 *         &lt;/xs:sequence>
 *       &lt;/xs:complexType>
 *     &lt;/xs:element>
 *   &lt;/xs:sequence>
 * &lt;/xs:complexType>
 * </pre>
 *
 * @see net.jxta.impl.endpoint.router.EndpointRouter
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-erp"
 *      target="_blank">JXTA Protocols Specification : Endpoint Routing Protocol</a>
 */
public class RouteResponse extends RouteResponseMsg {

    private static final String destRouteTag = "Dst";
    private static final String srcRouteTag = "Src";

    /**
     * Construct a new Route Response Message
     */
    public RouteResponse() {}

    /**
     * Construct from an XML document fragment.
     *
     * @param doc the element
     */
    public RouteResponse(XMLElement doc) {

        String doctype = doc.getName();

        if (!doctype.equals(getAdvertisementType())) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + " from doc containing a " + doctype);
        }

        Enumeration<XMLElement> elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = elements.nextElement();

            if (elem.getName().equals(destRouteTag)) {
                for (Enumeration<XMLElement> eachXpt = elem.getChildren(); eachXpt.hasMoreElements();) {
                    XMLElement aXpt = eachXpt.nextElement();

                    RouteAdvertisement route = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(aXpt);

                    setDestRoute(route);
                }
                continue;
            }

            if (elem.getName().equals(srcRouteTag)) {
                for (Enumeration<XMLElement> eachXpt = elem.getChildren(); eachXpt.hasMoreElements();) {
                    XMLElement aXpt = eachXpt.nextElement();

                    RouteAdvertisement route = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(aXpt);

                    setSrcRoute(route);
                }
            }
        }
        
        // Validate.
        
        if (null == getSrcRoute()) {
            throw new IllegalArgumentException("Missing source route.");
        }
        
        if (null == getDestRoute()) {
            throw new IllegalArgumentException("Missing destination route.");
        }

        if (null == getSrcRoute().getDestPeerID()) {
            throw new IllegalArgumentException("Bad source route.");
        }
        
        if (null == getDestRoute().getDestPeerID()) {
            throw new IllegalArgumentException("Bad destination route.");
        }
    }

    /**
     * return a Document representation of this object
     */
    @Override
    public Document getDocument(MimeMediaType asMimeType) {        
        if (null == getSrcRoute()) {
            throw new IllegalStateException("Missing source route.");
        }
        
        if (null == getDestRoute()) {
            throw new IllegalStateException("Missing destination route.");
        }

        if (null == getSrcRoute().getDestPeerID()) {
            throw new IllegalStateException("Bad source route.");
        }
        
        if (null == getDestRoute().getDestPeerID()) {
            throw new IllegalStateException("Bad destination route.");
        }

        StructuredDocument adv = StructuredDocumentFactory.newStructuredDocument(asMimeType, getAdvertisementType());

        if (adv instanceof XMLDocument) {
            ((XMLDocument) adv).addAttribute("xmlns:jxta", "http://jxta.org");
            ((XMLDocument) adv).addAttribute("xml:space", "preserve");
        }

        Element e;

        RouteAdvertisement route = getDestRoute();

        if (route != null) {
            e = adv.createElement(destRouteTag);
            adv.appendChild(e);
            StructuredTextDocument xptDoc = (StructuredTextDocument)
                    route.getDocument(asMimeType);

            StructuredDocumentUtils.copyElements(adv, e, xptDoc);
        }

        route = getSrcRoute();
        if (route != null) {
            e = adv.createElement(srcRouteTag);
            adv.appendChild(e);
            StructuredTextDocument xptDoc = (StructuredTextDocument)
                    route.getDocument(asMimeType);

            StructuredDocumentUtils.copyElements(adv, e, xptDoc);
        }
        return adv;
    }

    /**
     * {@inheritDoc}
     * 
     * <p/>String representation of this RouteResponse doc.
     */
    @Override
    public String toString() {
        XMLDocument doc = (XMLDocument) getDocument(MimeMediaType.XMLUTF8);

        doc.addAttribute("xml:space", "default");

        return doc.toString();
    }
}
