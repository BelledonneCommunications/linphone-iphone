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
import java.net.URISyntaxException;
import java.util.Enumeration;

import java.util.logging.Level;

import net.jxta.logging.Logging;

import java.util.logging.Logger;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.TextElement;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.id.IDFactory;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.RouteAdvertisement;

/**
 * Implements the Resolver Query Message according to the
 * schema defined by the core JXTA Peer Resolver Protocol (PRP).
 * <p/>
 * <p/><pre>
 * &lt;xs:element name="ResolverQuery" type="jxta:ResolverQuery"/>
 * <p/>
 * &lt;xs:complexType name="ResolverQuery">
 *   &lt;xs:all>
 *     &lt;xs:element ref="jxta:Cred" minOccurs="0"/>
 *     &lt;xs:element name="SrcPeerID" type="jxta:JXTAID"/>
 *     &lt;xs:element name="SrcPeerRoute" type="jxta:JXTA RouteAdv"/>
 *     &lt;!-- This could be extended with a pattern restriction -->
 *     &lt;xs:element name="HandlerName" type="xs:string"/>
 *     &lt;xs:element name="QueryID" type="xs:string"/>
 *     &lt;xs:element name="HC" type="xs:unsignedInt"/>
 *     &lt;xs:element name="Query" type="xs:anyType"/>
 *   &lt;/xs:all>
 * &lt;/xs:complexType>
 * </pre>
 * <p/>
 * <p/><ephasis>IMPORTANT</emphasis>: a ResolverQuery contains an internal
 * state, the hopCount, which is incremented by various services that needs to.
 * As a result, a ResolverQuery may have to be cloned when the hopCount state
 * needs to be reset.
 *
 * @see net.jxta.resolver.ResolverService
 * @see net.jxta.protocol.ResolverQueryMsg
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-prp" target="_blank">JXTA Protocols Specification : Peer Resolver Protocol</a>
 */
public class ResolverQuery extends ResolverQueryMsg implements Cloneable {

    /**
     * The logger
     */
    private final static Logger LOG = Logger.getLogger(ResolverQuery.class.getName());

    private static final String handlernameTag = "HandlerName";
    private static final String credentialTag = "jxta:Cred";
    private static final String queryIdTag = "QueryID";
    private static final String hopCountTag = "HC";
    private static final String srcPeerIdTag = "SrcPeerID";
    private static final String srcRouteTag = "SrcPeerRoute";
    private static final String queryTag = "Query";

    /**
     * Default constructor
     */
    public ResolverQuery() {
        super();
    }

    /**
     * Construct a doc from strings
     *
     * @param HandlerName the handler name
     * @param Credential  credential document
     * @param pId         source PeerID
     * @param Query       opaque query string
     * @param qid         query ID
     * @deprecated use the individual accessor methods instead.
     */
    @Deprecated
    public ResolverQuery(String HandlerName, StructuredDocument Credential, String pId, String Query, int qid) {

        this();
        setHandlerName(HandlerName);
        setCredential(Credential);
        setQueryId(qid);
        setSrc(pId);
        setQuery(Query);
    }

    /**
     * Construct from a StructuredDocument
     *
     * @param root the element
     */
    public ResolverQuery(Element root) {

        this();
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;
        String doctype = doc.getName();

        if (!getAdvertisementType().equals(doctype)) {
            throw new IllegalArgumentException("Could not construct : " + getClass().getName() + "from doc containing a " + doctype);
        }
        readIt(doc);

        // sanity check!
        if (null == getHandlerName()) {
            throw new IllegalArgumentException("Query message does not contain a handler name.");
        }

        if (null == getQuery()) {
            throw new IllegalArgumentException("Query message does not contain a query.");
        }

        if (null == getSrcPeer()) {
            throw new IllegalArgumentException("Query message does not define a source.");
        }

        RouteAdvertisement ra = getSrcPeerRoute();

        if ((null != ra) && (null == ra.getDestPeerID())) {
            throw new IllegalArgumentException("Route does not define a destination.");
        }
    }

    /**
     * parses an XML document into this object
     *
     * @param doc the element
     */
    public void readIt(XMLElement doc) {

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            TextElement elem = (TextElement) elements.nextElement();

            if (elem.getName().equals(handlernameTag)) {
                setHandlerName(elem.getTextValue());
                continue;
            }
            // Set credential
            if (elem.getName().equals(credentialTag)) {
                setCredential(StructuredDocumentUtils.copyAsDocument(elem));
                continue;
            }
            // Set queryid
            if (elem.getName().equals(queryIdTag)) {
                queryid = Integer.parseInt(elem.getTextValue());
                continue;
            }

            // Set source route
            if (elem.getName().equals(srcRouteTag)) {
                for (Enumeration eachXpt = elem.getChildren(); eachXpt.hasMoreElements();) {
                    XMLElement aXpt = (XMLElement) eachXpt.nextElement();
                    RouteAdvertisement routeAdv = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(aXpt);

                    if (null != routeAdv.getDestPeerID()) {
                        setSrcPeerRoute(routeAdv);
                        setSrcPeer(routeAdv.getDestPeerID());
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Incomplete Route Advertisement (missing peer id).");
                        }
                    }
                }
                continue;
            }

