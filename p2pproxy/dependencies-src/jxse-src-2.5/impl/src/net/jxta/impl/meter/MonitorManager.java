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
import net.jxta.impl.util.TimeUtils;
import net.jxta.meter.MonitorEvent;
import net.jxta.meter.MonitorException;
import net.jxta.meter.MonitorFilter;
import net.jxta.meter.MonitorFilterException;
import net.jxta.meter.MonitorListener;
import net.jxta.meter.MonitorReport;
import net.jxta.meter.MonitorResources;
import net.jxta.meter.PeerMonitorInfo;
import net.jxta.meter.ServiceMetric;
import net.jxta.meter.ServiceMonitor;
import net.jxta.meter.ServiceMonitorFilter;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.platform.ModuleClassID;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.service.Service;
import net.jxta.util.documentSerializable.DocumentSerializableUtilities;
import net.jxta.util.documentSerializable.DocumentSerializationException;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.LinkedList;


public class MonitorManager implements Service {
    private final static long timeZero = System.currentTimeMillis();
    public static final int NOT_PULSING = -1;

    private static final int NO_PRIOR_REPORT = 0;

    private static long supportedReportRates[] = new long[] {
        500, TimeUtils.ASECOND, 5 * TimeUtils.ASECOND, 10 * TimeUtils.ASECOND, 15 * TimeUtils.ASECOND, 30 * TimeUtils.ASECOND
                ,
        TimeUtils.AMINUTE, 5 * TimeUtils.AMINUTE, 10 * TimeUtils.AMINUTE, 15 * TimeUtils.AMINUTE, 30 * TimeUtils.AMINUTE
                ,
        TimeUtils.ANHOUR, 3 * TimeUtils.ANHOUR, 6 * TimeUtils.ANHOUR, 12 * TimeUtils.ANHOUR, TimeUtils.ADAY, TimeUtils.AWEEK};

    private int pulsesPerRate[] = new int[supportedReportRates.length];
    private long startTime = System.currentTimeMillis();

    private LinkedList<MonitorListenerInfo> monitorListenerInfos = new LinkedList<MonitorListenerInfo>();
    private Hashtable<ModuleClassID, ServiceMonitorPulseInfo> serviceMonitorPulseInfos = new Hashtable<ModuleClassID, ServiceMonitorPulseInfo>();
    private int filtersPerRate[] = new int[supportedReportRates.length];
    private long previousReportTimes[] = new long[supportedReportRates.length];

    private PeerGroup peerGroup;
    private Thread reportThread;

    private long pulseRate = NOT_PULSING;
    private int pulseRateIndex = NOT_PULSING;
    private int pulseNumber = 0;
    private long nextPulseTime = NO_PRIOR_REPORT;
    private boolean isRunning = true; // true until monitor is destroyed, triggers termination of report thread

    private ModuleClassID[] supportedModuleClassIDs;
    private ModuleImplAdvertisement implAdvertisement;
    private long lastResetTime = System.currentTimeMillis();

    public Advertisement getImplAdvertisement() {
        return implAdvertisement;
    }

    public Service getInterface() {
        // This is good enough. No need to get fancy.
        return this;
    }

    // public MonitorManager(PeerGroup peerGroup) {

    public void init(PeerGroup peerGroup, ID assignedID, Advertisement implAdvertisement) {
        this.implAdvertisement = (ModuleImplAdvertisement) implAdvertisement;
        this.peerGroup = peerGroup;
        createReportThread();

        for (int i = 0; i < previousReportTimes.length; i++) {
            pulsesPerRate[i] = (int) (supportedReportRates[i] / supportedReportRates[0]);
        }
    }

    public int startApp(java.lang.String[] args) {
        return 0; // fix-me
    }

    public void stopApp() {
        destroy();
    }

