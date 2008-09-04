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


import net.jxta.credential.Credential;
import net.jxta.credential.CredentialPCLSupport;
import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.service.Service;

import java.beans.PropertyChangeListener;
import java.beans.PropertyChangeSupport;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.SequenceInputStream;
import java.io.StringReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.InvalidKeyException;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.Signature;
import java.security.SignatureException;
import java.security.cert.CertPath;
import java.security.cert.CertificateExpiredException;
import java.security.cert.CertificateFactory;
import java.security.cert.CertificateNotYetValidException;
import java.security.cert.X509Certificate;
import java.security.cert.Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * This class provides the sub-class of Credential which is associated with the
 * PSE membership service.
 * <p/>
 * There are two varients of the credential:
 * <p/>
 * <ul>
 * <li>local - Generated as a result of local login. This type of
 * credential can be used for signing and can be serialized for inclusion
 * in protocols.</li>
 * <li>remote - Generated as a result of deserialization from protocols.
 * The credential is verified to ensure that the contents are valid at the
 * time it is created.</li>
 * </ul>
 * <p/>
 * The schema for this credential format:
 * <p/>
 * <pre><code>
 *  &lt;xs:element name="PSECred" type="jxta:PSECred" />
 * <p/>
 *  &lt;xs:complexType name="PSECred">
 *    &lt;xs:sequence>
 *      &lt;xs:element name="PeerGroupID" type="jxta:JXTAID" />
 *      &lt;xs:element name="PeerID" type="jxta:JXTAID" />
 *      &lt;!-- An X.509 Certificate -->
 *      &lt;xs:element name="Certificate" type="xs:string" minOccurs="1" maxOccurs="unbounded" />
 *      &lt;!-- A SHA1WithRSA Signature -->
 *      &lt;xs:element name="Signature" type="xs:string" />
 *    &lt;/xs:sequence>
 *  &lt;/xs:complexType>
 *  &lt;/code></pre>
 * <p/>
 * FIXME 20050625 bondolo If the certificate chain for a credential is
 * updated in the PSE keystore after a credential is created then the
 * credential instance will not reflect those changes. This can be a problem if
 * the issuer chain changes or expiries are updated. Even though it's going to
 * be hit on performance PSECredential needs to changed to be backed by the PSE
 * keystore directly rather than containing the certs. Either that or some kind
 * of notification systems. It's probably best to assume that our simple cm
 * based keystore is the easiest and least dynamic case. Every other key store
 * is going to be more dynamic and difficult. The work around for now is to
 * force a membership resign everytime the keystore contents are changed.
 *
 * @see net.jxta.credential.Credential
 * @see net.jxta.impl.membership.pse.PSEMembershipService
 */
public final class PSECredential implements Credential, CredentialPCLSupport {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(PSECredential.class.getName());

    /**
     * A Timer we use for managing the cert expirations.
     */
    private static Timer expirationTimer = new Timer("PSECredential Expiration Timer", true);

    /**
     * The MembershipService service which generated this credential.
     * <p/>
     * XXX 20030609 bondolo@jxta.org Perhaps this should be a weak reference.
     */
    private PSEMembershipService source;

    /**
     * The peer group associated with this credential.
     */
    private ID peerGroupID = null;

    /**
     * The peerid associated with this credential.
     */
    private ID peerID = null;

    /**
     * The pse alias from which this credential was generated. Only locally
     * created credentials will be intialized with a key ID.
     */
    private ID keyID = null;

    /**
     * The identity associated with this credential
     */
    private CertPath certs = null;

    /**
     * The private key associated with this credential. Used for signing. Only
     * a locally created credential will have an initialized private key.
     */
    private PrivateKey privateKey = null;

    /**
     * Optional Timer task
     */
    private TimerTask becomesValidTask = null;
    private TimerTask expiresTask = null;

    /**
     * Are we still a valid credential?
     */
    private boolean valid = true;

    /**
     * Is this a local credential?
     */
    private final boolean local;

