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


import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.peer.PeerID;


/**
 * This abstract class define the PeerInfoService advertisement.
 */

public abstract class PeerInfoResponseMessage {
    
    /**
     * The Sender's (or Querying) Peer Id.
     */
    private PeerID spid = null;
    
    /**
     * Peer Id of the peer which published this advertisement.
     */
    private PeerID tpid = null;
    
    /** String representing response  */
    private StructuredDocument response = null;

    /**
     * Time in seconds since this peer was started.
     */
    private long uptime = 0;
    
    /**
     * Time in milliseconds when this peer was last polled since
     * "the epoch", namely January 1, 1970, 00:00:00 GMT.
     */
    private long timestamp = 0;
 
    public PeerInfoResponseMessage() {
        super();
    }
    
    /**
     * returns the Message type
     * @return a string
     *
     */
    public static String getMessageType() {
        return "jxta:PeerInfoResponseMessage";
    }
    
    /**
     * returns the sender's pid
     *
     * @return a string representing the peer's id
     *
     */
    public PeerID getSourcePid() {
        return spid;
    }
    
    /**
     * sets the sender's pid
     *
     * @param pid a string representing a peer's id
     *
     */
    public void setSourcePid(PeerID pid) {
        this.spid = pid;
    }
    
    /**
     * returns the target pid
     *
     * @return a string representing the peer's id
     *
     */
    public PeerID getTargetPid() {
        return tpid;
    }
    
    /**
     * sets the target's pid
     *
     * @param pid a string representing a peer's id
     *
     */
    public void setTargetPid(PeerID pid) {
        this.tpid = pid;
    }

    /**
     * returns the response
     *
     * @return a structured document representing request
     *
     */
    public Element getResponse() {
        if (null != response) {
            return StructuredDocumentUtils.copyAsDocument(response);
        } else {
            return null;
        }
    }
    
    /**
     * sets the request
     *
     * @param response a structured document representing a peerinfo request
     *
     */
    public void setResponse(Element response) {
        if (null != response) {
            this.response = StructuredDocumentUtils.copyAsDocument(response);
        } else {
            this.response = null;
        }
    }

    /**
     * returns the number of milliseconds since this peer was started
     * @return the number of milliseconds
     *
     */
    public long getUptime() {
        return uptime;
    }
    
    /**
     * sets the number of milliseconds since this peer was started
     * @param milliseconds the number of milliseconds since this peer was started
     *
     */
    public void setUptime(long milliseconds) {
        this.uptime = milliseconds;
    }
    
    /**
     * returns the time when this peer was last polled
     *
     * @return long time in milliseconds when this peer was last polled in
     * milliseconds since "the epoch", namely January 1, 1970, 00:00:00
     * GMT.
     *
     */
    public long getTimestamp() {
        return timestamp;
    }
    
    /**
     * sets the time when this peer was last polled
     *
     * @param milliseconds time in milliseconds when this peer was last polled
     * in milliseconds since "the epoch", namely January 1, 1970, 00:00:00
     * GMT.
     *
     */
    public void setTimestamp(long milliseconds) {
        this.timestamp = milliseconds;
    }

    public abstract Document getDocument(MimeMediaType encodeAs);
  
}