    private class MonitorListenerInfo {
        MonitorListener monitorListener;
        MonitorFilter monitorFilter;
        long reportRate;
        int reportRateIndex;
        boolean sendCumulativeFirst = false;
        boolean wasCumulativeSent = false;

        MonitorListenerInfo(MonitorListener monitorListener, long reportRate, MonitorFilter monitorFilter, boolean cumulativeFirst) {
            this.monitorListener = monitorListener;
            this.monitorFilter = monitorFilter;
            this.reportRate = reportRate;
            this.sendCumulativeFirst = cumulativeFirst;
            this.reportRateIndex = getReportRateIndex(reportRate);
        }
    }

    public static long[] getReportRates() { // return copy so that users can't modify.
        long copy[] = new long[supportedReportRates.length];

        System.arraycopy(supportedReportRates, 0, copy, 0, supportedReportRates.length);
        return copy;
    }

    public boolean isLocalMonitoringAvailable(ModuleClassID moduleClassID) {
        ServiceMonitor serviceMonitor = getServiceMonitor(moduleClassID);

        return (serviceMonitor != null);
    }

    public PeerGroup getPeerGroup() {
        return peerGroup;
    }

    // Cooperate with the code that loaded this module to replace the strong
    // group interface given by init() with a non-counted one.
    private void setPeerGroup(PeerGroup pg) {
        PeerGroup tmp = peerGroup;

        peerGroup = pg;
        tmp.unref();
        tmp = null;
    }

    public PeerMonitorInfo getPeerMonitorInfo() {
        long[] reportRates = getReportRates(); // makes a copy
        ModuleClassID[] moduleClassIDs = getMonitorableServiceTypes(); // ensures that array is initialized.
        long runningTime = System.currentTimeMillis() - lastResetTime;

        return new PeerMonitorInfo(MeterBuildSettings.METERING, moduleClassIDs, reportRates, lastResetTime, runningTime);
    }

    public int getReportRatesCount() {
        return supportedReportRates.length;
    }

    public int getReportRateIndex(long reportRate) {
        for (int i = 0; i < supportedReportRates.length; i++) {
            if (supportedReportRates[i] == reportRate) {
                return i;
            }
        }

        return -1;
    }

    public boolean isSupportedReportRate(long reportRate) {
        return getReportRateIndex(reportRate) >= 0;
    }

    public long getReportRate(int index) {
        return supportedReportRates[index];
    }

    public long getBestReportRate(long desiredReportRate) {
        for (long supportedReportRate : supportedReportRates) {
            if (desiredReportRate <= supportedReportRate) {
                return supportedReportRate;
            }
        }

        return supportedReportRates[supportedReportRates.length - 1];
    }

    public ServiceMonitor getServiceMonitor(ModuleClassID moduleClassID) {
        ServiceMonitorPulseInfo serviceMonitorPulseInfo = serviceMonitorPulseInfos.get(moduleClassID);

        if (serviceMonitorPulseInfo != null) {
            return serviceMonitorPulseInfo.serviceMonitor;
        } else {

            try {
                ModuleImplAdvertisement moduleImplAdvertisement = MonitorResources.getServiceMonitorImplAdvertisement(
                        moduleClassID, implAdvertisement);
                ServiceMonitor serviceMonitor = (ServiceMonitor) peerGroup.loadModule(moduleClassID, moduleImplAdvertisement);

                MonitorResources.registerServiceMonitorModuleImplAdvertisement(moduleImplAdvertisement);

                if (serviceMonitor instanceof ServiceMonitorImpl) {
                    ((ServiceMonitorImpl) serviceMonitor).init(this);

                }

                serviceMonitorPulseInfo = new ServiceMonitorPulseInfo(this, serviceMonitor);
                serviceMonitorPulseInfos.put(moduleClassID, serviceMonitorPulseInfo);
                return serviceMonitor;
            } catch (JxtaException e) {
                throw new RuntimeException("Unable to load Service Monitor: " + moduleClassID + "\n\tException: " + e);
            }
        }
    }

