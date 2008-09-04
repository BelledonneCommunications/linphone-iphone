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

import java.security.MessageDigest;


/**
 * This is a utility class used to create pipe advertisement named and BinaryID for the pipeID to create
 * a private address space that can be hosted in the public discovery system or sent over unencrypted
 * channeds without revealing their intent or purpose. <p>
 * <p/>
 * We use a one-way hashing algorythum to create an ID from private information like
 * a user's social security number or a user's email address.
 * We search for the pipe by with this private information securly by creating the
 * matching hash using the same methods. <p>
 * <p/>
 * The purpose of this system is to create a way to search
 * for a pipe (or other BinaryID based system) without exposing the
 * pipe owner's clearTextID while allowing for  people that
 * know what they are looking for to find the right pipe. The
 * system also has the ability to create pipes that have a specific purpose.
 * For example, the email address is appended with a function name. Say you
 * have a pipe for messages and one for administrative purposes. You would
 * supply the email and a string for the function. The same combination can be
 * created by another peer to search for either of these pipes. <p>
 * <p/>
 * This implementation uses the "SHA-1" algorythum. This was selected for relitive
 * speed. It is used as a one-way conversion that cannot be reversed engineered to
 * create the original string. This allows you to publish the hash without the
 * possibility of the contents being decoded. This allows for public indexing of
 * data that is only known by the parties involved.<p>
 * <p/>
 * Note that this can also be used to generate safe password verification hash codes.
 * Sample useage:
 * <code>
 * String clearTextID = "turbogeek@cluck.com";
 * String function = "eventPipe";
 * System.out.println("clear text ID: "+clearTextID);
 * System.out.println("function text: "+function);
 * String digest1 = DigestID.generateHashString(clearTextID, function);
 * String digest2 = DigestID.generateHashString(clearTextID);
 * System.out.println("Digest1: '"+digest1+"'");
 * System.out.println("Digest2: '"+digest2+"'");
 * System.out.println("test1: "+DigestID.test(clearTextID, function,digest1));
 * System.out.println("test2: "+DigestID.test(clearTextID, digest2));
 * System.out.println("Digest1 != Digest2: "+DigestID.test(clearTextID, function,digest2));
 * </code><p>
 * <p/>
 * To use an algorythum other than SHA-1, you will need stronger encyption.
 * The BouncyCastle that comes with JXTA is just a minimum implimentation so
 * a good choice is  the normal bouncy castle (it is much larger, nearing a meg,
 * which is why it is not a part of the normal JXTA distribution. The full version
 * of bouncy includes SHA-128, SHA-256, SHA-384, and SHA-512.<p>
 * <p/>
 * Here is how you create a provider from the full version of Bouncy. Once you do this, you can access the extended
 * Digest ecryption levels.
 * <code>
 * provider = new org.bouncycastle.jce.provider.BouncyCastleProvider();
 * System.out.println("provider:"+provider.getName());
 * Security.addProvider(provider);
 * </code><p>
 * Security Note<p>
 * <p/>
 * This class should have all of its fields and properties marked as 'final' to prevent overriding the default behavior.
 * Failure to do so could allow a less scrupulous person to cause the BinaryID or hash codes to contain the original information.
 * Note that the class itself is not final to allow for additional convienience methods to be added. There
 * a no methods for creating ModuleClassBinaryID, ModuleSpecBinaryID, or CodatID because this is meant for general'
 * use, not for extending platform (you can write your own using similar code). <p>
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 * @version $Revision$
 */
public class DigestTool {
    private final static transient Logger LOG = Logger.getLogger(DigestTool.class.getName());

    /**
     * varaible used for conditional compile of debug printing.
     */
    public static final boolean debug = true;

    /**
     * Defualt SHA-1 digest algorithm type. This is a 20 byte hash function (note: that MD5 is only 16 so we don't use it).
     */
    public static final String SHAOne = "SHA-1";

    /**
     * SHA-128 digest algorithm type. This is a 128 bit hash function (note: must have another provider registered to use).
     */
    public static final String SHA128 = "SHA-128";

    /**
     * SHA-256 digest algorithm type. This is a 256 bit hash function (note: must have another provider registered to use).
     */
    public static final String SHA256 = "SHA-256";

