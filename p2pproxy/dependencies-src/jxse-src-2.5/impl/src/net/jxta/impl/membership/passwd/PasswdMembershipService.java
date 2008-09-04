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

package net.jxta.impl.membership.passwd;


import java.beans.PropertyChangeListener;
import java.beans.PropertyChangeSupport;
import java.net.URI;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.HashMap;

import java.net.URISyntaxException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.credential.AuthenticationCredential;
import net.jxta.credential.Credential;
import net.jxta.credential.CredentialPCLSupport;
import net.jxta.document.Advertisement;
import net.jxta.document.Attribute;
import net.jxta.document.Attributable;
import net.jxta.document.Element;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.membership.Authenticator;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.ModuleSpecID;
import net.jxta.service.Service;

import net.jxta.exception.JxtaError;
import net.jxta.exception.ProtocolNotSupportedException;
import net.jxta.exception.PeerGroupException;


/**
 *  The passwd membership service provides a Membership Service implementation
 *  which is based on a password scheme similar to the unix
 *  <code>/etc/passwd</code> system.</code>
 *
 * <p/><strong>This implementation is intended as an example of a
 *  simple Membership Service and <em>NOT</em> as a practical secure
 *  Membership Service.<strong>
 *
 * @see net.jxta.membership.MembershipService
 *
 **/
public class PasswdMembershipService implements MembershipService {
    
    /**
     *  Log4J Logger
     **/
    private static final Logger LOG = Logger.getLogger(PasswdMembershipService.class.getName());
    
    /**
     * Well known service specification identifier: password membership
     */
    public static final ModuleSpecID passwordMembershipSpecID = (ModuleSpecID)
            ID.create(URI.create("urn:jxta:uuid-DeadBeefDeafBabaFeedBabe000000050206"));
    
    /**
     * This class provides the sub-class of Credential which is associated
     * with the password membership service.
     **/
    public final static class PasswdCredential implements Credential, CredentialPCLSupport {
        
        /**
         * The MembershipService service which generated this credential.
         **/
        PasswdMembershipService source;
        
        /**
         * The identity associated with this credential
         **/
        String whoami;
        
        /**
         * The peerid associated with this credential.
         **/
        ID peerid;
        
        /**
         * The peerid which has been "signed" so that the identity may be verified.
         **/
        String signedPeerID;
        
        /**
         *  property change support
         **/
        private PropertyChangeSupport support = new PropertyChangeSupport(this);
        
        /**
         *  Whether the credential is valid.
         **/
        boolean valid = true;
        
        protected PasswdCredential(PasswdMembershipService source, String whoami, String signedPeerID) {
            
            this.source = source;
            this.whoami = whoami;
            this.peerid = source.peergroup.getPeerID();
            this.signedPeerID = signedPeerID;
        }
        
        protected PasswdCredential(PasswdMembershipService source, Element root) throws PeerGroupException {
            
            this.source = source;
            
            initialize(root);
        }
        
        /**
         *  Add a listener
         *
         *  @param listener the listener
         **/
        public void addPropertyChangeListener(PropertyChangeListener listener) {
            support.addPropertyChangeListener(listener);
        }
        
        /**
         *  Add a listener
         *
         *  @param propertyName the property to watch
         *  @param listener the listener
         **/
        public void addPropertyChangeListener(String propertyName, PropertyChangeListener listener) {
            support.addPropertyChangeListener(propertyName, listener);
        }
        
        /**
         *  Remove a listener
         *
         *  @param listener the listener
         **/
        public void removePropertyChangeListener(PropertyChangeListener listener) {
            support.removePropertyChangeListener(listener);
        }
        
        /**
         *  Remove a listener
         *
         *  @param propertyName the property which was watched
         *  @param listener the listener
         **/
        public void removePropertyChangeListener(String propertyName, PropertyChangeListener listener) {
            support.removePropertyChangeListener(propertyName, listener);
        }
        
        /**
         * {@inheritDoc}
         **/
        public ID getPeerGroupID() {
            return source.peergroup.getPeerGroupID();
        }
        
        /**
         * {@inheritDoc}
         **/
        public ID getPeerID() {
            return peerid;
        }
        
