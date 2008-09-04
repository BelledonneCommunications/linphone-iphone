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

package net.jxta.endpoint;


import net.jxta.document.MimeMediaType;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.*;
import java.lang.ref.SoftReference;
import java.util.zip.CRC32;
import java.util.zip.Checksum;


/**
 * A Message Element using byte arrays for the element data.
 *
 * <p/>This implementation does not copy the byte array provided and assumes
 * that the contents of the byte array will not change through out the lifetime
 * of the MessageElement.
 *
 * <p/>some synchronization is due to optimization in {@link #getBytes(boolean)}
 * which replaces value of internal member {@link #b}.
 */
public class ByteArrayMessageElement extends MessageElement {

    /**
     * Logger
     */
    private static transient final Logger LOG = Logger.getLogger(ByteArrayMessageElement.class.getName());

    /**
     * The bytes of this element.
     */
    protected byte[] b;

    /**
     * This is the offset of our data within the array
     */
    protected int offset;

    /**
     * length of the element data. sometimes the same as b.length, but may be
     * lesser.
     */
    protected int len;

    /**
     * Create a new Message Element. The contents of the provided byte array
     * are <b>not</b> copied during construction.
     *
     * @param name Name of the MessageElement. May be the empty string ("") if
     *             the MessageElement is not named.
     * @param type Type of the MessageElement. null is the same as specifying
     *             the type "Application/Octet-stream".
     * @param b    A byte array containing the contents of this element.
     * @param sig  optional message digest/digital signature element or null if
     *             no signature is desired.
     */
    public ByteArrayMessageElement(String name, MimeMediaType type, byte[] b, MessageElement sig) {
        this(name, type, b, 0, b.length, sig);
    }

    /**
     * Create a new MessageElement, The contents of the provided byte array are
     * <b>not</b> copied during construction.
     *
     * @param name   Name of the MessageElement. May be the empty string ("") if
     *               the MessageElement is not named.
     * @param type   Type of the MessageElement. null is the same as specifying
     *               the type "Application/Octet-stream".
     * @param b      A byte array containing the contents of this element.
     * @param offset all bytes before this location in <code>b</code>
     *               will be ignored.
     * @param sig    optional message digest/digital signature elemnent or null if
     *               no signature is desired.
     */
    public ByteArrayMessageElement(String name, MimeMediaType type, byte[] b, int offset, MessageElement sig) {
        this(name, type, b, offset, b.length - offset, sig);
    }

    /**
     * Create a new Element, but dont add it to the message.  The contents of
     * the byte array are <b>not</b> copied during construction.
     *
     * @param name   Name of the MessageElement. May be the empty string ("") if
     *               the MessageElement is not named.
     * @param type   Type of the MessageElement. null is the same as specifying
     *               the type "Application/Octet-stream".
     * @param b      A byte array containing the contents of this Element.
     * @param offset all bytes before this location will be ignored.
     * @param len    number of bytes to include
     * @param sig    optional message digest/digital signature element or null if
     *               no signature is desired.
     */
    public ByteArrayMessageElement(String name, MimeMediaType type, byte[] b, int offset, int len, MessageElement sig) {
        super(name, type, sig);

        if (null == b) {
            throw new IllegalArgumentException("byte array must not be null");
        }

        if (len < 0) {
            throw new IllegalArgumentException("len must be >= 0 : " + len);
        }

        if (offset < 0) {
            throw new IllegalArgumentException("offset must within byte array : " + offset);
        }

        if ((0 != len) && (offset >= b.length)) {
            throw new IllegalArgumentException("offset must be positioned within byte array : " + offset + "," + len);
        }

        if (((offset + len) > b.length) || ((offset + len) < 0)) {
            throw new IllegalArgumentException("offset + len must be positioned within byte array");
        }

        // if we get an empty request and a non-empty buffer, we don't use the provided buffer.
        if ((0 == len) && (0 != b.length)) {
            b = new byte[len];
            offset = 0;
        }

        this.b = b;
        this.offset = offset;
        this.len = len;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }

