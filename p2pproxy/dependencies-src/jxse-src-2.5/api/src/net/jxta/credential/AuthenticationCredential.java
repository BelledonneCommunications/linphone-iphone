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

package net.jxta.credential;

import net.jxta.document.*;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peergroup.PeerGroup;
import net.jxta.service.Service;

import java.util.logging.Level;

import net.jxta.logging.Logging;

import java.util.logging.Logger;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;

/**
 * Authenication credentials are used by JXTA Membership Services as the
 * basis for applications for peergroup membership. The AuthenticationCredential
 * provides two important pieces of inforamtion:
 * <ul>
 * <li>the authetication method being requested</li>
 * <li>identity information which will be provided to that authentication
 * method.</li>
 * <ul>
 * <p/>
 * Not all authentication methods use the identity nformation.
 *
 * @see net.jxta.credential.Credential
 * @see net.jxta.membership.MembershipService
 * @see net.jxta.membership.Authenticator
 */
public final class AuthenticationCredential implements Credential {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(AuthenticationCredential.class.getName());

    /**
     * the authentication method which will be requested when this credential is
     * provided during "apply" to a peergroup membership service.
     */
    private String authenticationMethod = null;

    /**
     * Any optional information which is required by the requested authentication method.
     */
    private Element identityInfo = null;

    /**
     * The peergroup of this AuthenticationCredential
     */
    private final PeerGroup peergroup;

    /**
     * Creates new AuthenticationCredential
     *
     * @param peergroup     The peergroup context in which this
     *                      AuthenticationCredential is created.
     * @param method        The authentication method which will be requested when the
     *                      AuthentiationCredential is provided to the peergroup Membership Service.
     *                      Authentication methods are specific to Membership services. Consult the
     *                      Documentation for the Membership service you will be authenticating
     *                      against in order to determine the valid <tt>method</tt> values. Every
     *                      Membership service should support a default authentication method which
     *                      can be specified as <tt>null</null>.
     * @param indentityInfo Optional additional information which is used by the
     *                      authentication method or <tt>null</tt>. This information is passed to the
     *                      authentication method during the apply operation of the Membership
     *                      Service. Consult the documentation for the specific Membership Service
     *                      you are using for details on how this information is used (if at all).
     */
    public AuthenticationCredential(PeerGroup peergroup, String method, Element indentityInfo) {
        this.peergroup = peergroup;

        authenticationMethod = method;

        if (null != indentityInfo) {
            this.identityInfo = StructuredDocumentUtils.copyAsDocument(indentityInfo);
        }
    }

