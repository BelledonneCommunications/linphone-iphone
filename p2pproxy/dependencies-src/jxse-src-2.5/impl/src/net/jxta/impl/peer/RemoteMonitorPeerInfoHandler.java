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

import java.util.Enumeration;
import java.util.Iterator;
import java.util.Hashtable;
import java.util.Random;
import java.util.Timer;
import java.util.TimerTask;
import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.document.Element;
import net.jxta.protocol.PeerInfoQueryMessage;
import net.jxta.protocol.PeerInfoResponseMessage;
import net.jxta.peergroup.PeerGroup;
import net.jxta.exception.PeerGroupException;
import net.jxta.peer.PeerInfoService;
import net.jxta.peergroup.PeerGroupID;

import net.jxta.meter.MonitorEvent;
import net.jxta.meter.MonitorException;
import net.jxta.meter.MonitorFilter;
import net.jxta.meter.MonitorFilterException;
import net.jxta.meter.MonitorListener;
import net.jxta.meter.MonitorReport;
import net.jxta.meter.MonitorResources;
import net.jxta.meter.PeerMonitorInfo;
import net.jxta.meter.PeerMonitorInfoEvent;
import net.jxta.meter.PeerMonitorInfoListener;
import net.jxta.meter.ServiceMonitorFilter;
import net.jxta.peer.PeerID;
import net.jxta.util.documentSerializable.DocumentSerializableUtilities;
import net.jxta.util.documentSerializable.DocumentSerializationException;
import net.jxta.impl.util.TimerThreadNamer;

class RemoteMonitorPeerInfoHandler implements PeerInfoHandler {
    public static final String MONITOR_HANDLER_NAME = "Monitor";
    public static final int MAX_LEASE = 5 * 60 * 1000; // 5 minutes
    public static final int MIN_LEASE = 60 * 1000; // 1 minute

    private static final Random rand = new Random();
    private final static Logger LOG = Logger.getLogger(RemoteMonitorPeerInfoHandler.class.getName());

    private Hashtable<Integer, RequestInfo> requestInfos = new Hashtable<Integer, RequestInfo>();
    private Hashtable<Integer, LeaseInfo> leaseInfos = new Hashtable<Integer, LeaseInfo>();
    private Hashtable<Integer, Long> timeouts = new Hashtable<Integer, Long>();
    private PeerGroup peerGroup;
    private PeerInfoServiceImpl peerInfoServiceImpl;
    private Timer timer = new Timer(true);

    RemoteMonitorPeerInfoHandler(PeerGroup peerGroup, PeerInfoServiceImpl peerInfoServiceImpl) {
        this.peerGroup = peerGroup;
        this.peerInfoServiceImpl = peerInfoServiceImpl;
        timer.schedule(new TimerThreadNamer("RemoteMonitorPeerInfo timer for " + peerGroup.getPeerGroupID()), 0);
    }

    public void stop() {
        timer.cancel();
    }

    private int getNextLeaseId() {
        int id;
        synchronized (rand) {
            id = rand.nextInt(Integer.MAX_VALUE);
        }
        return id;
    }

    private class RequestInfo {
        long requestTime = System.currentTimeMillis();
        PeerID peerId;
        int queryId;
        int origRequestId;
        MonitorListener monitorListener;
        PeerMonitorInfoListener peerMonitorInfoListener;
        long timeout;
        long validUntil;
        boolean responseReceived = false;
        int leaseId; // other guys leaseId.
        long requestedLease;
        PeerInfoMessenger peerInfoMessenger;
        RequestInfo(PeerID peerId, int queryId, MonitorListener monitorListener, long timeout, PeerInfoMessenger peerInfoMessenger) {
            this(peerId, queryId, timeout, peerInfoMessenger);
            this.monitorListener = monitorListener;
        }

        RequestInfo(PeerID peerId, int queryId, PeerMonitorInfoListener peerMonitorInfoListener, long timeout, PeerInfoMessenger peerInfoMessenger) {
            this(peerId, queryId, timeout, peerInfoMessenger);
            this.peerMonitorInfoListener = peerMonitorInfoListener;
        }

        RequestInfo(PeerID peerId, int queryId, long timeout, PeerInfoMessenger peerInfoMessenger) {
            this.peerId = peerId;
            this.queryId = queryId;
            this.timeout = timeout;
            this.peerInfoMessenger = peerInfoMessenger;
            this.validUntil = System.currentTimeMillis() + timeout;
        }
    }


