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


import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.Messenger;
import net.jxta.impl.endpoint.BlockingMessenger;
import net.jxta.logging.Logging;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * This class is the Messenger used to send CbJx Messages
 */
public class CbJxMessenger extends BlockingMessenger {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(CbJxMessenger.class.getName());

    /**
     * the new destination address computed by the CbJx Endpoint
     * this address is of the form jxta://<peerID>/CbJxService/<peerGroupID>
     */
    private final EndpointAddress newDestAddr;

    /**
     * A string which we can lock on while acquiring new messengers. We don't
     * want to lock the whole object.
     */
    private final Object acquireMessengerLock = new String("Messenger Acquire Lock");

    /**
     * Cached messenger for sending to {@link #newDestAddr}
     */
    private Messenger outBoundMessenger = null;

    /**
     *  The transport we are working for.
     */
    private final CbJxTransport transport;

    /**
     * constructor
     *
     * @param dest the destination address
     */
    public CbJxMessenger(CbJxTransport transport, EndpointAddress dest, Object hintIgnored) throws IOException {
        this(transport, dest);
    }

    /**
     * constructor
     *
     * @param dest the destination address
     */
    public CbJxMessenger(CbJxTransport transport, EndpointAddress dest) throws IOException {

        // Do not use self destruction. There's nothing we have that can't just let be GC'ed
        super(transport.group.getPeerGroupID(), dest, false);

        this.transport = transport;

        newDestAddr = new EndpointAddress("jxta", dest.getProtocolAddress(), CbJxTransport.cbjxServiceName, null);

        outBoundMessenger = transport.endpoint.getMessengerImmediate(newDestAddr, null);

        if (null == outBoundMessenger) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Could not get messenger for " + newDestAddr);
            }

            throw new IOException("Could not get messenger for " + newDestAddr);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void closeImpl() {
        synchronized (acquireMessengerLock) {
            outBoundMessenger.close();
            outBoundMessenger = null;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public EndpointAddress getLogicalDestinationImpl() {
        return newDestAddr;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Since CbJx is a virtual transport and consumes very few resources there
     * is no point to doing idle teardown.
     */
    @Override
    public boolean isIdleImpl() {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendMessageBImpl(Message msg, String service, String serviceParam) throws IOException {
        msg = msg.clone();

        EndpointAddress destAddressToUse = getDestAddressToUse(service, serviceParam);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Messenger: sending out " + msg + " to: " + destAddressToUse);
        }

        // add the cbjx info to the message
        msg = transport.addCryptoInfo(msg, destAddressToUse);

            if (isClosed()) {
                IOException failure = new IOException("Messenger was closed, it cannot be used to send messages.");

                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info(failure.toString());
                }

                throw failure;
            }

            // and sends out the message
        sendTo( msg );
    }

    /**
     * Send a message via the underlying messenger.
     *
     * @param msg The message to send to the remote peer.
     * @throws IOException if there was a problem sending the message.
     **/
    void sendTo( Message msg ) throws IOException {

        synchronized (acquireMessengerLock) {
            if ((null == outBoundMessenger) || outBoundMessenger.isClosed()) {

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Getting messenger for " + newDestAddr);
                }

                outBoundMessenger = transport.endpoint.getMessengerImmediate(newDestAddr, null);

                if (outBoundMessenger == null) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("Could not get messenger for " + newDestAddr);
                    }

                    throw new IOException("Underlying messenger could not be repaired");
                }
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending " + msg + " to endpoint " + newDestAddr);
        }

        // Good we have a messenger. Send the message.
        outBoundMessenger.sendMessageB(msg, null, null);
    }
}
