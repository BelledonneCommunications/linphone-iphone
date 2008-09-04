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

package net.jxta.impl.peer;


import net.jxta.document.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.meter.*;

import java.util.*;


public class RemoteMonitorQuery implements DocumentSerializable {

    public static final String CUMULATIVE_REPORT_REQUEST = "cumulativeReport";
    public static final String REGISTER_MONITOR_REQUEST = "registerMonitor";
    public static final String REMOVE_MONITOR_REQUEST = "removeMonitor";
    public static final String VALIDATE_FILTER_REQUEST = "validateFilter";
    public static final String VALIDATE_CUMULATIVE_FILTER_REQUEST = "validateCumulativeFilter";
    public static final String GET_MONITORING_CAPABILITIES_REQUEST = "remoteMonitoringCapabilities";
    public static final String PEER_MONITOR_INFO = "peerMonitorInfo";
    public static final String RENEW_LEASE = "renewLease";
	
    private String requestType;
    private MonitorFilter monitorFilter;
    private boolean includeCumulative;
    private long reportRate = -1;
    private long lease = -1;
    private int leaseId;

    public RemoteMonitorQuery() {} // for serialization code.

    private RemoteMonitorQuery(String requestType) { 
        this.requestType = requestType;
    }

    public MonitorFilter getMonitorFilter() {
        return monitorFilter;
    }

    private String getRequestType() {
        return requestType;
    }

    public long getReportRate() {
        return reportRate;
    }

    public long getLease() {
        return lease;
    }

    public int getLeaseId() {
        return leaseId;
    }

    public boolean isIncludeCumulative() {
        return includeCumulative;
    }
	
    boolean isCumulativeReportQuery() {
        return requestType.equals(CUMULATIVE_REPORT_REQUEST);
    }

    boolean isRegisterMonitorQuery() {
        return requestType.equals(REGISTER_MONITOR_REQUEST);
    }

    boolean isRemoveMonitorQuery() {
        return requestType.equals(REMOVE_MONITOR_REQUEST);
    }

    boolean isValidateFilterRequest() {
        return requestType.equals(VALIDATE_FILTER_REQUEST);
    }

    boolean isValidateCumulativeFilterRequest() {
        return requestType.equals(VALIDATE_CUMULATIVE_FILTER_REQUEST);
    }

    boolean isPeerMonitorInfoQuery() {
        return requestType.equals(PEER_MONITOR_INFO);
    }

    boolean isLeaseRenewal() {
        return requestType.equals(RENEW_LEASE);
    }

    static RemoteMonitorQuery createGetCumulativeReportQuery(MonitorFilter monitorFilter) {
        RemoteMonitorQuery remoteMonitorQuery = new RemoteMonitorQuery(CUMULATIVE_REPORT_REQUEST);

        remoteMonitorQuery.monitorFilter = monitorFilter;
        return remoteMonitorQuery;
    }
		
    static RemoteMonitorQuery createRegisterMonitorQuery(boolean includeCumulative, MonitorFilter monitorFilter, long reportRate, long lease) {
        RemoteMonitorQuery remoteMonitorQuery = new RemoteMonitorQuery(REGISTER_MONITOR_REQUEST);

        remoteMonitorQuery.monitorFilter = monitorFilter;
        remoteMonitorQuery.reportRate = reportRate;
        remoteMonitorQuery.lease = lease;
        remoteMonitorQuery.includeCumulative = includeCumulative;
        return remoteMonitorQuery;
    }

    static RemoteMonitorQuery createRemoveMonitorListenerQuery(int leaseId) {
        RemoteMonitorQuery remoteMonitorQuery = new RemoteMonitorQuery(REMOVE_MONITOR_REQUEST);

        remoteMonitorQuery.leaseId = leaseId;
        return remoteMonitorQuery;
    }

    static RemoteMonitorQuery createPeerMonitorInfoQuery() {
        RemoteMonitorQuery remoteMonitorQuery = new RemoteMonitorQuery(PEER_MONITOR_INFO);

        return remoteMonitorQuery;		
    }

    static RemoteMonitorQuery createLeaseRenewalQuery(int leaseId, long requestedLease) {
        RemoteMonitorQuery remoteMonitorQuery = new RemoteMonitorQuery(RENEW_LEASE);

        remoteMonitorQuery.leaseId = leaseId;
        remoteMonitorQuery.lease = requestedLease;
        return remoteMonitorQuery;
    }

    public void serializeTo(Element element) throws DocumentSerializationException {
        DocumentSerializableUtilities.addString(element, "requestType", requestType);

        if (monitorFilter != null) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "monitorFilter", monitorFilter);
        }
			
        if (lease > 0) {
            DocumentSerializableUtilities.addLong(element, "lease", lease);
        }

        if (leaseId > -1) {
            DocumentSerializableUtilities.addInt(element, "leaseId", leaseId);
        }			

        if (reportRate > 0) {		
            DocumentSerializableUtilities.addLong(element, "reportRate", reportRate);
        }

        if (includeCumulative) { 
            DocumentSerializableUtilities.addBoolean(element, "includeCumulative", includeCumulative);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
			
            if (tagName.equals("requestType")) { 
                requestType = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("monitorFilter")) { 
                monitorFilter = (MonitorFilter) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        MonitorFilter.class);
            } else if (tagName.equals("lease")) { 
                lease = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("leaseId")) { 
                leaseId = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("reportRate")) { 
                reportRate = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("includeCumulative")) { 
                includeCumulative = DocumentSerializableUtilities.getBoolean(childElement);
            }
        }
    }
}
