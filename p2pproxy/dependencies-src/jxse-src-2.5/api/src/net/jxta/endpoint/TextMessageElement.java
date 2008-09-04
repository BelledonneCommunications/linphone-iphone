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
import net.jxta.document.TextDocument;
import net.jxta.logging.Logging;
import net.jxta.util.CountingWriter;
import net.jxta.util.DevNullWriter;

import java.io.IOException;
import java.io.Reader;
import java.io.Writer;
import java.lang.ref.SoftReference;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * An extension of MessageElement for managing elements that are composed of
 * character text. (as opposed to raw bytes).
 * <p/>
 * The data contained within a {@code TextMessageElement} is accessible in three
 * additional ways to those provided by {@code MessageElement} :
 * <p/>
 * <ul>
 * <li>As an {@link java.io.Reader} from {@link #getReader()}</li>
 * <li>Sending the data a {@link java.io.Writer} via {@link #sendToWriter(Writer)}</li>
 * <li>As a char array from  from {@link #getChars(boolean)}</li>
 * </ul>
 *
 */
public abstract class TextMessageElement extends MessageElement implements TextDocument {

    /**
     * Log4J Logger
     */
    private static final Logger LOG = Logger.getLogger(TextMessageElement.class.getName());

    /**
     * cached result of {@link #getCharLength()} operation.
     */
    protected transient long cachedGetCharLength = -1;

    /**
     * cached result of {@link #getBytes(boolean)} operation.
     */
    protected transient SoftReference<char[]> cachedGetChars = null;

    /**
     * Internal constructor for initializaing everything but the data.
     *
     * @param name Name of the Element. May be the empty string ("") if
     *             the Element is not named.
     * @param type Type of the Element. null is equivalent to specifying
     *             the type "Application/Octet-stream"
     * @param sig  optional message digest/digital signature elemnent. If
     *             no signature is to be specified, pass null.
     */
    protected TextMessageElement(String name, MimeMediaType type, MessageElement sig) {
        super(name, type, sig);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * synchronized for caching purposes.
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

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("creating toString of " + getClass().getName() + '@' + Integer.toHexString(hashCode()));
        }

        StringBuilder theString = new StringBuilder();

        try {
            Reader asString = getReader();
            char[] characters = new char[256];

            do {
                int res = asString.read(characters);

                if (res < 0) {
                    break;
                }

                theString.append(characters, 0, res);
            } while (true);

            result = theString.toString();

            cachedToString = new SoftReference<String>(result);
            return result;
        } catch (IOException caught) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Could not generate string for element. ", caught);
            }

            throw new IllegalStateException("Could not generate string for element. " + caught);
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * We use the "text" extension and leave it to sub-classes to extend
     * this.
     */
    @Override
    public String getFileExtension() {
        return "txt";
    }

    /**
     * Returns the size of the element data in characters
     * <p/>
     * synchronized for caching purposes.
     *
     * @return long containing the size of the element data.
     */
    public synchronized long getCharLength() {
        if (cachedGetCharLength >= 0) {
            return cachedGetCharLength;
        }

        CountingWriter countChars = new CountingWriter(new DevNullWriter());

        try {
            sendToWriter(countChars);
            cachedGetByteLength = countChars.getCharsWritten();
            return cachedGetByteLength;
        } catch (IOException caught) {
            throw new IllegalStateException("Could not get length of element : " + caught.toString());
        }
    }

    /**
     * Returns a char array which contains the element data. The char array
     * returned <b>may be shared amongst all copies of the element</b>,
     * do not modify it. The <tt>copy</tt> parameter allows you to request a
     * private, modifiable copy of the element data.
     * <p/>
     * synchronized for caching purposes.
     *
     * @param copy return a copy if true
     * @return char[] Contents of message element.
     */
    public synchronized char[] getChars(boolean copy) {
        char[] result;

        if (null != cachedGetChars) {
            result = cachedGetChars.get();

            if (null != result) {
                if (copy) {
                    char[] theCopy = new char[result.length];

                    System.arraycopy(theCopy, 0, result, 0, result.length);
                } else {
                    return result;
                }
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("creating getChars of " + getClass().getName() + '@' + Integer.toHexString(hashCode()));
        }

        long len = getCharLength();

        if (len > Integer.MAX_VALUE) {
            throw new IllegalStateException("MessageElement is too large to be stored in a char array.");
        }

        result = new char[(int) len];

        try {
            Reader reader = getReader();

            int toRead = (int) len;
            int offset = 0;

            do {
                int read = reader.read(result, offset, toRead);

                if (-1 == read) {
                    break;
                }

                toRead -= read;
                offset += read;
            } while (toRead < len);

            if (toRead != 0) {
                IOException failure = new IOException("Unexpected EOF");

                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, failure.getMessage(), failure);
                }

                throw failure;
            }
        } catch (IOException caught) {
            IllegalStateException failure = new IllegalStateException("Failed to get bytes of Message Element");

            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, failure.getMessage(), caught);
            }
            throw failure;
        }

        // if this is supposed to be a shared buffer then we can cache it.
        if (!copy) {
            cachedGetChars = new SoftReference<char[]>(result);
        }

        return result;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This version probably has sub-optimal performance. Sub-classes
     * should override this implementation if possible.
     */
    public void sendToWriter(Writer sendTo) throws IOException {
        copyReaderToWriter(getReader(), sendTo);
    }

    /**
     * Copies a reader to a writer with buffering.
     *
     * @param source The reader to copy from.
     * @param sink   The writer to send the data to.
     * @throws IOException if there is a problem copying the data
     */
    private void copyReaderToWriter(Reader source, Writer sink) throws IOException {
        int c;
        char[] buf = new char[4096];

        do {
            c = source.read(buf);

            if (-1 == c) {
                break;
            }

            sink.write(buf, 0, c);
        } while (true);
    }
}
