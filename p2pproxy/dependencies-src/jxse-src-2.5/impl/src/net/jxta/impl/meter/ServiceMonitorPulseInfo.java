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

package net.jxta.impl.meter;


import net.jxta.meter.*;
import java.util.*;


public class ServiceMonitorPulseInfo {
    public static final int NOT_PULSING = MonitorManager.NOT_PULSING;
	
    private MonitorManager monitorManager;
    ServiceMonitor serviceMonitor;
    private LinkedList<RegisteredServiceMonitorFilter> registeredServiceMonitorFilters = new LinkedList<RegisteredServiceMonitorFilter>();
    private int pulseRateIndex = NOT_PULSING; // quickestReportRateIndex registered for this service
    private long pulseRate = NOT_PULSING; // quickestReportRate registered for this service
    private int filtersPerRate[];
    private boolean pulsing;

    private class RegisteredServiceMonitorFilter {
        ServiceMonitorFilter serviceMonitorFilter;
        int reportRateIndex;

        RegisteredServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter, int reportRateIndex) {
            this.serviceMonitorFilter = serviceMonitorFilter;
            this.reportRateIndex = reportRateIndex;
        }
    }

    public ServiceMonitorPulseInfo() {}

    ServiceMonitorPulseInfo(MonitorManager monitorManager, ServiceMonitor serviceMonitor) {
        this.monitorManager = monitorManager;
        this.serviceMonitor = serviceMonitor;
        filtersPerRate = new int[monitorManager.getReportRatesCount()];
    }

    public ServiceMonitor getServiceMonitor() {
        return serviceMonitor;
    }

    public int getPulseRateIndex() {
        return pulseRateIndex;
    }

    public long getPulseRate() {
        return pulseRate;
    }

    public boolean isPulsing() {
        return pulsing;
    }

    public boolean isEvenPulseForRateIndex(int pulseRateIndex) {
        return monitorManager.isEvenPulseForRateIndex(pulseRateIndex);
    }

    public boolean isRegisteredFilterForRate(int pulseRateIndex) {
        return (filtersPerRate[pulseRateIndex] != 0);
    }	

    void registerServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter, int reportRateIndex, long reportRate) {
        RegisteredServiceMonitorFilter registeredServiceMonitorFilter = new RegisteredServiceMonitorFilter(serviceMonitorFilter
                ,
                reportRateIndex);

        registeredServiceMonitorFilters.add(registeredServiceMonitorFilter);

        filtersPerRate[reportRateIndex]++;

        if ((pulseRateIndex == NOT_PULSING) || (reportRateIndex < pulseRateIndex)) {
            int oldPulseRateIndex = pulseRateIndex;

            pulseRateIndex = reportRateIndex;
            pulseRate = reportRate;
            pulsing = true;
            if (serviceMonitor instanceof ServiceMonitorImpl) {
                ((ServiceMonitorImpl) serviceMonitor).resetPulseRate(this, oldPulseRateIndex);
            }
        }

        boolean newRate = (filtersPerRate[reportRateIndex] == 1);

        serviceMonitor.serviceMonitorFilterRegistered(serviceMonitorFilter, reportRateIndex, reportRate, newRate);
    }

    boolean deregisterServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter, int reportRateIndex, long reportRate) {
        boolean removed = false;
		
        for (Iterator<RegisteredServiceMonitorFilter> i = registeredServiceMonitorFilters.iterator(); i.hasNext();) {
            RegisteredServiceMonitorFilter registeredMonitorFilter = i.next();

            if ((registeredMonitorFilter.serviceMonitorFilter == serviceMonitorFilter)
                    && (registeredMonitorFilter.reportRateIndex == reportRateIndex)) {
                i.remove();
                removed = true;
                break;
            }
        }

        if (!removed) { // Fix-Me: This should never happen if Monitor Manager works properly.
            return false;
        }

        filtersPerRate[reportRateIndex]--;

        serviceMonitor.serviceMonitorFilterDeregistered(serviceMonitorFilter, reportRateIndex, reportRate
                ,
                filtersPerRate[reportRateIndex] == 0);

        if ((filtersPerRate[reportRateIndex] == 0) && (reportRateIndex == pulseRateIndex)) { 
            // pulseRateSlowing Down
            int newPulseRateIndex = NOT_PULSING;
			
            for (int i = pulseRateIndex; i < filtersPerRate.length; i++) {
                if (filtersPerRate[i] != 0) {
                    newPulseRateIndex = i;
                    break;
                }
            }

            int oldPulserRateIndex = pulseRateIndex;

            pulseRateIndex = newPulseRateIndex;

            if (newPulseRateIndex != NOT_PULSING) {
                pulseRate = monitorManager.getReportRate(pulseRateIndex);
                pulsing = true;
            } else {
                pulseRate = NOT_PULSING;
                pulsing = false;
            }

            if (serviceMonitor instanceof ServiceMonitorImpl) {
                ((ServiceMonitorImpl) serviceMonitor).resetPulseRate(this, oldPulserRateIndex);
            }
        }
        return removed;
    }
}
