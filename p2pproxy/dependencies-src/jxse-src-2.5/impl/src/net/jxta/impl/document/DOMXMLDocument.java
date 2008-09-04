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

package net.jxta.impl.document;


import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Reader;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.Writer;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import java.io.IOException;
import java.lang.reflect.UndeclaredThrowableException;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Document;
import org.w3c.dom.DocumentType;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import org.w3c.dom.ls.DOMImplementationLS;
import org.w3c.dom.ls.LSSerializer;
import org.w3c.dom.ls.LSOutput;

import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentFactory.Instantiator.ExtensionMapping;
import net.jxta.document.XMLDocument;

import net.jxta.impl.document.TextDocumentCommon.Utils;


/**
 * This class is an implementation of the StructuredDocument interface using
 * DOM
 *
 * @see <a href="http://www.w3.org/DOM/">W3C Document Object Model (DOM)</a>
 * @see <a href="http://www.w3.org/TR/DOM-Level-2-Core/java-binding.html">DOM Java Language Binding</a>
 * @see <a href="http://www.w3.org/TR/2004/REC-DOM-Level-3-LS-20040407">Document Object Model (DOM) Level 3 Load and Save Specification</a>
 * @see <a href="http://java.sun.com/xml/jaxp/index.html">Java API for XML Processing (JAXP)</a>
 * @see org.w3c.dom
 */
public class DOMXMLDocument extends DOMXMLElement implements XMLDocument<DOMXMLElement> {

    private static final class Instantiator implements StructuredDocumentFactory.TextInstantiator {

        /**
         * The MIME Media Types which this <CODE>StructuredDocument</CODE> is
         * capable of emitting.
         */
        private static final MimeMediaType[] myTypes = {
            MimeMediaType.XML_DEFAULTENCODING, 
            new MimeMediaType("Application", "Xml")
        };

        /**
         * these are the file extensions which are likely to contain files of
         * the type i like.
         */
        private static final ExtensionMapping[] myExtensions = {
            new ExtensionMapping("xml", myTypes[0]), 
            new ExtensionMapping("xml", null)
        };

        /**
         * Creates new XMLDocumentInstantiator
         */
        public Instantiator() {}

        /**
         * {@inheritDoc}
         */
        public MimeMediaType[] getSupportedMimeTypes() {
            return (myTypes);
        }

        /**
         * {@inheritDoc}
         */
        public ExtensionMapping[] getSupportedFileExtensions() {
            return (myExtensions);
        }

        /**
         * {@inheritDoc}
         */
        public StructuredDocument newInstance(MimeMediaType mimeType, String doctype) {
            return new DOMXMLDocument(mimeType, doctype);
        }

        /**
         * {@inheritDoc}
         */
        public StructuredDocument newInstance(MimeMediaType mimeType, String doctype, String value) {
            return new DOMXMLDocument(mimeType, doctype, value);
        }

        /**
         * {@inheritDoc}
         */
        public StructuredDocument newInstance(MimeMediaType mimeType, InputStream source) throws IOException {
            return new DOMXMLDocument(mimeType, source);
        }

        /**
         * {@inheritDoc}
         */
        public StructuredDocument newInstance(MimeMediaType mimeType, Reader source) throws IOException {
            return new DOMXMLDocument(mimeType, source);
        }

    }

    public static final StructuredDocumentFactory.TextInstantiator INSTANTIATOR = new Instantiator();

    private MimeMediaType mimeType;

