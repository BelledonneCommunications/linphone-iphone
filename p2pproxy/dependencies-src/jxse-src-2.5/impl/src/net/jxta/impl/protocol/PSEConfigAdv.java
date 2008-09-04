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

package net.jxta.impl.protocol;


import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.StringReader;
import java.net.URI;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;
import java.security.cert.CertificateFactory;
import java.util.Enumeration;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import javax.crypto.EncryptedPrivateKeyInfo;

import java.io.IOException;
import java.lang.reflect.UndeclaredThrowableException;
import java.net.URISyntaxException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.XMLElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.membership.pse.PSEUtils;
import net.jxta.peergroup.PeerGroupID;


/**
 *  Contains parameters for configuration of the PSE Membership Service.
 *
 *  <p/>The configuration advertisement can include an optional seed certificate
 *  chain and encrypted private key. If this seed information is present the PSE 
 *  Membership Service will require an initial authentication to unlock the 
 *  encrypted private key before creating the PSE keystore. The newly created 
 *  PSE keystore will be "seeded" with the certificate chain and the private key.
 *
 *  <p/>This mechanism allows for out-of-band distribution of JXTA identity
 *  information and avoids the need for remote authentication.
 *
 *  <p/>Note: This implementation contemplates multiple root certs in its
 *  schema, but the API has not yet been extended to include this functionality.
 */
public final class PSEConfigAdv extends ExtendableAdvertisement implements Cloneable {

    /**
     *   Log4J Logger
     */
    private final static transient Logger LOG = Logger.getLogger(PSEConfigAdv.class.getName());

    /**
     *  Our DOCTYPE
     */
    private final static String advType = "jxta:PSEConfig";

