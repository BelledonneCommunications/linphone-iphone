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
package net.jxta.impl.rendezvous.limited;

import java.io.IOException;

import java.util.logging.Level;

import net.jxta.logging.Logging;

import java.util.logging.Logger;

import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;

import net.jxta.impl.protocol.LimitedRangeRdvMsg;
import net.jxta.impl.rendezvous.RdvGreeter;
import net.jxta.impl.rendezvous.rpv.PeerViewElement;

/**
 * The limited range rendezvous peer greeter.
 *
 * @see net.jxta.impl.rendezvous.RdvGreeter
 * @see net.jxta.impl.rendezvous.RdvWalk
 * @see net.jxta.impl.rendezvous.RdvWalker
 * @see net.jxta.impl.protocol.LimitedRangeRdvMsg
 */
public class LimitedRangeGreeter implements EndpointListener, RdvGreeter {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(LimitedRangeGreeter.class.getName());

    /**
     * The walk we are associated with.
     */
    private final LimitedRangeWalk walk;

    /**
     * XXX It would be nice to avoid making another link to the endpoint.
     */
    private final EndpointService endpoint;

    /**
     * Constructor
     *
     * @param walk The walk we will be associated with.
     */
    public LimitedRangeGreeter(LimitedRangeWalk walk) {
        this.walk = walk;

        this.endpoint = walk.getPeerGroup().getEndpointService();

        if (!endpoint.addIncomingMessageListener(this, walk.getWalkServiceName(), walk.getWalkServiceParam())) {
            throw new IllegalStateException("Could not register endpoint listener for greeter.");
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Listening on " + walk.getWalkServiceName() + "/" + walk.getWalkServiceParam());
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void stop() {
        endpoint.removeIncomingMessageListener(walk.getWalkServiceName(), walk.getWalkServiceParam());
    }

    /**
     * {@inheritDoc}
     * <p/>
     *  Listens on "LR-Greeter"&lt;groupid>/&lt;walkSvc>&lt;walkParam>
     * <p/>
     * Currently, all this method has to do, is to invoke the upper layer.
     */
    public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Processing " + message + " from " + srcAddr);
        }

        LimitedRangeRdvMsg rdvMsg = LimitedRangeWalk.getRdvMessage(message);

        // Check and update the Limited Range Rdv Message
        if (null == rdvMsg) {
            // Message is invalid, drop it
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Limited Range Greeter received invalid " + message + ". Dropping it.");
            }
            return;
        }

        if (rdvMsg.getTTL() <= 0) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("No TTL remaining for " + message + ". Dropping it.");
            }
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Limited Range Greeter calling listener");
        }

        try {
            walk.getListener().processIncomingMessage(message, srcAddr, dstAddr);
        } catch (Throwable ignored) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in listener (" + walk.getListener() + ")", ignored);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void replyMessage(Message msg, Message reply) throws IOException {
        LimitedRangeRdvMsg rdvMsg = LimitedRangeWalk.getRdvMessage(msg);

        if (rdvMsg == null) {
            // No RdvMessage. This message was not received by this Greeter.
            throw new IOException("LimitedRangeWalker was not able to send message" + ": not from this greeter");
        }

        PeerViewElement pve = walk.getPeerView().getPeerViewElement(rdvMsg.getSrcPeerID());

        if (null == pve) {
            throw new IOException("LimitedRangeWalker was not able to send message" + ": no pve");
        }

        if (!pve.sendMessage(msg, rdvMsg.getSrcSvcName(), rdvMsg.getSrcSvcParams())) {
            throw new IOException("LimitedRangeWalker was not able to send message" + ": send failed");
        }
    }
}
