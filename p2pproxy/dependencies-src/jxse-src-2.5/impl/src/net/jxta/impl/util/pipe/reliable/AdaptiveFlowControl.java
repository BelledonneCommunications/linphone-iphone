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


import net.jxta.impl.util.TimeUtils;


public class AdaptiveFlowControl extends FlowControl {

    static final int DEFAULT_RWINDOW = 2;

    /**
     * global state.
     */
    private int MAX_TENSION = 3;
    private int tension = 0;
    private long nextRwinChange = TimeUtils.timeNow();
    private long prevAveRTT = 10 * TimeUtils.ASECOND;
    private int RINGSZ = 8;
    private long[] ackTimeRing = new long[RINGSZ];
    private int currAckRingOff = 0;
    private int nbSamples = 0;
    private long currAvePeriod = 1;
    private long prevAvePeriod = 1; // not in use yet.
    private long periodRangeSlow = Long.MAX_VALUE; // not in use, yet.
    private long periodRangeFast = (periodRangeSlow / 3) * 2;

    /**
     * Current recommended rwindow.
     */
    private volatile int rwindow = 0;

    /**
     * state of the currentAck being processed
     */

    // Accum of acked packets
    private int numberACKed = 0;

    // Accum of missing packets
    private int numberMissing = 0;

    // Time this ACK arrived
    private long currACKTime = 0;

    // These variables are used to evaluate the longest run
    // of consecutive holes in the sack list. That is consecutive
    // seqnums from the retrQ that are not being acknowleged,
    // followed by an acknowleged one.
    private int prevHole = -2;
    private int btbHoles = 0;
    private int maxHoleRun = 0;

    /**
     * Constructs an adaptive flow control module with an initial rwindow of
     * DEFAULT_RWINDOW.
     */
    public AdaptiveFlowControl() {
        this(DEFAULT_RWINDOW);
    }

