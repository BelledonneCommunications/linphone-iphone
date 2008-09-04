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


import java.net.URI;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.UUID;

import java.net.URISyntaxException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.document.Attribute;
import net.jxta.document.Attributable;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peer.PeerID;
import net.jxta.protocol.PeerAdvertisement;


/**
 * A Leasing Protocol Request Message.
 *
 * <p/><pre><code>
 *  &lt;xs:complexType name="LeaseRequestMessage">
 *      &lt;xs:sequence>
 *          &lt;xs:element name="Credential" type="jxta:Cred" />
 *          &lt;xs:element name="ClientAdv" minOccurs="0" >
 *              &lt;xs:complexType>
 *                  &lt;xs:complexContent>
 *                      &lt;xs:extension  base="jxta:PA">
 *                          &lt;xs:attribute name="expiration" use="required" type="xs:unsignedLong" />
 *                      &lt;/xs:extension>
 *                  &lt;/xs:complexContent>
 *              &lt;/xs:complexType>
 *          &lt;/xs:element>
 *          &lt;xs:element name="Option" minOccurs="0" maxOccurs="unbounded" type="xs:anyType" />
 *      &lt;/xs:sequence>
 *      &lt;xs:attribute name="client_id" use="required" type="jxta:JXTAID" />
 *      &lt;xs:attribute name="requested_lease" type="xs:unsignedLong" />
 *      &lt;xs:attribute name="server_adv_gen" type="jxta:uuid" />
 *      &lt;xs:attribute name="referral_advs" type="xs:unsignedInt" />
 *  &lt;/xs:complexType>
 * </code></pre>
 *
 * @since 2.5
 */
public class LeaseRequestMsg {
    
    /**
     *  Log4J Logger
     */
    private final static transient Logger LOG = Logger.getLogger(LeaseRequestMsg.class.getName());
    
    private final static String LEASE_REQUEST_MSG = "LeaseRequestMessage";
    private final static String CLIENT_ID_ATTR = "client_id";
    private final static String REQUESTED_LEASE_ATTR = "requested_lease";
    private final static String SERVER_ADV_GEN_ATTR = "server_adv_gen";
    private final static String REFERRAL_ADVS_ATTR = "referral_advs";
    
    private final static String CLIENT_CRED_TAG = "Credential";
    private final static String CLIENT_ADV_TAG = "ClientAdv";
    private final static String CLIENT_ADV_EXP_ATTR = "ClientAdv";
    
    private final static String OPTION_TAG = "Options";
    
    /**
     *  The ID of the client.
     */
    private ID clientID = null;
    
    /**
     *  Length of lease to request. {@code Long.MIN_VALUE} means that no lease
     *  is being requested, instead the message is being sent to retrieve
     *  referrals.
     */
    private long requestedLease = Long.MIN_VALUE;
    
    /**
     *  last UUID version of the server's advertisement that the client saw or
     *  {@code null} if client claims to have seen no pervious version.
     */
    private UUID serverAdvGen = null;
    
    /**
     *  The number of referral advertisements being requested.
     */
    private int referralAdvs = Integer.MIN_VALUE;
    
    /**
     *  The credential of the client.
     */
    private XMLElement credential = null;
    
    /**
     *  The optional peer advertisement of the client.
     */
    private PeerAdvertisement clientAdv = null;
    
    /**
     *  Expiration value for client peer advertisement. {@code Long.MIN_VALUE} 
     *  means that no value has been specified.
     */
    private long clientAdvExp = Integer.MIN_VALUE;
    
    /**
     *  Options
     */
    private List options = new ArrayList();
    
    /**
     *  New LeaseRequestMsg
     */
    public LeaseRequestMsg() {}
    
