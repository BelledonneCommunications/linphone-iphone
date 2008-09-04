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

package net.jxta.impl.endpoint.transportMeter;


import net.jxta.document.Element;
import net.jxta.document.TextElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.Message;
import net.jxta.impl.meter.MetricUtilities;
import net.jxta.peer.PeerID;
import net.jxta.util.documentSerializable.DocumentSerializable;
import net.jxta.util.documentSerializable.DocumentSerializableUtilities;
import net.jxta.util.documentSerializable.DocumentSerializationException;

import java.util.Enumeration;


public class TransportBindingMetric implements DocumentSerializable {
    public static final String CONNECTED = "connected";
    public static final String CLOSED = "closed";
    public static final String DROPPED = "dropped";
    public static final String FAILED = "failed";

    private PeerID peerID;
    private EndpointAddress endpointAddress;

    private String initiatorState = null;
    private String acceptorState = null;
    private long initiatorTransitionTime;
    private long acceptorTransitionTime;

    private int acceptorBytesReceived;
    private int acceptorBytesSent;
    private int acceptorConnections;
    private int acceptorConnectionsClosed;
    private int acceptorConnectionsDropped;
    private int acceptorConnectionsFailed;
    private int acceptorMessagesReceived;
    private int acceptorMessagesSent;
    private long acceptorReceiveFailureProcessingTime;
    private int acceptorReceiveFailures;
    private long acceptorReceiveProcessingTime;
    private long acceptorSendFailureProcessingTime;
    private int acceptorSendFailures;
    private long acceptorSendProcessingTime;
    private long acceptorTotalTimeConnected;
    private long acceptorTimeToConnect;
    private long acceptorTimeToFail;
    private int initiatorBytesReceived;
    private int initiatorBytesSent;
    private long initiatorTotalTimeConnected;
    private int initiatorConnections;
    private int initiatorConnectionsClosed;
    private int initiatorConnectionsDropped;
    private int initiatorConnectionsFailed;
    private int initiatorMessagesReceived;
    private int initiatorMessagesSent;
    private long initiatorReceiveFailureProcessingTime;
    private int initiatorReceiveFailures;
    private long initiatorReceiveProcessingTime;
    private long initiatorSendFailureProcessingTime;
    private int initiatorSendFailures;
    private long initiatorSendProcessingTime;
    private long initiatorTimeToConnect;
    private long initiatorTimeToFail;

    private int numPings;
    private int numFailedPings;
    private long pingTime;
    private long pingFailedTime;
    private int numPingsReceived;

    public TransportBindingMetric(TransportBindingMeter transportBindingMeter, boolean initiatorConnected, boolean acceptorConnected) {
        this.peerID = transportBindingMeter.getPeerID();
        this.endpointAddress = transportBindingMeter.getEndpointAddress();
        this.initiatorState = initiatorConnected ? CONNECTED : CLOSED;
        this.acceptorState = acceptorConnected ? CONNECTED : CLOSED;
    }

    public TransportBindingMetric() {}

    public TransportBindingMetric(TransportBindingMetric prototype) {
        this.peerID = prototype.peerID;
        this.endpointAddress = prototype.endpointAddress;
        this.initiatorState = prototype.initiatorState;
        this.acceptorState = prototype.acceptorState;
        this.initiatorTransitionTime = prototype.initiatorTransitionTime;
        this.acceptorTransitionTime = prototype.acceptorTransitionTime;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof TransportBindingMetric) {
            TransportBindingMetric other = (TransportBindingMetric) obj;

            return endpointAddress.equals(other.endpointAddress);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return peerID.hashCode() + endpointAddress.hashCode();
    }

    public PeerID getPeerID() {
        return peerID;
    }

    public void setPeerID(PeerID peerID) {
        this.peerID = peerID;
    }

    public EndpointAddress getEndpointAddress() {
        return endpointAddress;
    }

    /**
     * State of this Initiator Binding
     *
     * @return TransportBindingMetric.CONNECTED, TransportBindingMetric.DISCONNECTED or TransportBindingMetric.FAILED
     */
    public String getInitiatorState() {
        return initiatorState;
    }

    /**
     * State of this Acceptor Binding
     *
     * @return TransportBindingMetric.CONNECTED, TransportBindingMetric.DISCONNECTED or TransportBindingMetric.FAILED
     */
    public String getAcceptorState() {
        return acceptorState;
    }