    /**
     * @param rwindow Use this value as the initial value (not recommended
     * except for experimental purposes.
     */
    public AdaptiveFlowControl(int rwindow) {
        this.currACKTime = TimeUtils.timeNow();
        this.rwindow = rwindow;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getRwindow() {
        return rwindow;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void ackEventBegin() {
        currACKTime = TimeUtils.timeNow();
        numberACKed = 0;
        numberMissing = 0;
        maxHoleRun = 0;

        // Note the currently open holerun carries over from the prev ACK.
        // So, we leave prevHole and btbHoles alone.
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void packetACKed(int seqnum) {
        if (btbHoles > maxHoleRun) {
            maxHoleRun = btbHoles;
        }
        btbHoles = 0;
        prevHole = -2;
        numberACKed++;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void packetMissing(int seqnum) {

        if (seqnum != prevHole + 1) {
            // End of run, begining of next
            if (btbHoles > maxHoleRun) {
                maxHoleRun = btbHoles;
            }
            btbHoles = 0;
        }
        btbHoles++;
        prevHole = seqnum;
        numberMissing++;
    }

    boolean fastMode = true;
    int takeAchance = 0;

    /**
     * {@inheritDoc}
     */
    @Override
    public int ackEventEnd(int rQSize, long aveRTT, long lastRTT) {

        // Compute average ack rate. If nothing was acked by this
        // ack msg, consider it a bad sign as far as ack rates go: as good
        // as no ack at all.

        // Even if a few ack messages where lost, the current event
        // encapsulate all the acks we missed. Count each of them
        // as one individual ack for the purpose of rate computation:
        // we want to count the messages the other side has received
        // not the number of ack messages that found their way back.

        if (numberACKed > 0) {
            for (int a = 0; a < numberACKed; ++a) {

                // Adds a new sample to the ring. Returns the new average
                // sample period. Once the ring is filled, the average is
                // computed by substracting the sample at the current
                // offset (the oldest) from the new sample that replaces
                // it, and dividing by the ring size (10).  During the
                // first round, we use the first sample, so precision is
                // poorer.

                long oldest = ackTimeRing[currAckRingOff];

                if (nbSamples < RINGSZ) {
                    // make a fake (very) oldest sample if there is nothing yet.
                    if (nbSamples == 0) {
                        ackTimeRing[0] = currACKTime / 2;
                    }
                    ++nbSamples;
                    oldest = ackTimeRing[0];
                }
                ackTimeRing[currAckRingOff++] = currACKTime;
                if (currAckRingOff == RINGSZ) {
                    currAckRingOff = 0;
                }
                prevAvePeriod = currAvePeriod;
                currAvePeriod = (currACKTime - oldest) / nbSamples;
            }
        }

        // Compute rwindow. It should keep oscillating around
        // the best value.
        // Up to a certain point, the higher we keep rwindow the more
        // we keep all the bandwidth utilized. Beyond that point we have
        // it just serves to create congestion.

        int oldSize = rwindow;

        if (TimeUtils.toRelativeTimeMillis(nextRwinChange) < 0) {
            if (maxHoleRun < 4) {

                if (numberACKed > 0) {

                    if (currAvePeriod < periodRangeFast) {

                        // All is well: new rate record. We can
                        // push some more.  and adjust the
                        // expected rate range towards speed.

                        periodRangeFast = currAvePeriod;
                        periodRangeSlow = (periodRangeFast * 3) / 2;

                        prevAveRTT = aveRTT;

                        tension = 0;

                    } else {

                        // If rate is not up and RTT has
                        // increased by one inter-ack period or more
                        // since the last time we took the mark, it
                        // looks like one or more packet just had to
                        // wait its turn. So packets are being
                        // buffered, which does not do any
                        // good. Refrain from pushing under these
                        // conditions. Wait for a more favorable time.

                        // This compares the change in RTT with the period
                        // and gives us a badness index from 0 to 10 * n
                        // Beyond 20 or so, we start getting worried.
                        // The hairy formula below compensates for non
                        // linearity of the rtt_diff/period ratio with
                        // period. The formulat compresses the scale towards
                        // a period of 0. There is no compression at a period
                        // of around 100 and maximum compression at 0.
                        // To make the index less sensitive for low periods,
                        // increase the compression ratio.

                        int compressionRatio100 = 90;
                        int pivot = 100;
                        long period = currAvePeriod <= 0 ? 1 : currAvePeriod;
                        long backupSign = (10 * pivot * (aveRTT - prevAveRTT))
                                / (pivot * period + Math.max((compressionRatio100 * (pivot * period - period * period)) / 100, 0));

                        if (backupSign > 18) {

                            // if detect a speed increase, we'll reset our
                            // idea of the normal RTT for next time. But we
                            // will drop rwindow tension for now.

                            rwindow--;
                            tension = MAX_TENSION;

                            // The first time this happens, it's the end of fast
                            // mode.
                            fastMode = false;

                        } else if ((backupSign < 1) && (currAvePeriod >= periodRangeSlow)) {

                            if (tension >= MAX_TENSION) {
                                // May be we should give it another chance
                                // and nudge it just a little. On very lossy
                                // links we may end-up with no congestion
                                // at all but stuck at low speed because
                                // we have stopped believing in speed increase.
                                if (takeAchance++ > 10) {
                                    takeAchance = 0;
                                    tension--;
                                }
                            }
                        } else {
                            takeAchance = 0;
                        }
                    }

                    if (tension < MAX_TENSION) {
                        tension++;
                        rwindow++;
                    }

                } else {
                    // Carefull, the other side did not ack anything
                    // it is stuck on a missing packet...better slow down
                    tension = MAX_TENSION;
                }

            } else {

                // We saturated the pipe. We need to slow down
                // arbitrarily without changing our idea of speed,
                // so that the correlation between speed and
                // rwindow is shifted towards a smaller
                // rwindow.

                rwindow -= (MAX_TENSION + 1);
                prevAveRTT = aveRTT;
                tension = MAX_TENSION;

                // The first time this happens, it's the end of fast
                // mode.
                fastMode = false;
            }

            if (rwindow > rQSize) {
                rwindow = rQSize;
            }

            if (rwindow < 2) {
                rwindow = 2;
            }

            if (oldSize != rwindow) {
                if (fastMode && (tension < MAX_TENSION)) {
                    nextRwinChange = TimeUtils.toAbsoluteTimeMillis(lastRTT / 10);
                } else {
                    nextRwinChange = TimeUtils.toAbsoluteTimeMillis(aveRTT * 2);
                }
            }
        }

        return rwindow;
    }
}
