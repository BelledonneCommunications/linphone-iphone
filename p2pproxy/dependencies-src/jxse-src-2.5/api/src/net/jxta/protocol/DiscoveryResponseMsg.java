/*
 * Copyright(c) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
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


import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.util.*;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Document;
import net.jxta.document.MimeMediaType;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 *  This class defines the DiscoveryService message "Response". <p/>
 *
 *  The default behavior of this abstract class is simply a place holder for the
 *  generic resolver query fields. This message is the response to the
 *  DiscoveryQueryMsg.
 *
 *@see    net.jxta.discovery.DiscoveryService
 *@see    net.jxta.protocol.DiscoveryQueryMsg
 */
public abstract class DiscoveryResponseMsg {
    
    /**
     *  Logger
     */
    private final static transient Logger LOG = Logger.getLogger(DiscoveryResponseMsg.class.getName());
    
    /**
     *  attribute used by the query
     */
    protected String attr = null;
    
    /**
     *  Responding peer's advertisement
     */
    protected PeerAdvertisement peerAdvertisement = null;
    
    /**
     * The advertisement responses serialized into strings.
     */
    protected final List<String> responses = new ArrayList<String>(0);
    
    /**
     *  The advertisement responses deserialized.
     */
    protected final List<Advertisement> advertisements = new ArrayList<Advertisement>(0);
    
    /**
     *  Expirations
     */
    protected final List<Long> expirations = new ArrayList<Long>(0);
    
    /**
     *  Advertisement type used by the query
     *
     *  <p/>FIXME 20040514 bondolo@jxta.org not a great default...
     */
    protected int type = DiscoveryService.PEER;
    
    /**
     *  Value used by the query
     */
    protected String value = null;
    
    /**
     *  All messages have a type(in xml this is !doctype) which identifies the
     *  message
     *
     * @return    String "jxta:ResolverResponse"
     */
    public static String getAdvertisementType() {
        return "jxta:DiscoveryResponse";
    }
    
    /**
     *  Get the response type
     *
     * @return int type of discovery message PEER, GROUP or ADV discovery type
     *      response
     */
    public int getDiscoveryType() {
        return type;
    }
    
    /**
     *  set the Response type whether it's peer, or group discovery
     *
     * @param  type  int representing the type
     */
    public void setDiscoveryType(int type) {
        this.type = type;
    }
    
    /**
     *  Write advertisement into a document. asMimeType is a mime media-type
     *  specification and provides the form of the document which is being
     *  requested. Two standard document forms are defined. "text/text" encodes
     *  the document in a form nice for printing out and "text/xml" which
     *  provides an XML format.
     *
     * @param  asMimeType  mime-type requested
     * @return             Document document that represents the advertisement
     */
    
    public abstract Document getDocument(MimeMediaType asMimeType);
    
    /**
     *  returns the responding peer's advertisement
     *
     * @return the Peer's advertisement
     */
    public PeerAdvertisement getPeerAdvertisement() {
        return peerAdvertisement;
    }
    
    /**
     *  Sets the responding peer's advertisement
     *
     * @param newAdv the responding Peer's advertisement
     */
    public void setPeerAdvertisement(PeerAdvertisement newAdv) {
        peerAdvertisement = newAdv;
    }
    
    /**
     * returns the attributes used by the query
     *
     * @return    String attribute of the query
     */
    public String getQueryAttr() {
        return attr;
    }
    
    /**
     *  returns the value used by the query
     *
     *@return    String value used by the query
     */
    public String getQueryValue() {
        return value;
    }
    
    /**
     *  Get the response count
     *
     * @return    int count
     */
    public int getResponseCount() {
        if (expirations.isEmpty() && (peerAdvertisement != null) && (type == DiscoveryService.PEER)) {
            return 1;
        } else {
            return responses.size();
        }
    }
    
