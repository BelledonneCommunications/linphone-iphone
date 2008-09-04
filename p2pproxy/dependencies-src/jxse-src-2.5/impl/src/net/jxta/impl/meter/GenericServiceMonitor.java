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


import net.jxta.document.Advertisement;
import net.jxta.exception.JxtaException;
import net.jxta.id.ID;
import net.jxta.meter.MonitorResources;
import net.jxta.meter.ServiceMetric;
import net.jxta.meter.ServiceMonitorFilter;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.platform.ModuleClassID;
import net.jxta.protocol.ModuleImplAdvertisement;


public abstract class GenericServiceMonitor implements ServiceMonitorImpl, Module {
    private ModuleClassID moduleClassID;
    protected MonitorManager monitorManager;

    protected long reportRate;
    protected int reportRateIndex;
    protected ServiceMetric cumulativeServiceMetric;
    protected ServiceMetric deltaServiceMetrics[];
    protected ModuleImplAdvertisement implAdvertisement;

    public GenericServiceMonitor() {}

    // public void init(MonitorManager monitorManager, ModuleClassID moduleClassID) {

    public void init(PeerGroup group, ID assignedID, Advertisement advertisement) {
        group.unref(); // We do not use the group. These are not quite real modules.
        this.implAdvertisement = (ModuleImplAdvertisement) advertisement;
        this.moduleClassID = (ModuleClassID) assignedID;
    }

    public void init(MonitorManager monitorManager) {
        this.monitorManager = monitorManager;

        if (MeterBuildSettings.METERING) {
            cumulativeServiceMetric = createServiceMetric();
            deltaServiceMetrics = new ServiceMetric[monitorManager.getReportRatesCount()];
            init();
        }
    }

    /*
     public void init(MonitorManager monitorManager) {
     this.monitorManager = monitorManager;
     }
     */

    public int startApp(java.lang.String[] args) {
        return 0;
    } // fix-me: what's the right return?

    public void stopApp() {}

    protected void init() {}

    public ModuleClassID getModuleClassID() {
        return moduleClassID;
    }

    public PeerGroup getPeerGroup() {
        return monitorManager.getPeerGroup();
    }

    protected ServiceMetric getCumulativeServiceMetric() {
        return cumulativeServiceMetric;
    }

    public void resetPulseRate(ServiceMonitorPulseInfo pulseInfo, int oldPulseRateIndex) {
        this.reportRate = pulseInfo.getPulseRate();
        this.reportRateIndex = pulseInfo.getPulseRateIndex();
    }

    public void validateCumulativeServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter) {// base implementation is to accept
    }

    public ServiceMonitorFilter createSupportedCumulativeServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter) {
        // base implementation is to accept
        return serviceMonitorFilter;
    }

    public void validateServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter, long reportRate) {// base implementation is to accept
    }

    public ServiceMonitorFilter createSupportedServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter, long reportRate) {
        // base implementation is to accept
        return serviceMonitorFilter;
    }

    protected ServiceMetric createServiceMetric() {
        try {
            return MonitorResources.createServiceMetric(moduleClassID);
        } catch (JxtaException e) { // this will always succeed since we were able to lad the monitor
            return null;
        }
    }

    /**
     * Get the service metrics accrued since the last pulse
     *
     * @return null if there were no metrices since the last call
     */
    protected abstract ServiceMetric collectServiceMetrics();

    public synchronized void beginPulse(ServiceMonitorPulseInfo pulseInfo) {
        ServiceMetric baseDeltaServiceMetric = collectServiceMetrics();

        deltaServiceMetrics[reportRateIndex] = baseDeltaServiceMetric;

        if (baseDeltaServiceMetric != null) {
            for (int reportRate = reportRateIndex + 1; reportRate < deltaServiceMetrics.length; reportRate++) {

                if (pulseInfo.isRegisteredFilterForRate(reportRate)) {

                    if (deltaServiceMetrics[reportRate] == null) {
                        deltaServiceMetrics[reportRate] = createServiceMetric();
                    }

                    deltaServiceMetrics[reportRate].mergeMetrics(baseDeltaServiceMetric);
                }
            }
        }
    }

    public void endPulse(ServiceMonitorPulseInfo pulseInfo) {
        for (int reportRateIndex = 0; reportRateIndex < monitorManager.getReportRatesCount(); reportRateIndex++) {
            if (pulseInfo.isEvenPulseForRateIndex(reportRateIndex)) {
                deltaServiceMetrics[reportRateIndex] = null;
            }
        }
    }

    public void beginCumulativeReport() {}

    public void endCumulativeReport() {}

    public ServiceMetric getServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime, int reportIndex, long reportRate) {
        int deltaReportRateIndex = monitorManager.getReportRateIndex(reportRate);

        // Fix-Me: For now we are not yet  supporting filters

        return deltaServiceMetrics[deltaReportRateIndex];
    }

    public ServiceMetric getCumulativeServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime) {
        // Fix-Me: For now we are not yet  supporting filters

        return cumulativeServiceMetric;
    }

    public void serviceMonitorFilterRegistered(ServiceMonitorFilter serviceMonitorFilter, int reportRateIndex, long reportRate, boolean newRate) {
        if (newRate) {
            deltaServiceMetrics[reportRateIndex] = createServiceMetric();
        }
    }

    public void serviceMonitorFilterDeregistered(ServiceMonitorFilter serviceMonitorFilter, int reportRateIndex, long reportRate, boolean retiredRate) {
        if (retiredRate) {
            deltaServiceMetrics[reportRateIndex] = null;
        }
    }

    public void destroy() {}
}
