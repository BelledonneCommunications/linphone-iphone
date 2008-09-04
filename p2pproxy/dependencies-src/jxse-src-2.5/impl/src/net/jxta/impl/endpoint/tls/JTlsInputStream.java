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

package net.jxta.impl.endpoint.tls;


import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.net.SocketTimeoutException;
import java.io.InterruptedIOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.impl.util.TimeUtils;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 *  Acts as the input for TLS. Accepts ciphertext which arrives in messages
 *  and orders it before passing it to TLS for decryption.
 *
 * TLS will do its raw reads off of this InputStream
 * Here, we will have queued up the payload of TLS message
 * elements to be passed to TLS code as TLS Records.
 *
 */
class JTlsInputStream extends InputStream {
    private static final Logger LOG = Logger.getLogger(JTlsInputStream.class.getName());
    
    private static final boolean  DEBUGIO = false;
    
    static private int MAXQUEUESIZE = 25;
    
    /**
     *  Connection we are working for.
     */
    private TlsConn conn;
    
    private volatile boolean closed = false;
    private boolean closing = false;
    
    private long timeout = 2 * TimeUtils.AMINUTE;
    private JTlsRecord jtrec = null;
    private volatile int sequenceNumber = 0;
    private final Vector<IQElt> inputQueue = new Vector<IQElt>(MAXQUEUESIZE); // For incoming messages.
    
    /**
     * Input TLS record Object
     **/
    private static class JTlsRecord {
        // This dummy message elt
        public InputStream tlsRecord; // TLS Record
        public long nextByte; // next inbuff byte
        public long size; // size of TLS Record
        
        public JTlsRecord() {
            tlsRecord = null; // allocated by caller
            nextByte = 0; // We read here (set by caller)
            size = 0; // TLS Record size(set by caller)
        }
        
        // reset the jxta tls record element
        
        public void resetRecord() {
            if (null != tlsRecord) {
                try {
                    tlsRecord.close();
                } catch (IOException ignored) {// ignored
                }
            }
            tlsRecord = null;
            size = nextByte = 0;
        }
    }
    
    
    // An input queue element which breaks out a
    // received message in enqueueMessage().
    private static class IQElt {
        int seqnum;
        MessageElement elt;
        boolean ackd;
    }
    
    public JTlsInputStream(TlsConn conn, long timeout) {
        this.timeout = timeout;
        this.conn = conn;
        jtrec = new JTlsRecord();
        // 1 <= seq# <= maxint, monotonically increasing
        // Incremented before compare.
        sequenceNumber = 0;
        
    }
    
    /**
     * {@inheritDoc}
     **/
    @Override
    public void close() throws IOException {
        super.close();
        
        closed = true;
        synchronized (inputQueue) {
            inputQueue.clear();
            inputQueue.notifyAll();
        }
    }

    /**
     * prepare this input stream to being closed. It will still
     * deliver the packets that have been received, but nothing
     * more. This is meant to be called in response to the other side
     * having initiated closure. We assume that when the other side does it
     * it means that it is satified with what we have acknoleged so far.
     */
    public void setClosing() throws IOException {
        synchronized (inputQueue) {
            closing = true;
            inputQueue.notifyAll();
        }
    }
    
    // Here we read the TLS Record data from the incoming JXTA message.
    // (We will really have a full jxta message available.)
    //
    // TLS  Record input only calls the following  methods.
    // They are called from SSLRecord.decode(SSLConn, Inputstream);
    //
    
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
                if (DEBUGIO && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Read() : " + (a[0] & 255));
                }
                
                return (a[0] & 0xFF); // The byte
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
        
        if (DEBUGIO && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Read(byte[], int, " + length + "), bytes read = " + i);
        }
        
