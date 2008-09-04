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


import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.XMLElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.protocol.AccessPointAdvertisement;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * Associates a set of EndpointAddresses with a PeerID.
 * <p/>
 * <pre>
 *    &lt;xs:complexType name ="jxta:APA">
 *      &lt;xs:sequence>
 *        &lt;xs:element name="PID" type="jxta:JXTAID" minOccurs="0" maxOccurs="1"/>
 *        &lt;xs:sequence>
 *          &lt;xs:element name="EA" type="jxta:JXTAID" minOccurs="0" maxOccurs="unbounded"/>
 *        &lt;/xs:sequence>
 *      &lt;/xs:sequence>
 *    &lt;/xs:complexType>
 *  </pre>
 *
 * @see net.jxta.protocol.AccessPointAdvertisement
 */
public class AccessPointAdv extends AccessPointAdvertisement implements Cloneable {

    /**
     * Log4J Logger
     */
    private static final Logger LOG = Logger.getLogger(AccessPointAdv.class.getName());

    private static final String PID_TAG = "PID";
    private static final String EA_TAG = "EA";

    private static final String[] INDEXFIELDS = { PID_TAG};

    /**
     * Instantiator for AdvertisementFactory
     */
    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return AccessPointAdv.getAdvertisementType();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new AccessPointAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            return new AccessPointAdv(root);
        }
    }

    /**
     * Returns the identifying type of this Advertisement.
     * <p/>
     * <b>Note:</b> This is a static method. It cannot be used to determine
     * the runtime type of an advertisment. ie.
     * </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     * <p/>
     * <b>This is wrong and does not work the way you might expect.</b>
     * This call is not polymorphic and calls
     * Advertiement.getAdvertisementType() no matter what the real type of the
     * advertisment.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:APA";
    }

    /**
     * Private constructor.  Use the Advertisement factory and accessors.
     */
    private AccessPointAdv() {}

    /**
     * Private constructor.  Use the Advertisement factory and accessors.
     *
     * @param root the element
     */
    private AccessPointAdv(Element root) {
        if (!(root instanceof XMLElement)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;

        String doctype = doc.getName();

        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(getAdvertisementType()) && !getAdvertisementType().equals(typedoctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + elem.toString());
                }
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public AccessPointAdv clone() {
        return (AccessPointAdv) super.clone();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return INDEXFIELDS;
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
    public ID getID() {
        ID pid = getPeerID();

        if (null == pid) {
            throw new IllegalStateException("cannot build ID: no peer id");
        }

        try {
            // We have not yet built it. Do it now
            String seed = getAdvType() + getPeerID().toString();

            InputStream in = new ByteArrayInputStream(seed.getBytes());

            return IDFactory.newCodatID(PeerGroupID.worldPeerGroupID, seed.getBytes(), in);
        } catch (Exception ez) {
            IllegalStateException failed = new IllegalStateException("cannot build ID");

            failed.initCause(ez);

            throw failed;
        }
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

        if (PID_TAG.equals(elem.getName())) {
            String uri = elem.getTextValue();

            if (null != uri) {
                try {
                    URI pID = new URI(uri.trim());

                    setPeerID((PeerID) IDFactory.fromURI(pID));
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("Bad PeerID ID in advertisement");
                } catch (ClassCastException notPID) {
                    throw new IllegalArgumentException("Not a Peer ID");
                }

                return true;
            }
        }

        if (EA_TAG.equals(elem.getName())) {
            String epa = elem.getTextValue();

            if (null != epa) {
                addEndpointAddress(epa.trim());
                return true;
            }
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        if (getPeerID() != null) {
            Element e = adv.createElement(PID_TAG, getPeerID().toString());

            adv.appendChild(e);
        }

        Enumeration each = getEndpointAddresses();

        while (each.hasMoreElements()) {
            Element e2 = adv.createElement(EA_TAG, each.nextElement().toString());

            adv.appendChild(e2);
        }
        return adv;
    }
}