        /**
         *  Set the peerid for this credential.
         *
         *  @param  peerid   the peerid for this credential
         **/
        private void setPeerID(PeerID peerid) {
            this.peerid = peerid;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>PasswdCredential never expire.
         **/
        public boolean isExpired() {
            return false;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>PasswdCredential are almost always valid.
         **/
        public boolean isValid() {
            return valid;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>PasswdCredential are always valid except after resign.
         **/
        private void setValid(boolean valid) {
            boolean oldValid = isValid();

            this.valid = valid;
            
            if (oldValid != valid) {
                support.firePropertyChange("valid", oldValid, valid);
            }
        }
        
        /**
         * {@inheritDoc}
         **/
        public Object getSubject() {
            return whoami;
        }
        
        /**
         *  Sets the subject for this Credential
         *
         *  @param  subject The subject for this credential.
         **/
        private void setSubject(String subject) {
            whoami = subject;
        }
        
        /**
         * {@inheritDoc}
         **/
        public Service getSourceService() {
            return source;
        }
        
        /**
         * {@inheritDoc}
         **/
        public StructuredDocument getDocument(MimeMediaType as) throws Exception {
            StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(as, "jxta:Cred");
            
            if (doc instanceof XMLDocument) {
                ((Attributable) doc).addAttribute("xmlns:jxta", "http://jxta.org");
                ((Attributable) doc).addAttribute("xml:space", "preserve");
            }
            
            if (doc instanceof Attributable) {
                ((Attributable) doc).addAttribute("type", "jxta:PasswdCred");
            }
            
            Element e = doc.createElement("PeerGroupID", getPeerGroupID().toString());

            doc.appendChild(e);
            
            e = doc.createElement("PeerID", getPeerID().toString());
            doc.appendChild(e);
            
            e = doc.createElement("Identity", whoami);
            doc.appendChild(e);
            
            // FIXME 20010327   Do some kind of signing here based on password.
            e = doc.createElement("ReallyInsecureSignature", signedPeerID);
            doc.appendChild(e);
            
            return doc;
        }
        
        /**
         *  Process an individual element from the document.
         *
         *  @param elem the element to be processed.
         *  @return true if the element was recognized, otherwise false.
         **/
        protected boolean handleElement(XMLElement elem) {
            if (elem.getName().equals("PeerGroupID")) {
                try {
                    URI gID = new URI(elem.getTextValue());
                    ID pgid = IDFactory.fromURI(gID);

                    if (!pgid.equals(getPeerGroupID())) {
                        throw new IllegalArgumentException(
                                "Operation is from a different group. " + pgid + " != " + getPeerGroupID());
                    }
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("Bad PeerGroupID in advertisement: " + elem.getTextValue());
                }
                return true;
            }
            
            if (elem.getName().equals("PeerID")) {
                try {
                    URI pID = new URI(elem.getTextValue());
                    ID pid = IDFactory.fromURI(pID);

                    setPeerID((PeerID) pid);
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("Bad Peer ID in advertisement: " + elem.getTextValue());
                } catch (ClassCastException badID) {
                    throw new IllegalArgumentException("Id is not a peer id: " + elem.getTextValue());
                }
                return true;
            }
            
            if (elem.getName().equals("Identity")) {
                setSubject(elem.getTextValue());
                return true;
            }
            
            if (elem.getName().equals("ReallyInsecureSignature")) {
                signedPeerID = elem.getTextValue();
                return true;
            }
            
            // element was not handled
            return false;
        }
        
        /**
         *  Intialize from a portion of a structured document.
         **/
        protected void initialize(Element root) {
            
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
            }
            
            XMLElement doc = (XMLElement) root;
            
            String typedoctype = "";
            Attribute itsType = ((Attributable) root).getAttribute("type");

            if (null != itsType) {
                typedoctype = itsType.getValue();
            }
            
            String doctype = doc.getName();
            
            if (!doctype.equals("jxta:PasswdCred") && !typedoctype.equals("jxta:PasswdCred")) {
                throw new IllegalArgumentException(
                        "Could not construct : " + getClass().getName() + "from doc containing a " + doctype);
            }
            
            Enumeration elements = doc.getChildren();
            
            while (elements.hasMoreElements()) {
                XMLElement elem = (XMLElement) elements.nextElement();

                if (!handleElement(elem)) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandleded element \'" + elem.getName() + "\' in " + doc.getName());
                    }
                }
            }
            
            // sanity check time!
            
            if (null == getSubject()) {
                throw new IllegalArgumentException("subject was never initialized.");
            }
            
            if (null == getPeerID()) {
                throw new IllegalArgumentException("peer id was never initialized.");
            }
            
            if (null == signedPeerID) {
                throw new IllegalArgumentException("signed peer id was never initialized.");
            }
            
            // FIXME bondolo@jxta.org 20030409 should check for duplicate elements and for peergroup element
        }
    }
    

