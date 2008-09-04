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
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.UUID;

import java.net.URISyntaxException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.AdvertisementFactory;
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
import net.jxta.protocol.RdvAdvertisement;


/**
 * A Leasing Protocol Response Message.
 *
 * <p/><pre><code>
 * <xs:complexType name="LeaseResponseMessage">
 *   <xs:sequence>
 *     <xs:element name="credential" minOccurs="0" maxOccurs="1" type="jxta:Cred" />
 *     <xs:element name="serverAdv" minOccurs="0" maxOccurs="1">
 *       <xs:complexType>
 *         <xs:complexContent>
 *           <xs:extension  base="jxta:RdvAdvertisement">
 *             <xs:attribute name="serverAdvGen" use="required" type="xs:string" /> <!-- a UUID -->
 *             <xs:attribute name="expiration" use="required" type="xs:unsignedLong" />
 *           </xs:extension>
 *         </xs:complexContent>
 *       </xs:complexType>
 *     </xs:element>
 *     <xs:element name="referralAdv" minOccurs="0" maxOccurs="unbounded">
 *       <xs:complexType>
 *         <xs:complexContent>
 *           <xs:extension  base="jxta:RdvAdvertisement">
 *             <xs:attribute name="expiration" use="required" type="xs:unsignedLong" />
 *           </xs:extension>
 *         </xs:complexContent>
 *       </xs:complexType>
 *     </xs:element>
 *   </xs:sequence>
 *   <xs:attribute name="serverID" use="required" type="jxta:JXTAID" />
 *   <xs:attribute name="offeredLease" type="xs:unsignedLong" />
 * </xs:complexType>
 * </code></pre>
 *
 * @since JXTA 2.4
 */
public class LeaseResponseMsg {
    
    /**
     *  Log4J Logger
     **/
    private final static transient Logger LOG = Logger.getLogger(LeaseResponseMsg.class.getName());
    
    private final static String OFFERED_LEASE_ATTR = "offeredLease";
    
    private final static String SERVER_ID_ATTR = "serverID";
    
    private final static String SERVER_ADV_TAG = "serverAdv";
    private final static String ADV_GEN_ATTR = "advGen";
    private final static String ADV_EXP_ATTR = "expiration";
    private final static String REFERRAL_ADV_TAG = "referralAdv";
    
    private final static String SERVER_CRED_TAG = "credential";
    
    /**
     *  ID of the server providing this response.
     */
    private ID serverID = null;
    
    /**
     *  The advertisement of the server providing this response.
     */
    private RdvAdvertisement serverAdv = null;
    
    /**
     *  The expiration duration of the server advertisement.
     */
    private long serverAdvExp = Long.MIN_VALUE;
    
    /**
     *  The advertisement generation of the server advertisement.
     */
    private UUID serverAdvGen = null;
    
    /**
     *  Credential of the server.
     */
    private XMLElement credential = null;
    
    /**
     *  Ordered list of referral advertisements.
     */
    private List<RdvAdvertisement> referralAdvs = new ArrayList<RdvAdvertisement>();
    
    /**
     *  Ordered list of referral advertisement expirations. The order matches
     *  the order of advertisements in {@link #referralAdvs}.
     */
    private List<Long> referralAdvExps = new ArrayList<Long>();
    
    /**
     *  The duration of the offered lease. May also be {@code Long.MIN_VALUE} to
     *  indicate that no lease is being offered.
     */
    private long offeredLease = Long.MIN_VALUE;
    
    /**
     *  New LeaseResponseMsg
     */
    public LeaseResponseMsg() {}
    