            // Set hopcount
            if (elem.getName().equals(hopCountTag)) {
                setHopCount(Integer.parseInt(elem.getTextValue()));
                continue;
            }

            // Set source peer
            // FIXME tra 20031108 Since Peer Id is already part
            // of the SrcRoute Tag. We should be able to remove
            // processing this tag in the future.
            if (elem.getName().equals(srcPeerIdTag)) {
                try {
                    String value = elem.getTextValue();
                    if (value != null && value.length() > 0) {
                        URI srcURI = new URI(elem.getTextValue());
                        setSrcPeer(IDFactory.fromURI(srcURI));
                    }
                } catch (URISyntaxException failed) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Bad ID in message", failed);
                    }
                    RuntimeException failure = new IllegalArgumentException("Bad ID in message");
                    failure.initCause(failed);
                    throw failure;
                }
                continue;
            }
            // Set query
            if (elem.getName().equals(queryTag)) {
                setQuery(elem.getTextValue());
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        // sanity check!
        if (null == getHandlerName()) {
            throw new IllegalStateException("Query message does not contain a handler name.");
        }

        if (null == getQuery()) {
            throw new IllegalStateException("Query message does not contain a query.");
        }

        if (null == getSrcPeer()) {
            throw new IllegalStateException("Query message does not define a source.");
        }

        RouteAdvertisement ra = getSrcPeerRoute();

        if ((null != ra) && (null == ra.getDestPeerID())) {
            throw new IllegalStateException("Route does not define a destination.");
        }

        StructuredTextDocument adv = (StructuredTextDocument)
                StructuredDocumentFactory.newStructuredDocument(encodeAs, getAdvertisementType());

        if (adv instanceof XMLDocument) {
            ((XMLDocument) adv).addAttribute("xmlns:jxta", "http://jxta.org");
        }
        Element e;

        e = adv.createElement(handlernameTag, getHandlerName());
        adv.appendChild(e);
        if (getCredential() != null) {
            StructuredDocumentUtils.copyElements(adv, adv, getCredential());
        }
        e = adv.createElement(queryIdTag, Integer.toString(queryid));
        adv.appendChild(e);
        e = adv.createElement(hopCountTag, Integer.toString(hopcount));
        adv.appendChild(e);

        // FIXME tra 20031108 Since Peer Id is already part of the SrcRoute Tag.
        // We should stop emitting this tag in the future.
        if (null != getSrcPeer()) {
            e = adv.createElement(srcPeerIdTag, getSrcPeer().toString());
            adv.appendChild(e);
        }

        RouteAdvertisement radv = this.getSrcPeerRoute();

        if (radv != null) {
            e = adv.createElement(srcRouteTag);
            adv.appendChild(e);
            StructuredTextDocument xptDoc = (StructuredTextDocument)radv.getDocument(encodeAs);
            StructuredDocumentUtils.copyElements(adv, e, xptDoc);
        }

        e = adv.createElement(queryTag, getQuery());
        adv.appendChild(e);
        return adv;
    }

    /**
     * {@inheritDoc}
     * <p/>Result is the query as an XML string.
     */
    @Override
    public String toString() {
        return getDocument(MimeMediaType.XMLUTF8).toString();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ResolverQuery clone() {
        ResolverQuery tmp;

        try {
            tmp = (ResolverQuery) super.clone();
        } catch (CloneNotSupportedException e) {
            throw new Error("Object.clone() threw CloneNotSupportedException", e);
        }

        tmp.setHandlerName(getHandlerName());
        tmp.setCredential(getCredential());
        tmp.setSrcPeer(getSrcPeer());
        tmp.setQuery(getQuery());
        tmp.setQueryId(getQueryId());
        tmp.setHopCount(getHopCount());
        tmp.setSrcPeerRoute(getSrcPeerRoute());
        
        return tmp;
    }

    /**
     * {@inheritDoc}
     *
     * @return ResolverResponse Msg
     */
    @Override
    public ResolverResponseMsg makeResponse() {
        // construct a new response
        ResolverResponse res = new ResolverResponse();

        // transfer the query information
        res.setHandlerName(this.getHandlerName());
        res.setQueryId(this.getQueryId());

        // transfer optional route information available in the query
        // to the response.
        //
        // NOTE: The route field is just attached to the response and
        // will not be sent as part of the response. We just use this to
        // pass information to the resolver. The other alternative will
        // be to add a resolver or peergroup arg to makeResponse() as
        // we don't have access to any peergroup info here to process
        // the route information we just received. We will process
        // the information just before we send the response. This
        // may be better anyway as the service may never really respond.
        res.setSrcPeerRoute(this.getSrcPeerRoute());

        return res;
    }
}
