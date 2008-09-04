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

import java.io.IOException;
import java.lang.reflect.UndeclaredThrowableException;

import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentFactory.Instantiator.ExtensionMapping;
import net.jxta.document.XMLDocument;
import net.jxta.impl.document.LiteXMLElement.charRange;
import net.jxta.impl.document.LiteXMLElement.tagRange;


/**
 * This class is an implementation of the StructuredDocument interface using
 * a simplified XML implementation.
 */
public class LiteXMLDocument extends LiteXMLElement implements XMLDocument<LiteXMLElement> {

    /**
     * {@inheritDoc}
     */
    private final static class Instantiator implements StructuredDocumentFactory.TextInstantiator {

        // "x-" is a mime-type convention for indicating partial or provisional
        // compliance to a standard
        private static final MimeMediaType[] myTypes = {
            MimeMediaType.XML_DEFAULTENCODING, 
            MimeMediaType.valueOf("Text/x-Xml"), 
            MimeMediaType.valueOf("Application/Xml"),
            MimeMediaType.valueOf("Application/x-Xml")
        };

        // these are the file extensions which are likely to contain files of
        // the type I like.
        private static final ExtensionMapping[] myExtensions = {
            new ExtensionMapping("xml", myTypes[0]), 
            new ExtensionMapping("xml", (MimeMediaType) null) 
        };

        /**
         * Creates new LiteXMLDocumentInstantiator
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
            return new LiteXMLDocument(mimeType, doctype);
        }

        /**
         * {@inheritDoc}
         */
        public StructuredDocument newInstance(MimeMediaType mimeType, String doctype, String value) {
            return new LiteXMLDocument(mimeType, doctype, value);
        }

        /**
         * {@inheritDoc}
         */
        public StructuredDocument newInstance(MimeMediaType mimeType, InputStream source) throws IOException {
            return new LiteXMLDocument(mimeType, source);
        }

        /**
         * {@inheritDoc}
         */
        public StructuredDocument newInstance(MimeMediaType mimeType, Reader source) throws IOException {
            return new LiteXMLDocument(mimeType, source);
        }
    }

    /**
     * The instantiator for instances of our documents.
     */
    public static final StructuredDocumentFactory.TextInstantiator INSTANTIATOR = new Instantiator();

    /**
     * The actual document contents.
     */
    final StringBuilder docContent;

    /**
     * The mimetype of this document.
     */
    private final MimeMediaType mimeType;

    /**
     * Creates new LiteXMLDocument
     */
    LiteXMLDocument(MimeMediaType mimeType, String type) {
        this(mimeType, type, "");
    }

    /**
     * Creates new LiteXMLDocument with a textValue in the root element
     */
    LiteXMLDocument(MimeMediaType mimeType, String type, String textValue) {
        super(null, (LiteXMLElement.tagRange) null);

        parent = this;

        this.mimeType = mimeType;

        docContent = new StringBuilder();

        for (int eachChar = type.length() - 1; eachChar >= 0; eachChar--) {
            if (Character.isWhitespace(type.charAt(eachChar))) {
                throw new IllegalArgumentException("Root tag may not contain spaces");
            }
        }

        if (null == textValue) {
            textValue = "";
        }

        StringBuilder seedDoc = new StringBuilder(textValue.length() + 3 * type.length() + 128);

        seedDoc.append("<?xml version=\"1.0\"");

        String charset = mimeType.getParameter("charset");

        if (charset != null) {
            seedDoc.append(" encoding=\"");
            seedDoc.append(charset);
            seedDoc.append("\"");
        }
        seedDoc.append("?>\n");

        seedDoc.append("<!DOCTYPE ");
        seedDoc.append(type);
        seedDoc.append(">\n");

        seedDoc.append('<');
        seedDoc.append(type);
        seedDoc.append('>');

        seedDoc.append(textValue);

        seedDoc.append("</");
        seedDoc.append(type);
        seedDoc.append('>');

        try {
            init(new StringReader(seedDoc.toString()));
        } catch (IOException caught) {
            throw new UndeclaredThrowableException(caught);
        }
    }

    /**
     * Creates new LiteXMLDocument
     */
    LiteXMLDocument(MimeMediaType mimeType, InputStream in) throws IOException {
        super(null, (LiteXMLElement.tagRange) null);

        parent = this;

        this.mimeType = mimeType;

        docContent = new StringBuilder();

        String charset = mimeType.getParameter("charset");

        if (charset == null) {
            init(new InputStreamReader(in));
        } else {
            init(new InputStreamReader(in, charset));
        }
    }

    /**
     * Creates new LiteXMLDocument
     */
    LiteXMLDocument(MimeMediaType mimeType, Reader in) throws IOException {
        super(null, (LiteXMLElement.tagRange) null);

        parent = this;

        this.mimeType = mimeType;

        docContent = new StringBuilder();

        init(in);
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
        } catch (IOException caught) {
            throw new UndeclaredThrowableException(caught);
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
        return TextDocumentCommon.Utils.getExtensionForMime(INSTANTIATOR.getSupportedFileExtensions(), getMimeType());
    }

    /**
     * {@inheritDoc}
     */
    public LiteXMLElement createElement(Object key) {
        return createElement(key, null);
    }