    /**
     * Creates an authenticator for the passwd membership service. Anything
     *  entered into the identity info section of the Authentication
     *  credential is ignored.
     *
     *  <p/><strong>HACK ALERT!</strong> THE INHERITANCE FROM
     *  <code>net.jxta.impl.membership.PasswdMembershipService.PasswdAuthenticator</code>
     *  IS A TOTAL HACK FOR BACKWARDS COMPATIBILITY.
     *
     *  @param source The instance of the passwd membership service which
     *  created this authenticator.
     *  @param application Anything entered into the identity info section of
     *  the Authentication credential is ignored.
     **/
    public final static class PasswdAuthenticator extends net.jxta.impl.membership.PasswdMembershipService.PasswdAuthenticator {
        
        /**
         * The Membership Service which generated this authenticator.
         **/
        PasswdMembershipService source;
        
        /**
         * The Authentication which was provided to the Apply operation of the
         * membership service.
         **/
        AuthenticationCredential application;
        
        /**
         * the identity which is being claimed
         **/
        String whoami = null;
        
        /**
         * the password for that identity.
         **/
        String password = null;
        
        /**
         * Creates an authenticator for the password MembershipService service. The only method
         * supported is "PasswdAuthentication". Anything entered into the identity info
         * section of the Authentication credential is ignored.
         *
         * @param source The instance of the password membership service which created this
         * authenticator.
         * @param application The Anything entered into the identity info section of the Authentication
         * credential is ignored.
         **/
        PasswdAuthenticator(PasswdMembershipService source, AuthenticationCredential application) {
            this.source = source;
            this.application = application;
            
            // XXX 20010328 bondolo@jxta.org Could do something with the authentication credential here.
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
        synchronized public boolean isReadyForJoin() {
            return ((null != password) && (null != whoami));
        }
        
        /**
         * {@inheritDoc}
         **/
        public String getMethodName() {
            return "PasswdAuthentication";
        }
        
        /**
         * {@inheritDoc}
         **/
        public AuthenticationCredential getAuthenticationCredential() {
            return application;
        }
        
        @Override
        public void setAuth1Identity(String who) {
            whoami = who;
        }
        
        @Override
        public String getAuth1Identity() {
            return whoami;
        }
        
        @Override
        public void setAuth2_Password(String secret) {
            password = secret;
        }
        
        @Override
        protected String getAuth2_Password() {
            return password;
        }
    }
        
    /**
     * the peergroup to which this service is associated.
     **/
    private PeerGroup peergroup = null;
    
    /**
     *  the default "nobody" credential
     **/
    private Credential  defaultCredential = null;
    
    /**
     * The current set of principals associated with this peer within this peegroup.
     **/
    private List principals;
    
    /**
     * The set of AuthenticationCredentials which were used to establish the principals.
     **/
    private List authCredentials;
    
    /**
     * The ModuleImplAdvertisement which was used to instantiate this service.
     **/
    private ModuleImplAdvertisement implAdvertisement = null;
    
    /**
     * An internal table containing the identity and password pairs as parsed from the
     * the PeerGroupAdvertisement.
     **/
    private Map logins = null;
    
    /**
     *  property change support
     **/
    private PropertyChangeSupport support;
    
    /**
     *  Default constructor. Normally only called by the peer group.
     **/
    public PasswdMembershipService() throws PeerGroupException {
        principals = new ArrayList();
        authCredentials = new ArrayList();
        
        support = new PropertyChangeSupport(getInterface());
    }
    
