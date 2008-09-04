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
package net.jxta.protocol;

import java.net.URI;

import net.jxta.document.Document;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.id.ID;

/**
 * Generic Resolver Service message "Query".
 *
 * @see net.jxta.resolver.ResolverService
 * @see net.jxta.protocol.ResolverResponseMsg
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-prp" target="_blank">JXTA Protocols Specification : Peer Resolver Protocol</a>
 */
public abstract class ResolverQueryMsg {

    /**
     * credential
     */
    private StructuredDocument credential = null;

    /**
     * Resolver query handler
     */
    private String handlername = null;

    /**
     * Number of times a message has been forwarded, not propagated or walked
     */
    protected int hopcount = 0;

    /**
     * Resolver query
     */
    private String query = null;

    /**
     * Query ID of this query. Unique to the originating node only, it can be
     * utilized to match queries to responses.
     */
    protected int queryid = 0;

    /**
     * issuer of the query
     */
    private ID srcPeerId = null;

    /**
     * Optional route info about the issuer
     */
    private RouteAdvertisement srcPeerRoute = null;

    /**
     * returns the credential
     *
     * @return String credential
     */
    public StructuredDocument getCredential() {
        return credential;
    }

    /**
     * Write advertisement into a document. asMimeType is a mime media-type
     * specification and provides the form of the document which is being
     * requested. Two standard document forms are defined. 'text/text' encodes
     * the document in a form nice for printing out and 'text/xml' which
     * provides an XML format.
     *
     * @param asMimeType mime-type requested representation for the returned
     *                   document
     * @return Document document representing the advertisement
     */
    public abstract Document getDocument(MimeMediaType asMimeType);

    /**
     * returns the handlername
     *
     * @return String handlername name
     */
    public String getHandlerName() {
        return handlername;
    }

    /**
     * returns the hop count
     *
     * @return int hop count
     */
    public int getHopCount() {
        return hopcount;
    }

    /**
     * increment hop count
     */
    public void incrementHopCount() {
        hopcount++;
    }

    /**
     * Set hop count
     *
     * @param newCount hop count
     */
    public void setHopCount(int newCount) {
        hopcount = newCount;
    }

    /**
     * returns the query
     *
     * @return String value of the query
     */
    public String getQuery() {
        return query;
    }

    /**
     * returns queryid value
     *
     * @return int queryid value
     */
    public int getQueryId() {
        return queryid;
    }

    /**
     * Returns the source of the query
     *
     * @return String the peerid of the source of the query
     * @deprecated Use {@link #getSrcPeer()} instead.
     */
    @Deprecated
    public String getSrc() {
        return (null == srcPeerId) ? null : srcPeerId.toString();
    }

    /**
     * Returns the source of the query
     *
     * @return The peerid of the source of the query
     */
    public ID getSrcPeer() {
        return (null == srcPeerId) ? null : srcPeerId;
    }

    /**
     * Returns the source route of the query
     *
     * @return RouteAdvertisement route to the issuer of the query
     */

    public RouteAdvertisement getSrcPeerRoute() {
        if (srcPeerRoute == null) {
            return null;
        } else {
            return srcPeerRoute.clone();
        }
    }

    /**
     * set the credential
     *
     * @param cred string representing credential
     */
    public void setCredential(StructuredDocument cred) {
        this.credential = cred;
    }

    /**
     * set the handlername
     *
     * @param name handler name
     */
    public void setHandlerName(String name) {
        this.handlername = name;
    }

    /**
     * set the Query string
     *
     * @param Query string representing the query
     */
    public void setQuery(String Query) {
        this.query = Query;
    }

    /**
     * set the query id. Each query has a unique id.
     *
     * @param id int id
     */
    public void setQueryId(int id) {
        queryid = id;
    }

    /**
     * set the source route of the query
     *
     * @param route route advertisement of the source peer
     */
    public void setSrcPeerRoute(RouteAdvertisement route) {
        srcPeerRoute = route;
    }

    /**
     * Set the source of the query
     *
     * @param src is a containing the peerid of the source
     * @deprecated Use {@link #setSrcPeer(ID)} instead.
     */
    @Deprecated
    public void setSrc(String src) {
        if (null == src) {
            setSrcPeer(null);
        } else {
            setSrcPeer(ID.create(URI.create(src)));
        }
    }

    /**
     * Set the source of the query
     *
     * @param srcPeer the peerid of the source
     */
    public void setSrcPeer(ID srcPeer) {
        srcPeerId = srcPeer;
    }

    /**
     * All messages have a type (in xml this is !doctype) which identifies the
     * message
     *
     * @return String "jxta:ResolverQuery"
     */
    public static String getAdvertisementType() {
        return "jxta:ResolverQuery";
    }

    /**
     * Create a ResolverResponse from a ResolverQuery message. This method
     * takes advantage of any internal information available in an incoming
     * Resolver query to build a resolver response for that query. For instance,
     * optional route information which may be available in the query will
     * be used to bypass the route resolution to send the response.
     * <p/>
     * WARNING: A side effect of this call is that the following fields are
     * transfered from the query to the response:
     * - HandlerName
     * - QueryId
     *
     * @return ResolverResponseMsg resolverResponse built from the resolverQuery msg
     */
    public abstract ResolverResponseMsg makeResponse();
}
