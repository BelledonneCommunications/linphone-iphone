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


import java.io.OutputStream;
import java.io.IOException;
import java.net.*;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;

import net.jxta.impl.endpoint.tls.TlsConn.HandshakeState;
import net.jxta.impl.util.TimeUtils;


/**
 *  Acts as the output for TLS. Accepts ciphertext from TLS and packages it into
 *  messages for sending to the remote. The messages are kept in a retry queue
 *  until the remote peer acknowledges receipt of the message.
 **/
class JTlsOutputStream extends OutputStream {

    /**
     *  Log4J Logger
     **/
    private static final Logger LOG = Logger.getLogger(JTlsOutputStream.class.getName());

    // constants

    /**
     * This maximum is only enforced if we have not heard
     * from the remote for RETRMAXAGE.
     **/
    private static final int MAXRETRQSIZE = 100;

    /**
     *  Initial estimated Round Trip Time
     **/
    private static final long initRTT = 1 * TimeUtils.ASECOND;

    private static final MessageElement RETELT = new StringMessageElement(JTlsDefs.RETR, "TLSRET", null);

    /**
     * Retrans window. When reached, we up the RTO.
     **/
    private static final int RWINDOW = 5;

    /**
     *  If true then the stream has been closed.
     **/
    private volatile boolean closed = false;

    /**
     * If true then the stream is being closed.
     * It means that it still works completely for all messages already
     * queued, but no new message may be enqueued.
     **/
    private volatile boolean closing = false;

    /**
     *  Sequence number of the message we most recently sent out.
     **/
    private volatile int sequenceNumber = 0;

    /**
     *  Sequence number of highest sequential ACK.
     **/
    private volatile int maxACK = 0;

    /**
     *  Transport we are working for
     **/
    private TlsTransport tp = null;

    /**
     *  connection we are working for
     **/
    private TlsConn conn = null;

    private Retransmitter retransmitter = null;

    // for retransmission

    /**
     *  Average round trip time in milliseconds.
     **/
    private volatile long aveRTT = initRTT;

    /**
     *  Number of ACK message received.
     **/
    private int nACKS = 0;

    /**
     *  Retry Time Out measured in milliseconds.
     **/
    private volatile long RTO = 0;

    /**
     *  Minimum Retry Timeout measured in milliseconds.
     **/
    private volatile long minRTO = initRTT;

    /**
     *  Maximum Retry Timeout measured in milliseconds.
     **/
    private volatile long maxRTO = initRTT * 5;

    /**
     *  absolute time in milliseconds of last sequential ACK.
     **/
    private volatile long lastACKTime = 0;

    /**
     *  absolute time in milliseconds of last SACK based retransmit.
     **/
    private volatile long sackRetransTime = 0;

    /**
     *   The collection of messages available for re-transmission.
     */
    final List<RetrQElt> retrQ = new Vector<RetrQElt>(25, 5);

    // running average of receipients Input Queue
    private int nIQTests = 0;
    private int aveIQSize = 0;

    /**
     *  Our estimation of the current free space in the remote input queue.
     **/
    private volatile int mrrIQFreeSpace = 0;

    /**
     *  Our estimation of the maximum sise of the remote input queue.
     **/
    private int rmaxQSize = 0;

    /**
     * retrans queue element
     **/
    private static class RetrQElt {
        int seqnum; // sequence number of this message.
        long enqueuedAt; // absolute time of original enqueing.
        volatile Message msg; // the message
        int marked; // has been marked as retransmission
        long sentAt; // when this msg was last transmitted

        public RetrQElt(int seqnum, Message msg) {
            this.seqnum = seqnum;
            this.msg = msg;
            this.enqueuedAt = TimeUtils.timeNow();
            this.sentAt = this.enqueuedAt;
            this.marked = 0;
        }
    }

    JTlsOutputStream(TlsTransport tp, TlsConn conn) {
        this.conn = conn; // TlsConnection.
        this.tp = tp; // our transport

        this.RTO = minRTO; // initial RTO

        // input free queue size
        this.rmaxQSize = 20;
        this.mrrIQFreeSpace = rmaxQSize;

        // Init last ACK Time to now
        this.lastACKTime = TimeUtils.timeNow();
        this.sackRetransTime = TimeUtils.timeNow();

        // Start retransmission thread
        this.retransmitter = new Retransmitter();
    }

