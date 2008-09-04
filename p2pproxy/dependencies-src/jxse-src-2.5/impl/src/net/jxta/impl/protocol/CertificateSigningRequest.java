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


import java.io.StringReader;

import java.io.IOException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import org.bouncycastle.jce.PKCS10CertificationRequest;

import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;

import net.jxta.impl.membership.pse.PSEUtils;


/**
 * A lightweight container for a PKCS#10/RFC2986 Certificate Signing Request.
 *
 *  @deprecated The types exported from this implementation are subject to
 *  change.
 *
 * <pre><code>
 *   &lt;xs:element name="jxta:csr" type="CertificateSigningRequest"/>
 *
 *   &lt;xs:complexType name="csr">
 *     &lt;xs:all>
 *     &lt;/xs:all>
 *   &lt;/xs:complexType>
 * </code></pre>
 *
 *  @see <a href="http://http://www.rsasecurity.com/rsalabs/pkcs/pkcs-10/" target='_blank'>PKCS #10</a>
 *  @see <a href="ftp://ftp.isi.edu/in-notes/rfc2986.txt" target='_blank'>IETF RFC 2986</a>
 **/
@Deprecated
public class CertificateSigningRequest {
    
    /**
     *  Logger
     **/
    private final static transient Logger LOG = Logger.getLogger(CertificateSigningRequest.class.getName());
    
    private PKCS10CertificationRequest csr = null;
    
    public CertificateSigningRequest() {
        super();
    }
    
    public CertificateSigningRequest(Element root) {
        this();
        initialize(root);
    }
    
    /**
     *    returns the Message type. This will match the XML doctype declaration.
     *
     *    @return a string
     **/
    public static String getMessageType() {
        return "jxta:CertificateSigningRequest";
    }
    
    public PKCS10CertificationRequest getCSR() {
        return csr;
    }
    
    public void setCSR(PKCS10CertificationRequest csr) {
        this.csr = csr;
    }
    
    /**
     *  Initializes the message from a document.
     **/
    protected void initialize(Element root) {
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
        }
        
        XMLElement doc = (XMLElement) root;
        
        String docName = doc.getName();
        
        if (!docName.equals(getMessageType())) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + docName);
        }
        
        String value = doc.getTextValue();

        value = value.trim();
        
        try {
            byte[] csr_der = PSEUtils.base64Decode(new StringReader(value));

            csr = new PKCS10CertificationRequest(csr_der);
        } catch (IOException error) {
            throw new IllegalArgumentException("bad certificate signing request.");
        }
        
        // Begin checking sanity!
        if (null == csr) {
            throw new IllegalArgumentException("certificate signing request not initialized.");
        }
    }
    
    /**
     *  Creates a document out of the message.
     *
     *  @param  encodeAs  The document representation format requested.
     *  @return the message as a document.
     **/
    public Document getDocument(MimeMediaType encodeAs) {
        
        String encodedCSR;

        try {
            encodedCSR = PSEUtils.base64Encode(csr.getEncoded());
        } catch (IOException failed) {
            IllegalStateException failure = new IllegalStateException("Could not encode certificate signing request.");

            failure.initCause(failed);
            
            throw failure;
        }
        
        StructuredTextDocument doc = (StructuredTextDocument) StructuredDocumentFactory.newStructuredDocument(encodeAs
                ,
                getMessageType(), encodedCSR);
        
        if (doc instanceof XMLDocument) {
            ((XMLDocument) doc).addAttribute("xmlns:jxta", "http://jxta.org");
            ((XMLDocument) doc).addAttribute("xml:space", "preserve");
        }
        
        return doc;
    }
}
