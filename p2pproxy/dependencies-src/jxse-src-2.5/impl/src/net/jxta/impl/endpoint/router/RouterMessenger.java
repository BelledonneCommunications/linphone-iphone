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

package net.jxta.impl.endpoint.router;


import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.Message;
import net.jxta.impl.endpoint.BlockingMessenger;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.IOException;
import net.jxta.protocol.RouteAdvertisement;


/**
 * Messenger for destinations which are logical peers. This messenger is used
 * only at the origin of routes. Incoming messages that are being forwarded to
 * another peer do not use this form of messenger.
 */
class RouterMessenger extends BlockingMessenger {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(RouterMessenger.class.getName());

    /**
     * The router we are working for. Also who we make route queries to.
     */
    private final EndpointRouter router;

    /**
     * Constructor for a RouterMessenger.
     *
     * @param dstAddress the peer which is the final destination of the message.
     * @param r          the router which this messenger is servicing.
     * @param hint       potential hint information that we passed
     * @throws IOException Thrown if the messenger cannot be constructed for this destination.
     */
    public RouterMessenger(EndpointAddress dstAddress, EndpointRouter r, Object hint) throws IOException {

        // Make sure that we do not ask for self destruction.
        super(r.getEndpointService().getGroup().getPeerGroupID(), dstAddress, false);

        this.router = r;

        // Probably redundant. getGatewayAddress does it.
        EndpointAddress plainAddr = new EndpointAddress(dstAddress, null, null);

        // Discard the hint if it is not a route advertisement.
        if (!(hint instanceof RouteAdvertisement)) {
            hint = null;
        }

        // We aggressively look for a route upfront. If it fails, we must refuse to create the messenger.
        EndpointAddress gate = router.getGatewayAddress(plainAddr, true, (RouteAdvertisement) hint);

        if (gate == null) {
            throw new IOException("Could not construct RouterMessenger, no route for " + plainAddr);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public EndpointAddress getLogicalDestinationImpl() {
        return getDestinationAddress();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void closeImpl() {// Nothing to do. The underlying connection is not affected.
        // The messenger will be marked closed by the state machine once completely down; that's it.
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isIdleImpl() {
        // We do not need self destruction.
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendMessageBImpl(Message message, String service, String serviceParam) throws IOException {

        if (isClosed()) {
            IOException failure = new IOException("Messenger was closed, it cannot be used to send messages.");

            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, failure.getMessage(), failure);
            }

            throw failure;
        }

        EndpointAddress dest = getDestAddressToUse(service, serviceParam);

        // Loop trying to send message until we run out of routes.
        Throwable lastFailure = null;

        while (true) {
            EndpointAddress sendTo = null;

            try {
                sendTo = router.addressMessage(message, dest);
                if (null == sendTo) {
                    break;
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Sending " + message + " to " + sendTo);
                }

                router.sendOnLocalRoute(sendTo, message);

                // it worked! We are done.
                return;
            } catch (RuntimeException rte) {
                // Either the message is invalid, or there is
                // a transport loop and the upper layer should close.
                // Either way, we must not retry. The loop could be
                // unbounded.

                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure while routing " + message, rte);
                }

                lastFailure = rte;
                break;
            } catch (Throwable theMatter) {
                if (sendTo == null) {
                    // This is bad: address message was not able to
                    // do anything. Stop the loop.
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Unknown failure while routing " + message, theMatter);
                    }
                    break;
                }

                // Everything else is treated like a bad route.
                lastFailure = theMatter;
            }

            // Currently we have only one long route per destination.
            // If the first hop is broken, then that long route is broken
            // as well. We must dump it, under penalty of trying it over and over again.

            EndpointAddress destPeer = new EndpointAddress(getDestinationAddress(), null, null);

            router.removeRoute(EndpointRouter.addr2pid(destPeer));

            // reset the router message for the next attempt.
            message.removeMessageElement(message.getMessageElement(EndpointRouterMessage.MESSAGE_NS, EndpointRouterMessage.MESSAGE_NAME));
        }

        if (lastFailure == null) {
            lastFailure = new IOException("Could not find a route for : " + dest);
        }

        // Except if we hit an illegal transport loop, we've exhausted all
        // the options we had, or there was a RuntimeException.
        // In both cases we must close. In the latter case it is a
        // precaution: we're not 100% sure that the message is at fault;
        // it could be this messenger as well. For illegal transport loops
        // the invoking messenger should close, not this one.

        if (!(lastFailure instanceof IllegalStateException)) {
            // FIXME - jice@jxta.org 20040413: as for all the transports. This used to be how this messenger broke itself.  Now,
            // all it does is too pretend that someone called for a nice close...just before the exception we throw causes the
            // BlockingMessenger state machine to go into breackage mode. Ultimately transports should get a deeper retrofit.
            close();
        }

        // Kind of stupid. Have to convert the runtime exceptions so that we
        // can re-throw them.

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.log(Level.FINE, "Messenger failed:", lastFailure);
        }

        if (lastFailure instanceof IOException) {
            throw (IOException) lastFailure;
        } else if (lastFailure instanceof RuntimeException) {
            throw (RuntimeException) lastFailure;
        } else if (lastFailure instanceof Error) {
            throw (Error) lastFailure;
        } else {
            IOException failure = new IOException("Failed sending " + message);
            failure.initCause(lastFailure);
            throw failure;
        }
    }
}