    /**
     *  Add a listener
     *
     *  @param listener the listener
     **/
    public void addPropertyChangeListener(PropertyChangeListener listener) {
        support.addPropertyChangeListener(listener);
    }
    
    /**
     *  Add a listener
     *
     *  @param propertyName the property to watch
     *  @param listener the listener
     **/
    public void addPropertyChangeListener(String propertyName, PropertyChangeListener listener) {
        support.addPropertyChangeListener(propertyName, listener);
    }
    
    /**
     *  Remove a listener
     *
     *  @param listener the listener
     **/
    public void removePropertyChangeListener(PropertyChangeListener listener) {
        support.removePropertyChangeListener(listener);
    }
    
    /**
     *  Remove a listener
     *
     *  @param propertyName the property which was watched
     *  @param listener the listener
     **/
    public void removePropertyChangeListener(String propertyName, PropertyChangeListener listener) {
        support.removePropertyChangeListener(propertyName, listener);
    }
    
    /**
     * {@inheritDoc}
     **/
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {
        
        peergroup = group;
        implAdvertisement = (ModuleImplAdvertisement) impl;
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Password Membership Service : " + assignedID);

            configInfo.append("\n\tImplementation:");
            configInfo.append("\n\t\tModule Spec ID: " + implAdvertisement.getModuleSpecID());
            configInfo.append("\n\t\tImpl Description : " + implAdvertisement.getDescription());
            configInfo.append("\n\t\tImpl URI : " + implAdvertisement.getUri());
            configInfo.append("\n\t\tImpl Code : " + implAdvertisement.getCode());
            configInfo.append("\n\tGroup Params:");
            configInfo.append("\n\t\tGroup: " + group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID: " + group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID: " + group.getPeerID());
            LOG.config(configInfo.toString());
        }
        
        PeerGroupAdvertisement configAdv = group.getPeerGroupAdvertisement();
        
        XMLElement myParam = (XMLElement) configAdv.getServiceParam(assignedID);
        
        logins = new HashMap();
        
        if (null == myParam) {
            throw new PeerGroupException("parameters for group passwords missing");
        }
        
        for (Enumeration allLogins = myParam.getChildren(); allLogins.hasMoreElements();) {
            XMLElement aLogin = (XMLElement) allLogins.nextElement();

            if (aLogin.getName().equals("login")) {
                String etcPasswd = aLogin.getTextValue();
                int nextDelim = etcPasswd.indexOf(':');

                if (-1 == nextDelim) {
                    continue;
                }
                String login = etcPasswd.substring(0, nextDelim).trim();
                int lastDelim = etcPasswd.indexOf(':', nextDelim + 1);
                String passwd = etcPasswd.substring(nextDelim + 1, lastDelim);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Adding login : \'" + login + "\' with encoded password : \'" + passwd + "\'");
                }
                logins.put(login, passwd);
            }
        }
        
        // FIXME    20010327    bondolo@jxta.org Make up the signed bit.
        
        // We initialise our set of principals to the resigned state.
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
        return 0;
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>This request is currently ignored.
     **/
    public void stopApp() {
        resign();
    }
    
