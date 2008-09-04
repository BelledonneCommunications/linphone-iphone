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
package net.jxta.socket;


import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;

import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.net.SocketTimeoutException;
import java.util.LinkedList;
import java.util.Queue;


/**
 * Provides the stream data source for JxtaSocket.
 *
 * @author Athomas Goldberg
 */
class JxtaSocketInputStream extends InputStream {

    /**
     * We push this "poison" value into the accept backlog queue in order to
     * signal that the queue has been closed.
     */
    protected static final MessageElement QUEUE_END = new StringMessageElement("Terminal", "Terminal", null);

    /**
     * Our read timeout.
     */
    private long timeout = 60 * 1000;

    /**
     * The associated socket.
     */
    private final JxtaSocket socket;

    /**
     * Our queue of message elements waiting to be read.
     */
    protected final Queue<MessageElement> queue;

    /**
     * The maximum number of message elements we will allow in the queue.
     */
    protected final int queueSize;

    /**
     * The current message element input stream we are processing.
     */
    private InputStream currentMsgStream = null;

    /**
     * Construct an InputStream for a specified JxtaSocket.
     *
     * @param socket  the JxtaSocket
     * @param queueSize the queue size
     */
    JxtaSocketInputStream(JxtaSocket socket, int queueSize) {
        this.socket = socket;
        this.queueSize = queueSize;
        queue = new LinkedList<MessageElement>();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized int available() throws IOException {
        int result;
        InputStream in = getCurrentStream(false);

        if (in != null) {
            result = in.available();
        } else {
            // We chose not to block, if we have no inputstream then
            // that means there are no bytes available.
            result = 0;
        }
        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized int read() throws IOException {
        byte[] b = new byte[1];
        int result = 0;

        // The result of read() can be -1 (EOF), 0 (yes, its true) or 1.
        while (0 == result) {
            result = read(b, 0, 1);
        }

        if (-1 != result) {
            result = (int) b[0];
        }
        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized int read(byte b[], int off, int len) throws IOException {
        if ((off < 0) || (off > b.length) || (len < 0) || ((off + len) > b.length) || ((off + len) < 0)) {
            throw new IndexOutOfBoundsException();
        }

        while (true) {
            int result = -1;
            InputStream in = getCurrentStream(true);

            if (null == in) {
                return -1;
            }

            result = in.read(b, off, len);
            if (0 == result) {
                // Some streams annoyingly return 0 result. We won't
                // perpetuate this behaviour.
                continue;
            }

            if (result == -1) {
                closeCurrentStream();
                continue;
            }
            return result;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void close() {
        queue.clear();
        closeCurrentStream();
        queue.offer(QUEUE_END);
        notify();
    }

    /**
     * Rather than force the InputStream closed we add the EOF at the end of
     * any current data.
     */
    synchronized void softClose() {
        queue.offer(QUEUE_END);
        notify();
    }

    /**
     * Get the input stream for the current segment and optionally block until
     * a segment is available.
     *
     * @param block If {@code true} then block until a segment is available.
     * @return the InputStream
     * @throws IOException if an io error occurs
     */
    private InputStream getCurrentStream(boolean block) throws IOException {

        if (currentMsgStream == null) {

            if (QUEUE_END == queue.peek()) {
                // We are at the end of the queue.
                return null;
            }

            MessageElement me = null;
            long pollUntil = (Long.MAX_VALUE == timeout) ? Long.MAX_VALUE : System.currentTimeMillis() + timeout;

            while (pollUntil >= System.currentTimeMillis()) {
                try {
                    me = queue.poll();

                    if (null == me) {
                        long sleepFor = pollUntil - System.currentTimeMillis();

                        if (sleepFor > 0) {
                            wait(sleepFor);
                        }
                    } else {
                        break;
                    }
                } catch (InterruptedException woken) {
                    InterruptedIOException incomplete = new InterruptedIOException("Interrupted waiting for data.");

                    incomplete.initCause(woken);
                    incomplete.bytesTransferred = 0;
                    throw incomplete;
                }
            }

            if (block && (null == me)) {
                throw new SocketTimeoutException("Socket timeout during read.");
            }

            if (me != null) {
                currentMsgStream = me.getStream();
            }
        }
        return currentMsgStream;
    }

    private void closeCurrentStream() {
        if (currentMsgStream != null) {
            try {
                currentMsgStream.close();
            } catch (IOException ignored) {// ignored
            }
            currentMsgStream = null;
        }
    }

    synchronized void enqueue(MessageElement element) {
        if (queue.contains(QUEUE_END)) {
            // We have already marked the end of the queue.
            return;
        }
        if (queue.size() < queueSize) {
            queue.offer(element);
        }
        notify();
    }

    /**
     * Returns the timeout value for this socket. This is the amount of time in
     * relative milliseconds which we will wait for read() operations to
     * complete.
     *
     * @return The timeout value in milliseconds or 0 (zero) for
     *         infinite timeout.
     */
    long getTimeout() {
        if (timeout < Long.MAX_VALUE) {
            return timeout;
        } else {
            return 0;
        }
    }

    /**
     * Returns the timeout value for this socket. This is the amount of time in
     * relative milliseconds which we will wait for read() operations to
     * operations to complete.
     *
     * @param timeout The timeout value in milliseconds or 0 (zero) for
     *                infinite timeout.
     */
    void setTimeout(long timeout) {
        if (timeout < 0) {
            throw new IllegalArgumentException("Negative timeout not allowed.");
        }

        if (0 == timeout) {
            timeout = Long.MAX_VALUE;
        }
        this.timeout = timeout;
    }
}
