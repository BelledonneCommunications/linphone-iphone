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


import net.jxta.peergroup.*;
import net.jxta.peer.*;


/**
 *	Information regarding a Remote Monitor's Report
 **/
public class MonitorEvent {

    /** Report received and is available in the event **/
    public static final int OK = 200;

    /** Report will not come because the registration was cancelled locally **/
    public static final int CANCELLED_LOCALLY = 201;

    /** Report will not come because the registration was cancelled remotely **/
    public static final int CANCELLED_REMOTELY = 202;

    /** Report will not come because the of a Timeout **/
    public static final int TIMEOUT = 203;

    /** Report will not come because the Remote peer has refused it (for security, not supported or load reasons) **/
    public static final int REFUSED = 204;

    /** Report will not come because the requested report rate is not supported/invalid **/
    public static final int INVALID_REPORT_RATE = 205;

    /** Report will not come because the provided MonitorFilter was invalid **/
    public static final int INVALID_MONITOR_FILTER = 206;

    /** Further Reports will not come because the lease was cancelled **/
    public static final int LEASE_CANCELLED = 207;

    /** Further Reports will not come because the lease renewal request did not receive a response.  
     * In all likelihood the remote peer has failed (ie crashed)
     **/
    public static final int LEASE_RENEWAL_TIMEOUT = 208;

    /** Internal Error processing Reports, probably due to a bad Monitor Implementation **/
    public static final int ERROR = 209;

    /** Further Reports will not come because this PeerGroup was destroyed locally **/
    public static final int PEERGROUP_DESTROYED = 210;

    private int type;
    private int requestId;
    private PeerID peerID;
    private PeerGroupID peerGroupID;
    private MonitorFilter monitorFilter; // FIX-ME: Is this necessary?
    private long reportRate; // FIX-ME: Is this necessary?
    private long leaseTime; // FIX-ME: Is this necessary?
    private MonitorReport monitorReport;

    private MonitorEvent() {}

    public MonitorEvent(PeerGroupID peerGroupID, MonitorReport monitorReport) {
        this.peerGroupID = peerGroupID;
        this.monitorReport = monitorReport;
    }

    /**
     * Get the Type of Event (one of the above constants)
     *
     */
    public int getType() {
        return type;
    }

    /**
     * PeerID of reporting Peer.  My PeerID if local
     */
    public PeerID getPeerID() {
        return peerID;
    }

    /**
     * PeerGroup of reported event 
     */
    public PeerGroupID getPeerGroupID() {
        return peerGroupID;
    }

    /**
     * MonitorFilter provided when the report was requested
     */
    public MonitorFilter getMonitorFilter() {
        return monitorFilter;
    }

    /**
     * Reporting rate (unless Cumulative) specified when the report was requested
     */
    public long getReportRate() {
        return reportRate;
    }

    /**
     * Most recent Lease time granted (not specified if a cumulative Report)
     */
    public long getLeaseTime() {
        return leaseTime;
    }

    /**
     * Get the corresponding MonitorReport
     */
    public MonitorReport getMonitorReport() {
        return monitorReport;
    }

    /**
     * The Validated MonitorFilter from the registration or query
     *
     * @return MonitorFilter
     */
    public MonitorFilter getValidMonitorFilter() {
        return monitorFilter;
    }

    /**
     * Convenience factory method
     */
    public static MonitorEvent createRemoteMonitorReportEvent(PeerID peerID, int requestId, MonitorReport monitorReport) {
        MonitorEvent event = new MonitorEvent();

        event.type = OK;
        event.peerID = peerID;
        event.requestId = requestId;
        event.monitorReport = monitorReport;
        return event;
    }

    /**
     * Convenience factory method
     */
    public static MonitorEvent createFailureEvent(int type, PeerID peerID, int requestId) {
        MonitorEvent event = new MonitorEvent();

        event.type = type;
        event.peerID = peerID;
        event.requestId = requestId;
        return event;
    }
}
