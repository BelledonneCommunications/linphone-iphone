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


import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Document;
import net.jxta.document.MimeMediaType;


/**
 * This class defines the Discovery Service message "Query". The default
 * behavior of this abstract class is simply a place holder for the generic
 * resolver query fields.
 *
 * @see net.jxta.discovery.DiscoveryService
 * @see net.jxta.protocol.DiscoveryResponseMsg
 */

public abstract class DiscoveryQueryMsg {

    private PeerAdvertisement peerAdvertisement = null;

    /**
     * default threshold to limit the number of responses from one peer
     */
    private int threshold = 10;

    /**
     * FIXME 20030227 bondolo@jxta.org not a great default...
     */
    private int type = DiscoveryService.PEER;

    private String attr = null;
    private String value = null;

    /**
     * returns the Attr value
     *
     * @return String value of Attribute
     */
    public String getAttr() {
        return attr;
    }

    /**
     * Get the response type
     *
     * @return int PEER, or GROUP discovery type response
     */
    public int getDiscoveryType() {
        return type;
    }

    /**
     * Write advertisement into a document. asMimeType is a mime media-type
     * specification and provides the form of the document which is being
     * requested. Two standard document forms are defined. "text/text" encodes
     * the document in a form nice for printing out, and "text/xml" which
     * provides an XML representation.
     *
     * @param asMimeType mime-type format requested
     * @return Document representation of the document as an
     *         advertisement
     */
    public abstract Document getDocument(MimeMediaType asMimeType);

    /**
     * returns the responding peer advertisement
     *
     * @return String handlername name
     * @deprecated Peer Advertisement is available directly via
     *             {@link #getPeerAdvertisement()}.
     */
    @Deprecated
    public String getPeerAdv() {
        if (null != peerAdvertisement) {
            return peerAdvertisement.toString();
        } else {
            return null;
        }
    }

    /**
     * returns the querying peer's advertisement
     *
     * @return peer advertisement of querier.
     */
    public PeerAdvertisement getPeerAdvertisement() {
        return peerAdvertisement;
    }

    /**
     * returns the responding peer's advertisement
     *
     * @param newAdv peer advertisement of querier.
     */
    public void setPeerAdvertisement(PeerAdvertisement newAdv) {
        peerAdvertisement = newAdv;
    }

    /**
     * Get the Threshold for number of responses
     *
     * @return int threshold
     */
    public int getThreshold() {
        return threshold;
    }

    /**
     * returns the value of Attr
     *
     * @return String
     */
    public String getValue() {
        return value;
    }

    /**
     * set the attr
     *
     * @param attr attribute of the query
     */
    public void setAttr(String attr) {
        this.attr = attr;
    }

    /**
     * set the Response type whether it's peer, or group discovery
     *
     * @param type type of discovery
     */
    public void setDiscoveryType(int type) {
        // FIXME 20030227 bondolo@jxta.org This should be value checked.
        this.type = type;
    }

    /**
     * set the threshold
     *
     * @param threshold value to be set
     */
    public void setThreshold(int threshold) {
        this.threshold = threshold;
    }

    /**
     * set the query
     *
     * @param value value of the attribute to query
     */
    public void setValue(String value) {
        this.value = value;
    }

    /**
     * All messages have a type (in xml this is &#0033;doctype) which
     * identifies the message
     *
     * @return String "jxta:DiscoveryQuery"
     */
    public static String getAdvertisementType() {
        return "jxta:DiscoveryQuery";
    }
}