    /**
     * Construct from a XLMElement
     **/
    public LeaseRequestMsg(Element root) {
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
        
        if (!doc.getName().equals(getMessageType())) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + " from doc containing a '" + doc.getName()
                    + "'. Should be : " + getMessageType());
        }
        
        Enumeration eachAttr = doc.getAttributes();
        
        while (eachAttr.hasMoreElements()) {
            Attribute aLeaseReqAttr = (Attribute) eachAttr.nextElement();
            
            if (REQUESTED_LEASE_ATTR.equals(aLeaseReqAttr.getName())) {
                requestedLease = Long.valueOf(aLeaseReqAttr.getValue());
            } else if (SERVER_ADV_GEN_ATTR.equals(aLeaseReqAttr.getName())) {
                serverAdvGen = UUID.fromString(aLeaseReqAttr.getValue());
            } else if (CLIENT_ID_ATTR.equals(aLeaseReqAttr.getName())) {
                try {
                    URI srcURI = new URI(aLeaseReqAttr.getValue());
                    ID srcID = IDFactory.fromURI(srcURI);
                    
                    setClientID(srcID);
                } catch (URISyntaxException badID) {
                    IllegalArgumentException iae = new IllegalArgumentException("Bad ID in message");
                    
                    iae.initCause(badID);
                    throw iae;
                }
            } else if ("type".equals(aLeaseReqAttr.getName())) {
                ;
            } else if ("xmlns:jxta".equals(aLeaseReqAttr.getName())) {
                ;
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Attribute: " + aLeaseReqAttr.getName());
                }
            }
        }
        
        Enumeration elements = doc.getChildren();
        
        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();
            
            if (!handleElement(elem)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Element: " + elem.toString());
                }
            }
        }
        
        // Sanity Check!!!
        
        if (null == getClientID()) {
            throw new IllegalArgumentException("Missing client ID value.");
        }
        
        if ((getRequestedLease() < 0) && (getRequestedLease() != Long.MIN_VALUE)) {
            throw new IllegalArgumentException("Invalid requested lease duration.");
        }
        
        if ((getReferralAdvs() < 0) && (getReferralAdvs() != Integer.MIN_VALUE)) {
            throw new IllegalArgumentException("Invalid referral advertisements request value.");
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Object clone() throws CloneNotSupportedException {
        LeaseRequestMsg clone = (LeaseRequestMsg) super.clone();

        clone.setClientID(getClientID());
        clone.setServerAdvGen(getServerAdvGen());
        clone.setRequestedLease(getRequestedLease());
        clone.setReferralAdvs(getReferralAdvs());
        clone.setCredential(getCredential());
        return clone;
    }
    
    /**
     *  Returns the ID of the client making this request.
     *
     *  @return ID of the client.
     */
    public ID getClientID() {
        return clientID;
    }
    
    /**
     *  Sets the ID of the client making this request.
     *
     *  @param clientID ID of the client.
     */
    public void setClientID(ID clientID) {
        this.clientID = clientID;
    }
    
    /**
     *  Returns the advertisement generation of the server advertisement which
     *  is known to the client. May be {@code null} to indicate that the client
     *  does not have a previous version of the server advertisement.
     *
     *  @return The server advertisement generation.
     */
    public UUID getServerAdvGen() {
        return serverAdvGen;
    }
    
    /**
     *  Sets the advertisement generation of the server advertisement which is
     *  known to the client. May be {@code null} to indicate that the client
     *  does not have a previous version of the server advertisement.
     *
     *  @param serverAdvGen The server advertisement generation.
     */
    public void setServerAdvGen(UUID serverAdvGen) {
        this.serverAdvGen = serverAdvGen;
    }
    
    /**
     *  Return the duration of the lease being requested. The duration must be
     *  a positive integer or {@code Long.MIN_VALUE} which indicates that no
     *  lease is being requested.
     *
     *  @return The duration of the lease being requested.
     */
    public long getRequestedLease() {
        return requestedLease;
    }
    
    /**
     *  Set the duration of the lease being requested. The duration must be
     *  a positive integer or {@code Long.MIN_VALUE} which indicates that no
     *  lease is being requested.
     *
     *  @param requestedLease The duration of the lease being requested.
     */
    public void setRequestedLease(long requestedLease) {
        this.requestedLease = requestedLease;
    }
    
    /**
     *  Returns the number of referral advertisements requested by the client.
     *  Must be a positive integer or {@code Integer.MIN_VALUE} which indicates
     *  that the default number is requested.
     * @return the number of referral advertisements requested by the client.
     */
    public int getReferralAdvs() {
        return referralAdvs;
    }
    
    /**
     *  Sets the number of referral advertisements requested by the client.
     *  Must be a positive integer or {@code Integer.MIN_VALUE} which indicates
     *  that the default number is requested.
     *
     *  @param referralAdvs The number of referral advertisements requested.
     */
    public void setReferralAdvs(int referralAdvs) {
        this.referralAdvs = referralAdvs;
    }
    
    /**
     *  Returns the credential of the client making this request in XML format.
     *
     *  @return The credential associated with this request if any. May be
     *  {@code null} to indicate that no credential was provided.
     */
    public XMLElement getCredential() {
        return (XMLElement) ((null != credential) ? StructuredDocumentUtils.copyAsDocument(credential) : null);
    }
    
    /**
     *  Sets the credential of the client making this request in XML format.
     *
     *  @param newCred The credential associated with this request if any. May
     *  be {@code null} to indicate that no credential is being provided.
     */
    public void setCredential(XMLElement newCred) {
        this.credential = (XMLElement) ((null != newCred) ? StructuredDocumentUtils.copyAsDocument(newCred) : null);
    }
    
    /**
     *  Our DOCTYPE
     *
     *  @return the type of this message.
     **/
    public static String getMessageType() {
        return "jxta:LeaseRequestMsg";
    }
    
    protected boolean handleElement(XMLElement elem) {
        
        if (CLIENT_CRED_TAG.equals(elem.getName())) {
            credential = (XMLElement) StructuredDocumentUtils.copyAsDocument(elem);
            
            return true;
        }
        
        String value = elem.getTextValue();
        
        if (null != value) {
            value = value.trim();
            
            if (0 == value.length()) {
                value = null;
            }
        }
        
        if (null == value) {
            return false;
        }
        
        return false;
    }
    
    /**
     *  {@inheritDoc}
     **/
    public Document getDocument(MimeMediaType mediaType) {
        
        if (null == getClientID()) {
            throw new IllegalStateException("Missing client ID value.");
        }
        
        if ((getRequestedLease() < 0) && (getRequestedLease() != Long.MIN_VALUE)) {
            throw new IllegalStateException("Invalid requested lease duration.");
        }
        
        if ((getReferralAdvs() < 0) && (getReferralAdvs() != Integer.MIN_VALUE)) {
            throw new IllegalStateException("Invalid referral advertisements request value.");
        }
        
        StructuredDocument msg = StructuredDocumentFactory.newStructuredDocument(mediaType, getMessageType());
        
        if (!(msg instanceof Attributable)) {
            throw new UnsupportedOperationException("Only 'Attributable' document types are supported.");
        }
        
        if (msg instanceof XMLDocument) {
            ((XMLDocument) msg).addAttribute("xmlns:jxta", "http://jxta.org");
        }
        
        ((Attributable) msg).addAttribute(CLIENT_ID_ATTR, getClientID().toString());
        
        if (Long.MIN_VALUE != getRequestedLease()) {
            ((Attributable) msg).addAttribute(REQUESTED_LEASE_ATTR, Long.toString(getRequestedLease()));
        }
        
        if (null != getServerAdvGen()) {
            ((Attributable) msg).addAttribute(SERVER_ADV_GEN_ATTR, getServerAdvGen().toString());
        }
        
        if (null != credential) {
            StructuredDocumentUtils.copyElements(msg, msg, credential, CLIENT_CRED_TAG);
        }
        
        return msg;
    }
}
