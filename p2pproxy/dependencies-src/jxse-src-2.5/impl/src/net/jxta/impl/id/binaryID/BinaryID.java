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

package net.jxta.impl.id.binaryID;


import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.Serializable;


/**
 * A <code>BinaryID</code> is a 256-byte, identifier.
 * This class should be immutable so that it is thread safe.
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 * @see net.jxta.id.ID
 * @see net.jxta.id.IDFactory
 */

public class BinaryID implements Serializable {

    private final static transient Logger LOG = Logger.getLogger(BinaryID.class.getName());
    public static String UUIDEncoded = "uuid";
    public final static int flagsSize = 1;

    public final static byte flagPeerGroupID = 'a';
    public final static byte flagPeerID = 'b';
    public final static byte flagPipeID = 'c';

    public final static byte flagModuleClassID = 'd';
    public final static byte flagModuleClassRoleID = 'e';

    public final static byte flagModuleSpecID = 'f';
    public final static byte flagCodatID = 'g';
    public final static byte flagGenericID = 'z';

    /**
     * location of the byte designating its type.
     */
    public final static int flagsOffset = 0;

    /**
     * location of the byte where the data starts.
     */
    public final static int dataOffset = 1;

    /**
     * Null id contents.
     */
    private static final byte[] nullID = { 0};
    public static final BinaryID nullBinaryID = new BinaryID(flagGenericID, nullID, true);

    /**
     * Array that holds the length and the value of the id in base64 format. This is the default format
     * rather than binary byte because it saves time converting. Odds of returning the actual binary are
     * low so it is done on demand only. Callers of the toByteArray() method should consider the cost of
     * decoding if it is to be called often.
     */
    protected String encodedValue = null;

    /**
     * Creates a null value ID.
     */
    public BinaryID() {
        this.encodedValue = nullBinaryID.encodedValue; // (flagGenericID, nullID, true); 
    }

    /**
     * Creates zero content ID of a specific type.
     */
    public BinaryID(byte id) {
        this(id, nullID, true);
    }

    /**
     * Creates a ID from a string. Note that the ID is not currently validated.
     *
     * @param encodedValue Value to convert ID.
     */
    protected BinaryID(String encodedValue) {
        this.encodedValue = encodedValue;
    }

    /**
     * Simple constructor that takes a binary array to signify the contents of the array.
     *
     * @param type           The Type of ID. Valid values: flagPeerGroupID,flagPeerID,flagPipeID,flagGenericID
     * @param data           the array of up to 256 bytes. Max is 256 if lengthIncluded is true or 255 if the first byte is the length-1.
     * @param lengthIncluded Boolean that signifies if the first byte is the length of the bytes to follow.
     * @throws RuntimeException Runtime exception trhown if array is not correct or if included, the array length does not match actual size.
     */

    public BinaryID(byte type, byte data[], boolean lengthIncluded) {
        if (lengthIncluded && data.length < 256 && data.length > 1) {
            if (data[0] == data.length - 1) {
                try {
                    java.io.StringWriter base64 = new java.io.StringWriter();
                    net.jxta.impl.util.BASE64OutputStream encoder = new net.jxta.impl.util.BASE64OutputStream(base64);

                    encoder.write(data);
                    encoder.close();

                    encodedValue = ((char) type) + base64.toString();
                } catch (Exception e) {
                    LOG.log(Level.SEVERE, "Unable to encode binary value.", e);
                    throw new RuntimeException("Unable to encode binary value.");
                }
            } else {
                throw new RuntimeException(
                        "Length of data section is " + (data.length - 1) + " but byte zero says length is:" + data[0] + ".");
            }
        } else if (!lengthIncluded && data.length > 0) {
            byte temp[] = new byte[data.length + 1];

            temp[0] = (byte) data.length;
            System.arraycopy(data, 0, temp, 1, data.length);
            try {
                java.io.StringWriter base64 = new java.io.StringWriter();
                net.jxta.impl.util.BASE64OutputStream encoder = new net.jxta.impl.util.BASE64OutputStream(base64);

                encoder.write(temp);
                encoder.close();

                encodedValue = ((char) type) + base64.toString();
            } catch (Exception e) {
                LOG.log(Level.SEVERE, "Unable to encode binary value.", e);
                throw new RuntimeException("Unable to encode binary value.");
            }
        } else if (lengthIncluded && (data.length > 256 || data.length == 0)) {
            throw new RuntimeException("Length of 'data' is " + data.length + " must be >0 and less or equal to 256.");
        } else if (!lengthIncluded && data.length > 255) {
            throw new RuntimeException("Length of 'data' is " + data.length + "  must be less than 256. ");
        }
    }