        if (target instanceof MessageElement) {
            if (!super.equals(target)) {
                return false;
            }

            if (target instanceof ByteArrayMessageElement) {
                ByteArrayMessageElement likeMe = (ByteArrayMessageElement) target;

                synchronized (this) {
                    synchronized (likeMe) {
                        if (likeMe.len != len) {
                            return false;
                        }

                        for (int eachByte = len - 1; eachByte >= 0; eachByte--) {
                            if (likeMe.b[likeMe.offset + eachByte] != b[offset + eachByte]) {
                                return false;
                            }
                        }
                    }
                }

                return true;
            } else {
                // have to do a slow stream comparison.
                // XXX 20020615 bondolo@jxta.org the performance of this could be much improved.
                try {
                    MessageElement likeMe = (MessageElement) target;

                    InputStream myStream = getStream();
                    InputStream itsStream = likeMe.getStream();

                    int mine;
                    int its;

                    do {
                        mine = myStream.read();
                        its = itsStream.read();

                        if (mine != its) {
                            return false;
                        }       // content didn't match

                    } while ((-1 != mine) && (-1 != its));

                    return ((-1 == mine) && (-1 == its)); // end at the same time?
                } catch (IOException fatal) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "MessageElements could not be compared.", fatal);
                    }

                    throw new IllegalStateException("MessageElements could not be compared." + fatal);
                }
            }
        }

        return false; // not a message element
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized int hashCode() {
        Checksum crc = new CRC32();

        crc.update(b, offset, len);
        int dataHash = (int) crc.getValue();

        int result = super.hashCode() * 6037 + // a prime
                dataHash;

        return (0 != result) ? result : 1;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Returns the string representation of this element. The 'charset'
     * parameter of the mimetype, if any, is used to determine encoding. If
     * the charset specified is unsupported then the default encoding will be
     * used.
     *
     * @return String string representation of this message element.
     */
    @Override
    public synchronized String toString() {
        String result;

        if (null != cachedToString) {
            result = cachedToString.get();

            if (null != result) {
                return result;
            }
        }

        if (LOG.isLoggable(Level.FINER)) {
            LOG.finer("creating toString of " + getClass().getName() + '@' + Integer.toHexString(hashCode()));
        }

        String charset = type.getParameter("charset");

        try {
            if (null == charset) {
                result = new String(b, offset, len);
            } else {
                result = new String(b, offset, len, charset);
            }
        } catch (UnsupportedEncodingException caught) {
            result = new String(b, offset, len);
        }

        cachedToString = new SoftReference<String>(result);

        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getByteLength() {
        return len;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>synchronized so that we can replace our internal buffer with
     * the buffer we are returning if we were using a shared buffer.
     */
    @Override
    public synchronized byte[] getBytes(boolean copy) {
        if ((!copy) && (0 == offset) && (b.length == len)) {
            return b;
        }

        byte[] result = new byte[len];

        System.arraycopy(b, offset, result, 0, len);

        // if we were using a sub-array we can switch to using this copy.
        if (!copy) {
            b = result;
            offset = 0;
        }

        return result;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized InputStream getStream() {
        return new ByteArrayInputStream(b, offset, len);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendToStream(OutputStream sendTo) throws IOException {
        byte[] sending;
        int sendingOffset;

        // locals enable us to reduce the time which the object is synchronized.
        synchronized (this) {
            sending = b;
            sendingOffset = offset;
        }

        sendTo.write(sending, sendingOffset, len);
    }

    /**
     * Returns the contents of this element as a byte array. If this elements
     * was originally constructed from a intact byte array, the array returned
     * is a "shared" copy of the byte array used by this element. If this
     * element was constructed with an offset of other than zero and a length
     * different than the length of the source array then this function <b>WILL
     * RETURN A COPY OF THE BYTE ARRAY</b>.
     *
     * @return a byte array containing the contents of this element.
     */
    public byte[] getBytes() {
        return getBytes(false);
    }
}
