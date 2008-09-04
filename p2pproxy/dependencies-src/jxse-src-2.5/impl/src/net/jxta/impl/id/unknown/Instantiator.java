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

package net.jxta.impl.id.unknown;


import java.io.InputStream;
import java.net.URI;
import java.net.URL;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.UnknownServiceException;
import java.net.URISyntaxException;
import java.security.ProviderException;

import net.jxta.id.IDFactory;


final class Instantiator implements IDFactory.URIInstantiator {
    
    /**
     *  Our ID Format
     **/
    final static String unknownFormat = "unknown";
    
    /**
     * {@inheritDoc}
     **/
    public String getSupportedIDFormat() {
        return unknownFormat;
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.id.ID fromURL(URL source) throws MalformedURLException, UnknownServiceException {
        
        net.jxta.id.ID result = null;
        
        // check the protocol
        if (!net.jxta.id.ID.URIEncodingName.equalsIgnoreCase(source.getProtocol())) {
            throw new UnknownServiceException("URI protocol type was not as expected.");
        }
        
        String  encoded = source.getFile();
        
        // Decode the URN to convert any % encodings and convert it from UTF8.
        String decoded = sun.net.www.protocol.urn.Handler.decodeURN(encoded);
        
        int colonAt = decoded.indexOf(':');
        
        // There's a colon right?
        if (-1 == colonAt) {
            throw new UnknownServiceException("URN namespace was missing.");
        }
        
        // check the namespace
        if (!net.jxta.id.ID.URNNamespace.equalsIgnoreCase(decoded.substring(0, colonAt))) {
            throw new UnknownServiceException("URN namespace was not as expected.");
        }
        
        // skip the namespace portion and the colon
        decoded = decoded.substring(colonAt + 1);
        
        result = new ID(decoded);
        
        return result;
    }
    ;
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.codat.CodatID newCodatID(net.jxta.peergroup.PeerGroupID groupID) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.codat.CodatID newCodatID(net.jxta.peergroup.PeerGroupID groupID, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.codat.CodatID newCodatID(net.jxta.peergroup.PeerGroupID groupID, InputStream in) throws IOException {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.codat.CodatID newCodatID(net.jxta.peergroup.PeerGroupID groupID, byte[] seed, InputStream in) throws IOException {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.peer.PeerID newPeerID(net.jxta.peergroup.PeerGroupID groupID) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.peer.PeerID newPeerID(net.jxta.peergroup.PeerGroupID groupID, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.peergroup.PeerGroupID newPeerGroupID() {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(byte[] seed) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(net.jxta.peergroup.PeerGroupID parent) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.peergroup.PeerGroupID newPeerGroupID(net.jxta.peergroup.PeerGroupID parent, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.pipe.PipeID newPipeID(net.jxta.peergroup.PeerGroupID groupID) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.pipe.PipeID newPipeID(net.jxta.peergroup.PeerGroupID groupID, byte[] seed) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.platform.ModuleClassID newModuleClassID() {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.platform.ModuleClassID newModuleClassID(final net.jxta.platform.ModuleClassID classID) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
    public net.jxta.platform.ModuleSpecID newModuleSpecID(final net.jxta.platform.ModuleClassID classID) {
        throw new ProviderException("unsupported id type");
    }
    
    /**
     * {@inheritDoc}
     **/
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
     **/
    public net.jxta.id.ID fromURNNamespaceSpecificPart(String source) throws URISyntaxException {
        ID result = new ID(source);
        
        return result;
        
    }
}
