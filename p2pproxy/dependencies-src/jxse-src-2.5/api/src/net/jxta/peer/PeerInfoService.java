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

package net.jxta.peer;


import net.jxta.meter.*;
import net.jxta.platform.ModuleClassID;
import net.jxta.service.Service;


/**
 * The PeerInfoService is a generic API for getting information about
 * the local Peer as well as remote Peers. <P>
 * <p/>
 * The most important type of information about a Peer may be gotten through
 * the Monitoring Service that may be accessed via the PeerInfoService.  The
 * Monitoring Service provides an open mechanism for reporting any type of
 * Metrics gathered on a Peer by a ServiceMonitor.  Attached Service Monitors
 * are identified by their ModuleClassID.  A ServiceMonitor may monitor anything
 * (ie it is not restricted to JXTA Services).  <p>
 * <p/>
 * There are several methods for accessing the capabilities and metrics
 * from ServiceMonitors
 * attached to the Peer (either locally or from remote peers). <p>
 * <p/>
 * Cumulative MonitorReports containing metrics since the Monitoring began
 * (or was reset) on a local/remote Peer may be obtained.  Alternatively, you may
 * register listeners get periodic MonitorReports (at a specified rate) of
 * metrics (since the previous report).  The amount of information obtained (either
 * cumulatively or periodically) is determined by a MonitorFilter whi
 * <p/>
 * The PeerInfoService utilizes the ResolverService to send queries and receive
 * responses (PeerInfoQueryMessage / PeerInfoResponseMessage).  These contain
 * requests and responses that are specific to the type of info being requested.
 * Depending upon the type of information requested, a peer may provide multiple
 * varying responses over time (as is the case for periodic remote peer Monitoring). <P>
 * <p/>
 * At the time of writing this documentation Service Monitoring is the only type
 * of Peer Information available though the implementation and underlying protocol
 * can support other types of information. <P>
 * <p/>
 * See the document:
 * <UL>
 * <LI> <I> JXTA Metering and Monitoring Project </I> </LI>
 * <LI> <I> The JXTA Metering and Monitoring Project Architecture </I> </LI>
 * <LI> <I> Building and Configuring JXTA with Monitoring Capabilities </I> </LI>
 * <LI> <I> JXTA Monitor: GUI Rendering of Metered Peer Info </I> </LI>
 * </UL>
 *
 * @see net.jxta.meter.MonitorFilter
 * @see net.jxta.meter.MonitorReport
 * @see net.jxta.meter.ServiceMonitor
 * @see net.jxta.meter.PeerMonitorInfo
 * @see net.jxta.protocol.PeerInfoQueryMessage
 * @see net.jxta.protocol.PeerInfoResponseMessage
 * @since JXTA 1.0
 */

public interface PeerInfoService extends Service {

    /**
     * See if Local Monitoring is available on this Peer
     * Local monitoring is only available if you are using a version of
     * of jxta.jar that was build with metering activated.  <p>
     * <p/>
     * See the document:
     * <UL>
     * <LI> <I> Building and Configuring JXTA with Monitoring Capabilities </I> </LI>
     * </UL>
     *
     * @return true if local monitoring is available
     */
    public boolean isLocalMonitoringAvailable();

    /**
     * See if Local monitoring is available from a specific ServiceMonitor.
     * Local monitoring is only available if you are using a version of
     * of jxta.jar that was build with metering activated.  <p>
     * <p/>
     * See the document:
     * <UL>
     * <LI> <I> Building and Configuring JXTA with Monitoring Capabilities </I> </LI>
     * </UL>
     *
     * @param moduleClassID The Module classID of the ServiceMonitor.  Note that the ServiceMonitor
     *                      moduleClassID is not the same as moduleClassID of the Service
     *                      being monitored.
     * @return true if local monitoring is available for a specific module
     */
    public boolean isLocalMonitoringAvailable(ModuleClassID moduleClassID);

    /**
     * Asynchronous reporting of Monitored data may be obtained only at rates supported by
     * the MonitorManager on the peer.  This method returns the locally supported rates (in milliseconds)
     * @return all supported rates
     */
    public long[] getSupportedReportRates();

    /**
     * Asynchronous reporting of Monitored data may be obtained only at rates supported by
     * the MonitorManager on the peer.  This method validates whether a specific
     * rate (in milliseconds) is locally supported.
     * @param reportRate the report rate to check
     * @return true if a report rate is supported
     */
    public boolean isSupportedReportRate(long reportRate);

    /**
     * Asynchronous reporting of Monitored data may be obtained only at rates supported by
     * the MonitorManager on the peer.  This method supplies the closest (rounded up)
     * rate (in milliseconds) to the specified rate that is locally supported.
     * @param desiredReportRate the desired rate
     * @return the desired rate
     */
    public long getBestReportRate(long desiredReportRate);

    /**
     * Obtain the monitoring capabilities of the Local Peer. <P>
     * The PeerMonitorInfo provides:
     * <UL>
     * <LI> Whether any monitoring is available for this Peer </LI>
     * <LI> The supported rates of asynchronous monitoring </LI>
     * <LI> A list (as ModuleClassIDs) of ServiceMonitors attached to this Peer </LI>
     * </UL>
     * @return PeerMonitorInfo
     */
    public PeerMonitorInfo getPeerMonitorInfo();

