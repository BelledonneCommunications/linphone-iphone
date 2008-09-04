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
package net.jxta.impl.membership.pse;


import java.io.InputStream;
import java.io.IOException;
import java.security.PublicKey;
import java.security.InvalidKeyException;
import java.security.Signature;
import java.security.SignatureException;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.KeyPair;

import javax.security.auth.x500.X500Principal;

import net.jxta.impl.membership.pse.PSEUtils.IssuerInfo;


/**
 *  Provides algorithm support for PSE.
 *
 **/
public interface PSEPeerSecurityEngine {
    
    /**
     *  Cryptographically sign an input stream using the specified credential
     *  with the specified algorithm.
     *
     *  @param algorithm The signature algorithm to use.
     *  @param credential   The credential which is signing the stream.
     *  @param bis  The input stream to be signed.
     *  @return The resulting signature.
     **/
    public byte[] sign(String algorithm, PSECredential credential, InputStream bis) throws InvalidKeyException, SignatureException, IOException;
    
    /**
     *  Cryptographically verify a signature against an input stream using the 
     * specified credential with the specified algorithm.
     *
     *  @param algorithm The signature algorithm to use.
     *  @param signature The signature to be verified.
     *  @param credential   The credential which is signing the stream.
     *  @param bis  The input stream to be signed.
     *  @return {@code true} If the signature is valid otherwise {@code false}.
     **/
    public boolean verify(String algorithm, PSECredential credential, byte[] signature, InputStream bis) throws InvalidKeyException, SignatureException, IOException;
    
    /**
     *   Generate a new service certificate.
     *
     *   @param credential The credential which will be the issuer of the service certificate.
     **/
    public IssuerInfo generateCertificate(PSECredential credential) throws SecurityException;
    
    /**
     *   Returns the default signature algorithm for this security engine.
     **/
    public String getSignatureAlgorithm();
}
