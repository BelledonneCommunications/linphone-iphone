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

package net.jxta.impl.protocol;


import net.jxta.document.*;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.pipe.PipeID;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeResolverMessage;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.IOException;
import java.io.StringReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.Set;


/**
 * This class implements {@link net.jxta.protocol.PipeResolverMessage} by
 * providing {@link #initialize(Element)} and {@link #getDocument(MimeMediaType)}
 * implementations.
 * <p/>
 * <p/>It implements the PipeResolver message for the standard Pipe
 * Binding Protocol (PBP) with the following schema:
 * <p/>
 * <pre><code>
 * &lt;xs:element name="jxta:PipeResolver" type="jxta:PipeResolver"/>
 * <p/>
 * &lt;xs:simpleType name="PipeResolverMsgType">
 *   &lt;xs:restriction base="xs:string">
 *     &lt;!-- QUERY -->
 *     &lt;xs:enumeration value="Query"/>
 *     &lt;!-- ANSWER -->
 *     &lt;xs:enumeration value="Answer"/>
 *   &lt;/xs:restriction>
 * &lt;/xs:simpleType>
 * <p/>
 * &lt;xs:complexType name="PipeResolver">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="MsgType" type="jxta:PipeResolverMsgType"/>
 *     &lt;xs:element name="PipeId" type="jxta:JXTAID"/>
 * &lt;xs:element name="Type" type="xs:string"/>
 * <p/>
 *     &lt;!-- used in the query -->
 * &lt;xs:element name="Cached" type="xs:boolean" default="true" minOccurs="0"/>
 *     &lt;xs:element name="Peer" type="jxta:JXTAID" minOccurs="0" maxOccurs="unbounded"/>
 * <p/>
 *     &lt;!-- used in the answer -->
 *     &lt;xs:element name="Found" minOccurs="0" type="xs:boolean"/>
 * &lt;!-- This should refer to a peer adv, but is instead a whole doc -->
 * &lt;xs:element name="PeerAdv" minOccurs="0" type="xs:string" />
 *   &lt;/xs:sequence>
 * &lt;/xs:complexType>
 * </code></pre>
 *
 * @see net.jxta.pipe.PipeService
 * @see net.jxta.impl.pipe.PipeServiceImpl
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pbp" target="_blank">JXTA Protocols Specification : Pipe Binding Protocol</a>
 */
public class PipeResolverMsg extends PipeResolverMessage {

    /**
     * Log4J Logger
     */
    private final static transient Logger LOG = Logger.getLogger(PipeResolverMsg.class.getName());

    private final static String MsgTypeTag = "MsgType";
    private final static String PipeIdTag = "PipeId";
    private final static String PipeTypeTag = "Type";
    private final static String PeerIdTag = "Peer";
    private final static String PeerAdvTag = "PeerAdv";
    private final static String FoundTag = "Found";

    private final static String QueryMsgType = "Query";
    private final static String AnswerMsgType = "Answer";

    public PipeResolverMsg() {}

    public PipeResolverMsg(Element root) {
        initialize(root);
    }

