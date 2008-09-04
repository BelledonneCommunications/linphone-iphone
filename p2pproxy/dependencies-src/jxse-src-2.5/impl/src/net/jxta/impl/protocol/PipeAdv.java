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


import java.net.URI;
import java.util.Enumeration;
import java.net.URISyntaxException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.pipe.PipeService;
import net.jxta.protocol.PipeAdvertisement;


/**
 * This class implements the Pipe Advertisement according to the schema used by
 * the standard Pipe Binding Protocol. (PBP)
 *
 * <p/><pre>
 * &lt;xs:complexType name="PipeAdvertisement">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="Id" type="jxta:JXTAID"/>
 *     &lt;xs:element name="Type" type="xs:string"/>
 *     &lt;xs:element name="Name" type="xs:string" minOccurs="0"/>
 *     &lt;xs:element name="Desc" type="xs:anyType" minOccurs="0"/>
 *   &lt;/xs:sequence>
 * &lt;/xs:complexType>
 * </pre>
 *
 *  @see net.jxta.protocol.PipeAdvertisement
 *  @see net.jxta.pipe.PipeService
 *  @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pbp" target="_blank">JXTA Protocols Specification : Pipe Binding Protocol</a>
 */
public class PipeAdv extends PipeAdvertisement {

    /**
     *  Logger
     */
    private final static Logger LOG = Logger.getLogger(PipeAdv.class.getName());

    /**
     *  Fields which will be returned by {@link #getIndexFields()}
     **/
    private static final String[] INDEX_FIELDS = { PipeAdvertisement.NameTag, PipeAdvertisement.IdTag};

    /**
     *  AvertisementFactory instantiator for our type.
     */
    public final static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         *  {@inheritDoc}
         */
        public String getAdvertisementType() {
            return PipeAdv.getAdvertisementType();
        }

        /**
         *  {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new PipeAdv();
        }

        /**
         *  {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }

            return new PipeAdv((XMLElement) root);
        }
    }

    /**
     *  Private constructor for new instances. Use the instantiator.
     */
    private PipeAdv() {}

    /**
     *  Private constructor for xml serialized instances. Use the instantiator.
     *  
     *  @param doc The XML serialization of the advertisement.
     */
    private PipeAdv(XMLElement doc) {
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
                    LOG.fine("Unhandled Element: " + elem);
                }
            }
        }

        // Sanity Check!!!
        if ((null == getPipeID()) || getPipeID().equals(ID.nullID)) {
            throw new IllegalArgumentException("Bad pipe ID in advertisement");
        }

        if ((null == getType()) || (0 == getType().length())) {
            throw new IllegalArgumentException("Bad pipe type in advertisement");
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
     *  {@inheritDoc}
     */
    @Override
    protected boolean handleElement(Element raw) {

        if (super.handleElement(raw)) {
            return true;
        }

        XMLElement elem = (XMLElement) raw;

        if (PipeAdvertisement.descTag.equals(elem.getName())) {
            setDesc(elem);
            return true;
        }

        String value = elem.getTextValue();
        
        if ((null == value) || (0 == value.trim().length())) {
            return false;
        }
        
        value = value.trim();
        
        if (PipeAdvertisement.IdTag.equals(elem.getName())) {
            try {
                URI pipeID = new URI(value);

                setPipeID(IDFactory.fromURI(pipeID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad pipe ID in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("ID is not a pipe ID");
            }
            return true;
        }

        if (PipeAdvertisement.NameTag.equals(elem.getName())) {
            setName(value);
            return true;
        }

        if (PipeAdvertisement.TypeTag.equals(elem.getName())) {
            setType(value);
            return true;
        }
        
        return false;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        ID itsID = getPipeID();

        if ((null == itsID) || itsID.equals(ID.nullID)) {
            throw new IllegalStateException("Pipe has no assigned ID");
        }

        Element e = adv.createElement(PipeAdvertisement.IdTag, itsID.toString());

        adv.appendChild(e);

        if ((null == getType()) || (0 == getType().length())) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Pipe type not set. Defaulting to " + PipeService.UnicastType + "."
                        + "\n This default is deprecated. Please set the pipe type in your code.");
            }

            setType(PipeService.UnicastType);
            // throw new IllegalArgumentException("Pipe type missing in advertisement");
        }

        e = adv.createElement(PipeAdvertisement.TypeTag, getType());
        adv.appendChild(e);

        // name is optional
        if (getName() != null) {
            e = adv.createElement(PipeAdvertisement.NameTag, getName());
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
     *  {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return INDEX_FIELDS;
    }
}
