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

package net.jxta.util;


import java.io.ByteArrayInputStream;
import java.io.FilterInputStream;
import java.io.InputStream;

import java.io.IOException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;


/**
 *  Implements a bounds on the number of bytes which may be read from an
 *  InputStream. {link LimitInputStream.close() close()} does not close the
 *  underlying stream.
 **/
public class LimitInputStream extends FilterInputStream {
    
    /*
     *  Log4J Catagory
     */
    private static final Logger LOG = Logger.getLogger(LimitInputStream.class.getName());
    
    private transient long limit;
    
    private transient long read;
    
    private transient long mark;
    
    private transient boolean fatalUnderflow;
    
    private transient boolean alreadycounting;
    
    /**
     * Creates a new instance of LimitInputStream
     *
     *    @param  in  the stream which will be limited.
     *    @param  limit   the number of bytes which can be read from the stream.
     **/
    public LimitInputStream(InputStream in, long limit) {
        this(in, limit, false);
    }
    
    /**
     * Creates a new instance of LimitInputStream
     *
     *    @param  in  the stream which will be limited.
     *    @param  limit   the number of bytes which can be read from the stream.
     *    @param  underflowThrows if the underlying stream EOFs before limit then
     *    an IOException will be thrown.
     **/
    public LimitInputStream(InputStream in, long limit, boolean underflowThrows) {
        super(in);
        this.limit = limit;
        this.mark = -1;
        this.read = 0;
        this.fatalUnderflow = underflowThrows;
        this.alreadycounting = false;
    }
    
    /**
     * {@inheritDoc}
     *
     *  <p/>Debugging toString.
     **/
    @Override
    public String toString() {
        if (null == in) {
            return "closed/" + super.toString();
        } else {
            if (in instanceof ByteArrayInputStream) {
                // ByteArrayInputStream.toString() prints the entire stream! 
                return in.getClass().getName() + "@" + System.identityHashCode(in) + "/" + super.toString() + ":" + read + "-"
                        + limit;
            } else {
                return in.toString() + "/" + super.toString() + ":" + read + "-" + limit;
            }
        }
    }
    
    /**
     * Closes this input stream and releases any system resources
     * associated with the stream.
     * <p/>
     * This method simply forgets the underlying stream.
     *
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FilterInputStream#in
     */
    @Override
    public void close() throws IOException {
        in = null;
    }
    
    /**
     * Returns the number of bytes that can be read from this input
     * stream without blocking.
     * <p>
     * This method
     * simply performs <code>in.available(n)</code> and
     * returns the result.
     *
     * @return     the number of bytes that can be read from the input stream
     *             without blocking.
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FilterInputStream#in
     */
    @Override
    public int available() throws IOException {
        if (null == in) {
            throw new IOException("Stream has been closed.");
        }
        
        return (int) Math.min(super.available(), (limit - read));
    }
    
    /**
     * Marks the current position in this input stream. A subsequent
     * call to the <code>reset</code> method repositions this stream at
     * the last marked position so that subsequent reads re-read the same bytes.
     * <p>
     * The <code>readlimit</code> argument tells this input stream to
     * allow that many bytes to be read before the mark position gets
     * invalidated.
     * <p>
     * This method simply performs <code>in.mark(readlimit)</code>.
     *
     * @param   readlimit   the maximum limit of bytes that can be read before
     *                      the mark position becomes invalid.
     * @see     java.io.FilterInputStream#in
     * @see     java.io.FilterInputStream#reset()
     */
    @Override
    public void mark(int readlimit) {
        if (null == in) {
            return;
        } // don't throw exception to be consistent with other impls.
        
        super.mark(readlimit);
        mark = read;
    }
    
