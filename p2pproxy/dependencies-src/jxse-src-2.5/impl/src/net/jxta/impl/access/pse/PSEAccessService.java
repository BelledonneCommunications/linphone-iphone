/*
 * Copyright (c) 2003-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.access.pse;


import java.net.URI;
import java.security.cert.CertPath;
import java.security.cert.CertPathValidator;
import java.security.cert.TrustAnchor;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

import java.net.URISyntaxException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.access.AccessService;
import net.jxta.credential.Credential;
import net.jxta.credential.PrivilegedOperation;
import net.jxta.document.Advertisement;
import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.TextElement;
import net.jxta.exception.PeerGroupException;
import net.jxta.exception.JxtaError;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.membership.MembershipService;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.ModuleSpecID;
import net.jxta.platform.Module;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.service.Service;

import net.jxta.impl.membership.pse.PSECredential;
import net.jxta.impl.membership.pse.PSEMembershipService;


/**
 * Implements the {@link net.jxta.access.AccessService} using PKIX validation.
 *
 * FIXME 20060409 bondolo THIS IS AN EARLY DEVELOPMENT RELEASE FOR INVESTIGATING 
 * THE API. IT IS *NOT* SECURE! DO NOT SHIP THIS CODE (IN IT'S CURRENT FORM) IN 
 * A REAL PRODUCT.
 *
 * @see net.jxta.access.AccessService
 * @see net.jxta.impl.membership.pse.PSEMembershipService
 */
public class PSEAccessService implements AccessService {
    
    /**
     *  Logger.
     */
    private final static Logger LOG = Logger.getLogger(PSEAccessService.class.getName());
    
    /**
     * Well known access specification identifier: the pse access service
     */
    public final static ModuleSpecID PSE_ACCESS_SPEC_ID = (ModuleSpecID)
            ID.create(URI.create("urn:jxta:uuid-DeadBeefDeafBabaFeedBabe000000100306"));
    
    /**
     *  Operation for the PSE Access Service.
     */
    private static class PSEOperation implements PrivilegedOperation {
        
        final PSEAccessService source;
        
        PSECredential op;
        
        protected PSEOperation(PSEAccessService source, PSECredential op) {
            this.source = source;
            this.op = op;
        }
        
        protected PSEOperation(PSEAccessService source, Element root) {
            this.source = source;
            initialize(root);
        }
        
        /**
         * {@inheritDoc}
         */
        public ID getPeerGroupID() {
            return source.getPeerGroup().getPeerGroupID();
        }
        
        /**
         * {@inheritDoc}
         */
        public ID getPeerID() {
            return null;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>AlwaysOperation are always valid.
         */
        public boolean isExpired() {
            return false;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>AlwaysOperation are always valid.
         */
        public boolean isValid() {
            return true;
        }
        
        /**
         * {@inheritDoc}
         */
        public PSECredential getSubject() {
            return op;
        }
        
        /**
         * {@inheritDoc}
         */
        public Service getSourceService() {
            return source;
        }
        
        /**
         * {@inheritDoc}
         *
         *  FIXME 20060317 bondolo This implementation is not secure. The
         *  operation should be signed by the offerer.
         */
        public StructuredDocument getDocument(MimeMediaType as) throws Exception {
            StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(as, "jxta:Cred");
            
            if (doc instanceof Attributable) {
                ((Attributable) doc).addAttribute("xmlns:jxta", "http://jxta.org");
                ((Attributable) doc).addAttribute("xml:space", "preserve");
                ((Attributable) doc).addAttribute("type", "jxta:PSEOp");
            }
            
            Element e = doc.createElement("PeerGroupID", getPeerGroupID().toString());

            doc.appendChild(e);
            
            e = doc.createElement("Operation", op);
            doc.appendChild(e);
                       
            return doc;
        }
        
        /**
         * {@inheritDoc}
         */
        public PSECredential getOfferer() {
            return null;
        }
        
        /**
         *  Process an individual element from the document.
         *
         *  @param elem the element to be processed.
         *  @return true if the element was recognized, otherwise false.
         */
        protected boolean handleElement(TextElement elem) {
            if (elem.getName().equals("PeerGroupID")) {
                try {
                    URI gID = new URI(elem.getTextValue().trim());
                    ID pgid = IDFactory.fromURI(gID);

                    if (!pgid.equals(getPeerGroupID())) {
                        throw new IllegalArgumentException(
                                "Operation is from a different group. " + pgid + " != " + getPeerGroupID());
                    }
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("Unusable ID in advertisement: " + elem.getTextValue());
                } catch (ClassCastException badID) {
                    throw new IllegalArgumentException("Id is not a group id: " + elem.getTextValue());
                }
                return true;
            }
            
            if (elem.getName().equals("Operation")) {
                op = (PSECredential) source.pseMembership.makeCredential(elem);
                
                return true;
            }
                       
            // element was not handled
            return false;
        }
        
        /**
         *  Intialize from a portion of a structured document.
         */
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
            
            if (!doctype.equals("jxta:PSEOp") && !typedoctype.equals("jxta:PSEOp")) {
                throw new IllegalArgumentException(
                        "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
            }
            
            Enumeration elements = doc.getChildren();
            
            while (elements.hasMoreElements()) {
                TextElement elem = (TextElement) elements.nextElement();

                if (!handleElement(elem)) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled element \'" + elem.getName() + "\' in " + doc.getName());
                    }
                }
            }
            
            // sanity check time!
            
            if (null == op) {
                throw new IllegalArgumentException("operation was never initialized.");
            }
        }
    }
    
