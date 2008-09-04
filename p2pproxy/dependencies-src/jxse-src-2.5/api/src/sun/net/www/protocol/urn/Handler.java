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

package sun.net.www.protocol.urn;


import java.net.URL;
import java.net.URLConnection;
import java.net.URLStreamHandler;

import java.io.IOException;


/**
 * Handler for URN
 *
 * @deprecated Use the URI interfaces for JXTA IDs instead of the URLs.
 */
@Deprecated
public final class Handler extends URLStreamHandler {
    
    public static Handler handler = new Handler();
    
    /**
     * Creates new Handler
     **/
    public Handler() {}
    
    /**
     *
     **/
    @Override
    public URLConnection openConnection(URL connect) throws
                IOException {
        return null;
    }
    
    /**
     *
     *  Private replacement for toHexString since we need the leading 0 digits.
     *  Returns a String containing byte value encoded as 2 hex characters.
     *
     *  @param  theByte a byte containing the value to be encoded.
     *  @return	String containing byte value encoded as 2 hex characters.
     */
    private static String toHexDigits(byte theByte) {
        final char[] HEXDIGITS = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        StringBuilder result = new StringBuilder(2);
        
        result.append(HEXDIGITS[(theByte >>> 4) & 15]);
        result.append(HEXDIGITS[theByte & 15]);
        
        return result.toString();
    }
    
    /**
     *
     * 2.4 of RFC2141 says we have to encode these chars.
     *
     **/
    static final String needsEncoding = "%/?#" + "\\\"&<>[]^`{|}~";
    
    /**
     *
     * The byte values of the chars we have to encode.
     *
     **/
    static final byte[] encodesTo = new byte[] {
        0x25, 0x2F, 0x3F, 0x23, 0x5c, 0x22, 0x26, 0x3C, 0x3E, 0x5B, 0x5D, 0x5E, 0x60, 0x7B, 0x7C, 0x7D, 0x7E
    };
    
    /**
     *  Encode a string such that it is in a form acceptable for presentation
     *  as a URN. First the string is encoded as UTF8 so that any high byte
     *  unicode chars are ascii representable. Then any special characters in
     *  the string are escaped using the URN % syntax.
     *
     *  @param source   the string to encode
     *  @return String containing the URN acceptable presentation form.
     **/
    public static String encodeURN(String source) {
        String asISO8559_1 = null;
        
        try {
            // first we get its bytes using UTF to encode its characters.
            byte[] asBytes = source.getBytes("UTF8");
            
            // then read it back in as ISO-8859-1. This allows us to see the
            // bytes with no translation. This string will have chars in the
            // range 0-255 only.
            asISO8559_1 = new String(asBytes, "ISO-8859-1");
        } catch (java.io.UnsupportedEncodingException never) {
            // these 2 encodings are required by all java implementations
            // so this exception will never happen.
            ;
        }
        
        StringBuilder result = new StringBuilder(asISO8559_1.length());
        
        // now do the % encoding for all chars which need it.
        for (int eachChar = 0; eachChar < asISO8559_1.length(); eachChar++) {
            char aChar = asISO8559_1.charAt(eachChar);
            
            // null char is bad
            if (0 == aChar) {
                throw new IllegalArgumentException("URN string cannot contain null char");
            }
            
            // in the excluded range
            if ((aChar <= 32) || (aChar >= 127)) {
                result.append('%');
                result.append(toHexDigits((byte) aChar));
            } else {
                int inSpecials = needsEncoding.indexOf(aChar);
                
                // one of the special chars which must be encoded?
                if (-1 != inSpecials) {
                    result.append('%');
                    result.append(toHexDigits(encodesTo[inSpecials]));
                } else {
                    result.append(aChar);
                } // needed no encoding
            }
        }
        
        return result.toString();
    }
    
    /**
     *  Converts a string which was previously conveted to URN format back into
     *  the unencoded format.
     *
     *  @param source   the string to decode
     *  @return String containing the decoded form of the URN.
     **/
    public static String decodeURN(String source) {
        StringBuilder result = new StringBuilder(source.length());

        // remove the % encoding for all chars which needed it.
        for (int eachChar = 0; eachChar < source.length(); eachChar++) {
            char aChar = source.charAt(eachChar);

            if ('%' != aChar) {
                result.append(aChar);
            } else {
                String twoChars = source.substring(eachChar + 1, eachChar + 3);

                result.append((char) Integer.parseInt(twoChars, 16));
                eachChar += 2;
            }
        }
        String fromUTF8 = null;

        try {
            // first we get its bytes using ISO-8859-1 to encode its characters.
            // ISO-8859-1 does no mapping. Each byte is the same as the character.
            byte[] asBytes = result.toString().getBytes("ISO-8859-1");

            // then read it back in as UTF8. This gets us any high byte chars back
            fromUTF8 = new String(asBytes, "UTF8");
        } catch (java.io.UnsupportedEncodingException never) {
            // these 2 encodings are required so this exception will never happen
            ;
        }
        return fromUTF8;
    }
}
