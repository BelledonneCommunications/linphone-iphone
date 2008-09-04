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

package net.jxta.impl.membership.none;


import java.beans.PropertyChangeListener;
import java.beans.PropertyChangeSupport;
import java.net.URI;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;

import java.net.URISyntaxException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.credential.AuthenticationCredential;
import net.jxta.credential.Credential;
import net.jxta.credential.CredentialPCLSupport;
import net.jxta.document.Attribute;
import net.jxta.document.Attributable;
import net.jxta.document.Advertisement;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.TextElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.membership.Authenticator;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.service.Service;

import net.jxta.exception.ProtocolNotSupportedException;
import net.jxta.exception.PeerGroupException;


/**
 *  A Membership Service implementation which is intended to be used with peer
 *  groups which require no real authentication.
 *
 * <p/>The none service allows you to claim any identity within the peergroup,
 * but for peergroups which use this Membership Service method, it is
 * likely that the identity is used only for informational purposes.
 *
 * <p/>A default credential with the name "nobody" is automatically available
 * without requiring authentication by this service.
 *
 */
public class NoneMembershipService implements MembershipService {
    
    /**
     *  Log4J Logger
     **/
    private static final Logger LOG = Logger.getLogger(NoneMembershipService.class.getName());
    
    /**
     *  Credential format for the None Membership service.
     *
     *  <p/>Credentials for the None Membership Service consist of the following
     *  unencrypted, unsigned XML tags:
     *
     *  <ul>
     *      <li>PeerGroupID</li>
     *      <li>PeerID</li>
     *      <li>Identity</li>
     *  </ul>
     **/
    private final static class NoneCredential implements Credential, CredentialPCLSupport {
        
        private NoneMembershipService source;
        
        private String whoami;
        
        private ID peerid;
        
        /**
         *  Whether the credential is valid.
         **/
        boolean valid = true;
        
        /**
         *  property change support
         **/
        private PropertyChangeSupport support = new PropertyChangeSupport(this);
        
        protected NoneCredential(NoneMembershipService source, String whoami) {
            
            this.source = source;
            this.whoami = whoami;
            this.peerid = source.peergroup.getPeerID();
        }
        
