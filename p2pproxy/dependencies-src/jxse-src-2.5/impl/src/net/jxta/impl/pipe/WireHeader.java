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
package net.jxta.impl.pipe;

import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
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
 * This class implements a JXTA-WIRE header.
 */
public class WireHeader {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(WireHeader.class.getName());

    private static final String Name = "JxtaWire";
    private static final String MsgIdTag = "MsgId";
    private static final String PipeIdTag = "PipeId";
    private static final String SrcTag = "SrcPeer";
    private static final String TTLTag = "TTL";
    private static final String PeerTag = "VisitedPeer";

    private ID srcPeer = ID.nullID;
    private ID pipeID = ID.nullID;
    private String msgId = null;
    private int TTL = Integer.MIN_VALUE;

    public WireHeader() {}

    public WireHeader(Element root) {
        initialize(root);
    }

    public void setSrcPeer(ID p) {
        srcPeer = p;
    }

    public ID getSrcPeer() {
        return srcPeer;
    }

    public void setTTL(int t) {
        TTL = t;
    }

    public int getTTL() {
        return TTL;
    }

    public String getMsgId() {
        return msgId;
    }

    public void setMsgId(String id) {
        this.msgId = id;
    }

    public ID getPipeID() {
        return pipeID;
    }

    public void setPipeID(ID id) {
        this.pipeID = id;
    }

    /**
     * Called to handle elements during parsing.
     *
     * @param elem Element to parse
     * @return true if element was handled, otherwise false.
     */
    protected boolean handleElement(XMLElement elem) {
        if (elem.getName().equals(SrcTag)) {
            try {
                URI pID = new URI(elem.getTextValue());
                setSrcPeer(IDFactory.fromURI(pID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad PeerID ID in header: " + elem.getTextValue());
            }
            return true;
        }

        if (elem.getName().equals(MsgIdTag)) {
            msgId = elem.getTextValue();
            return true;
        }

        if (elem.getName().equals(PipeIdTag)) {
            try {
                URI pipeID = new URI(elem.getTextValue());
                setPipeID(IDFactory.fromURI(pipeID));
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad pipe ID in header");
            }
            return true;
        }

        if (elem.getName().equals(TTLTag)) {
            TTL = Integer.parseInt(elem.getTextValue());
            return true;
        }

        // IGNORE obsolete element "VisitedPeer"
        return elem.getName().equals(PeerTag);

    }

    /**
     * internal method to process a document into a header.
     *
     * @param root where to start.
     */
    protected void initialize(Element root) {
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;
        String doctype = doc.getName();
        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(Name) && !Name.equals(typedoctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        Enumeration elements = doc.getChildren();
        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + elem.getName());
                }
            }
        }

        // Sanity Check!

        if (null == getMsgId()) {
            throw new IllegalArgumentException("Header does not contain a message id");
        }

        if (ID.nullID == getPipeID()) {
            throw new IllegalArgumentException("Header does not contain a pipe id");
        }

        if (TTL < 1) {
            throw new IllegalArgumentException("TTL must be >= 1");
        }
    }

    /**
     * Returns the docment for this header
     *
     * @param encodeAs mime type encoding
     * @return the docment for this header
     */
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredTextDocument doc = (StructuredTextDocument)
                StructuredDocumentFactory.newStructuredDocument(encodeAs, Name);

        if (doc instanceof XMLDocument) {
            ((XMLDocument) doc).addAttribute("xmlns:jxta", "http://jxta.org");
        }

        if (null == getMsgId()) {
            throw new IllegalStateException("Message id is not initialized");
        }

        if (ID.nullID == getPipeID()) {
            throw new IllegalStateException("PipeID is not initialized");
        }

        if (TTL < 1) {
            throw new IllegalStateException("TTL must be >= 1");
        }

        Element e;
        if ((srcPeer != null) && (srcPeer != ID.nullID)) {
            e = doc.createElement(SrcTag, srcPeer.toString());
            doc.appendChild(e);
        }

        e = doc.createElement(PipeIdTag, getPipeID().toString());
        doc.appendChild(e);

        e = doc.createElement(MsgIdTag, getMsgId());
        doc.appendChild(e);

        e = doc.createElement(TTLTag, Integer.toString(TTL));
        doc.appendChild(e);

        return doc;
    }
}
