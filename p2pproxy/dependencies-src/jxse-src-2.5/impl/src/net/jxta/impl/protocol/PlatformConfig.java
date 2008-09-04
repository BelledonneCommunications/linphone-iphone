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
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.protocol.ConfigParams;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * Configuration container for the World Peer Group. For historical reasons the
 * same configuration container and instance is also used for the Net Peer Group.
 */
public final class PlatformConfig extends GroupConfig implements Cloneable {

    private static final String advType = "jxta:PlatformConfig";

    /**
     * Instantiator for PlatformConfig
     */
    public final static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return advType;
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new PlatformConfig();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(advType + " only supports XLMElement");
            }

            return new PlatformConfig((XMLElement) root);
        }
    }

    private static final Logger LOG = Logger.getLogger(PlatformConfig.class.getName());

    private static final String PID_TAG = "PID";
    private static final String NAME_TAG = "Name";
    private static final String DESC_TAG = "Desc";

    /**
     * The id of this peer.
     */
    private PeerID pid = null;

    /**
     * The name of this peer. Not guaranteed to be unique in any way. May be
     * empty or null.
     */
    private String name = null;

    /**
     * Descriptive meta-data about this peer.
     */
    private Element description = null;

    /**
     * Use the Instantiator through the factory
     */
    PlatformConfig() {}

    /**
     * Use the Instantiator through the factory
     *
     * @param doc the element
     */
    PlatformConfig(XMLElement doc) {
        String doctype = doc.getName();

        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(getAdvertisementType()) && !getAdvertisementType().equals(typedoctype)) {
            throw new IllegalArgumentException( "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
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
    }

    /**
     * Make a safe clone of this PlatformConfig.
     *
     * @return Object an object of class PlatformConfig that is a deep-enough
     *         copy of this one.
     */
    @Override
    public PlatformConfig clone() {
        PlatformConfig result = (PlatformConfig) super.clone();

        result.setPeerID(getPeerID());
        result.setName(getName());
        result.setDesc(getDesc());

        return result;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object other) {
        if(this == other) {
            return true;
        }
        
        if(other instanceof PlatformConfig) {
            PlatformConfig likeMe = (PlatformConfig) other;
            
            boolean se = super.equals(likeMe);
            
            boolean ne = ((null == name) && (null == likeMe.name)) || ((null != name) && name.equals(likeMe.name));
            boolean ie = ((null == pid) && (null == likeMe.pid)) || ((null != pid) && pid.equals(likeMe.pid));
            boolean de = ((null == description) && (null == likeMe.description)) || ((null != description) && description.equals(likeMe.description));
            
            return se && ne && ie && de;
        }
        
        return false;
    }
    
    /**
     * returns the advertisement type
     *
     * @return string type
     */
    public static String getAdvertisementType() {
        return advType;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getAdvType() {
        return getAdvertisementType();
    }

    /**
     * returns the name of the peer.
     *
     * @return String name of the peer.
     */
    public String getName() {
        return name;
    }

    /**
     * sets the name of the peer.
     *
     * @param name name of the peer.
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * Returns the id of the peer.
     *
     * @return PeerID the peer id
     */
    public PeerID getPeerID() {
        return pid;
    }

    /**
     * Sets the peer ID to use for this peer.
     *
     * @param pid The peer ID to use for this peer.
     */
    public void setPeerID(PeerID pid) {
        this.pid = pid;
    }

    /**
     * Returns a unique ID for that peer X group intersection. This is for
     * indexing purposes only.
     */
    @Override
    public ID getID() {
        return pid;
    }

    /**
     * returns the description
     *
     * @return String the description
     */
    public String getDescription() {
        return (null == description) ? null : (String) description.getValue();
    }

    /**
     * Sets the description
     *
     * @param description the description
     */
    public void setDescription(String description) {
        StructuredDocument newdoc = null;

        if (null != description) {
            newdoc = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, DESC_TAG, description);
        }
        setDesc(newdoc);
    }

    /**
     * Returns the description
     *
     * @return the description
     */
    public StructuredDocument getDesc() {
        StructuredDocument newDoc = null;

        if (null != description) {
            newDoc = StructuredDocumentUtils.copyAsDocument(description);
        }
        return newDoc;
    }

    /**
     * Sets the description
     *
     * @param desc the description
     */
    public void setDesc(Element desc) {
        if (null != desc) {
            this.description = StructuredDocumentUtils.copyAsDocument(desc);
        } else {
            this.description = null;
        }
    }

    /**
     * returns the debugLevel
     *
     * @deprecated The debug level is no longer set via this api. See
     * {@link net.jxta.logging.Logging}.
     *
     * @return String the debugLevel
     */
    @Deprecated
    public String getDebugLevel() {
        return "user default";
    }

    /**
     * Sets the debugLevel
     *
     * @deprecated The debug level is no longer set via this api. See
     * {@link net.jxta.logging.Logging}.
     *
     * @param debugLevel the debugLevel
     */
    @Deprecated
    public void setDebugLevel(String debugLevel) {}

    /**
     * {@inheritDoc}
     */
    @Override
    protected boolean handleElement(Element raw) {

        if (super.handleElement(raw)) {
            return true;
        }

        XMLElement elem = (XMLElement) raw;
        String elName = elem.getName();

        if (DESC_TAG.equals(elName)) {
            setDesc(elem);
            return true;
        }

        String value = elem.getTextValue();

        if (null == value) {
            return false;
        }
        value = value.trim();

        if (0 == value.length()) {
            return false;
        }

        if (PID_TAG.equals(elName)) {
            try {
                URI asURI = new URI(value);
                PeerID pID = (PeerID) IDFactory.fromURI(asURI);

                setPeerID(pID);
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Invalid PeerID in advertisement: " + value);
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Invalid PeerID: " + value);
            }
            return true;
        }

        if (NAME_TAG.equals(elName)) {
            setName(value);
            return true;
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean addDocumentElements(StructuredDocument adv) {

        Element e;
        // peer ID is optional. (at least for the PlatformConfig it is)
        PeerID peerID = getPeerID();

        if ((null != peerID) && !ID.nullID.equals(peerID)) {
            e = adv.createElement(PID_TAG, peerID.toString());
            adv.appendChild(e);
        }

        // name is optional
        if (getName() != null) {
            e = adv.createElement(NAME_TAG, getName());
            adv.appendChild(e);
        }

        // desc is optional
        StructuredDocument desc = getDesc();

        if (desc != null) {
            StructuredDocumentUtils.copyElements(adv, adv, desc);
        }
        
        super.addDocumentElements(adv);

        return true;
    }
}