    /**
     * Returns the value of the ID as a binary array. This is always decoded from the base64
     * string rather than caching of the binary array. Callers of the toByteArray() method
     * should consider the cost of decoding if the method is called often.
     *
     * @return returns the data part of the array.
     */
    public byte[] toByteArray() {
        try {
            java.io.ByteArrayOutputStream bos = new java.io.ByteArrayOutputStream();
            net.jxta.impl.util.BASE64InputStream decoder = new net.jxta.impl.util.BASE64InputStream(
                    new java.io.StringReader(encodedValue.substring(1)));

            while (true) {
                int c = decoder.read();

                if (-1 == c) {
                    break;
                }

                bos.write(c);
            }

            return bos.toByteArray();
        } catch (Exception e) {
            LOG.log(Level.SEVERE, "Unable to decode binary value.", e);
            throw new RuntimeException("Unable to encode binary value.");
        }

    }

    /**
     * Returns the value of the ID as a binary array without the size in byte zero. This is always decoded from the base64
     * string rather than caching of the binary array. Callers of the toByteArray() method
     * should consider the cost of decoding if the method is called often.<p>
     * <p/>
     * Note that we assume the array size-1 equals the contents of byte zero.
     *
     * @return returns the array with the first byte as the length of the remaining bytes.
     */
    public byte[] toSizeIncludedByteArray() {
        byte[] data = toByteArray();

        byte temp[] = new byte[data.length - 1];

        System.arraycopy(data, 1, temp, 0, temp.length);

        return temp;
    }

    /**
     * @return The ID which consists of a character designating type, followed by the base64 encoded value of the size and array of bytes.
     */
    public String encodedValue() {
        return encodedValue;
    }

    /**
     * Returns the hash code of the BinaryID<p>
     * <p/>
     * WARNING: Do not use this hash as a network ID. Use a stronger digest hash like SHA-1 to get the hash of the contents.
     *
     * @return int hashcode
     */

    @Override
    public int hashCode() {
        return encodedValue.hashCode();
    }

    /**
     * Compares two BinaryIDs for equality.<p>
     * true: taget == this<p>
     * false: target == null<p>
     * true: taget.encodedValue == this.encodedValue<p>
     * true: target instance of ID && ID==ID.nullID && nullBinaryID.encodedValue().equals( encodedValue())<p>
     * false: all other posibilities<p>
     *
     * @param target the BidaryID to be compared against.
     * @return boolean true if IDs are equal, false otherwise.
     */
    @Override
    public boolean equals(Object target) {
        boolean result = false;

        if (target == this) {
            result = true;
        } else if (target == null) {
            result = false;
        } else if (target instanceof BinaryID) {
            result = encodedValue().equals(((BinaryID) target).encodedValue());
            LOG.fine("((BinaryID)target).encodedValue():" + ((BinaryID) target).encodedValue());
        } else if (target instanceof net.jxta.id.ID && ((net.jxta.id.ID) target) == net.jxta.id.ID.nullID
                && nullBinaryID.encodedValue().equals(encodedValue())) {
            result = true;
        }
        // LOG.error("this:"+encodedValue()+" type:"+target.getClass().getName()+" target:"+target+" equals:"+result,new RuntimeException("test exception")); 
        return result;
    }

    /**
     * Return the type of ID.
     *
     * @return byte value designating type.
     */

    public byte type() {
        return (byte) encodedValue.charAt(0);
    }

    /**
     * Returns base 64 encoded value.
     *
     * @return String return value
     */

    @Override
    public String toString() {
        return encodedValue;
    }

    /**
     * returns the raw encoded value. Not cloned because it is a string.
     */
    public String getID() {
        return encodedValue;
    }
}
