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


import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.security.KeyStore;

import java.io.IOException;
import java.security.KeyStoreException;
import java.security.NoSuchProviderException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.id.ID;
import net.jxta.peergroup.PeerGroup;

import net.jxta.impl.cm.Cm;
import net.jxta.impl.peergroup.StdPeerGroup;


/**
 *  Manages a Keystore located within the JXTA CM.
 **/
public class CMKeyStoreManager implements KeyStoreManager {
    
    /**
     *  Log4J Logger
     **/
    private final static transient Logger LOG = Logger.getLogger(CMKeyStoreManager.class.getName());
    
    /**
     *  Our default keystore type.
     **/
    private final static String DEFAULT_KEYSTORE_TYPE = "jks";
    
    /**
     *  The keystore type
     **/
    private final String keystore_type;
    
    /**
     *  The keystore type
     **/
    private final String keystore_provider;
    
    /**
     *  The JXTA CM where the keystore lives.
     **/
    private final Cm keystore_cm;
    
    /**
     *  The CM ID where the keystore lives.
     **/
    private final ID keystore_location;
    
    /**
     *  Default constructor.
     *
     *  @param type The keystore type to use. The current default is the "JKS"
     *  keystore which is specified via {@code null}.
     *  @param provider The JCE cryptographic provider to use for the keystore.
     *  May also be  {@code null} for the default provider.
     *  @param group The peer group which will provide the CM.
     *  @param location The ID under which the keystore will be stored in the
     *  CM.
     *  @throws NoSuchProviderException Thrown if the requested provider is not
     *  available.
     *  @throws KeyStoreException Thrown for errors getting a keystore of the
     *  requested type.
     **/
    public CMKeyStoreManager(String type, String provider, PeerGroup group, ID location) throws NoSuchProviderException, KeyStoreException {
        
        if (null == type) {
            type = DEFAULT_KEYSTORE_TYPE;
            provider = null;
        }
        
        keystore_type = type;
        
        keystore_provider = provider;
        
        keystore_cm = ((StdPeerGroup) group).getCacheManager();
        
        keystore_location = location;
        
        // check if we can get an instance.
        if (null == keystore_provider) {
            KeyStore.getInstance(keystore_type);
        } else {
            KeyStore.getInstance(keystore_type, keystore_provider);
        }
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            LOG.config("pse location = " + keystore_location + " in " + keystore_cm);
        }
    }
    
    /**
     *  {@inheritDoc}
     **/
    public boolean isInitialized() {
        return isInitialized(null);
    }
    
    /**
     *  {@inheritDoc}
     **/
    public boolean isInitialized(char[] store_password) {
        try {
            KeyStore store;

            if (null == keystore_provider) {
                store = KeyStore.getInstance(keystore_type);
            } else {
                store = KeyStore.getInstance(keystore_type, keystore_provider);
            }
            
            InputStream is = keystore_cm.getInputStream("Raw", keystore_location.toString());
            
            if (null == is) {
                return false;
            }
            
            store.load(is, store_password);
            
            return true;
        } catch (Exception failed) {
            return false;
        }
    }
    
    /**
     *  {@inheritDoc}
     **/
    public void createKeyStore(char[] store_password) throws KeyStoreException, IOException {
        try {
            KeyStore store;

            if (null == keystore_provider) {
                store = KeyStore.getInstance(keystore_type);
            } else {
                store = KeyStore.getInstance(keystore_type, keystore_provider);
            }
            
            store.load(null, store_password);
            
            saveKeyStore(store, store_password);
        } catch (NoSuchProviderException failed) {
            KeyStoreException failure = new KeyStoreException("NoSuchProviderException during keystore processing");

            failure.initCause(failed);
            throw failure;
        } catch (NoSuchAlgorithmException failed) {
            KeyStoreException failure = new KeyStoreException("NoSuchAlgorithmException during keystore processing");

            failure.initCause(failed);
            throw failure;
        } catch (CertificateException failed) {
            KeyStoreException failure = new KeyStoreException("CertificateException during keystore processing");

            failure.initCause(failed);
            throw failure;
        }
    }
    
    /**
     *  {@inheritDoc}
     **/
    public KeyStore loadKeyStore(char[] password) throws KeyStoreException, IOException {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Loading (" + keystore_type + "," + keystore_provider + ") store from " + keystore_location);
        }
        
        try {
            KeyStore store;

            if (null == keystore_provider) {
                store = KeyStore.getInstance(keystore_type);
            } else {
                store = KeyStore.getInstance(keystore_type, keystore_provider);
            }
            
            InputStream is = keystore_cm.getInputStream("Raw", keystore_location.toString());
            
            store.load(is, password);
            
            return store;
        } catch (NoSuchAlgorithmException failed) {
            KeyStoreException failure = new KeyStoreException("NoSuchAlgorithmException during keystore processing");

            failure.initCause(failed);
            throw failure;
        } catch (CertificateException failed) {
            KeyStoreException failure = new KeyStoreException("CertificateException during keystore processing");

            failure.initCause(failed);
            throw failure;
        } catch (NoSuchProviderException failed) {
            KeyStoreException failure = new KeyStoreException("NoSuchProviderException during keystore processing");

            failure.initCause(failed);
            throw failure;
        }
    }
    
    /**
     *  {@inheritDoc}
     **/
    public void saveKeyStore(KeyStore store, char[] password) throws IOException, KeyStoreException {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Writing " + store + " to " + keystore_location);
        }
        
        try {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            
            store.store(bos, password);
            bos.close();
            
            keystore_cm.save("Raw", keystore_location.toString(), bos.toByteArray(), Long.MAX_VALUE, 0);
        } catch (NoSuchAlgorithmException failed) {
            KeyStoreException failure = new KeyStoreException("NoSuchAlgorithmException during keystore processing");

            failure.initCause(failed);
            throw failure;
        } catch (CertificateException failed) {
            KeyStoreException failure = new KeyStoreException("CertificateException during keystore processing");

            failure.initCause(failed);
            throw failure;
        }
    }
    
    /**
     *  {@inheritDoc}
     **/
    public void eraseKeyStore() throws IOException {
        
        keystore_cm.remove("Raw", keystore_location.toString());
    }

    /**
     *  {@inheritDoc}
     **/
    public String toString() {
       StringBuilder sb = new StringBuilder("PSE keystore details:  \n");
       sb.append("   Class:  ").append(this.getClass().getName()).append("\n");
       sb.append("   Type:  ").append(keystore_type==null ? "<default>" : keystore_type).append("\n");
       sb.append("   Provider:  ").append(keystore_provider==null ? "<default>" : keystore_provider).append("\n");
       sb.append("   Location:  ").append(keystore_location==null ? "<default>" : keystore_location.toString()).append("\n");
       return sb.toString();
    }
}