    /**
     * {@inheritDoc}
     **/
    public Authenticator apply(AuthenticationCredential application) throws PeerGroupException, ProtocolNotSupportedException {
        
        String method = application.getMethod();
        
        if ((null != method) && !"StringAuthentication".equals(method) && !"PasswdAuthentication".equals(method)) {
            throw new ProtocolNotSupportedException("Authentication method not recognized");
        }
        
        return new PasswdAuthenticator(this, application);
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential getDefaultCredential() {
        return defaultCredential;
    }
    
    /**
     * {@inheritDoc}
     **/
    private void setDefaultCredential(Credential newDefault) {
        Credential oldDefault = defaultCredential;

        defaultCredential = newDefault;
        
        support.firePropertyChange("defaultCredential", oldDefault, newDefault);
    }
    
    /**
     * {@inheritDoc}
     **/
    public synchronized Enumeration<Credential> getCurrentCredentials() {
        return Collections.enumeration(principals);
    }
    
    /**
     * {@inheritDoc}
     **/
    public synchronized Enumeration<AuthenticationCredential> getAuthCredentials() {
        return Collections.enumeration(authCredentials);
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential join(Authenticator authenticated) throws PeerGroupException {
        
        if (!(authenticated instanceof PasswdAuthenticator)) {
            throw new ClassCastException("This is not my authenticator!");
        }
        
        if (this != authenticated.getSourceService()) {
            throw new ClassCastException("This is not my authenticator!");
        }
        
        if (!authenticated.isReadyForJoin()) {
            throw new PeerGroupException("Not Ready to join!");
        }
        
        String identity = ((PasswdAuthenticator) authenticated).getAuth1Identity();
        String password = ((PasswdAuthenticator) authenticated).getAuth2_Password();
        
        if (!checkPasswd(identity, password)) {
            throw new PeerGroupException("Incorrect Password!");
        }
        
        // FIXME    20010327    bondolo@jxta.org Make up the signed bit.
        
        Credential newCred;

        synchronized (this) {
            newCred = new PasswdCredential(this, identity, "blah");
            
            principals.add(newCred);
            
            authCredentials.add(authenticated.getAuthenticationCredential());
        }
        
        support.firePropertyChange("addCredential", null, newCred);
        
        if (null == getDefaultCredential()) {
            setDefaultCredential(newCred);
        }
        
        return newCred;
    }
    
    /**
     * {@inheritDoc}
     **/
    public synchronized void resign() {
        Iterator eachCred = Arrays.asList(principals.toArray()).iterator();
        
        synchronized (this) {
            principals.clear();
            authCredentials.clear();
        }
        
        setDefaultCredential(null);
        
        while (eachCred.hasNext()) {
            PasswdCredential aCred = (PasswdCredential) eachCred.next();
            
            aCred.setValid(false);
        }
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential makeCredential(Element element) throws PeerGroupException, Exception {
        
        return new PasswdCredential(this, element);
    }
    
    /**
     * Given an identity and an encoded password determine if the password is
     * correct.
     *
     * @param identity the identity which the user is trying to claim
     * @param passwd the password guess being tested.
     * @return true if the password was correct for the specified identity
     * otherwise false.
     **/
    private boolean checkPasswd(String identity, String passwd) {
        boolean result;
        
        if (!logins.containsKey(identity)) {
            return false;
        }
        
        String encodedPW = makePsswd(passwd);
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Password \'" + passwd + "\' encodes as: \'" + encodedPW + "\'");
        }
        
        String mustMatch = (String) logins.get(identity);
        
        // if there is a null password for this identity then match everything.
        if (mustMatch.equals("")) {
            return true;
        }
        
        result = encodedPW.equals(mustMatch);
        
        return result;
    }
    
    /**
     *  This is the method used to make the password strings. We only provide
     *  one way encoding since we can compare the encoded strings.
     *
     *  <p/>FIXME 20010402  bondolo : switch to use the standard
     *  crypt(3) algorithm for encoding the passwords. The current algorithm has
     *  been breakable since ancient times, crypt(3) is also weak, but harder to
     *  break.
     *
     *   @param source  the string to encode
     *   @return String the encoded version of the password.
     *
     **/
    public static String makePsswd(String source) {

        /**
         *
         * A->D  B->Q  C->K  D->W  E->H  F->R  G->T  H->E  I->N  J->O  K->G  L->X  M->C
         * N->V  O->Y  P->S  Q->F  R->J  S->P  T->I  U->L  V->Z  W->A  X->B  Y->M  Z->U
         *
         **/
        
        final String xlateTable = "DQKWHRTENOGXCVYSFJPILZABMU";
        
        StringBuilder work = new StringBuilder(source);
        
        for (int eachChar = work.length() - 1; eachChar >= 0; eachChar--) {
            char aChar = Character.toUpperCase(work.charAt(eachChar));
            
            int replaceIdx = xlateTable.indexOf(aChar);

            if (-1 != replaceIdx) {
                work.setCharAt(eachChar, (char) ('A' + replaceIdx));
            }
        }
        
        return work.toString();
    }
}

