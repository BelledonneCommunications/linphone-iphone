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

package net.jxta.impl.endpoint.msgframing;


import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.text.MessageFormat;
import java.util.ListIterator;

import net.jxta.document.MimeMediaType;


/**
 * Header Package for Messages. Analogous to HTTP Headers.
 */
public class MessagePackageHeader {
    
    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(MessagePackageHeader.class.getName());
    
    /**
     * Standard header name for content-length
     */
    private final static String CONTENT_LENGTH = "content-length";
    
    /**
     * Standard header name for content-type
     */
    private final static String CONTENT_TYPE = "content-type";
    
    /**
     * The maximum size of Header data buffers we will emit.
     */
    private final static int MAX_HEADER_LEN = 1024;
    
    /**
     * Used for storing individual header elements.
     */
    public static class Header {
        
        final String  name;
        final byte[] value;
        
        public Header(String name, byte[] value) {
            this.name = name;
            
            assert value.length <= 65535;
            
            this.value = value;
        }
        
        /**
         * {@inheritDoc}
         */
        @Override
        public String toString() {
            return MessageFormat.format("{0} := {1}", name, value);
        }
        
        public String getName() {
            return name;
        }
        
        public byte[] getValue() {
            return value;
        }
        
        public String getValueString() {
            try {
                return new String(value, "UTF-8");
            } catch (UnsupportedEncodingException never) {
                // utf-8 is a required encoding.
                throw new Error("UTF-8 encoding support missing!");
            }
        }
    }
    
    /**
     * The individual header elements in the order they were read.
     */
    private final List<Header> headers = new ArrayList<Header>();
    
    /**
     * Creates a new instance of MessagePackageHeader. Used for outgoing messages.
     */
    public MessagePackageHeader() {}
    