    /**
     * Get the time that it entered the current state
     *
     * @return transition time in ms since January 1, 1970, 00:00:00 GMT
     */
    public long getInitiatorTransitionTime() {
        return initiatorTransitionTime;
    }

    /**
     * Get the time that it entered the current state
     *
     * @return transition time in ms since January 1, 1970, 00:00:00 GMT
     */
    public long getAcceptorTransitionTime() {
        return acceptorTransitionTime;
    }

    public boolean isAcceptorConnected() {
        return (acceptorState != null) && acceptorState.equals(CONNECTED);
    }

    public boolean isInitiatorConnected() {
        return (initiatorState != null) && initiatorState.equals(CONNECTED);
    }

    public long getTimeAcceptorConnectionEstablished() {
        return isAcceptorConnected() ? acceptorTransitionTime : 0;
    }

    public long getTimeInitiatorConnectionEstablished() {
        return isInitiatorConnected() ? initiatorTransitionTime : 0;
    }

    public int getAcceptorBytesReceived() {
        return acceptorBytesReceived;
    }

    public int getAcceptorBytesSent() {
        return acceptorBytesSent;
    }

    public int getAcceptorConnections() {
        return acceptorConnections;
    }

    public int getAcceptorConnectionsClosed() {
        return acceptorConnectionsClosed;
    }

    public int getAcceptorConnectionsDropped() {
        return acceptorConnectionsDropped;
    }

    public int getAcceptorConnectionsFailed() {
        return acceptorConnectionsFailed;
    }

    public int getAcceptorMessagesReceived() {
        return acceptorMessagesReceived;
    }

    public int getAcceptorMessagesSent() {
        return acceptorMessagesSent;
    }

    public long getAcceptorReceiveFailureProcessingTime() {
        return acceptorReceiveFailureProcessingTime;
    }

    public int getAcceptorReceiveFailures() {
        return acceptorReceiveFailures;
    }

    public long getAcceptorReceiveProcessingTime() {
        return acceptorReceiveProcessingTime;
    }

    public long getAcceptorSendFailureProcessingTime() {
        return acceptorSendFailureProcessingTime;
    }

    public int getAcceptorSendFailures() {
        return acceptorSendFailures;
    }

    public long getAcceptorSendProcessingTime() {
        return acceptorSendProcessingTime;
    }

    public long getAcceptorTimeToConnect() {
        return acceptorTimeToConnect;
    }

    public long getAcceptorTimeToFail() {
        return acceptorTimeToFail;
    }

    public int getInitiatorBytesReceived() {
        return initiatorBytesReceived;
    }

    public int getInitiatorBytesSent() {
        return initiatorBytesSent;
    }

    public int getInitiatorConnections() {
        return initiatorConnections;
    }

    public int getInitiatorConnectionsClosed() {
        return initiatorConnectionsClosed;
    }

    public int getInitiatorConnectionsDropped() {
        return initiatorConnectionsDropped;
    }

    public int getInitiatorConnectionsFailed() {
        return initiatorConnectionsFailed;
    }

    public int getInitiatorMessagesReceived() {
        return initiatorMessagesReceived;
    }

    public int getInitiatorMessagesSent() {
        return initiatorMessagesSent;
    }

    public long getInitiatorReceiveFailureProcessingTime() {
        return initiatorReceiveFailureProcessingTime;
    }

    public int getInitiatorReceiveFailures() {
        return initiatorReceiveFailures;
    }

    public long getInitiatorReceiveProcessingTime() {
        return initiatorReceiveProcessingTime;
    }

    public long getInitiatorSendFailureProcessingTime() {
        return initiatorSendFailureProcessingTime;
    }

    public int getInitiatorSendFailures() {
        return initiatorSendFailures;
    }

    public long getInitiatorSendProcessingTime() {
        return initiatorSendProcessingTime;
    }

    public long getInitiatorTimeToConnect() {
        return initiatorTimeToConnect;
    }

    public long getInitiatorTimeToFail() {
        return initiatorTimeToFail;
    }

    public int getNumPings() {
        return numPings;
    }

    public int getNumFailedPings() {
        return numFailedPings;
    }

    public long getPingTime() {
        return pingTime;
    }

