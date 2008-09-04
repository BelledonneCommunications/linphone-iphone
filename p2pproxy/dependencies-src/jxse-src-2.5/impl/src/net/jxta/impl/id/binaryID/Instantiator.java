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


import java.io.InputStream;
import java.net.URI;
import java.net.URL;
import java.security.SecureRandom;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.UnknownServiceException;

import net.jxta.peergroup.PeerGroupID;


/**
 * ID Factory for the binary ID type. All identifiers in this type are prefixed by "binaryid".
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 */

public final class Instantiator implements net.jxta.id.IDFactory.URIInstantiator {

    /**
     * LOG object for this class.
     */
    private final static transient Logger LOG = Logger.getLogger(Instantiator.class.getName());
 
    /**
     * Our ID Format
     */
    final static String BinaryIDEncoded = "binaryid";

    /**
     * Random generator used for ID creation where a seed (idValue) is not provided.
     */
    private static final Random randNumGenerator = new SecureRandom();

    /**
     * {@inheritDoc}
     */
    public String getSupportedIDFormat() {
        return BinaryIDEncoded;
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.id.ID fromURL(URL source) throws MalformedURLException, UnknownServiceException {

        // check the protocol
        if (!net.jxta.id.ID.URIEncodingName.equalsIgnoreCase(source.getProtocol())) {
            throw new UnknownServiceException("URI protocol type was not as expected.");
        }

        String encoded = source.getFile();

        int colonAt = encoded.indexOf(':');

        // There's a colon right?
        if (-1 == colonAt) {
            throw new UnknownServiceException("URN namespace was missing.");
        }

        // check the namespace
        if (!net.jxta.id.ID.URNNamespace.equalsIgnoreCase(encoded.substring(0, colonAt))) {
            throw new UnknownServiceException("URN namespace was not as expected.");
        }

        // skip the namespace portion and the colon
        encoded = encoded.substring(colonAt + 1);

        try {
            return fromURNNamespaceSpecificPart(encoded);
        } catch (URISyntaxException failed) {
            MalformedURLException failure = new MalformedURLException("Failure parsing URL");

            failure.initCause(failed);
            
            throw failure;
        }
    }
        
    /**
     * {@inheritDoc}
     */
    public net.jxta.id.ID fromURI(URI source) throws URISyntaxException {
        
        // check the protocol
        if (!net.jxta.id.ID.URIEncodingName.equalsIgnoreCase(source.getScheme())) {
            throw new URISyntaxException(source.toString(), "URI scheme was not as expected.");
        }
        
        String decoded = source.getSchemeSpecificPart();
        
        int colonAt = decoded.indexOf(':');
        
        // There's a colon right?
        if (-1 == colonAt) {
            throw new URISyntaxException(source.toString(), "URN namespace was missing.");
        }
        
        // check the namespace
        if (!net.jxta.id.ID.URNNamespace.equalsIgnoreCase(decoded.substring(0, colonAt))) {
            throw new URISyntaxException(source.toString()
                    ,
                    "URN namespace was not as expected. (" + net.jxta.id.ID.URNNamespace + "!=" + decoded.substring(0, colonAt)
                    + ")");
        }
        
        // skip the namespace portion and the colon
        decoded = decoded.substring(colonAt + 1);
        
        return fromURNNamespaceSpecificPart(decoded);
    }
    
    /**
     * {@inheritDoc}
     */
    public net.jxta.id.ID fromURNNamespaceSpecificPart(String encoded) throws URISyntaxException {
        int dashAt = encoded.indexOf('-');

        // there's a dash, right?
        if (-1 == dashAt) {
            throw new URISyntaxException(encoded, "URN Encodingtype was missing.");
        }

        if (!encoded.substring(0, dashAt).equals(BinaryIDEncoded)) {
            throw new URISyntaxException(encoded
                    ,
                    "JXTA id format was not as expected. Should have been BinaryIDEncoded found:" + encoded.substring(0, dashAt));
        }

        // skip the dash
        encoded = encoded.substring(dashAt + 1);
        // check that the length is long enough
        if (encoded.length() < 1) {
            throw new URISyntaxException(encoded, "URN does not contain enough chars. Must have at least one byte");
        }
        BinaryID id = new BinaryID(encoded);
        net.jxta.id.ID result = null;

        switch (id.type()) {

        case BinaryID.flagCodatID:
            result = new CodatBinaryID(encoded);
            break;

        case BinaryID.flagPeerGroupID:
            result = new PeerGroupBinaryID(encoded);
            if (PeerGroupID.worldPeerGroupID.equals(result)) {
                result = net.jxta.peergroup.PeerGroupID.worldPeerGroupID;
            }
            break;

        case BinaryID.flagPeerID:
            result = new PeerBinaryID(encoded);
            break;

        case BinaryID.flagPipeID:
            result = new PipeBinaryID(encoded);
            break;

        case BinaryID.flagModuleClassID:
            result = new ModuleClassBinaryID(encoded);
            break;

        case BinaryID.flagModuleSpecID:
            result = new ModuleSpecBinaryID(encoded);
            break;

        default:
            throw new URISyntaxException(encoded, "jxta ID type not recognized");
        }

        return result;
    }

    /**
     * Utility to create a random array of bits to be used when a random value is required.
     */
    private byte[] randomID() {
        byte[] randBuf16 = new byte[16];

        randNumGenerator.nextBytes(randBuf16);

        return randBuf16;
    }

    /**
     * {@inheritDoc}
     *
     * @throws UnsupportedOperationException This form is not supported. Use CODAT from UUID package instead.
     */
    public net.jxta.codat.CodatID newCodatID(final net.jxta.peergroup.PeerGroupID groupID) {
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return new net.jxta.impl.id.binaryID.CodatBinaryID(parentGroupID, randomID(), false);
        // throw new UnsupportedOperationException("This form is not supported. Use CODAT from UUID package instead.");
    }

    /**
     * {@inheritDoc}
     *
     * @throws UnsupportedOperationException This form is not supported. Use CODAT from UUID package instead.
     */
    public net.jxta.codat.CodatID newCodatID(final net.jxta.peergroup.PeerGroupID groupID, byte[] seed) {
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return new net.jxta.impl.id.binaryID.CodatBinaryID(parentGroupID, seed, false);
    }

    /**
     * {@inheritDoc}
     *
     * @throws UnsupportedOperationException This form is not supported. Use CODAT from UUID package instead.
     */
    public net.jxta.codat.CodatID newCodatID(final net.jxta.peergroup.PeerGroupID groupID, InputStream in) throws IOException {
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return new net.jxta.impl.id.binaryID.CodatBinaryID(parentGroupID, randomID(), false);
    }

    /**
     * {@inheritDoc}
     *
     * @throws UnsupportedOperationException This form is not supported. Use CODAT from UUID package instead.
     */
    public net.jxta.codat.CodatID newCodatID(final net.jxta.peergroup.PeerGroupID groupID, byte[] idValue, InputStream in) throws IOException {
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return new net.jxta.impl.id.binaryID.CodatBinaryID(parentGroupID, idValue, false);
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peer.PeerID newPeerID(final net.jxta.peergroup.PeerGroupID groupID) {
        LOG.log(Level.SEVERE, "random peer created", new RuntimeException());
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return new net.jxta.impl.id.binaryID.PeerBinaryID(parentGroupID, randomID(), false);
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peer.PeerID newPeerID(final net.jxta.peergroup.PeerGroupID groupID, byte[] idValue) {
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return new net.jxta.impl.id.binaryID.PeerBinaryID(parentGroupID, idValue, false);
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID() {
        return net.jxta.id.IDFactory.newPeerGroupID(randomID());
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(byte[] idValue) {
        return new PeerGroupBinaryID(idValue, false);
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(net.jxta.peergroup.PeerGroupID parent) {
        LOG.log(Level.SEVERE, "random peergroup created", new RuntimeException());
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(parent);

        return net.jxta.id.IDFactory.newPeerGroupID(parentGroupID, randomID());
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(net.jxta.peergroup.PeerGroupID parent, byte[] idValue) {
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(parent);

        return new PeerGroupBinaryID(parentGroupID, idValue, false);
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.pipe.PipeID newPipeID(final net.jxta.peergroup.PeerGroupID groupID) {
        PeerGroupID parentGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return net.jxta.id.IDFactory.newPipeID(parentGroupID, randomID());
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.pipe.PipeID newPipeID(final net.jxta.peergroup.PeerGroupID groupID, byte[] idValue) {
        PeerGroupID peerGroupID = (PeerGroupID) IDFormat.translateFromWellKnown(groupID);

        return new PipeBinaryID(peerGroupID, idValue, false);
    }

    /**
     * {@inheritDoc}
     *
     * @throws UnsupportedOperationException This form is not supported because a binary ID is meant to be created with a random ID.
     */
    public net.jxta.platform.ModuleClassID newModuleClassID() {
        throw new UnsupportedOperationException(
                "This form is not supported because a binary ID is meant to be created with a random ID. Use UUID package instead.");
    }

    /**
     * {@inheritDoc}
     *
     * @throws UnsupportedOperationException This form is not supported because a binary ID is meant to be created with a random ID.
     */
    public net.jxta.platform.ModuleClassID newModuleClassID(final net.jxta.platform.ModuleClassID classID) {
        throw new UnsupportedOperationException(
                "This form is not supported because a binary ID is meant to be created with a random ID. Use UUID package instead.");
    }

    /**
     * {@inheritDoc}
     *
     * @throws UnsupportedOperationException This form is not supported because a binary ID is meant to be created with a random ID. Use UUID instead.
     */
    public net.jxta.platform.ModuleSpecID newModuleSpecID(final net.jxta.platform.ModuleClassID classID) {
        throw new UnsupportedOperationException(
                "This form is not supported because a binary ID is meant to be created with a random ID. Use UUID package instead.");
    }

}
