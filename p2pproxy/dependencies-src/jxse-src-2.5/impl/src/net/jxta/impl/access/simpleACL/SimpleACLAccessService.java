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

package net.jxta.impl.access.simpleACL;


import java.net.URI;
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
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.service.Service;


/**
 * Implements the {@link net.jxta.access.AccessService} using a simple ACL
 * scheme.
 *
 * <p/>The ACL table is read from the group advertisement. Each
 * <code>perm</code> entry of the Access Service parameters in the group adv is
 * assumed to be a permission in the following format:
 *
 * <p/><pre>
 *    &lt;operation> ":" ( &lt;identity> )* ( "," &lt;identity> )*
 * </pre>
 *
 * <p/>A sample ACL table extracted from a PeerGroupAdvertisement:
 *
 * <p/><pre>
 * ...
 * &lt;Svc>
 *   &lt;MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000001005&lt;/MCID>
 *   &lt;Parm>
 *     &lt;perm>&amp;lt;&amp;lt;DEFAULT>>:nobody,permit&lt;/perm>
 *     &lt;perm>everyone:&amp;lt;&amp;lt;ALL>>&lt;/perm>
 *     &lt;perm>permit:nobody,permit,allow&lt;/perm>
 *     &lt;perm>deny:notpermit,notallow&lt;/perm>
 *   &lt;/Parm>
 * &lt;/Svc>
 * ...
 * </pre>
 *
 * <p/>If <code>&lt;&lt;ALL>></code> is provided as an identity then the
 * operation is permitted for all valid credentials.
 *
 * <p/>if <code>&lt;&lt;DEFAULT>></code> is provided as an operation then the
 * provided identities will be allowed for all operations which are not
 * recognized.
 *
 * <p/><strong>This implementation makes <em>no effort</em> to ensure that the
 * permission table has not been altered. It is <em>not appropriate</em> for use
 * in security sensitive deployments unless the integrity of the group
 * advertisement is ensured.</strong>
 *
 * @see net.jxta.access.AccessService
 **/
public class SimpleACLAccessService implements AccessService {
    
    /**
     *  Logger.
     **/
    private final static Logger LOG = Logger.getLogger(SimpleACLAccessService.class.getName());
    
    /**
     * Well known access specification identifier: the simple ACL access service
     **/
    public static final ModuleSpecID simpleACLAccessSpecID = (ModuleSpecID)
            ID.create(URI.create("urn:jxta:uuid-DeadBeefDeafBabaFeedBabe000000100206"));
    
    /**
     *  Operation for the Always Access Service.
     **/
    private static class SimpleACLOperation implements PrivilegedOperation {
        
        SimpleACLAccessService source;
        
        String op;
        
        Credential offerer;
        
        protected SimpleACLOperation(SimpleACLAccessService source, String op, Credential offerer) {
            this.source = source;
            this.op = op;
            this.offerer = offerer;
        }
        
        protected SimpleACLOperation(SimpleACLAccessService source, Element root) {
            this.source = source;
            initialize(root);
        }
        
        /**
         * {@inheritDoc}
         **/
        public ID getPeerGroupID() {
            return source.getPeerGroup().getPeerGroupID();
        }
        
        /**
         * {@inheritDoc}
         **/
        public ID getPeerID() {
            return null;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>AlwaysOperation are always valid.
         **/
        public boolean isExpired() {
            return false;
        }
        
        /**
         * {@inheritDoc}
         *
         * <p/>AlwaysOperation are always valid.
         **/
        public boolean isValid() {
            return true;
        }
        
        /**
         * {@inheritDoc}
         **/
        public String getSubject() {
            return op;
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
            
            if (doc instanceof Attributable) {
                ((Attributable) doc).addAttribute("xmlns:jxta", "http://jxta.org");
                ((Attributable) doc).addAttribute("xml:space", "preserve");
                ((Attributable) doc).addAttribute("type", "jxta:SimpleACLOp");
            }
            
            Element e = doc.createElement("PeerGroupID", getPeerGroupID().toString());

            doc.appendChild(e);
            
            e = doc.createElement("Operation", op);
            doc.appendChild(e);
            
            StructuredDocumentUtils.copyElements(doc, doc, offerer.getDocument(as), "Offerer");
            
            return doc;
        }
        
        /**
         * {@inheritDoc}
         **/
        public Credential getOfferer() {
            return offerer;
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
                op = elem.getTextValue();
                return true;
            }
            
            if (elem.getName().equals("Offerer")) {
                try {
                    offerer = source.getPeerGroup().getMembershipService().makeCredential(elem);
                } catch (Throwable failed) {
                    throw new IllegalArgumentException("Offerer credential could not be constructed" + failed);
                }
                return true;
            }
            
            // element was not handled
            return false;
        }
        
        /**
         *  Initialize from a portion of a structured document.
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
            
            if (!doctype.equals("jxta:SimpleACLOp") && !typedoctype.equals("jxta:SimpleACLOp")) {
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
            
            if (null == offerer) {
                throw new IllegalArgumentException("offerer was never initialized.");
            }
        }
    }
    
    /**
     *  The peer group we are working for.
     **/
    PeerGroup group;
    