    public long getPingFailedTime() {
        return pingFailedTime;
    }

    public int getNumPingsReceived() {
        return numPingsReceived;
    }

    public int getBytesReceived() {
        return acceptorBytesReceived + initiatorBytesReceived;
    }

    public int getBytesSent() {
        return acceptorBytesSent + initiatorBytesSent;
    }

    public int getConnections() {
        return acceptorConnections + initiatorConnections;
    }

    public int getConnectionsClosed() {
        return acceptorConnectionsClosed + initiatorConnectionsClosed;
    }

    public int getConnectionsDropped() {
        return acceptorConnectionsDropped + initiatorConnectionsDropped;
    }

    public int getConnectionsFailed() {
        return acceptorConnectionsFailed + initiatorConnectionsFailed;
    }

    public int getMessagesReceived() {
        return acceptorMessagesReceived + initiatorMessagesReceived;
    }

    public int getMessagesSent() {
        return acceptorMessagesSent + initiatorMessagesSent;
    }

    public long getReceiveFailureProcessingTime() {
        return acceptorReceiveFailureProcessingTime + initiatorReceiveFailureProcessingTime;
    }

    public int getReceiveFailures() {
        return acceptorReceiveFailures + initiatorReceiveFailures;
    }

    public long getReceiveProcessingTime() {
        return acceptorReceiveProcessingTime + initiatorReceiveProcessingTime;
    }

    public long getSendFailureProcessingTime() {
        return acceptorSendFailureProcessingTime + initiatorSendFailureProcessingTime;
    }

    public int getSendFailures() {
        return acceptorSendFailures + initiatorSendFailures;
    }

    public long getSendProcessingTime() {
        return acceptorSendProcessingTime + initiatorSendProcessingTime;
    }

    public long getTotalTimeConnected() {
        return acceptorTotalTimeConnected + initiatorTotalTimeConnected;
    }

    public long getTimeToConnect() {
        return acceptorTimeToConnect + initiatorTimeToConnect;
    }

    public long getTimeToFail() {
        return acceptorTimeToFail + initiatorTimeToFail;
    }

    public int getAveragePingTime() {
        return (int) ((numPings != 0) ? (pingTime / numPings) : 0);
    }

    public int getAveragePingFailedTime() {
        return (int) ((numFailedPings != 0) ? (pingFailedTime / numFailedPings) : 0);
    }

    public int getAverageAcceptorReceiveFailureProcessingTime() {
        return (int) ((acceptorReceiveFailures != 0) ? (acceptorReceiveFailureProcessingTime / acceptorReceiveFailures) : 0);
    }

    public int getAverageAcceptorReceiveProcessingTime() {
        return (int) ((acceptorMessagesReceived != 0) ? (acceptorReceiveProcessingTime / acceptorMessagesReceived) : 0);
    }

    public int getAverageAcceptorSendFailureProcessingTime() {
        return (int) ((acceptorSendFailures != 0) ? (acceptorSendFailureProcessingTime / acceptorSendFailures) : 0);
    }

    public int getAverageAcceptorSendProcessingTime() {
        return (int) ((acceptorMessagesSent != 0) ? (acceptorSendProcessingTime / acceptorMessagesSent) : 0);
    }

    public int getAverageAcceptorTimeToConnect() {
        return (int) ((acceptorConnections != 0) ? (acceptorTimeToConnect / acceptorConnections) : 0);
    }

    public int getAverageAcceptorTimeToFail() {
        return (int) ((acceptorConnectionsFailed != 0) ? (acceptorTimeToFail / acceptorConnectionsFailed) : 0);
    }

    public int getAverageInitiatorReceiveFailureProcessingTime() {
        return (int) ((initiatorReceiveFailures != 0) ? (initiatorReceiveFailureProcessingTime / initiatorReceiveFailures) : 0);
    }

    public int getAverageInitiatorReceiveProcessingTime() {
        return (int) ((initiatorMessagesReceived != 0) ? (initiatorReceiveProcessingTime / initiatorMessagesReceived) : 0);
    }

    public int getAverageInitiatorSendFailureProcessingTime() {
        return (int) ((initiatorSendFailures != 0) ? (initiatorSendFailureProcessingTime / initiatorSendFailures) : 0);
    }

