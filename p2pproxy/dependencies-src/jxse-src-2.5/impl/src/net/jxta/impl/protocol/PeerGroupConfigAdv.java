/*
 * Copyright (c) 2004-2007 Sun Microsystems, Inc.  All rights reserved.
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
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * Defines Peer Group Runtime Configuration parameters.
 * <p/>
 * This typically includes the peer group ID to use, the peer group name (if any) to use, and optional descriptive
 * meta-data.
 * <p/>
 * <pre><code>
 *   NetPeerGroupID=uuid-59313231343132314A484431544E504702
 *   PeerGroupName=Network Infrastructure PeerGroup
 *   PeerGroupDesc=Infrastructure Group Description
 * </code></pre>
 */
public final class PeerGroupConfigAdv extends ExtendableAdvertisement implements Cloneable {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(PeerGroupConfigAdv.class.getName());

    /**
     *  The advertisement index fields. (currently none).
     */
    private final static String[] INDEX_FIELDS = {};

    /**
     * The DOCTYPE
     */
    private final static String advType = "jxta:PeerGroupConfigAdv";

    private final static String PEERGROUP_ID_TAG = "PeerGroupID";
    private final static String PEERGROUP_NAME_TAG = "PeerGroupName";
    private final static String PEERGROUP_DESC_TAG = "PeerGroupDesc";

    /**
     * Instantiator for PeerGroupConfigAdv
     */
    public static class Instantiator implements AdvertisementFactory.Instantiator {

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
            return new PeerGroupConfigAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }

            return new PeerGroupConfigAdv((XMLElement) root);
        }
    }

    /**
     * ID for the peer group.
     */
    private ID gid = null;

    /**
     * Informal, non-canonical name of this peer group
     */
    private String name = null;

    /**
     * Descriptive meta-data about this peer group.
     */
    private Element description = null;

    /**
     * Returns the identifying type of this Advertisement.
     * <p/>
     * <b>Note:</b> This is a static method. It cannot be used to determine
     * the runtime type of an advertisement. ie.
     * </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     * <p/>
     * <b>This is wrong and does not work the way you might expect.</b>
     * This call is not polymorphic and calls
     * Advertisement.getAdvertisementType() no matter what the real type of the
     * advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return advType;
    }

    /**
     * Use the Instantiator through the factory
     */
    private PeerGroupConfigAdv() {}

    /**
     * Use the Instantiator method to construct Peer Group Config Advs.
     *
     * @param doc the element
     */
    private PeerGroupConfigAdv(XMLElement doc) {
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

        // Validate group id
        if (null == getPeerGroupID()) {
            throw new IllegalArgumentException("Peer Group Config Advertisement does not contain a peer group id.");
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

        if (PEERGROUP_ID_TAG.equals(elem.getName())) {
            try {
                URI grID = new URI(elem.getTextValue());

                setPeerGroupID(IDFactory.fromURI(grID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Invalid peer group ID in advertisement: " + elem.getTextValue());
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Invalid group ID: " + elem.getTextValue());
            }
            return true;
        }

        if (PEERGROUP_NAME_TAG.equals(elem.getName())) {
            setName(elem.getTextValue());
            return true;
        }

        if (PEERGROUP_DESC_TAG.equals(elem.getName())) {
            setDesc(elem);
            return true;
        }

        return false;
    }

    /**
     * Make a safe clone of this PeerGroupConfigAdv.
     *
     * @return Object A copy of this PeerGroupConfigAdv
     */
    @Override
    public PeerGroupConfigAdv clone() {
        try {
            PeerGroupConfigAdv clone = (PeerGroupConfigAdv) super.clone();
            
            clone.setPeerGroupID(getPeerGroupID());
            clone.setName(getName());
            clone.setDesc(getDesc());
            
            return clone;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
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
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ID getID() {
        return ID.nullID;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        if (null == getPeerGroupID()) {
            throw new IllegalStateException("Peer Group Config Advertisement does not contain a peer group id.");
        }

        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        Element e = adv.createElement(PEERGROUP_ID_TAG, getPeerGroupID().toString());

        adv.appendChild(e);

        // name is optional
        if (null != getName()) {
            e = adv.createElement(PEERGROUP_NAME_TAG, getName());
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
        return INDEX_FIELDS;
    }

    /**
     * Returns the id of the peer group.
     *
     * @return ID the group id
     */

    public ID getPeerGroupID() {
        return gid;
    }

    /**
     * Sets the id of the peer group.
     *
     * @param gid The id of this group.
     */

    public void setPeerGroupID(ID gid) {
        this.gid = gid;
    }

    /**
     * Gets the name to use for the peer group.
     *
     * @return The name value
     */
    public String getName() {
        return name;
    }

    /**
     * Sets the name to use for the peer group.
     *
     * @param name The new name value
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * returns the description
     *
     * @return String the description
     */
    public String getDescription() {
        if (description != null) {
            return (String) description.getValue();
        } else {
            return null;
        }
    }

    /**
     * sets the description
     *
     * @param description the description
     */
    public void setDescription(String description) {

        if (null != description) {
            XMLDocument newdoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8
                    ,
                    PEERGROUP_DESC_TAG, description);

            setDesc(newdoc);
        } else {
            this.description = null;
        }
    }

    /**
     * returns the description
     *
     * @return the description
     */
    public XMLDocument getDesc() {
        XMLDocument newDoc = null;

        if (description != null) {
            newDoc = (XMLDocument) StructuredDocumentUtils.copyAsDocument(description);
        }
        return newDoc;
    }

    /**
     * Sets the description
     *
     * @param desc the description
     */
    public void setDesc(XMLElement desc) {

        if (desc != null) {
            this.description = StructuredDocumentUtils.copyAsDocument(desc);
        } else {
            this.description = null;
        }
    }
}
