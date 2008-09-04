/*
 * Copyright (c) 2003-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util.pipe.reliable;


import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.WireFormatMessageFactory;
import net.jxta.impl.util.TimeUtils;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 *  Acts as a reliable input stream. Accepts data which
 *  arrives in messages and orders it.
 */
public class ReliableInputStream extends InputStream implements Incoming {
    
    /**
     *  Logger
     */
    private static final Logger LOG = Logger.getLogger(ReliableInputStream.class.getName());
    
    /**
     *  Connection we are working for.
     */
    private Outgoing outgoing;
    
    private volatile boolean closed = false;
    private boolean closing = false;
    
    private MsgListener listener = null;
    
    /**
     *  The amount of time that read() operation will block. > 0
     */
    private long timeout;
    
    /**
     *  The current sequence number we are reading bytes from.
     */
    private volatile int sequenceNumber = 0;
    
    /**
     *  Queue of incoming messages.
     */
    private final List<IQElt> inputQueue = new ArrayList<IQElt>();
    
    /**
     *  The I/O record for the message we are currently using for stream data.
     */
    private final Record record;
    
    /**
     * Input record Object
     */
    private static class Record {
        public InputStream inputStream;
        // next inbuff byte
        public long nextByte;
        // size of Record
        public long size;
        
        public Record() {
            inputStream = null; // allocated by caller
            nextByte = 0; // We read here (set by caller)
            size = 0; // Record size(set by caller)
        }
        
        /** reset the record element
         *
         */
        public void resetRecord() {
            if (null != inputStream) {
                try {
                    inputStream.close();
                } catch (IOException ignored) {}
            }
            inputStream = null;
            size = nextByte = 0;
        }
    }
    

    /**
     *  An input queue element which breaks out a received message in 
     *  enqueueMessage().
     */
    private static class IQElt implements Comparable {
        final int seqnum;
        final MessageElement elt;
        boolean ackd = false;
        
        IQElt(int sequence, MessageElement element) {
            seqnum = sequence;
            elt = element;
        }
        