    public int getAverageInitiatorSendProcessingTime() {
        return (int) ((initiatorMessagesSent != 0) ? (initiatorSendProcessingTime / initiatorMessagesSent) : 0);
    }

    public int getAverageInitiatorTimeToConnect() {
        return (int) ((initiatorConnections != 0) ? (initiatorTimeToConnect / initiatorConnections) : 0);
    }

    public int getAverageInitiatorTimeToFail() {
        return (int) ((initiatorConnectionsFailed != 0) ? (initiatorTimeToFail / initiatorConnectionsFailed) : 0);
    }

    public int getAverageReceiveFailureProcessingTime() {
        return (int) (((initiatorReceiveFailures + acceptorReceiveFailures) != 0)
                ? ((initiatorReceiveFailureProcessingTime + acceptorReceiveFailureProcessingTime)
                        / (initiatorReceiveFailures + acceptorReceiveFailures))
                        : 0);
    }

    public int getAverageReceiveProcessingTime() {
        return (int) (((initiatorMessagesReceived + acceptorMessagesReceived) != 0)
                ? ((initiatorReceiveProcessingTime + acceptorReceiveProcessingTime)
                        / (initiatorMessagesReceived + acceptorMessagesReceived))
                        : 0);
    }

    public int getAverageSendFailureProcessingTime() {
        return (int) (((initiatorSendFailures + acceptorSendFailures) != 0)
                ? ((initiatorSendFailureProcessingTime + acceptorSendFailureProcessingTime)
                        / (initiatorSendFailures + acceptorSendFailures))
                        : 0);
    }

    public int getAverageSendProcessingTime() {
        return (int) (((initiatorMessagesSent + acceptorMessagesSent) != 0)
                ? ((initiatorSendProcessingTime + acceptorSendProcessingTime) / (initiatorMessagesSent + acceptorMessagesSent))
                : 0);
    }

    public int getAverageTimeToConnect() {
        return (int) (((initiatorConnections + acceptorConnections) != 0)
                ? ((initiatorTimeToConnect + acceptorTimeToConnect) / (initiatorConnections + acceptorConnections))
                : 0);
    }

    public int getAverageTimeToFail() {
        return (int) (((initiatorConnectionsFailed + acceptorConnectionsFailed) != 0)
                ? ((initiatorTimeToFail + acceptorTimeToFail) / (initiatorConnectionsFailed + acceptorConnectionsFailed))
                : 0);
    }

    /**
     * Get the total time this intiated connection has been connected.
     * <BR><BR>
     * <B>Note:</B> This does not include the current time connected (if it is currently connected)
     *
     * @return time in ms (see note above)
     * @see #getTotalTimeConnected()
     */
    public long getInitiatorTotalTimeConnected() {
        return initiatorTotalTimeConnected;
    }

    /**
     * Get the total time this initiating connection has been connected.  If it is currently
     * connected, then the total time is adjusted to include the time since the transition time
     * to become connected until the provided time
     *
     * @param adjustmentTime The time of this metric will be adjusted to
     * @return time in ms (see note above)
     * @see #getTotalTimeConnected()
     */
    public long getInitiatorTotalTimeConnected(long adjustmentTime) {
        long result = initiatorTotalTimeConnected;

        if (isInitiatorConnected()) {
            result += (adjustmentTime - this.initiatorTransitionTime);
        }

        return result;
    }

    /**
     * Get the duration of current connection relative to local clock (from transition time)
     * <BR><BR>
     * <B>Note:</B> This assumes the clocks are in sync with the reporting peer
     *
     * @return time in ms (see note above) or 0 if not connected
     * @see #getTotalTimeConnected()
     */
    public long getInitiatorTimeConnected() {
        return getInitiatorTimeConnected(System.currentTimeMillis());
    }

    /**
     * Get the duration of current connection until the specified time
     *
     * @param adjustmentTime The time of this metric will be computed until
     * @return time in ms (see note above) or 0 if not connected
     */
    public long getInitiatorTimeConnected(long adjustmentTime) {

        if (isInitiatorConnected()) {
            return (adjustmentTime - this.initiatorTransitionTime);
        } else {
            return 0;
        }
    }

    /**
     * Get the total time this intiated connection has been connected.
     * <BR><BR>
     * <B>Note:</B> This does not include the current time connected (if it is currently connected)
     *
     * @return time in ms (see note above)
     * @see #getTotalTimeConnected()
     */
    public long getAcceptorTotalTimeConnected() {
        return acceptorTotalTimeConnected;
    }

