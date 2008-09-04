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
import java.util.Hashtable;

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
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.platform.ModuleClassID;
import net.jxta.protocol.PeerAdvertisement;


/**
 * Implementation of {@link PeerAdvertisement} matching the standard JXTA
 * Protocol Specification.
 *
 * It implements Peer Advertisement using the following schema:
 *
 * <pre><tt>
 * &lt;xs:complexType name="PA">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="PID" type="JXTAID"/>
 *     &lt;xs:element name="GID" type="JXTAID"/>
 *     &lt;xs:element name="Name" type="xs:string" minOccurs="0"/>
 *     &lt;xs:element name="Desc" type="xs:anyType" minOccurs="0"/>
 *     &lt;xs:element name="Svc" type="jxta:serviceParams" minOccurs="0" maxOccurs="unbounded"/>
 *   &lt;xs:sequence>
 * &lt;/xs:complexType>
 * </tt></pre>
 *
 * @see net.jxta.protocol.PeerAdvertisement
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#advert-pa" target="_blank">JXTA Protocols Specification : Peer Advertisement</a>
 **/
public class PeerAdv extends PeerAdvertisement {

    /**
     *  Logger
     **/
    private static final Logger LOG = Logger.getLogger(PeerAdv.class.getName());
    
    private static final String pidTag = "PID";
    private static final String gidTag = "GID";
    private static final String nameTag = "Name";
    private static final String descTag = "Desc";
    private static final String svcTag = "Svc";
    private static final String mcidTag = "MCID";
    private static final String paramTag = "Parm";
    private static final String[] fields = { nameTag, pidTag };
    
    /**
     *  Creates instances of PeerAdvertisement.
     **/
    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         *  {@inheritDoc}
         **/
        public String getAdvertisementType() {
            return PeerAdvertisement.getAdvertisementType();
        }
        
        /**
         *  {@inheritDoc}
         **/
        public Advertisement newInstance() {
            return new PeerAdv();
        }
        
        /**
         *  {@inheritDoc}
         **/
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }
        
            return new PeerAdv((XMLElement) root);
        }
    }

    /**
     *  Private constructor for new instances. Use the instantiator.
     */
    private PeerAdv() {}

    /**
     *  Private constructor for xml serialized instances. Use the instantiator.
     *  
     *  @param doc The XML serialization of the advertisement.
     */
    private PeerAdv(XMLElement doc) {
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
        
        // sanity check time!
        if (null == getPeerID()) {
            throw new IllegalArgumentException("Peer Advertisement did not contain a peer id.");
        }
        
        if (null == getPeerGroupID()) {
            throw new IllegalArgumentException("Peer Advertisement did not contain a peer group id.");
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
     **/
    @Override
    protected boolean handleElement(Element raw) {
        
        if (super.handleElement(raw)) {
            return true;
        }
        
        XMLElement elem = (XMLElement) raw;
        
        if (elem.getName().equals(pidTag)) {
            try {
                URI pID = new URI(elem.getTextValue());

                setPeerID((PeerID) IDFactory.fromURI(pID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad PeerID ID in advertisement: " + elem.getTextValue());
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Id is not a peer id: " + elem.getTextValue());
            }
            return true;
        }
        
        if (elem.getName().equals(gidTag)) {
            try {
                URI gID = new URI(elem.getTextValue());

                setPeerGroupID((PeerGroupID) IDFactory.fromURI(gID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad PeerGroupID in advertisement: " + elem.getTextValue());
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Id is not a group id: " + elem.getTextValue());
            }
            return true;
        }

        if (elem.getName().equals(nameTag)) {
            setName(elem.getTextValue());
            return true;
        }
        
        if (elem.getName().equals(descTag)) {
            setDesc(elem);
            return true;
        }
        
        if (elem.getName().equals(svcTag)) {
            Enumeration elems = elem.getChildren();
            ModuleClassID classID = null;
            Element param = null;

            while (elems.hasMoreElements()) {
                XMLElement e = (XMLElement) elems.nextElement();

                if (e.getName().equals(mcidTag)) {
                    try {
                        URI mcid = new URI(e.getTextValue());

                        classID = (ModuleClassID) IDFactory.fromURI(mcid);
                    } catch (URISyntaxException badID) {
                        throw new IllegalArgumentException("Unusable ModuleClassID in advertisement: " + e.getTextValue());
                    } catch (ClassCastException badID) {
                        throw new IllegalArgumentException("Id is not a ModuleClassID: " + e.getTextValue());
                    }
                    continue;
                }
                if (e.getName().equals(paramTag)) {
                    param = e;
                }
            }
            if (classID != null && param != null) {
                // Add this param to the table. putServiceParam()
                // clones param into a standalone document automatically.
                // (classID gets cloned too).
                putServiceParam(classID, param);
            }
            return true;
        }
        
        // element was not handled
        return false;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);
        
        PeerID peerID = getPeerID();

        if ((null == peerID) || ID.nullID.equals(peerID)) {
            throw new IllegalStateException("Cannot generate Peer Advertisement with no Peer ID!");
        }
        Element e = adv.createElement(pidTag, peerID.toString());

        adv.appendChild(e);
        
        PeerGroupID groupID = getPeerGroupID();

        if ((null == groupID) || ID.nullID.equals(groupID)) {
            throw new IllegalStateException("Cannot generate Peer Advertisement with no group ID!");
        } else {
            e = adv.createElement(gidTag, groupID.toString());
            adv.appendChild(e);
        }
        
        // name is optional
        if (getName() != null) {
            e = adv.createElement(nameTag, getName());
            adv.appendChild(e);
        }
        
        // desc is optional
        StructuredDocument desc = getDesc();

        if (desc != null) {
            StructuredDocumentUtils.copyElements(adv, adv, desc);
        }
        
        // service params are optional
        // FIXME: this is inefficient - we force our base class to make
        // a deep clone of the table.
        Hashtable serviceParams = getServiceParams();
        Enumeration classIds = serviceParams.keys();

        while (classIds.hasMoreElements()) {
            ModuleClassID classId = (ModuleClassID) classIds.nextElement();
            
            Element s = adv.createElement(svcTag);

            adv.appendChild(s);
            
            e = adv.createElement(mcidTag, classId.toString());
            s.appendChild(e);
            
            e = (Element) serviceParams.get(classId);
            StructuredDocumentUtils.copyElements(adv, s, e, paramTag);
        }
        return adv;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public String[] getIndexFields() {
        return fields;
    }
}
