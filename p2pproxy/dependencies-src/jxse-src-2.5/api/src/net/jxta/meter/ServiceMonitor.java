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

package net.jxta.meter;


import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.*;


/**
 *  The base interface for all ServiceMonitors
 *  For complete information about Service Monitors see the 
 *  Document <I> Designing and Implementing Service Monitors </I>
 */
public interface ServiceMonitor extends Module {

    /**
     * Get ModuleClassID of this ServiceMonitor
     */
    public ModuleClassID getModuleClassID();

    /**
     * Get ServiceMetrics accrued in during this pulse interval
     *
     * @param serviceMonitorFilter  Filter Metrics based upon this MonitorFilter
     * @param fromTime	Beginning time as determined by the MonitorManager
     * @param toTime	Ending time as determined by the MonitorManager
     * @param pulseIndex Pulse Index of the reporting rate Pyramid
     * @param reportRate Reporting Rate (corresponding to the PulseNumber's index)
     */
    public ServiceMetric getServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime, int pulseIndex, long reportRate);

    /**
     * A request for a cumulative Report(s) are coming, prepare to receive them
     * @see #endCumulativeReport()
     *
     */
    public void beginCumulativeReport();

    /**
     * Get ServiceMetrics since the start (or last reset time) of this ServiceMonitor.
     * Calls to this will only occurr between calls to beginCumulativeReport() and endCumulativeReport()
     * @see #beginCumulativeReport()
     * @see #endCumulativeReport()
     * @param serviceMonitorFilter  Filter Metrics based upon this MonitorFilter
     * @param fromTime	Beginning time as determined by the MonitorManager
     * @param toTime	Ending time as determined by the MonitorManager
     */
    public ServiceMetric getCumulativeServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime);

    /**
     * Indication that the flurry of requests for cumulative Report is over
     * @see #beginCumulativeReport()
     */
    public void endCumulativeReport();

    /**
     * Validate ServiceMonitorFilter for a cumulative Report
     */
    public void validateCumulativeServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter) throws MonitorFilterException;

    /**
     * Validate ServiceMonitorFilter for a periodic Reporting
     */
    public void validateServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter, long reportRate) throws MonitorFilterException;

    /**
     * Transform the provided ServiceMonitorFilter into one that is supported for cumulative reporting
     */
    public ServiceMonitorFilter createSupportedCumulativeServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter) throws MonitorFilterException;

    /**
     * Transform the provided ServiceMonitorFilter into one that is supported for periodic reporting at the specified rate
     */
    public ServiceMonitorFilter createSupportedServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter, long reportRate) throws MonitorFilterException;

    /**
     * Information that the Monitor Manager has accepted a Monitoring for this filter at this rate
     *
     * @param serviceMonitorFilter	Accepted Filter
     * @param reportRateIndex		Pulse Index into pyramid of accepted rate
     * @param reportRate			Accepted reporting rate
     * @param newRate				Is this a new reporting rate (ie you don't have any currently registered at this rate)
     */
    public void serviceMonitorFilterRegistered(ServiceMonitorFilter serviceMonitorFilter, int reportRateIndex, long reportRate, boolean newRate);

    /**
     * Information that the Monitor Manager is deregistering the Monitoring for this filter at this rate
     *
     * @param serviceMonitorFilter	Deregistered Filter
     * @param reportRateIndex		Pulse Index into pyramid of deregistered filter
     * @param reportRate			Reporting rate of deregistered filter
     * @param retiredRate			Is this a retired filter the last one registered at this rate (ie you don't have to keep metrics for this rate any longer)
     */
    public void serviceMonitorFilterDeregistered(ServiceMonitorFilter serviceMonitorFilter, int reportRateIndex, long reportRate, boolean retiredRate);

    /**
     *    Clean up.  The PeerGroup is probably about to be destroyed
     */
    public void destroy();
}
