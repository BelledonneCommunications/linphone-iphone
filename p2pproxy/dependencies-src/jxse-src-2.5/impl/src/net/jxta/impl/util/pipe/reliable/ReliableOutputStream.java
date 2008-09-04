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


import java.io.ByteArrayOutputStream;
import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.endpoint.WireFormatMessage;
import net.jxta.endpoint.WireFormatMessageFactory;
import net.jxta.impl.util.TimeUtils;
import net.jxta.logging.Logging;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * Accepts data and packages it into messages for sending to the remote. The
 * messages are kept in a retry queue until the remote peer acknowledges
 * receipt of the message.
 */
public class ReliableOutputStream extends OutputStream implements Incoming {
    
    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(ReliableOutputStream.class.getName());
    
    /**
     * Initial estimated Round Trip Time
     */
    private final static long initRTT = 10 * TimeUtils.ASECOND;
    
    /**
     *  The default size for the blocks we will chunk the stream into.
     */
    private final static int DEFAULT_MESSAGE_CHUNK_SIZE = 63 * 1024;
    
    private final static MessageElement RETELT = new StringMessageElement(Defs.RETRY_ELEMENT_NAME, Defs.RETRY_ELEMENT_VALUE, null);
    
    /**
     * A lock we use to ensure that write operations happen in order.
     */
    private final Object writeLock = new String("writeLock");
    
    /**
     * The buffer we cache writes to.
     */
    private byte[] writeBuffer = null;
    
    /**
     * Number of bytes written to the write buffer.
     */
    private int writeCount = 0;
    
    /**
     * Set the default write buffer size.
     */
    private int writeBufferSize = DEFAULT_MESSAGE_CHUNK_SIZE;
    
    /**
     * Absolute time in milliseconds at which the write buffer began
     * accumulating bytes to be written.
     */
    private long writeBufferAge = Long.MAX_VALUE;
    
    /**
     * If less than {@code TimeUtils.timenow()} then we are closed otherwise
     * this is the absolute time at which we will become closed. We begin by
     * setting this value as {@Long.MAX_VALUE} until we establish an earlier
     * close deadline.
     */
    private long closedAt = Long.MAX_VALUE;
    
    /**
     * If {@code true} then we have received a close request from the remote
     * side. They do not want to receive any more messages from us.
     */
    private volatile boolean remoteClosed = false;
    
    /**
     * If {@code true} then we have closed this stream locally and will not
     * accept any further messages for sending. Unacknowledged messages will
     * be retransmitted until the linger delay is passed.
     */
    private volatile boolean localClosed = false;
    
    /**
     * The relative time in milliseconds that we will allow our connection to
     * linger.
     */
    private long lingerDelay = 120 * TimeUtils.ASECOND;
    
    /**
     * Sequence number of the message we most recently sent out.
     */
    private AtomicInteger sequenceNumber = new AtomicInteger(0);
    
    /**
     * Sequence number of highest sequential ACK.
     */
    private volatile int maxACK = 0;
    
    /**
     * connection we are working for
     */
    private final Outgoing outgoing;
    
    /**
     *  The daemon thread that performs retransmissions.
     */
    private Thread retrThread = null;
    
    // for retransmission
    /**
     * Average round trip time in milliseconds.
     */
    private volatile long aveRTT = initRTT;
    private volatile long remRTT = 0;
    
    /**
     * Has aveRTT been set at least once over its initial guesstimate value.
     */
    private boolean aveRTTreset = false;
    
    /**
     * Number of ACK message received.
     */
    private AtomicInteger numACKS = new AtomicInteger(0);
    
    /**
     * When to start computing aveRTT
     */
    private int rttThreshold = 0;
    
    /**
     * Retry Time Out measured in milliseconds.
     */
    private volatile long RTO = 0;
    
    /**
     * Minimum Retry Timeout measured in milliseconds.
     */
    private volatile long minRTO = initRTT * 5;
    
    /**
     * Maximum Retry Timeout measured in milliseconds.
     */
    private volatile long maxRTO = initRTT * 60;
    
    /**
     * absolute time in milliseconds of last sequential ACK.
     */
    private volatile long lastACKTime = 0;
    
    /**
     * absolute time in milliseconds of last SACK based retransmit.
     */
    private volatile long sackRetransTime = 0;
    
    // running average of receipients Input Queue
    private int nIQTests = 0;
    private int aveIQSize = 0;
    
