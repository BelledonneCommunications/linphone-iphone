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


public class RemoteMonitorResponse implements DocumentSerializable {
    public static final String MONITOR_REGISTERED = "monitorRegistered";
    public static final String MONITOR_REMOVED = "monitorGone";
    public static final String CUMULATIVE_REPORT_RESPONSE = "cumulativeReport";
    public static final String MONITOR_REPORT_RESPONSE = "monitorReport";
    public static final String INVALID_FILTER = "invalidFilter";
    public static final String INVALID_REPORT_RATE = "invalidReportRate";
    public static final String VALID_FILTER = "validFilter";
    public static final String SERVICE_NOT_SUPPORTED = "serviceNotSupported";
    public static final String METERING_NOT_SUPPORTED = "metereringNotSupported";
    public static final String METERING_SUPPORTED = "meteringSupported";
    public static final String METERING_REQUEST_DENIED = "meteringRequestDenied";
    public static final String PEER_MONITOR_INFO = "peerMonitorInfo";
    public static final String LEASE_ENDED = "leaseOver";
    public static final String LEASE_RENEWED = "leaseRenewed";

    /*
     ResponseCode: Not Supported
     ResponseCode: Invalid Reporting Rate
     Suggested Reporting Rate
     ResponseCode: Invalid Filter
     Suggested Filter
     */

    private String responseType;
    private int requestId;
    private int leaseId;
    private MonitorReport monitorReport;
    private boolean isCumulative;
    private MonitorFilter monitorFilter;
    private long lease = -1;
    private PeerMonitorInfo peerMonitorInfo;
	
    private long reportRates[];
    private LinkedList supportedModuleClassIDs;

    public RemoteMonitorResponse() {} // for serialization.
	
    private RemoteMonitorResponse(String responseType, int requestId) { 
        this.responseType = responseType;
        this.requestId = requestId;
    }

    public long getLease() {
        return lease;
    }

    public boolean isCumulative() {
        return isCumulative;
    }

    public String getResponseType() {
        return responseType;
    }

    public int getRequestId() {
        return requestId;
    }

    public int getLeaseId() {
        return leaseId;
    }

    public MonitorReport getMonitorReport() {
        return monitorReport;
    }

    public MonitorFilter getMonitorFilter() {
        return monitorFilter;
    }

    public PeerMonitorInfo getPeerMonitorInfo() {
        return peerMonitorInfo;
    }
	
    public boolean isMonitorRegistered() {
        return responseType.equals(MONITOR_REGISTERED);
    }

    public boolean isMonitorRemoved() {
        return responseType.equals(MONITOR_REMOVED);
    }

    public boolean isCumulativeReport() {
        return responseType.equals(CUMULATIVE_REPORT_RESPONSE);
    }

    public boolean isMonitorReport() {
        return responseType.equals(MONITOR_REPORT_RESPONSE);
    }

    public boolean isValidFilter() {
        return responseType.equals(VALID_FILTER);
    }

    public boolean isInvalidFilter() {
        return responseType.equals(INVALID_FILTER);
    }

    public boolean isInvalidReportRate() {
        return responseType.equals(INVALID_REPORT_RATE);
    }

    public boolean isServiceNotSupported() {
        return responseType.equals(SERVICE_NOT_SUPPORTED);
    }

    public boolean isMeteringNotSupported() {
        return responseType.equals(METERING_NOT_SUPPORTED);
    }

    public boolean isMeteringSupported() {
        return responseType.equals(METERING_SUPPORTED);
    }

    public boolean isRequestDenied() {
        return responseType.equals(METERING_REQUEST_DENIED);
    }

    public boolean isPeerMonitorInfo() {
        return responseType.equals(PEER_MONITOR_INFO);
    }

    public boolean isLeaseRenewed() {
        return responseType.equals(LEASE_RENEWED);
    }

    public static RemoteMonitorResponse createMonitorRegisteredResponse(int requestId, int leaseId, long lease) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(MONITOR_REGISTERED, requestId);