    /**
     * Obtain the monitoring capabilities of a Remote Peer. <P>
     * The PeerMonitorInfo provides:
     * <UL>
     * <LI> Whether any monitoring is available for this Peer </LI>
     * <LI> The supported rates of asynchronous monitoring </LI>
     * <LI> A list (as ModuleClassIDs) of ServiceMonitors attached to this Peer </LI>
     * </UL>
     * <p/>
     * Via the PeerMonitorInfoListener, you will be informed of the PeerMonitorInfo or why it was
     * not provided (error, timeout, unavailable, etc)
     *
     * @param peerID                  The PeerID of the Peer you wish information about
     * @param peerMonitorInfoListener The Listener to be told about the obtained PeerMonitorInfo
     * @param timeout                 Generate a timeout event if no answer has been received in this time (in Milliseconds)
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public void getPeerMonitorInfo(PeerID peerID, PeerMonitorInfoListener peerMonitorInfoListener, long timeout) throws MonitorException;

    /**
     * Get a MonitorReport of total accumulated metrics from the ServiceMonitors (specified in the
     * MonitorFilter) since they were created/reset for the local Peer.
     *
     * @param monitorFilter The MonitorFilter containing the specific ServiceMonitors and types of Service Metrics desired
     * @return  the report
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public MonitorReport getCumulativeMonitorReport(MonitorFilter monitorFilter) throws MonitorException;

    /**
     * Get a MonitorReport of total accumulated metrics from the ServiceMonitors (specified in the
     * MonitorFilter) since they were created/reset for the specified remote Peer.
     *
     * @param peerID          The PeerID of the Peer you wish information about
     * @param monitorFilter   The MonitorFilter containing the specific ServiceMonitors and types of Service Metrics desired
     * @param monitorListener The Listener to obtain the report when it arrives (or timed out)
     * @param timeout         The timeout for reporting that the information has not arrived.
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public void getCumulativeMonitorReport(PeerID peerID, MonitorFilter monitorFilter, MonitorListener monitorListener, long timeout) throws MonitorException;

    /**
     * Get MonitorReports at a specified rates of metrics accrued over time from the ServiceMonitors
     * (specified in the MonitorFilter) about the local Peer.  For many applications it is required to obtain metrics from
     * the beginning of time and then augment over time as more data arrives. <P>
     * <p/>
     * There is a problem with the following approach:
     * <OL>
     * <LI>	Call getCumulativeMonitorReport to get the cumulative totals </LI>
     * <LI>	Call addMonitorListener to get periodic changes </LI>
     * <LI>	Add the periodic data to the totals </LI>
     * </OL>
     * <p/>
     * Because of a potential race condition related to metrics that are measured between the two calls it is
     * possible to lose some metrics.  To address this, this method supports this by combining them into a single
     * call that allows you to specify whether you wish the first report delivered to be a cumulative report.
     *
     * @param monitorFilter     The MonitorFilter containing the specific ServiceMonitors and types of Service Metrics desired
     * @param reportRate        The rate at which you wish metric delta reports
     * @param includeCumulative Should the first report you receive be the cumulative data since the ServiceMonitors were created/reset?
     * @param monitorListener   The Listener to obtain the report when it arrives (or timed out)
     * @return report rate
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public long addMonitorListener(MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener) throws MonitorException;

    /**
     * Get MonitorReports at a specified rates of metrics accrued over time from the ServiceMonitors
     * (specified in the MonitorFilter) about the specified remote Peer.  For many applications it is required to obtain metrics from
     * the beginning of time and then augment over time as more data arrives. <P>
     * <p/>
     * There is a problem with the following approach:
     * <OL>
     * <LI>	Call getCumulativeMonitorReport to get the cumulative totals </LI>
     * <LI>	Call addMonitorListener to get periodic changes </LI>
     * <LI>	Add the periodic data to the totals </LI>
     * </OL>
     * <p/>
     * Because of a potential race condition related to metrics that are measured between the two calls it is
     * possible to lose some metrics.  To address this, this method supports this by combining them into a single
     * call that allows you to specify whether you wish the first report delivered to be a cumulative report.
     *
     * @param peerID            The PeerID of the Peer you wish information about
     * @param monitorFilter     The MonitorFilter containing the specific ServiceMonitors and types of Service Metrics desired
     * @param reportRate        The rate at which you wish metric delta reports
     * @param includeCumulative Should the first report you receive be the cumulative data since the ServiceMonitors were created/reset?
     * @param monitorListener   The Listener to obtain the report when it arrives (or timed out)
     * @param timeout           The timeout for reporting that the information has not arrived.
     * @param lease the lease
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public void addRemoteMonitorListener(PeerID peerID, MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener, long lease, long timeout) throws MonitorException;

    /**
     * Stop the periodic reporting for all registered filters corresponding to this MonitorListener
     * @param monitorListener the monitor listener
     * @return  true if successfully removed
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public boolean removeMonitorListener(MonitorListener monitorListener) throws MonitorException;

    /**
     * Stop the periodic reporting for all registered filters to the specified Peer corresponding to this MonitorListener.
     *
     * @param peerID          The Peer that you wish to deregister periodic reporting
     * @param monitorListener The MonitorListener that was originally registered
     * @param timeout         The timeout for reporting that the remote listener was acknowledged as deregistered
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public void removeRemoteMonitorListener(PeerID peerID, MonitorListener monitorListener, long timeout) throws MonitorException;

    /**
     * Stop the periodic reporting for all registered filters to the all remote Peers corresponding to this MonitorListener.
     *
     * @param monitorListener The MonitorListener that was originally registered
     * @param timeout         The timeout for reporting that the remote listener was acknowledged as deregistered
     * @throws net.jxta.meter.MonitorException if a monitor error occurs
     */
    public void removeRemoteMonitorListener(MonitorListener monitorListener, long timeout) throws MonitorException;
}

