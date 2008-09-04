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

package net.jxta.impl.proxy;


import net.jxta.document.Advertisement;
import net.jxta.endpoint.*;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.IOException;


public class Requestor {
    private final static Logger LOG = Logger.getLogger(Requestor.class.getName());

    private PeerGroup group;
    private EndpointAddress address;
    private MessageElement requestId;
    private Messenger messenger;
    private int threshold = 1;

    public boolean send(Message message) {
        int count;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("send to " + address.toString());
        }

        try {
            synchronized (this) {
                if ((null == messenger) || messenger.isClosed()) {
                    messenger = null;
                    count = 0;
                    // Add a retry in case we did not obtain a new messenger.
                    // Due to the heavy polling of the client, it seems that
                    // we can run in a race condition where we don't get
                    // a new messenger.
                    while (count < 2 && messenger == null) {
                        messenger = group.getEndpointService().getMessengerImmediate(address, null);
                        if (messenger != null) {
                            break;
                        }
                        try {
                            Thread.sleep(500);
                        } catch (InterruptedException e) {
                            Thread.interrupted();
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Retry getting a messenger" + e);
                            }
                        }
                        count++;
                    }

                    if (null == messenger) {
                        LOG.warning("Could not get messenger for " + address);
                        return false;
                    }
                }
            }
            messenger.sendMessage(message);
        } catch (IOException e) {
            LOG.log(Level.WARNING, "Could not send message to requestor for " + address, e);
            return false;
        }

        ProxyService.logMessage(message, LOG);

        return true;
    }

    public boolean send(Advertisement adv, String resultType) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("send " + adv);
        }

        Message message = new Message();

        if (resultType == null) {
            resultType = "";
        }
        setString(message, ProxyService.RESPONSE_TAG, resultType);

        if (requestId != null) {
            message.addMessageElement(ProxyService.PROXYNS, requestId);
        }

        if (adv instanceof PeerAdvertisement) {
            PeerAdvertisement peerAdv = (PeerAdvertisement) adv;

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.TYPE_TAG, ProxyService.TYPE_PEER, null));

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.NAME_TAG, peerAdv.getName(), null));

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.ID_TAG, peerAdv.getPeerID().toString(), null));

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("send PeerAdvertisement name=" + peerAdv.getName() + " id=" + peerAdv.getPeerID().toString());
            }

        } else if (adv instanceof PeerGroupAdvertisement) {
            PeerGroupAdvertisement groupAdv = (PeerGroupAdvertisement) adv;

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.TYPE_TAG, ProxyService.TYPE_GROUP, null));

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.NAME_TAG, groupAdv.getName(), null));

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.ID_TAG, groupAdv.getPeerGroupID().toString(), null));

            LOG.fine("send GroupAdvertisement name=" + groupAdv.getName() + " id=" + groupAdv.getPeerGroupID().toString());

        } else if (adv instanceof PipeAdvertisement) {
            PipeAdvertisement pipeAdv = (PipeAdvertisement) adv;

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.TYPE_TAG, ProxyService.TYPE_PIPE, null));

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.NAME_TAG, pipeAdv.getName(), null));

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.ID_TAG, pipeAdv.getPipeID().toString(), null));

            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.ARG_TAG, pipeAdv.getType(), null));

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        "send PipeAdvertisement name=" + pipeAdv.getName() + " id=" + pipeAdv.getPipeID().toString() + " arg="
                        + pipeAdv.getType());
            }

        } else {
            return false;
        }

        return send(message);
    }

    public boolean notifySuccess() {
        LOG.fine("notifySuccess");

        Message message = new Message();

        message.addMessageElement(ProxyService.PROXYNS
                ,
                new StringMessageElement(ProxyService.RESPONSE_TAG, ProxyService.RESPONSE_SUCCESS, null));

        if (requestId != null) {
            message.addMessageElement(ProxyService.PROXYNS, requestId);
        }

        return send(message);
    }

    public boolean notifyError(String errorString) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("notifyError " + errorString);
        }

        Message message = new Message();

        if (requestId != null) {
            message.addMessageElement(ProxyService.PROXYNS, requestId);
        }

        if (errorString != null && errorString.length() > 0) {
            message.addMessageElement(ProxyService.PROXYNS
                    ,
                    new StringMessageElement(ProxyService.ERROR_MESSAGE_TAG, errorString, null));
        }

        return send(message);
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object obj) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(this + " equals " + obj);
        }

        if (obj instanceof Requestor) {
            Requestor dest = (Requestor) obj;

            if (address != null && dest.address != null) {
                if (dest.address.toString().equals(address.toString())) {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public int hashCode() {
        int result = 17;

        return 37 * result + requestId.hashCode();
    }

    /**
     *  {@inheritDoc}
     */
    @Override    
    public String toString() {
        return "Requestor " + address.toString();
    }

    private Requestor(PeerGroup group, EndpointAddress address, MessageElement requestId) throws IOException {
        this.group = group;
        this.address = address;
        this.requestId = requestId;
    }

    public static Requestor createRequestor(PeerGroup group, Message message, EndpointAddress address, int threshold) throws IOException {
        Requestor requestor;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("create new Requestor - " + address.toString());
        }

        MessageElement elem = message.getMessageElement(ProxyService.REQUESTID_TAG);

        requestor = new Requestor(group, address, elem);
        requestor.setThreshold(threshold);
        message.removeMessageElement(elem);
        return requestor;
    }

    void setThreshold(int threshold) {
        this.threshold = threshold;
    }

    int getThreshold() {
        return threshold;
    }

    private void setString(Message message, String tag, String value) {
        StringMessageElement sme = new StringMessageElement(tag, value, null);

        message.addMessageElement(ProxyService.PROXYNS, sme);
    }
}