    /**
     * SHA-384 digest algorithm type. This is a 384 bit hash function (note: must have another provider registered to use).
     */
    public static final String SHA384 = "SHA-384";

    /**
     * SHA-512 digest algorithm type. This is a 512 bit hash function (note: must have another provider registered to use).
     */
    public static final String SHA512 = "SHA-512";

    /**
     * Tilde character used to seperate candidate strings from a function.
     */
    public final String functionSeperator = "~";
    String algorithmType;

    public DigestTool() {
        algorithmType = SHAOne;
    }

    public DigestTool(String algorithmType) {
        this.algorithmType = algorithmType;
    }

    /**
     * Create a PipeID based on the BinaryID type with a digest of the clearTextID and function.
     *
     * @param peerGroupID Parent peer group ID.
     * @param clearTextID String used as the significant part of the address
     * @param function    String used to diferentiate different clearTextID addresses (can be null).
     * @return PipeBinaryID with the digest hash of the string: clearTextID+"~"+function.
     */
    public final PipeBinaryID createPipeID(net.jxta.peergroup.PeerGroupID peerGroupID, String clearTextID, String function) {
        byte[] digest = generateHash(clearTextID, function);
        PipeBinaryID pipe = new PipeBinaryID(peerGroupID, digest, false);

        return pipe;
    }

    /**
     * Create a PeerGroupID based on the BinaryID type with a digest of the clearTextID and function.
     *
     * @param parentPeerGroupID Parent peer group ID.
     * @param clearTextID       String used as the significant part of the address
     * @param function          String used to diferentiate different clearTextID addresses (can be null).
     * @return PeerGroupBinaryID with the digest hash of the string: clearTextID+"~"+function.
     */
    public final PeerGroupBinaryID createPeerGroupID(net.jxta.peergroup.PeerGroupID parentPeerGroupID, String clearTextID, String function) {
        byte[] digest = generateHash(clearTextID, function);
        PeerGroupBinaryID peerGroupID = new PeerGroupBinaryID(parentPeerGroupID, digest, false);

        return peerGroupID;
    }

    /**
     * Create a PeerID based on the BinaryID type with a digest of the clearTextID and function.
     *
     * @param peerGroupID Parent peer group ID.
     * @param clearTextID String used as the significant part of the address
     * @param function    String used to diferentiate different clearTextID addresses (can be null).
     * @return PeerBinaryID with the digest hash of the string: clearTextID+"~"+function.
     */
    public final PeerBinaryID createPeerID(net.jxta.peergroup.PeerGroupID peerGroupID, String clearTextID, String function) {
        byte[] digest = generateHash(clearTextID, function);
        PeerBinaryID peerID = new PeerBinaryID(peerGroupID, digest, false);

        return peerID;
    }

    /**
     * Creates a new instance of DigestPipe. Because this is a  utility,
     * this is private to prevent construction.
     */
    
    /**
     * Generates a Base64 encoded string of an SHA-1 digest hash of the string: clearTextID.<p>
     *
     * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
     * @return Base64 encoded string containing the hash of the string: clearTextID.
     */
    public final String generateHashString(String clearTextID) {
        try {
            java.io.StringWriter base64 = new java.io.StringWriter();
            net.jxta.impl.util.BASE64OutputStream encode = new net.jxta.impl.util.BASE64OutputStream(base64);

            encode.write(generateHash(clearTextID));
            encode.close();

            return base64.toString();
        } catch (Exception failed) {
            LOG.log(Level.SEVERE, "Unable to encode hash value.", failed);
            throw new RuntimeException("Unable to encode hash value.");
        }
    }

    /**
     * Generates a Base64 encoded string of an SHA-1 digest hash of the string: clearTextID+"-"+function or: clearTextID if function was blank.<p>
     *
     * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
     * @param function    A function related to the clearTextID string. This is used to create a hash associated with clearTextID so that it is a uique code.
     * @return Base64 encoded string containing the hash of the string: clearTextID+"-"+function or clearTextID if function was blank.
     */
    public final String generateHashString(String clearTextID, String function) {
        try {
            java.io.StringWriter base64 = new java.io.StringWriter();
            net.jxta.impl.util.BASE64OutputStream encode = new net.jxta.impl.util.BASE64OutputStream(base64);

            encode.write(generateHash(clearTextID, function));
            encode.close();

            return base64.toString();
        } catch (Exception failed) {
            LOG.log(Level.SEVERE, "Unable to encode hash value.", failed);
            throw new RuntimeException("Unable to encode hash value.");
        }
    }