    /**
     * Creates new AuthenticationCredential
     *
     * @param peergroup The peergroup context in which this
     *                  AuthenticationCredential is created.
     * @param root      the document containing the serialized representation of the
     *                  AuthenticationCredential.
     */
    public AuthenticationCredential(PeerGroup peergroup, Element root) {

        this.peergroup = peergroup;

        initialize(root);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * AuthenticationCredentials are created in the context of a PeerGroup
     * though they are generally independant of peergroups. The intent is that
     * the AuthenticationCredential will be passed to the MembershipService
     * service of the same peergroup as the AuthenticationCredenitals.
     *
     * @return PeerGroupID associated with this AuthenticationCredential.
     */
    public ID getPeerGroupID() {
        return peergroup.getPeerGroupID();
    }

    /**
     * {@inheritDoc}
     */
    public ID getPeerID() {
        return peergroup.getPeerID();
    }

    /**
     * {@inheritDoc}
     */
    public Service getSourceService() {
        return null;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * AuthenticationCredential are never expired. The Authenticator will
     * determine the true validity from the included identity info.
     */
    public boolean isExpired() {
        return false;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * AuthenticationCredential are always valid, the Authenticator will
     * determine the true validity from the included identity info.
     */
    public boolean isValid() {
        return true;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * There is no straightforward mechansim for identifying the subject
     * unfortunately.
     */
    public Object getSubject() {
        return null;
    }

    /**
     * Write credential into a document. as is a mime media-type
     * specification and provides the form of the document which is being
     * requested. Two standard document forms are defined. "text/text" encodes
     * the document in a form nice for printing out and "text/xml" which
     * provides an XML format.
     *
     * @param as The mime media type of the encoding format being requested.
     * @return the StructuredDocument which represents this credential.
     * @throws Exception When errors occur.
     */
    public StructuredDocument getDocument(MimeMediaType as) throws Exception {

        StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(as, "jxta:Cred");

        if (doc instanceof Attributable) {
            ((Attributable) doc).addAttribute("xmlns:jxta", "http://jxta.org");
            ((Attributable) doc).addAttribute("xml:space", "preserve");
            ((Attributable) doc).addAttribute("type", "AuthenticationCredential");
        }

        Element e = doc.createElement("Method", getMethod());

        doc.appendChild(e);

        e = doc.createElement("PeerGroupID", getPeerGroupID().toString());
        doc.appendChild(e);

        e = doc.createElement("PeerID", getPeerID().toString());
        doc.appendChild(e);

        if (null != identityInfo) {
            e = doc.createElement("IdentityInfo");
            doc.appendChild(e);

            StructuredDocumentUtils.copyElements(doc, e, identityInfo);
        }

        return doc;
    }

    /**
     * Returns the authentication method which this AuthenticationCredential
     * will be requesting when it is provided to a Membership Service during the
     * "Apply" operation.
     *
     * @return String containing the authentication method being requested.
     */
    public String getMethod() {
        return authenticationMethod;
    }

    protected void setMethod(String authenticationMethod) {
        this.authenticationMethod = authenticationMethod;
    }

    /**
     * Returns the StructuredDocument Element containing the identity information which was
     * originally provided when this AuthenticationCredential was created.
     *
     * @return StructuredDocument Element containing the identity information which was
     *         originally provided when this AuthenticationCredential was created.
     */
    public Element getIdentityInfo() {
        return (null == identityInfo) ? null : StructuredDocumentUtils.copyAsDocument(identityInfo);
    }

    /**
     * Process an individual element from the document.
     *
     * @param elem the element to be processed.
     * @return true if the element was recognized, otherwise false.
     */
    protected boolean handleElement(TextElement elem) {
        if (elem.getName().equals("PeerGroupID")) {
            try {
                URI gID = new URI(elem.getTextValue());
                ID pgid = IDFactory.fromURI(gID);

                if (!pgid.equals(getPeerGroupID())) {
                    throw new IllegalArgumentException("Operation is from a different group. " + pgid + " != " + getPeerGroupID());
                }
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Unusable ID in advertisement: " + elem.getTextValue());
            }
            return true;
        }

        if (elem.getName().equals("PeerID")) {
            try {
                URI pID = new URI(elem.getTextValue());
                ID pid = IDFactory.fromURI(pID);

                if (!pid.equals(getPeerID())) {
                    throw new IllegalArgumentException("Operation is from a different group. " + pid + " != " + getPeerID());
                }
            } catch (URISyntaxException badID) {
                throw new IllegalArgumentException("Bad Peer ID in advertisement: " + elem.getTextValue());
            } catch (ClassCastException badID) {
                throw new IllegalArgumentException("Id is not a peer id: " + elem.getTextValue());
            }
            return true;
        }

        if (elem.getName().equals("Method")) {
            setMethod(elem.getTextValue());
            return true;
        }

        if (elem.getName().equals("IdentityInfo")) {
            Enumeration firstChild = elem.getChildren();

            if (!firstChild.hasMoreElements()) {
                throw new IllegalArgumentException("Missing identity info");
            }

            identityInfo = StructuredDocumentUtils.copyAsDocument((Element) firstChild.nextElement());

            return true;
        }

        // element was not handled
        return false;
    }

    /**
     * Intialize from a portion of a structured document.
     *
     * @param root the element
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

        if (!"jxta:AuthenticationCredential".equals(doctype) && !"jxta:AuthenticationCredential".equals(typedoctype)) {
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

        // FIXME bondolo@jxta.org 20030409 check things
    }
}