    /**
     * {@inheritDoc}
     *
     *  <p/>We don't current support linger.
     **/
    @Override
    public void close() throws IOException {
        synchronized (this) {
            super.close();
            closed = true;
        }
        synchronized (retrQ) {
            retrQ.notifyAll();
            retrQ.clear();
        }
    }

    /**
     * indicate that we're in the process of closing. To respect the semantics
     * of close()/isClosed(), we do not set the closed flag, yet. Instead, we
     * set the flag "closing", which simply garantees that no new message
     * will be queued.
     * This, in combination with getSequenceNumber and getMaxAck, and
     * waitQevent, enables fine grain control of the tear down process.
     **/
    public void setClosing() {
        synchronized (retrQ) {
            closing = true;
            retrQ.clear();
            retrQ.notifyAll();
        }
    }

    /**
     * {@inheritDoc}
     **/
    @Override
    public void write(int c) throws IOException {
        byte[] a = new byte[1];

        a[0] = (byte) (c & 0xFF);
        write(a, 0, 1);
    }

    /**
     * {@inheritDoc}
     *
     * <p/>We override the write(byte[], offset, length);
     * method which is called by SSLRecord.send(SSLConn conn)
     * via tos.writeTo(conn.sock_out), tos a ByteArrayOutputStream
     * which has buffered the TLS output record in the byte array.
     * The actual call is write(byte[] b, 0, length);
     *
     * <p/>We put this TLS record into a msssage element for the output
     * pipe to send along.
     *
     * <p/>This is reasonable since in fact, if more than 16K bytes of
     * application data are sent, then the max TLS Record is a little
     * larger than 16K bytes including the TLS overhead.
     *
     * <p/>Therefore, an app. message is N+r TLS Records,
     * Message length = Nx16K + r, N >= 0, r >= 0,
     * N > 0 || r > 0 true.
     **/
    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        // flag to allow connection closure in finally block
        // Connection can not be closed when holding a lock on this
        boolean closeStale = false;
        // allocate new message
        Message jmsg = new Message();