    /**
     * Constructor for new instances of <CODE>DOMXMLDocument</CODE>
     * with a value for the root element.
     *
     * @param mimeType This is the MIME Media Type being requested. In general
     *                 it should be equivalent with the MIME Media Type returned by
     *                 {@link #getMimeType()}. A <CODE>StructuredDocument</CODE>
     *                 sub-class may, however, support more than one MIME Media type so this may be a
     *                 true parameter. XMLDocument supports additional the MIME Media Type parameters,
     *                 "charset". The charset parameter must be a value per IETF RFC 2279 or ISO-10646.
     * @param docType  Used as the root type of this document. {@link net.jxta.document.XMLDocument} uses this as the XML
     *                 <CODE>DOCTYPE</CODE>.
     * @param value    String value to be associated with the root element.
     *                 null if none is wanted.
     * @throws RuntimeException Exceptions generated by the underlying implementation.
     */
    private DOMXMLDocument(final MimeMediaType mimeType, final String docType, final String value) {
        super(null, null);
        this.root = this;

        this.mimeType = mimeType;

        DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();

        docBuilderFactory.setNamespaceAware(true);
        docBuilderFactory.setValidating(false);
        try {
            DocumentBuilder dataDocBuilder = docBuilderFactory.newDocumentBuilder();
            DOMImplementation domImpl = dataDocBuilder.getDOMImplementation();
            DocumentType doctypeNode = domImpl.createDocumentType(docType, null, null);

            domNode = domImpl.createDocument("http://jxta.org", docType, doctypeNode);
        } catch (ParserConfigurationException misconfig) {
            throw new UndeclaredThrowableException(misconfig);
        }

        if (value != null) {
            Node text = ((Document) domNode).createTextNode(value);

            ((Document) domNode).getDocumentElement().appendChild(text);
        }
    }

    /**
     * Constructor for new instances of <CODE>DOMXMLDocument</CODE>
     *
     * @param mimeType This is the MIME Media Type being requested. In general it should be equivalent with
     *                 the MIME Media Type returned by {@link #getMimeType()}. A <CODE>StructuredDocument</CODE>
     *                 sub-class may, however, support more than one MIME Media type so this may be a
     *                 true parameter. XMLDocument supports additional the MIME Media Type parameters,
     *                 "charset". The charset parameter must be a value per IETF RFC 2279 or ISO-10646.
     * @param docType  Used as the root type of this document. {@link net.jxta.document.XMLDocument} uses this as the XML
     *                 <CODE>DOCTYPE</CODE>.
     * @throws RuntimeException Exceptions generated by the underlying implementation.
     */
    private DOMXMLDocument(final MimeMediaType mimeType, final String docType) {
        this(mimeType, docType, null);
    }

    /**
     * Constructor for existing documents.
     *
     * @param mimeType This is the MIME Media Type being requested. In general
     *                 it should be equivalent with the MIME Media Type returned by
     *                 {@link #getMimeType()}. A <CODE>StructuredDocument</CODE> sub-class may,
     *                 however, support more than one MIME Media type so this may be a
     *                 true parameter. XMLDocument supports additional the MIME Media Type parameters,
     *                 "charset". The charset parameter must be a value per IETF RFC 2279 or ISO-10646.
     * @param stream   Contains the input used to construct this object. This stream should be of a type
     *                 conformant with the type specified by the MIME Media Type "charset" parameter.
     * @throws RuntimeException    Propagates exceptions from the underlying implementation.
     * @throws java.io.IOException Thrown for failures processing the document.
     */
    private DOMXMLDocument(final MimeMediaType mimeType, final InputStream stream) throws IOException {
        super(null, null);
        this.root = this;

        this.mimeType = mimeType;

        String charset = mimeType.getParameter("charset");

        Reader source;

        if (charset == null) {
            source = new InputStreamReader(stream);
        } else {
            source = new InputStreamReader(stream, charset);
        }

        DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();

        docBuilderFactory.setNamespaceAware(true);
        docBuilderFactory.setValidating(false);
        try {
            DocumentBuilder dataDocBuilder = docBuilderFactory.newDocumentBuilder();

            domNode = dataDocBuilder.parse(new InputSource(source));
        } catch (ParserConfigurationException misconfig) {
            throw new UndeclaredThrowableException(misconfig);
        } catch (SAXException parseError) {
            throw new IOException(parseError.toString());
        }
    }

