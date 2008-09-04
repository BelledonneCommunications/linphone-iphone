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


import net.jxta.endpoint.EndpointAddress;
import net.jxta.impl.meter.MetricUtilities;
import net.jxta.peer.PeerID;

import java.util.Enumeration;
import java.util.Hashtable;


/**
 * Transport Meter for a specific registered Transport
 */
public class TransportMeter {
    public static final EndpointAddress UNKNOWN_ADDRESS = new EndpointAddress("<unknown>", "<unknown>", null, null);
    public static final String UNKNOWN_PEER = MetricUtilities.UNKNOWN_PEERID.toString();
    private String protocol;
    private EndpointAddress endpointAddress;

    private Hashtable<EndpointAddress, TransportBindingMeter> transportBindingMeters = new Hashtable<EndpointAddress, TransportBindingMeter>();
    private TransportMetric cumulativeMetrics;

    public TransportMeter(String protocol, EndpointAddress endpointAddress) {
        this.endpointAddress = endpointAddress;
        this.protocol = protocol;
        cumulativeMetrics = new TransportMetric(this);
    }

    public TransportMetric getCumulativeMetrics() {
        return cumulativeMetrics;
    }

    public TransportMetric collectMetrics() {
        TransportMetric transportMetric = new TransportMetric(this);
        boolean anyData = false;

        for (Enumeration<TransportBindingMeter> e = transportBindingMeters.elements(); e.hasMoreElements();) {
            TransportBindingMeter transportBindingMeter = e.nextElement();
            TransportBindingMetric transportBindingMetric = transportBindingMeter.collectMetrics();

            if (transportBindingMetric != null) {
                transportMetric.addTransportBindingMetric(transportBindingMetric);
                anyData = true;
            }
        }

        if (anyData) {
            return transportMetric;
        } else {
            return null;
        }
    }

    /**
     * Get a specific Binding Meter corresponding to a connection for this transport
     *
     * @param peerIdString       PeerID of destination
     * @param destinationAddress Destination Address of connected peer transport
     * @return The Binding Meter for tracking this connection
     */
    public synchronized TransportBindingMeter getTransportBindingMeter(String peerIdString, EndpointAddress destinationAddress) {
        PeerID peerID = MetricUtilities.getPeerIdFromString(peerIdString);

        return getTransportBindingMeter(peerID, destinationAddress);
    }

    /**
     * Get a specific Binding Meter corresponding to a connection for this transport
     *
     * @param peerID  destination <code>PeerID<code>
     * @param destinationAddress Destination Address of connected peer transport
     * @return The Binding Meter for tracking this connection
     */
    public synchronized TransportBindingMeter getTransportBindingMeter(PeerID peerID, EndpointAddress destinationAddress) {
        destinationAddress = new EndpointAddress(destinationAddress, null, null);

        TransportBindingMeter transportBindingMeter = transportBindingMeters.get(destinationAddress);

        if (transportBindingMeter == null) {
            transportBindingMeter = new TransportBindingMeter(peerID, destinationAddress);
            transportBindingMeters.put(destinationAddress, transportBindingMeter);
            cumulativeMetrics.addTransportBindingMetric(transportBindingMeter.getCumulativeMetrics());
        } else {
            transportBindingMeter.setPeerID(peerID);
        }

        return transportBindingMeter;
    }

    public Enumeration<TransportBindingMeter> getTransportBindingMeters() {
        return transportBindingMeters.elements();
    }

    public int getTransportBindingCount() {
        return transportBindingMeters.size();
    }

    public String getProtocol() {
        return protocol;
    }

    public EndpointAddress getEndpointAddress() {
        return endpointAddress;
    }

    @Override
    public String toString() {
        return "TransportMeter(" + endpointAddress + ")";
    }
}
