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

import net.jxta.document.MimeMediaType;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.TextDocumentMessageElement;
import net.jxta.peer.PeerID;

import net.jxta.impl.protocol.LimitedRangeRdvMsg;
import net.jxta.impl.rendezvous.RdvWalker;
import net.jxta.impl.rendezvous.rpv.PeerViewElement;

/**
 * The Limited Range Walker is designed to be used by Rendezvous Peer in
 * order to propagate a message amongst them. A target destination peer
 * is used in order to send the message to a primary peer. Then, depending
 * on the TTL, the message is duplicated into two messages, each of them
 * being sent in opposite "directions" of the RPV.
 */
public class LimitedRangeWalker implements RdvWalker {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(LimitedRangeWalker.class.getName());

    /**
     * The walk we are associated with.
     */
    private final LimitedRangeWalk walk;

    /**
     * Constructor
     *
     * @param walk The walk we will be associated with.
     */
    public LimitedRangeWalker(LimitedRangeWalk walk) {
        this.walk = walk;
    }

    /**
     * {@inheritDoc}
     */
    public void stop() {
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Sends message on :
     * <pre>
     *  "LR-Greeter"&lt;groupid>/&lt;walkSvc>&lt;walkParam>
     *  </pre>
     * <p/>
     * XXX bondolo 20060720 This method will currently fail to walk the
     * message to the DOWN peer if there is a failure sending the message to
     * the UP peer. Perhaps it would be better to ignore any errors from
     * sending to either peer?
     */
    private void walkMessage(Message msg, LimitedRangeRdvMsg rdvMsg) throws IOException {

        LimitedRangeRdvMsg.WalkDirection dir = rdvMsg.getDirection();

        if ((dir == LimitedRangeRdvMsg.WalkDirection.BOTH) || (dir == LimitedRangeRdvMsg.WalkDirection.UP)) {
            PeerViewElement upPeer = walk.getPeerView().getUpPeer();

            if ((upPeer != null) && upPeer.isAlive()) {
                Message newMsg = msg.clone();

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Walking " + newMsg + " [UP] to " + upPeer);
                }

                rdvMsg.setDirection(LimitedRangeRdvMsg.WalkDirection.UP);

                updateRdvMessage(newMsg, rdvMsg);
                upPeer.sendMessage(newMsg, walk.getWalkServiceName(), walk.getWalkServiceParam());
            }
        }

        if ((dir == LimitedRangeRdvMsg.WalkDirection.BOTH) || (dir == LimitedRangeRdvMsg.WalkDirection.DOWN)) {
            PeerViewElement downPeer = walk.getPeerView().getDownPeer();

            if ((downPeer != null) && downPeer.isAlive()) {
                Message newMsg = msg.clone();

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Walking " + newMsg + " [DOWN] to " + downPeer);
                }

                rdvMsg.setDirection(LimitedRangeRdvMsg.WalkDirection.DOWN);

                updateRdvMessage(newMsg, rdvMsg);
                downPeer.sendMessage(newMsg, walk.getWalkServiceName(), walk.getWalkServiceParam());
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void walkMessage(PeerID destination, Message msg, String srcSvcName, String srcSvcParam, int ttl) throws IOException {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Walking " + msg + " to " + srcSvcName + "/" + srcSvcParam);
        }

        // Check if there is already a Rdv Message
        LimitedRangeRdvMsg rdvMsg = LimitedRangeWalk.getRdvMessage(msg);

        if (rdvMsg == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Creating new Walk Header for " + msg + " with TTL=" + ttl);
            }

            // Create a new one.
            rdvMsg = new LimitedRangeRdvMsg();

            rdvMsg.setTTL(Integer.MAX_VALUE); // will be reduced.
            rdvMsg.setDirection(LimitedRangeRdvMsg.WalkDirection.BOTH);
            rdvMsg.setSrcPeerID(walk.getPeerGroup().getPeerID());
            rdvMsg.setSrcSvcName(srcSvcName);
            rdvMsg.setSrcSvcParams(srcSvcParam);
        } else {
            // decrement TTL before walk.
            rdvMsg.setTTL(rdvMsg.getTTL() - 1);
        }

        int useTTL = Math.min(ttl, rdvMsg.getTTL());

        useTTL = Math.min(useTTL, walk.getPeerView().getView().size() + 1);

        rdvMsg.setTTL(useTTL);

        if (useTTL <= 0) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("LimitedRangeWalker was not able to send " + msg + " : No TTL remaining");
            }

            return;
        }

        // Forward the message according to the direction set in the Rdv Message.
        if (null != destination) {
            Message tmp = msg.clone();

            updateRdvMessage(tmp, rdvMsg);
            sendToPeer(destination, walk.getWalkServiceName(), walk.getWalkServiceParam(), tmp);
        } else {
            walkMessage(msg, rdvMsg);
        }
    }

    /**
     * Replace the old version of the rdvMsg
     *
     * @param msg    The message to be updated.
     * @param rdvMsg The LimitedRangeRdvMsg which will update the message.
     * @return the updated message
     */
    private Message updateRdvMessage(Message msg, LimitedRangeRdvMsg rdvMsg) {
        XMLDocument asDoc = (XMLDocument) rdvMsg.getDocument(MimeMediaType.XMLUTF8);
        MessageElement el = new TextDocumentMessageElement(LimitedRangeWalk.ELEMENTNAME, asDoc, null);

        msg.replaceMessageElement("jxta", el);

        return msg;
    }

    /**
     * Sends the provided message to the specified peer. The peer must be a
     * current member of the PeerView.
     *
     * @param dest     The destination peer.
     * @param svcName  The destinations service.
     * @param svcParam The destination service params.
     * @param msg      The message to send.
     * @throws IOException Thrown for problems sending the message.
     */
    private void sendToPeer(PeerID dest, String svcName, String svcParam, Message msg) throws IOException {
        PeerViewElement pve = walk.getPeerView().getPeerViewElement(dest);

        if (null == pve) {
            throw new IOException("LimitedRangeWalker was not able to send " + msg + " : no pve");
        }

        if (!pve.sendMessage(msg, svcName, svcParam)) {
            throw new IOException("LimitedRangeWalker was not able to send " + msg + " : send failed");
        }
    }
}
