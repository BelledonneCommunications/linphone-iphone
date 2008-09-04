/*
 * Copyright (c) 2001-2007 Sun Micro//Systems, Inc.  All rights reserved.
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

package net.jxta.impl.rendezvous.rendezvousMeter;


import net.jxta.peer.*;
import net.jxta.peergroup.*;
import net.jxta.rendezvous.*;
import net.jxta.impl.rendezvous.*;
import java.net.*;
import java.util.*;


/**
 The Meter corresponding to the state and aggregate information of a Rendezvous Service
 **/
public class RendezvousMeter {
    private RendezvousMetric cumulativeMetrics;
    private RendezvousMetric deltaMetrics;

    private long transitionTime = System.currentTimeMillis();

    public RendezvousMeter() {
        cumulativeMetrics = new RendezvousMetric(null);
    }

    public RendezvousMetric getCumulativeMetrics() { 
        return cumulativeMetrics; 
    }

    public synchronized RendezvousMetric collectMetrics() {	
        RendezvousMetric prevDelta = deltaMetrics;

        deltaMetrics = null;

        return prevDelta;
    }

    private void createDeltaMetric() {
        deltaMetrics = new RendezvousMetric(cumulativeMetrics);
    }

    @Override
    public String toString() {
        return "RendezvousMeter";
    }

    public void startEdge() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        transitionTime = System.currentTimeMillis();
        deltaMetrics.startEdge(transitionTime);
        cumulativeMetrics.startEdge(transitionTime);
    }
	
    public void stopEdge() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        long now = System.currentTimeMillis();
        long timeAsEdge = now - transitionTime;
		
        transitionTime = now;

        if (!cumulativeMetrics.isEdge()) {
            timeAsEdge = 0;
        }
		
        deltaMetrics.stopEdge(now, timeAsEdge);
        cumulativeMetrics.stopEdge(now, timeAsEdge);
    }

    public void startRendezvous() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        transitionTime = System.currentTimeMillis();
        deltaMetrics.startRendezvous(transitionTime);
        cumulativeMetrics.startRendezvous(transitionTime);
    }

    public void stopRendezvous() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        long now = System.currentTimeMillis();
        long timeAsRendezvous = cumulativeMetrics.isRendezvous() ? (now - transitionTime) : 0;
		
        transitionTime = now;

        deltaMetrics.stopRendezvous(now, timeAsRendezvous);
        cumulativeMetrics.stopRendezvous(now, timeAsRendezvous);
    }

    public void invalidMessageReceived() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.invalidMessageReceived();
        cumulativeMetrics.invalidMessageReceived();
    }

    public void receivedMessageProcessedLocally() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.receivedMessageProcessedLocally();
        cumulativeMetrics.receivedMessageProcessedLocally();
    }

    public void receivedMessageRepropagatedInGroup() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.receivedMessageRepropagatedInGroup();
        cumulativeMetrics.receivedMessageRepropagatedInGroup();
    }

    public void receivedDeadMessage() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.receivedDeadMessage();
        cumulativeMetrics.receivedDeadMessage();
    }

    public void receivedLoopbackMessage() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.receivedLoopbackMessage();
        cumulativeMetrics.receivedLoopbackMessage();
    }

    public void receivedDuplicateMessage() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.receivedDuplicateMessage();
        cumulativeMetrics.receivedDuplicateMessage();
    }

    public void propagateToPeers(int numPeers) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.propagateToPeers(numPeers);
        cumulativeMetrics.propagateToPeers(numPeers);
    }

    public void propagateToNeighbors() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.propagateToNeighbors();
        cumulativeMetrics.propagateToNeighbors();
    }
	
    public void propagateToNeighborsFailed() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.propagateToNeighborsFailed();
        cumulativeMetrics.propagateToNeighborsFailed();
    }

    public void propagateToGroup() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.propagateToGroup();
        cumulativeMetrics.propagateToGroup();
    }

    public void walk() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.walk();
        cumulativeMetrics.walk();
    }

    public void walkFailed() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.walkFailed();
        cumulativeMetrics.walkFailed();
    }

    public void walkToPeers(int numPeers) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.walkToPeers(numPeers);
        cumulativeMetrics.walkToPeers(numPeers);
    }

    public void walkToPeersFailed() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.walkToPeersFailed();
        cumulativeMetrics.walkToPeersFailed();
    }
}
