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
package net.jxta.impl.endpoint.router;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.protocol.AccessPointAdvertisement;
import net.jxta.protocol.RouteAdvertisement;
import net.jxta.logging.Logging;

import java.util.Enumeration;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Router Message element. This element is added to every message to carry route 
 * information for the EndpointRouter service.
 */
public class EndpointRouterMessage {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(EndpointRouterMessage.class.getName());

    public static final String MESSAGE_NS = "jxta";
    public static final String MESSAGE_NAME = "EndpointRouterMsg";
    public static final String Name = "jxta:ERM";

    public static final String SrcTag = "Src";
    public static final String DestTag = "Dest";
    public static final String LastHopTag = "Last";
    public static final String GatewayForwardTag = "Fwd";
    public static final String GatewayReverseTag = "Rvs";

    private EndpointAddress srcAddress = null; // PeerID-based EndpointAddress
    private EndpointAddress destAddress = null; // PeerID-based EndpointAddress
    private EndpointAddress lastHop = null; // Plain PeerID
    
    private transient Vector<AccessPointAdvertisement> forwardGateways = null;
    private transient Vector<XMLElement> forwardCache = null;
    private transient Vector<AccessPointAdvertisement> reverseGateways = null;
    private transient Vector<XMLElement> reverseCache = null;
    private transient RouteAdvertisement radv = null;

    // A flag that represents the existence of data.  Which is
    // different from all fields being empty.
    private transient boolean rmExists = false;

    // A flag that tells us that the message is not uptodate compared to
    // This object.
    private transient boolean rmDirty = false;

    // Keep tied to one and only one message.
    private final transient Message message;

    // Cache the element. At the minimum it simplifies removal.
    private transient MessageElement rmElem = null;

    public boolean msgExists() {
        return rmExists;
    }

    public boolean isDirty() {
        return rmDirty;
    }

    public EndpointRouterMessage(Message message, boolean removeMsg) {

        this.message = message;

        try {
            rmElem = message.getMessageElement(MESSAGE_NS, MESSAGE_NAME);
            if (rmElem == null) {
                return;
            }

            // We have an element, but until we read it, no data to
            // match (rmExists == false). If the data cannot be read
            // from the element, the element is scheduled for removal.
            rmDirty = true;

            // If we have been instructed so, do not parse any existing
            // element, and leave it marked for removal from the message
            // as if it were invalid.
            if (removeMsg) {
                return;
            }

            XMLDocument doc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(rmElem);

            Enumeration<XMLElement> each;
            XMLElement e;

            each = doc.getChildren();
            if (!each.hasMoreElements()) {
                // results in rmExists being false.
                return;
            }

            while (each.hasMoreElements()) {
                try {
                    e = each.nextElement();

                    if (e.getName().equals(SrcTag)) {
                        srcAddress = new EndpointAddress(e.getTextValue());
                        continue;
                    }

                    if (e.getName().equals(DestTag)) {
                        destAddress = new EndpointAddress(e.getTextValue());
                        continue;
                    }

                    if (e.getName().equals(LastHopTag)) {
                        lastHop = new EndpointAddress(e.getTextValue());
                        continue;
                    }

                    if (e.getName().equals(GatewayForwardTag)) {
                        for (Enumeration<XMLElement> eachXpt = e.getChildren(); eachXpt.hasMoreElements();) {

                            if (forwardGateways == null) {
                                forwardGateways = new Vector<AccessPointAdvertisement>();
                            }
                            if (forwardCache == null) {
                                forwardCache = new Vector<XMLElement>();
                            }
                            XMLElement aXpt = eachXpt.nextElement();
                            AccessPointAdvertisement xptAdv = (AccessPointAdvertisement)
                                    AdvertisementFactory.newAdvertisement(aXpt);

                            forwardGateways.addElement(xptAdv);
                            // Save the original element
                            forwardCache.addElement(aXpt);
                        }
                        continue;
                    }

                    if (e.getName().equals(GatewayReverseTag)) {
                        for (Enumeration<XMLElement> eachXpt = e.getChildren(); eachXpt.hasMoreElements();) {
                            if (reverseGateways == null) {
                                reverseGateways = new Vector<AccessPointAdvertisement>();
                            }
                            if (reverseCache == null) {
                                reverseCache = new Vector<XMLElement>();
                            }
                            XMLElement aXpt = eachXpt.nextElement();
                            AccessPointAdvertisement xptAdv = (AccessPointAdvertisement)
                                    AdvertisementFactory.newAdvertisement(aXpt);

                            reverseGateways.addElement(xptAdv);
                            // Save the original element
                            reverseCache.addElement(aXpt);
                        }
                        continue;
                    }

                    if (e.getName().equals(RouteAdvertisement.getAdvertisementType())) {
                        radv = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(e);
                    }
                } catch (Exception ee) {
                    // keep going
                }
            }
            
            // XXX 20040929 bondolo Should be doing validation here.

            // All parsed ok, we're in sync.
            rmExists = true;
            rmDirty = false;
        } catch (Exception eee) {
            // give up. The dirty flag will get the element removed
            // from the message (if there was one) and we'll report
            // there was none.
        }
    }