    /**
     * Creates a new instance of MessagePackageHeader. Used for incoming messages.
     *
     * @param in The stream from which the headers will be read.
     * @throws java.io.IOException if an io error occurs.
     */
    public MessagePackageHeader(InputStream in) throws IOException {
        boolean sawEmpty = false;
        boolean sawLength = false;
        boolean sawType = false;
        DataInput di = new DataInputStream(in);
        
        // todo 20021014 bondolo@jxta.org A framing signature would help here.
        
        do {
            byte headerNameLength = di.readByte();
            
            if (0 == headerNameLength) {
                sawEmpty = true;
            } else {
                byte[] headerNameBytes = new byte[headerNameLength];
                
                di.readFully(headerNameBytes);
                
                String headerNameString = new String(headerNameBytes, "UTF-8");
                
                if (headerNameString.equalsIgnoreCase(CONTENT_LENGTH)) {
                    if (sawLength) {
                        throw new IOException("Duplicate content-length header");
                    }
                    sawLength = true;
                }
                
                if (headerNameString.equalsIgnoreCase(CONTENT_TYPE)) {
                    if (sawType) {
                        throw new IOException("Duplicate content-type header");
                    }
                    sawType = true;
                }
                
                int headerValueLength = di.readUnsignedShort();
                
                byte[] headerValueBytes = new byte[headerValueLength];
                
                di.readFully(headerValueBytes);
                
                headers.add(new Header(headerNameString, headerValueBytes));
                
            }
        } while (!sawEmpty);
        
        if (!sawLength) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Content Length header was missing");
            }
            throw new IOException("Content Length header was missing");
        }
        
        if (!sawType) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Content Type header was missing");
            }
            throw new IOException("Content Type header was missing");
        }
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        StringBuilder result = new StringBuilder();
        
        result.append('[');
        
        Iterator<Header> eachHeader = getHeaders();

        while (eachHeader.hasNext()) {
            Header aHeader = eachHeader.next();
            
            result.append(" {");
            result.append(aHeader);
            result.append('}');
            
            if (eachHeader.hasNext()) {
                result.append(',');
            }
        }
        
        result.append(']');
        
        return result.toString();
    }
    
    /**
     * Returns number of header elements otherwise -1
     *
     * @param buffer the byte buffer
     * @return number of header elements
     */
    private int getHeaderCount(ByteBuffer buffer) {
        int pos = buffer.position();
        int limit = buffer.limit();
        int headerCount = 0;
        boolean sawZero = false;
        
        while (pos < limit) {
            // get header name length
            int len = buffer.get(pos) & 0xFF;

            pos += 1;
            
            if (0 == len) {
                sawZero = true;
                break;
            }
            
            // advance past name
            pos += len;
            
            if ((pos + 2) >= limit) {
                // not enough data
                break;
            }
            
            // get value length
            len = buffer.getShort(pos) & 0xFFFF;
            pos += 2;
            
            // advance past value
            pos += len;
            
            headerCount++;
        }
        
        return sawZero ? headerCount : -1;
    }
    
    /**
     * Reads a Header from a ByteBuffer
     *
     * @param buffer the input buffer
     * @return {@code true} If the header block was completely read.
     * @throws IOException if an io error is encountered
     */
    public boolean readHeader(ByteBuffer buffer) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("Parsing Package Header from byte buffer :{0}", buffer.toString()));
        }
        
        int count = getHeaderCount(buffer);

        if (count < 0) {
            return false;
        }
        for (int i = 1; i <= count; i++) {
            byte headerNameLength = buffer.get();
            byte[] headerNameBytes = new byte[headerNameLength];
            
            buffer.get(headerNameBytes);
            String headerNameString = new String(headerNameBytes, "UTF-8");
            int headerValueLength = buffer.getShort() & 0x0FFFF;  // unsigned
            byte[] headerValueBytes = new byte[headerValueLength];
            
            buffer.get(headerValueBytes);
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(MessageFormat.format("Adding Name {0}: {1}", headerNameString, headerValueBytes));
            }
            headers.add(new Header(headerNameString, headerValueBytes));
        }
        
        // get the end-of-pkg
        buffer.get();
        
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer(MessageFormat.format("Parsed {0} header elements, buffer stats :{1}", count, buffer.toString()));
        }
        return true;
    }
    
    /**
     * Add a header.
     *
     * @param name  The header name. The UTF-8 encoded representation of this
     * name may not be longer than 255 bytes.
     * @param value The value for the header. May not exceed 65535 bytes in
     * length.
     */
    public void addHeader(String name, byte[] value) {
        if (name.length() > 255) {
            throw new IllegalArgumentException("name may not exceed 255 bytes in length.");
        }
        
        if (value.length > 65535) {
            throw new IllegalArgumentException("value may not exceed 65535 bytes in length.");
        }
        
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Add header :" + name + "(" + name.length() + ") with " + value.length + " bytes of value");
        }
        
        headers.add(new Header(name, value));
    }
    
    /**
     * Add a header.
     *
     * @param name  The header name. The UTF-8 encoded representation of this
     * name may not be longer than 255 bytes.
     * @param value The value for the header. May not exceed 65535 bytes in
     * length.
     */
    public void addHeader(String name, String value) {        
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Add header :" + name + "(" + name.length() + ") with " + value.length() + " chars of value");
        }
        
        try {
            addHeader(name, value.getBytes("UTF-8"));
        } catch (UnsupportedEncodingException never) {
            // utf-8 is a required encoding.
            throw new IllegalStateException("UTF-8 encoding support missing!");
        }
    }
 
    /**
     * Replace a header. Replaces all existing headers with the same name.
     *
     * @param name  The header name. The UTF-8 encoded representation of this
     * name may not be longer than 255 bytes.
     * @param value The value for the header. May not exceed 65535 bytes in
     * length.
     */
    public void replaceHeader(String name, byte[] value) {
        if (name.length() > 255) {
            throw new IllegalArgumentException("name may not exceed 255 bytes in length.");
        }
        
        if (value.length > 65535) {
            throw new IllegalArgumentException("value may not exceed 65535 bytes in length.");
        }
        
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Replace header :" + name + "(" + name.length() + ") with " + value.length + " bytes of value");
        }
        
        Header newHeader = new Header(name, value);
        ListIterator<Header> eachHeader = getHeaders();
        boolean replaced = false;
        
        while (eachHeader.hasNext()) {
            Header aHeader = eachHeader.next();
            
            if (aHeader.getName().equalsIgnoreCase(name)) {
                eachHeader.set(newHeader);
                replaced = true;
            }
        }
        
        if(!replaced) {
            headers.add(newHeader);
        }
    }
 
    /**
     * Replace a header. Replaces all existing headers with the same name
     *
     * @param name  The header name. The UTF-8 encoded representation of this
     * name may not be longer than 255 bytes.
     * @param value The value for the header. May not exceed 65535 bytes in
     * length.
     */
    public void replaceHeader(String name, String value) {
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Replace header :" + name + "(" + name.length() + ") with " + value.length() + " chars of value");
        }
        
        try {
            replaceHeader(name, value.getBytes("UTF-8"));
        } catch (UnsupportedEncodingException never) {
            // utf-8 is a required encoding.
            throw new IllegalStateException("UTF-8 encoding support missing!");
        }
    }
    
    /**
     * Gets all of the headers. This iterator provides access to the live
     * data of this instance. Modifying the headers using {@code add()},
     * {@code set()}, {@code remove()} is permitted.
     *
     * @return all of the headers
     */
    public ListIterator<Header> getHeaders() {
        return headers.listIterator();
    }
    
    /**
     * Gets all of the headers matching the specified name
     *
     * @param name the name of the header we are seeking.
     */
    public Iterator<Header> getHeader(String name) {
        List<Header> matchingHeaders = new ArrayList<Header>();
        
        for (Header aHeader : headers) {
            if (name.equals(aHeader.getName())) {
                matchingHeaders.add(aHeader);
            }
        }
        return matchingHeaders.iterator();
    }
    
    /**
     * Write this group of header elements to a stream.
     *
     * @param out the stream to send the headers to.
     * @throws java.io.IOException if an io error occurs
     */
    public void sendToStream(OutputStream out) throws IOException {
        Iterator<Header> eachHeader = getHeaders();
        DataOutput dos = new DataOutputStream(out);
        
        // todo 20021014 bondolo@jxta.org A framing signature would help here
        
        while (eachHeader.hasNext()) {
            Header aHeader = eachHeader.next();
            
            byte[] nameBytes = aHeader.getName().getBytes("UTF-8");
            byte[] value = aHeader.getValue();
            
            assert nameBytes.length <= 255;
            assert value.length <= 65535;
            
            dos.write(nameBytes.length);
            dos.write(nameBytes);
            dos.writeShort(value.length);
            dos.write(value);
        }
        
        // write empty header
        dos.write(0);
    }
    
    /**
     * Return a ByteBuffer representing this group of header elements.
     *
     * @return ByteBuffer representing this Header
     */
    public ByteBuffer getByteBuffer() {
        // note: according to the spec this may exceed MAX_HEADER_LEN,
        // but since there are practically only 3 header elements used
        // it's safe to assume this implemention detail.
        ByteBuffer buffer = ByteBuffer.allocate(MAX_HEADER_LEN);
        
        for (Header header : headers) {
            byte[] name;

            try {
                name = header.getName().getBytes("UTF-8");
            } catch (UnsupportedEncodingException never) {
                throw new Error("Required UTF-8 encoding not available.");
            }
            
            byte[] value = header.getValue();
            
            assert name.length <= 255;
            assert value.length <= 65535;
            
            buffer.put((byte) name.length);
            buffer.put(name);
            buffer.putShort((short) value.length);
            buffer.put(value);
        }
        
        // write empty header
        buffer.put((byte) 0);
        buffer.flip();
        
        return buffer;
    }
    
    /**
     * Convenience method setting the "{@code content-length}" header.
     *
     * @param length length of the message.
     */
    public void setContentLengthHeader(long length) {
        byte[] lengthAsBytes = new byte[8];

        for (int eachByte = 0; eachByte < 8; eachByte++) {
            lengthAsBytes[eachByte] = (byte) (length >> ((7 - eachByte) * 8L));
        }
        
        replaceHeader(CONTENT_LENGTH, lengthAsBytes);
    }
    
    /**
     * Convenience method for getting the "{@code content-length}" header.
     *
     * @return length from the header or -1 if there was no
     * {@code content-length} header element.
     */
    public long getContentLengthHeader() {
        Iterator<Header> it = getHeader(CONTENT_LENGTH);

        if (!it.hasNext()) {
            return -1L;
        }
        Header header = it.next();
        byte[] lengthAsBytes = header.getValue();
        
        long lengthAsLong = 0L;
        
        for (int eachByte = 0; eachByte < 8; eachByte++) {
            lengthAsLong |= ((long) (lengthAsBytes[eachByte] & 0xff)) << ((7 - eachByte) * 8L);
        }
        
        return lengthAsLong;
    }
    
    /**
     * Convenience method for setting the "{@code content-type}" header.
     *
     * @param type type of the message.
     */
    public void setContentTypeHeader(MimeMediaType type) {
        replaceHeader(CONTENT_TYPE, type.toString());
    }
    
    /**
     * Convenience method for getting the "{@code content-type}" header.
     *
     * @return type from the header or "{@code application/octet-stream}" if
     * there was no {@code content-type} header.
     */
    public MimeMediaType getContentTypeHeader() {
        Iterator<Header> it = getHeader(CONTENT_TYPE);

        if (!it.hasNext()) {
            // return the generic type. Better than returning "null".
            return MimeMediaType.AOS;
        }
        Header header = it.next();
        
        return MimeMediaType.valueOf(header.getValueString());
    }
}