    private void resetPulseRate() {
        int oldPulseRateIndex = pulseRateIndex;

        pulseRateIndex = NOT_PULSING;
        pulseRate = NOT_PULSING;

        for (int i = 0; i < filtersPerRate.length; i++) {
            if (filtersPerRate[i] != 0) {
                pulseRateIndex = i;
                pulseRate = getReportRate(pulseRateIndex);
                break;
            }
        }

        if (oldPulseRateIndex == pulseRateIndex) {
            return;
        }    // nothing changed

        long now = System.currentTimeMillis();

        if (oldPulseRateIndex == NOT_PULSING) { // case 1: No pulse to pulse
            for (int i = 0; i < filtersPerRate.length; i++) {
                if (filtersPerRate[i] != 0) {
                    previousReportTimes[i] = now;
                } else {
                    previousReportTimes[i] = NO_PRIOR_REPORT;
                }
            }

            pulseNumber = 0;
            nextPulseTime = now + pulseRate;
        } else if (pulseRateIndex == NOT_PULSING) {// case 2: pulse to No pulse
            // Do nothing
        } else if (pulseRateIndex < oldPulseRateIndex) { // case 3: pulse going to a faster pulse
            for (int i = pulseRateIndex; i < (oldPulseRateIndex - 1); i++) {
                if (filtersPerRate[i] != 0) {
                    previousReportTimes[i] = now;
                } else {
                    previousReportTimes[i] = NO_PRIOR_REPORT;
                }
            }

            long timeToNextPulse = nextPulseTime - now;

            if (pulseRate < timeToNextPulse) {
                int numPulsesToNow = (int) (timeToNextPulse / pulseRate);
                int numNewToOldPulses = (int) (supportedReportRates[oldPulseRateIndex] / supportedReportRates[pulseRateIndex]);

                pulseNumber += (numNewToOldPulses - numPulsesToNow) * pulsesPerRate[pulseRateIndex];
                timeToNextPulse = now - (numPulsesToNow * pulseRate);
            } else {
                pulseNumber += (pulsesPerRate[oldPulseRateIndex] - pulsesPerRate[pulseRateIndex]);
            }

        } else if (pulseRateIndex > oldPulseRateIndex) { // case 3: pulse going to a  slower pulse
            int nextPulseNumber = pulseNumber + pulsesPerRate[oldPulseRateIndex];

            pulseNumber = ((nextPulseNumber - 1) / pulsesPerRate[pulseRateIndex]) * pulsesPerRate[pulseRateIndex];
            nextPulseTime += (nextPulseNumber - pulseNumber) * supportedReportRates[0];

            for (int i = 0; i < pulseRateIndex; i++) {
                previousReportTimes[i] = NO_PRIOR_REPORT;
            }
        }

        reportThread.interrupt();
    }

    private MonitorReport getMonitorReport(MonitorFilter monitorFilter, long reportRate, long previousDeltaTime, long beginReportTime) {
        MonitorReport monitorReport = new MonitorReport(previousDeltaTime, beginReportTime, false);

        for (Iterator i = monitorFilter.getModuleClassIDs(); i.hasNext();) {
            ModuleClassID moduleClassID = (ModuleClassID) i.next();

            ServiceMonitorFilter serviceMonitorFilter = monitorFilter.getServiceMonitorFilter(moduleClassID);

            ServiceMonitor serviceMonitor = getServiceMonitor(moduleClassID);

            if (serviceMonitorFilter != null) {
                ServiceMetric serviceMetric = serviceMonitor.getServiceMetric(serviceMonitorFilter, previousDeltaTime
                        ,
                        beginReportTime, getReportRateIndex(reportRate), reportRate);

                if (serviceMetric != null) {
                    monitorReport.addServiceMetric(serviceMetric);
                }
            }
        }
        return monitorReport;
    }