    /**
     * Repositions this stream to the position at the time the
     * <code>mark</code> method was last called on this input stream.
     * <p>
     * This method simply performs <code>in.reset()</code>.
     * <p>
     * Stream marks are intended to be used in
     * situations where you need to read ahead a little to see what's in
     * the stream. Often this is most easily done by invoking some
     * general parser. If the stream is of the type handled by the
     * parse, it just chugs along happily. If the stream is not of
     * that type, the parser should toss an exception when it fails.
     * If this happens within readlimit bytes, it allows the outer
     * code to reset the stream and try another parser.
     *
     * @exception  IOException  if the stream has not been marked or if the
     *               mark has been invalidated.
     * @see        java.io.FilterInputStream#in
     * @see        java.io.FilterInputStream#mark(int)
     */
    @Override
    public void reset() throws IOException {
        if (null == in) {
            throw new IOException("Stream has been closed.");
        }
        
        if (-1 == mark) {
            throw new IOException("reset() without mark(), or I dont know where mark is");
        }
        
        super.reset();
        
        read = mark;
    }
    
    /**
     * Skips over and discards <code>n</code> bytes of data from the
     * input stream. The <code>skip</code> method may, for a variety of
     * reasons, end up skipping over some smaller number of bytes,
     * possibly <code>0</code>. The actual number of bytes skipped is
     * returned.
     * <p>
     * This method
     * simply performs <code>in.skip(n)</code>.
     *
     * @param      n   the number of bytes to be skipped.
     * @return     the actual number of bytes skipped.
     * @exception  IOException  if an I/O error occurs.
     */
    @Override
    public synchronized long skip(long n) throws IOException {
        if (null == in) {
            throw new IOException("Stream has been closed.");
        }
        
        long skipLen = Math.min(n, (limit - read));
        
        boolean wascounting = alreadycounting;

        alreadycounting = true;
        long result = super.skip(skipLen);

        alreadycounting = wascounting;
        
        if ((-1 != result) && !alreadycounting) {
            read += result;
        }
        
        return result;
    }
    
    /**
     * Reads the next byte of data from this input stream. The value
     * byte is returned as an <code>int</code> in the range
     * <code>0</code> to <code>255</code>. If no byte is available
     * because the end of the stream has been reached, the value
     * <code>-1</code> is returned. This method blocks until input data
     * is available, the end of the stream is detected, or an exception
     * is thrown.
     * <p>
     * This method
     * simply performs <code>in.read()</code> and returns the result.
     *
     * @return     the next byte of data, or <code>-1</code> if the end of the
     *             stream is reached.
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FilterInputStream#in
     */
    @Override
    public synchronized int read() throws IOException {
        if (null == in) {
            throw new IOException("Stream has been closed.");
        }
        
        if (read >= limit) {
            return -1;
        }
        
        boolean wascounting = alreadycounting;

        alreadycounting = true;
        int result = super.read();

        alreadycounting = wascounting;
        
        if (!alreadycounting) {
            if (-1 != result) {
                read++;
            } else {
                if (fatalUnderflow && (read != limit)) {
                    IOException failed = new IOException(
                            "Underflow in read, stream EOFed at " + read + " before limit of " + limit);

                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, failed.getMessage(), failed);
                    }
                    throw failed;
                }
            }
        }
        
        return result;
    }
    
    /**
     * Reads up to <code>len</code> bytes of data from this input stream
     * into an array of bytes. This method blocks until some input is
     * available.
     * <p>
     * This method simply performs <code>in.read(b, off, len)</code>
     * and returns the result.
     *
     * @param      b     the buffer into which the data is read.
     * @param      off   the start offset of the data.
     * @param      len   the maximum number of bytes read.
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the stream has been reached.
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FilterInputStream#in
     */
    @Override
    public synchronized int read(byte[] b, int off, int len) throws IOException {
        if (null == in) {
            throw new IOException("Stream has been closed.");
        }
        
        if (read >= limit) {
            return -1;
        }
        
        int readLen = (int) Math.min(len, limit - read);
        
        boolean wascounting = alreadycounting;

        alreadycounting = true;
        int result = super.read(b, off, readLen);

        alreadycounting = wascounting;
        
        if (!alreadycounting) {
            if (-1 != result) {
                read += result;
            } else {
                if (fatalUnderflow && (read != limit)) {
                    IOException failed = new IOException(
                            "Underflow while tring to read " + readLen + ", stream EOFed at " + read + " before limit of " + limit);

                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, failed.getMessage(), failed);
                    }
                    throw failed;

                }
            }
        }
        
        return result;
    }
}
