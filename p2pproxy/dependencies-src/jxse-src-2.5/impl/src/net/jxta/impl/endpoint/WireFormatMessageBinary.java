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

package net.jxta.impl.endpoint;


import net.jxta.document.MimeMediaType;
import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.WireFormatMessage;
import net.jxta.endpoint.WireFormatMessageFactory;
import net.jxta.util.LimitInputStream;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.SequenceInputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.nio.ByteBuffer;
import java.text.MessageFormat;
import java.util.Arrays;


/**
 * A Wire Format Message which encodes the message into MIME Type
 * "application/x-jxta-msg".
 * <p/>
 * <p/>This implementation does nothing with encodings.
 * <p/>
 * <p/>This implementation does not use any MIME parameters attached to the
 * requesting mime type.
 *
 * @see net.jxta.endpoint.WireFormatMessageFactory
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#msgs-fmts-jbm" target="_blank">JXTA Protocols Specification : Binary Message Format</a>
 */
public class WireFormatMessageBinary implements WireFormatMessage {

    /*
     *  Log4J Logger
     */
    private final static transient Logger LOG = Logger.getLogger(WireFormatMessageBinary.class.getName());

    // Flag bits
    protected static final byte HAS_TYPE = 0x01;
    protected static final byte HAS_ENCODING = 0x02;
    protected static final byte HAS_SIGNATURE = 0x04;

    protected static final int MESSAGE_VERSION = 0;

    /**
     * Our Mime Media Type(s)
     */
    private static final MimeMediaType[] myTypes = {
        MimeMediaType.valueOf("application/x-jxta-msg") };

    /**
     * These are the content encodings we support.
     */
    private static final MimeMediaType[] myContentEncodings = {

        // we support raw binary!
        null
    };

    /**
     * Our instantiator for the factory.
     */
    public static final WireFormatMessageFactory.Instantiator INSTANTIATOR = new Instantiator();

    /**
     * Our instantiator.
     */
    static class Instantiator implements WireFormatMessageFactory.Instantiator {

        /**
         * Creates new WireFormatMessageBinary Instantiator
         */
        public Instantiator() {}

        /**
         * {@inheritDoc}
         */
        public MimeMediaType[] getSupportedMimeTypes() {
            return myTypes;
        }

        /**
         * {@inheritDoc}
         */
        public MimeMediaType[] getSupportedContentEncodings() {
            return myContentEncodings;
        }

        /**
         * {@inheritDoc}
         */
        public Message fromWire(InputStream is, MimeMediaType type, MimeMediaType contentEncoding) throws IOException {
            // FIXME 20020504 bondolo@jxta.org  Ignores type and contentEncoding completely.
            Message msg = new Message();

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Reading " + msg + " from " + is);
            }

            DataInputStream dis = new DataInputStream(is);

            HashMap idToNamespace = readHeader(dis);

            int elementCnt = dis.readShort();

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Message element count " + elementCnt + " from " + is);
            }

            int eachElement = 0;

            do {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("Read element " + eachElement + " of " + elementCnt + " from " + is + " for " + msg);
                }

                Object[] anElement;