    /**
     * Constructor for existing documents.
     *
     * @param mimeType This is the MIME Media Type being requested. In general
     *                 it should be equivalent with the MIME Media Type returned by
     *                 {@link #getMimeType()}. A <CODE>StructuredDocument</CODE> sub-class may,
     *                 however, support more than one MIME Media type so this may be a
     *                 true parameter. XMLDocument supports additional the MIME Media Type parameters,
     *                 "charset". The charset parameter must be a value per IETF RFC 2279 or ISO-10646.
     * @param source   Contains the input used to construct this object. This reader
     *                 should be of a type conformant with the type specified by the MIME Media Type
     *                 "charset" parameter.
     * @throws RuntimeException Propagates exceptions from the underlying implementation.
     */
    private DOMXMLDocument(final MimeMediaType mimeType, final Reader source) throws IOException {
        super(null, null);
        this.root = this;

        this.mimeType = mimeType;

        DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();

        docBuilderFactory.setNamespaceAware(true);
        docBuilderFactory.setValidating(false);
        try {
            DocumentBuilder dataDocBuilder = docBuilderFactory.newDocumentBuilder();

            domNode = dataDocBuilder.parse(new InputSource(source));
        } catch (ParserConfigurationException misconfig) {
            throw new UndeclaredThrowableException(misconfig);
        } catch (SAXException parseError) {
            throw new IOException(parseError.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        try {
            StringWriter stringOut = new StringWriter();

            sendToWriter(stringOut);
            stringOut.close();

            return stringOut.toString();
        } catch (IOException ex) {
            throw new UndeclaredThrowableException(ex);
        }
    }

    /**
     * {@inheritDoc}
     */
    public MimeMediaType getMimeType() {
        return mimeType;
    }

    /**
     * {@inheritDoc}
     */
    public String getFileExtension() {
        return Utils.getExtensionForMime(INSTANTIATOR.getSupportedFileExtensions(), getMimeType());
    }

    /**
     * {@inheritDoc}
     */
    public InputStream getStream() throws IOException {
        String result = toString();

        if (null == result) {
            return null;
        }

        String charset = mimeType.getParameter("charset");

        if (charset == null) {
            return new ByteArrayInputStream(result.getBytes());
        } else {
            return new ByteArrayInputStream(result.getBytes(charset));
        }
    }

    /**
     * {@inheritDoc}
     */
    public void sendToStream(OutputStream stream) throws IOException {
        Writer osw;
        String charset = mimeType.getParameter("charset");

        if (charset == null) {
            osw = new OutputStreamWriter(stream);
        } else {
            osw = new OutputStreamWriter(stream, charset);
        }

        Writer out = new BufferedWriter(osw);

        sendToWriter(out);
        out.flush();
        osw.flush();
    }

    /**
     * {@inheritDoc}
     */
    public Reader getReader() {
        String result = toString();

        if (null == result) {
            return null;
        }

        return new StringReader(result);
    }

    /**
     * {@inheritDoc}
     */
    public void sendToWriter(Writer writer) throws IOException {
        String charset = mimeType.getParameter("charset");

        try {
            DOMImplementationLS domImpl = (DOMImplementationLS) ((Document) domNode).getImplementation().getFeature("LS", "3.0");
            LSOutput output = domImpl.createLSOutput();

            if (charset != null) {
                output.setEncoding(charset);
            } else {
                output.setEncoding(java.nio.charset.Charset.defaultCharset().name());
            }

            output.setCharacterStream(writer);

            LSSerializer serial = domImpl.createLSSerializer();

            serial.write(domNode, output);
        } catch (Throwable ex) {
            if (ex instanceof RuntimeException) {
                throw (RuntimeException) ex;
            } else if (ex instanceof Error) {
                throw (Error) ex;
            } else {
                throw new UndeclaredThrowableException(ex);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public DOMXMLElement createElement(Object key) {
        return createElement(key, null);
    }

    /**
     * {@inheritDoc}
     */
    public DOMXMLElement createElement(Object key, Object val) {
        if (!String.class.isAssignableFrom(key.getClass())) {
            throw new ClassCastException(key.getClass().getName() + " not supported by createElement.");
        }

        if ((null != val) && !String.class.isAssignableFrom(val.getClass())) {
            throw new ClassCastException(val.getClass().getName() + " not supported by createElement.");
        }

        return createElement((String) key, (String) val);
    }

    // StructuredDocument Methods

    /**
     * {@inheritDoc}
     */
    public DOMXMLElement createElement(String name) {
        return new DOMXMLElement(this, ((Document) domNode).createElement(name));
    }

    /**
     * {@inheritDoc}
     */
    public DOMXMLElement createElement(String name, String value) {
        Element root;

        root = ((Document) domNode).createElement(name);
        if (null != value) {
            root.appendChild(((Document) domNode).createTextNode(value));
        }
        return new DOMXMLElement(this, root);
    }

    // Element Methods

    // Protected & Private Methods

    @Override
    protected Node getAssocNode() {
        return ((Document) domNode).getDocumentElement();
    }
}
