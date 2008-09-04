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
package net.jxta.socket;


import java.io.IOException;
import java.io.OutputStream;
import java.net.SocketException;


/**
 * This class implements a buffered output stream. By setting up such an output
 * stream, an application can write bytes to the underlying output stream
 * without necessarily causing a call to the underlying system for each byte
 * written. Data buffer is flushed to the underlying stream, when it is full,
 * or an explicit call to flush is made.
 */
class JxtaSocketOutputStream extends OutputStream {

    /**
     * If {@code true} then this socket is closed.
     */
    protected boolean closed = false;

    /**
     * Data buffer
     */
    protected byte buf[];

    /**
     * byte count in buffer
     */
    protected int count;

    /**
     * JxtaSocket associated with this stream
     */
    protected JxtaSocket socket;

    /**
     * Constructor for the JxtaSocketOutputStream object
     *
     * @param socket JxtaSocket associated with this stream
     * @param size   buffer size in bytes
     */
    public JxtaSocketOutputStream(JxtaSocket socket, int size) {
        if (size <= 0) {
            throw new IllegalArgumentException("Buffer size <= 0");
        }
        buf = new byte[size];
        this.socket = socket;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void close() throws IOException {
        flushBuffer();
        closed = true;
    }

    /**
     * Similar to close except that any buffered data is discarded.
     */
    synchronized void hardClose() {
        count = 0;
        closed = true;
    }

    /**
     * Flush the internal buffer
     *
     * @throws IOException if an i/o error occurs
     */
    private void flushBuffer() throws IOException {
        if (count > 0) {
            // send the message
            socket.write(buf, 0, count);
            count = 0;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void write(int b) throws IOException {

        if (closed) {
            throw new SocketException("Socket Closed.");
        }

        if (count >= buf.length) {
            flushBuffer();
        }
        buf[count++] = (byte) b;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void write(byte b[], int off, int len) throws IOException {
        int left = buf.length - count;

        if (closed) {
            throw new SocketException("Socket Closed.");
        }

        if (len > left) {
            System.arraycopy(b, off, buf, count, left);
            len -= left;
            off += left;
            count += left;
            flushBuffer();
        }

        // chunk data if larger than buf.length
        while (len >= buf.length) {
            socket.write(b, off, buf.length);
            len -= buf.length;
            off += buf.length;
        }
        System.arraycopy(b, off, buf, count, len);
        count += len;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void flush() throws IOException {
        flushBuffer();
    }
}

