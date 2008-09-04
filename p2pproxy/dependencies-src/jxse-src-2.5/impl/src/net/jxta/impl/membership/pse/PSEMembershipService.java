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


import java.beans.PropertyChangeListener;
import java.beans.PropertyChangeSupport;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.net.URI;
import java.security.PrivateKey;
import java.security.cert.CertPath;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;

import java.io.IOException;
import java.security.KeyStoreException;
import java.security.NoSuchProviderException;
import java.security.cert.CertificateException;
import java.security.InvalidKeyException;
import java.security.SignatureException;
import java.security.spec.InvalidKeySpecException;
import java.util.NoSuchElementException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.credential.AuthenticationCredential;
import net.jxta.credential.Credential;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.membership.Authenticator;
import net.jxta.membership.MembershipService;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.service.Service;

import net.jxta.id.ID;

import net.jxta.exception.PeerGroupException;
import net.jxta.exception.ProtocolNotSupportedException;

import net.jxta.impl.protocol.Certificate;
import net.jxta.impl.protocol.PSEConfigAdv;
import net.jxta.impl.membership.pse.PSEUtils.IssuerInfo;


/**
 *  A JXTA Membership Service utilizing PKI to provide secure identities.
 *
 *  @see net.jxta.membership.MembershipService
 **/
public final class PSEMembershipService implements MembershipService {
    
    /**
     *  Log4J Logger
     **/
    private final static transient Logger LOG = Logger.getLogger(PSEMembershipService.class.getName());
    
    /**
     * Well known service specification identifier: pse membership
     */
    public final static ModuleSpecID pseMembershipSpecID = (ModuleSpecID) ID.create(
            URI.create(ID.URIEncodingName + ":" + ID.URNNamespace + ":uuid-DeadBeefDeafBabaFeedBabe000000050306"));
    
    /**
     * the peergroup to which this service is associated.
     **/
    PeerGroup group = null;
    
    /**
     *  The ID assigned to this instance.
     **/
    private ID assignedID = null;
    
    /**
     * The ModuleImplAdvertisement which was used to instantiate this service.
     **/
    private ModuleImplAdvertisement implAdvertisement = null;
    
    /**
     * The current set of principals associated with this peer within this peergroup.
     **/
    private final List<PSECredential> principals = new ArrayList<PSECredential>();
    
    /**
     * The set of AuthenticationCredentials which were used to establish the principals.
     **/
    private final List<AuthenticationCredential> authCredentials = new ArrayList<AuthenticationCredential>();
    
    /**
     *  property change support
     **/
    private final PropertyChangeSupport support;
    
    /**
     *  the keystore we are working with.
     **/
    PSEConfig pseStore = null;
    
    /**
     *  the default credential
     **/
    private PSECredential defaultCredential = null;
    
    /**
     *  The configuration we are using.
     **/
    private PSEConfigAdv config;
    
    /**
     * PSEPeerSecurityEngine ( and PSEAuthenticatorEngine ) loader
     */
    
    PSEPeerSecurityEngine peerSecurityEngine = null;
    
    private PSEAuthenticatorEngine authenticatorEngine = null;
    
    /**
     *  Default constructor. Normally only called by the peer group.
     **/
    public PSEMembershipService() throws PeerGroupException {
        support = new PropertyChangeSupport(getInterface());
    }
    
    /**
     *  @inheritDoc
     **/
    public void addPropertyChangeListener(PropertyChangeListener listener) {
        support.addPropertyChangeListener(listener);
    }
    
    /**
     *  @inheritDoc
     **/
    public void addPropertyChangeListener(String propertyName, PropertyChangeListener listener) {
        support.addPropertyChangeListener(propertyName, listener);
    }
    
    /**
     *  @inheritDoc
     **/
    public void removePropertyChangeListener(PropertyChangeListener listener) {
        support.removePropertyChangeListener(listener);
    }
    
    /**
     *  @inheritDoc
     **/
    public void removePropertyChangeListener(String propertyName, PropertyChangeListener listener) {
        support.removePropertyChangeListener(propertyName, listener);
    }
    