    public void validateCumulativeMonitorFilter(MonitorFilter monitorFilter) throws MonitorFilterException {
        boolean isAnyServiceFilters = false;

        for (Iterator i = monitorFilter.getServiceMonitorFilters(); i.hasNext();) {
            ServiceMonitorFilter serviceMonitorFilter = (ServiceMonitorFilter) i.next();

            ModuleClassID moduleClassID = serviceMonitorFilter.getModuleClassID();
            ServiceMonitor serviceMonitor = getServiceMonitor(moduleClassID);

            if (serviceMonitor == null) {
                throw new MonitorFilterException(MonitorFilterException.SERVICE_NOT_SUPPORTED, moduleClassID);
            }

            serviceMonitor.validateCumulativeServiceMonitorFilter(serviceMonitorFilter);
            isAnyServiceFilters = true;
        }

        if (!isAnyServiceFilters) {
            throw new MonitorFilterException("Empty Monitor Filter");
        }

    }

    public void validateMonitorFilter(MonitorFilter monitorFilter, long reportRate) throws MonitorFilterException {

        if (!isSupportedReportRate(reportRate)) {
            throw new MonitorFilterException(MonitorFilterException.REPORT_RATE_NOT_SUPPORTED, reportRate);
        }

        boolean isAnyServiceFilters = false;

        for (Iterator i = monitorFilter.getServiceMonitorFilters(); i.hasNext();) {
            ServiceMonitorFilter serviceMonitorFilter = (ServiceMonitorFilter) i.next();

            ModuleClassID moduleClassID = serviceMonitorFilter.getModuleClassID();
            ServiceMonitor serviceMonitor = getServiceMonitor(moduleClassID);

            if (serviceMonitor == null) {
                throw new MonitorFilterException(MonitorFilterException.SERVICE_NOT_SUPPORTED, moduleClassID);
            }

            serviceMonitor.validateServiceMonitorFilter(serviceMonitorFilter, reportRate);
            isAnyServiceFilters = true;
        }

        if (!isAnyServiceFilters) {
            throw new MonitorFilterException("Empty Monitor Filter");
        }
    }

    public MonitorFilter createSupportedCumulativeMonitorFilter(MonitorFilter monitorFilter) throws MonitorFilterException {
        MonitorFilter newMonitorFilter = new MonitorFilter(monitorFilter.getDescription());
        boolean anythingAdded = false;

        for (Iterator i = monitorFilter.getServiceMonitorFilters(); i.hasNext();) {
            ServiceMonitorFilter serviceMonitorFilter = (ServiceMonitorFilter) i.next();

            ModuleClassID moduleClassID = serviceMonitorFilter.getModuleClassID();
            ServiceMonitor serviceMonitor = getServiceMonitor(moduleClassID);

            if (serviceMonitor == null) {
                continue;
            }

            ServiceMonitorFilter newServiceMonitorFilter = serviceMonitor.createSupportedCumulativeServiceMonitorFilter(
                    serviceMonitorFilter);

            if (newServiceMonitorFilter != null) {
                newMonitorFilter.addServiceMonitorFilter(newServiceMonitorFilter);
                anythingAdded = true;
            }
        }

        if (anythingAdded) {
            return newMonitorFilter;
        } else {
            return null;
        }
    }

    public MonitorFilter createSupportedMonitorFilter(MonitorFilter monitorFilter, long reportRate) throws MonitorFilterException {
        MonitorFilter newMonitorFilter = new MonitorFilter(monitorFilter.getDescription());
        boolean anythingAdded = false;

        for (Iterator i = monitorFilter.getServiceMonitorFilters(); i.hasNext();) {
            ServiceMonitorFilter serviceMonitorFilter = (ServiceMonitorFilter) i.next();

            ModuleClassID moduleClassID = serviceMonitorFilter.getModuleClassID();
            ServiceMonitor serviceMonitor = getServiceMonitor(moduleClassID);

            if (serviceMonitor == null) {
                continue;
            }

            ServiceMonitorFilter newServiceMonitorFilter = serviceMonitor.createSupportedServiceMonitorFilter(serviceMonitorFilter
                    ,
                    reportRate);

            if (newServiceMonitorFilter != null) {
                newMonitorFilter.addServiceMonitorFilter(newServiceMonitorFilter);
                anythingAdded = true;
            }
        }

        if (anythingAdded) {
            return newMonitorFilter;
        } else {
            return null;
        }
    }