    /**
     * Get the total time this initiating connection has been connected.  If it is currently
     * connected, then the total time is adjusted to include the time since the transition time
     * to become connected until the provided time
     *
     * @param adjustmentTime The time of this metric will be adjusted to
     * @return time in ms (see note above)
     * @see #getTotalTimeConnected()
     */
    public long getAcceptorTotalTimeConnected(long adjustmentTime) {
        long result = acceptorTotalTimeConnected;

        if (isAcceptorConnected()) {
            result += (adjustmentTime - this.acceptorTransitionTime);
        }

        return result;
    }

    /**
     * Get the duration of current connection relative to local clock (from transition time)
     * <BR><BR>
     * <B>Note:</B> This assumes the clocks are in sync with the reporting peer
     *
     * @return time in ms (see note above) or 0 if not connected
     * @see #getTotalTimeConnected()
     */
    public long getAcceptorTimeConnected() {
        return getAcceptorTimeConnected(System.currentTimeMillis());
    }

    /**
     * Get the duration of current connection until the specified time
     *
     * @param adjustmentTime The time of this metric will be computed until
     * @return time in ms (see note above) or 0 if not connected
     */
    public long getAcceptorTimeConnected(long adjustmentTime) {

        if (isAcceptorConnected()) {
            return (adjustmentTime - this.acceptorTransitionTime);
        } else {
            return 0;
        }
    }

    void resetInitiatorState(String state, long transitionTime) {
        if (isInitiatorConnected()) {
            acceptorTotalTimeConnected += (System.currentTimeMillis() - this.initiatorTransitionTime);
        }

        this.initiatorState = state;
        this.initiatorTransitionTime = transitionTime;
        // System.out.println("initiatorState: " + initiatorState + " " + endpointAddress);
    }

    void resetAcceptorState(String state, long transitionTime) {
        if (isAcceptorConnected()) {
            initiatorTotalTimeConnected += (System.currentTimeMillis() - this.acceptorTransitionTime);
        }

        this.acceptorState = state;
        this.acceptorTransitionTime = transitionTime;
        // System.out.println("acceptorState: " + acceptorState + " " + endpointAddress);
    }

    void connectionEstablished(boolean initiator, long timeToConnect, long transitionTime) {
        if (initiator) {
            resetInitiatorState(CONNECTED, transitionTime);
            initiatorConnections++;
            initiatorTimeToConnect += timeToConnect;
        } else {
            resetAcceptorState(CONNECTED, transitionTime);
            acceptorConnections++;
            acceptorTimeToConnect += timeToConnect;
        }

    }

    void connectionFailed(boolean initiator, long timeToConnect, long transitionTime) {
        if (initiator) {
            resetInitiatorState(FAILED, transitionTime);
            initiatorConnectionsFailed++;
            initiatorTimeToFail += timeToConnect;
        } else {
            resetAcceptorState(FAILED, transitionTime);
            acceptorConnectionsFailed++;
            acceptorTimeToFail += timeToConnect;
        }
    }

    void connectionClosed(boolean initiator, long transitionTime) {
        if (initiator) {
            resetInitiatorState(CLOSED, transitionTime);
            initiatorConnectionsClosed++;
        } else {
            resetAcceptorState(CLOSED, transitionTime);
            acceptorConnectionsClosed++;
        }
    }

    void connectionDropped(boolean initiator, long transitionTime) {
        if (initiator) {
            resetInitiatorState(DROPPED, transitionTime);
            initiatorConnectionsDropped++;
        } else {
            resetAcceptorState(DROPPED, transitionTime);
            acceptorConnectionsDropped++;
        }
    }

    void pingReceived() {
        numPingsReceived++;
    }

    void ping(long time) {
        numPings++;
        pingTime += time;
    }

    void pingFailed(long time) {
        numFailedPings++;
        pingFailedTime += time;
    }

    void dataReceived(boolean initiator, int size) {
        if (initiator) {
            initiatorBytesReceived += size;
        } else {
            acceptorBytesReceived += size;
        }
    }

