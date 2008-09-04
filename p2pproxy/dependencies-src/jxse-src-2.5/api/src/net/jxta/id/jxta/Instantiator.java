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

package net.jxta.id.jxta;


import net.jxta.codat.CodatID;
import net.jxta.id.ID;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.pipe.PipeID;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.UnknownServiceException;
import java.security.ProviderException;


/**
 * Instantiator for the 'jxta' ID Format.
 *
 * @see net.jxta.id.ID
 * @see net.jxta.id.IDFactory
 * @see net.jxta.id.jxta.IDFormat
 */
final class Instantiator implements net.jxta.id.IDFactory.URIInstantiator {

    /**
     * This table maps the textual values of the well known ids to the
     * singleton classes which match those textual names.
     */
    final static Object[][] wellKnownIDs = {
        { net.jxta.id.ID.nullID.getUniqueValue(), net.jxta.id.ID.nullID}
                ,

        { net.jxta.peergroup.PeerGroupID.worldPeerGroupID.getUniqueValue(), net.jxta.peergroup.PeerGroupID.worldPeerGroupID}
                ,

        {
            net.jxta.peergroup.PeerGroupID.defaultNetPeerGroupID.getUniqueValue()
                    ,
            net.jxta.peergroup.PeerGroupID.defaultNetPeerGroupID}
    };

    /**
     * {@inheritDoc}
     */
    public String getSupportedIDFormat() {
        return IDFormat.JXTAFormat;
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    public ID fromURL(URL source) throws MalformedURLException, UnknownServiceException {

        // check the protocol
        if (!ID.URIEncodingName.equalsIgnoreCase(source.getProtocol())) {
            throw new UnknownServiceException("URI protocol type was not as expected.");
        }

        String encoded = source.getFile();

        int colonAt = encoded.indexOf(':');

        // There's a colon right?
        if (-1 == colonAt) {
            throw new UnknownServiceException("URN namespace was missing.");
        }

        // check the namespace
        if (!ID.URNNamespace.equalsIgnoreCase(encoded.substring(0, colonAt))) {
            throw new UnknownServiceException("URN namespace was not as expected.");
        }

        // skip the namespace portion and the colon
        encoded = encoded.substring(colonAt + 1);

        int dashAt = encoded.indexOf('-');

        // there's a dash, right?
        if (-1 == dashAt) {
            throw new UnknownServiceException("JXTA ID Format was missing.");
        }

        if (!encoded.substring(0, dashAt).equals(getSupportedIDFormat())) {
            throw new UnknownServiceException("JXTA ID Format was not as expected.");
        }

        for (Object[] wellKnownID : wellKnownIDs) {
            if (encoded.equalsIgnoreCase(wellKnownID[0].toString())) {
                return (ID) wellKnownID[1];
            }
        }

        throw new MalformedURLException("unrecognized id");
    }

    /**
     * {@inheritDoc}
     */
    public CodatID newCodatID(PeerGroupID groupID) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public CodatID newCodatID(PeerGroupID groupID, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public CodatID newCodatID(PeerGroupID groupID, InputStream in) throws IOException {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public CodatID newCodatID(PeerGroupID groupID, byte[] seed, InputStream in) throws IOException {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public PeerID newPeerID(PeerGroupID groupID) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public PeerID newPeerID(PeerGroupID groupID, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID() {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(byte[] seed) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(net.jxta.peergroup.PeerGroupID parent) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(net.jxta.peergroup.PeerGroupID parent, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public PipeID newPipeID(PeerGroupID groupID) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public PipeID newPipeID(PeerGroupID groupID, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public ModuleClassID newModuleClassID() {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public ModuleClassID newModuleClassID(ModuleClassID classID) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public ModuleSpecID newModuleSpecID(ModuleClassID classID) {
        throw new ProviderException("unsupported id type");
    }

    /**
     * {@inheritDoc}
     */
    public ID fromURI(URI source) throws URISyntaxException {

        // check the protocol
        if (!ID.URIEncodingName.equalsIgnoreCase(source.getScheme())) {
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
    public ID fromURNNamespaceSpecificPart(String source) throws URISyntaxException {
        int dashAt = source.indexOf('-');

        // there's a dash, right?
        if (-1 == dashAt) {
            throw new URISyntaxException(source, "URN jxta namespace IDFormat was missing.");
        }

        if (!source.substring(0, dashAt).equals(getSupportedIDFormat())) {
            throw new URISyntaxException(source, "JXTA ID Format was not as expected.");
        }

        for (Object[] wellKnownID : wellKnownIDs) {
            if (source.equalsIgnoreCase(wellKnownID[0].toString())) {
                return (ID) wellKnownID[1];
            }
        }

        throw new URISyntaxException(source, "unrecognized id");
    }
}
