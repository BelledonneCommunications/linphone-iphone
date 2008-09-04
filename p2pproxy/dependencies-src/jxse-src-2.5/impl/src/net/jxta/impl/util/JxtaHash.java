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


import java.util.*;
import java.io.File;
import java.math.BigInteger;
import java.io.InputStream;
import java.io.IOException;
import java.io.ByteArrayInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;


/**
 *  A message digest wrapper to provide hashing using java.security.MesssageDigest
 */
public class JxtaHash {
    private final static Logger LOG = Logger.getLogger(JxtaHash.class.getName());
    public final static String SHA = "SHA";
    public final static String SHA1 = "SHA1";
    public final static String MD2 = "MD2";
    public final static String MD5 = "MD5";
    public final static String DSA = "DSA";
    public final static String RSA = "RSA";
    public final static String SHA1withDSA = "SHA1WITHDSA";
    private MessageDigest dig = null;

    /**
     * Default JxtaHash constructor, with the default algorithm SHA1
     *
     */
    public JxtaHash() {
        try {
            dig = MessageDigest.getInstance(SHA1);
        } catch (NoSuchAlgorithmException ex) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(ex.toString());
            }
        }
    }

    /**
     * Default JxtaHash constructor, with the default algorithm SHA1
     *
     * @param  expression  message to hash
     */
    public JxtaHash(String expression) {
        this(SHA1, expression);
    }

    /**
     * Constructor for the JxtaHash object
     *
     * @deprecated This implementation may produce inconsistent results
     * based upon varience of the locale. (The locale of getBytes() is
     * not defined).
     *
     * @param  algorithm   algorithm - the name of the algorithm requested
     * @param  expression  expression to digest
     */
    @Deprecated
    public JxtaHash(String algorithm, String expression) {
		
        this(algorithm, expression.getBytes());
    }

    /**
     * Constructor for the JxtaHash object
     *
     * @param  algorithm   algorithm - the name of the algorithm requested
     * @param  expression  expression to digest
     */
    public JxtaHash(String algorithm, byte[] expression) {
        try {
            dig = MessageDigest.getInstance(algorithm);
            if (expression != null) {
                dig.update(expression);
            }
        } catch (NoSuchAlgorithmException ex) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(ex.toString());
            }
        }
    }

    /**
     * Constructor for the JxtaHash object
     *
     * @param  expression  expression to digest
     */
    public void update(String expression) {
        if (expression != null) {
            dig.update(expression.getBytes());
        }
    }

    /**
     *  Gets the digest as digestInteger
     *
     * @return    The digestInteger value
     */
    public BigInteger getDigestInteger() {

        return new BigInteger(dig.digest());
    }

    /**
     *  Gets the digest as digestInteger
     *
     * @param  expression  expression to digest
     * @return             The digestInteger value
     */
    public BigInteger getDigestInteger(byte[] expression) {
        dig.reset();
        dig.update(expression);
        return new BigInteger(dig.digest());
    }

    /**
     *  Gets the digest as digestInteger
     *
     * @param  expression  expression to digest
     * @return             The digestInteger value
     */
    public BigInteger getDigestInteger(String expression) {

        return getDigestInteger(expression.getBytes());
    }

    /**
     *   Returns a int whose value is (getDigestInteger mod m).
     *
     * @param  m  the modulus.
     * @return    (getDigestInteger mod m).
     */
    public int mod(long m) {
        BigInteger bi = getDigestInteger();
        BigInteger mod = new BigInteger(longToBytes(m));
        BigInteger result = bi.mod(mod);

        return result.intValue();
    }

    /**
     *  convert a long into the byte array
     *
     * @param  value  long value to convert
     * @return        byte array
     */
    private byte[] longToBytes(long value) {
        byte[] bytes = new byte[8];

        for (int eachByte = 0; eachByte < 8; eachByte++) {
            bytes[eachByte] = (byte) (value >> ((7 - eachByte) * 8L));
        }
        return bytes;
    }

}

