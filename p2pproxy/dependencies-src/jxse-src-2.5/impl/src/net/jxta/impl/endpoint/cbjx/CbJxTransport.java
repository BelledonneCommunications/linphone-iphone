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

package net.jxta.impl.endpoint.cbjx;


import net.jxta.document.Advertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.TextDocument;
import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.MessageFilterListener;
import net.jxta.endpoint.MessageReceiver;
import net.jxta.endpoint.MessageSender;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.endpoint.WireFormatMessage;
import net.jxta.endpoint.WireFormatMessageFactory;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.endpoint.JxtaMessageMessageElement;
import net.jxta.impl.membership.pse.PSECredential;
import net.jxta.impl.membership.pse.PSEMembershipService;
import net.jxta.impl.membership.pse.PSEUtils;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.ModuleImplAdvertisement;

import java.io.IOException;
import java.security.cert.Certificate;
import java.security.interfaces.RSAPublicKey;
import java.util.Collections;
import java.util.Iterator;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * A JXTA {@link net.jxta.endpoint.MessageTransport} implementation which
 * which provides message verification by examining message signatures. A
 * virtual transport, the messages are transfered between peers using some
 * other message transport.
 */
public class CbJxTransport implements Module, MessageSender, MessageReceiver, EndpointListener {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(CbJxTransport.class.getName());

    /**
     * the name of the cbjx valid element
     */
    public static final String CBJX_MSG_NS = "cbjx";

    /**
     * the name of the cbjx crypto element
     */
    static final String CBJX_MSG_INFO = "CryptoInfo";

    /**
     * the name of the cbjx body element
     */
    static final String CBJX_MSG_BODY = "Body";

    /**
     * the name of the cbjx body element
     */
    static final String CBJX_MSG_SIG = "Signature";

    /**
     * the cbjx protocol name
     */
    static final String cbjxProtocolName = "cbjx";

    /**
     * the cbjx service name
     */
    static final String cbjxServiceName = "CbJxTransport";

    /**
     * the local peer ID
     */
    static PeerID localPeerID = null;

    /**
     * the endpoint address of this peer
     */
    static EndpointAddress localPeerAddr = null;

    /**
     * the peer group to which this module belongs
     */
    PeerGroup group = null;

    /**
     * the endpoint service in my group
     */
    EndpointService endpoint = null;

    /**
     * the membership service in my group
     */
    PSEMembershipService membership = null;