    /**
     * {@inheritDoc}
     **/
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {
        this.group = group;
        this.assignedID = assignedID;
        this.implAdvertisement = (ModuleImplAdvertisement) impl;
        
        ConfigParams configAdv = group.getConfigAdvertisement();
        
        // Get our peer-defined parameters in the configAdv
        Element param = configAdv.getServiceParam(assignedID);
        
        Advertisement paramsAdv = null;
        
        if (null != param) {
            try {
                paramsAdv = AdvertisementFactory.newAdvertisement((XMLElement) param);
            } catch (NoSuchElementException ignored) {
                ;
            }
            
            if (!(paramsAdv instanceof PSEConfigAdv)) {
                throw new PeerGroupException("Provided Advertisement was not a " + PSEConfigAdv.getAdvertisementType());
            }
            
            config = (PSEConfigAdv) paramsAdv;
        } else {
            // Create the default advertisement.
            config = (PSEConfigAdv) AdvertisementFactory.newAdvertisement(PSEConfigAdv.getAdvertisementType());
        }
        
        peerSecurityEngine = PSESecurityEngineFactory.getDefault().getInstance(this, config);
        
        authenticatorEngine = PSEAuthenticatorEngineFactory.getDefault().getInstance(this, config);
        
        KeyStoreManager storeManager = PSEKeyStoreManagerFactory.getDefault().getInstance(this, config);
        
        pseStore = new PSEConfig(storeManager, null);
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring PSE Membership Service : " + assignedID);

            configInfo.append("\n\tImplementation :");
            configInfo.append("\n\t\tModule Spec ID: " + implAdvertisement.getModuleSpecID());
            configInfo.append("\n\t\tImpl Description : " + implAdvertisement.getDescription());
            configInfo.append("\n\t\tImpl URI : " + implAdvertisement.getUri());
            configInfo.append("\n\t\tImpl Code : " + implAdvertisement.getCode());
            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tGroup : " + group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID : " + group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID : " + group.getPeerID());
            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tPSE state : " + (pseStore.isInitialized() ? "inited" : "new"));
            configInfo.append(
                    "\n\t\tPSE KeyStore location : "
                            + ((null != config.getKeyStoreLocation())
                                    ? config.getKeyStoreLocation().toString()
                                    : assignedID.toString()));
            configInfo.append(
                    "\n\t\tPSE KeyStore type : " + ((null != config.getKeyStoreType()) ? config.getKeyStoreType() : "<default>"));
            configInfo.append(
                    "\n\t\tPSE KeyStore provider : "
                            + ((null != config.getKeyStoreProvider()) ? config.getKeyStoreProvider() : "<default>"));
            LOG.config(configInfo.toString());
        }
        
