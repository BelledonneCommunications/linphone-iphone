/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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
import net.jxta.document.MimeMediaType;
import net.jxta.id.ID;
import net.jxta.peer.PeerID;
import net.jxta.pipe.PipeID;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;


/**
 * This abstract class defines the PipeResolver Message.
 * <p/>
 * <p/>This message is part of the Pipe Resolver Protocol.
 *
 * @see net.jxta.pipe.PipeService
 * @see net.jxta.protocol.PipeAdvertisement
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pbp" target='_blank'>JXTA Protocols Specification : Standard JXTA Protocols</a>
 */
public abstract class PipeResolverMessage {

    /**
     * The type of message this object is communicating.
     */
    private MessageType msgType = null;

    /**
     * The pipe which is the subject of this message.
     */
    private ID pipeid = ID.nullID;

    /**
     * The type of the pipe which is the subject of this message.
     */
    private String pipeType = null;

    /**
     * <ul>
     * <li>For query : The peer ids which should respond to this query.</li>
     * <li>For response : The peer id on which is responding.</li>
     * </ul>
     */
    private Set<ID> peerids = new HashSet<ID>();

    /**
     * For responses, if true then this message indicates the pipe is present
     * otherwise the pipe is not present.
     */
    private boolean found = true;

    /**
     * <ul>
     * <li>For query : The peer advertisement of the querying peer.</li>
     * <li>For response : The peer advertisement of the responding peer.</li>
     * </ul>
     */
    private PeerAdvertisement inputPeerAdv = null;

    /**
     * An enumeration class for message types.
     */
    public enum MessageType {

        /**
         * A query message
         */
        QUERY, /**
         * A response message
         */ ANSWER
    }

    /**
     * Creates a new unintialized pipe resolver message
     */
    public PipeResolverMessage() {
        super();
    }

    /**
     * returns the Message type. This will match the XML doctype declaration.
     *
     * @return a string
     */
    public static String getMessageType() {
        return "jxta:PipeResolver";
    }

    /**
     * Write message into a document. asMimeType is a mime media-type
     * specification and provides the form of the document which is being
     * requested. Two standard document forms are defined. "text/plain" encodes
     * the document in "pretty-print" format for human viewing and "text/xml"
     * which provides an XML format.
     *
     * @param asMimeType MimeMediaType format representation requested
     * @return Document   the document to be used in the construction
     */
    public abstract Document getDocument(MimeMediaType asMimeType);

    /**
     * Returns whether this message is a query or a response.
     *
     * @return the type of this message.
     */
    public MessageType getMsgType() {
        return msgType;
    }

    /**
     * Sets the message type of this message.
     *
     * @param type the type this message is to be.
     */
    public void setMsgType(MessageType type) {
        msgType = type;
    }

    /**
     * Return the id of the pipe which is the subject of this message.
     *
     * @return the id of the pipe which is the subject of this message.
     */
    public ID getPipeID() {
        return pipeid;
    }

    /**
     * Set the id of pipe which is to be subject of this message.
     *
     * @param id the pipe id which is the subject of this message.
     */
    public void setPipeID(ID id) {

        if (!(id instanceof PipeID)) {
            throw new IllegalArgumentException("can only set to pipe ids.");
        }

        pipeid = id;
    }

    /**
     * Return the pipe type of the pipe which is the subject of this message.
     *
     * @return the pipe type of the pipe which is the subject of this message.
     */
    public String getPipeType() {
        return pipeType;
    }

    /**
     * Set the pipe type of the pipe which is the subject of this message.
     *
     * @param type The pipe type of the pipe which is to be the subject of this
     *             message.
     */
    public void setPipeType(String type) {
        pipeType = type;
    }

    // Query

    /**
     * Returns a {@link java.util.Set} (possibly empty) containing the peer ids
     * which should respond to this query.
     *
     * @return set containing the peer ids to which this peer is directed.
     */
    public Set<ID> getPeerIDs() {

        return Collections.unmodifiableSet(peerids);
    }

    /**
     * Add a peer to the set of peers to which this query is directed.
     *
     * @param id the peer id to add.
     */
    public void addPeerID(ID id) {

        if (!(id instanceof PeerID)) {
            throw new IllegalArgumentException("can only add peer ids");
        }

        peerids.add(id);
    }

    // Answer

    /**
     * If true then the pipe was found ont he
     *
     * @return true if found
     */
    public boolean isFound() {
        return found;
    }

    public void setFound(boolean isFound) {
        found = isFound;
    }

    public PeerAdvertisement getInputPeerAdv() {
        return inputPeerAdv;
    }

    public void setInputPeerAdv(PeerAdvertisement peerAdv) {
        inputPeerAdv = peerAdv;
    }
}
