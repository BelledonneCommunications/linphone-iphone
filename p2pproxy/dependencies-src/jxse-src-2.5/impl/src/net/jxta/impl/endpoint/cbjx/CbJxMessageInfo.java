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

package net.jxta.impl.endpoint.cbjx;


import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.membership.pse.PSEUtils;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.util.Enumeration;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * this class defines the xml document used to store message information
 * that is useful for the cbjx endpoint
 */
public class CbJxMessageInfo {

    /**
     * Logger
     */
    private static final transient Logger LOG = Logger.getLogger(CbJxMessageInfo.class.getName());

    /**
     * the root element for the information's XML document
     */
    private static final String DOCTYPE = "CbJxMessageInfo";

    /**
     * the element name for the peer certificate
     */
    private static final String tagPeerCert = "PeerCert";

    /**
     * the element name of the destination address
     */
    private static final String tagDestination = "DestinationAddress";

    /**
     * the element name of the source address
     */
    private static final String tagSource = "SourceAddress";

    /**
     * the element name for the sourceID of the message
     */
    private static final String tagSourceID = "SourceID";

    /**
     * the peer root cert
     */
    private Certificate rootCert = null;

    /**
     * the destination address of the message
     */
    private EndpointAddress destinationAddress = null;

    /**
     * the source address of the message
     */
    private EndpointAddress sourceAddress = null;

    /**
     * the source ID of the sender
     */
    private ID sourceID = null;

    /**
     * creates a new information object
     */
    public CbJxMessageInfo() {
        super();
    }

    /**
     * creates a new Message information by parsing the given stream
     *
     * @param stream the InputStream  source of the info data
     * @throws IOException if the info can't be parsed from the stream
     */
    public CbJxMessageInfo(InputStream stream, MimeMediaType type) throws IOException {
        super();

        XMLDocument document = (XMLDocument)
                StructuredDocumentFactory.newStructuredDocument(type, stream);

        initialize(document);
    }

    /**
     * retrieve the peer certificate
     *
     * @return Certificate the peer certificate
     */
    public Certificate getPeerCert() {
        return rootCert;
    }

    /**
     * set the peer certificate
     *
     * @param cert the peer certificate
     */
    public void setPeerCert(Certificate cert) {
        rootCert = cert;
    }

    /**
     * retrieve the destination address of the message
     *
     * @return String the original destination address of the message
     */
    public EndpointAddress getDestinationAddress() {
        return destinationAddress;
    }

    /**
     * set the destination address of the message
     *
     * @param addr the destination address
     */
    public void setDestinationAddress(EndpointAddress addr) {
        destinationAddress = addr;
    }

    /**
     * retrieve the original source address of the message
     *
     * @return String the original source address of the message
     */
    public EndpointAddress getSourceAddress() {
        return sourceAddress;
    }

    /**
     * set the source address of the messsage
     *
     * @param addr the source address
     */
    public void setSourceAddress(EndpointAddress addr) {
        sourceAddress = addr;
    }

    /**
     * retrieve the source id of the message
     *
     * @return String the source id of the sender
     */
    public ID getSourceID() {
        return sourceID;
    }

    /**
     * set the source id of the message
     *
     * @param src the ID of the sender
     */
    public void setSourceID(ID src) {
        sourceID = src;
    }

    /**
     * returns a Document containing the information's document tree
     *
     * @param asMimeType the desired MIME type for the information rendering
     * @return Document the Document containing the information's document tree
     */
    public Document getDocument(MimeMediaType asMimeType) {
        StructuredDocument document = StructuredDocumentFactory.newStructuredDocument(asMimeType, DOCTYPE);

        Element element;

        if (getPeerCert() == null) {
            throw new IllegalStateException("Missing Peer Root Certificate");
        }

        try {
            String base64cert = PSEUtils.base64Encode(getPeerCert().getEncoded());

            element = document.createElement(tagPeerCert, base64cert);
            document.appendChild(element);
        } catch (Exception e) {
            throw new IllegalStateException("Bad root cert! " + e);
        }

        if (getSourceID() == null) {
            throw new IllegalStateException("Missing Source ID");
        }

        element = document.createElement(tagSourceID, getSourceID().toString());
        document.appendChild(element);

        if (getDestinationAddress() == null) {
            throw new IllegalStateException("Missing Destination Address");
        }

        element = document.createElement(tagDestination, getDestinationAddress().toString());
        document.appendChild(element);

        if (getSourceAddress() == null) {
            throw new IllegalStateException("Missing Source Address");
        }

        element = document.createElement(tagSource, getSourceAddress().toString());
        document.appendChild(element);

        return document;
    }

    /**
     * Called to handle elements during parsing.
     *
     * @param elem Element to parse
     * @return true if element was handled, otherwise false.
     */
    protected boolean handleElement(XMLElement elem) {
        if (elem.getName().equals(tagPeerCert)) {
            try {
                byte[] cert_der = PSEUtils.base64Decode(new StringReader(elem.getTextValue()));

                CertificateFactory cf = CertificateFactory.getInstance("X.509");

                Certificate cert = cf.generateCertificate(new ByteArrayInputStream(cert_der));

                setPeerCert(cert);
            } catch (Exception e) {
                throw new IllegalArgumentException("Bad X509 Cert!");
            }
            return true;
        }

        if (elem.getName().equals(tagDestination)) {
            EndpointAddress destination = new EndpointAddress(elem.getTextValue());

            setDestinationAddress(destination);
            return true;
        }

        if (elem.getName().equals(tagSource)) {
            EndpointAddress source = new EndpointAddress(elem.getTextValue());

            setSourceAddress(source);
            return true;
        }

        if (elem.getName().equals(tagSourceID)) {

            try {
                URI sourcePeerURL = new URI(elem.getTextValue().trim());
                PeerID sourcePeerID = (PeerID) IDFactory.fromURI(sourcePeerURL);

                setSourceID(sourcePeerID);
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad PeerGroupID in advertisement: " + elem.getTextValue());
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Id is not a peer id: " + elem.getTextValue());
            }
            return true;
        }

        return false;
    }

    /**
     * internal method to process a document into an advertisement.
     *
     * @param root where to start.
     */
    protected void initialize(Element root) {

        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;

        String doctype = doc.getName();

        if (!doctype.equals(DOCTYPE)) {
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

        // sanity checking time.

        if (null == rootCert) {
            throw new IllegalArgumentException("Document did not contain a root cert element");
        }

        if (null == destinationAddress) {
            throw new IllegalArgumentException("Document did not contain a destination address element");
        }

        if (null == sourceAddress) {
            throw new IllegalArgumentException("Document did not contain a source address element");
        }

        if (null == sourceID) {
            throw new IllegalArgumentException("Document did not contain a source ID element");
        }
    }
}