    /**
     * Construct from a StructuredDocument
     * @param root the element
     */
    public LeaseResponseMsg(Element root) {
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
            Attribute aRdvAttr = (Attribute) eachAttr.nextElement();
            
            if (SERVER_ID_ATTR.equals(aRdvAttr.getName())) {
                try {
                    URI srcURI = new URI(aRdvAttr.getValue());
                    ID srcID = IDFactory.fromURI(srcURI);
                    
                    setServerID(srcID);
                } catch (URISyntaxException badID) {
                    IllegalArgumentException iae = new IllegalArgumentException("Bad server ID in message");
                    
                    iae.initCause(badID);
                    throw iae;
                }
            } else if (OFFERED_LEASE_ATTR.equals(aRdvAttr.getName())) {
                offeredLease = Long.valueOf(aRdvAttr.getValue());
            } else if ("type".equals(aRdvAttr.getName())) {
                ;
            } else if ("xmlns:jxta".equals(aRdvAttr.getName())) {
                ;
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Attribute: " + aRdvAttr.getName());
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
        
        if (null == serverID) {
            throw new IllegalArgumentException("Missing Server ID.");
        }
        
        if ((null != serverAdv) && (null == serverAdvGen)) {
            throw new IllegalArgumentException("Missing Server Advertisement Generation.");
        }
        
        if ((null != serverAdv) && (Long.MIN_VALUE == serverAdvExp)) {
            throw new IllegalArgumentException("Missing Server Advertisement Expiration.");
        }
        
        if ((null != serverAdv) && (serverAdvExp <= 0)) {
            throw new IllegalArgumentException("Illegal Server Advertisement Expiration.");
        }
        
        if ((offeredLease < 0) && (Long.MIN_VALUE != offeredLease)) {
            throw new IllegalArgumentException("Illegal Lease offered.");
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Object clone()  throws CloneNotSupportedException {
        LeaseResponseMsg clone = (LeaseResponseMsg) super.clone();

        clone.setServerID(getServerID());
        clone.setServerAdv(getServerAdv(), getServerAdvGen(), getServerAdvExp());
        clone.setCredential(getCredential());
        clone.addReferralAdvs(Arrays.asList(getReferralAdvs()), Arrays.asList(getReferralAdvExps()));
        clone.setOfferedLease(getOfferedLease());
        return clone;
    }
    
    /**
     *  Returns the ID of the server providing this response.
     *
     *  @return ID of the server.
     */
    public ID getServerID() {
        return serverID;
    }
    
    /**
     *  Sets the ID of the server providing this response.
     *
     *  @param serverID ID of the server.
     */
    public void setServerID(ID serverID) {
        this.serverID = serverID;
    }
    
    /**
     *  Returns the advertisement of the server providing this response.
     *
     *  @return The servers advertisement.
     */
    public RdvAdvertisement getServerAdv() {
        return serverAdv;
    }
    
    /**
     *  Returns the advertisement generation of the server's advertisement.
     *
     *  @return The advertisement generation of the server's advertisement.
     */
    public UUID getServerAdvGen() {
        return serverAdvGen;
    }
    
    /**
     *  Returns the advertisement expiration duration of the server's 
     *  advertisement. Must be a positive integer.
     *
     *  @return The advertisement expiration duration of the server's advertisement.
     */
    public long getServerAdvExp() {
        return serverAdvExp;
    }
    
    /**
     *  Sets the server advertisement and the associated advertisement 
     *  generation and expiration.
     *
     *  @param serverAdv The servers advertisement.
     *  @param serverAdvGen The advertisement generation of the server's 
     *  advertisement. Must be a positive integer.
     *  @param serverAdvExp The advertisement expiration duration of the 
     *  server's advertisement.
     */
    public void setServerAdv(RdvAdvertisement serverAdv, UUID serverAdvGen, long serverAdvExp) {
        this.serverAdv = serverAdv;
        this.serverAdvGen = serverAdvGen;
        this.serverAdvExp = serverAdvExp;
    }
    
    /**
     *  Returns an ordered list of the referral advertisements.
     *
     *  @return An ordered list of the referral advertisements.
     */
    public RdvAdvertisement[] getReferralAdvs() {
        return referralAdvs.toArray(new RdvAdvertisement[referralAdvs.size()]);
    }
    
    /**
     *  Returns an ordered list of the referral advertisements expirations. The
     *  order of the expirations matches the order of advertisements returned
     *  by {@link #getReferralAdvs()}. Each entry is a positive integer.
     *
     *  @return An ordered list of the referral advertisements expirations.
     */
    public Long[] getReferralAdvExps() {
        return referralAdvExps.toArray(new Long[referralAdvExps.size()]);
    }
    
    /**
     *  Adds a referral advertisement to the collection of referral 
     *  advertisements. The advertisement is added at the end of the ordered
     *  list.
     *
     *  @param referralAdv The referral advertisement.
     *  @param referralAdvExp The expiration time of the referral advertisement.
     *  The value must be a positive integer.
     */
    public void addReferralAdv(RdvAdvertisement referralAdv, long referralAdvExp) {
        referralAdvs.add(referralAdv);
        referralAdvExps.add(referralAdvExp);
    }
    
    /**
     *  Adds referral advertisements to the collection of referral 
     *  advertisements. The advertisements are added at the end of the ordered 
     *  list.
     *
     *  @param referralAdvs The referral advertisements.
     *  @param referralAdvExps The expiration times of the referral advertisement.
     *  The values must be a positive integer.
     */
    public void addReferralAdvs(List<RdvAdvertisement> referralAdvs, List<Long> referralAdvExps) {
        this.referralAdvs.addAll(referralAdvs);
        this.referralAdvExps.addAll(referralAdvExps);
    }
    
    /**
     *  Clears the list of referral advertisements.
     */
    public void clearReferralAdvs() {
        referralAdvs.clear();
        referralAdvExps.clear();
    }
    
    /**
     *  Returns the lease being offered. The value must be greater than or
     *  equal to zero or the constant {@code Long.MIN_VALUE} which indicates
     *  that no lease is being offered.
     *
     *  @return The lease being offered.
     */
    public long getOfferedLease() {
        return offeredLease;
    }
    
    /**
     *  Sets the lease being offered. The value must be greater than or
     *  equal to zero or the constant {@code Long.MIN_VALUE} which indicates
     *  that no lease is being offered.
     *
     *  @param offeredLease The lease being offered.
     */
    public void setOfferedLease(long offeredLease) {
        this.offeredLease = offeredLease;
    }
    
    /**
     *  Returns the credential of the server providing this response in XML 
     *  format.
     *
     *  @return The credential associated with this response if any. May be 
     *  {@code null} to indicate that no credential was provided.
     */
    public XMLElement getCredential() {
        return (XMLElement) ((null != credential) ? StructuredDocumentUtils.copyAsDocument(credential) : null);
    }
    
    /**
     *  Sets the credential of the server providing this response in XML 
     *  format.
     *
     *  @param newCred The credential associated with this response if any. May 
     *  be {@code null} to indicate that no credential is being provided.
     */
    public void setCredential(XMLElement newCred) {
        this.credential = (XMLElement) ((null != newCred) ? StructuredDocumentUtils.copyAsDocument(newCred) : null);
    }
    
    /**
     *  Our DOCTYPE
     *
     *  @return the type of this message.
     */
    public static String getMessageType() {
        return "jxta:LeaseResponseMsg";
    }
    
    /**
     *  Process an element of the message XML document.
     *
     *  @param elem The element to process.
     *  @return If {@code true} then the element was processed otherwise {@code false}.
     */
    protected boolean handleElement(XMLElement elem) {
        
        if (SERVER_ADV_TAG.equals(elem.getName())) {
            Enumeration eachAttr = elem.getAttributes();
            
            while (eachAttr.hasMoreElements()) {
                Attribute anAdvAttr = (Attribute) eachAttr.nextElement();
                
                if (ADV_GEN_ATTR.equals(anAdvAttr.getName())) {
                    serverAdvGen = UUID.fromString(anAdvAttr.getValue());
                } else if (ADV_EXP_ATTR.equals(anAdvAttr.getName())) {
                    serverAdvExp = Long.valueOf(anAdvAttr.getValue());
                } else if ("type".equals(anAdvAttr.getName())) {
                    ;
                } else if ("xmlns:jxta".equals(anAdvAttr.getName())) {
                    ;
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled Attribute: " + anAdvAttr.getName());
                    }
                }
            }
            
            serverAdv = (RdvAdvertisement) AdvertisementFactory.newAdvertisement(elem);
            return true;
        } else if (REFERRAL_ADV_TAG.equals(elem.getName())) {
            long expiration = Long.MIN_VALUE;
            
            Enumeration eachAttr = elem.getAttributes();
            
            while (eachAttr.hasMoreElements()) {
                Attribute anAdvAttr = (Attribute) eachAttr.nextElement();
                
                if (ADV_EXP_ATTR.equals(anAdvAttr.getName())) {
                    expiration = Long.valueOf(anAdvAttr.getValue());
                } else if ("type".equals(anAdvAttr.getName())) {
                    ;
                } else if ("xmlns:jxta".equals(anAdvAttr.getName())) {
                    ;
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled Attribute: " + anAdvAttr.getName());
                    }
                }
            }
            
            if (Long.MIN_VALUE == expiration) {
                throw new IllegalArgumentException("Missing Referral Advertisement Expiration.");
            }
            
            if (expiration <= 0) {
                throw new IllegalArgumentException("Illegal Referral Advertisement Expiration.");
            }
            
            RdvAdvertisement referralAdv = (RdvAdvertisement) AdvertisementFactory.newAdvertisement(elem);
            
            // Fix the embedded Route Adv. Often it does not contain a PeerID
            // in the route because its redundant.
            referralAdv.getRouteAdv().setDestPeerID(referralAdv.getPeerID());

            referralAdvs.add(referralAdv);
            referralAdvExps.add(expiration);
            return true;
        } else if (SERVER_CRED_TAG.equals(elem.getName())) {
            credential = (XMLElement) StructuredDocumentUtils.copyAsDocument(elem);
            
            return true;
        }
        
        return false;
    }
    
    /**
     *  {@inheritDoc}
     */
    public Document getDocument(MimeMediaType mediaType) {
        if (null == serverID) {
            throw new IllegalStateException("Missing Server ID.");
        }
        
        if ((null != serverAdv) && (null == serverAdvGen)) {
            throw new IllegalStateException("Missing Server Advertisement Generation.");
        }
        
        if ((null != serverAdv) && (Long.MIN_VALUE == serverAdvExp)) {
            throw new IllegalStateException("Missing Server Advertisement Expiration.");
        }
        
        if ((null != serverAdv) && (serverAdvExp <= 0)) {
            throw new IllegalStateException("Illegal Server Advertisement Expiration.");
        }
        
        if ((offeredLease < 0) && (Long.MIN_VALUE != offeredLease)) {
            throw new IllegalStateException("Illegal Lease offered.");
        }
        
        StructuredDocument msg = StructuredDocumentFactory.newStructuredDocument(mediaType, getMessageType());
        
        if (!(msg instanceof Attributable)) {
            throw new UnsupportedOperationException("Only 'Attributable' document types are supported.");
        }
        
        if (msg instanceof XMLDocument) {
            ((XMLDocument) msg).addAttribute("xmlns:jxta", "http://jxta.org");
        }
        
        ((Attributable) msg).addAttribute(SERVER_ID_ATTR, getServerID().toString());
        
        if (Long.MIN_VALUE != offeredLease) {
            ((Attributable) msg).addAttribute(OFFERED_LEASE_ATTR, Long.toString(getOfferedLease()));
        }
        
        if (null != credential) {
            StructuredDocumentUtils.copyElements(msg, msg, credential, SERVER_CRED_TAG);
        }
        
        Element e;
        
        if (null != serverAdv) {
            e = StructuredDocumentUtils.copyElements(msg, msg, (StructuredDocument) serverAdv.getDocument(mediaType)
                    ,
                    SERVER_ADV_TAG);
            
            if (null != getServerAdvGen()) {
                ((Attributable) e).addAttribute(ADV_GEN_ATTR, getServerAdvGen().toString());
            }
            
            if (Long.MIN_VALUE != getServerAdvExp()) {
                ((Attributable) e).addAttribute(ADV_EXP_ATTR, Long.toString(getServerAdvExp()));
            }
        }
        
        Iterator<Long> eachReferralAdvExp = referralAdvExps.iterator();

        for (RdvAdvertisement aReferralAdv : referralAdvs) {
            e = StructuredDocumentUtils.copyElements(msg, msg, (StructuredDocument) aReferralAdv.getDocument(mediaType)
                    ,
                    REFERRAL_ADV_TAG);
            
            long expiration = eachReferralAdvExp.next();
            
            if (Long.MIN_VALUE == expiration) {
                throw new IllegalStateException("Missing Referral Advertisement Expiration.");
            }
            
            if (expiration <= 0) {
                throw new IllegalStateException("Illegal Referral Advertisement Expiration.");
            }
            
            ((Attributable) e).addAttribute(ADV_EXP_ATTR, Long.toString(expiration));
        }
        
        return msg;
    }
}