    /**
     *  Implementation advertisement for this instance.
     **/
    ModuleImplAdvertisement implAdvertisement;
    
    /**
     *  The ACLs we are supporting.
     **/
    private final Map<String, Set<String>> ACLs = new HashMap<String, Set<String>>();
    
    /**
     *  The default constructor
     **/
    public SimpleACLAccessService() {}      
    
    /**
     * {@inheritDoc}
     **/
    public void init(PeerGroup group, ID assignedID, Advertisement implAdv) throws PeerGroupException {
        this.group = group;
        implAdvertisement = (ModuleImplAdvertisement) implAdv;
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Access Service : " + assignedID);

            configInfo.append("\n\tImplementation:");
            configInfo.append("\n\t\tImpl Description: " + implAdvertisement.getDescription());
            configInfo.append("\n\t\tImpl URI : " + implAdvertisement.getUri());
            configInfo.append("\n\t\tImpl Code : " + implAdvertisement.getCode());
            configInfo.append("\n\tGroup Params:");
            configInfo.append("\n\t\tGroup: " + group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID: " + group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID: " + group.getPeerID());
            LOG.config(configInfo.toString());
        }
        
        PeerGroupAdvertisement configAdv = group.getPeerGroupAdvertisement();
        
        TextElement myParam = (TextElement) configAdv.getServiceParam(assignedID);
        
        if (null == myParam) {
            throw new PeerGroupException("parameters for group access controls missing.");
        }
        
        Enumeration allACLS = myParam.getChildren();
        
        while (allACLS.hasMoreElements()) {
            TextElement anACL = (TextElement) allACLS.nextElement();
            
            if (!anACL.getName().equals("perm")) {
                continue;
            }
            
            String etcPasswd = anACL.getTextValue();
            
            int nextDelim = etcPasswd.indexOf(':');

            if (-1 == nextDelim) {
                continue;
            }
            
            String operation = etcPasswd.substring(0, nextDelim).trim();
            
            if ("<<DEFAULT>>".equals(operation)) {
                operation = null;
            }
            
            String identities = etcPasswd.substring(nextDelim + 1);
            Set allowed = new HashSet();
            
            StringTokenizer eachIdentity = new StringTokenizer(identities, ",");
            
            while (eachIdentity.hasMoreTokens()) {
                String anIdentity = eachIdentity.nextToken().trim();
                
                if ("<<ALL>>".equals(anIdentity)) {
                    anIdentity = null;
                }
                
                allowed.add(anIdentity);
            }
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        "Adding operation  : \'" + ((null == operation) ? "<<DEFAULT>>" : operation) + "\' with " + allowed.size()
                        + " identities.");
            }
            
            ACLs.put(operation, allowed);
        }
    }
    
    /**
     * {@inheritDoc}
     **/
    public int startApp(String[] args) {
        return 0;
    }
    
    /**
     * {@inheritDoc}
     **/
    public void stopApp() {}
    
    /**
     * {@inheritDoc}
     **/
    public ModuleImplAdvertisement getImplAdvertisement() {
        return implAdvertisement;
    }
    
    /**
     * {@inheritDoc}
     **/
    public SimpleACLAccessService getInterface() {
        return this;
    }
    
    /**
     * {@inheritDoc}
     **/
    public AccessResult doAccessCheck(PrivilegedOperation op, Credential cred) {
        if ((null != cred) && !cred.isValid()) {
            return AccessResult.DISALLOWED;
        }
        
        if ((null != op) && !op.isValid()) {
            return AccessResult.DISALLOWED;
        }
        
        Set<String> allowed = ACLs.get((null != op) ? op.getSubject() : null);
        
        // do we know this operation?
        if (null == allowed) {
            // try the default permission
            allowed = ACLs.get(null);
            
            if (null == allowed) {
                return AccessResult.DISALLOWED;
            }
        }
        
        String credSubject = (null != cred) ? cred.getSubject().toString() : null;
        
        return (allowed.contains(credSubject) || allowed.contains(null)) ? AccessResult.PERMITTED : AccessResult.DISALLOWED;
    }
    
    /**
     * {@inheritDoc}
     **/
    public PrivilegedOperation newPrivilegedOperation(Object subject, Credential offerer) {
        if (!(subject instanceof String)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports String subjects.");
        }
        
        if (!offerer.isValid()) {
            throw new IllegalArgumentException("offerer is not a valid credential");
        }
        
        return new SimpleACLOperation(this, (String) subject, offerer);
    }
    
    /**
     * {@inheritDoc}
     **/
    public PrivilegedOperation newPrivilegedOperation(Element source) {
        return new SimpleACLOperation(this, source);
    }
    
    /**
     * {@inheritDoc}
     **/
    PeerGroup getPeerGroup() {
        return group;
    }
}