                try {
                    anElement = readMessageElement(dis, is);
                } catch (IOException failed) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE
                                ,
                                "Failure reading element " + eachElement + " of " + elementCnt + " from " + is + " for " + msg
                                ,
                                failed);
                    }

                    throw failed;
                }

                if (null == anElement) {
                    break;
                }

                String namespace = (String) idToNamespace.get(anElement[0]);

                if (null == namespace) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("Element identified a namespace which was not defined for this message.");
                    }

                    throw new IOException("Element identified a namespace which was not defined for this message.");
                }

                msg.addMessageElement(namespace, (MessageElement) anElement[1]);
                eachElement++;

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINER)) {
                    LOG.finer(
                            "Add element (name=\'" + ((MessageElement) anElement[1]).getElementName() + "\') #" + eachElement
                            + " of #" + elementCnt + " elements from " + dis.toString());
                }
            } while (((0 == elementCnt) || (eachElement < elementCnt)));

            if ((elementCnt != 0) && (eachElement != elementCnt)) {
                throw new IOException("Found wrong number of elements in message.");
            }

            return msg;
        }

        public Message fromBuffer(ByteBuffer buffer, MimeMediaType type, MimeMediaType contentEncoding) throws IOException {
            // FIXME 20020504 bondolo@jxta.org  Ignores type and contentEncoding completely.
            Message msg = new Message();

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Reading " + msg + " from " + buffer);
            }

            HashMap idToNamespace = readHeader(buffer);

            int elementCnt = buffer.getShort();

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Message element count " + elementCnt + " from " + buffer);
            }

            int eachElement = 0;

            do {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("Read element " + eachElement + " of " + elementCnt + " from " + buffer + " for " + msg);
                }

                Object[] anElement;

                try {
                    anElement = readMessageElement(buffer);

                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.finer(
                                MessageFormat.format("Read element of size {0}, [{1}] {2}", anElement.length, anElement.toString()
                                ,
                                buffer.toString()));
                    }
                } catch (IOException failed) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE
                                ,
                                "Failure reading element " + eachElement + " of " + elementCnt + " from " + buffer + " for " + msg
                                ,
                                failed);
                    }
                    throw failed;
                }

                if (null == anElement) {
                    break;
                }

                String namespace = (String) idToNamespace.get(anElement[0]);

                if (null == namespace) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("Element identified a namespace which was not defined for this message.");
                    }
                    throw new IOException("Element identified a namespace which was not defined for this message.");
                }

                msg.addMessageElement(namespace, (MessageElement) anElement[1]);
                eachElement++;

                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer(
                            "Add element (name=\'" + ((MessageElement) anElement[1]).getElementName() + "\') #" + eachElement
                            + " of #" + elementCnt + " elements from " + buffer.toString());
                }
            } while (((0 == elementCnt) || (eachElement < elementCnt)));

            if ((elementCnt != 0) && (eachElement != elementCnt)) {
                throw new IOException("Found wrong number of elements in message.");
            }

            return msg;
        }

        /**
         * {@inheritDoc}
         */
        public WireFormatMessage toWire(Message msg, MimeMediaType type, MimeMediaType[] preferedContentEncoding) {
            try {
                return new WireFormatMessageBinary(msg, type, preferedContentEncoding);
            } catch (IOException caught) {
                throw new IllegalStateException("Could not build wire format for message due to " + caught.getMessage());
            }
        }

        /**
         * Read in a message header from the provided data stream.
         *
         * @param dis the data stream to read from
         * @return hashmap containing the namespace id to namespace values
         * @throws IOException if EOF or other IOException is encountered
         *                     during the reading of the header.
         */
        private static HashMap readHeader(DataInputStream dis) throws IOException {
            // Read message signature
            char[] msgsig = new char[4];

            try {
                msgsig[0] = (char) dis.readByte();
            } catch (EOFException failed) {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.log(Level.FINER, "EOF reading message at first byte of header.", failed);
                }

                throw failed;
            }
            msgsig[1] = (char) dis.readByte();
            msgsig[2] = (char) dis.readByte();
            msgsig[3] = (char) dis.readByte();

            if (msgsig[0] != 'j' || msgsig[1] != 'x' || msgsig[2] != 'm' || msgsig[3] != 'g') {
                IOException failure = new IOException(
                        "Not a message (incorrect signature '" + msgsig[0] + msgsig[1] + msgsig[2] + msgsig[3] + "') ");

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe(failure.toString());
                }

                throw failure;
            }

            // Message version
            if (dis.readByte() != MESSAGE_VERSION) {
                IOException failure = new IOException("Message not version " + MESSAGE_VERSION);

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, failure.getMessage(), failure);
                }

                throw failure;
            }

            int namespaceCnt = dis.readShort();

            if (namespaceCnt > 253) {
                IOException failure = new IOException("Message contains too many namespaces (>253)");

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, failure.getMessage(), failure);
                }

                throw failure;
            }

            HashMap<Integer, String> id2namespace = new HashMap<Integer, String>(2 + namespaceCnt);

            id2namespace.put(0, "");
            id2namespace.put(1, "jxta");

            int id = 2;

            for (int i = 0; i < namespaceCnt; ++i) {
                try {
                    String namespace = readString(dis);

                    id2namespace.put(id++, namespace);
                } catch (IOException caught) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Error Processing namespace", caught);
                    }
                    throw caught;
                }
            }

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Read Message Header with " + (namespaceCnt + 2) + " namespaces from " + dis.toString());
            }

            return id2namespace;
        }

        /**
         * Read in a message header from the provided data stream.
         *
         * @param buffer the data buffer to read from
         * @return hashmap containing the namespace id to namespace values
         * @throws IOException if EOF or other IOException is encountered
         *                     during the reading of the header.
         */
        private static HashMap readHeader(ByteBuffer buffer) throws IOException {
            // Read message signature
            char[] msgsig = new char[4];

            msgsig[0] = (char) buffer.get();
            msgsig[1] = (char) buffer.get();
            msgsig[2] = (char) buffer.get();
            msgsig[3] = (char) buffer.get();

            if (msgsig[0] != 'j' || msgsig[1] != 'x' || msgsig[2] != 'm' || msgsig[3] != 'g') {
                IOException failure = new IOException(
                        "Not a message (incorrect signature '" + msgsig[0] + msgsig[1] + msgsig[2] + msgsig[3] + "') ");

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe(failure.toString());
                }
                throw failure;
            }

            // Message version
            if (buffer.get() != MESSAGE_VERSION) {
                IOException failure = new IOException("Message not version " + MESSAGE_VERSION);

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, failure.getMessage(), failure);
                }
                throw failure;
            }

            int namespaceCnt = buffer.getShort();

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(MessageFormat.format("Message defines {0} namespaces buffer stats{1}", namespaceCnt, buffer.toString()));
            }

            if (namespaceCnt > 253) {
                IOException failure = new IOException(
                        MessageFormat.format("Message contains too many namespaces ({0} >253)", namespaceCnt));

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, failure.getMessage(), failure);
                }

                throw failure;
            }

            HashMap<Integer, String> id2namespace = new HashMap<Integer, String>(2 + namespaceCnt);

            id2namespace.put(0, "");
            id2namespace.put(1, "jxta");

            int id = 2;

            for (int i = 0; i < namespaceCnt; ++i) {
                try {
                    String namespace = readString(buffer);

                    id2namespace.put(id++, namespace);
                } catch (IOException caught) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Error Processing namespace", caught);
                    }
                    throw caught;
                }
            }

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Read Message Header with " + (namespaceCnt + 2) + " namespaces from " + buffer.toString());
            }

            return id2namespace;
        }

        /**
         * Read in a message element from the provided data stream.
         *
         * @param dis the data stream to read from
         * @param is  todo
         * @return object array containing two objects, index[0] contains an
         *         Integer which identifies the namespace to which this element belongs
         *         and index[1] contains a MessageElement. If null is returned then
         *         the DataInputStream reached EOF before reading the first byte of the
         *         element.
         * @throws IOException if EOF or other IOException is encountered
         *                     during the reading of the element.
         */
        private Object[] readMessageElement(DataInputStream dis, InputStream is) throws IOException {
            // Read message signature
            char[] elsig = new char[4];

            // if we EOF before the first byte, return null. EOF anywhere else
            // and its an error.
            try {
                elsig[0] = (char) dis.readByte();
            } catch (EOFException allDone) {
                return null;
            }

            elsig[1] = (char) dis.readByte();
            elsig[2] = (char) dis.readByte();
            elsig[3] = (char) dis.readByte();

            if (elsig[0] != 'j' || elsig[1] != 'x' || elsig[2] != 'e' || elsig[3] != 'l') {
                IOException failure = new IOException(
                        "Not a message element (incorrect signature '" + elsig[0] + elsig[1] + elsig[2] + elsig[3] + "') ");

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, failure.getMessage(), failure);
                }

                throw failure;
            }

            // Namespace id
            int nsid = dis.readByte();

            // flags
            byte flags = dis.readByte();

            // Name
            String name = readString(dis);

            // Mime type
            MimeMediaType type;

            if ((flags & HAS_TYPE) != 0) {
                String typeString = readString(dis);

                try {
                    type = new MimeMediaType(typeString);
                } catch (IllegalArgumentException uhoh) {
                    throw new IOException("Bad MIME type in message element header : " + uhoh.getMessage());
                }
            } else {
                type = MimeMediaType.AOS;
            }

            int dataLen = dis.readInt();

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(
                        "element : nsid = " + nsid + " name = \'" + name + "\' type = \'" + type + "\' flags = "
                        + Integer.toBinaryString(flags) + " datalen = " + dataLen);
            }

            Object[] res = new Object[2];

            res[0] = nsid & 0x000000FF;

            byte[] value = null;
            Message submsg = null;

            // Value
            if (type.equalsIngoringParams(myTypes[0])) {
                InputStream subis = new LimitInputStream(is, dataLen);

                submsg = WireFormatMessageFactory.fromWire(subis, type, null);
            } else {
                if (dataLen > Integer.MAX_VALUE) {
                    // FIXME hamada dataLen is an int, the above expression can never be true
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("WireFormatMessageBinary does not support elements longer than 2GB");
                    }
                    throw new IllegalStateException("WireFormatMessageBinary does not support elements longer than 2GB");
                }

                value = new byte[dataLen];

                String mayFail = null;

                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    mayFail = is.toString();
                }

                try {
                    dis.readFully(value);
                } catch (EOFException failed) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("had tried to read " + dataLen + " from " + mayFail + " which is now " + is);
                    }
                    throw failed;
                }
            }

            MessageElement sig = null;

            if ((flags & HAS_SIGNATURE) != 0) {
                Object[] sigRes = readMessageElement(dis, is);

                sig = (MessageElement) sigRes[1];
            }

            if (null != value) {
                res[1] = new ByteArrayMessageElement(name, type, value, sig);
            } else {
                res[1] = new JxtaMessageMessageElement(name, type, submsg, sig);
            }

            return res;
        }

        /**
         * Read in a message element from the provided data stream.
         *
         * @param buffer the data buffer to read from
         * @return object array containing two objects, index[0] contains an
         *         Integer which identifies the namespace to which this element belongs
         *         and index[1] contains a MessageElement. If null is returned then
         *         the DataInputStream reached EOF before reading the first byte of the
         *         element.
         * @throws IOException if EOF or other IOException is encountered
         *                     during the reading of the element.
         */
        private Object[] readMessageElement(ByteBuffer buffer) throws IOException {
            // Read message signature
            char[] elsig = new char[4];

            // if we EOF before the first byte, return null. EOF anywhere else
            // and its an error.
            elsig[0] = (char) buffer.get();
            elsig[1] = (char) buffer.get();
            elsig[2] = (char) buffer.get();
            elsig[3] = (char) buffer.get();

            if (elsig[0] != 'j' || elsig[1] != 'x' || elsig[2] != 'e' || elsig[3] != 'l') {
                IOException failure = new IOException(
                        "Not a message element (incorrect signature '" + elsig[0] + elsig[1] + elsig[2] + elsig[3] + "') ");

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, failure.getMessage(), failure);
                }

                throw failure;
            }

            // Namespace id
            int nsid = buffer.get();

            // flags
            byte flags = buffer.get();

            // Name
            String name = readString(buffer);

            // Mime type
            MimeMediaType type;

            if ((flags & HAS_TYPE) != 0) {
                String typeString = readString(buffer);

                try {
                    type = new MimeMediaType(typeString);
                } catch (IllegalArgumentException uhoh) {
                    throw new IOException("Bad MIME type in message element header : " + uhoh.getMessage());
                }
            } else {
                type = MimeMediaType.AOS;
            }

            int dataLen = buffer.getInt();

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(
                        "element : nsid = " + nsid + " name = \'" + name + "\' type = \'" + type + "\' flags = "
                        + Integer.toBinaryString(flags) + " datalen = " + dataLen);
            }

            Object[] res = new Object[2];

            res[0] = nsid & 0x000000FF;

            byte[] value = null;
            Message submsg = null;

            // Value
            if (type.equalsIngoringParams(myTypes[0])) {
                InputStream subis = new ByteArrayInputStream(buffer.array(), buffer.position(), dataLen);

                submsg = WireFormatMessageFactory.fromWire(subis, type, null);
                // buffer.position(buffer.position() + dataLen);
            } else {
                if (dataLen > Integer.MAX_VALUE) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("WireFormatMessageBinary does not support elements longer than 2GB");
                    }
                    throw new IllegalStateException("WireFormatMessageBinary does not support elements longer than 2GB");
                }

                value = new byte[dataLen];

                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer(MessageFormat.format("expecting {0} bytes, Buffer stats {1}", dataLen, buffer.toString()));
                }
                buffer.get(value);
            }

            MessageElement sig = null;

            if ((flags & HAS_SIGNATURE) != 0) {
                Object[] sigRes = readMessageElement(buffer);

                sig = (MessageElement) sigRes[1];
            }

            if (null != value) {
                res[1] = new ByteArrayMessageElement(name, type, value, sig);
            } else {
                res[1] = new JxtaMessageMessageElement(name, type, submsg, sig);
            }

            return res;
        }

        /**
         * Read and construct a string from the data stream.
         *
         * @param dis the stream to read from
         * @return the String which was read.
         * @throws IOException if EOF or other IOException is encountered
         *                     during the reading of the string.
         */
        private static String readString(DataInputStream dis) throws IOException {
            int len = dis.readShort();

            if (len < 0) {
                throw new IOException("Bad string length in message");
            }

            byte[] bytes = new byte[len];

            dis.readFully(bytes);
            return new String(bytes, "UTF8");
        }

        /**
         * Read and construct a string from the data stream.
         *
         * @param buffer the ByteBuffer to read from
         * @return the String which was read.
         * @throws IOException if EOF or other IOException is encountered
         *                     during the reading of the string.
         */
        private static String readString(ByteBuffer buffer) throws IOException {
            int len = buffer.getShort();

            if (len < 0) {
                throw new IOException("Bad string length in message");
            }

            byte[] bytes = new byte[len];

            buffer.get(bytes);
            return new String(bytes, "UTF8");
        }
    }


    /**
     * Internal representation for a binary format wire message. Implemented
     * as an inner class to allow content encodings to be easily mapped on
     * top of the streams this class produces.
     */
    static class binaryMessageProxy implements WireFormatMessage {
        final Message message;

        final MimeMediaType type;

        final List<binaryElementProxy> elements = new ArrayList<binaryElementProxy>();

        final Map<String, Integer> namespaceIDs = new HashMap<String, Integer>();

        final List<String> namespaces = new ArrayList<String>();

        byte[] header;

        binaryMessageProxy(Message msg, MimeMediaType type) throws IOException {
            message = msg;

            this.type = type; // we may generate different content based upon the type.

            assignNamespaceIds();

            // build the element proxies
            Message.ElementIterator eachElement = message.getMessageElements();

            while (eachElement.hasNext()) {
                MessageElement anElement = eachElement.next();
                byte namespaceid = namespaceIDs.get(eachElement.getNamespace()).byteValue();

                elements.add(new binaryElementProxy(namespaceid, anElement));
            }

            buildHeader();
        }

        /**
         * {@inheritDoc}
         */
        public String getFileExtension() {
            return "???";
        }

        /**
         * {@inheritDoc}
         */
        public MimeMediaType getMimeType() {
            return type;
        }

        /**
         * {@inheritDoc}
         */
        public ByteBuffer[] getByteBuffers() {
            List<ByteBuffer> partBuffers = new ArrayList<ByteBuffer>();
            
            partBuffers.add(ByteBuffer.wrap(header));
            
            for (binaryElementProxy anElement : elements) {
                partBuffers.addAll(Arrays.asList(anElement.getByteBuffers()));
            }
            
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(MessageFormat.format("Returning {0} buffers for {1}", partBuffers.size(), message));
            }

            return partBuffers.toArray(new ByteBuffer[partBuffers.size()]);
        }
        
        /**
         * {@inheritDoc}
         */
        public InputStream getStream() throws IOException {

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Getting stream for " + message);
            }

            List<InputStream> streamParts = new ArrayList<InputStream>();

            streamParts.add(new ByteArrayInputStream(header));

            for (binaryElementProxy anElement : elements) {
                streamParts.add(anElement.getStream());
            }

            InputStream theStream = new SequenceInputStream(Collections.enumeration(streamParts));

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(
                        MessageFormat.format("Returning {0}@{1} for {2}", theStream.getClass().getName()
                        ,
                        System.identityHashCode(theStream), message));
            }

            return theStream;
        }

        /**
         * {@inheritDoc}
         */
        public void sendToStream(OutputStream sendTo) throws IOException {

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Sending " + message + " to " + sendTo.getClass().getName() + "@" + System.identityHashCode(sendTo));
            }

            sendTo.write(header);

            Iterator eachElement = elements.listIterator();

            while (eachElement.hasNext()) {
                binaryElementProxy anElement = (binaryElementProxy) eachElement.next();

                anElement.sendToStream(sendTo);
            }
        }

        /**
         * {@inheritDoc}
         */
        public long getByteLength() {
            long size = 0;

            size += header.length;
            for (binaryElementProxy element : elements) {
                binaryElementProxy anElement = element;

                size += anElement.getByteLength();
            }
            return size;
        }

        /**
         *  {@inheritDoc}
         */
        public MimeMediaType getContentEncoding() {
            return null;
        }

        /**
         * Scans the source message to build a HashMap of the namespaces used
         * in the message and assign and id to each namespace.
         */
        private void assignNamespaceIds() {
            int id = 0;
            Iterator namespaces = message.getMessageNamespaces();

            // insert the predefined namespaces.
            namespaceIDs.put("", id++);
            this.namespaces.add("");
            namespaceIDs.put("jxta", id++);
            this.namespaces.add("jxta");

            // insert items in the vector if they are not found in the map
            while (namespaces.hasNext()) {
                String namespace = (String) namespaces.next();

                if (namespaceIDs.get(namespace) == null) {
                    namespaceIDs.put(namespace, id++);
                    this.namespaces.add(namespace);
                }
            }

            if (id >= 256) {
                throw new IllegalStateException("WireFormatMessageBinary does not support more than 255 namespaces");
            }
        }

        /**
         * Builds the wire format header for the message.
         *
         * @throws IOException if for some reason the header cannot be built.
         */
        private void buildHeader() throws IOException {
            ByteArrayOutputStream headerBytes = new ByteArrayOutputStream(256);
            DataOutputStream header = new DataOutputStream(headerBytes);

            header.writeBytes("jxmg");

            header.writeByte(MESSAGE_VERSION);
            header.writeShort(namespaces.size() - 2);

            for (int eachNamespace = 2; eachNamespace < namespaces.size(); eachNamespace++) {
                byte[] elementName = namespaces.get(eachNamespace).getBytes("UTF8");

                header.writeShort(elementName.length);
                header.write(elementName, 0, elementName.length);
            }

            header.writeShort(elements.size());

            header.flush();
            header.close();
            headerBytes.flush();
            headerBytes.close();

            this.header = headerBytes.toByteArray();
        }
    }


    /**
     * Proxy for a message element. Handles the serialization of the element
     * meta information.
     */
    static class binaryElementProxy {
        final byte namespaceid;

        binaryElementProxy sig;

        MessageElement element;

        byte[] header;

        binaryElementProxy(byte namespaceid, MessageElement element) throws IOException {
            this.namespaceid = namespaceid;

            this.element = element;

            MessageElement sig = element.getSignature();

            if (null != sig) {
                this.sig = new binaryElementProxy(namespaceid, sig);
            }

            buildHeader();
        }

        void buildHeader() throws IOException {
            byte[] elementName = element.getElementName().getBytes("UTF8");
            byte[] elementType = null;

            if (!MimeMediaType.AOS.equals(element.getMimeType())) {
                elementType = element.getMimeType().toString().getBytes("UTF8");
            }

            // FIXME  20020504 bondolo@jxta.org Do something with encodings.
            ByteArrayOutputStream headerBytes = new ByteArrayOutputStream(256);
            DataOutputStream header = new DataOutputStream(headerBytes);

            header.writeBytes("jxel");

            header.writeByte(namespaceid);
            header.writeByte(((null != elementType) ? HAS_TYPE : 0) | ((null != sig) ? HAS_SIGNATURE : 0));

            header.writeShort(elementName.length);
            header.write(elementName, 0, elementName.length);

            if (null != elementType) {
                header.writeShort(elementType.length);
                header.write(elementType, 0, elementType.length);
            }

            // FIXME content encoding should go here

            long dataLen = element.getByteLength();

            if (dataLen > Integer.MAX_VALUE) {
                throw new IllegalStateException("WireFormatMessageBinary does not support elements longer than 4GB");
            }

            header.writeInt((int) dataLen);

            header.flush();
            header.close();
            headerBytes.flush();
            headerBytes.close();

            this.header = headerBytes.toByteArray();
        }

        public long getByteLength() {
            long size = 0;

            size += header.length;
            size += element.getByteLength();
            if (null != sig) {
                size += sig.getByteLength();
            }

            return size;
        }

        public ByteBuffer[] getByteBuffers() {
            List<ByteBuffer> partBuffers = new ArrayList<ByteBuffer>();

            partBuffers.add(ByteBuffer.wrap(header));
            
            partBuffers.add(ByteBuffer.wrap(element.getBytes(false)));

            if (null != sig) {
                partBuffers.addAll(Arrays.asList(sig.getByteBuffers()));
            }

            return partBuffers.toArray(new ByteBuffer[partBuffers.size()]);           
        }
        
        public InputStream getStream() throws IOException {
            List<InputStream> streamParts = new ArrayList<InputStream>();

            streamParts.add(new ByteArrayInputStream(header));

            streamParts.add(element.getStream());

            if (null != sig) {
                streamParts.add(sig.getStream());
            }

            return new SequenceInputStream(Collections.enumeration(streamParts));
        }

        public void sendToStream(OutputStream sendTo) throws IOException {

            sendTo.write(header);
            element.sendToStream(sendTo);
            if (null != sig) {
                sig.sendToStream(sendTo);
            }
        }
    }

    /**
     * The message we are serializing.
     */
    private final Message msg;

    /**
     * The mod count of the message when we started. Used for detecting
     * (illegal) modifications.
     */
    private final int msgModCount;

    /**
     * The mime type of the encoded message.
     */
    private final MimeMediaType type;

    /**
     * The mime type of the content encoding for this message.
     */
    private final MimeMediaType contentEncoding;

    /**
     * The serialization peer to the Message.
     */
    private final binaryMessageProxy msgProxy;

    /**
     * Creates a new instance of WireFormatMessageBinary. Called only by the
     * Instantiator.
     *
     * @param msg                      the message being serialized
     * @param type                     the mime mediatype being requested.
     * @param preferedContentEncodings The ranked content encodings preferred by
     *                                 the recipient.
     * @throws java.io.IOException if an io error occurs
     */
    WireFormatMessageBinary(Message msg, MimeMediaType type, MimeMediaType[] preferedContentEncodings) throws IOException {
        if (null == msg) {
            throw new IllegalArgumentException("Null message!");
        }

        this.msg = msg;

        this.msgModCount = msg.getMessageModCount();

        if (null == type) {
            throw new IllegalArgumentException("Null mime type!");
        }

        int matchedIdx = -1;

        for (int eachType = 0; eachType < myTypes.length; eachType++) {
            if (type.equalsIngoringParams(myTypes[eachType])) {
                matchedIdx = eachType;
                break;
            }
        }

        if (-1 == matchedIdx) {
            throw new IllegalArgumentException("Unsupported mime type!");
        }

        // FIXME  20020504 bondolo@jxta.org Check the mimetype params to make
        // sure we can support them.
        this.type = type;

        // FIXME  20020504 bondolo@jxta.org Do something with encodings.
        this.contentEncoding = myContentEncodings[0];

        msgProxy = new binaryMessageProxy(msg, type);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object obj) {
        throw new UnsupportedOperationException("don't do this");
    }

    /*
     * The cost of just having a finalize routine is high. The finalizer is
     * a bottleneck and can delay garbage collection all the way to heap
     * exhaustion. Leave this comment as a reminder to future maintainers.
     * Below is the reason why finalize is not needed here.
     *
     * No critical non-memory resource held.
     protected void finalize( ) {
     }

     */

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        throw new UnsupportedOperationException("don't do this");
    }

    /**
     * {@inheritDoc}
     */
    public String getFileExtension() {
        return "???";
    }

    /**
     * {@inheritDoc}
     */
    public MimeMediaType getMimeType() {
        return type;
    }

    /**
     * {@inheritDoc}
     */
    public InputStream getStream() throws IOException {
        if (msg.getMessageModCount() != msgModCount) {
            throw new IllegalStateException("message was unexpectedly modified!");
        }

        msg.modifiable = false;
        try {
            InputStream result = msgProxy.getStream();

            return result;
        } finally {
            msg.modifiable = true;
        }
    }

    /**
     * {@inheritDoc}
     */
    public ByteBuffer[] getByteBuffers() {
        if (msg.getMessageModCount() != msgModCount) {
            throw new IllegalStateException("message was unexpectedly modified!");
        }

        msg.modifiable = false;
        try {
            ByteBuffer[] result = msgProxy.getByteBuffers();

            return result;
        } finally {
            msg.modifiable = true;
        }
    }

    /**
     * {@inheritDoc}
     */
    public void sendToStream(OutputStream sendTo) throws IOException {
        if (msg.getMessageModCount() != msgModCount) {
            throw new IllegalStateException("message was unexpectedly modified!");
        }

        msg.modifiable = false;
        try {
            msgProxy.sendToStream(sendTo);
        } finally {
            msg.modifiable = true;
        }
    }

    /**
     * {@inheritDoc}
     */
    public long getByteLength() {
        if (msg.getMessageModCount() != msgModCount) {
            throw new IllegalStateException("message was unexpectedly modified!");
        }

        return msgProxy.getByteLength();
    }

    /**
     * {@inheritDoc}
     */
    public MimeMediaType getContentEncoding() {
        return contentEncoding;
    }
}