    /**
     *  Instantiator for PSEConfigAdv
     */
    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return advType;
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new PSEConfigAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            return new PSEConfigAdv(root);
        }
    }

    private final static String ROOT_CERT_TAG = "RootCert";
    private final static String CERT_TAG = "Certificate";
    private final static String ENCRYPTED_PRIVATE_KEY_TAG = "EncryptedPrivateKey";
    private final static String KEY_STORE_TYPE_ATTR = "KeyStoreType";
    private final static String KEY_STORE_PROVIDER_ATTR = "KeyStoreProvider";
    private final static String KEY_STORE_LOCATION_TAG = "KeyStoreLocation";

    private final static String[] INDEX_FIELDS = {};

    private final List<X509Certificate> certs = new ArrayList<X509Certificate>();

    private EncryptedPrivateKeyInfo encryptedPrivateKey = null;

    private String privAlgorithm = null;

    private String keyStoreType = null;

    private String keyStoreProvider = null;

    private URI keyStoreLocation = null;

    /**
     *  Returns the identifying type of this Advertisement.
     *
     *  <p/><b>Note:</b> This is a static method. It cannot be used to determine
     *  the runtime type of an advertisement. ie.
     *  </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     *
     *  <p/><b>This is wrong and does not work the way you might expect.</b>
     *  This call is not polymorphic and calls
     *  {@code Advertisement.getAdvertisementType()} no matter what the real 
     *  type of the advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return advType;
    }

    /**
     *  Use the Instantiator through the factory
     */
    private PSEConfigAdv() {}

    /**
     *  Use the Instantiator through the factory
     *
     *  @param root The XMLElement which is the root element of the PSEConfigAdv.
     */
    private PSEConfigAdv(Element root) {
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;

        String doctype = doc.getName();

        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(getAdvertisementType()) && !getAdvertisementType().equals(typedoctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        Enumeration eachAttr = doc.getAttributes();

        while (eachAttr.hasMoreElements()) {
            Attribute anAttr = (Attribute) eachAttr.nextElement();

            if (super.handleAttribute(anAttr)) {
                // nothing to do
                ;
            } else if (KEY_STORE_TYPE_ATTR.equals(anAttr.getName())) {
                keyStoreType = anAttr.getValue().trim();
            } else if (KEY_STORE_PROVIDER_ATTR.equals(anAttr.getName())) {
                keyStoreProvider = anAttr.getValue().trim();
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Attribute: " + anAttr.getName());
                }
            }
        }

        certs.clear();

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + elem.toString());
                }
            }
        }

        // Sanity Check!!!
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public PSEConfigAdv clone() {

        PSEConfigAdv result;

        try {
            result = (PSEConfigAdv) super.clone();
        } catch (CloneNotSupportedException e) {
            throw new Error("Object.clone() threw CloneNotSupportedException", e);
        }

        result.setKeyStoreLocation(getKeyStoreLocation());
        result.setKeyStoreType(getKeyStoreType());
        result.setKeyStoreProvider(getKeyStoreProvider());

        result.setEncryptedPrivateKey(getEncryptedPrivateKey(), getEncryptedPrivateKeyAlgo());
        result.setCertificateChain(getCertificateChain());

        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public ID getID() {
        InputStream data = new ByteArrayInputStream(getCert().getBytes());

        try {
            return IDFactory.newCodatID(PeerGroupID.worldPeerGroupID, new byte[16], data);
        } catch (IOException failed) {
            throw new UndeclaredThrowableException(failed, "Could not generate id");
        }
    }

    /**
     *  Returns the seed certificate. If present, this certificate will be used
     *  to initialize the PSE keystore and will be stored using the peer id of
     *  the authenticating peer.
     *
     *  @return The seed certificate or {@code null} if there is no seed
     *  certificate defined.
     */
    public X509Certificate getCertificate() {
        if (certs.isEmpty()) {
            return null;
        } else {
            return certs.get(0);
        }
    }

    /**
     *  Returns the seed certificate chain. If present, this certificate chain 
     *  will be used to initialize the PSE keystore and will be stored using the 
     *  peer id of the authenticating peer.
     *
     *  @return the seed certificate chain for this peer or {@code null} if 
     *  there is no seed certificate chain defined.
     */
    public X509Certificate[] getCertificateChain() {
        return certs.toArray(new X509Certificate[certs.size()]);
    }

    /**
     *  Returns the seed certificate encoded as a BASE64 String.
     *
     *  @return the seed certificate encoded as a BASE64 String.
     */
    public String getCert() {
        X509Certificate rootCert = getCertificate();

        if (null != rootCert) {
            try {
                return PSEUtils.base64Encode(getCertificate().getEncoded());
            } catch (Throwable failed) {
                throw new IllegalStateException("Failed to process seed cert");
            }
        } else {
            return null;
        }
    }

    /**
     *  Sets the seed certificate for this peer from a BASE64 String.
     *
     *  @param newCert The seed certificate for this peer as a BASE64 String.
     */
    public void setCert(String newCert) {
        try {
            byte[] cert_der = PSEUtils.base64Decode(new StringReader(newCert));

            CertificateFactory cf = CertificateFactory.getInstance("X509");

            setCertificate((X509Certificate) cf.generateCertificate(new ByteArrayInputStream(cert_der)));
        } catch (Exception failed) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Failed to process seed cert", failed);
            }

            IllegalArgumentException failure = new IllegalArgumentException("Failed to process seed cert");

            failure.initCause(failed);

            throw failure;
        }
    }

    /**
     *  Sets the seed certificate for this peer. If {@code null} then the 
     *  Private Key is also cleared.
     *
     *  @param newCert The seed certificate for this PSE instance or {@code null}
     *  to clear the seed certificates and private key.
     */
    public void setCertificate(X509Certificate newCert) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("setCert : " + newCert);
        }

        certs.clear();

        if (null == newCert) {
            encryptedPrivateKey = null;
        } else {
            certs.add(newCert);
        }
    }

    /**
     *  Sets the seed Certificate chain for this peer. If {@code null} then the  
     *  Private Key is also cleared.
     *
     *  @param newCerts The seed certificate chain  or {@code null}
     *  to clear the seed certificates and private key.
     */
    public void setCertificateChain(X509Certificate[] newCerts) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("setCert : " + newCerts);
        }

        certs.clear();

        if (null == newCerts) {
            encryptedPrivateKey = null;
        } else {
            certs.addAll(Arrays.asList(newCerts));
        }
    }

    /**
     *  Get the seed private key from this advertisement. The private key is
     *  retrieved from the advertisement using the provided password.
     *
     *  @param password the password to use in attempting to decrypt the private
     *  key.
     *  @return the decrypted private key.
     */
    public PrivateKey getPrivateKey(char[] password) {

        return PSEUtils.pkcs5_Decrypt_pbePrivateKey(password, privAlgorithm, encryptedPrivateKey);
    }

    /**
     *  Get the encrypted seed private key from this advertisement.
     *
     *  @return the encrypted seed private key.
     */
    public EncryptedPrivateKeyInfo getEncryptedPrivateKey() {

        return encryptedPrivateKey;
    }

    /**
     *  Get the encrypted seed private key algorithm from this advertisement.
     *
     *  @return the decrypted seed private key algorithm.
     */
    public String getEncryptedPrivateKeyAlgo() {

        return privAlgorithm;
    }

    /**
     *  Get the encrypted seed private key from this advertisement.
     *
     *  @return the encoded encrypted private key, a BASE64 String of a DER
     *  encoded PKCS8 EncrpytePrivateKeyInfo.
     */
    public String getEncryptedPrivKey() {
        try {
            if (null == encryptedPrivateKey) {
                return null;
            }

            return PSEUtils.base64Encode(encryptedPrivateKey.getEncoded());
        } catch (Exception failed) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Failed to process private key", failed);
            }

            IllegalStateException failure = new IllegalStateException("Failed to process private key");

            failure.initCause(failed);
            throw failure;
        }
    }

    /**
     *  Return the JCE Keystore type which the PSE Membership Service should use.
     *  This value should be the name of valid JCE Keystore or {@code null} if
     *  the default Keystore type should be used. The PSE Membership Service
     *  will create the keystore via 
     *  {@code KeyStore.getInstance(keystore_type)}.
     *  
     *  @return The name of the Keystore type which the PSE Membership Service
     *  will use or {@code null} if the default keystore type should be used.
     */
    public String getKeyStoreType() {
        return keyStoreType;
    }

    /**
     *  Set the JCE Keystore type which the PSE Membership Service 
     *  should use. This value should be the name of valid JCE Keystore or 
     *  {@code null} if the default Keystore type should be used. The PSE 
     *  Membership Service will create the keystore via 
     *  {@code KeyStore.getInstance(keystore_type)}.
     *
     *  @param type The JCE Keystore type which the PSE Membership Service 
     *  should use. This value should be the name of valid JCE Keystore or 
     *  {@code null} if the default Keystore type should be used.
     */
    public void setKeyStoreType(String type) {
        keyStoreType = type;
    }

    /**
     *  Return the JCE provider which the PSE Membership Service 
     *  should use for Keystores. This value should be the name of valid JCE 
     *  provider or {@code null} if the default provider should be used. The PSE 
     *  Membership Service will create the keystore via 
     *  {@code KeyStore.getInstance(keystore_type, provider)}.
     *
     *  @return The JCE provider which the PSE Membership Service 
     *  should use for Keystores. This value should be the name of valid JCE 
     *  provider or {@code null} if the default provider should be used.
     */
    public String getKeyStoreProvider() {
        return keyStoreProvider;
    }

    /**
     *  Set the JCE provider which the PSE Membership Service 
     *  should use for Keystores. This value should be the name of valid JCE 
     *  provider or {@code null} if the default provider should be used. The PSE 
     *  Membership Service will create the keystore via 
     *  {@code KeyStore.getInstance(keystore_type, provider)}.
     *
     *  @param provider The JCE provider which the PSE Membership Service 
     *  should use for Keystores. This value should be the name of valid JCE 
     *  provider or {@code null} if the default provider should be used.
     */
    public void setKeyStoreProvider(String provider) {
        keyStoreProvider = provider;
    }

    /**
     *  Return the location of the Keystore or {@code null} if the PSE 
     *  Membership Service should use the default location. The actual default
     *  location may vary depending upon they Keystore type and provider and not
     *  all location values may be valid for all Keystore types and providers.
     *  
     *  @return The location of the Keystore or {@code null} if the PSE 
     *  Membership Service should use the default location.
     */
    public URI getKeyStoreLocation() {
        return keyStoreLocation;
    }

    /**
     *  Set the location of the Keystore or {@code null} if the PSE 
     *  Membership Service should use the default location. The actual default
     *  location may vary depending upon they Keystore type and provider and not
     *  all location values may be valid for all Keystore types and providers.
     *  
     *  @param location The location of the Keystore or {@code null} if the PSE 
     *  Membership Service should use the default location.
     */
    public void setKeyStoreLocation(URI location) {
        keyStoreLocation = location;
    }

    /**
     *  Set the encrypted private key for this advertisement. The private key
     *  is provided as a BASE64 String of a DER encoded PKCS8
     *  EncrpytePrivateKeyInfo.
     *
     *  @param newPriv a BASE64 String of a DER encoded PKCS8
     *  EncrpytePrivateKeyInfo.
     *  @param algorithm The public key algorithm used by this private key.
     *  Currently only "RSA" is supported.
     */
    public void setEncryptedPrivateKey(String newPriv, String algorithm) {
        try {
            byte[] key_der = PSEUtils.base64Decode(new StringReader(newPriv));

            EncryptedPrivateKeyInfo newEncryptedPriv = new EncryptedPrivateKeyInfo(key_der);

            setEncryptedPrivateKey(newEncryptedPriv, algorithm);
        } catch (Exception failed) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Failed to process private key", failed);
            }

            IllegalArgumentException failure = new IllegalArgumentException("Failed to process private key");

            failure.initCause(failed);

            throw failure;
        }
    }

    /**
     *  Set the encrypted seed private key for this advertisement.
     *
     *  @param newPriv The encrypted seed private key.
     *  @param algorithm The public key algorithm used by this private key.
     *  Currently only "RSA" is supported.
     */
    public void setEncryptedPrivateKey(EncryptedPrivateKeyInfo newPriv, String algorithm) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("setPrivateKey : " + newPriv);
        }

        encryptedPrivateKey = newPriv;
        privAlgorithm = algorithm;
    }

    /**
     *  Set the encrypted seed private key for this advertisement.
     *
     *  @param password The password to be used in encrypting the private key
     *  @param newPriv  The private key to be stored in encrypted form.
     */
    public void setPrivateKey(PrivateKey newPriv, char[] password) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("setPrivateKey : " + newPriv);
        }

        EncryptedPrivateKeyInfo encypted = PSEUtils.pkcs5_Encrypt_pbePrivateKey(password, newPriv, 500);

        setEncryptedPrivateKey(encypted, newPriv.getAlgorithm());
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    protected boolean handleElement(Element raw) {

        if (super.handleElement(raw)) {
            return true;
        }

        XMLElement elem = (XMLElement) raw;

        if (ROOT_CERT_TAG.equals(elem.getName())) {

            Enumeration elements = elem.getChildren();

            while (elements.hasMoreElements()) {
                XMLElement eachcertelem = (XMLElement) elements.nextElement();

                if (CERT_TAG.equals(eachcertelem.getName())) {
                    // XXX bondolo 20040415 backwards compatibility
                    eachcertelem.addAttribute("type", net.jxta.impl.protocol.Certificate.getMessageType());

                    net.jxta.impl.protocol.Certificate certChain = new net.jxta.impl.protocol.Certificate(eachcertelem);

                    setCertificateChain(certChain.getCertificates());

                    continue;
                }

                if (ENCRYPTED_PRIVATE_KEY_TAG.equals(eachcertelem.getName())) {
                    String value = eachcertelem.getTextValue();

                    if (null == value) {
                        throw new IllegalArgumentException("Empty Private Key element");
                    }

                    value = value.trim();

                    Attribute algo = eachcertelem.getAttribute("algorithm");

                    if (null == algo) {
                        throw new IllegalArgumentException("Private Key element must include algorithm attribute");
                    }

                    setEncryptedPrivateKey(value, algo.getValue());
                    continue;
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + eachcertelem.getName());
                }

            }

            return true;
        }

        if (KEY_STORE_LOCATION_TAG.equals(elem.getName())) {
            try {
                keyStoreLocation = new URI(elem.getTextValue());
            } catch (URISyntaxException badURI) {
                IllegalArgumentException iae = new IllegalArgumentException("Bad key store location URI");

                iae.initCause(badURI);

                throw iae;
            }
            
            return true;
        }

        return false;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        if (adv instanceof Attributable) {
            Attributable attrDoc = (Attributable) adv;

            if (null != keyStoreType) {
                attrDoc.addAttribute(KEY_STORE_TYPE_ATTR, keyStoreType);

                if (null != keyStoreProvider) {
                    attrDoc.addAttribute(KEY_STORE_PROVIDER_ATTR, keyStoreProvider);
                }
            }
        }

        if (null != keyStoreLocation) {
            Element keyStoreLocationURI = adv.createElement(KEY_STORE_LOCATION_TAG, keyStoreLocation.toString());

            adv.appendChild(keyStoreLocationURI);
        }

        String encodedRoot = getCert();
        String encodedPrivateKey = getEncryptedPrivKey();

        if ((null != encodedRoot) && (null != encodedPrivateKey)) {
            Element rootcert = adv.createElement(ROOT_CERT_TAG, null);

            adv.appendChild(rootcert);

            // FIXME bondolo 20040501 needs to write certificate chain.

            Element cert = adv.createElement(CERT_TAG, encodedRoot);

            rootcert.appendChild(cert);

            Element privatekey = adv.createElement(ENCRYPTED_PRIVATE_KEY_TAG, encodedPrivateKey);

            rootcert.appendChild(privatekey);

            if (privatekey instanceof Attributable) {
                ((Attributable) privatekey).addAttribute("algorithm", privAlgorithm);
            }
        }

        return adv;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return INDEX_FIELDS;
    }
}