    /**
     *  Gets the expirations attribute of the DiscoveryResponseMsg object
     *
     * @return    The expirations value
     */
    public Enumeration<Long> getExpirations() {
        if (expirations.isEmpty() && (peerAdvertisement != null) && (type == DiscoveryService.PEER)) {
            // this takes care of the case where the only response is the peerAdv
            expirations.add(DiscoveryService.DEFAULT_EXPIRATION);
        }
        
        return Collections.enumeration(expirations);
    }
    
    /**
     * set the expirations for this query
     *
     * @param  expirations  the expirations for this query
     */
    public void setExpirations(List<Long> expirations) {
        this.expirations.clear();
        this.expirations.addAll(expirations);
    }
    
    /**
     * returns the response(s)
     *
     * @return    Enumeration of String responses
     */
    public Enumeration<String> getResponses() {
        if (responses.isEmpty() && (peerAdvertisement != null) && (type == DiscoveryService.PEER)) {
            // this takes care of the case where the only response is the peerAdv
            responses.add(peerAdvertisement.toString());
        }
        
        return Collections.enumeration(responses);
    }
    
    /**
     *  Set the responses to the query. The responses may be either
     *  {@code Advertisement}, {@code String} or {@code InputStream}.
     *
     *  @param  responses List of responses
     */
    public void setResponses(List responses) {
        this.responses.clear();
        
        for (Object response : responses) {
            if (response instanceof Advertisement) {
                this.responses.add(((Advertisement) response).getDocument(MimeMediaType.XMLUTF8).toString());
            } else if (response instanceof String) {
                this.responses.add((String) response);
            } else if (response instanceof InputStream) {
                String result = streamToString((InputStream) response);
                
                if (null != result) {
                    this.responses.add(result);
                }
            } else {
                throw new IllegalArgumentException("Non-String or InputStream response recevied.");
            }
        }
    }

    /**
     *  Reads in a stream into a string
     *
     *  @param  is  inputstream
     *  @return     string representation of a stream
     */
    private String streamToString(InputStream is) {
        Reader reader = null;
        
        try {
            reader = new InputStreamReader(is, "UTF-8");
        } catch (UnsupportedEncodingException impossible) {
            throw new Error("UTF-8 encoding not supported?!?");
        }
        
        StringBuilder stw = new StringBuilder();
        char[] buf = new char[512];

        try {
            do {
                int c = reader.read(buf);

                if (c == -1) {
                    break;
                }
                stw.append(buf, 0, c);
            } while (true);
        } catch (IOException ie) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Got an Exception during stream conversion", ie);
            }
            return null;
        } finally {
            try {
                is.close();
            } catch (IOException ignored) {}
        }

        return stw.toString();
    }
    
    /**
     *  Set the attribute used by the query
     *
     *  @param  attr query attribute
     */
    public void setQueryAttr(String attr) {
        this.attr = attr;
    }
    
    /**
     *  Set the value used by the query
     *
     *  @param  value Query value
     */
    public void setQueryValue(String value) {
        this.value = value;
    }
    
    /**
     * Get the responses to the query as advertisements.
     *
     * @return The response advertisements.
     */
    public Enumeration<Advertisement> getAdvertisements() {
        if (responses.isEmpty() && (peerAdvertisement != null) && (type == DiscoveryService.PEER)) {
            // this takes care of the case where the only response is the peerAdv
            return Collections.enumeration(Collections.singletonList((Advertisement) peerAdvertisement));
        }
        
        if (advertisements.isEmpty() && !responses.isEmpty()) {
            // Convert the responses.
            for (String aResponse : responses) {
                try {
                    XMLDocument anXMLDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8
                            ,
                            new StringReader(aResponse));
                    Advertisement anAdv = AdvertisementFactory.newAdvertisement(anXMLDoc);
                    
                    advertisements.add(anAdv);
                } catch (IOException badAdvertisement) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Invalid response in message : ", badAdvertisement);
                    }
                }
            }
        }
        
        return Collections.enumeration(advertisements);
    }
}
