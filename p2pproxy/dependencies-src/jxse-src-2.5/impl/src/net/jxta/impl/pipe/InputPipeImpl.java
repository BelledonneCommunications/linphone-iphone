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
package net.jxta.impl.pipe;

import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.id.ID;
import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.util.UnbiasedQueue;
import net.jxta.logging.Logging;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.PipeID;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.protocol.PipeAdvertisement;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Implements the {@link net.jxta.pipe.InputPipe} interface by listening on the
 * endpoint for messages to service "PipeService" and a param of the Pipe ID.
 */
class InputPipeImpl implements EndpointListener, InputPipe {

    /**
     * logger
     */
    private final static Logger LOG = Logger.getLogger(InputPipeImpl.class.getName());

    protected final static int QUEUESIZE = 100;

    protected PipeRegistrar registrar;

    protected final PipeAdvertisement pipeAdv;
    protected final ID pipeID;

    protected volatile boolean closed = false;

    protected PipeMsgListener listener;
    protected final UnbiasedQueue queue;

    /**
     * Constructor for the InputPipeImpl object
     *
     * @param r        pipe resolver
     * @param adv      pipe advertisement
     * @param listener listener to receive messages
     * @throws IOException if an io error occurs
     */
    InputPipeImpl(PipeRegistrar r, PipeAdvertisement adv, PipeMsgListener listener) throws IOException {
        registrar = r;
        this.pipeAdv = adv;
        this.listener = listener;

        pipeID = adv.getPipeID();

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info(
                    "Creating InputPipe for " + pipeID + " of type " + adv.getType() + " with "
                    + ((null != listener) ? "listener" : "queue"));
        }

        // queue based inputpipe?
        if (listener == null) {
            queue = UnbiasedQueue.synchronizedQueue(new UnbiasedQueue(QUEUESIZE, true));
        } else {
            queue = null;
        }

        if (!registrar.register(this)) {
            throw new IOException("Could not register input pipe (already registered) for " + pipeID);
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Closes the pipe.
     */
    @Override
    protected synchronized void finalize() throws Throwable {
        if (!closed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Pipe is being finalized without being previously closed. This is likely a bug.");
            }
        }
        close();
        super.finalize();
    }

    /**
     * {@inheritDoc}
     */
    public Message waitForMessage() throws InterruptedException {
        return poll(0);
    }

    /**
     * {@inheritDoc}
     */
    public Message poll(int timeout) throws InterruptedException {
        if (listener == null) {
            return (Message) queue.pop(timeout);
        } else {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("poll() has no effect in listener mode.");
            }
            return null;
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void close() {
        if (closed) {
            return;
        }
        closed = true;

        // Close the queue
        if (null == listener) {
            queue.close();
        }

        listener = null;
        // Remove myself from the pipe registrar.
        if (!registrar.forget(this)) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("close() : pipe was not registered with registrar.");
            }
        }
        registrar = null;

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Closed " + pipeID);
        }
    }

    /**
     * {@inheritDoc}
     */
    public void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {
        // if we are closed, ignore any additional messages
        if (closed) {
            return;
        }

        // XXX: header check, security and such should be done here
        // before pushing the message onto the queue.
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Received " + msg + " from " + srcAddr + " for " + pipeID);
        }
        // determine where demux the msg, to listener, or onto the queue
        if (null == queue) {
            PipeMsgListener temp = listener;
            if (null == temp) {
                return;
            }

            PipeMsgEvent event = new PipeMsgEvent(this, msg, (PipeID) pipeID);
            try {
                temp.pipeMsgEvent(event);
            } catch (Throwable ignored) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in listener for : " + pipeID + "(" + temp.getClass().getName() + ")", ignored);
                }
            }
        } else {
            boolean pushed = false;
            while (!pushed && !queue.isClosed()) {
                try {
                    pushed = queue.push(msg, TimeUtils.ASECOND);
                } catch (InterruptedException woken) {
                    Thread.interrupted();
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                synchronized (this) {
                    LOG.fine("Queued " + msg + " for " + pipeID + "\n\tqueue closed : " + queue.isClosed() + "\tnumber in queue : "
                            + queue.getCurrentInQueue() + "\tnumber queued : " + queue.getNumEnqueued() + "\tnumber dequeued : "
                            + queue.getNumDequeued());
                }
            }
        }
    }

    /**
     * Gets the pipe type
     *
     * @return The type
     */
    public String getType() {
        return pipeAdv.getType();
    }

    /**
     * Gets the pipe id
     *
     * @return The type
     */
    public ID getPipeID() {
        return pipeID;
    }

    /**
     * Gets the pipe name
     *
     * @return The name
     */
    public String getName() {
        return pipeAdv.getName();
    }

    /**
     * Gets the pipe advertisement
     *
     * @return The advertisement
     */
    public PipeAdvertisement getAdvertisement() {
        return pipeAdv;
    }
}
