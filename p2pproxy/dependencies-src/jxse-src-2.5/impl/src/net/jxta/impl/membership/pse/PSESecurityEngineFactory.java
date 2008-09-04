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


import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;

import java.io.IOException;
import java.security.InvalidKeyException;
import java.security.SignatureException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.id.ID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.exception.PeerGroupException;
import net.jxta.impl.protocol.PSEConfigAdv;

import net.jxta.impl.membership.pse.PSEUtils.IssuerInfo;


/**
 *  A factory for PSE Security Engines.
 *
 * @see PSEPeerSecurityEngine
 */
public abstract class PSESecurityEngineFactory {
    
    private static PSESecurityEngineFactory defaultSecurityEngine = null;
    
    /**
     *  Set the default PSESecurityEngineFactoryss
     **/
    public static void setPSESecurityEngineManager(PSESecurityEngineFactory newSecurityEngine) {
        synchronized (PSESecurityEngineFactory.class) {
            defaultSecurityEngine = newSecurityEngine;
        }
    }
    
    /**
     *   Returns the default Security Engine Factory.
     *
     *   @return The current default Security Engine Factory.
     **/
    public static PSESecurityEngineFactory getDefault() {
        synchronized (PSESecurityEngineFactory.class) {
            if (defaultSecurityEngine == null) {
                defaultSecurityEngine = new PSESecurityEngineDefaultFactory();
            }
            
            return defaultSecurityEngine;
        }
    }
    
    /**
     *   Creates a new Peer Security Engine instance based upon the context and configuration.sss
     *
     *   @param service  The service that this keystore manager will be working for.
     *   @param config   The configuration parameters.
     **/
    public abstract PSEPeerSecurityEngine getInstance(PSEMembershipService service, PSEConfigAdv config) throws PeerGroupException;
    
    /**
     *   Default implementation which provides the default behaviour (which is to do nothing).
     **/
    private static class PSESecurityEngineDefaultFactory extends PSESecurityEngineFactory {
        @Override
        public PSEPeerSecurityEngine getInstance(PSEMembershipService service, PSEConfigAdv config) throws PeerGroupException {
            return new PSEPeerSecurityEngineDefault();
        }
    }
    

    /**
     *   Default implementation which provides the default behaviour.
     **/
    private static class PSEPeerSecurityEngineDefault implements PSEPeerSecurityEngine {

        /**
         *  Log4J Logger
         **/
        private static final Logger LOG = Logger.getLogger(PSEPeerSecurityEngineDefault.class.getName());
        
        /**
         *   {@inheritDoc}
         **/
        public byte[] sign(String algorithm, PSECredential credential, InputStream bis)  throws InvalidKeyException, SignatureException, IOException {
            
            if (null == algorithm) {
                algorithm = getSignatureAlgorithm();
            }
            
            return PSEUtils.computeSignature(algorithm, credential.getPrivateKey(), bis);
        }
        
        /**
         *   {@inheritDoc}
         **/
        public boolean verify(String algorithm, PSECredential credential, byte[] signature, InputStream bis) throws InvalidKeyException, SignatureException, IOException {
            if (null == algorithm) {
                algorithm = getSignatureAlgorithm();
            }
            
            return PSEUtils.verifySignature(algorithm, credential.getCertificate(), signature, bis);
        }
        
        /**
         *   {@inheritDoc}
         **/
        public IssuerInfo generateCertificate(PSECredential credential) throws SecurityException {
            
            // we need a new cert.
            IssuerInfo info = new IssuerInfo();
            
            info.cert = credential.getCertificate();
            info.subjectPkey = credential.getPrivateKey();
            String cname = PSEUtils.getCertSubjectCName(info.cert);
            
            if (null != cname) {
                // remove the -CA which is common to ca root certs.
                if (cname.endsWith("-CA")) {
                    cname = cname.substring(0, cname.length() - 3);
                }
            }
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Generating new service cert for \'" + cname + "\'");
            }
            
            // generate the service cert and private key
            IssuerInfo serviceinfo = PSEUtils.genCert(cname, info);

            // IssuerInfo serviceinfo = membership.genCert( cname, info, "SHA1withRSA" );
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Generated new service cert for \'" + cname + "\'");
            }
            
            return serviceinfo;
        }
        
        /**
         *   {@inheritDoc}
         **/
        public String getSignatureAlgorithm() {
            return "SHA1withRSA";
        }
    }
}