    private class LeaseInfo {
        int leaseId;
        PeerID peerID; // Peer that requested the lease
        int queryId; // The other guy's query Id
        MonitorListener monitorListener;
        long validUntil;
        boolean listenerAddedToWorldGroup = false;
        PeerGroup worldGroup;
        PeerInfoMessenger peerInfoMessenger;
        LeaseInfo(int leaseId, PeerID peerID, int queryId, MonitorListener monitorListener, long leaseLength, PeerInfoMessenger peerInfoMessenger) {
            this.leaseId = leaseId;
            this.peerID = peerID;
            this.queryId = queryId;
            this.monitorListener = monitorListener;
            this.peerInfoMessenger = peerInfoMessenger;
            validUntil = System.currentTimeMillis() + leaseLength;
        }

    }

    public void getPeerMonitorInfo(final PeerID peerID, PeerMonitorInfoListener peerMonitorInfoListener, long timeout, PeerInfoMessenger peerInfoMessenger) throws MonitorException {
        int queryId = peerInfoServiceImpl.getNextQueryId();

        RemoteMonitorQuery remoteMonitorQuery = RemoteMonitorQuery.createPeerMonitorInfoQuery();
        peerInfoMessenger.sendPeerInfoRequest(queryId, peerID, MONITOR_HANDLER_NAME, remoteMonitorQuery);
        final RequestInfo requestInfo = new RequestInfo(peerID, queryId, peerMonitorInfoListener, timeout, peerInfoMessenger);
        requestInfos.put(queryId, requestInfo);
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (!requestInfo.responseReceived) {
                    PeerMonitorInfoEvent peerMonitorInfoEvent = new PeerMonitorInfoEvent(peerID, null);

                    requestInfo.peerMonitorInfoListener.peerMonitorInfoNotReceived(peerMonitorInfoEvent);
                    requestInfos.remove(requestInfo.queryId);
                }
            }
        }, timeout);
    }

    public void getCumulativeMonitorReport(PeerID peerID, MonitorFilter monitorFilter, MonitorListener monitorListener, long timeout, PeerInfoMessenger peerInfoMessenger) throws MonitorException {
        int queryId = peerInfoServiceImpl.getNextQueryId();
        RemoteMonitorQuery remoteMonitorQuery = RemoteMonitorQuery.createGetCumulativeReportQuery(monitorFilter);
        peerInfoMessenger.sendPeerInfoRequest(queryId, peerID, MONITOR_HANDLER_NAME, remoteMonitorQuery);
        final RequestInfo requestInfo = new RequestInfo(peerID, queryId, monitorListener, timeout, peerInfoMessenger);
        requestInfos.put(queryId, requestInfo);
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (!requestInfo.responseReceived) {
                    requestInfos.remove(requestInfo.queryId);
                }
            }
        }, timeout);
    }

    public void addRemoteMonitorListener(PeerID peerID, MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener, long lease, long timeout, PeerInfoMessenger peerInfoMessenger) throws MonitorException {
        int queryId = peerInfoServiceImpl.getNextQueryId();
        RemoteMonitorQuery remoteMonitorQuery = RemoteMonitorQuery.createRegisterMonitorQuery(includeCumulative, monitorFilter, reportRate, lease);
        peerInfoMessenger.sendPeerInfoRequest(queryId, peerID, MONITOR_HANDLER_NAME, remoteMonitorQuery);
        final RequestInfo requestInfo = new RequestInfo(peerID, queryId, monitorListener, timeout, peerInfoMessenger);
        requestInfo.requestedLease = lease;
        requestInfos.put(queryId, requestInfo);
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (!requestInfo.responseReceived) {
                    MonitorEvent monitorEvent = MonitorEvent.createFailureEvent(MonitorEvent.TIMEOUT, requestInfo.peerId,
                            requestInfo.queryId);

                    requestInfo.monitorListener.monitorRequestFailed(monitorEvent);
                    requestInfos.remove(requestInfo.queryId);
                }
            }
        }, timeout);
        scheduleTimeout(requestInfo);
    }

    public void removeRemoteMonitorListener(PeerID peerID, MonitorListener monitorListener, long timeout, PeerInfoMessenger peerInfoMessenger) throws MonitorException {
        int queryId = peerInfoServiceImpl.getNextQueryId();

        RequestInfo oldRequestInfo = null;
        for (Enumeration<RequestInfo> e = requestInfos.elements(); e.hasMoreElements();) {
            RequestInfo ri = e.nextElement();

            if (ri.monitorListener == monitorListener) {
                oldRequestInfo = ri;
                break;
            }
        }

        if (oldRequestInfo != null) {
            RemoteMonitorQuery remoteMonitorQuery = RemoteMonitorQuery.createRemoveMonitorListenerQuery(oldRequestInfo.leaseId);
            peerInfoMessenger.sendPeerInfoRequest(queryId, peerID, MONITOR_HANDLER_NAME, remoteMonitorQuery);
            final RequestInfo requestInfo = new RequestInfo(peerID, queryId, monitorListener, timeout, peerInfoMessenger);
            requestInfo.origRequestId = oldRequestInfo.queryId;
            requestInfos.put(queryId, requestInfo);
        }

        final RequestInfo requestInfo = oldRequestInfo;
        timer.schedule(new TimerTask() {

            @Override
            public void run() {
                requestInfos.remove(new Integer(requestInfo.queryId));
            }
        }, timeout);
    }

    public void removeRemoteMonitorListener(MonitorListener monitorListener, long timeout, PeerInfoMessenger peerInfoMessenger) throws MonitorException {
        for (Enumeration<RequestInfo> e = requestInfos.elements(); e.hasMoreElements();) {
            RequestInfo requestInfo = e.nextElement();

            if (requestInfo.monitorListener == monitorListener) {
                removeRemoteMonitorListener(requestInfo.peerId, monitorListener, timeout, peerInfoMessenger);
            }
        }
    }

    public void processRequest(int queryId, PeerID requestSourceID, PeerInfoQueryMessage peerInfoQueryMessage, Element requestElement, PeerInfoMessenger peerInfoMessenger) {
        try {
            RemoteMonitorQuery remoteMonitorQuery = (RemoteMonitorQuery) DocumentSerializableUtilities.getDocumentSerializable(requestElement, RemoteMonitorQuery.class);

            if (remoteMonitorQuery.isRegisterMonitorQuery()) {
                handleRegisterMonitorQuery(queryId, requestSourceID, remoteMonitorQuery, peerInfoMessenger);

            } else if (remoteMonitorQuery.isCumulativeReportQuery()) {
                handleCumulativeReportQuery(queryId, requestSourceID, remoteMonitorQuery.getMonitorFilter(), peerInfoMessenger);

            } else if (remoteMonitorQuery.isRemoveMonitorQuery()) {
                handleRemoveMonitorQuery(queryId, requestSourceID, remoteMonitorQuery, peerInfoMessenger);

            } else if (remoteMonitorQuery.isPeerMonitorInfoQuery()) {
                handlePeerMonitorInfoQuery(queryId, requestSourceID, peerInfoMessenger);

            } else if (remoteMonitorQuery.isLeaseRenewal()) {
                handleLeaseRenewalQuery(queryId, requestSourceID, remoteMonitorQuery, peerInfoMessenger);

            }
        } catch (Exception e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Monitor failed in processQuery", e);
            }
        }
    }

    public void processResponse(int queryId, PeerInfoResponseMessage peerInfoResponseMessage, Element responseElement, PeerInfoMessenger peerInfoMessenger) {

        RemoteMonitorResponse remoteMonitorResponse;

        try {
            remoteMonitorResponse = (RemoteMonitorResponse) DocumentSerializableUtilities.getDocumentSerializable(responseElement, RemoteMonitorResponse.class);
            RequestInfo requestInfo = requestInfos.get(new Integer(queryId));

            if (requestInfo != null) {
                requestInfo.responseReceived = true;

                resetTimeout(requestInfo);
                if (remoteMonitorResponse.isMonitorRegistered()) {
                    int leaseId = remoteMonitorResponse.getLeaseId();
                    long leaseLength = remoteMonitorResponse.getLease();

                    requestInfo.leaseId = leaseId;
                    scheduleLeaseRenewal(requestInfo, leaseLength);

                } else if (remoteMonitorResponse.isMonitorRemoved()) {
                    requestInfos.remove(new Integer(requestInfo.origRequestId));
                    requestInfos.remove(new Integer(queryId));

                } else if (remoteMonitorResponse.isCumulativeReport() || remoteMonitorResponse.isMonitorReport()) {
                    MonitorReport monitorReport = remoteMonitorResponse.getMonitorReport();
                    MonitorEvent monitorEvent = MonitorEvent.createRemoteMonitorReportEvent(requestInfo.peerId, requestInfo.queryId, monitorReport);
                    requestInfo.monitorListener.processMonitorReport(monitorEvent);
                } else if (remoteMonitorResponse.isInvalidFilter()) {
                    MonitorEvent monitorEvent = MonitorEvent.createFailureEvent(MonitorEvent.INVALID_MONITOR_FILTER, requestInfo.peerId, requestInfo.queryId);
                    requestInfo.monitorListener.monitorRequestFailed(monitorEvent);
                    requestInfos.remove(new Integer(queryId));
                } else if (remoteMonitorResponse.isInvalidReportRate()) {
                    MonitorEvent monitorEvent = MonitorEvent.createFailureEvent(MonitorEvent.INVALID_REPORT_RATE, requestInfo.peerId, requestInfo.queryId);
                    requestInfo.monitorListener.monitorRequestFailed(monitorEvent);
                    requestInfos.remove(new Integer(queryId));
                } else if (remoteMonitorResponse.isMeteringNotSupported()) {
                    MonitorEvent monitorEvent = MonitorEvent.createFailureEvent(MonitorEvent.REFUSED, requestInfo.peerId, requestInfo.queryId);
                    requestInfo.monitorListener.monitorRequestFailed(monitorEvent);
                    requestInfos.remove(new Integer(queryId));
                } else if (remoteMonitorResponse.isRequestDenied()) {
                    MonitorEvent monitorEvent = MonitorEvent.createFailureEvent(MonitorEvent.REFUSED, requestInfo.peerId, requestInfo.queryId);
                    requestInfo.monitorListener.monitorRequestFailed(monitorEvent);
                } else if (remoteMonitorResponse.isPeerMonitorInfo()) {
                    PeerMonitorInfoEvent peerMonitorInfoEvent = new PeerMonitorInfoEvent(requestInfo.peerId, remoteMonitorResponse.getPeerMonitorInfo());
                    requestInfo.peerMonitorInfoListener.peerMonitorInfoReceived(peerMonitorInfoEvent);
                    requestInfos.remove(new Integer(queryId));
                } else if (remoteMonitorResponse.isLeaseRenewed()) {
                    long lease = remoteMonitorResponse.getLease();
                    int origRequestId = requestInfo.origRequestId;
                    RequestInfo origRequest = requestInfos.get(new Integer(origRequestId));
                    scheduleLeaseRenewal(origRequest, lease);
                    requestInfos.remove(new Integer(queryId));
                }
            }
        } catch (DocumentSerializationException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Document Serialization Failed", e);
            }
        }
    }

    private void resetTimeout(RequestInfo requestInfo) {
        timeouts.put(requestInfo.queryId, requestInfo.timeout + System.currentTimeMillis());
    }

    private long getTimeout(int queryId) {
        return timeouts.get(queryId);
    }

    private void scheduleTimeout(final RequestInfo requestInfo) {
        final int queryId = requestInfo.queryId;

        timer.schedule(
                new TimerTask() {
            @Override
            public void run() {
                if (requestInfos.containsKey(new Integer(queryId))) {
                    try {
                        if (System.currentTimeMillis() > getTimeout(queryId)) {
                            MonitorEvent monitorEvent = MonitorEvent.createFailureEvent(MonitorEvent.TIMEOUT, requestInfo.peerId, queryId);
                            requestInfo.monitorListener.monitorRequestFailed(monitorEvent);
                        }
                    } catch (Exception e) {
                        //ignored
                    }
                } else {
                    cancel();
                }
            }
        }, requestInfo.timeout, requestInfo.timeout);
    }

    private void scheduleLeaseRenewal(RequestInfo requestInfo, long leaseLength) {
        long roundTrip = requestInfo.requestTime - System.currentTimeMillis();
        long renewTime = leaseLength - roundTrip - 30 * 1000L; // 30s comfort
        // zone.
        final int queryId = requestInfo.queryId;

        if (renewTime > MIN_LEASE) {
            timer.schedule(new TimerTask() {

                @Override
                public void run() {
                    try {
                        renewLease(queryId);
                    } catch (Exception e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Lease Renewal Failed", e);
                        }
                    }
                }
            }, renewTime);
        }
    }

    private void handleRegisterMonitorQuery(final int queryId, final PeerID requestSourceID, RemoteMonitorQuery remoteMonitorQuery, final PeerInfoMessenger peerInfoMessenger) {
        MonitorFilter monitorFilter = remoteMonitorQuery.getMonitorFilter();
        long lease = remoteMonitorQuery.getLease();
        long reportRate = remoteMonitorQuery.getReportRate();
        boolean includeCumulative = remoteMonitorQuery.isIncludeCumulative();

        MonitorListener monitorListener = new MonitorListener() {

            public void processMonitorReport(MonitorEvent monitorEvent) {
                MonitorReport monitorReport = monitorEvent.getMonitorReport();
                try {
                    RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createMonitorReportResponse(queryId, monitorReport);
                    peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
                } catch (Exception e) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine(e.toString());
                    }
                }
            }

            public void monitorReportingCancelled(MonitorEvent monitorEvent) {
                throw new RuntimeException("METHOD NOT IMPLEMENTED");
            }

            public void monitorRequestFailed(MonitorEvent monitorEvent) {
                throw new RuntimeException("METHOD NOT IMPLEMENTED");
            }
        };

        int leaseId = getNextLeaseId();
        final LeaseInfo leaseInfo = new LeaseInfo(leaseId, requestSourceID, queryId, monitorListener, lease, peerInfoMessenger);
        long leaseTime = getLeaseTime(lease);

        setupLeaseTimeout(leaseInfo.leaseId, leaseTime);

        try {
            /*
             * Currently we can neither ask peers in the netgroup for transport
             * metrics, nor discover peers in the world group. Therefore we're
             * asking peers in the netgroup to send TransportMetrics, but that
             * peer is actually attaching the MonitorFilter to it's WorldGroup
             * peer.
             */
            for (Iterator i = monitorFilter.getServiceMonitorFilters(); i.hasNext();) {
                ServiceMonitorFilter serviceMonitorFilter = (ServiceMonitorFilter) i.next();

                if (serviceMonitorFilter.getModuleClassID().equals(MonitorResources.transportServiceMonitorClassID)) {
                    try {
                        MonitorFilter worldGroupFilter = new MonitorFilter("worldGroupFilter");

                        worldGroupFilter.addServiceMonitorFilter(serviceMonitorFilter);
                        i.remove();
                        PeerGroup worldGroup = peerGroup.newGroup(PeerGroupID.worldPeerGroupID);
                        PeerInfoService worldService = worldGroup.getPeerInfoService();

                        worldService.addMonitorListener(worldGroupFilter, remoteMonitorQuery.getReportRate(), includeCumulative,
                                monitorListener);
                        leaseInfo.listenerAddedToWorldGroup = true;
                        leaseInfo.worldGroup = worldGroup;
                    } catch (PeerGroupException e) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine(e.toString());
                        }
                    }
                }
            }
            if (monitorFilter.getServiceMonitorFilterCount() > 0) {
                peerInfoServiceImpl.addMonitorListener(monitorFilter, reportRate, includeCumulative, monitorListener);
            }
            leaseInfos.put(leaseId, leaseInfo);
            RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createMonitorRegisteredResponse(queryId, leaseId, leaseTime);
            peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
        } catch (MonitorFilterException e) {
            RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createInvalidFilterResponse(queryId);

            peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
        } catch (MonitorException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(e.toString());
            }
        }
    }

    private void handleRemoveMonitorQuery(int queryId, PeerID requestSourceID, RemoteMonitorQuery remoteMonitorQuery, PeerInfoMessenger peerInfoMessenger) {
        try {
            int leaseId = remoteMonitorQuery.getLeaseId();
            LeaseInfo leaseInfo = leaseInfos.get(new Integer(leaseId));

            if (leaseInfo != null) {
                MonitorListener monitorListener = leaseInfo.monitorListener;

                peerInfoServiceImpl.removeMonitorListener(monitorListener);
                if (leaseInfo.listenerAddedToWorldGroup) {
                    PeerInfoService peerInfoService = leaseInfo.worldGroup.getPeerInfoService();
                    peerInfoService.removeMonitorListener(monitorListener);
                }

                RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createMonitorRemovedResponse(queryId);
                peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
            }
        } catch (MonitorException e) {
            // Currently not thrown by MonitorManager.removeMonitorListener()
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(e.toString());
            }
        }
    }

    private void handleCumulativeReportQuery(int queryId, PeerID requestSourceID, MonitorFilter monitorFilter, PeerInfoMessenger peerInfoMessenger) throws MonitorException, DocumentSerializationException {
        MonitorReport monitorReport = peerInfoServiceImpl.getCumulativeMonitorReport(monitorFilter);
        RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createCumulativeReportResponse(queryId, monitorReport);
        peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
    }

    private void handlePeerMonitorInfoQuery(int queryId, PeerID requestSourceID, PeerInfoMessenger peerInfoMessenger) throws DocumentSerializationException {
        // FIX-ME:
        /* Asking the NetGroup Peer won't tell me if it supports transport
         * monitoring or not, but asking the world group guy gives me
         * everything I need because as currently implemented you can't turn
         * monitoring on or off at the PeerGroup level, only the device level.
         */
        try {
            PeerGroup worldGroup = peerGroup.newGroup(PeerGroupID.worldPeerGroupID);
            PeerInfoService worldService = worldGroup.getPeerInfoService();

            PeerMonitorInfo peerMonitorInfo = worldService.getPeerMonitorInfo();
            RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createPeerMonitorInfoResponse(queryId, peerMonitorInfo);
            peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
        } catch (PeerGroupException e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(e.toString());
            }
        }
    }

    private void handleLeaseRenewalQuery(int queryId, PeerID requestSourceID, RemoteMonitorQuery remoteMonitorQuery, PeerInfoMessenger peerInfoMessenger) throws DocumentSerializationException {
        int leaseId = remoteMonitorQuery.getLeaseId();
        LeaseInfo leaseInfo = leaseInfos.get(new Integer(leaseId));

        if (leaseInfo != null) {
            long reqLease = remoteMonitorQuery.getLease();
            long lease = getLeaseTime(reqLease);

            leaseInfo.validUntil = System.currentTimeMillis() + lease;
            setupLeaseTimeout(leaseInfo.leaseId, lease);

            RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createLeaseRenewedResponse(queryId, leaseInfo.leaseId, lease);
            peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
        } else {
            RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createDeniedResponse(queryId);
            peerInfoMessenger.sendPeerInfoResponse(queryId, requestSourceID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
        }
    }

    long getLeaseTime(long requestedLease) {
        long leaseTime = requestedLease < MAX_LEASE ? requestedLease : MAX_LEASE;

        leaseTime = leaseTime > MIN_LEASE ? leaseTime : MIN_LEASE;
        return leaseTime;
    }

    private void cancelLease(LeaseInfo leaseInfo) throws MonitorException, DocumentSerializationException {
        if (leaseInfo.listenerAddedToWorldGroup) {
            leaseInfo.worldGroup.getPeerInfoService().removeMonitorListener(leaseInfo.monitorListener);
        }

        RemoteMonitorResponse remoteMonitorResponse = RemoteMonitorResponse.createLeaseEndedResponse(leaseInfo.queryId, leaseInfo.leaseId);
        leaseInfo.peerInfoMessenger.sendPeerInfoResponse(leaseInfo.queryId, leaseInfo.peerID, MONITOR_HANDLER_NAME, remoteMonitorResponse);
    }

    private void renewLease(int queryId) {
        try {
            RequestInfo requestInfo = requestInfos.get(new Integer(queryId));

            if (requestInfo != null) {
                int renewalQueryId = peerInfoServiceImpl.getNextQueryId();
                PeerID peerID = requestInfo.peerId;
                long timeout = requestInfo.timeout;

                RemoteMonitorQuery remoteMonitorQuery = RemoteMonitorQuery.createLeaseRenewalQuery(requestInfo.leaseId, requestInfo.requestedLease);

                requestInfo.peerInfoMessenger.sendPeerInfoRequest(queryId, peerID, MONITOR_HANDLER_NAME, remoteMonitorQuery);
                final RequestInfo renewalRequestInfo = new RequestInfo(peerID, queryId, timeout, requestInfo.peerInfoMessenger);

                renewalRequestInfo.requestedLease = requestInfo.requestedLease;
                renewalRequestInfo.origRequestId = queryId;
                requestInfos.put(renewalQueryId, renewalRequestInfo);
            }
        } catch (Exception e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "error while attempting Monitor lease renewal", e);
            }
        }
    }

    private void setupLeaseTimeout(final int leaseId, long lease) {

        timer.schedule(new TimerTask() {

            @Override
            public void run() {
                LeaseInfo leaseInfo = leaseInfos.get(new Integer(leaseId));

                if (leaseInfo != null) {
                    long currentTime = System.currentTimeMillis();

                    if (leaseInfo.validUntil <= currentTime) {
                        try {
                            cancelLease(leaseInfo);
                        } catch (Exception e) {
                            // ignored
                        } finally {
                            leaseInfos.remove(leaseInfo.queryId);
                        }
                    }
                }
            }
        }, lease);
    }
}
