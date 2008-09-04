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


import java.io.OutputStream;
import java.io.Writer;

import java.io.IOException;
import java.io.StringReader;
import java.io.ByteArrayOutputStream;
import java.io.StringWriter;


/**
 * An OutputStream implementation which encodes the written bytes into BASE64
 * encoded character data and writes the output to an associated text Writer.
 *
 * <p/>This implementation is not thread safe.
 *
 * @see net.jxta.impl.util.BASE64InputStream
 * @see <a href="http://www.ietf.org/rfc/rfc2045.txt" target="_blank">IETF RFC 2045 <i>MIME : Format of Internet Message Bodies</i></a>
 **/
public class BASE64OutputStream extends OutputStream {
    
    /**
     *  If <code>true</code> then the output stream has been closed.
     **/
    private boolean closed = false;
    
    /**
     *  The output writer.
     **/
    private Writer sendTo = null;
    
    /**
     *  Column width to breakup out.
     **/
    private final int columnWidth;
    
    /**
     *  Current output column.
     **/
    private int column = 0;
    
    /**
     *  Buffer for incomplete characters.
     **/
    private byte[] buffer = new byte[] { 0, 0, 0 };
    
    /**
     *  The number of characters currently in the buffer.
     **/
    private byte inBuffer = 0;
    
    /**
     * Construct a BASE64 Output Stream.
     *
     * @param sendTo The text Writer to which the BASE64 output will be
     * written.
     **/
    public BASE64OutputStream(Writer sendTo) {
        this(sendTo, -1);
    }
    
    /**
     * Construct a BASE64 Output Stream. The output will be broken into lines
     * <code>columnWidth</code> long.
     *
     * @param sendTo The text Writer to which the BASE64 output will be
     * written.
     * @param columnWidth The width of lines to break output into.
     **/
    public BASE64OutputStream(Writer sendTo, int columnWidth) {
        this.sendTo = sendTo;
        this.columnWidth = columnWidth;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public void write(int b) throws IOException {
        if (closed) {
            throw new IOException("OutputStream closed.");
        }
        
        buffer[ inBuffer++ ] = (byte) b;
        
        if (buffer.length == inBuffer) {
            writeBuffer();
        }
    }
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>The output writer is <b>NOT</b> closed.
     **/
    @Override
    public void close() throws IOException {
        flush();
        
        closed = true;
        sendTo = null;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public void flush() throws IOException {
        writeBuffer();
    }
    
    /**
     *  Write a full or partial buffer to the output writer.
     **/
    private void writeBuffer() throws IOException {
        if (0 == inBuffer) {
            return;
        }
        
        int val = ((buffer[0] & 0x00FF) << 16) + ((buffer[1] & 0x00FF) << 8) + (buffer[2] & 0x00FF);
        
        int c0 = (val >> 18) & 0x003F;
        int c1 = (val >> 12) & 0x003F;
        int c2 = (val >> 6) & 0x003F;
        int c3 = val & 0x003F;
        
        if ((columnWidth > 0) && (column > columnWidth)) {
            sendTo.write('\n');
            column = 0;
        }
        
        sendTo.write(encodeSixBits(c0));
        sendTo.write(encodeSixBits(c1));
        
        if (inBuffer > 1) {
            sendTo.write(encodeSixBits(c2));
        } else {
            sendTo.write('=');
        }
        
        if (inBuffer > 2) {
            sendTo.write(encodeSixBits(c3));
        } else {
            sendTo.write('=');
        }
        
        buffer[0] = 0;
        buffer[1] = 0;
        buffer[2] = 0;
        
        inBuffer = 0;
        column += 4;
    }
    
    /**
     *  BASE64 Encoding Table
     **/
    static final char encode[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X'
                ,
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v'
                ,
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };
    
    /**
     *  Encode six bits into a character using the standard BASE64 table.
     *
     *  @param b the bits to encode. b must be >=0 and <= 63
     *  @return the appropriate character for the input value.
     **/
    private static char encodeSixBits(int b) {
        char c;
        
        if ((b < 0) || (b > 63)) {
            throw new IllegalArgumentException("bad encode value");
        } else {
            c = encode[b];
        }
        
        return c;
    }
}