    /**
     *  The Peer Group we are working for.
     */
    PeerGroup group;
    
    /**
     *  Implementation advertisement for this instance.
     */
    ModuleImplAdvertisement implAdvertisement;
    
    /**
     *  The PSE Membership service we are paired with.
     */
    PSEMembershipService pseMembership;
    
    /**
     *  If {@code true} then a null credential will be allowed for the null op.
     */
    final boolean allowNullCredentialForNullOperation = false;
    
    /**
     *  The default constructor
     */
    public PSEAccessService() {}
    
    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement implAdv) throws PeerGroupException {
        this.group = group;
        implAdvertisement = (ModuleImplAdvertisement) implAdv;
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring PSE Access Service : " + assignedID);
            
            configInfo.append("\n\tImplementation :");
            configInfo.append("\n\t\tModule Spec ID: " + implAdvertisement.getModuleSpecID());
            configInfo.append("\n\t\tImpl Description : " + implAdvertisement.getDescription());
            configInfo.append("\n\t\tImpl URI : " + implAdvertisement.getUri());
            configInfo.append("\n\t\tImpl Code : " + implAdvertisement.getCode());
            
            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tGroup : " + group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID : " + group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID : " + group.getPeerID());
            
            LOG.config(configInfo.toString());
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public int startApp(String[] args) {
        MembershipService membership = group.getMembershipService();
        
        if (null == membership) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a membership service");
            }
            
            return Module.START_AGAIN_STALLED;
        }
        
        ModuleImplAdvertisement membershipImplAdv = (ModuleImplAdvertisement) membership.getImplAdvertisement();
        
        if ((null != membershipImplAdv) && PSEMembershipService.pseMembershipSpecID.equals(membershipImplAdv.getModuleSpecID())
                && (membership instanceof PSEMembershipService)) {
            pseMembership = (PSEMembershipService) membership;
        } else {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("PSE Access Service requires a PSE Membership Service.");
            }
            
            return -1;
        }
        
        return 0;
    }
    
    /**
     * {@inheritDoc}
     */
    public void stopApp() {
        pseMembership = null;
    }
    
    /**
     * {@inheritDoc}
     */
    public ModuleImplAdvertisement getImplAdvertisement() {
        return implAdvertisement;
    }
    
    /**
     * {@inheritDoc}
     */
    public Service getInterface() {
        return this;
    }
    
    /**
     * {@inheritDoc}
     */
    public AccessResult doAccessCheck(PrivilegedOperation op, Credential cred) {
        if ((null == op) && (null == cred)) {
            return allowNullCredentialForNullOperation ? AccessResult.PERMITTED : AccessResult.DISALLOWED;
        }
        
        if ((null == cred) || !(cred instanceof PSECredential)) {
            return AccessResult.DISALLOWED;
        }
        
        if (!cred.isValid()) {
            return AccessResult.DISALLOWED;
        }
        
        if (null == op) {
            return AccessResult.PERMITTED;
        }
        
        if (!(op instanceof PSEOperation)) {
            return AccessResult.DISALLOWED;
        }
        
        if (op.getSourceService() != this) {
            return AccessResult.DISALLOWED;
        }
        
        if (!op.isValid()) {
            return AccessResult.DISALLOWED;
        }
        
        PSECredential offerer = ((PSEOperation) op).getOfferer();
        
        X509Certificate opCerts[] = offerer.getCertificateChain();
        
        X509Certificate credCerts[] = ((PSECredential) cred).getCertificateChain();
        
        // FIXME 20060409 bondolo THIS IS NOT A VALID TEST. It is a shortcut for
        // PKIX validation and assumes that all presented certificates chains 
        // are valid and trustworthy. IT IS NOT SECURE. (It does not ensure that
        // certficiates are really signed by their claimed issuer.)
        for (X509Certificate credCert : Arrays.asList(credCerts)) {
            for (X509Certificate opCert : Arrays.asList(opCerts)) {
                if (credCert.getPublicKey().equals(opCert.getPublicKey())) {
                    return AccessResult.PERMITTED;
                }
            }
        }
        
        return AccessResult.DISALLOWED;
    }
    
    /**
     * {@inheritDoc}
     */
    public PrivilegedOperation newPrivilegedOperation(Object subject, Credential offerer) {
        if (!(subject instanceof PSECredential)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports PSECredential subjects.");
        }
        
        if (subject != offerer) {
            throw new IllegalArgumentException("PSE Access Service requires operation and offerer to be the same object.");
        }
        
        if (!offerer.isValid()) {
            throw new IllegalArgumentException("offerer is not a valid credential");
        }
        
        return new PSEOperation((PSEAccessService) getInterface(), (PSECredential) offerer);
    }
    
    /**
     * {@inheritDoc}
     */
    public PrivilegedOperation newPrivilegedOperation(Element source) {
        return new PSEOperation((PSEAccessService) getInterface(), source);
    }
    
    /**
     * {@inheritDoc}
     */
    PeerGroup getPeerGroup() {
        return group;
    }
}