        resign();
    }
    
    /**
     * {@inheritDoc}
     **/
    public Service getInterface() {
        return this;
    }
    
    /**
     * {@inheritDoc}
     **/
    public Advertisement getImplAdvertisement() {
        return implAdvertisement;
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>Currently this service starts by itself and does not expect
     * arguments.
     */
    public int startApp(String[] arg) {
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("PSE Membmership Service started.");
        }
        
        return 0;
    }
    
    /**
     * {@inheritDoc}
     **/
    public void stopApp() {
        resign();
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("PSE Membmership Service stopped.");
        }
    }
    
    public PeerGroup getGroup() {
        return group;
    }
    
    public ID getAssignedID() {
        return assignedID;
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>Supports methods <code>"StringAuthentication"</code>,
     * <code>"DialogAuthentication"</code>,
     * <code>"EngineAuthentication"</code> and
     * <code>"InteractiveAuthentication"</code> (an alias for
     * <code>"DialogAuthentication"</code>)
     **/
    public Authenticator apply(AuthenticationCredential application) throws ProtocolNotSupportedException {
        
        String method = application.getMethod();
        
        boolean newKey;

        if (!pseStore.isInitialized()) {
            // It is not inited, it's new.
            newKey = true;
        } else {
            X509Certificate configCert = config.getCertificate();
            
            if (null != configCert) {
                try {
                    ID allTrustedCerts[] = pseStore.getTrustedCertsList();
                    
                    Iterator eachTrustedCert = Arrays.asList(allTrustedCerts).iterator();
                    
                    newKey = true;
                    
                    // See if the config cert is already in the keystore.
                    while (eachTrustedCert.hasNext()) {
                        ID aTrustedCertID = (ID) eachTrustedCert.next();
                        
                        if (pseStore.isKey(aTrustedCertID)) {
                            X509Certificate aTrustedCert = pseStore.getTrustedCertificate(aTrustedCertID);
                            
                            if (aTrustedCert.equals(configCert)) {
                                newKey = false;
                                break;
                            }
                        }
                    }
                } catch (KeyStoreException bad) {
                    // The keystore is probably initialized but locked. Nothing else we can do.
                    newKey = false;
                } catch (IOException bad) {
                    // Could not read the keystore. I'm not sure it wouldn't be better to just fail.
                    newKey = false;
                }
            } else {
                // don't have anything to validate against.
                newKey = false;
            }
        }
        
        if ("StringAuthentication".equals(method)) {
            if (newKey) {
                return new StringAuthenticator(this, application, config.getCertificate(), config.getEncryptedPrivateKey());
            } else {
                return new StringAuthenticator(this, application);
            }
        } else if ("EngineAuthentication".equals(method)) {
            if (pseStore.isInitialized()) {
                return new EngineAuthenticator(this, application, authenticatorEngine);
            } else {
                return new EngineAuthenticator(this, application, authenticatorEngine);
            }
        } else if ("DialogAuthentication".equals(method) || "InteractiveAuthentication".equals(method) || (null == method)) {
            if (newKey) {
                return new DialogAuthenticator(this, application, config.getCertificate(), config.getEncryptedPrivateKey());
            } else {
                return new DialogAuthenticator(this, application);
            }
        } else {
            throw new ProtocolNotSupportedException("Authentication method not recognized");
        }
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential getDefaultCredential() {
        return defaultCredential;
    }
    
    /**
     * Sets the default credential. Also updates the peer advertisement with
     * the certificate of the default credential.
     *
     *  @param newDefault the new default credential. May also be
     *  <code>null</code> if no default is desired.
     **/
    private void setDefaultCredential(PSECredential newDefault) {
        
        Credential oldDefault = defaultCredential;
        
        synchronized (this) {
            defaultCredential = newDefault;
        }
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            LOG.config("New Default credential : " + newDefault);
        }
        
        try {
            // include the root cert in the peer advertisement
            PeerAdvertisement peeradv = group.getPeerAdvertisement();
            
            if (null != newDefault) {
                // include the root cert in the peer advertisement
                XMLDocument paramDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");
                
                Certificate peerCerts = new Certificate();
                
                peerCerts.setCertificates(newDefault.getCertificateChain());
                
                XMLDocument peerCertsAsDoc = (XMLDocument) peerCerts.getDocument(MimeMediaType.XMLUTF8);
                
                StructuredDocumentUtils.copyElements(paramDoc, paramDoc, peerCertsAsDoc, "RootCert");
                
                peeradv.putServiceParam(PeerGroup.peerGroupClassID, paramDoc);
            } else {
                peeradv.removeServiceParam(PeerGroup.peerGroupClassID);
            }
        } catch (Exception ignored) {
            ;
        }
        
        support.firePropertyChange("defaultCredential", oldDefault, newDefault);
    }
    
    /**
     * {@inheritDoc}
     **/
    public Enumeration<Credential> getCurrentCredentials() {
        List<Credential> credList = new ArrayList<Credential>(principals);
        
        return Collections.enumeration(credList);
    }
    
    /**
     * {@inheritDoc}
     **/
    public Enumeration<AuthenticationCredential> getAuthCredentials() {
        List<AuthenticationCredential> credList = new ArrayList<AuthenticationCredential>(authCredentials);
        
        return Collections.enumeration(credList);
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential join(Authenticator authenticated) throws PeerGroupException {
        
        if (this != authenticated.getSourceService()) {
            throw new ClassCastException("This is not my authenticator!");
        }
        
        if (!authenticated.isReadyForJoin()) {
            throw new PeerGroupException("Authenticator not ready to join!");
        }
        
        PSECredential newCred;
        
        char[] store_password = null;
        ID identity;
        char[] key_password = null;
        
        try {
            if (authenticated instanceof StringAuthenticator) {
                StringAuthenticator auth = (StringAuthenticator) authenticated;
                
                store_password = auth.getAuth1_KeyStorePassword();
                identity = auth.getAuth2Identity();
                key_password = auth.getAuth3_IdentityPassword();
            } else  if (authenticated instanceof EngineAuthenticator) {
                EngineAuthenticator auth = (EngineAuthenticator) authenticated;
                
                store_password = auth.getAuth1_KeyStorePassword();
                identity = auth.getAuth2Identity();
                key_password = auth.getAuth3_IdentityPassword();
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("I dont know how to deal with this authenticator " + authenticated);
                }
                
                throw new PeerGroupException("I dont know how to deal with this authenticator");
            }
            
            if (null != store_password) {
                pseStore.setKeyStorePassword(store_password);
            }
            
            if (!pseStore.isInitialized()) {
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Initializing the PSE key store.");
                }
                
                try {
                    pseStore.initialize();
                } catch (KeyStoreException bad) {
                    throw new PeerGroupException("Could not initialize new PSE keystore.", bad);
                } catch (IOException bad) {
                    throw new PeerGroupException("Could not initialize new PSE keystore.", bad);
                }
            }
            
            try {
                ID[] allkeys = pseStore.getKeysList();
                
                if (!Arrays.asList(allkeys).contains(identity)) {
                    // Add this key to the keystore.
                    X509Certificate[] seed_cert = config.getCertificateChain();
                    
                    if (null == seed_cert) {
                        throw new IOException("Could not read root certificate chain");
                    }
                    
                    PrivateKey seedPrivKey = config.getPrivateKey(key_password);
                    
                    if (null == seedPrivKey) {
                        throw new IOException("Could not read private key");
                    }
                    
                    pseStore.setKey(identity, seed_cert, seedPrivKey, key_password);
                }
            } catch (IOException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not save new key pair.", failed);
                }
                
                throw new PeerGroupException("Could not save new key pair.", failed);
            } catch (KeyStoreException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not save new key pair.", failed);
                }
                
                throw new PeerGroupException("Could not save new key pair.", failed);
            }
            
            try {
                X509Certificate certList[] = pseStore.getTrustedCertificateChain(identity);
                
                if (null == certList) {
                    certList = new X509Certificate[1];
                    
                    certList[0] = pseStore.getTrustedCertificate(identity);
                    
                    if (certList[0] == null && authenticatorEngine != null) {
                        certList[0] = authenticatorEngine.getX509Certificate();
                    }
                }
                
                CertificateFactory cf = CertificateFactory.getInstance("X.509");
                
                CertPath certs = cf.generateCertPath(Arrays.asList(certList));
                
                PrivateKey privateKey = pseStore.getKey(identity, key_password);
                
                newCred = new PSECredential(this, identity, certs, privateKey);
                
                synchronized (this) {
                    principals.add(newCred);
                    
                    authCredentials.add(authenticated.getAuthenticationCredential());
                }
            } catch (IOException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not create credential.", failed);
                }
                
                throw new PeerGroupException("Could not create credential.", failed);
            } catch (KeyStoreException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not create credential.", failed);
                }
                
                throw new PeerGroupException("Could not create credential.", failed);
            } catch (CertificateException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not create credential.", failed);
                }
                
                throw new PeerGroupException("Could not create credential.", failed);
            }
        } finally {
            if (null != store_password) {
                Arrays.fill(store_password, '\0');
            }
            
            if (null != key_password) {
                Arrays.fill(key_password, '\0');
            }
        }
        
        // XXX bondolo potential but unlikely race condition here.
        if (null == getDefaultCredential()) {
            setDefaultCredential(newCred);
        }
        
        support.firePropertyChange("addCredential", null, newCred);
        
        return newCred;
    }
    
    /**
     * {@inheritDoc}
     **/
    public void resign() {
        Iterator eachCred = Arrays.asList(principals.toArray()).iterator();
        
        synchronized (this) {
            principals.clear();
            authCredentials.clear();
        }
        
        setDefaultCredential(null);
        
        // clear the keystore password.
        pseStore.setKeyStorePassword(null);
        
        while (eachCred.hasNext()) {
            PSECredential aCred = (PSECredential) eachCred.next();
            
            aCred.setValid(false);
        }
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential makeCredential(Element element) {
        
        return new PSECredential(this, element);
    }
    
    /**
     *  Returns the key store object associated with this PSE Membership Service.
     **/
    public PSEConfig getPSEConfig() {
        return pseStore;
    }
    
    /**
     * Service Certificates Support
     */
    
    /**
     *  Generate a new service certificate for the assigned ID given an authenticated local credential.
     *
     *  @param assignedID   The assigned ID of the service credential.
     *  @param credential   The issuer credential for the service credential.
     **/
    X509Certificate[] generateServiceCertificate(ID assignedID, PSECredential credential) throws  IOException, KeyStoreException, InvalidKeyException, SignatureException {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Generating new service cert for " + assignedID);
        }
        
        IssuerInfo serviceinfo = peerSecurityEngine.generateCertificate(credential);
        
        // write the client root cert and private key
        X509Certificate[] serviceChain = { serviceinfo.cert, serviceinfo.issuer };
        
        char keyPass[];
        
        if (null != serviceinfo.issuerPkey) {
            ByteArrayInputStream bis = new ByteArrayInputStream(serviceinfo.issuerPkey.getEncoded());
            byte privateKeySignature[] = peerSecurityEngine.sign(null, credential, bis);

            keyPass = PSEUtils.base64Encode(privateKeySignature, false).toCharArray();
        } else {
            keyPass = authenticatorEngine.getKeyPass(group);
        }
        
        getPSEConfig().setKey(assignedID, serviceChain, serviceinfo.subjectPkey, keyPass);
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Generated new service cert");
        }
        
        return serviceChain;
    }
    
    /**
     *  Recover the service credential for the assigned ID given an authenticated local credential.
     *
     *  @param assignedID   The assigned ID of the service credential.
     *  @param credential   The issuer credential for the service credential.
     **/
    public PSECredential getServiceCredential(ID assignedID, PSECredential credential) throws IOException, PeerGroupException, InvalidKeyException, SignatureException {
        
        PSECredential pseCredential = null;
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Getting service redential for " + assignedID);
        }
        
        Authenticator authenticate = null;
        
        if (null != authenticatorEngine) {
            AuthenticationCredential authCred = new AuthenticationCredential(group, "EngineAuthentication", null);

            try {
                authenticate = apply(authCred);
            } catch (Exception failed) {
                ;
            }
            
            if (null == authenticate) {
                return null;
            }

            EngineAuthenticator auth = (EngineAuthenticator) authenticate;

            auth.setAuth1_KeyStorePassword(authenticatorEngine.getStorePass(group));
            auth.setAuth2Identity(assignedID);
            auth.setAuth3_IdentityPassword(authenticatorEngine.getKeyPass(group));
        } else {
            AuthenticationCredential authCred = new AuthenticationCredential(group, "StringAuthentication", null);
            
            try {
                authenticate = apply(authCred);
            } catch (Exception failed) {
                ;
            }
            
            if (null == authenticate) {
                return null;
            }
            
            PrivateKey privateKey = credential.getPrivateKey();
            
            // make a new service certificate
            ByteArrayInputStream bis = new ByteArrayInputStream(privateKey.getEncoded());
            byte privateKeySignature[] = peerSecurityEngine.sign(null, credential, bis);
            String passkey = PSEUtils.base64Encode(privateKeySignature, false);
            
            StringAuthenticator auth = (StringAuthenticator) authenticate;

            auth.setAuth1_KeyStorePassword((String) null);
            auth.setAuth2Identity(assignedID);
            auth.setAuth3_IdentityPassword(passkey);
        }
        
        if (authenticate.isReadyForJoin()) {
            pseCredential = (PSECredential) join(authenticate);
        } else {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Could not authenticate service credential");
            }
        }
        
        return pseCredential;
    }
}