    /**
     * property change support
     */
    private PropertyChangeSupport support = new PropertyChangeSupport(this);

    /**
     * Create a new local credential. This credential can be used for signing
     * and can be serialized.
     */
    protected PSECredential(PSEMembershipService source, ID keyID, CertPath certChain, PrivateKey privateKey) throws IOException {
        this.source = source;
        this.peerID = source.group.getPeerID();
        this.peerGroupID = source.group.getPeerGroupID();
        setKeyID(keyID);
        setCertificateChain(certChain);
        setPrivateKey(privateKey);
        this.local = true;
    }

    /**
     * Create a new remote credential. This credential cannot be used for
     * signing and cannot be re-serialized.
     */
    public PSECredential(Element root) {
        this.local = false;
        initialize(root);
    }

    /**
     * Create a new remote credential. This credential cannot be used for
     * signing and cannot be re-serialized.
     */
    public PSECredential(PSEMembershipService source, Element root) {
        this.local = false;
        this.source = source;
        initialize(root);

        if (!peerGroupID.equals(source.group.getPeerGroupID())) {
            throw new IllegalArgumentException(
                    "Credential is from a different group. " + peerGroupID + " != " + source.group.getPeerGroupID());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {

        if (this == target) {
            return true;
        }

        if (target instanceof PSECredential) {
            PSECredential asCred = (PSECredential) target;

            boolean result = peerID.equals(asCred.peerID)
                    && source.group.getPeerGroupID().equals(asCred.source.group.getPeerGroupID());

            result &= certs.equals(asCred.certs);

            return result;
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void finalize() throws Throwable {
        if (null != becomesValidTask) {
            becomesValidTask.cancel();
        }

        if (null != expiresTask) {
            expiresTask.cancel();
        }

        super.finalize();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        int result = peerID.hashCode() * source.group.getPeerGroupID().hashCode() * certs.hashCode();

        if (0 == result) {
            result = 1;
        }

        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        return "\"" + getSubject() + "\" " + getPeerID() + " [" + source + " / " + getPeerGroupID() + "]";
    }

    /**
     * Add a listener
     *
     * @param listener the listener
     */
    public void addPropertyChangeListener(PropertyChangeListener listener) {
        support.addPropertyChangeListener(listener);
    }

    /**
     * Add a listener
     *
     * @param propertyName the property to watch
     * @param listener     the listener
     */
    public void addPropertyChangeListener(String propertyName, PropertyChangeListener listener) {
        support.addPropertyChangeListener(propertyName, listener);
    }

    /**
     * Remove a listener
     *
     * @param listener the listener
     */
    public void removePropertyChangeListener(PropertyChangeListener listener) {
        support.removePropertyChangeListener(listener);
    }

    /**
     * Remove a listener
     *
     * @param propertyName the property which was watched
     * @param listener     the listener
     */
    public void removePropertyChangeListener(String propertyName, PropertyChangeListener listener) {
        support.removePropertyChangeListener(propertyName, listener);
    }

    /**
     * {@inheritDoc}
     */
    public ID getPeerGroupID() {
        return peerGroupID;
    }

    /**
     * set the peer id associated with this credential
     */
    private void setPeerGroupID(ID newID) {
        this.peerGroupID = newID;
    }

    /**
     * {@inheritDoc}
     */
    public ID getPeerID() {
        return peerID;
    }

    /**
     * set the peer id associated with this credential
     */
    private void setPeerID(PeerID peerID) {
        this.peerID = peerID;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * A PSE Credential is valid as long as the associated certificate is
     * valid.
     */
    public boolean isExpired() {
        try {
            ((X509Certificate) certs.getCertificates().get(0)).checkValidity();
            return false;
        } catch (CertificateExpiredException expired) {
            return true;
        } catch (CertificateNotYetValidException notyet) {
            return true;
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * A PSE Credential is valid as long as the associated certificate is
     * valid and as long as the membership service still has the credential.
     */
    public boolean isValid() {
        return valid && !isExpired();
    }

    /**
     * {@inheritDoc}
     * <p/>
     * A PSE Credential is valid as long as the associated certificate is
     * valid.
     */
    void setValid(boolean valid) {
        boolean oldValid = isValid();

        this.valid = valid;

        if (oldValid != valid) {
            support.firePropertyChange("valid", oldValid, valid);
        }
    }

    /**
     * {@inheritDoc}
     */
    public Object getSubject() {
        return ((X509Certificate) certs.getCertificates().get(0)).getSubjectDN();
    }

    /**
     * {@inheritDoc}
     */
    public Service getSourceService() {
        return source;
    }

    /**
     * {@inheritDoc}
     */
    public StructuredDocument getDocument(MimeMediaType encodeAs) throws Exception {
        if (!isValid()) {
            throw new javax.security.cert.CertificateException("Credential is not valid. Cannot generate document.");
        }

        if (!local) {
            throw new IllegalStateException("This credential is not a local credential and document cannot be created.");
        }

        StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(encodeAs, "jxta:Cred");

        if (doc instanceof XMLDocument) {
            ((XMLDocument) doc).addAttribute("xmlns:jxta", "http://jxta.org");
            ((XMLDocument) doc).addAttribute("xml:space", "preserve");
        }

        if (doc instanceof Attributable) {
            ((Attributable) doc).addAttribute("type", "jxta:PSECred");
        }

        Element e;

        e = doc.createElement("PeerGroupID", getPeerGroupID().toString());
        doc.appendChild(e);

        e = doc.createElement("PeerID", getPeerID().toString());
        doc.appendChild(e);

        // add the Certificate element

        net.jxta.impl.protocol.Certificate certChain = new net.jxta.impl.protocol.Certificate();

        List certsList = certs.getCertificates();

        certChain.setCertificates(certsList);

        StructuredDocument certsDoc = (StructuredDocument) certChain.getDocument(encodeAs);

        if (certsDoc instanceof Attributable) {
            ((Attributable) certsDoc).addAttribute("type", certsDoc.getKey().toString());
        }

        StructuredDocumentUtils.copyElements(doc, doc, certsDoc, "Certificate");

        // Add the signature.

        List someStreams = new ArrayList(3);

        try {
            someStreams.add(new ByteArrayInputStream(getPeerGroupID().toString().getBytes("UTF-8")));
            someStreams.add(new ByteArrayInputStream(getPeerID().toString().getBytes("UTF-8")));
            for (Object aCertsList : certsList) {
                X509Certificate aCert = (X509Certificate) aCertsList;

                someStreams.add(new ByteArrayInputStream(aCert.getEncoded()));
            }

            InputStream signStream = new SequenceInputStream(Collections.enumeration(someStreams));

            byte[] sig = source.peerSecurityEngine.sign(source.peerSecurityEngine.getSignatureAlgorithm(), this, signStream);

            e = doc.createElement("Signature", PSEUtils.base64Encode(sig));
            doc.appendChild(e);
        } catch (java.io.UnsupportedEncodingException never) {// UTF-8 is always available
        }

        if (doc instanceof Attributable) {
            ((Attributable) doc).addAttribute("algorithm", source.peerSecurityEngine.getSignatureAlgorithm());
        }

        return doc;
    }

    /**
     * Returns the certificate associated with this credential.
     *
     * @return the certificate associated with this credential.
     */
    public X509Certificate getCertificate() {
        return (X509Certificate) certs.getCertificates().get(0);
    }

    /**
     * Returns the certificate chain associated with this credential.
     *
     * @return the certificate chain associated with this credential.
     */
    public X509Certificate[] getCertificateChain() {
        List certList = certs.getCertificates();

        return (X509Certificate[]) certList.toArray(new X509Certificate[certList.size()]);
    }

    /**
     * Set the certificate associated with this credential
     *
     * @param certChain the certificate chain associated with this credential.
     */
    private void setCertificateChain(CertPath certChain) {

        certs = certChain;

        Date now = new Date();
        Date becomesValid = ((X509Certificate) certs.getCertificates().get(0)).getNotBefore();
        Date expires = ((X509Certificate) certs.getCertificates().get(0)).getNotAfter();

        if (becomesValid.compareTo(now) > 0) {
            if (null != becomesValidTask) {
                becomesValidTask.cancel();
            }

            becomesValidTask = new TimerTask() {

                @Override
                public void run() {
                    support.firePropertyChange("expired", false, true);
                    if (valid) {
                        support.firePropertyChange("valid", false, true);
                    }
                }
            };

            expirationTimer.schedule(becomesValidTask, becomesValid);
        }

        if (null != expiresTask) {
            expiresTask.cancel();
        }

        if (expires.compareTo(now) > 0) {
            expiresTask = new TimerTask() {

                @Override
                public void run() {
                    support.firePropertyChange("expired", true, false);
                    if (valid) {
                        support.firePropertyChange("valid", true, false);
                    }
                }
            };

            expirationTimer.schedule(expiresTask, expires);
        }

        boolean nowGood = (null == becomesValidTask) && (null != expiresTask);

        support.firePropertyChange("expired", true, nowGood);
        setValid(nowGood);
    }

    /**
     * Returns the private key associated with this credential. Only valid for
     * locally generated credentials.
     *
     * @return the private key associated with this credential.
     * @deprecated Use <@link #getSigner(String)> or <@link #getSignatureVerifier(String)> instead.
     */
    @Deprecated
    public PrivateKey getPrivateKey() {

        if (!local) {
            throw new IllegalStateException("This credential is not a local credential and cannot be used for signing.");
        }

        if (null == privateKey) {
            throw new IllegalStateException("This local credential is engine based and cannot provide the private key.");
        }

        return privateKey;
    }

    /**
     * Sets the private key associated with this credential.
     *
     * @param privateKey the private key associated with this credential.
     */
    private void setPrivateKey(PrivateKey privateKey) {

        this.privateKey = privateKey;
    }

    /**
     * Returns the key id associated with this credential, if any. Only locally
     * generated credentials have a key ID.
     *
     * @return Returns the key id associated with this credential, if any.
     */
    public ID getKeyID() {
        return keyID;
    }

    /**
     * Sets the key id associated with this credential.
     */
    private void setKeyID(ID keyID) {
        this.keyID = keyID;
    }

    /**
     * Get a Signature object based upon the private key associated with this
     * credential.
     *
     * @param algorithm the signing algorithm to use.
     * @return Signature.
     */
    public Signature getSigner(String algorithm) throws NoSuchAlgorithmException {
        if (!local) {
            throw new IllegalStateException("This credential is not a local credential and cannot be used for signing.");
        }

        Signature sign = Signature.getInstance(algorithm);

        try {
            sign.initSign(privateKey);
        } catch (java.security.InvalidKeyException failed) {
            IllegalStateException failure = new IllegalStateException("Invalid private key");

            failure.initCause(failed);
            throw failure;
        }

        return sign;
    }

    /**
     * /**
     * Get a Signature verifier object based upon the certificate associated
     * with this credential.
     *
     * @param algorithm the signing algorithm to use.
     * @return Signature.
     */
    public Signature getSignatureVerifier(String algorithm) throws NoSuchAlgorithmException {
        Signature verify = Signature.getInstance(algorithm);

        try {
            verify.initVerify((X509Certificate) certs.getCertificates().get(0));
        } catch (java.security.InvalidKeyException failed) {
            IllegalStateException failure = new IllegalStateException("Invalid certificate");

            failure.initCause(failed);
            throw failure;
        }

        return verify;
    }

    /**
     * Process an individual element from the document.
     *
     * @param elem the element to be processed.
     * @return true if the element was recognized, otherwise false.
     */
    protected boolean handleElement(XMLElement elem) {
        if (elem.getName().equals("PeerGroupID")) {
            try {
                ID pid = IDFactory.fromURI(new URI(elem.getTextValue()));

                setPeerGroupID((PeerGroupID) pid);
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad PeerGroupID in advertisement: " + elem.getTextValue());
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Id is not a group id: " + elem.getTextValue());
            }
            return true;
        }

        if (elem.getName().equals("PeerID")) {
            try {
                ID pid = IDFactory.fromURI(new URI(elem.getTextValue()));

                setPeerID((PeerID) pid);
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad Peer ID in advertisement: " + elem.getTextValue());
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Id is not a peer id: " + elem.getTextValue());
            }
            return true;
        }

        if (elem.getName().equals("Certificate")) {
            // XXX Compatibility hack so that net.jxta.impl.protocol.Certificate will recognize element
            // as a certificate.
            if (null == elem.getAttribute("type")) {
                elem.addAttribute("type", net.jxta.impl.protocol.Certificate.getMessageType());
            }

            net.jxta.impl.protocol.Certificate certChain = new net.jxta.impl.protocol.Certificate(elem);

            try {
                CertificateFactory cf = CertificateFactory.getInstance("X.509");

                certs = cf.generateCertPath(Arrays.asList(certChain.getCertificates()));
            } catch (java.security.cert.CertificateException failure) {
                throw new IllegalArgumentException("bad certificates in chain.");
            }

            return true;
        }

        if (elem.getName().equals("Signature")) {

            if (null == certs) {
                throw new IllegalArgumentException("Signature out of order in Credential.");
            }

            List<InputStream> someStreams = new ArrayList<InputStream>(3);

            try {
                byte[] signatureToCompare = PSEUtils.base64Decode(new StringReader(elem.getTextValue()));

                someStreams.add(new ByteArrayInputStream(getPeerGroupID().toString().getBytes("UTF-8")));
                someStreams.add(new ByteArrayInputStream(getPeerID().toString().getBytes("UTF-8")));
                Iterator eachCert = certs.getCertificates().iterator();

                for (Certificate certificate : certs.getCertificates()) {
                    X509Certificate aCert = (X509Certificate) certificate;

                    someStreams.add(new ByteArrayInputStream(aCert.getEncoded()));
                }

                InputStream signStream = new SequenceInputStream(Collections.enumeration(someStreams));

                // FIXME 20051007 bondolo Fix handling of signature type.

                if (!PSEUtils.verifySignature("SHA1WITHRSA", getCertificate(), signatureToCompare, signStream)) {
                    throw new IllegalArgumentException("Certificated did not match");
                }
            } catch (Throwable failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to validate signature ", failed);
                }

                throw new IllegalArgumentException("Failed to validate signature " + failed.getMessage());
            }

            return true;
        }

        // element was not handled
        return false;
    }

    /**
     * Intialize from a portion of a structured document.
     */
    protected void initialize(Element root) {

        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
        }

        XMLElement doc = (XMLElement) root;

        String typedoctype = "";

        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        String doctype = doc.getName();

        if (!doctype.equals("jxta:PSECred") && !typedoctype.equals("jxta:PSECred")) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doctype);
        }

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled element \'" + elem.getName() + "\' in " + doc.getName());
                }
            }
        }

        // sanity check time!

        if (null == getSubject()) {
            throw new IllegalArgumentException("subject was never initialized.");
        }

        if (null == getPeerGroupID()) {
            throw new IllegalArgumentException("peer group was never initialized.");
        }

        if (null == getPeerID()) {
            throw new IllegalArgumentException("peer id was never initialized.");
        }

        if (null == certs) {
            throw new IllegalArgumentException("certificates were never initialized.");
        }

        // FIXME bondolo@jxta.org 20030409 should check for duplicate elements and for peergroup element
    }

    public X509Certificate[] generateServiceCertificate(ID assignedID) throws IOException, KeyStoreException, InvalidKeyException, SignatureException {
        return source.generateServiceCertificate(assignedID, this);
    }

    public PSECredential getServiceCredential(ID assignedID) throws IOException, PeerGroupException, InvalidKeyException, SignatureException {
        return source.getServiceCredential(assignedID, this);
    }
}