        remoteMonitorResponse.leaseId = leaseId;
        remoteMonitorResponse.lease = lease;
        return remoteMonitorResponse;
    }

    public static RemoteMonitorResponse createMonitorRemovedResponse(int requestId) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(MONITOR_REMOVED, requestId);

        return remoteMonitorResponse;
    }		

    public static RemoteMonitorResponse createLeaseEndedResponse(int requestId, int leaseId) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(LEASE_ENDED, requestId);

        remoteMonitorResponse.leaseId = leaseId;
        return remoteMonitorResponse;
    }
	
    public static RemoteMonitorResponse createCumulativeReportResponse(int requestId, MonitorReport monitorReport) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(CUMULATIVE_REPORT_RESPONSE, requestId);

        remoteMonitorResponse.monitorReport = monitorReport;
        remoteMonitorResponse.isCumulative = true;
        return remoteMonitorResponse;
    }

    public static RemoteMonitorResponse createMonitorReportResponse(int requestId, MonitorReport monitorReport) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(MONITOR_REPORT_RESPONSE, requestId);

        remoteMonitorResponse.monitorReport = monitorReport;
        remoteMonitorResponse.isCumulative = false;
        return remoteMonitorResponse;
    }

    public static RemoteMonitorResponse createInvalidReportRateResponse(int requestId) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(INVALID_REPORT_RATE, requestId);

        return remoteMonitorResponse;		
    }

    public static RemoteMonitorResponse createServiceNotSupportedResponse(int requestId) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(SERVICE_NOT_SUPPORTED, requestId);

        return remoteMonitorResponse;		
    }

    public static RemoteMonitorResponse createInvalidFilterResponse(int requestId) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(INVALID_FILTER, requestId);

        return remoteMonitorResponse;		
    }

    /*
     public static RemoteMonitorResponse createValidFilterResponse(int requestId) {
     RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(VALID_FILTER, requestId);
     return remoteMonitorResponse;		
     }
     */
    public static RemoteMonitorResponse createDeniedResponse(int requestId) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(METERING_REQUEST_DENIED, requestId);

        return remoteMonitorResponse;		
    }
	
    public static RemoteMonitorResponse createPeerMonitorInfoResponse(int requestId, PeerMonitorInfo peerMonitorInfo) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(PEER_MONITOR_INFO, requestId);

        remoteMonitorResponse.peerMonitorInfo = peerMonitorInfo;
        return remoteMonitorResponse;		
    }

    public static RemoteMonitorResponse createLeaseRenewedResponse(int requestId, int leaseId, long lease) {
        RemoteMonitorResponse remoteMonitorResponse = new RemoteMonitorResponse(LEASE_RENEWED, requestId);

        remoteMonitorResponse.leaseId = leaseId;
        remoteMonitorResponse.lease = lease;
        return remoteMonitorResponse;	
    }		

    public void serializeTo(Element element) throws DocumentSerializationException {
        DocumentSerializableUtilities.addString(element, "responseType", responseType);
        DocumentSerializableUtilities.addInt(element, "requestId", requestId);
		
        if (monitorReport != null) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "monitorReport", monitorReport);
        }
			
        if (monitorFilter != null) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "monitorFilter", monitorFilter);
        }

        if (lease > 0) {
            DocumentSerializableUtilities.addLong(element, "lease", lease);
        }

        if (leaseId > -1) {
            DocumentSerializableUtilities.addInt(element, "leaseId", leaseId);
        }		

        if (isCumulative) { 
            DocumentSerializableUtilities.addBoolean(element, "isCumulative", isCumulative);
        }

        if (peerMonitorInfo != null) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "peerMonitorInfo", peerMonitorInfo);
        }
    }
	
    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
			
            if (tagName.equals("responseType")) { 
                responseType = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("requestId")) { 
                requestId = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("monitorReport")) {
                monitorReport = (MonitorReport) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        MonitorReport.class);
            } else if (tagName.equals("monitorFilter")) {
                monitorFilter = (MonitorFilter) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        MonitorFilter.class);
            } else if (tagName.equals("lease")) { 
                lease = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("leaseId")) { 
                leaseId = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("isCumulative")) { 
                isCumulative = DocumentSerializableUtilities.getBoolean(childElement);
            } else if (tagName.equals("peerMonitorInfo")) {
                peerMonitorInfo = (PeerMonitorInfo) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        PeerMonitorInfo.class);
            }
        }
    }
}