    public void updateMessage() {

        if (!rmDirty) {
            return;
        }

        if (!rmExists) {

            // The change was to remove it.
            // If there was an rmElem, remove it and make sure to remove
            // all of them. We may have found one initialy but there may be
            // several. (just a sanity check for outgoing messages).

            while (rmElem != null) {
                message.removeMessageElement(MESSAGE_NS, rmElem);
                rmElem = message.getMessageElement(MESSAGE_NS, MESSAGE_NAME);
            }

            rmDirty = false;
            return;
        }

        // The element was either created or changed. Replace whatever
        // if anything was in the message

        XMLDocument doc = (XMLDocument)
                StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, Name);

        doc.addAttribute("xmlns:jxta", "http://jxta.org");
        doc.addAttribute("xml:space", "preserve");

        XMLElement e;

        if (srcAddress != null) {
            e = doc.createElement(SrcTag, srcAddress.toString());
            doc.appendChild(e);
        }

        if (destAddress != null) {
            e = doc.createElement(DestTag, destAddress.toString());
            doc.appendChild(e);
        }

        if (lastHop != null) {
            e = doc.createElement(LastHopTag, lastHop.toString());
            doc.appendChild(e);
        }

        e = doc.createElement(GatewayForwardTag);
        doc.appendChild(e);
        if ((forwardGateways != null) && (!forwardGateways.isEmpty())) {
            if (forwardCache != null) {
                for (XMLElement xptDoc : forwardCache) {
                    try {
                        StructuredDocumentUtils.copyElements(doc, e, xptDoc);
                    } catch (Exception e1) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Forward cache failed");
                        }
                        forwardCache = null;
                        break;
                    }
                }
            } else {
                for (AccessPointAdvertisement gateway : forwardGateways) {
                    try {
                        XMLDocument xptDoc = (XMLDocument) gateway.getDocument(MimeMediaType.XMLUTF8);
                        StructuredDocumentUtils.copyElements(doc, e, xptDoc);
                    } catch (Exception ignored) {
                        //ignored
                    }
                }
            }
        }

        e = doc.createElement(GatewayReverseTag);
        doc.appendChild(e);
        if ((reverseGateways != null) && (!reverseGateways.isEmpty())) {
            if (reverseCache != null) {
                for (XMLElement xptDoc : reverseCache) {
                    try {
                        StructuredDocumentUtils.copyElements(doc, e, xptDoc);
                    } catch (Exception e1) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Reverse cache failed");
                        }
                        reverseCache = null;
                        break;
                    }
                }
            } else {
                 for (AccessPointAdvertisement gateway : reverseGateways) {
                    try {
                        XMLDocument xptDoc = (XMLDocument) gateway.getDocument(MimeMediaType.XMLUTF8);
                        StructuredDocumentUtils.copyElements(doc, e, xptDoc);
                    } catch (Exception e1) {
                        // ignored
                    }
                }
            }
        }

        if (radv != null) {
            try {
                XMLDocument radvDoc = (XMLDocument) radv.getDocument(MimeMediaType.XMLUTF8);

                StructuredDocumentUtils.copyElements(doc, doc, radvDoc);
            } catch (Exception e1) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Cannot add route advertisement");
                }
            }
        }

        rmElem = new TextDocumentMessageElement(MESSAGE_NAME, doc, null);
        message.replaceMessageElement(MESSAGE_NS, rmElem);

        rmDirty = false;
    }

    public void setSrcAddress(EndpointAddress address) {
        rmExists = true;
        rmDirty = true;
        srcAddress = address;
    }

    public EndpointAddress getSrcAddress() {
        return srcAddress;
    }

    public void setDestAddress(EndpointAddress address) {
        rmExists = true;
        rmDirty = true;
        destAddress = address;
    }

    public EndpointAddress getDestAddress() {
        return destAddress;
    }

    public void setLastHop(EndpointAddress lhop) {
        rmExists = true;
        rmDirty = true;
        lastHop = lhop;
    }

    public EndpointAddress getLastHop() {
        return lastHop;
    }

    public void setForwardHops(Vector<AccessPointAdvertisement> fhops) {
        rmExists = true;
        rmDirty = true;
        forwardGateways = fhops;
        forwardCache = null;
    }

    public Vector<AccessPointAdvertisement> getForwardHops() {
        return forwardGateways;
    }

    public void prependReverseHop(AccessPointAdvertisement apa) {
        rmExists = true;
        rmDirty = true;
        if (reverseGateways == null) {
            reverseGateways = new Vector<AccessPointAdvertisement>();
            reverseCache = new Vector<XMLElement>();
        }

        reverseGateways.add(0, apa);

        if (reverseCache == null) {
            return;
        }

        // if we still have a cache (we where able to keep it conistent, update it
        XMLDocument apDoc = (XMLDocument) apa.getDocument(MimeMediaType.XMLUTF8);

        reverseCache.add(0, apDoc);
    }

    // Do not call this routine lightly: it blasts the cache.
    public void setReverseHops(Vector<AccessPointAdvertisement> rhops) {
        rmExists = true;
        rmDirty = true;

        // No inplace changes allowed, we need to keep the cache
        // consistent: clone

        if (rhops == null) {
            reverseGateways = null;
        } else {
            reverseGateways = (Vector<AccessPointAdvertisement>) rhops.clone();
        }

        // Not worth updating the cache. Blast it.
        reverseCache = null;
    }

    public Vector<AccessPointAdvertisement> getReverseHops() {

        if (reverseGateways == null) {
            return null;
        }

        return (Vector<AccessPointAdvertisement>) reverseGateways.clone();
    }

    public RouteAdvertisement getRouteAdv() {
        return radv;
    }

    public void setRouteAdv(RouteAdvertisement radv) {
        rmExists = true;
        rmDirty = true;
        this.radv = radv;
    }

    // Used only for debugging
    public String display() {
        StringBuilder msgInfo = new StringBuilder("Endpoint Router Message : ");

        msgInfo.append("\n\tsrc=");
        msgInfo.append((srcAddress != null) ? srcAddress : "none");
        msgInfo.append("\n\tdest== ");
        msgInfo.append((destAddress != null) ? destAddress : "none");
        msgInfo.append("\n\tlastHop= ");
        msgInfo.append((lastHop != null) ? lastHop : "none");
        msgInfo.append("\n\tembedded radv= ");
        msgInfo.append(radv != null ? radv.display() : "none");
        if (forwardGateways != null) {
            msgInfo.append("\n\tForward Hops:");
            for (int i = 0; i < forwardGateways.size(); ++i) {
                try {
                    msgInfo.append("   [").append(i).append("] ");
                    msgInfo.append(forwardGateways.elementAt(i).getPeerID());
                }
                catch (Exception ez1) {
                    break;
                }
            }
        }
        if (reverseGateways != null) {
            msgInfo.append("\n\tReverse Hops:");
            for (int i = 0; i < reverseGateways.size(); ++i) {
                    msgInfo.append("   [").append(i).append("] ");
                    msgInfo.append(reverseGateways.elementAt(i).getPeerID());
            }
        }
        return msgInfo.toString();
    }

    // This will ensure that all older elements will be removed from
    // the message in case they do not get replaced by new ones before
    // updateMsg is called.

    public void clearAll() {

        if (rmExists) {
            rmDirty = true;

            srcAddress = null;
            destAddress = null;
            lastHop = null;
            forwardGateways = null;
            reverseGateways = null;
            radv = null;
            rmExists = false;
        }
    }
}