    void messageReceived(boolean initiator, Message message, long time, long size) {
        if (initiator) {
            initiatorMessagesReceived++;
            initiatorReceiveProcessingTime += time;
            initiatorBytesReceived += size;
        } else {
            acceptorMessagesReceived++;
            acceptorReceiveProcessingTime += time;
            acceptorBytesReceived += size;
        }
    }

    void receiveFailure(boolean initiator, long time, long size) {
        if (initiator) {
            initiatorReceiveFailures++;
            initiatorReceiveFailureProcessingTime += time;
            initiatorBytesReceived += size;
        } else {
            acceptorReceiveFailures++;
            acceptorReceiveFailureProcessingTime += time;
            acceptorBytesReceived += size;
        }
    }

    void dataSent(boolean initiator, long size) {
        if (initiator) {
            initiatorBytesSent += size;
        } else {
            acceptorBytesSent += size;
        }
    }

    void sendFailure(boolean initiator, Message message, long time, long size) {
        if (initiator) {
            initiatorSendFailures++;
            initiatorSendFailureProcessingTime += time;
            initiatorBytesSent += size;
        } else {
            acceptorSendFailures++;
            acceptorSendFailureProcessingTime += time;
            acceptorBytesSent += size;
        }
    }

    void messageSent(boolean initiator, Message message, long time, long size) {
        if (initiator) {
            initiatorMessagesSent++;
            initiatorSendProcessingTime += time;
            initiatorBytesSent += size;
        } else {
            acceptorMessagesSent++;
            acceptorSendProcessingTime += time;
            acceptorBytesSent += size;
        }
    }

    public void mergeMetrics(TransportBindingMetric other) {
        peerID = other.peerID;

        if (other.initiatorState != null) {
            initiatorState = other.initiatorState;
        }

        if (other.initiatorTransitionTime != 0) {
            initiatorTransitionTime = other.initiatorTransitionTime;
        }

        if (other.acceptorState != null) {
            acceptorState = other.acceptorState;
        }

        if (other.initiatorTransitionTime != 0) {
            acceptorTransitionTime = other.acceptorTransitionTime;
        }

        acceptorBytesReceived += other.acceptorBytesReceived;
        acceptorBytesSent += other.acceptorBytesSent;
        acceptorConnections += other.acceptorConnections;
        acceptorConnectionsClosed += other.acceptorConnectionsClosed;
        acceptorConnectionsDropped += other.acceptorConnectionsDropped;
        acceptorConnectionsFailed += other.acceptorConnectionsFailed;
        acceptorMessagesReceived += other.acceptorMessagesReceived;
        acceptorMessagesSent += other.acceptorMessagesSent;
        acceptorReceiveFailureProcessingTime += other.acceptorReceiveFailureProcessingTime;
        acceptorReceiveFailures += other.acceptorReceiveFailures;
        acceptorReceiveProcessingTime += other.acceptorReceiveProcessingTime;
        acceptorSendFailureProcessingTime += other.acceptorSendFailureProcessingTime;
        acceptorSendFailures += other.acceptorSendFailures;
        acceptorSendProcessingTime += other.acceptorSendProcessingTime;
        acceptorTotalTimeConnected += other.acceptorTotalTimeConnected;
        acceptorTimeToConnect += other.acceptorTimeToConnect;
        acceptorTimeToFail += other.acceptorTimeToFail;
        initiatorBytesReceived += other.initiatorBytesReceived;
        initiatorBytesSent += other.initiatorBytesSent;
        initiatorTotalTimeConnected += other.initiatorTotalTimeConnected;
        initiatorConnections += other.initiatorConnections;
        initiatorConnectionsClosed += other.initiatorConnectionsClosed;
        initiatorConnectionsDropped += other.initiatorConnectionsDropped;
        initiatorConnectionsFailed += other.initiatorConnectionsFailed;
        initiatorMessagesReceived += other.initiatorMessagesReceived;
        initiatorMessagesSent += other.initiatorMessagesSent;
        initiatorReceiveFailureProcessingTime += other.initiatorReceiveFailureProcessingTime;
        initiatorReceiveFailures += other.initiatorReceiveFailures;
        initiatorReceiveProcessingTime += other.initiatorReceiveProcessingTime;
        initiatorSendFailureProcessingTime += other.initiatorSendFailureProcessingTime;
        initiatorSendFailures += other.initiatorSendFailures;
        initiatorSendProcessingTime += other.initiatorSendProcessingTime;
        initiatorTimeToConnect += other.initiatorTimeToConnect;
        initiatorTimeToFail += other.initiatorTimeToFail;
        numPings += other.numPings;
        numFailedPings += other.numFailedPings;
        pingTime += other.pingTime;
        pingFailedTime += other.pingFailedTime;
        numPingsReceived += other.numPingsReceived;

    }

