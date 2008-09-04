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


import java.net.URI;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.security.cert.X509Certificate;
import javax.crypto.EncryptedPrivateKeyInfo;

import java.io.IOException;
import java.net.URISyntaxException;
import java.security.KeyStoreException;

import net.jxta.credential.AuthenticationCredential;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peer.PeerID;
import net.jxta.membership.Authenticator;
import net.jxta.membership.MembershipService;


/**
 * An authenticator associated with the PSE membership service.
 *
 *@see net.jxta.membership.Authenticator
 *@see net.jxta.membership.MembershipService
 **/
public class StringAuthenticator implements Authenticator {
    
    /**
     * The Membership Service which generated this authenticator.
     **/
    transient PSEMembershipService source;
    
    /**
     * The Authentication which was provided to the Apply operation of the
     * membership service.
     **/
    transient AuthenticationCredential application;
    
    /**
     *  The certficate which we are authenticating against
     **/
    transient X509Certificate seedCert;
    
    /**
     *  The encrypted private key which we must unlock.
     **/
    transient EncryptedPrivateKeyInfo seedKey;
    
    /**
     * the password for that identity.
     **/
    transient char[] store_password = null;
    
    /**
     * the identity which is being claimed
     **/
    transient ID identity = null;
    
    /**
     * the password for that identity.
     **/
    transient char[] key_password = null;
    
    /**
     * Creates an authenticator for the PSE membership service. Anything entered
     * into the identity info section of the Authentication credential is
     * ignored.
     *
     *  @param source The instance of the PSE membership service which
     *  created this authenticator.
     *  @param application Anything entered into the identity info section of
     *  the Authentication credential is ignored.
     **/
    StringAuthenticator(PSEMembershipService source, AuthenticationCredential application, X509Certificate seedCert, EncryptedPrivateKeyInfo seedKey) {
        this(source, application);
        
        this.seedCert = seedCert;
        this.seedKey = seedKey;
    }
    
    /**
     * Creates an authenticator for the PSE membership service. Anything entered
     * into the identity info section of the Authentication credential is
     * ignored.
     *
     *  @param source The instance of the PSE membership service which created
     *  this authenticator.
     *  @param application Anything entered into the identity info section of
     *  the Authentication credential is ignored.
     **/
    StringAuthenticator(PSEMembershipService source, AuthenticationCredential application) {
        this.source = source;
        this.application = application;
        
        // XXX 20010328 bondolo@jxta.org Could do something with the authentication credential here.
    }
    
    /**
     * {@inheritDoc}
     **/
    @Override
    protected void finalize() throws Throwable {
        if (null != store_password) {
            Arrays.fill(store_password, '\0');
        }
        
        if (null != key_password) {
            Arrays.fill(key_password, '\0');
        }
        
        super.finalize();
    }
    
    /**
     * {@inheritDoc}
     **/
    public MembershipService getSourceService() {
        return (MembershipService) source.getInterface();
    }
    
    /**
     * {@inheritDoc}
     **/
    public AuthenticationCredential getAuthenticationCredential() {
        return application;
    }
    
    /**
     * {@inheritDoc}
     **/
    public String getMethodName() {
        return "StringAuthentication";
    }
    
    /**
     * {@inheritDoc}
     **/
    synchronized public boolean isReadyForJoin() {
        if (null != seedCert) {
            return null != PSEUtils.pkcs5_Decrypt_pbePrivateKey(key_password, seedCert.getPublicKey().getAlgorithm(), seedKey);
        } else {
            return source.pseStore.validPasswd(identity, store_password, key_password);
        }
    }
    
    /**
     *  Get KeyStore password
     **/
    public char[] getAuth1_KeyStorePassword() {
        return store_password;
    }
    
    /**
     *  Set KeyStore password
     **/
    public void setAuth1_KeyStorePassword(String store_password) {
        if (null == store_password) {
            setAuth1_KeyStorePassword((char[]) null);
        } else {
            setAuth1_KeyStorePassword(store_password.toCharArray());
        }
    }
    
    /**
     *  Set KeyStore password
     **/
    public void setAuth1_KeyStorePassword(char[] store_password) {
        if (null != this.store_password) {
            Arrays.fill(this.store_password, '\0');
        }
        
        if (null == store_password) {
            this.store_password = null;
        } else {
            this.store_password = store_password.clone();
        }
    }
    
    /**
     *  Return the available identities.
     **/
    public PeerID[] getIdentities(char[] store_password) {
        
        if (seedCert != null) {
            PeerID[] seed = { source.group.getPeerID() };

            return seed;
        } else {
            try {
                ID[] allkeys = source.pseStore.getKeysList(store_password);
                
                // XXX bondolo 20040329 it may be appropriate to login
                // something other than a peer id.
                List peersOnly = new ArrayList();
                
                Iterator eachKey = Arrays.asList(allkeys).iterator();
                
                while (eachKey.hasNext()) {
                    ID aKey = (ID) eachKey.next();
                    
                    if (aKey instanceof PeerID) {
                        peersOnly.add(aKey);
                    }
                }
                
                return (PeerID[]) peersOnly.toArray(new PeerID[peersOnly.size()]);
            } catch (IOException failed) {
                return null;
            } catch (KeyStoreException failed) {
                return null;
            }
        }
    }
    
    /**
     *  Returns the X509 Certificate associated with the specified ID.
     *
     *  @param store_password   The password for the keystore.
     *  @param aPeer    The peer who's certificate is desired. For uninitialized
     *  keystores this must be the peerid of the registering peer.
     **/
    public X509Certificate getCertificate(char[] store_password, ID aPeer) {
        if (seedCert != null) {
            if (aPeer.equals(source.group.getPeerID())) {
                return seedCert;
            } else {
                return null;
            }
        } else {
            try {
                return source.pseStore.getTrustedCertificate(aPeer, store_password);
            } catch (IOException failed) {
                return null;
            } catch (KeyStoreException failed) {
                return null;
            }
        }
    }
    
    /**
     *  Get Identity
     **/
    public ID getAuth2Identity() {
        return identity;
    }
    
    /**
     *  Set Identity
     **/
    public void setAuth2Identity(String id) {
        try {
            URI idURI = new URI(id);
            ID identity = IDFactory.fromURI(idURI);

            setAuth2Identity(identity);
        } catch (URISyntaxException badID) {
            throw new IllegalArgumentException("Bad ID");
        } 
    }
    
    /**
     *  Set Identity
     **/
    public void setAuth2Identity(ID identity) {
        this.identity = identity;
    }
    
    /**
     *  Get identity password
     **/
    public char[] getAuth3_IdentityPassword() {
        return key_password;
    }
    
    /**
     *  Set identity password
     **/
    public void setAuth3_IdentityPassword(String key_password) {
        setAuth3_IdentityPassword(key_password.toCharArray());
    }
    
    /**
     *  Set identity password
     **/
    public void setAuth3_IdentityPassword(char[] key_password) {
        if (null != this.key_password) {
            Arrays.fill(this.key_password, '\0');
        }
        
        if (null == key_password) {
            this.key_password = null;
        } else {
            this.key_password = key_password.clone();
        }
    }
}
