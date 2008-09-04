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

package net.jxta.impl.resolver.resolverMeter;


import net.jxta.protocol.ResolverSrdiMsg;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.peer.*;
import net.jxta.util.*;
import net.jxta.impl.meter.*;
import net.jxta.exception.*;
import java.util.*;


public class SrdiHandlerMeter {
    private String handlerName;

    private SrdiHandlerMetric cumulativeMetrics;
    private SrdiHandlerMetric deltaMetrics;
    private Hashtable srdiDestinationMeters = new Hashtable();

    public SrdiHandlerMeter(String handlerName) {
        this.handlerName = handlerName;
        cumulativeMetrics = new SrdiHandlerMetric(handlerName);
    }

    public SrdiHandlerMetric getCumulativeMetrics() {
        return cumulativeMetrics;
    }

    public String getHandlerName() {
        return handlerName;
    }
	
    public synchronized SrdiHandlerMetric collectMetrics() {
        SrdiHandlerMetric prevDelta = deltaMetrics;

        for (Enumeration e = srdiDestinationMeters.elements(); e.hasMoreElements();) {
            SrdiDestinationMeter srdiDestinationMeter = (SrdiDestinationMeter) e.nextElement();

            SrdiDestinationMetric srdiDestinationMetric = srdiDestinationMeter.collectMetrics();

            if (srdiDestinationMetric != null) {

                /* Fix-me: Apparantly, we can have a case where NO Srdi mteric is tickled, but a 
                 destination metric is available.  This may be a bug.  For now, we'll create the
                 delta in that case.
                 */
                if (prevDelta == null) {
                    createDeltaMetric();
                    prevDelta = deltaMetrics;
                }
                prevDelta.addSrdiDestinationMetric(srdiDestinationMetric);
            }
        }
        deltaMetrics = null;
        return prevDelta;
    }

    public synchronized SrdiDestinationMeter getSrdiDestinationMeter(EndpointAddress endpointAddress) {
        PeerID peerID = MetricUtilities.getPeerIdFromEndpointAddress(endpointAddress);

        return getSrdiDestinationMeter(peerID);
    }
	
    public synchronized SrdiDestinationMeter getSrdiDestinationMeter(String peer) {
        PeerID peerID = MetricUtilities.getPeerIdFromString(peer);

        return getSrdiDestinationMeter(peerID);
    }
	
    public synchronized SrdiDestinationMeter getSrdiDestinationMeter(PeerID peerID) {
		
        SrdiDestinationMeter srdiDestinationMeter = (SrdiDestinationMeter) srdiDestinationMeters.get(peerID);

        if (srdiDestinationMeter == null) {
            srdiDestinationMeter = new SrdiDestinationMeter(peerID);
            srdiDestinationMeters.put(peerID, srdiDestinationMeter);
            cumulativeMetrics.addSrdiDestinationMetric(srdiDestinationMeter.getCumulativeMetrics());
        }

        return srdiDestinationMeter;
    }
	
    private void createDeltaMetric() {
        deltaMetrics = new SrdiHandlerMetric(handlerName);
    }

    @Override
    public String toString() {
        return "SrdiHandlerMeter(" + handlerName + ")";
    }

    public void setRegistered(boolean registered) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.setRegistered(registered);
        cumulativeMetrics.setRegistered(registered);
    }

    // received

    public void messageProcessed(ResolverSrdiMsg srdi, long processTime, EndpointAddress srcAddr) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.messageProcessed(processTime);
        cumulativeMetrics.messageProcessed(processTime);

        SrdiDestinationMeter destinationMeter = getSrdiDestinationMeter(srcAddr);

        destinationMeter.messageProcessed();
    }

    public void errorWhileProcessing(EndpointAddress srcAddr) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
        deltaMetrics.errorWhileProcessing();
        cumulativeMetrics.errorWhileProcessing();

        SrdiDestinationMeter destinationMeter = getSrdiDestinationMeter(srcAddr);

        destinationMeter.errorWhileProcessing();
    }

    public void srdiToUnregisteredHandler(EndpointAddress srcAddr) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }
			
        deltaMetrics.srdiToUnregisteredHandler();
        cumulativeMetrics.srdiToUnregisteredHandler();

        SrdiDestinationMeter destinationMeter = getSrdiDestinationMeter(srcAddr);

        destinationMeter.srdiToUnregisteredHandler();
    }

    // send


    public void messageSentViaUnicast(String peer, ResolverSrdiMsg srdi) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.messageSentViaUnicast();
        cumulativeMetrics.messageSentViaUnicast();	
			
        SrdiDestinationMeter destinationMeter = getSrdiDestinationMeter(peer);

        destinationMeter.messageSentViaUnicast();
    }

    public void messageSentViaWalker(ResolverSrdiMsg srdi) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.messageSentViaWalker();
        cumulativeMetrics.messageSentViaWalker();		
    }

    public void errorSendingMessage() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.errorSendingMessage();
        cumulativeMetrics.errorSendingMessage();		
    }

    public void errorPropagatingMessage() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.errorPropagatingMessage();
        cumulativeMetrics.errorPropagatingMessage();		
    }
}