    /**
     * Our estimation of the current free space in the remote input queue.
     */
    private volatile int mrrIQFreeSpace = 0;
    
    /**
     * Our estimation of the maximum size of the remote input queue.
     */
    private int rmaxQSize = Defs.MAXQUEUESIZE;
    
    /**
     * The flow control module.
     */
    private final FlowControl fc;
    
    /**
     * Cache of the last rwindow recommendation by fc.
     */
    private volatile int rwindow = 0;
    
    /**
     * retrans queue element
     */
    private static class RetrQElt {
        
        /**
         * sequence number of this message.
         */
        final int seqnum;
        
        /**
         * the message
         */
        final Message msg;
        
        /**
         * absolute time of original enqueuing
         */
        final long enqueuedAt;
        
        /**
         * has been marked as retransmission
         */
        int marked;
        
        /**
         * absolute time when this msg was last transmitted
         */
        long sentAt;
        
        /**
         * Constructor for the RetrQElt object
         *
         * @param seqnum sequence number
         * @param msg    the message
         */
        public RetrQElt(int seqnum, Message msg) {
            this.seqnum = seqnum;
            this.msg = msg;
            this.enqueuedAt = TimeUtils.timeNow();
            this.sentAt = this.enqueuedAt;
            this.marked = 0;
        }
    }
    
    /**
     * The collection of messages available for re-transmission.
     */
    protected final List<RetrQElt> retrQ = new ArrayList<RetrQElt>();
    
    /**
     * Constructor for the ReliableOutputStream object
     *
     * @param outgoing the outgoing object
     */
    public ReliableOutputStream(Outgoing outgoing) {
        // By default use the old behaviour: fixed fc with a rwin of 20
        this(outgoing, new FixedFlowControl(20));
    }
    
    /**
     * Constructor for the ReliableOutputStream object
     *
     * @param outgoing the outgoing object
     * @param fc       flow-control
     */
    public ReliableOutputStream(Outgoing outgoing, FlowControl fc) {
        this.outgoing = outgoing;
        
        // initial RTO is set to maxRTO so as to give time
        // to the receiver to catch-up
        this.RTO = maxRTO;
        
        this.mrrIQFreeSpace = rmaxQSize;
        this.rttThreshold = rmaxQSize;
        
        // Init last ACK Time to now
        this.lastACKTime = TimeUtils.timeNow();
        this.sackRetransTime = TimeUtils.timeNow();
        
        // Attach the flowControl module
        this.fc = fc;
        
        // Update our initial rwindow to reflect fc's initial value
        this.rwindow = fc.getRwindow();
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void close() throws IOException {
        flush();
        
        super.close();
        localClosed = true;
        closedAt = TimeUtils.toRelativeTimeMillis(lingerDelay);
        
        synchronized (retrQ) {
            retrQ.notifyAll();
        }
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Closed.");
        }
    }
    
    public long getLingerDelay() {
        return lingerDelay;
    }
    
    public void setLingerDelay(long linger) {
        if (linger < 0) {
            throw new IllegalArgumentException("Linger delay may not be negative.");
        }
        
        if (0 == linger) {
            linger = Long.MAX_VALUE;
        }
        
        lingerDelay = linger;
    }
    
    /**
     * Return the size of the buffers we are using for accumulating writes.
     *
     * @return size of our write buffers.
     */
    public int setSendBufferSize() {
        return writeBufferSize;
    }
    
    /**
     * Set the size of the buffers we will use for accumulating writes.
     *
     * @param size The desired size of write buffers.
     * @throws IOException if an I/O error occurs. In particular, an IOException is thrown if the output stream is closed.
     */
    public void setSendBufferSize(int size) throws IOException {
        if (size <= 0) {
            throw new IllegalArgumentException("Send buffer size may not be <= 0");
        }
        
        // Flush any existing buffered writes. Then next write will use the new buffer size.
        synchronized (writeLock) {
            flushBuffer();
            writeBufferSize = size;
        }
    }
    
