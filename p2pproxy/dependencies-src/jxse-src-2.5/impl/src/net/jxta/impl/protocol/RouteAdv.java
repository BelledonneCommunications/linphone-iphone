/*
Copyright (c) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
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


import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLElement;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.protocol.AccessPointAdvertisement;
import net.jxta.protocol.RouteAdvertisement;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * This class implements the basic Route advertisement.
 * <p/>
 * <pre>
 *    &lt;xs:complexType name="RA">
 *      &lt;xs:sequence>
 *          &lt;xs:element name="DstPID" type="jxta:JXTAID" minOccurs="0"/>
 *          &lt;xs:element name="Dst">
 *              &lt;xs:complexType>
 *            &lt;xs:sequence>
 *                      &lt;xs:element ref="jxta:APA" />
 *            &lt;/xs:sequence>
 *              &lt;/xs:complexType>
 *        &lt;/xs:element>
 *        &lt;xs:element name="Hops" minOccurs="0">
 *              &lt;xs:complexType>
 *            &lt;xs:sequence>
 *                      &lt;xs:element ref="jxta:APA" maxOccurs="unbounded" />
 *            &lt;/xs:sequence>
 *              &lt;/xs:complexType>
 *        &lt;/xs:element>
 *      &lt;/xs:sequence>
 *    &lt;/xs:complexType>
 * </pre>
 *
 * @see net.jxta.protocol.RouteAdvertisement
 */
public class RouteAdv extends RouteAdvertisement implements Cloneable {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(RouteAdv.class.getName());

    private static final String[] INDEX_FIELDS = {DEST_PID_TAG};

    /**
     * Instantiator for our advertisement
     */
    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return RouteAdv.getAdvertisementType();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new RouteAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getAdvertisementType() + " only supports XLMElement");
            }

            return new RouteAdv((XMLElement) root);
        }
    }

    /**
     * Private constructor. Use instantiator
     */
    private RouteAdv() {}

    /**
     * Private constructor. Use instantiator
     *
     * @param doc the element
     */
    private RouteAdv(XMLElement doc) {
        String doctype = doc.getName();

        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(getAdvertisementType()) && !getAdvertisementType().equals(typedoctype)) {
            throw new IllegalArgumentException("Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        Enumeration<XMLElement> elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + elem.toString());
                }
            }
        }

        // Compatibility hack
        setDestPeerID(getDestPeerID());

        // Sanity Check!!!
        if (hasALoop()) {
            throw new IllegalArgumentException("Route contains a loop!");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public RouteAdv clone() {
        return (RouteAdv) super.clone();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected boolean handleElement(Element raw) {

        if (super.handleElement(raw)) {
            return true;
        }

        XMLElement elem = (XMLElement) raw;

        if (DEST_PID_TAG.equals(elem.getName())) {
            try {
                URI pID = new URI(elem.getTextValue());

                setDestPeerID((PeerID) IDFactory.fromURI(pID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad PeerID in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("ID in advertisement is not a peer id");
            }
            return true;
        }

        if ("Dst".equals(elem.getName())) {
            for (Enumeration eachXpt = elem.getChildren(); eachXpt.hasMoreElements();) {
                XMLElement aXpt = (XMLElement) eachXpt.nextElement();

                AccessPointAdvertisement xptAdv = (AccessPointAdvertisement)
                        AdvertisementFactory.newAdvertisement(aXpt);

                setDest(xptAdv);
            }
            return true;
        }

        if ("Hops".equals(elem.getName())) {
            Vector<AccessPointAdvertisement> hops = new Vector<AccessPointAdvertisement>();

            for (Enumeration eachXpt = elem.getChildren(); eachXpt.hasMoreElements();) {
                XMLElement aXpt = (XMLElement) eachXpt.nextElement();

                AccessPointAdvertisement xptAdv = (AccessPointAdvertisement)
                        AdvertisementFactory.newAdvertisement(aXpt);

                hops.addElement(xptAdv);
            }
            setHops(hops);
            return true;
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        if (hasALoop()) {
            throw new IllegalStateException("I won't write a doc for a route with a loop");
        }

        PeerID pid = getDestPeerID();

        if (null != pid) {
            Element e0 = adv.createElement(DEST_PID_TAG, pid.toString());

            adv.appendChild(e0);
        }

        Element e1 = adv.createElement("Dst");

        adv.appendChild(e1);

        AccessPointAdvertisement dest = getDest();

        // create a copy without the PID if necessary (the pid is redundant)
        AccessPointAdvertisement destAPA = dest;
        if(null != dest.getPeerID()) {
            destAPA = dest.clone();
            destAPA.setPeerID(null);
        }
        
        StructuredDocument xptDoc = (StructuredDocument) destAPA.getDocument(encodeAs);
        StructuredDocumentUtils.copyElements(adv, e1, xptDoc);

        Enumeration<AccessPointAdvertisement> eachHop = getHops();

        // only include hops if we have some
        if (eachHop.hasMoreElements()) {            
            Element e2 = adv.createElement("Hops");

            adv.appendChild(e2);

            while (eachHop.hasMoreElements()) {
                AccessPointAdvertisement hop = eachHop.nextElement();
                
                if (null == hop.getPeerID()) {
                    // Refuse to write illegal hops.
                    continue;
                }

                xptDoc = (StructuredDocument) hop.getDocument(encodeAs);
                    
                StructuredDocumentUtils.copyElements(adv, e2, xptDoc);
            }
        }
        return adv;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return INDEX_FIELDS;
    }
}
