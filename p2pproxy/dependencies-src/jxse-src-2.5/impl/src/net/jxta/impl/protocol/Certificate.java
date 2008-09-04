/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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
import net.jxta.impl.membership.pse.PSEUtils;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.StringReader;
import java.security.cert.CertificateEncodingException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.*;


/**
 * A lightweight container for X.509 Certificates.
 * *
 * <pre><code>
 *   &lt;xs:element name="jxta:cert" type="Certificate"/>
 * <p/>
 *   &lt;xs:complexType name="Certificate" type="xs:string">
 *      &lt;xs:element name="Issuer" type="jxta:cert" minOccurs="0" />
 *   &lt;/xs:complexType>
 * </code></pre>
 */
public class Certificate {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(Certificate.class.getName());

    /**
     *
     **/
    private List<X509Certificate> certs = null;

    /**
     **/
    public Certificate() {
        super();
    }

    public Certificate(Element root) {
        this();
        certs = new ArrayList<X509Certificate>();
        initialize(root);
    }

    /**
     * returns the Message type. This will match the XML doctype declaration.
     *
     * @return a string
     */
    public static String getMessageType() {
        return "jxta:cert";
    }

    public X509Certificate[] getCertificates() {
        return certs.toArray(new X509Certificate[certs.size()]);
    }

    public void setCertificates(X509Certificate[] certs) {
        this.certs = new ArrayList<X509Certificate>(Arrays.asList(certs));
    }

    public void setCertificates(List<X509Certificate> certs) {
        this.certs = new ArrayList<X509Certificate>(certs);
    }

    /**
     * Initializes the message from a document.
     *
     * @param root the element
     */
    private void initialize(Element root) {

        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
        }

        XMLElement doc = (XMLElement) root;

        String doctype = doc.getName();

        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(getMessageType()) && !getMessageType().equals(typedoctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        String value = doc.getTextValue();

        value = value.trim();

        try {
            byte[] cert_der = PSEUtils.base64Decode(new StringReader(value));

            CertificateFactory cf = CertificateFactory.getInstance("X.509");

            certs.add((X509Certificate) cf.generateCertificate(new ByteArrayInputStream(cert_der)));
        } catch (IOException error) {
            throw new IllegalArgumentException("bad certificate.");
        } catch (CertificateException error) {
            throw new IllegalArgumentException("bad certificate.");
        }

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            Element elem = (Element) elements.nextElement();

            if (!elem.getKey().equals("Issuer")) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + elem.toString());
                }
                continue;
            }

            Certificate issuer = new Certificate(elem);

            certs.addAll(Arrays.asList(issuer.getCertificates()));
        }

        // Begin checking sanity!
        if (certs.isEmpty()) {
            throw new IllegalArgumentException("certificate not initialized.");
        }
    }

    /**
     * Creates a document out of the message.
     *
     * @param encodeAs The document representation format requested.
     * @return the message as a document.
     */
    public Document getDocument(MimeMediaType encodeAs) {
        String encodedCert;

        try {
            encodedCert = PSEUtils.base64Encode((certs.get(0)).getEncoded());
        } catch (CertificateEncodingException failed) {
            IllegalStateException failure = new IllegalStateException("bad certificate.");

            failure.initCause(failed);

            throw failure;
        } catch (IOException failed) {
            IllegalStateException failure = new IllegalStateException("Could not encode certificate.");

            failure.initCause(failed);

            throw failure;
        }

        StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(encodeAs, getMessageType(), encodedCert);

        if (doc instanceof XMLDocument) {
            ((XMLDocument) doc).addAttribute("xmlns:jxta", "http://jxta.org");
            ((XMLDocument) doc).addAttribute("xml:space", "preserve");
        }

        Iterator<X509Certificate> eachCert = certs.iterator();

        eachCert.next(); // skip me.

        Element addTo = doc;

        while (eachCert.hasNext()) {
            X509Certificate anIssuer = eachCert.next();

            try {
                encodedCert = PSEUtils.base64Encode(anIssuer.getEncoded());
            } catch (CertificateEncodingException failed) {
                IllegalStateException failure = new IllegalStateException("bad certificate.");

                failure.initCause(failed);

                throw failure;
            } catch (IOException failed) {
                IllegalStateException failure = new IllegalStateException("Could not encode certificate.");

                failure.initCause(failed);

                throw failure;
            }

            Element issuerElement = doc.createElement("Issuer", encodedCert);

            addTo.appendChild(issuerElement);

            if (doc instanceof Attributable) {
                ((Attributable) issuerElement).addAttribute("type", getMessageType());
            }

            addTo = issuerElement;
        }

        return doc;
    }
}