    /**
     * We have received a close request from the remote peer. We must stop
     * retransmissions immediately.
     */
    public void hardClose() {
        remoteClosed = true;
        closedAt = TimeUtils.timeNow();
        
        // Clear the retry queue. Remote side doesn't care.
        synchronized (retrQ) {
            retrQ.clear();
            retrQ.notifyAll();
        }
        
        // Clear the write queue. Remote side doesn't care.
        synchronized (writeLock) {
            writeCount = 0;
            writeBuffer = null;
            writeBufferAge = Long.MAX_VALUE;
        }
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Hard closed.");
        }
    }
    
    /**
     * Returns the state of the stream
     *
     * @return true if closed
     */
    public boolean isClosed() {
        return localClosed || remoteClosed;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void flush() throws IOException {
        synchronized (writeLock) {
            flushBuffer();
        }
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void write(int b) throws IOException {
        write(new byte[] { (byte) b }, 0, 1);
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void write(byte b[], int off, int len) throws IOException {
        synchronized (writeLock) {
            if (isClosed()) {
                throw new IOException("stream is closed");
            }
            
            if ((off < 0) || (off > b.length) || (len < 0) || ((off + len) > b.length) || ((off + len) < 0)) {
                throw new IndexOutOfBoundsException();
            }
            
            if (len == 0) {
                return;
            }
            
            int current = off;
            int end = current + len;
            
            while (current < end) {
                if (0 == writeCount) {
                    // No bytes written? We need a new buffer.
                    writeBuffer = new byte[writeBufferSize];
                    writeBufferAge = TimeUtils.timeNow();
                }
                
                int remain = end - current;
                
                int available = writeBuffer.length - writeCount;
                int copy = Math.min(available, remain);
                
                System.arraycopy(b, current, writeBuffer, writeCount, copy);
                writeCount += copy;
                current += copy;
                
                if (writeBuffer.length == writeCount) {
                    flushBuffer();
                }
            }
        }
    }
    
    /**
     * Flush the internal buffer. {@code writeLock} must have been previously
     * acquired.
     * @throws IOException if an I/O error occurs. In particular, an IOException is thrown if the output stream is closed.
     */
    private void flushBuffer() throws IOException {
        if (writeCount > 0) {
            // send the message
            try {
                writeBuffer(writeBuffer, 0, writeCount);
            } finally {
                writeCount = 0;
                writeBuffer = null;
                writeBufferAge = Long.MAX_VALUE;
            }
        }
    }
    
    /**
     * Write the internal buffer. {@code writeLock} must have been previously
     * acquired.
     *
     * @param b data
     * @param off  the start offset in the data.
     * @param len     the number of bytes to write.
     * @throws IOException if an I/O error occurs. In particular, an IOException is thrown if the output stream is closed.
     */
    private void writeBuffer(byte[] b, int off, int len) throws IOException {
        if ((off < 0) || (off > b.length) || (len < 0) || ((off + len) > b.length) || ((off + len) < 0)) {
            throw new IndexOutOfBoundsException();
        }
        
        if (len == 0) {
            return;
        }
        
        if (null == retrThread) {
            retrThread = new Thread(new Retransmitter(), "JXTA Reliable Retransmiter for " + this);
            retrThread.setDaemon(true);
            retrThread.start();
        }
        
        // allocate new message
        Message jmsg = new Message();

        synchronized (retrQ) {
            while (true) {
                if (isClosed()) {
                    throw new IOException("Connection is " + (localClosed ? "closing" : "closed"));
                }
                if (retrQ.size() > Math.min(rwindow, mrrIQFreeSpace * 2)) {
                    try {
                        retrQ.wait(1000);
                    } catch (InterruptedException ignored) {// ignored
                    }
                    continue;
                }
                break;
            }
            
            int sequenceToUse = sequenceNumber.incrementAndGet();
            MessageElement element = new ByteArrayMessageElement(Integer.toString(sequenceToUse), Defs.MIME_TYPE_BLOCK, b, off
                    ,
                    len, null);

            jmsg.addMessageElement(Defs.NAMESPACE, element);
            RetrQElt retrQel = new RetrQElt(sequenceToUse, jmsg.clone());
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Reliable WRITE : seqn#" + sequenceNumber + " length=" + len);
            }
            
            // place copy on retransmission queue
            retrQ.add(retrQel);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Retrans Enqueue added seqn#" + sequenceNumber + " retrQ.size()=" + retrQ.size());
            }
        }
        
        outgoing.send(jmsg);
        mrrIQFreeSpace--;
        // assume we have now taken a slot
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("SENT : seqn#" + sequenceNumber + " length=" + len);
        }
    }
    
    /**
     * Serialize a JXTA message as a reliable message.
     *
     * <p/>This method bypasses the built-in buffering and ignores the MTU size.
     *
     * @param msg message to send
     * @return message sequence number
     * @throws IOException if an I/O error occurs
     */
    public int send(Message msg) throws IOException {
        WireFormatMessage msgSerialized = WireFormatMessageFactory.toWire(msg, Defs.MIME_TYPE_MSG, null);
        ByteArrayOutputStream baos = new ByteArrayOutputStream((int) msgSerialized.getByteLength());

        msgSerialized.sendToStream(baos);
        baos.close();
        byte[] bytes = baos.toByteArray();
        
        synchronized (writeLock) {
            flushBuffer();
            writeBuffer(bytes, 0, bytes.length);
            return sequenceNumber.get();
        }
    }
    
    /**
     * Gets the maxAck attribute of the ReliableOutputStream object
     *
     * @return The maxAck value
     */
    public int getMaxAck() {
        return maxACK;
    }
    
    /**
     * Gets the seqNumber attribute of the ReliableOutputStream object
     *
     * @return The seqNumber value
     */
    public int getSeqNumber() {
        return sequenceNumber.get();
    }
    
    /**
     * Gets the queueFull attribute of the ReliableOutputStream object
     *
     * @return The queueFull value
     */
    protected boolean isQueueFull() {
        return mrrIQFreeSpace < 1;
    }
    
    /**
     * Gets the queueEmpty attribute of the ReliableOutputStream object.
     *
     * @return {@code true} if the queue is empty otherwise {@code false}.
     */
    public boolean isQueueEmpty() {
        synchronized (retrQ) {
            return retrQ.isEmpty();
        }
    }
    
    /**
     * Waits for the retransmit queue to become empty.
     *
     * @param timeout The relative time in milliseconds to wait for the queue to
     *                become empty.
     * @return {@code true} if the queue is empty otherwise {@code false}.
     * @throws InterruptedException if interrupted
     */
    public boolean waitQueueEmpty(long timeout) throws InterruptedException {
        long timeoutAt = TimeUtils.toAbsoluteTimeMillis(timeout);
        
        synchronized (retrQ) {
            while (!retrQ.isEmpty() && (TimeUtils.timeNow() < timeoutAt)) {
                long sleepTime = TimeUtils.toRelativeTimeMillis(timeoutAt);
                
                if (sleepTime > 0) {
                    retrQ.wait(sleepTime);
                }
            }
            
            return retrQ.isEmpty();
        }
    }
    
    /**
     * wait for activity on the retry queue
     *
     * @param timeout timeout in millis
     * @throws InterruptedException when interrupted
     */
    public void waitQueueEvent(long timeout) throws InterruptedException {
        synchronized (retrQ) {
            retrQ.wait(timeout);
        }
    }
    
    /**
     * Calculates a message retransmission time-out
     *
     * @param dt        base time
     * @param msgSeqNum Message sequence number
     */
    private void calcRTT(long dt, int msgSeqNum) {
        
        if (numACKS.incrementAndGet() == 1) {
            // First ACK arrived. We can start computing aveRTT on the messages
            // we send from now on.
            rttThreshold = sequenceNumber.get() + 1;
        }
        
        if (msgSeqNum > rttThreshold) {
            // Compute only when it has stabilized a bit
            // Since the initial mrrIQFreeSpace is small; the first few
            // messages will be sent early on and may wait a long time
            // for the return channel to initialize. After that things
            // start flowing and RTT becomes relevant.
            // Carefull with the computation: integer division with round-down
            // causes cumulative damage: the ave never goes up if this is not
            // taken care of. We keep the reminder from one round to the other.
            
            if (!aveRTTreset) {
                aveRTT = dt;
                aveRTTreset = true;
            } else {
                long tmp = (8 * aveRTT) + ((8 * remRTT) / 9) + dt;

                aveRTT = tmp / 9;
                remRTT = tmp - aveRTT * 9;
            }
        }
        
        // Set retransmission time out: 2.5 x RTT
        // RTO = (aveRTT << 1) + (aveRTT >> 1);
        RTO = aveRTT * 2;
        
        // Enforce a min/max
        RTO = Math.max(RTO, minRTO);
        RTO = Math.min(RTO, maxRTO);
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("RTT = " + dt + "ms aveRTT = " + aveRTT + "ms" + " RTO = " + RTO + "ms");
        }
    }
    
    /**
     * @param iq Description of the Parameter
     * @return Description of the Return Value
     */
    private int calcAVEIQ(int iq) {
        int n = nIQTests;

        nIQTests += 1;
        aveIQSize = ((n * aveIQSize) + iq) / nIQTests;
        return aveIQSize;
    }
    
    /**
     * process an incoming message
     *
     * @param msg message to process
     */
    public void recv(Message msg) {
        
        Iterator<MessageElement> eachACK = msg.getMessageElements(Defs.NAMESPACE, Defs.MIME_TYPE_ACK);
        
        while (eachACK.hasNext()) {
            MessageElement elt = eachACK.next();

            eachACK.remove();
            int sackCount = ((int) elt.getByteLength() / 4) - 1;
            
            try {
                DataInputStream dis = new DataInputStream(elt.getStream());
                int seqack = dis.readInt();
                int[] sacs = new int[sackCount];

                for (int eachSac = 0; eachSac < sackCount; eachSac++) {
                    sacs[eachSac] = dis.readInt();
                }
                Arrays.sort(sacs);
                // take care of the ACK here;
                ackReceived(seqack, sacs);
            } catch (IOException failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure processing ACK", failed);
                }
            }
        }
    }
    
    /**
     * Process an ACK Message. We remove ACKed
     * messages from the retry queue.  We only
     * acknowledge messages received in sequence.
     * <p/>
     * The seqnum is for the largest unacknowledged seqnum
     * the recipient has received.
     * <p/>
     * The sackList is a sequence of all of the
     * received messages in the sender's input Q. All
     * will be sequence numbers higher than the
     * sequential ACK seqnum.
     * <p/>
     * Recipients are passive and only ack upon the
     * receipt of an in sequence message.
     * <p/>
     * They depend on our RTO to fill holes in message
     * sequences.
     *
     * @param seqnum   message sequence number
     * @param sackList array of message sequence numbers
     */
    public void ackReceived(int seqnum, int[] sackList) {
        
        int numberACKed = 0;
        long rttCalcDt = 0;
        int rttCalcSeqnum = -1;
        long fallBackDt = 0;
        int fallBackSeqnum = -1;
        
        // remove acknowledged messages from retrans Q.
        synchronized (retrQ) {
            lastACKTime = TimeUtils.timeNow();
            fc.ackEventBegin();
            maxACK = Math.max(maxACK, seqnum);
            
            // dump the current Retry queue and the SACK list
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                StringBuilder dumpRETRQ = new StringBuilder("ACK RECEIVE : " + Integer.toString(seqnum));
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    dumpRETRQ.append('\n');
                }
                dumpRETRQ.append("\tRETRQ (size=").append(retrQ.size()).append(")");
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    dumpRETRQ.append(" : ");
                    for (int y = 0; y < retrQ.size(); y++) {
                        if (0 != y) {
                            dumpRETRQ.append(", ");
                        }
                        RetrQElt r = retrQ.get(y);
                        
                        dumpRETRQ.append(r.seqnum);
                    }
                }
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    dumpRETRQ.append('\n');
                }
                dumpRETRQ.append("\tSACKLIST (size=").append(sackList.length).append(")");
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    dumpRETRQ.append(" : ");
                    for (int y = 0; y < sackList.length; y++) {
                        if (0 != y) {
                            dumpRETRQ.append(", ");
                        }
                        dumpRETRQ.append(sackList[y]);
                    }
                }
                LOG.fine(dumpRETRQ.toString());
            }
            
            Iterator<RetrQElt> eachRetryQueueEntry = retrQ.iterator();

            // First remove monotonically increasing seq#s in retrans vector
            while (eachRetryQueueEntry.hasNext()) {
                RetrQElt retrQElt = eachRetryQueueEntry.next();

                if (retrQElt.seqnum > seqnum) {
                    break;
                }
                // Acknowledged
                eachRetryQueueEntry.remove();
                
                // Update RTT, RTO. Use only those that where acked
                // w/o retrans otherwise the number may be phony (ack
                // of first xmit received just after resending => RTT
                // seems small).  Also, we keep the worst of the bunch
                // we encounter.  If we really can't find a single
                // non-resent message, we make do with a pessimistic
                // approximation: we must not be left behind with an
                // RTT that's too short, we'd keep resending like
                // crazy.
                long enqueuetime = retrQElt.enqueuedAt;
                long dt = TimeUtils.toRelativeTimeMillis(lastACKTime, enqueuetime);

                // Update RTT, RTO
                if (retrQElt.marked == 0) {
                    if (dt > rttCalcDt) {
                        rttCalcDt = dt;
                        rttCalcSeqnum = retrQElt.seqnum;
                    }
                } else {
                    // In case we find no good candidate, make
                    // a guess by dividing by the number of attempts
                    // and keep the worst of them too. Since we
                    // know it may be too short, we will not use it
                    // if shortens rtt.
                    dt /= (retrQElt.marked + 1);
                    if (dt > fallBackDt) {
                        fallBackDt = dt;
                        fallBackSeqnum = retrQElt.seqnum;
                    }
                }
                fc.packetACKed(retrQElt.seqnum);
                retrQElt = null;
                numberACKed++;
            }
            // Update last accessed time in response to getting seq acks.
            if (numberACKed > 0) {
                outgoing.setLastAccessed(TimeUtils.timeNow());
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("SEQUENTIALLY ACKD SEQN = " + seqnum + ", (" + numberACKed + " acked)");
            }
            // most recent remote IQ free space
            mrrIQFreeSpace = rmaxQSize - sackList.length;
            // let's look at average sacs.size(). If it is big, then this
            // probably means we must back off because the system is slow.
            // Our retrans Queue can be large and we can overwhelm the
            // receiver with retransmissions.
            // We will keep the rwin <= ave real input queue size.
            int aveIQ = calcAVEIQ(sackList.length);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("remote IQ free space = " + mrrIQFreeSpace + " remote avg IQ occupancy = " + aveIQ);
            }
            
            int retrans = 0;

            if (sackList.length > 0) {
                Iterator<RetrQElt> eachRetrQElement = retrQ.iterator();
                int currentSACK = 0;

                while (eachRetrQElement.hasNext()) {
                    RetrQElt retrQElt = eachRetrQElement.next();

                    while (sackList[currentSACK] < retrQElt.seqnum) {
                        currentSACK++;
                        if (currentSACK == sackList.length) {
                            break;
                        }
                    }
                    if (currentSACK == sackList.length) {
                        break;
                    }
                    if (sackList[currentSACK] == retrQElt.seqnum) {
                        fc.packetACKed(retrQElt.seqnum);
                        numberACKed++;
                        eachRetrQElement.remove();
                        
                        // Update RTT, RTO. Use only those that where acked w/o retrans
                        // otherwise the number is completely phony.
                        // Also, we keep the worst of the bunch we encounter.
                        long enqueuetime = retrQElt.enqueuedAt;
                        long dt = TimeUtils.toRelativeTimeMillis(lastACKTime, enqueuetime);

                        // Update RTT, RTO
                        if (retrQElt.marked == 0) {
                            if (dt > rttCalcDt) {
                                rttCalcDt = dt;
                                rttCalcSeqnum = retrQElt.seqnum;
                            }
                        } else {
                            // In case we find no good candidate, make
                            // a guess by dividing by the number of attempts
                            // and keep the worst of them too. Since we
                            // know it may be too short, we will not use it
                            // if shortens rtt.
                            dt /= (retrQElt.marked + 1);
                            if (dt > fallBackDt) {
                                fallBackDt = dt;
                                fallBackSeqnum = retrQElt.seqnum;
                            }
                        }
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("SACKD SEQN = " + retrQElt.seqnum);
                        }
                        
                        // GC this stuff
                        retrQElt = null;
                        
                    } else {
                        // Retransmit? Only if there is a hole in the selected
                        // acknowledgement list. Otherwise let RTO deal.
                        
                        // Given that this SACK acknowledged messages still
                        // in the retrQ:
                        // seqnum is the max consectively SACKD message.
                        // seqnum < retrQElt.seqnum means a message has not reached
                        // receiver. EG: sacklist == 10,11,13 seqnum == 11
                        // We retransmit 12.
                        if (seqnum < retrQElt.seqnum) {
                            fc.packetMissing(retrQElt.seqnum);
                            retrans++;
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("RETR: Fill hole, SACK, seqn#" + retrQElt.seqnum + ", Window =" + retrans);
                            }
                        }
                    }
                }
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("SELECTIVE ACKD (" + numberACKed + ") " + retrans + " retrans wanted");
                }
            }
            
            // Compute aveRTT on the most representative message,
            // if any. That's the most accurate data.
            // Failing that we use the fall back, provided that it not
            // more recent than aveRTT ago - that would decrease aveRTT
            // and in the absence of solid data, we do not want to take
            // that risk.
            if (rttCalcSeqnum != -1) {
                calcRTT(rttCalcDt, rttCalcSeqnum);
                // get fc to recompute rwindow
                rwindow = fc.ackEventEnd(rmaxQSize, aveRTT, rttCalcDt);
            } else if ((fallBackSeqnum != -1) && (fallBackDt > aveRTT)) {
                calcRTT(fallBackDt, fallBackSeqnum);
                // get fc to recompute rwindow
                rwindow = fc.ackEventEnd(rmaxQSize, aveRTT, fallBackDt);
            }
            retrQ.notifyAll();
        }
    }
    
    /**
     * retransmit unacknowledged  messages
     *
     * @param rwin        max number of messages to retransmit
     * @param triggerTime base time
     * @return number of messages retransmitted.
     */
    private int retransmit(int rwin, long triggerTime) {
        
        List<RetrQElt> retransMsgs = new ArrayList<RetrQElt>();
        
        int numberToRetrans;
        
        // build a list of retries.
        synchronized (retrQ) {
            numberToRetrans = Math.min(retrQ.size(), rwin);
            if (numberToRetrans > 0 && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Number of messages pending retransmit =" + numberToRetrans);
            }
            for (int j = 0; j < numberToRetrans; j++) {
                RetrQElt r = retrQ.get(j);

                // Mark message as retransmission
                // need to know if a msg was retr or not for RTT eval
                if (r.marked == 0) {
                    // First time: we're here because this message has not arrived, but
                    // the next one has. It may be an out of order message.
                    // Experience shows that such a message rarely arrives older than
                    // 1.2 * aveRTT. Beyond that, it's lost. It is also rare that we
                    // detect a hole within that delay. So, often enough, as soon as
                    // a hole is detected, it's time to resend...but not always.
                    if (TimeUtils.toRelativeTimeMillis(triggerTime, r.sentAt) < (6 * aveRTT) / 5) {
                        // Nothing to worry about, yet.
                        continue;
                    }
                } else {
                    // That one has been retransmitted at least once already.
                    // So, we don't have much of a clue other than the age of the
                    // last transmission. It is unlikely that it arrives before aveRTT/2
                    // but we have to anticipate its loss at the risk of making dupes.
                    // Otherwise the receiver will reach the hole, and that's really
                    // expensive. (Think that we've been trying for a while already.)
                    
                    if (TimeUtils.toRelativeTimeMillis(triggerTime, r.sentAt) < aveRTT) {
                        // Nothing to worry about, yet.
                        continue;
                    }
                }
                r.marked++;
                // Make a copy to for sending
                retransMsgs.add(r);
            }
        }
        
        // send the retries.
        int retransmitted = 0;
        Iterator<RetrQElt> eachRetrans = retransMsgs.iterator();

        while (eachRetrans.hasNext()) {
            RetrQElt r = eachRetrans.next();

            eachRetrans.remove();
            try {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("RETRANSMIT seqn#" + r.seqnum);
                }
                Message sending = r.msg;

                // its possible that the message was
                // acked while we were working in this
                // case r.msg will have been nulled.
                if (null != sending) {
                    sending = sending.clone();
                    sending.replaceMessageElement(Defs.NAMESPACE, RETELT);
                    if (outgoing.send(sending)) {
                        r.sentAt = TimeUtils.timeNow();
                        mrrIQFreeSpace--;
                        // assume we have now taken a slot
                        retransmitted++;
                    } else {
                        break;
                        // don't bother continuing sending now.
                    }
                }
            } catch (IOException e) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "FAILED RETRANS seqn#" + r.seqnum, e);
                }
                break;
                // don't bother continuing.
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("RETRANSMITED " + retransmitted + " of " + numberToRetrans);
        }
        
        return retransmitted;
    }
    
    /**
     * Retransmission daemon thread
     */
    private class Retransmitter implements Runnable {
        
        int nAtThisRTO = 0;
        volatile int nretransmitted = 0;
        
        /**
         * Constructor for the Retransmitter object
         */
        public Retransmitter() {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("STARTED Reliable Retransmitter, RTO = " + RTO);
            }
        }
        
        /**
         * Gets the retransCount attribute of the Retransmitter object
         *
         * @return The retransCount value
         */
        public int getRetransCount() {
            return nretransmitted;
        }
        
        /**
         *  {@inheritDoc}
         *
         *  <p/>Main processing method for the Retransmitter object
         */
        public void run() {
            try {
                int idleCounter = 0;
                
                while (TimeUtils.toRelativeTimeMillis(closedAt) > 0) {
                    long conn_idle = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), outgoing.getLastAccessed());

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine(outgoing + " idle for " + conn_idle);
                    }
                    
                    // check to see if we have not idled out.
                    if (outgoing.getIdleTimeout() < conn_idle) {
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("Shutting down idle " + "connection " + outgoing);
                        }
                        
                        break;
                    }
                    
                    long sinceLastACK;
                    long oldestInQueueWait;
                    
                    synchronized (retrQ) {
                        try {
                            if (RTO > 0) {
                                retrQ.wait(RTO);
                            }
                            Thread.currentThread().setName(
                                    "JXTA Reliable Retransmiter for " + this + " Queue size : " + retrQ.size());
                        } catch (InterruptedException e) {// ignored
                        }
                        
                        if (TimeUtils.toRelativeTimeMillis(closedAt) <= 0) {
                            break;
                        }
                        
                        // see if we recently did a retransmit triggered by a SACK
                        long sinceLastSACKRetr = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), sackRetransTime);

                        if (sinceLastSACKRetr < RTO) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("SACK retrans " + sinceLastSACKRetr + "ms ago");
                            }
                            continue;
                        }
                        // See how long we've waited since RTO was set
                        sinceLastACK = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), lastACKTime);
                        
                        if (!retrQ.isEmpty()) {
                            RetrQElt elt = retrQ.get(0);

                            oldestInQueueWait = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), elt.enqueuedAt);
                        } else {
                            oldestInQueueWait = 0;
                        }
                    }
                    
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Last ACK " + sinceLastACK + "ms ago. Age of oldest in Queue " + oldestInQueueWait + "ms.");
                    }
                    
                    // see if the queue has gone dead
                    if (oldestInQueueWait > outgoing.getMaxRetryAge()) {
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("Shutting down stale connection " + outgoing);
                        }
                        
                        break;
                    }
                    
                    // get real wait as max of age of oldest in retrQ and
                    // lastAck time
                    long realWait = Math.max(oldestInQueueWait, sinceLastACK);

                    // Retransmit only if RTO has expired.
                    // a. real wait time is longer than RTO
                    // b. oldest message on Q has been there longer
                    // than RTO. This is necessary because we may
                    // have just sent a message, and we do not
                    // want to overrun the receiver. Also, we
                    // do not want to restransmit a message that
                    // has not been idle for the RTO.
                    if ((realWait >= RTO) && (oldestInQueueWait >= RTO)) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("RTO RETRANSMISSION [" + rwindow + "]");
                        }
                        // retransmit
                        int retransed = retransmit(rwindow, TimeUtils.timeNow());

                        // Total
                        nretransmitted += retransed;
                        // number at this RTO
                        nAtThisRTO += retransed;
                        // See if real wait is too long and queue is non-empty
                        // Remote may be dead - double until max.
                        // Double after window restransmitted msgs at this RTO
                        // exceeds the rwindow, and we've had no response for
                        // twice the current RTO.
                        if ((retransed > 0) && (realWait >= 2 * RTO) && (nAtThisRTO >= 2 * rwindow)) {
                            RTO = (realWait > maxRTO ? maxRTO : 2 * RTO);
                            nAtThisRTO = 0;
                        }
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(
                                    "RETRANSMISSION " + retransed + " retrans " + nAtThisRTO + " at this RTO (" + RTO + ") "
                                    + nretransmitted + " total retrans");
                        }
                    } else {
                        idleCounter += 1;
                        // reset RTO to min if we are idle
                        if (idleCounter == 2) {
                            RTO = minRTO;
                            idleCounter = 0;
                            nAtThisRTO = 0;
                        }
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("IDLE : RTO=" + RTO + " WAIT=" + realWait);
                        }
                    }
                }
            } catch (Throwable all) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            } finally {
                hardClose();
                
                retrThread = null;
                
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("STOPPED Retransmit thread");
                }
            }
        }
    }
}