        // If we've reached EOF; there's nothing to do but close().
        if (i == -1) {
            close();
        }
        return i;
    }
    
    // protected accessor for sequence number
    int getSequenceNumber() {
        return sequenceNumber;
    }
    
    // Our input queue max size
    int getMaxIQSize() {
        return MAXQUEUESIZE;
    }
    
    /**
     *  Send a sequential ACK and selective ACKs for all of the queued messages.
     *
     *  @param seqnAck the sequence number being sequential ACKed
     **/
    private void sendACK(int seqnAck) {
        List<Integer> selectedAckList = new ArrayList<Integer>();
        
        synchronized (inputQueue) {
            Iterator<IQElt> eachInQueue = inputQueue.iterator();
            
            while (eachInQueue.hasNext() && (selectedAckList.size() < MAXQUEUESIZE)) {
                IQElt anIQElt = eachInQueue.next();

                if (anIQElt.seqnum > seqnAck) {
                    selectedAckList.add(new Integer(anIQElt.seqnum));
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
            
            Iterator<Integer> eachSACK = sackList.iterator();
            
            while (eachSACK.hasNext()) {
                int aSack = (eachSACK.next()).intValue();

                dos.writeInt(aSack);
            }
            dos.close();
            bos.close();
            
            Message ACKMsg = new Message();
            MessageElement elt = new ByteArrayMessageElement(JTlsDefs.ACKKEY, JTlsDefs.ACKS, bos.toByteArray(), null);
            
            ACKMsg.addMessageElement(JTlsDefs.TLSNameSpace, elt);
            
            conn.sendToRemoteTls(ACKMsg);
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("SENT ACK, seqn#" + seqnAck + " and " + sackList.size() + " SACKs ");
            }
        } catch (IOException e) {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.log(Level.INFO, "sendACK caught IOException:", e);
            }
        }
    }
    
    /**
     *  queue messages by sequence number.
     */
    public void queueIncomingMessage(Message msg) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Queue Incoming Message begins for " + msg);
        }
        
        long startEnqueue = TimeUtils.timeNow();
        
        Message.ElementIterator e = msg.getMessageElements(JTlsDefs.TLSNameSpace, JTlsDefs.BLOCKS);
        
        // OK look for jxta message
        while (!closed && !closing && e.hasNext()) {
            MessageElement elt = e.next();

            e.remove();
            
            int msgSeqn = 0;
            
            try {
                msgSeqn = Integer.parseInt(elt.getElementName());
            } catch (NumberFormatException n) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Discarding element (" + elt.getElementName() + ") Not one of ours.");
                }
                continue;
            }
            
            IQElt newElt = new IQElt();
            
            newElt.seqnum = msgSeqn;
            newElt.elt = elt;
            newElt.ackd = false;
            
            // OK we must inqueue:
            // Wait until someone dequeues if we are at the size limit
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
                    IQElt iq = inputQueue.elementAt(j);

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
                        LOG.fine("RCVD OLD MESSAGE : Discard duplicate msg, seqn#" + newElt.seqnum);
                    }
                    newElt = null;
                    break;
                }
                
                inputQueue.add(insertIndex, newElt);
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Enqueued msg with seqn#" + newElt.seqnum + " at index " + insertIndex);
                }
                
                inputQueue.notifyAll();
                newElt = null;
            }
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            long waited = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), startEnqueue);
            
            LOG.fine("Queue Incoming Message for " + msg + " completed in " + waited + " msec.");
        }
    }
    
    /**
     *  Dequeue the message with the desired sequence number waiting as needed
     *  until the message is available.
     *
     *  @param desiredSeqn the sequence number to be dequeued.
     *  @return the Message Element with the desired sequence number or null if
     *  the queue has been closed.
     **/
    private MessageElement dequeueMessage(int desiredSeqn) throws IOException {
        IQElt iQ = null;
        
        // Wait for incoming message here
        long startDequeue = TimeUtils.timeNow();
        long whenToTimeout = startDequeue + timeout;
        int wct = 0;
        
        long nextRetransRequest = TimeUtils.toAbsoluteTimeMillis(TimeUtils.ASECOND);
        
        synchronized (inputQueue) {
            while (!closed) {
                if (inputQueue.size() == 0) {
                    if (closing) {
                        return null;
                    }
                    try {
                        wct++;
                        inputQueue.wait(TimeUtils.ASECOND);
                        if (whenToTimeout < TimeUtils.timeNow()) {
                            throw new SocketTimeoutException("Read timeout reached");
                        }
                    } catch (InterruptedException e) {
                        Thread.interrupted(); // just continue
                    }
                    // we reset the retrans request timer since we don't want to
                    // immediately request retry after a long wait for out of
                    // order messages.
                    
                    nextRetransRequest = TimeUtils.toAbsoluteTimeMillis(TimeUtils.ASECOND);
                    continue;
                }
                
                iQ = inputQueue.elementAt(0); // FIFO
                
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
                    
                    try {
                        wct++;
                        inputQueue.wait(TimeUtils.ASECOND);
                        if (whenToTimeout < TimeUtils.timeNow()) {
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
        sendACK(desiredSeqn);
        // if we are closed then we return null
        if (null == iQ) {
            return null;
        }
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            long waited = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), startDequeue);
            
            LOG.info("DEQUEUED seqn#" + iQ.seqnum + " in " + waited + " msec on input queue");
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                if (wct > 0) {
                    LOG.fine("DEQUEUE waited " + wct + " times on input queue");
                }
            }
        }
        
        return iQ.elt;
    }
    
    /**
     *
     */
    private int local_read(byte[] a, int offset, int length) throws IOException {
        
        synchronized (jtrec) {
            if ((jtrec.size == 0) || (jtrec.nextByte == jtrec.size)) {
                
                // reset the record
                jtrec.resetRecord(); // GC as necessary(tlsRecord byte[])
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("local_read: getting next data block at seqn#" + (sequenceNumber + 1));
                }
                
                MessageElement elt = null;

                try {
                    elt = dequeueMessage(sequenceNumber + 1);
                } catch (SocketTimeoutException ste) {
                    // timed out with no data
                    // SSLSocket expects a 0 data in this case
                    return 0;
                }
                
                if (null == elt) {
                    return -1;
                }
                
                sequenceNumber += 1; // next msg sequence number
                
                // Get the length of the TLS Record
                jtrec.size = elt.getByteLength();
                jtrec.tlsRecord = elt.getStream();
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("local_read: new seqn#" + sequenceNumber + ", bytes = " + jtrec.size);
                }
            }
            
            // return the requested TLS Record data
            // These calls should NEVER ask for more data than is in the
            // received TLS Record.
            
            long left = jtrec.size - jtrec.nextByte;
            int copyLen = (int) Math.min(length, left);
            int copied = 0;
            
            do {
                int res = jtrec.tlsRecord.read(a, offset + copied, copyLen - copied);
                
                if (res < 0) {
                    break;
                }
                
                copied += res;
            } while (copied < copyLen);
            
            jtrec.nextByte += copied;
            
            if (DEBUGIO) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("local_read: Requested " + length + ", Read " + copied + " bytes");
                }
            }
            
            return copied;
        }
    }
}