    public synchronized long addMonitorListener(MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener) throws MonitorException {

        validateMonitorFilter(monitorFilter, reportRate); // if validation fails, it will throw an exception

        if (includeCumulative) {
            validateCumulativeMonitorFilter(monitorFilter);
        }    // if validation fails, it will throw an exception

        int reportRateIndex = getReportRateIndex(reportRate);

        try {
            monitorFilter = (MonitorFilter) DocumentSerializableUtilities.copyDocumentSerializable(monitorFilter); // make a copy of the filter
        } catch (DocumentSerializationException e) {
            throw new MonitorException(MonitorException.SERIALIZATION, "Error trying to copy MonitorFilter");
        }

        MonitorListenerInfo monitorListenerInfo = new MonitorListenerInfo(monitorListener, reportRate, monitorFilter
                ,
                includeCumulative);

        monitorListenerInfos.add(monitorListenerInfo);
        filtersPerRate[reportRateIndex]++;

        if ((filtersPerRate[reportRateIndex] == 1) && (pulseRateIndex != NOT_PULSING) && (reportRateIndex > pulseRateIndex)) {
            previousReportTimes[reportRateIndex] = previousReportTimes[pulseRateIndex];
        }

        for (Iterator i = monitorFilter.getModuleClassIDs(); i.hasNext();) {
            ModuleClassID moduleClassID = (ModuleClassID) i.next();

            ServiceMonitorFilter serviceMonitorFilter = monitorFilter.getServiceMonitorFilter(moduleClassID);
            ServiceMonitorPulseInfo serviceMonitorPulseInfo = serviceMonitorPulseInfos.get(moduleClassID);

            serviceMonitorPulseInfo.registerServiceMonitorFilter(serviceMonitorFilter, reportRateIndex, reportRate);
        }

        resetPulseRate();

        return reportRate;
    }

    private MonitorListenerInfo getMonitorListenerInfo(MonitorListener monitorListener) {
        for (Object monitorListenerInfo1 : monitorListenerInfos) {
            MonitorListenerInfo monitorListenerInfo = (MonitorListenerInfo) monitorListenerInfo1;

            if (monitorListenerInfo.monitorListener == monitorListener) {
                return monitorListenerInfo;
            }
        }
        return null;
    }

    public synchronized int removeMonitorListener(MonitorListener monitorListener) {
        int numRemoved = 0;

        for (;;) { // remove all instances of this listener
            MonitorListenerInfo monitorListenerInfo = getMonitorListenerInfo(monitorListener);

            if (monitorListenerInfo == null) {
                break;
            } else {
                MonitorFilter monitorFilter = monitorListenerInfo.monitorFilter;
                long reportRate = monitorListenerInfo.reportRate;
                int reportRateIndex = monitorListenerInfo.reportRateIndex;

                monitorListenerInfos.remove(monitorListenerInfo);
                numRemoved++;
                filtersPerRate[reportRateIndex]--;

                for (Iterator i = monitorFilter.getModuleClassIDs(); i.hasNext();) {
                    ModuleClassID moduleClassID = (ModuleClassID) i.next();

                    ServiceMonitorFilter serviceMonitorFilter = monitorFilter.getServiceMonitorFilter(moduleClassID);
                    ServiceMonitorPulseInfo serviceMonitorPulseInfo = serviceMonitorPulseInfos.get(moduleClassID);

                    serviceMonitorPulseInfo.deregisterServiceMonitorFilter(serviceMonitorFilter, reportRateIndex, reportRate);
                }
            }
        }

        resetPulseRate();

        return numRemoved;
    }