    private void initialize(Element root) {
        if (!TextElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports TextElement");
        }

        TextElement doc = (TextElement) root;

        String docName = doc.getName();

        if (!docName.equals(getMessageType())) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + docName);
        }

        Enumeration each = doc.getChildren();

        while (each.hasMoreElements()) {
            TextElement elem = (TextElement) each.nextElement();

            if (elem.getName().equals(MsgTypeTag)) {
                String msgtype = elem.getTextValue();

                if (msgtype.equals(QueryMsgType)) {
                    setMsgType(PipeResolverMessage.MessageType.QUERY);
                } else if (msgtype.equals(AnswerMsgType)) {
                    setMsgType(PipeResolverMessage.MessageType.ANSWER);
                } else {
                    throw new IllegalArgumentException("Unexpected Message Type in parsing.");
                }
                continue;
            }

            if (elem.getName().equals(PipeIdTag)) {
                try {
                    URI pipeID = new URI(elem.getTextValue());

                    setPipeID(IDFactory.fromURI(pipeID));
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("Bad pipe ID in message");
                }
                continue;
            }

            if (elem.getName().equals(PipeTypeTag)) {
                setPipeType(elem.getTextValue());
                continue;
            }

            if (elem.getName().equals(PeerIdTag)) {
                try {
                    URI peerID = new URI(elem.getTextValue());

                    addPeerID(IDFactory.fromURI(peerID));
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("Bad peer ID in message");
                }
                continue;
            }

            if (elem.getName().equals(FoundTag)) {
                setFound(Boolean.valueOf(elem.getTextValue()));
                continue;
            }

            // let's check whether the responder sent us a adv
            if (elem.getName().equals(PeerAdvTag)) {
                String peerAdv = elem.getTextValue();

                try {
                    setInputPeerAdv(
                            (PeerAdvertisement) AdvertisementFactory.newAdvertisement(MimeMediaType.XMLUTF8
                            ,
                            new StringReader(peerAdv)));
                } catch (IOException caught) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Malformed peer adv in message", caught);
                    }
                    throw new IllegalArgumentException("Malformed peer adv in message : " + caught.getMessage());
                }
            }

        }

        // Begin validation
        PipeResolverMessage.MessageType msgType = getMsgType();

        if (null == msgType) {
            throw new IllegalArgumentException("Message type was never set!");
        }

        ID pipeID = getPipeID();

        if ((null == pipeID) || ID.nullID.equals(pipeID) || !(pipeID instanceof PipeID)) {
            throw new IllegalArgumentException("Input Pipe ID not set or invalid");
        }

        if (null == getPipeType()) {
            throw new IllegalArgumentException("Pipe type was never set!");
        }

        // Response extra checks
        if (PipeResolverMessage.MessageType.ANSWER.equals(msgType)) {
            if (getPeerIDs().isEmpty()) {
                throw new IllegalArgumentException("An answer without responses is invalid");
            }
        }
    }

    /**
     * Creates a document out of the message.
     *
     * @param encodeAs The document representation format requested.
     * @return the message as a document.
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredTextDocument doc = (StructuredTextDocument)
                StructuredDocumentFactory.newStructuredDocument(encodeAs, getMessageType());

        if (doc instanceof Attributable) {
            ((Attributable) doc).addAttribute("xmlns:jxta", "http://jxta.org");
        }

        PipeResolverMessage.MessageType msgType = getMsgType();

        if (null == msgType) {
            throw new IllegalStateException("Message type was never set!");
        }

        ID pipeID = getPipeID();

        if ((null == pipeID) || ID.nullID.equals(pipeID) || !(pipeID instanceof PipeID)) {
            throw new IllegalStateException("Pipe ID not set or invalid.");
        }

        String pipeType = getPipeType();

        if ((null == pipeType) || (0 == pipeType.trim().length())) {
            throw new IllegalStateException("Pipe type was never set or is invalid.");
        }

        Element element;

        if (PipeResolverMessage.MessageType.QUERY.equals(msgType)) {
            element = doc.createElement(MsgTypeTag, QueryMsgType);
        } else if (PipeResolverMessage.MessageType.ANSWER.equals(msgType)) {
            element = doc.createElement(MsgTypeTag, AnswerMsgType);
        } else {
            throw new IllegalStateException("Unknown message type :" + msgType.toString());
        }
        doc.appendChild(element);

        element = doc.createElement(PipeIdTag, pipeID.toString());
        doc.appendChild(element);

        if ((pipeType != null) && (0 != pipeType.length())) {
            element = doc.createElement(PipeTypeTag, pipeType);
            doc.appendChild(element);
        }

        // Write the peer ids.
        Set peers = getPeerIDs();

        if (PipeResolverMessage.MessageType.ANSWER.equals(msgType) && peers.isEmpty()) {
            throw new IllegalStateException("An ANSWER message must contain at least one peer as part of the response.");
        }

        for (Object peer : peers) {
            ID aPeer = (ID) peer;

            element = doc.createElement(PeerIdTag, aPeer.toString());
            doc.appendChild(element);
        }

        if (PipeResolverMessage.MessageType.QUERY.equals(msgType)) {// nothing for now...
        } else if (PipeResolverMessage.MessageType.ANSWER.equals(msgType)) {
            element = doc.createElement(FoundTag, (isFound() ? Boolean.TRUE : Boolean.FALSE).toString());
            doc.appendChild(element);

            PeerAdvertisement peerAdv = getInputPeerAdv();

            if (peerAdv != null) {
                if (!peers.contains(peerAdv.getPeerID())) {
                    throw new IllegalStateException(
                            "Provided Peer Advertisement does not refer to one of the peers in the response list.");
                }

                StructuredTextDocument asDoc = (StructuredTextDocument) peerAdv.getDocument(MimeMediaType.XMLUTF8);

                element = doc.createElement(PeerAdvTag, asDoc.toString());
                doc.appendChild(element);
            }
        } else {
            throw new IllegalStateException("Unknown message type :" + msgType.toString());
        }
        return doc;
    }
}

