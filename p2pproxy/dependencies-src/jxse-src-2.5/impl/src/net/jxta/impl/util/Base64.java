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


import java.io.IOException;
import java.io.StringReader;
import java.io.ByteArrayOutputStream;
import java.io.StringWriter;


/**
 * @deprecated Planned for removal. This implementation has been replaced by
 * {@link net.jxta.impl.util.BASE64InputStream} and 
 * {@link net.jxta.impl.util.BASE64OutputStream}.
 *
 **/
@Deprecated
public final class Base64 {
    
    private Base64() {}
    
    // Base64 encoding.  See Rfc1341
    
    static public byte[] decodeBase64(String text) throws IOException {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        StringReader r = new StringReader(text);
        
        for (;;) {
            char c0 = getBase64Char(r);

            if (c0 == '\0') {
                break;
            }
            char c1 = getBase64Char(r);

            if (c1 == '\0') {
                throw new IOException("binary data not a multiple of four bytes");
            }
            char c2 = getBase64Char(r);

            if (c2 == '\0') {
                throw new IOException("binary data not a multiple of four bytes");
            }
            char c3 = getBase64Char(r);

            if (c3 == '\0') {
                throw new IOException("binary data not a multiple of four bytes");
            }
            
            if (c0 == '=') {
                throw new IOException("'=' found in first position of base64 data");
            }
            if (c1 == '=') {
                throw new IOException("'=' found in second position of base64 data");
            }
            int n = 3;

            if (c2 == '=') {
                n = 1;
                c2 = c3 = 'A'; // So we get a value of 0.
            } else if (c3 == '=') {
                n = 2;
                c3 = 'A'; // So we get a value of 0.
            }
            
            int v = (decodeSixBits(c0) << 18) + (decodeSixBits(c1) << 12) + (decodeSixBits(c2) << 6) + decodeSixBits(c3);
            
            int b0 = (v >> 16) & 0xff;
            int b1 = (v >> 8) & 0xff;
            int b2 = (v) & 0xff;

            os.write(b0);
            if (n >= 2) {
                os.write(b1);
            }
            if (n == 3) {
                os.write(b2);
            }
        }
        
        return os.toByteArray();
    }
    
    /**
     * '\0' represents end of file.
     */
    static private char getBase64Char(StringReader r) throws IOException {
        for (;;) {
            int i = r.read();

            if (i == -1) {
                return '\0';
            }
            
            char c = (char) i;

            if ('A' <= c && c <= 'Z') {
                return c;
            } else if ('a' <= c && c <= 'z') {
                return c;
            } else if ('0' <= c && c <= '9') {
                return c;
            } else if (c == '+') {
                return c;
            } else if (c == '/') {
                return c;
            } else if (c == '=') {
                return c;
            }
            
            // Not a base64 char, loop around and try again.
            
        }
    }
    
    static public String encodeBase64(byte[] bytes) {
        int n = 0;
        StringWriter w = new StringWriter();
        int v;
        int h; // Six bits.  Six, thus h for hex.
        
        int len = bytes.length / 3 * 3;

        for (int i = 0; i < len; i += 3) {
            v = (bytes[i] << 16) + ((bytes[i + 1] & 0xff) << 8) + (bytes[i + 2] & 0xff);
            
            h = (v >> 18) & 0x3f;
            w.write(encodeSixBits(h));
            
            h = ((v >> 12) & 0x3f);
            w.write(encodeSixBits(h));
            
            h = ((v >> 6) & 0x3f);
            w.write(encodeSixBits(h));
            
            h = ((v >> 0) & 0x3f);
            w.write(encodeSixBits(h));
            
            n += 4;
            if (n >= 76) {
                w.write("\r\n");
                n = 0;
            }
            
        }
        
        switch (bytes.length - len) {
        case 0:
            break;

        case 1:
            v = (bytes[len] << 16);
                
            h = (v >> 18) & 0x3f;
            w.write(encodeSixBits(h));
                
            h = (v >> 12) & 0x3f;
            w.write(encodeSixBits(h));
                
            w.write('=');
            w.write('=');
            break;

        case 2:
            v = (bytes[len] << 16) + ((bytes[len + 1] & 0xff) << 8);
                
            h = (v >> 18) & 0x3f;
            w.write(encodeSixBits(h));
                
            h = ((v >> 12) & 0x3f);
            w.write(encodeSixBits(h));
                
            h = ((v >> 6) & 0x3f);
            w.write(encodeSixBits(h));
                
            w.write('=');
            break;
        }
        w.write("\r\n");
        return w.toString();
    }
    
    static private char encodeSixBits(int b) {
        char c;
        
        if (b <= 25) {
            c = (char) ('A' + b);
        } else if (b <= 51) {
            c = (char) ('a' + b - 26);
        } else if (b <= 61) {
            c = (char) ('0' + b - 52);
        } else if (b == 62) {
            c = '+';
        } else {
            // if (b == 63)
            c = '/';
        }
        return c;
    }
    
    static private int decodeSixBits(char c) {
        int v;
        
        if ('A' <= c && c <= 'Z') {
            v = (c - 'A');
        } else if ('a' <= c && c <= 'z') {
            v = (c - 'a') + 26;
        } else if ('0' <= c && c <= '9') {
            v = (c - '0') + 52;
        } else if (c == '+') {
            v = 62;
        } else {
            // if (c == '/')
            v = 63;
        }
        
        return v;
    }
    
    public static void main(String args[]) {
        byte bytes[] = new byte[200];
        
        for (int i = 0; i < bytes.length; ++i) {
            bytes[i] = (byte) i;
        }
        
        System.out.println(encodeBase64(bytes));
    }
}