    public synchronized MonitorReport getCumulativeMonitorReport(MonitorFilter monitorFilter) throws MonitorException {
        validateCumulativeMonitorFilter(monitorFilter);

        long beginReportTime = System.currentTimeMillis();

        MonitorReport monitorReport = new MonitorReport(startTime, beginReportTime, true);

        for (Iterator i = monitorFilter.getModuleClassIDs(); i.hasNext();) {
            ModuleClassID moduleClassID = (ModuleClassID) i.next();

            ServiceMonitorFilter serviceMonitorFilter = monitorFilter.getServiceMonitorFilter(moduleClassID);
            ServiceMonitor serviceMonitor = getServiceMonitor(moduleClassID);

            ServiceMetric serviceMetric = serviceMonitor.getCumulativeServiceMetric(serviceMonitorFilter, timeZero
                    ,
                    beginReportTime);

            monitorReport.addServiceMetric(moduleClassID, serviceMetric);
        }

        return monitorReport;
    }

    public ModuleClassID[] getMonitorableServiceTypes() {
        if (supportedModuleClassIDs == null) {
            ModuleClassID[] registeredModuleClassIDs = MonitorResources.getRegisteredModuleClassIDs();
            LinkedList<ModuleClassID> supportedModuleClassIDsList = new LinkedList<ModuleClassID>();

            for (ModuleClassID registeredModuleClassID : registeredModuleClassIDs) {
                if (isLocalMonitoringAvailable(registeredModuleClassID)) {
                    supportedModuleClassIDsList.add(registeredModuleClassID);
                }
            }

            supportedModuleClassIDs = supportedModuleClassIDsList.toArray(new ModuleClassID[0]);
        }
        return supportedModuleClassIDs;
    }

    // fastest pulse rate registered anywhere
    public long getPulseRate() {
        return getReportRate(pulseRateIndex);
    }

    // index of fastest pulse anywhere
    public int getPulseRateIndex() {
        return pulseRateIndex;
    }

    // pulse rate for this monitor
    public long getPulseRate(ServiceMonitor serviceMonitor) {
        ServiceMonitorPulseInfo serviceMonitorPulseInfo = serviceMonitorPulseInfos.get(serviceMonitor.getModuleClassID());

        if (serviceMonitorPulseInfo != null) {
            return serviceMonitorPulseInfo.getPulseRate();
        } else {
            return ServiceMonitorPulseInfo.NOT_PULSING;
        }
    }

    // index of pulse rate for this monitor
    public long getPulseRateIndex(ServiceMonitor serviceMonitor) {
        ServiceMonitorPulseInfo serviceMonitorPulseInfo = serviceMonitorPulseInfos.get(serviceMonitor.getModuleClassID());

        if (serviceMonitorPulseInfo != null) {
            return serviceMonitorPulseInfo.getPulseRateIndex();
        } else {
            return ServiceMonitorPulseInfo.NOT_PULSING;
        }
    }