    public void serializeTo(Element element) throws DocumentSerializationException {

        DocumentSerializableUtilities.addString(element, "peerID", peerID.toString());
        DocumentSerializableUtilities.addString(element, "endpointAddress", endpointAddress.toString());

        if (initiatorState != null) {
            DocumentSerializableUtilities.addString(element, "initiatorState", initiatorState);
        }
        if (initiatorTransitionTime != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorTransitionTime", initiatorTransitionTime);
        }

        if (acceptorState != null) {
            DocumentSerializableUtilities.addString(element, "acceptorState", acceptorState);
        }
        if (acceptorTransitionTime != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorTransitionTime", acceptorTransitionTime);
        }

        if (acceptorBytesReceived != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorBytesReceived", acceptorBytesReceived);
        }

        if (acceptorBytesSent != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorBytesSent", acceptorBytesSent);
        }

        if (acceptorConnections != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorConnections", acceptorConnections);
        }

        if (acceptorConnectionsClosed != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorConnectionsClosed", acceptorConnectionsClosed);
        }

        if (acceptorConnectionsDropped != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorConnectionsDropped", acceptorConnectionsDropped);
        }

        if (acceptorConnectionsFailed != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorConnectionsFailed", acceptorConnectionsFailed);
        }

        if (acceptorMessagesReceived != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorMessagesReceived", acceptorMessagesReceived);
        }

        if (acceptorMessagesSent != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorMessagesSent", acceptorMessagesSent);
        }

        if (acceptorReceiveFailureProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorReceiveFailureProcessingTime"
                    ,
                    acceptorReceiveFailureProcessingTime);
        }

        if (acceptorReceiveFailures != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorReceiveFailures", acceptorReceiveFailures);
        }

        if (acceptorReceiveProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorReceiveProcessingTime", acceptorReceiveProcessingTime);
        }

        if (acceptorSendFailureProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorSendFailureProcessingTime", acceptorSendFailureProcessingTime);
        }

        if (acceptorSendFailures != 0) {
            DocumentSerializableUtilities.addInt(element, "acceptorSendFailures", acceptorSendFailures);
        }

        if (acceptorSendProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorSendProcessingTime", acceptorSendProcessingTime);
        }

        if (acceptorTotalTimeConnected != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorTotalTimeConnected", acceptorTotalTimeConnected);
        }

        if (acceptorTimeToConnect != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorTimeToConnect", acceptorTimeToConnect);
        }

        if (acceptorTimeToFail != 0) {
            DocumentSerializableUtilities.addLong(element, "acceptorTimeToFail", acceptorTimeToFail);
        }

        if (initiatorBytesReceived != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorBytesReceived", initiatorBytesReceived);
        }

        if (initiatorBytesSent != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorBytesSent", initiatorBytesSent);
        }

        if (initiatorTotalTimeConnected != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorTotalTimeConnected", initiatorTotalTimeConnected);
        }

        if (initiatorConnections != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorConnections", initiatorConnections);
        }

        if (initiatorConnectionsClosed != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorConnectionsClosed", initiatorConnectionsClosed);
        }

        if (initiatorConnectionsDropped != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorConnectionsDropped", initiatorConnectionsDropped);
        }

        if (initiatorConnectionsFailed != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorConnectionsFailed", initiatorConnectionsFailed);
        }

        if (initiatorMessagesReceived != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorMessagesReceived", initiatorMessagesReceived);
        }

        if (initiatorMessagesSent != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorMessagesSent", initiatorMessagesSent);
        }

