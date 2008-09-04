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
import net.jxta.id.IDFactory;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleSpecAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;


/**
 * Provides XML serialization support for ModuleSpecAdvertisement matching the
 * schema defined by the JXTA Core Specification.
 * <p/>
 * <p/><pre>
 *  &lt;xs:complexType name="MSA">
 *    &lt;xs:sequence>
 *      &lt;xs:element name="MSID" type="jxta:JXTAID" />
 *      &lt;xs:element name="Name" type="xs:string" minOccurs="0" />
 *      &lt;xs:element name="Desc" type="xs:anyType" minOccurs="0" />
 *      &lt;xs:element name="Crtr" type="xs:string" minOccurs="0" />
 *      &lt;xs:element name="SURI" type="xs:anyURI" minOccurs="0" />
 *      &lt;xs:element name="Vers" type="xs:string" />
 *      &lt;xs:element name="Parm" type="xs:anyType" minOccurs="0" />
 *      &lt;xs:element ref="jxta:PipeAdvertisement" minOccurs="0" />
 *      &lt;xs:element name="Proxy" type="xs:anyURI" minOccurs="0" />
 *      &lt;xs:element name="Auth" type="jxta:JXTAID" minOccurs="0" />
 *    &lt;/xs:sequence>
 *  &lt;/xs:complexType>
 * </pre>
 *
 * @see net.jxta.document.Advertisement
 * @see net.jxta.protocol.ModuleSpecAdvertisement
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#advert-msa> target='_blank'>JXTA Protocols Specification - Advertisements : Module Specification Advertisement</a>
 */
public class ModuleSpecAdv extends ModuleSpecAdvertisement {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(ModuleSpecAdv.class.getName());

    private static final String idTag = "MSID";
    private static final String nameTag = "Name";
    private static final String creatorTag = "Crtr";
    private static final String uriTag = "SURI";
    private static final String versTag = "Vers";
    private static final String descTag = "Desc";
    private static final String paramTag = "Parm";
    private static final String proxyIdTag = "Proxy";
    private static final String authIdTag = "Auth";
    private static final String[] fields = { nameTag, idTag};

    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */

        public String getAdvertisementType() {
            return ModuleSpecAdv.getAdvertisementType();
        }

        /**
         * {@inheritDoc}
         */

        public Advertisement newInstance() {
            return new ModuleSpecAdv();
        }

        /**
         * {@inheritDoc}
         */

        public Advertisement newInstance(net.jxta.document.Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }

            return new ModuleSpecAdv((XMLElement) root);
        }
    }

    /**
     *  Private constructor for new instances. Use the instantiator.
     */
    private ModuleSpecAdv() {}

    /**
     *  Private constructor for xml serialized instances. Use the instantiator.
     *  
     *  @param doc The XML serialization of the advertisement.
     */
    private ModuleSpecAdv(XMLElement doc) {
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

        // Sanity Check!!!
        if (null == getModuleSpecID()) {
            throw new IllegalArgumentException("Module Spec Advertisement did not contain a module spec id.");
        }
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

        String nm = elem.getName();

        if (nm.equals(nameTag)) {
            setName(elem.getTextValue());
            return true;
        }

        if (nm.equals(descTag)) {
            setDesc(elem);
            return true;
        }

        if (nm.equals(idTag)) {
            try {
                URI specID = new URI(elem.getTextValue());

                setModuleSpecID((ModuleSpecID) IDFactory.fromURI(specID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad msid in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Unusable msid in advertisement");
            }
            return true;
        }

        if (nm.equals(creatorTag)) {
            setCreator(elem.getTextValue());
            return true;
        }

        if (nm.equals(uriTag)) {
            setSpecURI(elem.getTextValue());
            return true;
        }

        if (nm.equals(versTag)) {
            setVersion(elem.getTextValue());
            return true;
        }

        if (nm.equals(paramTag)) {
            // Copy the element into a complete new document
            // which type matches the element name. There is no
            // API Advertisement for it, each module implementation
            // may have its own Advertisement subclass for its param.
            setParam(elem);
            return true;
        }

        if (nm.equals(proxyIdTag)) {
            try {
                URI spID = new URI(elem.getTextValue());

                setProxySpecID((ModuleSpecID) IDFactory.fromURI(spID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad proxy spec id in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Unusable proxy spec id in advertisement");
            }
            return true;
        }

        if (nm.equals(authIdTag)) {
            try {
                URI spID = new URI(elem.getTextValue());

                setAuthSpecID((ModuleSpecID) IDFactory.fromURI(spID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad auth spec id in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Unusable auth spec id in advertisement");
            }
            return true;
        }

        if (nm.equals(PipeAdvertisement.getAdvertisementType())) {
            try {
                PipeAdvertisement pipeAdv = (PipeAdvertisement)
                        AdvertisementFactory.newAdvertisement(elem);

                setPipeAdvertisement(pipeAdv);
            } catch (ClassCastException wrongAdv) {
                throw new IllegalArgumentException("Bad pipe advertisement in advertisement");
            }
            return true;
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        if (null == getModuleSpecID()) {
            throw new IllegalStateException("Module Spec Advertisement did not contain a module spec id.");
        }

        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        Element e;

        e = adv.createElement(idTag, getModuleSpecID().toString());
        adv.appendChild(e);

        if (null != getName()) {
            e = adv.createElement(nameTag, getName());
            adv.appendChild(e);
        }

        // desc is optional
        StructuredDocument desc = getDesc();

        if (desc != null) {
            StructuredDocumentUtils.copyElements(adv, adv, desc);
        }

        e = adv.createElement(creatorTag, getCreator());
        adv.appendChild(e);

        e = adv.createElement(uriTag, getSpecURI());
        adv.appendChild(e);

        e = adv.createElement(versTag, getVersion());
        adv.appendChild(e);

        PipeAdvertisement pipeAdv = getPipeAdvertisement();

        if (pipeAdv != null) {
            StructuredTextDocument advDoc = (StructuredTextDocument)
                    pipeAdv.getDocument(encodeAs);

            StructuredDocumentUtils.copyElements(adv, adv, advDoc);
        }
        ModuleSpecID tmpId = getProxySpecID();

        if (tmpId != null) {
            e = adv.createElement(proxyIdTag, tmpId.toString());
            adv.appendChild(e);
        }
        tmpId = getAuthSpecID();
        if (tmpId != null) {
            e = adv.createElement(authIdTag, tmpId.toString());
            adv.appendChild(e);
        }

        e = getParamPriv();
        // Copy the param document as an element of adv.
        if (e != null) {
            // Force the element to be named "Parm" even if that is not
            // the name of paramDoc.
            StructuredDocumentUtils.copyElements(adv, adv, e, paramTag);
        }

        return adv;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return fields;
    }
}