    /**
     * Default constructor
     */
    public CbJxTransport() {}

    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {
        this.group = group;

        ModuleImplAdvertisement implAdvertisement = (ModuleImplAdvertisement) impl;

        localPeerID = group.getPeerID();

        CbJxTransport.localPeerAddr = new EndpointAddress(cbjxProtocolName, group.getPeerID().getUniqueValue().toString(), null
                ,
                null);

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring CBJX Message Transport : " + assignedID);

            if (implAdvertisement != null) {
                configInfo.append("\n\tImplementation :");
                configInfo.append("\n\t\tModule Spec ID: ").append(implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description : ").append(implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI : ").append(implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code : ").append(implAdvertisement.getCode());
            }

            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tGroup : ").append(group);
            configInfo.append("\n\t\tPeer ID : ").append(group.getPeerID());

            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tPublic Address : ").append(CbJxTransport.localPeerAddr);

            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public int startApp(String[] arg) {

        endpoint = group.getEndpointService();

        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }
            return START_AGAIN_STALLED;
        }

        MembershipService groupMembership = group.getMembershipService();

        if (null == groupMembership) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a membership service");
            }
            return START_AGAIN_STALLED;
        }

        if (!(groupMembership instanceof PSEMembershipService)) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("CBJX Transport requires PSE Membership Service");
            }
            return -1;
        }

        membership = (PSEMembershipService) groupMembership;

        if (endpoint.addMessageTransport(this) == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Transport registration refused");
            }
            return -1;
        }

        // XXX bondolo@jxta.org 20030526 check for errors

        endpoint.addIncomingMessageListener(this, cbjxServiceName, null);

        endpoint.addIncomingMessageFilterListener(new CbJxInputFilter(), null, null);
        // endpoint.addOutgoingMessageFilterListener( new CbJxOutputFilter(), null, null );

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("CbJxTransport started");
        }

        return 0;
    }

    /**
     * {@inheritDoc}
     */
    public void stopApp() {

        if (endpoint != null) {
            // FIXME 20030516 bondolo@jxta.org remove filters and listener

            endpoint.removeMessageTransport(this);
            endpoint = null;
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("CbJxTransport stopped");
        }
    }

    /**
     * {@inheritDoc}
     */
    public EndpointAddress getPublicAddress() {
        return CbJxTransport.localPeerAddr;
    }

    /**
     * {@inheritDoc}
     */
    public boolean isConnectionOriented() {
        // since we rely on other endpoint protocol we are not connection oriented
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public boolean allowsRouting() {
        // since we are using the endpoint router
        // the endpoint router cannot use our endpoint to send messages
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        return (EndpointService) endpoint.getInterface();
    }

    /**
     * {@inheritDoc}
     */
    public Object transportControl(Object operation, Object value) {
        return null;
    }

    /**
     * {@inheritDoc}
     */
    public Iterator getPublicAddresses() {
        return Collections.singletonList(getPublicAddress()).iterator();
    }

    /**
     * {@inheritDoc}
     */
    public String getProtocolName() {
        return cbjxProtocolName;
    }

    /**
     * {@inheritDoc}
     */
    public Messenger getMessenger(EndpointAddress dest, Object hintIgnored) {
        try {
            return new CbJxMessenger(this, dest, hintIgnored);
        } catch (IOException failed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to create cbjx messenger", failed);
            }
            return null;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    public boolean ping(EndpointAddress addr) {
        Messenger messenger = getMessenger(addr, null);

        boolean reachable = (null != messenger);

        if (messenger != null) {
            messenger.close();
        }

        return reachable;
    }

    /**
     * {@inheritDoc}
     */
    public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("processIncomingMessage : Received message from: " + srcAddr);
        }

        // extract the Crypto info from the message
        MessageElement cryptoElement = message.getMessageElement(CBJX_MSG_NS, CBJX_MSG_INFO);

        if (cryptoElement == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("processIncomingMessage : No \'" + CBJX_MSG_INFO + "\' in the message");
            }
            return;
        }
        message.removeMessageElement(cryptoElement);

        // the cbjx message info
        CbJxMessageInfo cryptoInfo = null;

        try {
            cryptoInfo = new CbJxMessageInfo(cryptoElement.getStream(), cryptoElement.getMimeType());
        } catch (Throwable e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING
                        ,
                        "processIncomingMessage : Couldn\'t retrieve CbJxMessageInfo from \'" + CBJX_MSG_INFO + "\' element", e);
            }
            return;
        }

        Message submessage = checkCryptoInfo(message, cryptoElement, cryptoInfo);

        if (null == submessage) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("processIncomingMessage : discarding message from " + srcAddr);
            }
            return;
        }

        // give back the message to the endpoint
        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("processIncomingMessage: delivering " + submessage + " to: " + cryptoInfo.getDestinationAddress());
            }

            endpoint.processIncomingMessage(submessage, cryptoInfo.getSourceAddress(), cryptoInfo.getDestinationAddress());
        } catch (Throwable all) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "processIncomingMessage: endpoint failed to demux message", all);
            }
        }
    }

    /**
     * add the CryptoInfo into the message
     *
     * @param submessage  the message
     * @param destAddress the destination
     * @return Message the message with the CbJxMessageInfo added
     */
    public Message addCryptoInfo(Message submessage, EndpointAddress destAddress) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Building CBJX wrapper for " + submessage);
        }

        // Remove all existing CbJx Elements from source
        Iterator eachCbJxElement = submessage.getMessageElementsOfNamespace(CbJxTransport.CBJX_MSG_NS);

        while (eachCbJxElement.hasNext()) {
            MessageElement aMessageElement = (MessageElement) eachCbJxElement.next();

            eachCbJxElement.remove();
        }

        Message message = new Message();

        CbJxMessageInfo cryptoInfo = new CbJxMessageInfo();

        // set the source Id of the message
        cryptoInfo.setSourceID(localPeerID);
        cryptoInfo.setSourceAddress(localPeerAddr);
        cryptoInfo.setDestinationAddress(destAddress);

        // add the root cert into the message info
        PSECredential cred = (PSECredential) membership.getDefaultCredential();

        if (null == cred) {
            throw new IOException("No authentication available for message signing.");
        }

        Certificate cert = cred.getCertificate();

        cryptoInfo.setPeerCert(cert);

        // compute the signature of the message body
        TextDocument infoDoc = (TextDocument) cryptoInfo.getDocument(MimeMediaType.XMLUTF8);
        byte[] infoSignature = null;

        try {
            infoSignature = PSEUtils.computeSignature(CbJxDefs.signAlgoName, cred.getPrivateKey(), infoDoc.getStream());
        } catch (Throwable e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "failed to sign " + submessage, e);
            }
            return null;
        }

        // add the cbjx:CryptoInfo into the message
        MessageElement infoSigElement = new ByteArrayMessageElement(CBJX_MSG_SIG, MimeMediaType.AOS, infoSignature, null);

        // add the cbjx:CryptoInfo into the message
        MessageElement cryptoInfoElement = new TextDocumentMessageElement(CBJX_MSG_INFO, infoDoc, infoSigElement);

        message.addMessageElement(CBJX_MSG_NS, cryptoInfoElement);

        // Compute the signature of the encapsulated message and append it to
        // the container.

        // serialize the container
        WireFormatMessage subserial = WireFormatMessageFactory.toWire(submessage, WireFormatMessageFactory.DEFAULT_WIRE_MIME, null);

        // calculate the signature
        byte[] bodySignature = null;

        try {
            bodySignature = PSEUtils.computeSignature(CbJxDefs.signAlgoName, cred.getPrivateKey(), subserial.getStream());
        } catch (Throwable e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "failed to sign" + submessage, e);
            }
            return null;
        }

        subserial = null;

        // Make the signature into an element
        MessageElement bodySigElement = new ByteArrayMessageElement(CBJX_MSG_SIG, MimeMediaType.AOS, bodySignature, null);

        // Add the encapsulated body into the container message.
        message.addMessageElement(CBJX_MSG_NS
                ,
                new JxtaMessageMessageElement(CBJX_MSG_BODY, new MimeMediaType("application/x-jxta-msg"), submessage
                ,
                bodySigElement));

        return message;
    }

    public Message checkCryptoInfo(Message message, MessageElement cryptoElement, CbJxMessageInfo cryptoInfo) {

        // extract the body element  from the message
        JxtaMessageMessageElement bodyElement = (JxtaMessageMessageElement) message.getMessageElement(CBJX_MSG_NS, CBJX_MSG_BODY);

        if (null == bodyElement) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("No \'" + CBJX_MSG_BODY + "\' in " + message);
            }
            return null;
        }
        message.removeMessageElement(bodyElement);

        // extract the peer certificate
        Certificate peerCert = cryptoInfo.getPeerCert();

        // and from it the public key
        // the public key from the message
        RSAPublicKey publicKey = (RSAPublicKey) peerCert.getPublicKey();

        // check the cert validity
        try {
            peerCert.verify(publicKey);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Invalid peer cert", e);
            }
            return null;
        }

        // check the cbid
        try {
            net.jxta.impl.id.CBID.PeerID srcPeerID = (net.jxta.impl.id.CBID.PeerID) cryptoInfo.getSourceID();

            byte[] pub_der = peerCert.getPublicKey().getEncoded();
            net.jxta.impl.id.CBID.PeerID genID = (net.jxta.impl.id.CBID.PeerID) IDFactory.newPeerID(group.getPeerGroupID()
                    ,
                    pub_der);

            if (!srcPeerID.getUUID().equals(genID.getUUID())) {
                // the cbid is not valid. Discard the message
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("CBID of " + message + " is not valid : " + srcPeerID + " != " + genID);
                }
                return null;
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("CBID of the message is valid");
            }
        } catch (Throwable e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to verify cbid", e);
            }
            return null;
        }

        // verify the signature of the cryptinfo message
        try {
            boolean valid = PSEUtils.verifySignature(CbJxDefs.signAlgoName, peerCert, cryptoElement.getSignature().getBytes(false)
                    ,
                    cryptoElement.getStream());

            if (!valid) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Failed to verify the signature of cryptinfo for " + message);
                }
                return null;
            }
        } catch (Throwable e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to verify the signature of cryptinfo for " + message, e);
            }
            return null;
        }

        // then verify the signature
        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
            LOG.warning("verifying signature");
        }

        // verify the signature of the message
        try {
            boolean valid = PSEUtils.verifySignature(CbJxDefs.signAlgoName, peerCert, bodyElement.getSignature().getBytes(false)
                    ,
                    bodyElement.getStream());

            if (!valid) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("failed to verify the signature of " + message);
                }
                return null;
            }
        } catch (Throwable e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to verify the signature of " + message, e);
            }
            return null;
        }

        // the message is valid
        return bodyElement.getMessage();
    }

    /**
     * this class filters incoming messages.
     * it checks if messages are valid and if not discard them
     */
    public class CbJxInputFilter implements MessageFilterListener {
        public CbJxInputFilter() {
            super();
        }

        /**
         * {@inheritDoc}
         */
        public Message filterMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {

            if (dstAddr.getProtocolAddress().equals(getProtocolName())) {
                // extract the Crypto info from the message
                MessageElement cryptoElement = message.getMessageElement(CBJX_MSG_NS, CBJX_MSG_INFO);

                if (cryptoElement == null) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("No \'" + CBJX_MSG_INFO + "\' in the message");
                    }
                    return null;
                }
                message.removeMessageElement(cryptoElement);

                // the cbjx message info
                CbJxMessageInfo cryptoInfo = null;

                try {
                    cryptoInfo = new CbJxMessageInfo(cryptoElement.getStream(), cryptoElement.getMimeType());
                } catch (Throwable e) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Couldn\'t retrieve CbJxMessageInfo from \'" + CBJX_MSG_INFO + "\' element", e);
                    }
                    return null;
                }

                return checkCryptoInfo(message, cryptoElement, cryptoInfo);
            }

            return message;
        }
    }


    /**
     * this class filters all outgoing messages that are not sent with
     * messengers. (that is propagate messages). It adds CbJxInformation
     * into to messages.
     */
    public class CbJxOutputFilter implements MessageFilterListener {

        /**
         * Default constructor
         */
        public CbJxOutputFilter() {
            super();
        }

        /**
         * {@inheritDoc}
         */
        public Message filterMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {
            Message msg = message.clone();

            if (null == msg.getMessageElement(CBJX_MSG_NS, CBJX_MSG_INFO)) {
                try {
                    msg = addCryptoInfo(msg, dstAddr);
                } catch (IOException failed) {
                    return null;
                }
            }

            return msg;
        }
    }
}
