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
import net.jxta.platform.ModuleClassID;
import net.jxta.protocol.ModuleClassAdvertisement;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;


/**
 * Provides XML serialization support for ModuleClassAdvertisement matching the
 * schema defined by the JXTA Core Specification.
 * <p/>
 * <p/><pre>
 *  &lt;xs:complexType name="MCA">
 *    &lt;xs:sequence>
 *      &lt;xs:element name="MCID" type="jxta:JXTAID" />
 *      &lt;xs:element name="Name" type="xs:string" minOccurs="0" />
 *      &lt;xs:element name="Desc" type="xs:anyType" minOccurs="0" />
 *    &lt;/xs:sequence>
 *  &lt;/xs:complexType>
 *  </pre>
 *
 * @see net.jxta.document.Advertisement
 * @see net.jxta.protocol.ModuleSpecAdvertisement
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#advert-mca> target='_blank'>JXTA Protocols Specification - Advertisements : Module Class Advertisement</a>
 */
public class ModuleClassAdv extends ModuleClassAdvertisement {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(ModuleClassAdv.class.getName());

    private static final String nameTag = "Name";
    private static final String idTag = "MCID";
    private static final String descTag = "Desc";
    private static final String[] fields = {nameTag, idTag};

    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return ModuleClassAdvertisement.getAdvertisementType();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new ModuleClassAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }

            return new ModuleClassAdv((XMLElement) root);
        }
    }

    /**
     *  Private constructor for new instances. Use the instantiator.
     */
    private ModuleClassAdv() {}

    /**
     *  Private constructor for xml serialized instances. Use the instantiator.
     *  
     *  @param doc The XML serialization of the advertisement.
     */
    private ModuleClassAdv(XMLElement doc) {

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

        Enumeration<XMLElement> elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + elem.toString());
                }
            }
        }

        // Sanity Check!!!
        if (null == getModuleClassID()) {
            throw new IllegalArgumentException("Module Class ID was not specified.");
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

        if (elem.getName().equals(nameTag)) {
            setName(elem.getTextValue());
            return true;
        }

        if (elem.getName().equals(descTag)) {
            setDesc(elem);
            return true;
        }

        if (elem.getName().equals(idTag)) {
            try {
                URI clID = new URI(elem.getTextValue());

                setModuleClassID((ModuleClassID) IDFactory.fromURI(clID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad mcid in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Unusable mcid in advertisement");
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
        if (null == getModuleClassID()) {
            throw new IllegalStateException("Module Class ID was not specified.");
        }

        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        Element e;

        e = adv.createElement(idTag, getModuleClassID().toString());
        adv.appendChild(e);

        // name is optional
        String name = getName();

        if (null != name) {
            e = adv.createElement(nameTag, name);
            adv.appendChild(e);
        }

        // desc is optional
        StructuredDocument desc = getDesc();

        if (desc != null) {
            StructuredDocumentUtils.copyElements(adv, adv, desc);
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