    private void generateReports() {
        long beginReportTime = System.currentTimeMillis();

        for (Enumeration<ServiceMonitorPulseInfo> e = serviceMonitorPulseInfos.elements(); e.hasMoreElements();) {
            ServiceMonitorPulseInfo serviceMonitorPulseInfo = e.nextElement();
            int servicePulseRateIndex = serviceMonitorPulseInfo.getPulseRateIndex();

            if ((serviceMonitorPulseInfo.serviceMonitor instanceof ServiceMonitorImpl)
                    && isEvenPulseForRateIndex(servicePulseRateIndex)) {
                ((ServiceMonitorImpl) serviceMonitorPulseInfo.serviceMonitor).beginPulse(serviceMonitorPulseInfo);
            }
        }

        for (Object monitorListenerInfo1 : monitorListenerInfos) {
            MonitorListenerInfo monitorListenerInfo = (MonitorListenerInfo) monitorListenerInfo1;
            MonitorFilter monitorFilter = monitorListenerInfo.monitorFilter;
            MonitorListener monitorListener = monitorListenerInfo.monitorListener;

            int reportRateIndex = monitorListenerInfo.reportRateIndex;
            long reportRate = monitorListenerInfo.reportRate;

            if (isEvenPulseForRateIndex(reportRateIndex)) {
                MonitorReport monitorReport = null;

                try {
                    if (monitorListenerInfo.sendCumulativeFirst && !monitorListenerInfo.wasCumulativeSent) {
                        monitorReport = getCumulativeMonitorReport(monitorFilter);

                        MonitorEvent monitorEvent = new MonitorEvent(peerGroup.getPeerGroupID(), monitorReport);

                        monitorListener.processMonitorReport(monitorEvent);
                        monitorListenerInfo.wasCumulativeSent = true;
                    } else {
                        monitorReport = getMonitorReport(monitorFilter, reportRate, previousReportTimes[reportRateIndex]
                                ,
                                beginReportTime);
                        MonitorEvent monitorEvent = new MonitorEvent(peerGroup.getPeerGroupID(), monitorReport);

                        monitorListener.processMonitorReport(monitorEvent);
                    }
                } catch (Throwable e) {
                    e.printStackTrace();
                    // Fix-Me: Where should we report an uncaught exception in one of our listeners?
                }
            }
        }

        for (int rateIndex = 0; rateIndex < supportedReportRates.length; rateIndex++) {
            if (isEvenPulseForRateIndex(rateIndex)) {
                if (filtersPerRate[rateIndex] != 0) {
                    previousReportTimes[rateIndex] = beginReportTime;
                } else {
                    previousReportTimes[rateIndex] = NO_PRIOR_REPORT;
                }
            }
        }

        for (Enumeration<ServiceMonitorPulseInfo> e = serviceMonitorPulseInfos.elements(); e.hasMoreElements();) {
            ServiceMonitorPulseInfo serviceMonitorPulseInfo = e.nextElement();
            int servicePulseRateIndex = serviceMonitorPulseInfo.getPulseRateIndex();

            if ((serviceMonitorPulseInfo.serviceMonitor instanceof ServiceMonitorImpl)
                    && isEvenPulseForRateIndex(servicePulseRateIndex)) {
                ((ServiceMonitorImpl) serviceMonitorPulseInfo.serviceMonitor).endPulse(serviceMonitorPulseInfo);
            }
        }
    }

    boolean isEvenPulseForRateIndex(int pulseRateIndex) {
        if (pulseRateIndex < 0 || pulseRateIndex > pulsesPerRate.length) {
            return false;
        }
        return ((pulseNumber % pulsesPerRate[pulseRateIndex]) == 0);
    }

    private void createReportThread() {
        reportThread = new Thread(new Runnable() {
            public void run() {
                mainLoop:
                while (isRunning) {
                    synchronized (MonitorManager.this) { // no new listeners while reporting
                        while (pulseRate == NOT_PULSING) {
                            try {
                                MonitorManager.this.wait();
                            } catch (InterruptedException e) {
                                continue mainLoop;
                            }
                        }

                        while (pulseRate != NOT_PULSING) {
                            if (Thread.interrupted()) {
                                continue mainLoop;
                            }

                            long now = System.currentTimeMillis();

                            try {
                                long waitTime = nextPulseTime - now;

                                if (waitTime > 0) {
                                    MonitorManager.this.wait(nextPulseTime - now);
                                }

                                pulseNumber += pulsesPerRate[pulseRateIndex];
                                generateReports();
                                nextPulseTime += pulseRate;
                            } catch (InterruptedException e) {
                                if (pulseRateIndex == NOT_PULSING) {
                                    continue mainLoop;
                                }
                            } catch (Exception ex) {
                                // don't die forever on exceptions!!
                                ex.printStackTrace(); // fix-me: report this
                            }
                        }
                    }
                }
            }
        }, "Meter-Monitor-Report");

        reportThread.setDaemon(true);
        reportThread.start();
    }