        try {
            if (closed) {
                throw new IOException("stream is closed");
            }
            if (closing) {
                throw new IOException("stream is being closed");
            }
            if (b == null) {
                throw new IllegalArgumentException("buffer is null");
            }

            if ((off < 0) || (off > b.length) || (len < 0) || ((off + len) > b.length) || ((off + len) < 0)) {
                throw new IndexOutOfBoundsException();
            }

            if (len == 0) {
                return;
            }

            // Copy the data since it will be queued, and caller may
            // overwrite the same byte[] buffer.
            byte[] data = new byte[len];

            System.arraycopy(b, off, data, 0, len);

            // sync so that writes don't get out of order.
            synchronized (retrQ) {
                // add TLS record as element
                MessageElement ciphertext = new ByteArrayMessageElement(Integer.toString(++sequenceNumber), JTlsDefs.BLOCKS, data
                        ,
                        null);

                jmsg.addMessageElement(JTlsDefs.TLSNameSpace, ciphertext);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("TLS CT WRITE : seqn#" + sequenceNumber + " length=" + len);
                }

                // (1)  See if the most recent remote input queue size is close to
                // it's maximum input queue size
                // Send only if at least 20% or more of the queue is free.
                // (2) Also, if our retransQ is larger than the remotes inputQ,
                // wait until we've received an ack.
                // We assume some msgs are in transit or the remote system buffers
                // We do not want to overrun the receiver.
                // (3) We need to release from the loop because of possible deadlocks
                // EG: retrQ.size() == 0 and mrrIQFreeSpace forces looping
                // forever because the most recent SACK cleared it, and the receiver
                // is waiting for more data.

                // max of 200ms wait
                int maxwait = Math.min((int) aveRTT, 200);
                // iterations to wait (max 3, min 1)
                int waitCt = Math.max(maxwait / 60, 1);

                // check if the queue has gone dead.
                if (retrQ.size() > 0) {
                    long inQueue = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), retrQ.get(0).enqueuedAt);

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("write : Retry queue idle for " + inQueue);
                    }

                    if (inQueue > tp.RETRMAXAGE) {
                        if (inQueue > (2 * tp.RETRMAXAGE)) {
                            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                                LOG.info("Closing stale connection " + conn);
                            }
                            // SPT - set flag for connection close in finally block
                            closeStale = true;
                            throw new IOException("Stale connection closure in progress");
                        } else if (retrQ.size() >= MAXRETRQSIZE) {
                            // if the the queue is "full" and we are long idle, delay new writes forever.
                            waitCt = Integer.MAX_VALUE;
                        }
                    }
                }

                int i = 0;

                while (!closed && ((mrrIQFreeSpace < rmaxQSize / 5) || (retrQ.size() > rmaxQSize))) {

                    // see if max. wait has arrived.
                    if (i++ == waitCt) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("write() wait for ACK, maxwait timer expired while enqueuing seqn#" + sequenceNumber);
                        }
                        break;
                    }

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("write() wait 60ms for ACK while enqueuing seqn#" + sequenceNumber + "\n\tremote IQ free space = "
                                + mrrIQFreeSpace + "\n\tMIN free space to continue = " + (rmaxQSize / 5) + "" + "\n\tretQ.size()="
                                + retrQ.size());
                    }

                    // Less than 20% free queue space is left. Wait.
                    try {
                        retrQ.wait(60);
                    } catch (InterruptedException ignored) {
                        Thread.interrupted();
                    }
                }

                // place copy on retransmission queue
                RetrQElt r = new RetrQElt(sequenceNumber, jmsg.clone());

                retrQ.add(r);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Retrans Enqueue added seqn#" + sequenceNumber + " retQ.size()=" + retrQ.size());
                }
            }

            // Here we will send the message to the transport
            conn.sendToRemoteTls(jmsg);
            // assume we have now taken a slot
            mrrIQFreeSpace--;

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("TLS CT SENT : seqn#" + sequenceNumber + " length=" + len);
            }
        } finally {
            if (closeStale) {
                // The retry queue has really gone stale.
                try {
                    setClosing();
                    // in this we close ourself
                    conn.close(HandshakeState.CONNECTIONDEAD);
                } catch (IOException ignored) {
                    ;
                }
            }
        }
    }

    private void calcRTT(long enqueuedAt) {
        long dt = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), enqueuedAt);

        if (dt == 0) {
            dt += 1;
        }

        int n = nACKS;

        nACKS += 1;

        aveRTT = ((n * aveRTT) + dt) / (nACKS);

        // Set retransmission time out: 2.5 x RTT
        RTO = (aveRTT << 1) + (aveRTT >> 1);

        // Enforce a min/max

        RTO = Math.max(RTO, minRTO);
        RTO = Math.min(RTO, maxRTO);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("TLS!! RTT = " + dt + "ms aveRTT = " + aveRTT + "ms" + " RTO = " + RTO + "ms");
        }
    }

    private int calcAVEIQ(int iq) {
        int n = nIQTests;

        nIQTests += 1;

        aveIQSize = ((n * aveIQSize) + iq) / nIQTests;

        return aveIQSize;
    }

    /**
     * Process an ACK Message. We remove ACKed messages from the retry queue.
     * We only acknowledge messages received in sequence.
     *
     * The seqnum is for the largest unacknowledged seqnum
     * the receipient has received.
     *
     * The sackList is a sequence of all of the received
     *    messages in the sender's input Q. All will be sequence numbers higher
     *    than the sequential ACK seqnum.
     *
     *    Recepients are passive and only ack upon the receipt
     *    of an in sequence message.
     *
     *    They depend on our RTO to fill holes in message
     *   sequences.
     **/
    void ackReceived(int seqnum, int[] sackList) {
        lastACKTime = TimeUtils.timeNow();
        int numberACKed = 0;

        // remove acknowledged messages from retrans Q.

        synchronized (retrQ) {
            maxACK = Math.max(maxACK, seqnum);

            // dump the current Retry queue and the SACK list
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                StringBuilder dumpRETRQ = new StringBuilder("ACK RECEIVE : " + Integer.toString(seqnum));

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    dumpRETRQ.append('\n');
                }
                dumpRETRQ.append("\tRETRQ (size=" + retrQ.size() + ")");
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
                dumpRETRQ.append("\tSACKLIST (size=" + sackList.length + ")");
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

            Iterator eachRetryQueueEntry = retrQ.iterator();

            // First remove monotonically increasing seq#s in retrans vector
            while (eachRetryQueueEntry.hasNext()) {
                RetrQElt r = (RetrQElt) eachRetryQueueEntry.next();

                if (r.seqnum > seqnum) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("r.seqnum :" + r.seqnum + " > seqnum :" + seqnum);
                    }
                    break;
                }

                // Acknowledged
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("seqnum :" + seqnum);
                    LOG.fine("Removing :" + r.seqnum + " from retransmit queue");
                }
                eachRetryQueueEntry.remove();

                // Update RTT, RTO
                if (0 != r.enqueuedAt) {
                    calcRTT(r.enqueuedAt);
                }

                r.msg.clear();
                r.msg = null;
                r = null;
                numberACKed++;
            }

            // Update last accessed time in response to getting seq acks.
            if (numberACKed > 0) {
                conn.lastAccessed = TimeUtils.timeNow();
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("TLS!! SEQUENTIALLY ACKD SEQN = " + seqnum + ", (" + numberACKed + " acked)");
            }

            // most recent remote IQ free space
            rmaxQSize = Math.max(rmaxQSize, sackList.length);
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
                Iterator eachRetrQElement = retrQ.iterator();

                int currentSACK = 0;

                while (eachRetrQElement.hasNext()) {
                    RetrQElt r = (RetrQElt) eachRetrQElement.next();

                    while (sackList[currentSACK] < r.seqnum) {
                        currentSACK++;
                        if (currentSACK == sackList.length) {
                            break;
                        }
                    }

                    if (currentSACK == sackList.length) {
                        break;
                    }

                    if (sackList[currentSACK] == r.seqnum) {
                        eachRetrQElement.remove();

                        // ack counter
                        numberACKed++;

                        // for aveRTT calculation
                        long enqueuetime = r.enqueuedAt;

                        // Update RTT, RTO
                        if (enqueuetime != 0) {
                            calcRTT(enqueuetime);
                        }

                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("TLS!! SACKD SEQN = " + r.seqnum);
                        }

                        // GC this stuff
                        r.msg.clear();
                        r.msg = null;
                        r = null;

                    } else {
                        // Retransmit? Only if there is a hole in the selected
                        // acknowledgement list. Otherwise let RTO deal.
                        // Given that this SACK acknowledged messages still
                        // in the retrQ:
                        // seqnum is the max consectively SACKD message.
                        // seqnum < r.seqnum means a message has not reached
                        // receiver. EG: sacklist == 10,11,13 seqnum == 11
                        // We retransmit 12.
                        if (seqnum < r.seqnum) {
                            retrans++;

                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("RETR: Fill hole, SACK, seqn#" + r.seqnum + ", Window =" + retrans);
                            }
                        }
                    }
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("TLS!! SELECTIVE ACKD (" + numberACKed + ") " + retrans + " retrans wanted");
                }

                // retransmit 1 retq mem. only
                if (retrans > 0) {
                    retransmit(Math.min(RWINDOW, retrans), lastACKTime);
                    sackRetransTime = TimeUtils.timeNow();
                }
            }

            retrQ.notify();
        }
    }

    /**
     * retransmit unacknowledged  messages
     *
     *  @param rwin max number of messages to retransmit
     *  @return number of messages retransmitted.
     **/
    private int retransmit(int rwin, long triggerTime) {
        List retransMsgs = new ArrayList();

        int numberToRetrans;

        // build a list of retries.
        synchronized (retrQ) {
            numberToRetrans = Math.min(retrQ.size(), rwin);

            if (numberToRetrans > 0 && Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("RETRANSMITING [rwindow = " + numberToRetrans + "]");
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

        Iterator eachRetrans = retransMsgs.iterator();

        while (eachRetrans.hasNext()) {
            RetrQElt r = (RetrQElt) eachRetrans.next();

            eachRetrans.remove();

            try {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("TLS!! RETRANSMIT seqn#" + r.seqnum);
                }

                Message sending = r.msg;

                // its possible that the message was acked while we were working
                // in this case r.msg will have been nulled.
                if (null != sending) {
                    sending = sending.clone();
                    sending.replaceMessageElement(JTlsDefs.TLSNameSpace, RETELT);
                    if (conn.sendToRemoteTls(sending)) {
                        mrrIQFreeSpace--; // assume we have now taken a slot
                        retransmitted++;
                    } else {
                        break;
                    } // don't bother continuing.
                }
            } catch (IOException e) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "FAILED RETRANS seqn#" + r.seqnum, e);
                }
                break; // don't bother continuing.
            }
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("RETRANSMITED " + retransmitted + " of " + numberToRetrans);
        }

        return retransmitted;
    }

    /**
     * Retransmission daemon thread
     **/
    private class Retransmitter implements Runnable {

        Thread retransmitterThread;
        volatile int nretransmitted = 0;
        int nAtThisRTO = 0;

        public Retransmitter() {

            this.retransmitterThread = new Thread(tp.myThreadGroup, this, "JXTA TLS Retransmiter for " + conn.destAddr);
            retransmitterThread.setDaemon(true);
            retransmitterThread.start();

            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("STARTED TLS Retransmit thread, RTO = " + RTO);
            }
        }

        public int getRetransCount() {
            return nretransmitted;
        }

        /**
         *  {@inheritDoc]
         **/
        public void run() {

            try {
                int idleCounter = 0;

                while (!closed) {
                    long conn_idle = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), conn.lastAccessed);

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("RETRANS : " + conn + " idle for " + conn_idle);
                    }

                    // check to see if we have not idled out.
                    if (tp.CONNECTION_IDLE_TIMEOUT < conn_idle) {
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("RETRANS : Shutting down idle connection: " + conn);
                        }
                        try {
                            setClosing();
                            // the following call eventually closes this stream
                            conn.close(HandshakeState.CONNECTIONDEAD);
                            // Leave. Otherwise we'll be spinning forever
                            return;
                        } catch (IOException ignored) {
                            ;
                        }
                        continue;
                    }

                    synchronized (retrQ) {
                        try {
                            retrQ.wait(RTO);
                        } catch (InterruptedException e) {
                            Thread.interrupted();
                        }
                    }
                    if (closed) {
                        break;
                    }

                    // see if we recently did a retransmit triggered by a SACK
                    long sinceLastSACKRetr = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), sackRetransTime);

                    if (sinceLastSACKRetr < RTO) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("RETRANS : SACK retrans " + sinceLastSACKRetr + "ms ago");
                        }

                        continue;
                    }

                    // See how long we've waited since RTO was set
                    long sinceLastACK = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), lastACKTime);
                    long oldestInQueueWait;

                    synchronized (retrQ) {
                        if (retrQ.size() > 0) {
                            oldestInQueueWait = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), retrQ.get(0).enqueuedAt);
                        } else {
                            oldestInQueueWait = 0;
                        }
                    }

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("RETRANS : Last ACK " + sinceLastACK + "ms ago. Age of oldest in Queue " + oldestInQueueWait + "ms");
                    }

                    // see if the queue has gone dead
                    if (oldestInQueueWait > (tp.RETRMAXAGE * 2)) {
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("RETRANS : Shutting down stale connection: " + conn);
                        }
                        try {
                            setClosing();
                            conn.close(HandshakeState.CONNECTIONDEAD);
                            // Leave. Otherwise we'll be spinning forever.
                            return;
                        } catch (IOException ignored) {
                            ;
                        }
                        continue;
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
                            LOG.fine("RETRANS : RTO RETRANSMISSION [" + RWINDOW + "]");
                        }

                        // retrasmit
                        int retransed = retransmit(RWINDOW, TimeUtils.timeNow());

                        // Total
                        nretransmitted += retransed;

                        // number at this RTO
                        nAtThisRTO += retransed;

                        // See if real wait is too long and queue is non-empty
                        // Remote may be dead - double until max.
                        // Double after window restransmitted msgs at this RTO
                        // exceeds the RWINDOW, and we've had no response for
                        // twice the current RTO.
                        if ((retransed > 0) && (realWait >= 2 * RTO) && (nAtThisRTO >= 2 * RWINDOW)) {
                            RTO = (realWait > maxRTO ? maxRTO : 2 * RTO);
                            nAtThisRTO = 0;
                        }

                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("RETRANS : RETRANSMISSION " + retransed + " retrans " + nAtThisRTO + " at this RTO (" + RTO
                                    + ") " + nretransmitted + " total retrans");
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
                            LOG.fine("RETRANS : IDLE : RTO=" + RTO + " WAIT=" + realWait);
                        }
                    }
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
                }
            } finally {
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("STOPPED TLS Retransmit thread");
                }

                retransmitterThread = null;
                retransmitter = null;
            }
        }
    }
}