        /**
         * {@inheritDoc}
         */
        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj instanceof IQElt) {
                IQElt targ = (IQElt) obj;

                return (this.seqnum == targ.seqnum);
            }
            return false;
        }
        
        public int compareTo(IQElt el) {
            return this.seqnum < el.seqnum ? -1 : this.seqnum == el.seqnum ? 0 : 1;
        }
        
        /**
         * {@inheritDoc}
         */
        public int compareTo(Object o) {
            return compareTo((IQElt) o);
        }
    }
    
    public ReliableInputStream(Outgoing outgoing, int timeout) {
        this(outgoing, timeout, null);
    }
    
    public ReliableInputStream(Outgoing outgoing, int timeout, MsgListener listener) {
        this.outgoing = outgoing;
        setTimeout(timeout);
        
        record = new Record();
        this.listener = listener;
        // 1 <= seq# <= maxint, monotonically increasing
        // Incremented before compare.
        sequenceNumber = 0;
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            if (listener != null) {
                LOG.info("Listener based ReliableInputStream created");
            }
        }
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>This is an explicit close operation. All subsequent {@code read()}
     * operations will fail.
     */
    @Override
    public void close() throws IOException {
        super.close();
        synchronized (inputQueue) {
            closed = true;
            inputQueue.clear();
            inputQueue.notifyAll();
        }
    }
    
    /**
     * Returns true if closed
     *
     * @return true if closed
     */
    public boolean isInputShutdown() {
        return closed;
    }
    
    /**
     * Prepare this input stream to being closed. It will still deliver the
     * packets that have been received, but nothing more. This is meant to be
     * called in response to the other side having initiated closure. We assume
     * that when the other side does it it means that it is satisfied with what
     * we have acknowledged so far.
     */
    public void softClose() {
        synchronized (inputQueue) {
            closing = true;
            inputQueue.notifyAll();
        }
    }
    
    /**
     *  Sets the Timeout attribute. A timeout of 0 blocks forever
     *
     * @param  timeout The new soTimeout value
     */
    public void setTimeout(int timeout) {
        if (timeout < 0) {
            throw new IllegalArgumentException("Timeout must be >=0");
        }
        
        this.timeout = (0 == timeout) ? Long.MAX_VALUE : timeout;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public int read() throws IOException {
        if (closed) {
            return -1;
        }
        
        byte[] a = new byte[1];
        
        while (true) {
            int len = local_read(a, 0, 1);
            
            if (len < 0) {
                break;
            }
            if (len > 0) {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("Read() : " + (a[0] & 255));
                }
                
                return a[0] & 0xFF; // The byte
            }
        }
        
        // If we've reached EOF, there's nothing to do but close().
        
        close();
        return -1;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public int read(byte[] a, int offset, int length) throws IOException {
        if (closed) {
            return -1;
        }
        
        if (0 == length) {
            return 0;
        }
        
        int i = local_read(a, offset, length);
        
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Read(byte[], int, " + length + "), bytes read = " + i);
        }
        
        // If we've reached EOF; there's nothing to do but close().
        if (i == -1) {
            close();
        }
        return i;
    }
    
    /**
     *  Send a sequential ACK and selective ACKs for all of
     *  the queued messages.
     *
     *  @param seqnAck the sequence number being sequential ACKed
     */
    private void sendACK(int seqnAck) {
        // No need to sync on inputQueue, acking as many as we can is want we want
        List<Integer> selectedAckList = new ArrayList<Integer>();
        List<IQElt> queue;
        
        synchronized (inputQueue) {
            queue = new ArrayList<IQElt>(inputQueue);
        }
        
        Iterator<IQElt> eachInQueue = queue.iterator();

        while (eachInQueue.hasNext() && (selectedAckList.size() < Defs.MAXQUEUESIZE)) {
            IQElt anIQElt = eachInQueue.next();

            if (anIQElt.seqnum > seqnAck) {
                if (!anIQElt.ackd) {
                    selectedAckList.add(anIQElt.seqnum);
                    anIQElt.ackd = true;
                }
            }
        }
        
        // PERMIT DUPLICATE ACKS. Just a list and one small message.
        sendACK(seqnAck, selectedAckList);
    }
    
    /**
     *  Build an ACK message. The message provides a sequential ACK count and
     *  an optional list of selective ACKs.
     *
     *  @param seqnAck the sequence number being sequential ACKed
     *  @param sackList a list of selective ACKs. Must be sorted in increasing
     *  order.
     */
    private void sendACK(int seqnAck, List<Integer> sackList) {
        ByteArrayOutputStream bos = new ByteArrayOutputStream((1 + sackList.size()) * 4);
        DataOutputStream dos = new DataOutputStream(bos);
        
        try {
            dos.writeInt(seqnAck);
            for (Integer aSackList : sackList) {
                dos.writeInt(aSackList);
            }
            dos.close();
            bos.close();
            
            Message ACKMsg = new Message();
            MessageElement elt = new ByteArrayMessageElement(Defs.ACK_ELEMENT_NAME, Defs.MIME_TYPE_ACK, bos.toByteArray(), null);
            
            ACKMsg.addMessageElement(Defs.NAMESPACE, elt);
            
            outgoing.send(ACKMsg);
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("SENT ACK, seqn#" + seqnAck + " and " + sackList.size() + " SACKs ");
            }
        } catch (IOException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "sendACK caught IOException:", e);
            }
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    public void recv(Message msg) {
        queueIncomingMessage(msg);
    }
    
    public boolean hasNextMessage() {
        return !inputQueue.isEmpty();
    }
       
    Message nextMessage(boolean blocking) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("nextMessage blocking?  [" + blocking + "]");
        }
        MessageElement elt = dequeueMessage(sequenceNumber + 1, blocking);

        if (null == elt) {
            return null;
        }
        sequenceNumber += 1; // next msg sequence number
        
        Message msg;

        try {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Converting message seqn :" + (sequenceNumber - 1) + "element to message");
            }
            
            msg = WireFormatMessageFactory.fromWire(elt.getStream(), Defs.MIME_TYPE_MSG, null);
        } catch (IOException ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not deserialize message " + elt.getElementName(), ex);
            }
            return null;
        }
        return msg;
    }
    
    /**
     *  queue messages by sequence number.
     */
    private void queueIncomingMessage(Message msg) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Queue Incoming Message begins for " + msg);
        }
        
        long startEnqueue = TimeUtils.timeNow();
        
        Iterator<MessageElement> eachElement = msg.getMessageElements(Defs.NAMESPACE, Defs.MIME_TYPE_BLOCK);
        
        // OK look for jxta message
        while (!closed && !closing && eachElement.hasNext()) {
            MessageElement elt = eachElement.next();

            eachElement.remove();
            
            int msgSeqn;

            try {
                msgSeqn = Integer.parseInt(elt.getElementName());
            } catch (NumberFormatException n) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Discarding element (" + elt.getElementName() + ") Not one of ours.");
                }
                continue;
            }
            
            IQElt newElt = new IQElt(msgSeqn, elt);
            
            // OK we must enqueue
            
            // We rely on the sender to not to send more than the window size
            // because we do not limit the number of elements we allow to be
            // enqueued.
            
            // see if this is a duplicate
            if (newElt.seqnum <= sequenceNumber) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("RCVD OLD MESSAGE : Discard seqn#" + newElt.seqnum + " now at seqn#" + sequenceNumber);
                }
                break;
            }
            
            synchronized (inputQueue) {
                
                // dbl check with the lock held.
                if (closing || closed) {
                    return;
                }
                
                // Insert this message into the input queue.
                // 1. Do not add duplicate messages
                // 2. Store in increasing sequence nos.
                int insertIndex = inputQueue.size();
                boolean duplicate = false;
                
                for (int j = 0; j < inputQueue.size(); j++) {
                    IQElt iq = inputQueue.get(j);

                    if (newElt.seqnum < iq.seqnum) {
                        insertIndex = j;
                        break;
                    } else if (newElt.seqnum == iq.seqnum) {
                        duplicate = true;
                        break;
                    }
                }
                
                if (duplicate) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("RCVD OLD MESSAGE :  Discard duplicate msg, seqn#" + newElt.seqnum);
                    }
                    break;
                }
                
                inputQueue.add(insertIndex, newElt);
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Enqueued msg with seqn#" + newElt.seqnum + " at index " + insertIndex);
                }
                
                inputQueue.notifyAll();
            }
        }
        
        if (listener != null) {
            Message newmsg = null;

            while (true) {
                try {
                    newmsg = nextMessage(false);
                } catch (IOException io) {// do nothing as this exception will never occur
                }
                if (newmsg == null) {
                    break;
                }
                try {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("In listener mode, calling back listener");
                    }
                    listener.processIncomingMessage(newmsg);
                } catch (Throwable all) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Uncaught Throwable calling listener", all);
                    }
                }
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            long waited = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), startEnqueue);

            LOG.fine("Queue Incoming Message for " + msg + " completed in " + waited + " msec.");
        }
    }
    
    long nextRetransRequest = TimeUtils.toAbsoluteTimeMillis(TimeUtils.ASECOND);
    
    /**
     *  Dequeue the message with the desired sequence number waiting as needed
     *  until the message is available.
     *
     *  @param desiredSeqn the sequence number to be dequeued.
     *  @param blocking If {@code true} then this method should block while
     *  waiting for the specified message sequence number.
     *  @return the Message Element with the desired sequence number or null if
     *  the queue has been closed.
     */
    private MessageElement dequeueMessage(int desiredSeqn, boolean blocking) throws IOException {
        IQElt iQ = null;
        
        // Wait for incoming message here
        long startDequeue = TimeUtils.timeNow();
        long timeoutAt = TimeUtils.toAbsoluteTimeMillis(timeout);
        int wct = 0;
        
        synchronized (inputQueue) {
            while (!closed) {
                if (inputQueue.isEmpty()) {
                    if (!blocking) {
                        return null;
                    }
                    if (closing) {
                        return null;
                    }
                    try {
                        wct++;
                        inputQueue.wait(TimeUtils.ASECOND);
                        if (timeoutAt < TimeUtils.timeNow()) {
                            throw new SocketTimeoutException("Read timeout reached");
                        }
                    } catch (InterruptedException e) {
                        Thread.interrupted();
                    }
                    // reset retrans request timer since we don't want to immediately
                    // request retry after a long wait for out of order messages.
                    
                    nextRetransRequest = TimeUtils.toAbsoluteTimeMillis(TimeUtils.ASECOND);
                    continue;
                }
                
                iQ = inputQueue.get(0); // FIFO
                
                if (iQ.seqnum < desiredSeqn) {
                    // Ooops a DUPE slipped in the head of the queue undetected
                    // (seqnum consistency issue).
                    // Just drop it.
                    inputQueue.remove(0);
                    // if such is the case then notify the other end so that
                    // the message does not remain in the retry queue eventually
                    // triggering a broken pipe exception
                    sendACK(iQ.seqnum);
                    continue;
                } else if (iQ.seqnum != desiredSeqn) {
                    if (TimeUtils.toRelativeTimeMillis(nextRetransRequest) < 0) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Trigger retransmission. Wanted seqn#" + desiredSeqn + " found seqn#" + iQ.seqnum);
                        }
                        sendACK(desiredSeqn - 1);
                        nextRetransRequest = TimeUtils.toAbsoluteTimeMillis(TimeUtils.ASECOND);
                    }
                    if (!blocking) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Message out of sequece in Non-Blocking mode. returning");
                        }
                        // not the element of interest return nothing
                        return null;
                    }
                    try {
                        wct++;
                        inputQueue.wait(TimeUtils.ASECOND);
                        if (timeoutAt < TimeUtils.timeNow()) {
                            throw new SocketTimeoutException("Read timeout reached");
                        }
                    } catch (InterruptedException e) {
                        throw new InterruptedIOException("IO interrupted ");
                    }
                    continue;
                }
                inputQueue.remove(0);
                break;
            }
        }
        nextRetransRequest = 0;
        // if we are closed then we return null
        if (null == iQ) {
            return null;
        }
        
        sendACK(desiredSeqn);
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            long waited = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), startDequeue);

            LOG.fine("DEQUEUED seqn#" + iQ.seqnum + " in " + waited + " msec on input queue");
            if (wct > 0) {
                LOG.fine("DEQUEUE waited " + wct + " times on input queue");
            }
        }
        return iQ.elt;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public int available() throws IOException {
        if (listener != null) {
            throw new IOException("available() not supported in async mode");
        }
        if (closed) {
            throw new IOException("Stream closed");
        }
        synchronized (record) {
            if (record.inputStream != null) {
                if ((record.size == 0) || (record.nextByte == record.size)) {
                    if (inputQueue.isEmpty()) {
                        return 0;
                    }
                    // reset the record
                    record.resetRecord(); // GC as necessary(inputStream byte[])
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Getting next data block at seqn#" + (sequenceNumber + 1));
                    }
                    MessageElement elt = dequeueMessage(sequenceNumber + 1, false);

                    if (null == elt) {
                        return 0;
                    }
                    sequenceNumber += 1; // next msg sequence number
                    // Get the length of the Record
                    record.size = elt.getByteLength();
                    record.inputStream = elt.getStream();
                }
                return record.inputStream.available();
            }
        }
        return 0;
    }
    
    private int local_read(byte[] buf, int offset, int length) throws IOException {
        
        if (listener != null) {
            throw new IOException("read() not supported in async mode");
        }
        
        synchronized (record) {
            if ((record.size == 0) || (record.nextByte == record.size)) {
                
                // reset the record
                record.resetRecord(); // GC as necessary(inputStream byte[])
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Getting next data block at seqn#" + (sequenceNumber + 1));
                }
                
                MessageElement elt = dequeueMessage(sequenceNumber + 1, true);
                
                if (null == elt) {
                    return -1;
                }
                
                sequenceNumber += 1; // next msg sequence number
                
                // Get the length of the Record
                record.size = elt.getByteLength();
                record.inputStream = elt.getStream();
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("new seqn#" + sequenceNumber + ", bytes = " + record.size);
                }
            }
            
            // return the requested Record data
            // These calls should NEVER ask for more data than is in the
            // received Record.
            
            long left = record.size - record.nextByte;
            int copyLen = (int) Math.min(length, left);
            int copied = 0;
            
            do {
                int res = record.inputStream.read(buf, offset + copied, copyLen - copied);
                
                if (res < 0) {
                    break;
                }
                copied += res;
            } while (copied < copyLen);
            
            record.nextByte += copied;
            
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Requested " + length + ", Read " + copied + " bytes");
            }
            
            return copied;
        }
    }
    
    /**
     * Returns the message listener for this pipe
     * @return MsgListener
     *
     */
    public MsgListener getListener() {
        return listener;
    }
    
    /**
     *  The listener interface for receiving {@link net.jxta.endpoint.Message}
     */
    public interface MsgListener {
        
        /**
         * Called for each message received.
         *
         * @param message The message to be received.
         */
        void processIncomingMessage(Message message);
    }
}
