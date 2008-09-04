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

package net.jxta.impl.util;


import java.io.InputStream;
import java.io.Reader;

import java.io.IOException;

/**
 *  An <code>InputStream</code> implementation which decodes BASE64 encoded
 *  data from a text <code>Reader</code>.
 *
 * <p/>This implementation is not thread safe.
 *
 *  @see net.jxta.impl.util.BASE64OutputStream
 *  @see <a href="http://www.ietf.org/rfc/rfc2045.txt" target="_blank">IETF RFC 2045 <i>MIME : Format of Internet Message Bodies</i></a>
 **/
public class BASE64InputStream extends InputStream {
    
    /**
     *  The input source of BASE64 text data.
     **/
    private Reader source;
    
    /**
     *  If <code>true</code> then the stream has been closed.
     **/
    private boolean closed = false;
    
    /**
     *  If <code>true</code> then the source reader has reached EOF.
     **/
    private boolean atEOF = false;
    
    /**
     *  Buffer for unread but decoded bytes.
     **/
    private byte buffer[] = new byte[3];
    
    /**
     *  Position of current available character in buffer.
     **/
    private byte inBuffer = 3;
    
    /**
     *  Construct InputStream given a source of BASE64 encoded text.
     **/
    public BASE64InputStream(Reader source) {
        this.source = source;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public int available() throws IOException {
        if (closed) {
            throw new IOException("InputStream closed.");
        }
        
        int bufferAvail = buffer.length - inBuffer;
        
        return (bufferAvail > 0) ? bufferAvail : (source.ready() ? 1 : 0);
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public void close() throws IOException {
        closed = true;
        
        source.close();
        source = null;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public void mark(int readLimit) {
        throw new IllegalStateException("Mark not supported.");
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public boolean markSupported() {
        return false;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public int read() throws IOException {
        if (closed) {
            throw new IOException("InputStream closed.");
        }
        
        while (!atEOF) {
            if (inBuffer < buffer.length) {
                return buffer[inBuffer++] & 0x00FF;
            }
            
            inBuffer = 0;
            
            int s0;

            do {
                s0 = source.read();
            } while ((-1 != s0) && Character.isWhitespace((char) s0));
            
            if (-1 == s0) {
                atEOF = true;
                break;
            }
            
            int s1 = source.read();
            int s2 = source.read();
            int s3 = source.read();
            
            if ((-1 == s1) || (-1 == s2) || (-1 == s3)) {
                throw new IOException("Unexpected EOF");
            }
            
            int c0 = decodeSixBits((char) s0);
            int c1 = decodeSixBits((char) s1);
            int c2 = decodeSixBits((char) s2);
            int c3 = decodeSixBits((char) s3);
            
            if ((-1 == c0) || (-1 == c1) || (-1 == c2) || (-1 == c3)) {
                throw new IOException("Bad character in BASE64 data");
            }
            
            if (Integer.MAX_VALUE == c0) {
                throw new IOException("'=' found in first position of BASE64 data");
            }
            
            if (Integer.MAX_VALUE == c1) {
                throw new IOException("'=' found in second position of BASE64 data");
            }
            
            if (Integer.MAX_VALUE == c3) {
                c3 = 0;
                inBuffer++;
            }
            
            if (Integer.MAX_VALUE == c2) {
                c2 = 0;
                inBuffer++;
            }
            
            int val = (c0 << 18) + (c1 << 12) + (c2 << 6) + c3;
            
            buffer[ inBuffer ] = (byte) (val >> 16);
            
            if ((inBuffer + 1) < buffer.length) {
                buffer[ inBuffer + 1 ] = (byte) (val >> 8);
            }
            
            if ((inBuffer + 2) < buffer.length) {
                buffer[ inBuffer + 2 ] = (byte) val;
            }
        }
        
        return -1;
    }
    
    /**
     *
     *  XXX bondolo this would be faster as a table.
     **/
    private static int decodeSixBits(char c) {
        int v;
        
        if ('A' <= c && c <= 'Z') {
            v = (c - 'A');
        } else if ('a' <= c && c <= 'z') {
            v = (c - 'a') + 26;
        } else if ('0' <= c && c <= '9') {
            v = (c - '0') + 52;
        } else if (c == '+') {
            v = 62;
        } else if (c == '/') {
            v = 63;
        } else  if (c == '=') {
            v = Integer.MAX_VALUE;
        } else {
            v = -1;
        }
        
        return v;
    }
}