        if (initiatorReceiveFailureProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorReceiveFailureProcessingTime"
                    ,
                    initiatorReceiveFailureProcessingTime);
        }

        if (initiatorReceiveFailures != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorReceiveFailures", initiatorReceiveFailures);
        }

        if (initiatorReceiveProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorReceiveProcessingTime", initiatorReceiveProcessingTime);
        }

        if (initiatorSendFailureProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorSendFailureProcessingTime"
                    ,
                    initiatorSendFailureProcessingTime);
        }

        if (initiatorSendFailures != 0) {
            DocumentSerializableUtilities.addInt(element, "initiatorSendFailures", initiatorSendFailures);
        }

        if (initiatorSendProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorSendProcessingTime", initiatorSendProcessingTime);
        }

        if (initiatorTimeToConnect != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorTimeToConnect", initiatorTimeToConnect);
        }

        if (initiatorTimeToFail != 0) {
            DocumentSerializableUtilities.addLong(element, "initiatorTimeToFail", initiatorTimeToFail);
        }

        if (numPings != 0) {
            DocumentSerializableUtilities.addInt(element, "numPings", numPings);
        }

        if (numFailedPings != 0) {
            DocumentSerializableUtilities.addInt(element, "numFailedPings", numFailedPings);
        }

        if (pingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "pingTime", pingTime);
        }

        if (pingFailedTime != 0) {
            DocumentSerializableUtilities.addLong(element, "pingFailedTime", pingFailedTime);
        }

        if (initiatorTimeToFail != 0) {
            DocumentSerializableUtilities.addInt(element, "numPingsReceived", numPingsReceived);
        }

    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("peerID")) {
                String peerIdString = DocumentSerializableUtilities.getString(childElement);

                peerID = MetricUtilities.getPeerIdFromString(peerIdString);
            }
            if (tagName.equals("endpointAddress")) {
                String endpointAddressString = DocumentSerializableUtilities.getString(childElement);

                endpointAddress = new EndpointAddress(endpointAddressString);
            } else if (tagName.equals("acceptorBytesReceived")) {
                acceptorBytesReceived = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorBytesSent")) {
                acceptorBytesSent = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorConnections")) {
                acceptorConnections = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorConnectionsClosed")) {
                acceptorConnectionsClosed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorConnectionsDropped")) {
                acceptorConnectionsDropped = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorConnectionsFailed")) {
                acceptorConnectionsFailed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorMessagesReceived")) {
                acceptorMessagesReceived = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorMessagesSent")) {
                acceptorMessagesSent = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorReceiveFailureProcessingTime")) {
                acceptorReceiveFailureProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("acceptorReceiveFailures")) {
                acceptorReceiveFailures = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorReceiveProcessingTime")) {
                acceptorReceiveProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("acceptorSendFailureProcessingTime")) {
                acceptorSendFailureProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("acceptorSendFailures")) {
                acceptorSendFailures = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("acceptorSendProcessingTime")) {
                acceptorSendProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("acceptorTotalTimeConnected")) {
                acceptorTotalTimeConnected = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("acceptorTimeToConnect")) {
                acceptorTimeToConnect = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("acceptorTimeToFail")) {
                acceptorTimeToFail = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("initiatorBytesReceived")) {
                initiatorBytesReceived = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorBytesSent")) {
                initiatorBytesSent = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorTotalTimeConnected")) {
                initiatorTotalTimeConnected = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("initiatorConnections")) {
                initiatorConnections = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorConnectionsClosed")) {
                initiatorConnectionsClosed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorConnectionsDropped")) {
                initiatorConnectionsDropped = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorConnectionsFailed")) {
                initiatorConnectionsFailed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorMessagesReceived")) {
                initiatorMessagesReceived = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorMessagesSent")) {
                initiatorMessagesSent = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorReceiveFailureProcessingTime")) {
                initiatorReceiveFailureProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("initiatorReceiveFailures")) {
                initiatorReceiveFailures = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorReceiveProcessingTime")) {
                initiatorReceiveProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("initiatorSendFailureProcessingTime")) {
                initiatorSendFailureProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("initiatorSendFailures")) {
                initiatorSendFailures = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("initiatorSendProcessingTime")) {
                initiatorSendProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("initiatorTimeToConnect")) {
                initiatorTimeToConnect = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("initiatorTimeToFail")) {
                initiatorTimeToFail = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numPingsReceived")) {
                numPingsReceived = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPings")) {
                numPings = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFailedPings")) {
                numFailedPings = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("pingTime")) {
                pingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("pingFailedTime")) {
                pingFailedTime = DocumentSerializableUtilities.getLong(childElement);
            }
        }
    }
}