    /**
     * Generates a SHA-1 digest hash of the string: clearTextID.<p>
     *
     * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
     * @return String containing the hash of the string: clearTextID.
     */
    public final byte[] generateHash(String clearTextID) {
        return generateHash(clearTextID, null);
    }

    /**
     * Generates an SHA-1 digest hash of the string: clearTextID+"-"+function or: clearTextID if function was blank.<p>
     * <p/>
     * Note that the SHA-1 used only creates a 20 byte hash.<p>
     *
     * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
     * @param function    A function related to the clearTextID string. This is used to create a hash associated with clearTextID so that it is a uique code.
     * @return array of bytes containing the hash of the string: clearTextID+"-"+function or clearTextID if function was blank. Can return null if SHA-1 does not exist on platform.
     */
    public final byte[] generateHash(String clearTextID, String function) {
        String id;

        if (function == null) {
            id = clearTextID;
        } else {
            id = clearTextID + functionSeperator + function;
        }
        byte[] buffer = id.getBytes();

        MessageDigest algorithm;

        try {
            algorithm = MessageDigest.getInstance(algorithmType);
        } catch (Exception e) {
            LOG.log(Level.SEVERE, "Cannot load selected Digest Hash implementation", e);
            return null;
        }

        // Generate the digest.
        algorithm.reset();
        algorithm.update(buffer);

        try {
            byte[] digest1 = algorithm.digest();

            return digest1;
        } catch (Exception de) {
            LOG.log(Level.SEVERE, "Failed to creat a digest.", de);
            return null;
        }
    }

    /**
     * Generates an SHA-1 digest hash of the string: clearTextID.<p>
     *
     * @param function    the function
     * @param testHash    test hash
     * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
     * @return String containing the hash of the string: clearTextID.
     */
    public final boolean test(String clearTextID, String function, String testHash) {
        String id = clearTextID + functionSeperator + function;

        return test(id, testHash);

    }

    /**
     * Compares a clear text code or ID with a candidate hash code.
     * This is used to confirm that the clearTextID can be successfully converted to the hash.
     *
     * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
     * @param testHash    A string of hashed string.
     * @return true if the hash created from clearTextID is equal to the testHash string.Can return false if SHA-1 does not exist on platform.
     */
    public final boolean test(String clearTextID, String testHash) {

        byte[] digest1 = generateHash(clearTextID);
        byte[] digest2;

        try {
            java.io.ByteArrayOutputStream bos = new java.io.ByteArrayOutputStream();
            net.jxta.impl.util.BASE64InputStream decoder = new net.jxta.impl.util.BASE64InputStream(
                    new java.io.StringReader(testHash));

            while (true) {
                int c = decoder.read();

                if (-1 == c) {
                    break;
                }

                bos.write(c);
            }

            digest2 = bos.toByteArray();
        } catch (Exception e) {
            LOG.log(Level.SEVERE, "Failed to create a digest.", e);
            return false;
        }

        if (digest1.length != digest2.length) {
            // Not a match! because of length.
            return false;
        }

        for (int i = 0; i < digest1.length; i++) {
            if (digest1[i] != digest2[i]) {
                // Not a match because of byte:"+i+" did not match
                return false;
            }
        }

        // Match was ok
        return true;
    }

    /**
     * Compares a clear text code or ID with a candidate hash code.
     * This is used to confirm that the clearTextID can be successfully converted to the hash.
     *
     * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
     * @param testHash    A string of hashed string.
     * @return true if the hash created from clearTextID is equal to the testHash string.Can return false if SHA-1 does not exist on platform.
     */
    public final boolean test(String clearTextID, byte[] testHash) {

        byte[] digest1 = generateHash(clearTextID);

        if (digest1.length != testHash.length) {
            // Not a match! because of length.
            return false;
        }

        for (int i = 0; i < testHash.length; i++) {
            if (digest1[i] != testHash[i]) {
                // Not a match because of byte:"+i+" did not match
                return false;
            }
        }

        // Match was ok
        return true;
    }
}
