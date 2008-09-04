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
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.XMLElement;
import net.jxta.id.IDFactory;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.protocol.RdvAdvertisement;
import net.jxta.protocol.RouteAdvertisement;


/**
 * This class implements the RdvAdvertisement.
 *
 * <p/><pre>
 *   &lt;xs:complexType name="RdvAdvertisement">
 *     &lt;xs:sequence>
 *       &lt;xs:element name="RdvGroupId" type="jxta:JXTAID"/>
 *       &lt;xs:element name="RdvPeerId" type="jxta:JXTAID"/>
 *       &lt;xs:element name="RdvServiceName" type="xs:string"/>
 *       &lt;xs:element name="Name" type="xs:string" minOccurs="0"/>
 *       &lt;!-- This should be a route -->
 *       &lt;xs:element name="RdvRoute" type="xs:anyType" minOccurs="0"/>
 *     &lt;/xs:sequence>
 *   &lt;/xs:complexType>
 * </pre>
 **/
public class RdvAdv extends RdvAdvertisement {
    
    /**
     *  Log4J Logger
     **/
    private final static transient Logger LOG = Logger.getLogger(RdvAdv.class.getName());   
    
    private static final String[] INDEX_FIELDS = { PeerIDTag, ServiceNameTag, GroupIDTag };
    
    /**
     * Instantiator for our advertisement
     **/
    public static class Instantiator implements AdvertisementFactory.Instantiator {
        
        /**
         * {@inheritDoc}
         **/
        public String getAdvertisementType() {
            return RdvAdv.getAdvertisementType();
        }
        
        /**
         * {@inheritDoc}
         **/
        public Advertisement newInstance() {
            return new RdvAdv();
        }
        
        /**
         * {@inheritDoc}
         **/
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }
        
            return new RdvAdv((XMLElement) root);
        }
    }
    
    /**
     *  Private constructor for new instances. Use the instantiator.
     */
    private RdvAdv() {
    }

    /**
     *  Private constructor for xml serialized instances. Use the instantiator.
     *  
     *  @param doc The XML serialization of the advertisement.
     */
    private RdvAdv(XMLElement doc) {
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
        if (null == getGroupID()) {
            throw new IllegalArgumentException("Missing peer group ID");
        }
        
        if (null == getPeerID()) {
            throw new IllegalArgumentException("Missing peer ID");
        }
        
        if (null == getServiceName()) {
            throw new IllegalArgumentException("Missing service name");
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
        
        if (elem.getName().equals(RdvAdvertisement.GroupIDTag)) {
            try {
                URI groupID = new URI(elem.getTextValue().trim());

                setGroupID((PeerGroupID) IDFactory.fromURI(groupID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad group ID in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("ID is not a group ID");
            }
            return true;
        }
        
        if (elem.getName().equals(RdvAdvertisement.PeerIDTag)) {
            try {
                URI peerID = new URI(elem.getTextValue().trim());

                setPeerID((PeerID) IDFactory.fromURI(peerID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad group ID in advertisement");
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("ID is not a group ID");
            }
            return true;
        }
        
        if (elem.getName().equals(RdvAdvertisement.ServiceNameTag)) {
            setServiceName(elem.getTextValue());
            return true;
        }
        
        if (elem.getName().equals(RdvAdvertisement.RouteTag)) {
            for (Enumeration eachXpt = elem.getChildren(); eachXpt.hasMoreElements();) {
                
                XMLElement aXpt = (XMLElement) eachXpt.nextElement();
                
                RouteAdvertisement xptAdv = (RouteAdvertisement)
                        AdvertisementFactory.newAdvertisement(aXpt);

                setRouteAdv(xptAdv);
            }
            return true;
        }
        
        if (elem.getName().equals(RdvAdvertisement.NameTag)) {
            setName(elem.getTextValue());
            return true;
        }
        
        return false;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        
        // Sanity Check!!!
        if (null == getGroupID()) {
            throw new IllegalStateException("Missing peer group ID");
        }
        
        if (null == getPeerID()) {
            throw new IllegalStateException("Missing peer ID");
        }
        
        if (null == getServiceName()) {
            throw new IllegalStateException("Missing service name");
        }

        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);
        
        Element e = adv.createElement(RdvAdvertisement.GroupIDTag, getGroupID().toString());

        adv.appendChild(e);
        
        e = adv.createElement(RdvAdvertisement.PeerIDTag, getPeerID().toString());
        adv.appendChild(e);
        
        e = adv.createElement(RdvAdvertisement.ServiceNameTag, getServiceName());
        adv.appendChild(e);
        
        String peerName = getName();

        if (null != peerName) {
            e = adv.createElement(RdvAdvertisement.NameTag, getName());
            adv.appendChild(e);
        }
        
        if (getRouteAdv() != null) {
            Element el = adv.createElement(RdvAdvertisement.RouteTag);

            adv.appendChild(el);
            
            StructuredTextDocument xptDoc = (StructuredTextDocument)
                    getRouteAdv().getDocument(encodeAs);

            StructuredDocumentUtils.copyElements(adv, el, xptDoc);
        }
        
        return adv;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public String[] getIndexFields() {
        return INDEX_FIELDS;
    }
}