        protected NoneCredential(NoneMembershipService source, Element root) throws PeerGroupException {
            
            this.source = source;
            
            initialize(root);
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
         *
         **/
        private void setPeerID(PeerID peerid) {
            this.peerid = peerid;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>NoneCredential are always valid.
         **/
        public boolean isExpired() {
            return false;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>NoneCredential are always valid.
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
         *
         **/
        private void setSubject(String subject) {
            whoami = subject;
        }
        
        /**
         * {@inheritDoc}
         **/
        public Service getSourceService() {
            return source.getInterface();
        }
        
        /**
         * {@inheritDoc}
         **/
        public StructuredDocument getDocument(MimeMediaType as) throws Exception {
            StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(as, "jxta:Cred");
            
            if (doc instanceof Attributable) {
                ((Attributable) doc).addAttribute("xmlns:jxta", "http://jxta.org");
                ((Attributable) doc).addAttribute("xml:space", "preserve");
                ((Attributable) doc).addAttribute("type", "jxta:NullCred");
            }
            
            Element e = doc.createElement("PeerGroupID", getPeerGroupID().toString());

            doc.appendChild(e);
            
            e = doc.createElement("PeerID", peerid.toString());
            doc.appendChild(e);
            
            e = doc.createElement("Identity", whoami);
            doc.appendChild(e);
            
            return doc;
        }
        
        /**
         *  Process an individual element from the document.
         *
         *  @param elem the element to be processed.
         *  @return true if the element was recognized, otherwise false.
         **/
        protected boolean handleElement(TextElement elem) {
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
            
            // element was not handled
            return false;
        }
        
        /**
         *  Intialize from a portion of a structured document.
         **/
        protected void initialize(Element root) {
            
            if (!TextElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports TextElement");
            }
            
            TextElement doc = (TextElement) root;
            
            String typedoctype = "";

            if (root instanceof Attributable) {
                Attribute itsType = ((Attributable) root).getAttribute("type");

                if (null != itsType) {
                    typedoctype = itsType.getValue();
                }
            }
            
            String doctype = doc.getName();
            
            if (!doctype.equals("jxta:NullCred") && !typedoctype.equals("jxta:NullCred")) {
                throw new IllegalArgumentException(
                        "Could not construct : " + getClass().getName() + "from doc containing a " + doctype);
            }
            
            Enumeration elements = doc.getChildren();
            
            while (elements.hasMoreElements()) {
                TextElement elem = (TextElement) elements.nextElement();

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
            
            // FIXME bondolo@jxta.org 20030409 should check for duplicate elements and for peergroup element
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
    }
    

    /**
     *  Authenticator Class for the None Membership Service. Pre-filled in and
     *  ready for <code>join()</code>.
     **/
    public final static class NoneAuthenticator implements Authenticator {
        
        MembershipService source;
        AuthenticationCredential application;
        
        String whoami = "nobody";
        
        /**
         * Creates an authenticator for the null membership service. Anything entered
         * into the identity info section of the Authentication credential is
         * ignored.
         *
         *  @param source The instance of the null membership service which
         *  created this authenticator.
         *  @param application Anything entered into the identity info section of
         *  the Authentication credential is ignored.
         **/
        NoneAuthenticator(NoneMembershipService source, AuthenticationCredential application) {
            this.source = source;
            this.application = application;
        }
        
        /**
         * Returns the service which generated this authenticator.
         **/
        public MembershipService getSourceService() {
            return source;
        }
        
        /**
         * {@inheritDoc}
         *
         *  <p/>This implementation is <strong>always</strong> ready for
         *  <code>join()</code>
         **/
        synchronized public boolean isReadyForJoin() {
            // always ready.
            return true;
        }
        
        /**
         * {@inheritDoc}
         **/
        public String getMethodName() {
            return "NullAuthentication";
        }
        
        /**
         * {@inheritDoc}
         **/
        public AuthenticationCredential getAuthenticationCredential() {
            return application;
        }
        
        public void setAuth1Identity(String who) {
            if (null == who) {
                throw new IllegalArgumentException("You must supply an identity");
            }
            whoami = who;
        }
        
        public String getAuth1Identity() {
            return whoami;
        }
    }
    
    private ModuleImplAdvertisement implAdvertisement = null;
    
    /**
     *  The peergroup we live in.
     **/
    private PeerGroup peergroup = null;
    
    /**
     *  our current credentials
     **/
    private List principals;
    
    /**
     *  our current auth credentials
     **/
    private List principalsAuth;
    
    /**
     *  the default "nobody" credential
     **/
    private NoneCredential  defaultCredential = null;
    
    /**
     *  property change support
     **/
    private PropertyChangeSupport support;
    
    /**
     *  default constructor. Normally called only by the peer group.
     **/
    public NoneMembershipService() throws PeerGroupException {
        principals = new ArrayList();
        principalsAuth = new ArrayList();
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
        
        implAdvertisement = (ModuleImplAdvertisement) impl;
        
        peergroup = group;
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring None Membership Service : " + assignedID);

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
        
        defaultCredential = new NoneCredential(this, "nobody");
        
        resign();
    }
    
    /**
     * {@inheritDoc}
     **/
    public Service getInterface() {
        return this; // we have no method access control
    }
    
    /**
     * {@inheritDoc}
     **/
    public int startApp(String[] arg) {
        return 0;
    }
    
    /**
     * {@inheritDoc}
     **/
    public void stopApp() {
        resign();
        
        peergroup = null;
    }
    
    /**
     * {@inheritDoc}
     **/
    public Advertisement getImplAdvertisement() {
        return implAdvertisement;
    }
    
    /**
     * {@inheritDoc}
     **/
    public Authenticator apply(AuthenticationCredential application) throws PeerGroupException, ProtocolNotSupportedException {
        
        String method = application.getMethod();
        
        if ((null != method) && !"StringAuthentication".equals(method) && !"NoneAuthentication".equals(method)) {
            throw new ProtocolNotSupportedException("Authentication method not recognized");
        }
        
        return new NoneAuthenticator(this, application);
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
    public synchronized Enumeration<Credential> getCurrentCredentials() {
        return Collections.enumeration(principals);
    }
    
    /**
     * {@inheritDoc}
     **/
    public synchronized Enumeration<AuthenticationCredential> getAuthCredentials() {
        return Collections.enumeration(principalsAuth);
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential join(Authenticator authenticated) throws PeerGroupException {
        
        if (!(authenticated instanceof NoneAuthenticator)) {
            throw new ClassCastException("This is not my authenticator!");
        }
        
        if (!authenticated.isReadyForJoin()) {
            throw new PeerGroupException("Not ready to join()!");
        }
        
        NoneAuthenticator myAuthenticated = (NoneAuthenticator) authenticated;
        
        Credential newCred;

        synchronized (this) {
            newCred = new NoneCredential(this, myAuthenticated.getAuth1Identity());
            
            principals.add(newCred);
            principalsAuth.add(myAuthenticated.application);
        }
        
        support.firePropertyChange("addCredential", null, newCred);
        
        return newCred;
    }
    
    /**
     * {@inheritDoc}
     **/
    public void resign() {
        List allCreds = new ArrayList();

        allCreds.addAll(principals);
        allCreds.remove(defaultCredential);
        
        synchronized (this) {
            // remove all existing credentials
            principals.clear();
            principalsAuth.clear();
            
            // re-add the default credential.
            principals.add(defaultCredential);
        }
        
        Iterator eachCred = allCreds.iterator();
        
        while (eachCred.hasNext()) {
            NoneCredential aCred = (NoneCredential) eachCred.next();
            
            aCred.setValid(false);
        }
    }
    
    /**
     * {@inheritDoc}
     **/
    public Credential makeCredential(Element element) throws PeerGroupException, Exception {
        return new NoneCredential(this, element);
    }
}
