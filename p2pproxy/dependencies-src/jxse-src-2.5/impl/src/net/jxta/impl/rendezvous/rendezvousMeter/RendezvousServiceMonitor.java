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

package net.jxta.impl.rendezvous.rendezvousMeter;


import net.jxta.endpoint.EndpointAddress;
import net.jxta.impl.meter.GenericServiceMonitor;
import net.jxta.impl.meter.MetricUtilities;
import net.jxta.meter.ServiceMetric;
import net.jxta.meter.ServiceMonitorFilter;
import net.jxta.peer.PeerID;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.LinkedList;


/**
 * The Service Monitor for the standard Rendezvous Service
 */
public class RendezvousServiceMonitor extends GenericServiceMonitor {
    private Hashtable<PeerID, ClientConnectionMeter> clientConnectionMeters = new Hashtable<PeerID, ClientConnectionMeter>();
    private LinkedList<RendezvousConnectionMeter> rendezvousConnectionMeters = new LinkedList<RendezvousConnectionMeter>();

    private RendezvousMeter rendezvousMeter = new RendezvousMeter();

    private RendezvousServiceMetric cumulativeRendezvousServiceMetric;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void init() {
        cumulativeRendezvousServiceMetric = (RendezvousServiceMetric) getCumulativeServiceMetric();
        cumulativeRendezvousServiceMetric.setRendezvousMetric(rendezvousMeter.getCumulativeMetrics());
    }

    /**
     * Get the General RendezvousMeter
     * @return client RendezvousMeter
     */
    public RendezvousMeter getRendezvousMeter() {
        return rendezvousMeter;
    }

    /**
     * Get a Client Connection Meter
     *
     * @param endpointAddress containing Peer Id for the Meter
     * @return client connection meter
     */
    public synchronized ClientConnectionMeter getClientConnectionMeter(EndpointAddress endpointAddress) {
        PeerID peerID = MetricUtilities.getPeerIdFromEndpointAddress(endpointAddress);

        return getClientConnectionMeter(peerID);
    }

    /**
     * Get a Client Connection Meter
     *
     * @param peerId Peer Id for the Meter
     * @return client connection meter
     */
    public synchronized ClientConnectionMeter getClientConnectionMeter(PeerID peerId) {
        ClientConnectionMeter clientConnectionMeter = clientConnectionMeters.get(peerId);

        if (clientConnectionMeter == null) {
            clientConnectionMeter = new ClientConnectionMeter(peerId);
            clientConnectionMeters.put(peerId, clientConnectionMeter);
            cumulativeRendezvousServiceMetric.addClientConnectionMetric(clientConnectionMeter.getCumulativeMetrics());
        }

        return clientConnectionMeter;
    }

    /**
     * Get a Client Connection Meter
     *
     * @param peerIdString Peer Id as a String
     * @return client connection meter
     */
    public synchronized ClientConnectionMeter getClientConnectionMeter(String peerIdString) {
        PeerID peerID = MetricUtilities.getPeerIdFromString(peerIdString);

        return getClientConnectionMeter(peerID);
    }

    /*
     public synchronized void removeClientConnectionMeter(String peerId) {
     clientConnectionMeters.remove(peerId);
     }
     */

    /**
     * Get a Rendezvous Connection Meter
     *
     * @param peerIdStr Peer Id for the Meter as a String
     * @return  the Rendezvous Connection Meter
     */
    public synchronized RendezvousConnectionMeter getRendezvousConnectionMeter(String peerIdStr) {
        PeerID peerID = MetricUtilities.getPeerIdFromString(peerIdStr);

        return getRendezvousConnectionMeter(peerID);
    }

    /**
     * Get a Rendezvous Connection Meter
     *
     * @param peerID Peer Id for the Meter
     * @return  the Rendezvous Connection Meter
     */
    public synchronized RendezvousConnectionMeter getRendezvousConnectionMeter(PeerID peerID) {
        if (peerID == null) {
            peerID = MetricUtilities.BAD_PEERID;
        }

        for (Object rendezvousConnectionMeter1 : rendezvousConnectionMeters) {
            RendezvousConnectionMeter rendezvousConnectionMeter = (RendezvousConnectionMeter) rendezvousConnectionMeter1;

            if (peerID.equals(rendezvousConnectionMeter.getPeerID())) {
                return rendezvousConnectionMeter;
            }
        }

        RendezvousConnectionMeter rendezvousConnectionMeter = new RendezvousConnectionMeter(peerID);

        rendezvousConnectionMeters.add(rendezvousConnectionMeter);
        cumulativeRendezvousServiceMetric.addRendezvousConnectionMetric(rendezvousConnectionMeter.getCumulativeMetrics());

        return rendezvousConnectionMeter;
    }

    /*
     public synchronized void removeRendezvousConnectionMeter(PeerID peerID) {
     if (peerID == null)
     return;
     
     for (Iterator i = rendezvousConnectionMeters.iterator(); i.hasNext(); ) {
     RendezvousConnectionMeter rendezvousConnectionMeter = (RendezvousConnectionMeter)i.next();
     if (peerID.equals(rendezvousConnectionMeter.getPeerID())) {
     i.remove();
     return;
     }
     }
     }
     */

    /**
     * {@inheritDoc}
     */
    @Override
    protected synchronized ServiceMetric collectServiceMetrics() {
        RendezvousServiceMetric rendezvousServiceMetric = (RendezvousServiceMetric) createServiceMetric();

        boolean anyData = false;

        for (Enumeration<ClientConnectionMeter> e = clientConnectionMeters.elements(); e.hasMoreElements();) {
            ClientConnectionMeter clientConnectionMeter = e.nextElement();
            ClientConnectionMetric clientConnectionMetric = clientConnectionMeter.collectMetrics(); // clears delta from meter

            if (clientConnectionMetric != null) {
                rendezvousServiceMetric.addClientConnectionMetric(clientConnectionMetric);
                anyData = true;
            }
        }

        for (RendezvousConnectionMeter rendezvousConnectionMeter : rendezvousConnectionMeters) {
            RendezvousConnectionMetric rendezvousConnectionMetric = rendezvousConnectionMeter.collectMetrics(); // clears delta from meter

            if (rendezvousConnectionMetric != null) {
                rendezvousServiceMetric.addRendezvousConnectionMetric(rendezvousConnectionMetric);
                anyData = true;
            }
        }

        RendezvousMetric rendezvousMetric = rendezvousMeter.collectMetrics();

        if (rendezvousMetric != null) {
            rendezvousServiceMetric.setRendezvousMetric(rendezvousMetric);
            anyData = true;
        }

        if (anyData) {
            return rendezvousServiceMetric;
        } else {
            return null;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ServiceMetric getServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime, int pulseIndex, long reportRate) {
        int deltaReportRateIndex = monitorManager.getReportRateIndex(reportRate);
        RendezvousServiceMetric origMetric = (RendezvousServiceMetric) deltaServiceMetrics[deltaReportRateIndex];

        if (origMetric == null) {
            return null;
        }

        RendezvousServiceMonitorFilter rendezvousServiceMonitorFilter = (RendezvousServiceMonitorFilter) serviceMonitorFilter;

        return origMetric.shallowCopy(rendezvousServiceMonitorFilter);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ServiceMetric getCumulativeServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime) {
        RendezvousServiceMonitorFilter rendezvousServiceMonitorFilter = (RendezvousServiceMonitorFilter) serviceMonitorFilter;
        RendezvousServiceMetric origMetric = (RendezvousServiceMetric) getCumulativeServiceMetric();

        return origMetric.deepCopy(rendezvousServiceMonitorFilter);
    }

}