    /**
     * {@inheritDoc}
     */
    public LiteXMLElement createElement(Object key, Object val) {
        if (!(key instanceof String)) {
            throw new ClassCastException(key.getClass().getName() + " not supported by createElement as key.");
        }

        if ((null != val) && !(val instanceof String)) {
            throw new ClassCastException(val.getClass().getName() + " not supported by createElement as value.");
        }

        return createElement((String) key, (String) val);
    }

    /**
     * {@inheritDoc}
     */
    public LiteXMLElement createElement(String name) {
        return createElement(name, (String) null);
    }

    /**
     * {@inheritDoc}
     */
    public LiteXMLElement createElement(String name, String val) {
        return new LiteXMLElement(this, name, val);
    }

    /**
     * Create a new text element as a sub-range of this document.
     *
     * @param loc The document range for the new element.
     * @return The newly created element.
     */
    protected LiteXMLElement createElement(tagRange loc) {
        return new LiteXMLElement(this, loc);
    }

    /**
     * {@inheritDoc}
     */
    public Reader getReader() {
        return new StringReader(toString());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LiteXMLDocument getRoot() {
        return this;
    }

    /**
     * {@inheritDoc}
     */
    public InputStream getStream() throws IOException {
        String charset = mimeType.getParameter("charset");

        if (charset == null) {
            return new ByteArrayInputStream(toString().getBytes());
        } else {
            return new ByteArrayInputStream(toString().getBytes(charset));
        }
    }

    /**
     * {@inheritDoc}
     */
    public void sendToWriter(Writer writer) throws IOException {
        String charset = mimeType.getParameter("charset");

        if (charset == null) {
            writer.write("<?xml version=\"1.0\"?>\n");
        } else {
            writer.write("<?xml version=\"1.0\" encoding=\"" + charset + "\"?>\n");
        }

        tagRange result = getDocType(docContent, true);

        if (result.isValid()) {
            writer.write(docContent.substring(result.startTag.start, result.startTag.end + 1));
            writer.write('\n');
        }

        printNice(writer, 0, true);
    }

    /**
     * {@inheritDoc}
     */
    public void sendToStream(OutputStream stream) throws IOException {
        String charset = mimeType.getParameter("charset");

        Writer osw;

        if (charset == null) {
            osw = new OutputStreamWriter(stream);
        } else {
            osw = new OutputStreamWriter(stream, charset);
        }

        Writer out = new BufferedWriter(osw);

        sendToWriter(out);
        out.flush();
    }

    /**
     * Initialises LiteXMLDocument.
     */
    protected void init(Reader in) throws IOException {
        loc = new tagRange();

        char[] smallBuffer = new char[512];

        do {
            int readCount = in.read(smallBuffer);

            if (readCount < 0) {
                break;
            }

            if (readCount > 0) {
                docContent.append(smallBuffer, 0, readCount);
            }

        } while (true);

        // startTag will contain the xml declaration
        loc.startTag.start = 0;
        loc.startTag.end = docContent.indexOf(">");

        // body is everything after the xml declaration
        loc.body.start = loc.startTag.end + 1;
        loc.body.end = docContent.length() - 1;

        // end is the end of the doc.
        loc.endTag.start = loc.body.end;
        loc.endTag.end = loc.body.end;

        tagRange docType = getDocType(getDocument().docContent, false);

        if (docType.isValid()) {
            loc = getTagRanges(getDocument().docContent, docContent.substring(docType.body.start, docType.body.end + 1)
                    ,
                    docType.endTag);
        } else {
            loc = getTagRanges(getDocument().docContent, null, loc.body);
        }

        if (!loc.isValid()) {
            throw new RuntimeException("Parsing error in source document.");
        }

        if (!loc.startTag.equals(loc.endTag)) {
            addChildTags(loc.body, this); // now add the subtags
        }

        if (paranoidConsistencyChecking) {
            checkConsistency();
        }
    }

    protected tagRange getDocType(final StringBuilder source, boolean wholeElement) {
        final String xmldoctype = "!DOCTYPE";
        int start = 0;
        int end = getDocument().docContent.length() - 1;
        tagRange ranges = getTagRanges(source, xmldoctype, new charRange(start, end));

        if (!ranges.startTag.isValid()) {
            return ranges;
        }

        // the rest of the document will be the "end"
        ranges.endTag.start = ranges.body.start;
        ranges.endTag.end = ranges.body.end;

        if (wholeElement) {
            // this will be an empty element
            ranges.body.start = ranges.startTag.end + 1;
            ranges.body.end = ranges.endTag.start - 1;
        } else {
            ranges.body.start = ranges.startTag.start + 1 + xmldoctype.length() - 1 + 1;
            ranges.startTag.end = ranges.body.start - 1;

            while ((ranges.body.start < end) && // immediately followed by a delimiter or the end of the tag
                    Character.isWhitespace(source.charAt(ranges.body.start))) {
                ranges.body.start++;
            }

            ranges.body.end = ranges.body.start;

            while ((ranges.body.end + 1) < end) { // immediately followed by a delimiter or the end of the tag
                char possibleEnd = source.charAt(ranges.body.end + 1);

                if (Character.isWhitespace(possibleEnd) || ('/' == possibleEnd) || ('>' == possibleEnd)) {
                    break;
                }
                ranges.body.end++;
            }
        }

        return ranges;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    LiteXMLDocument getDocument() {
        return this;
    }
}
