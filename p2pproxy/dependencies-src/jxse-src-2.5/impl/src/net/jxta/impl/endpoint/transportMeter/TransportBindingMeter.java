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


import net.jxta.peer.PeerID;
import net.jxta.endpoint.*;


public class TransportBindingMeter {
    private PeerID peerID;
    private EndpointAddress endpointAddress;

    private TransportBindingMetric cumulativeMetrics;
    private TransportBindingMetric deltaMetrics;

    public TransportBindingMeter(PeerID peerID, EndpointAddress endpointAddress) {
        this(peerID, endpointAddress, false, false);
    }
	
    public TransportBindingMeter(PeerID peerID, EndpointAddress endpointAddress, boolean initiatorConnected, boolean acceptorConnected) {
        this.peerID = peerID;
        this.endpointAddress = endpointAddress;
        cumulativeMetrics = new TransportBindingMetric(this, initiatorConnected, acceptorConnected);
    }

    @Override
    public String toString() {
        return "TransportBindingMeter(" + endpointAddress + ";" + peerID + ")";
    }

    public synchronized TransportBindingMetric collectMetrics() {
        TransportBindingMetric prevDelta = deltaMetrics;

        deltaMetrics = null;
        return prevDelta;
    }
		
    private void createDeltaMetric() {
        deltaMetrics = new TransportBindingMetric(cumulativeMetrics);
    }

    public TransportBindingMetric getCumulativeMetrics() {
        return cumulativeMetrics;
    }

    public PeerID getPeerID() {
        return peerID;
    }

    public EndpointAddress getEndpointAddress() {
        return endpointAddress;
    }

    public void setPeerID(PeerID peerID) { 
        this.peerID = peerID; 
        cumulativeMetrics.setPeerID(peerID);
		
        if (deltaMetrics != null) {	
            deltaMetrics.setPeerID(peerID);
        }
    }

    public synchronized void connectionEstablished(boolean initator, long timeToConnect) {		
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        long now = System.currentTimeMillis();

        deltaMetrics.connectionEstablished(initator, timeToConnect, now);
        cumulativeMetrics.connectionEstablished(initator, timeToConnect, now);
    }

    public synchronized void connectionFailed(boolean initator, long timeToConnect) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        long now = System.currentTimeMillis();

        deltaMetrics.connectionFailed(initator, timeToConnect, now);
        cumulativeMetrics.connectionFailed(initator, timeToConnect, now);
    }
	
    public synchronized void connectionClosed(boolean initator, long connectionLife) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        long now = System.currentTimeMillis();

        deltaMetrics.connectionClosed(initator, now);
        cumulativeMetrics.connectionClosed(initator, now);
    }
	
    public synchronized void connectionDropped(boolean initator, long connectionLife) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        long now = System.currentTimeMillis();

        deltaMetrics.connectionDropped(initator, now);
        cumulativeMetrics.connectionDropped(initator, now);
    }

    public synchronized void pingReceived() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.pingReceived();
        cumulativeMetrics.pingReceived();
    }
	
    public synchronized void ping(long time) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.ping(time);
        cumulativeMetrics.ping(time);
    }
	
    public synchronized void pingFailed(long time) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.pingFailed(time);
        cumulativeMetrics.pingFailed(time);
    }

    public synchronized void dataReceived(boolean initator, int size) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.dataReceived(initator, size);
        cumulativeMetrics.dataReceived(initator, size);
    }

    public synchronized void messageReceived(boolean initator, Message message, long time, long size) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.messageReceived(initator, message, time, size);
        cumulativeMetrics.messageReceived(initator, message, time, size);
    }

    public synchronized void receiveFailure(boolean initator, long time, long size) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.receiveFailure(initator, time, size);
        cumulativeMetrics.receiveFailure(initator, time, size);
    }

    public synchronized void dataSent(boolean initator, long size) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.dataSent(initator, size);
        cumulativeMetrics.dataSent(initator, size);
    }		
	
    public synchronized void sendFailure(boolean initator, Message message, long time, long size) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.sendFailure(initator, message, time, size);
        cumulativeMetrics.sendFailure(initator, message, time, size);
    }

    public synchronized void messageSent(boolean initator, Message message, long time, long size) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.messageSent(initator, message, time, size);
        cumulativeMetrics.messageSent(initator, message, time, size);
    }		
}
