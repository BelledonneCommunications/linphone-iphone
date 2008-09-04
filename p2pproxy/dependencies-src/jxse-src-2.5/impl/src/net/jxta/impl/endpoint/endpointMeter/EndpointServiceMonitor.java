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

package net.jxta.impl.endpoint.endpointMeter;


import java.util.Hashtable;
import java.util.Map;

import net.jxta.endpoint.EndpointAddress;
import net.jxta.impl.meter.GenericServiceMonitor;
import net.jxta.meter.ServiceMetric;
import net.jxta.meter.ServiceMonitorFilter;


/**
 *  Standard EndpointService Monitor
 **/
public class EndpointServiceMonitor extends GenericServiceMonitor {
    private EndpointServiceMetric cumulativeEndpointServiceMetric;
    private final Map<String, InboundMeter> inboundMeters = new Hashtable<String, InboundMeter>();
    private final Map<EndpointAddress, OutboundMeter> outboundMeters = new Hashtable<EndpointAddress, OutboundMeter>();
    private final Map<String, PropagationMeter> propagationMeters = new Hashtable<String, PropagationMeter>();
    private final EndpointMeter endpointMeter = new EndpointMeter();

    public EndpointServiceMonitor() {}

    /**
     *  {@inheritDoc}
     */
    @Override
    protected void init() {
        cumulativeEndpointServiceMetric = (EndpointServiceMetric) getCumulativeServiceMetric();
        cumulativeEndpointServiceMetric.setEndpointMetric(endpointMeter.getCumulativeMetrics());
    }

    public EndpointMeter getEndpointMeter() {
        return endpointMeter;
    }

    public synchronized InboundMeter getInboundMeter(String serviceName, String serviceParam) {
        String address = serviceName;

        if (null != serviceParam) {
            address += "/" + serviceParam;
        }

        InboundMeter inboundMeter = inboundMeters.get(address);

        if (inboundMeter == null) {
            inboundMeter = new InboundMeter(serviceName, serviceParam);
            inboundMeters.put(address, inboundMeter);
            cumulativeEndpointServiceMetric.addInboundMetric(inboundMeter.getCumulativeMetrics());
        }

        return inboundMeter;
    }

    public synchronized PropagationMeter getPropagationMeter(String serviceName, String serviceParam) {
        String address = serviceName;

        if (null != serviceParam) {
            address += "/" + serviceParam;
        }

        PropagationMeter propagationMeter = propagationMeters.get(address);

        if (propagationMeter == null) {
            propagationMeter = new PropagationMeter(serviceName, serviceParam);
            propagationMeters.put(address, propagationMeter);
            cumulativeEndpointServiceMetric.addPropagationMetric(propagationMeter.getCumulativeMetrics());
        }

        return propagationMeter;
    }

    public synchronized OutboundMeter getOutboundMeter(EndpointAddress endpointAddress) {
        OutboundMeter outboundMeter = outboundMeters.get(endpointAddress);

        if (outboundMeter == null) {
            outboundMeter = new OutboundMeter(endpointAddress);
            cumulativeEndpointServiceMetric.addOutboundMetric(outboundMeter.getCumulativeMetrics());
            outboundMeters.put(endpointAddress, outboundMeter);
        }

        return outboundMeter;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public ServiceMetric getServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime, int pulseIndex, long reportRate) {
        int deltaReportRateIndex = monitorManager.getReportRateIndex(reportRate);
        EndpointServiceMetric origEndpointServiceMetric = (EndpointServiceMetric) deltaServiceMetrics[deltaReportRateIndex];

        if (origEndpointServiceMetric == null) {
            return null;
        }

        EndpointServiceMonitorFilter endpointServiceMonitorFilter = (EndpointServiceMonitorFilter) serviceMonitorFilter;

        return origEndpointServiceMetric.shallowCopy(endpointServiceMonitorFilter);
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public ServiceMetric getCumulativeServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime) {
        EndpointServiceMetric origEndpointServiceMetric = (EndpointServiceMetric) cumulativeServiceMetric;
        EndpointServiceMonitorFilter endpointServiceMonitorFilter = (EndpointServiceMonitorFilter) serviceMonitorFilter;

        return origEndpointServiceMetric.deepCopy(endpointServiceMonitorFilter);
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    protected ServiceMetric collectServiceMetrics() {
        EndpointServiceMetric endpointServiceMetric = (EndpointServiceMetric) createServiceMetric();

        boolean anyData = false;

        for (InboundMeter inboundMeter : inboundMeters.values()) {
            InboundMetric inboundMetric = inboundMeter.collectMetrics(); // clears delta from meter

            if (inboundMetric != null) {
                endpointServiceMetric.addInboundMetric(inboundMetric);
                anyData = true;
            }
        }

        for (OutboundMeter outboundMeter : outboundMeters.values()) {
            OutboundMetric outboundMetric = outboundMeter.collectMetrics(); // clears delta from meter

            if (outboundMetric != null) {
                endpointServiceMetric.addOutboundMetric(outboundMetric);
                anyData = true;
            }
        }

        for (PropagationMeter propagationMeter : propagationMeters.values()) {
            PropagationMetric propagationMetric = propagationMeter.collectMetrics(); // clears delta from meter

            if (propagationMetric != null) {
                endpointServiceMetric.addPropagationMetric(propagationMetric);
                anyData = true;
            }
        }

        EndpointMetric endpointMetric = endpointMeter.collectMetrics();

        if (endpointMetric != null) {
            endpointServiceMetric.setEndpointMetric(endpointMetric);
            anyData = true;
        }

        if (anyData) {
            return endpointServiceMetric;
        } else {
            return null;
        }
    }
}