    public synchronized void destroy() {
        isRunning = false;
        reportThread.interrupt();

        for (Enumeration<ServiceMonitorPulseInfo> e = serviceMonitorPulseInfos.elements(); e.hasMoreElements();) {
            ServiceMonitorPulseInfo serviceMonitorPulseInfo = e.nextElement();
            ServiceMonitor serviceMonitor = serviceMonitorPulseInfo.serviceMonitor;

            serviceMonitor.destroy();
        }
    }

    /**
     * DO NOT USE THIS FIELD: It will be deprecated when MonitorManager becomes a
     * FULL FLEDGED SERVICE
     */

    private static Hashtable<PeerGroupID, MonitorManager> monitorManagers = new Hashtable<PeerGroupID, MonitorManager>();

    /**
     * DO NOT USE THIS METHOD: It will be deprecated when MonitorManager becomes a
     * FULL FLEDGED SERVICE
     */

    public static MonitorManager registerMonitorManager(PeerGroup peerGroup) throws JxtaException {
        PeerGroupID peerGroupID = peerGroup.getPeerGroupID();
        MonitorManager monitorManager = monitorManagers.get(peerGroupID);

        if (monitorManager == null) {
            boolean includeTransports = true;
            ModuleImplAdvertisement moduleImplAdvertisement = MonitorResources.getReferenceAllPurposeMonitorServiceImplAdvertisement(
                    includeTransports);

            monitorManager = (MonitorManager) peerGroup.loadModule(MonitorResources.refMonitorServiceSpecID
                    ,
                    moduleImplAdvertisement);
            monitorManagers.put(peerGroupID, monitorManager);

            // FIXME jice@jxta.org - 20021103 : this
            // monitorManager is not a real group service:
            // it is being loadModule()'d by another as a
            // result, it holds a counted reference to the
            // group.  Idealy, we'd need the groupAPI to
            // offer a means to loadModule() without
            // making a counted reference, so that group
            // services can loadModule() things without
            // preventing group termination. This could be
            // achieved elegantly by making this only
            // behaviour available through a weak
            // GroupInterface. So it would be enough to
            // obtain a weak interface from one's group
            // and then use its loadmodule method rather
            // than that of the strong group interface.
            // However that's a bit too big a change to be
            // decided without more carefull
            // consideration.  Instead, we just simulate
            // it for now: we give to the monitor manager
            // the real group reference after loadModule
            // is done, and it discards the strong
            // interface object that was passed to its
            // init routine.

            monitorManager.setPeerGroup(peerGroup);
        }
        return monitorManager;
    }

    /**
     * DO NOT USE THIS METHOD: It will be deprecated when MonitorManager becomes a
     * FULL FLEDGED SERVICE
     */

    public static void unregisterMonitorManager(PeerGroup peerGroup) {
        PeerGroupID peerGroupID = peerGroup.getPeerGroupID();

        monitorManagers.remove(peerGroupID);
    }

    public static ServiceMonitor getServiceMonitor(PeerGroup peerGroup, ModuleClassID serviceClassID) {
        try {
            PeerGroupID peerGroupID = peerGroup.getPeerGroupID();
            MonitorManager monitorManager = monitorManagers.get(peerGroupID);

            return monitorManager.getServiceMonitor(serviceClassID);
        } catch (Exception e) { // Fix-Me: This is a bit sloppy
            throw new RuntimeException("Unable to find MonitorManager or MonitorService");
        }
    }
}
