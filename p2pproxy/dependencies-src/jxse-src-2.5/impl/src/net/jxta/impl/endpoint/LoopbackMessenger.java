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

package net.jxta.impl.endpoint;


import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.IOException;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import net.jxta.impl.peergroup.GenericPeerGroup;
import net.jxta.peergroup.PeerGroup;


/**
 * This class implements local delivery of messages (for example when the
 * InputPipe and the OutputPipe are located on the same peer)
 * <p/>
 * The reason this class is useful is that it may not always be possible to
 * connect to oneself without actually through the relay. i.e. A peer with outgoing
 * only http transport, can not possibly connect to self through the transport.
 * <p/>
 * Since transports cannot be relied on to perform a loopback, some layer
 * above has to figure out that a message is looping back.
 * Since peerid loopback does not explicitly request to go through a real
 * transport, and since peerid addressing is the job of the router, it is
 * the router that performs loopback.
 * <p/>
 * The router could probably perform the loopback by delivering the message
 * to its own input queue, that would take a special transport instead of a
 * special messenger, which is the same kind of deal but would imply some
 * incoming message processing by the router for every message. In
 * contrast, the loopback messenger is setup once and the router will never
 * sees the messages. That's a good optimization.
 * <p/>
 * Alternatively, the endpoint service itself could figure out the
 * loopback, but since the API wants to give a messenger to the requestor
 * rather than just sending a message, the endpoint would have to setup a
 * loopback messenger anyway. So it is pretty much the same.
 */
public class LoopbackMessenger extends BlockingMessenger {
    
    /**
     *  Logger
     */
    private final static transient Logger LOG = Logger.getLogger(LoopbackMessenger.class.getName());
    
    /**
     * The peergroup we are working for, ie. that we will loop back to.
     */
    private final PeerGroup group;
    
    /**
     * The endpoint we are working for, ie. that we will loop back to.
     */
    private final EndpointService endpoint;
    
    /**
     * The source address of messages sent on this messenger.
     */
    private final EndpointAddress srcAddress;
    
    /**
     * The location destination of this messenger.
     */
    private final EndpointAddress logicalDestination;
    
    /**
     *  Used to ensure that only a single message is demuxed at a time.
     */
    private final Lock orderingLock = new ReentrantLock(true);
    
    /**
     * Create a new loopback messenger.
     *
     * @param group       The group context.
     * @param ep          where messages go
     * @param src         who messages should be addressed from
     * @param dest        who messages should be addressed to
     * @param logicalDest The logical destination address.
     */
    public LoopbackMessenger(PeerGroup group, EndpointService ep, EndpointAddress src, EndpointAddress dest, EndpointAddress logicalDest) {
        super(group.getPeerGroupID(), dest, false);
        
        this.group = group;
        endpoint = ep;
        srcAddress = src;
        logicalDestination = logicalDest;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public EndpointAddress getLogicalDestinationImpl() {
        return logicalDestination;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public long getMTU() {
        return Long.MAX_VALUE;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isIdleImpl() {
        return false;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void closeImpl() {}
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void sendMessageBImpl(final Message message, final String service, final String serviceParam) throws IOException {
        
        if (isClosed()) {
            IOException failure = new IOException("Messenger was closed, it cannot be used to send messages.");
            
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, failure.getMessage(), failure);
            }
            throw failure;
        }
        
        orderingLock.lock();
        try {
            // Process the message with the appropriate src and dest address
            ((GenericPeerGroup)group).getExecutor().execute( new Runnable() {
                public void run() {
                    try {
                        endpoint.processIncomingMessage(message, srcAddress, getDestAddressToUse(service, serviceParam));
                    } catch(Throwable uncaught) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Uncaught Throwable in Loopback Messenger ", uncaught);
                        }
                    }
                }
            });
        } finally {
            orderingLock.unlock();
        }
    }
}
