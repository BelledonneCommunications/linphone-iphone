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


import java.util.Hashtable;
import java.util.Random;
import java.io.StringReader;
import java.net.URI;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.service.Service;
import net.jxta.endpoint.EndpointService;
import net.jxta.resolver.ResolverService;
import net.jxta.resolver.QueryHandler;
import net.jxta.document.Advertisement;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.document.MimeMediaType;
import net.jxta.document.Element;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerInfoQueryMessage;
import net.jxta.protocol.PeerInfoResponseMessage;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peer.PeerInfoService;
import net.jxta.credential.Credential;
import net.jxta.membership.MembershipService;
import net.jxta.impl.protocol.ResolverQuery;
import net.jxta.impl.protocol.ResolverResponse;
import net.jxta.impl.protocol.PeerInfoQueryMsg;
import net.jxta.impl.protocol.PeerInfoResponseMsg;
import net.jxta.exception.JxtaException;
import net.jxta.exception.PeerGroupException;

import net.jxta.meter.*;
import net.jxta.impl.meter.*;
import net.jxta.peer.*;
import net.jxta.platform.*;
import net.jxta.util.documentSerializable.*;


/**
 *  Peer Info provides a mechanism to obtain  information about peers.
 *
 */

public class PeerInfoServiceImpl implements PeerInfoService {
    
    private final static Logger LOG = Logger.getLogger(PeerInfoServiceImpl.class.getName());
    
    /**
     *  Time in milli seconds since midnight, January 1, 1970 UTC and when this
     *  peer was started.
     */
    private long startTime = 0;
    
    private ResolverService resolver = null;
    private PeerGroup group = null;
    private EndpointService endpoint = null;
    private PeerID localPeerId = null;
    private ModuleImplAdvertisement implAdvertisement = null;
    private String resolverHandlerName = null;
    private MembershipService membership = null;
    private Credential credential = null;
    private StructuredDocument credentialDoc = null;
    private MonitorManager monitorManager;
    private Hashtable peerInfoHandlers = new Hashtable();
    private PipQueryHandler pipQueryHandler = new PipQueryHandler();
    private RemoteMonitorPeerInfoHandler remoteMonitorPeerInfoHandler;
    private PeerInfoMessenger resolverServicePeerInfoMessenger = new ResolverServicePeerInfoMessenger();
    
    private int nextQueryId = 1000;
    private static final Random rand = new Random();
    
    // This static package public hashtable of registered PeerInfoServiceImpls
    // allows us to do Peergroup Monitoring via an IP Bridge to the PIP
    // See the documentation on the JXTA Monitor
    static Hashtable peerInfoServices = new Hashtable();
    
    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {
        this.group = group;
        
        implAdvertisement = (ModuleImplAdvertisement) impl;
        localPeerId = group.getPeerID();
        resolverHandlerName = assignedID.toString();
        
        // Fix-Me: When MonitorManager is a true Service, this should be moved to startApp()
        try {
            if (MeterBuildSettings.METERING) {
                monitorManager = MonitorManager.registerMonitorManager(group);
            }
            // FIXME This will become a service lookup when MonitorService is a real service
        } catch (JxtaException e) {
            throw new PeerGroupException("Unable to load MonitorManager", e);
        }
        
        // record start time at end of successful init
        startTime = System.currentTimeMillis();
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring PeerInfo Service : " + assignedID);

            configInfo.append("\n\tImplementation:");
            configInfo.append("\n\t\tImpl Description: ");
            configInfo.append(implAdvertisement.getDescription());
            configInfo.append("\n\t\tImpl URI : ").append(implAdvertisement.getUri());
            configInfo.append("\n\t\tImpl Code : ").append(implAdvertisement.getCode());
            configInfo.append("\n\tGroup Params:");
            configInfo.append("\n\t\tGroup: ").append(group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID: ").append(group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID: ").append(group.getPeerID());
            LOG.config(configInfo.toString());
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public int startApp(String[] arg) {
        
        resolver = group.getResolverService();
        
        if (null == resolver) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a resolver service");
            }
            
            return START_AGAIN_STALLED;
        }
        
        /* Fix-Me: When MonitorService is a true service, this should be moved here from init()
         try {
         if (MeterBuildSettings.METERING)
         monitorManager = MonitorManager.registerMonitorManager(group);
         // FIXME : This will become a service lookup when MonitorService is a real service
         } catch (JxtaException e) {
         return -1;		// Fix-Me: This is related to the initialization sequence work on the dev list on load order
         }
         */
        
        // remoteMonitorPeerInfoHandler = new RemoteMonitorPeerInfoHandler(group, this);
        // peerInfoHandlers.put(RemoteMonitorPeerInfoHandler.MONITOR_HANDLER_NAME, remoteMonitorPeerInfoHandler);
        
        resolver = group.getResolverService();
        resolver.registerHandler(resolverHandlerName, pipQueryHandler);
        
        peerInfoServices.put(group, this);
        
        return Module.START_OK;
    }
    
    /**
     * {@inheritDoc}
     */
    public void stopApp() {
        
        peerInfoServices.remove(group);
        resolver.unregisterHandler(resolverHandlerName);
        resolver = null;
        
        // peerInfoHandlers.remove(RemoteMonitorPeerInfoHandler.MONITOR_HANDLER_NAME);
        // remoteMonitorPeerInfoHandler.stop();
        
        if (MeterBuildSettings.METERING) {
            MonitorManager.unregisterMonitorManager(group);
        }
        
        group = null;
    }
    
    /**
     * {@inheritDoc}
     */
    public Service getInterface() {
        return new PeerInfoServiceInterface(this);
    }
    
    /**
     * {@inheritDoc}
     */
    public Advertisement getImplAdvertisement() {
        return implAdvertisement;
    }
    
    PeerInfoHandler getPeerInfoHandler(String name) {
        return (PeerInfoHandler) peerInfoHandlers.get(name);
    }
    
    int getNextQueryId() {
        int id = 0;

        synchronized (rand) {
            id = rand.nextInt(Integer.MAX_VALUE);
        }
        return id;
    }
    
    /**
     *  Returns the group to which this service is attached.
     *
     * @return    PeerGroup the group
     */
    public PeerGroup getGroup() {
        return group;
    }
    
    /**
     * {@inheritDoc}
     */
    public boolean isLocalMonitoringAvailable() {
        return MeterBuildSettings.METERING;
    }
    
    /**
     * {@inheritDoc}
     */
    public boolean isLocalMonitoringAvailable(ModuleClassID moduleClassID) {
        return MeterBuildSettings.METERING && monitorManager.isLocalMonitoringAvailable(moduleClassID);
    }
    
    /**
     * {@inheritDoc}
     */
    public long[] getSupportedReportRates() {
        return MonitorManager.getReportRates();
    }
    
    /**
     * {@inheritDoc}
     */
    public boolean isSupportedReportRate(long reportRate) {
        return monitorManager.isSupportedReportRate(reportRate);
    }
    
    /**
     * {@inheritDoc}
     */
    public long getBestReportRate(long desiredReportRate) {
        return monitorManager.getBestReportRate(desiredReportRate);
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerMonitorInfo getPeerMonitorInfo() {
        if (monitorManager != null) {
            return monitorManager.getPeerMonitorInfo();
        } else {
            return PeerMonitorInfo.NO_PEER_MONITOR_INFO;
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public void getPeerMonitorInfo(final PeerID peerID, PeerMonitorInfoListener peerMonitorInfoListener, long timeout) throws MonitorException {
        remoteMonitorPeerInfoHandler.getPeerMonitorInfo(peerID, peerMonitorInfoListener, timeout, resolverServicePeerInfoMessenger);
    }
    
    /**
     * {@inheritDoc}
     */
    public MonitorReport getCumulativeMonitorReport(MonitorFilter monitorFilter) throws MonitorException {
        if (MeterBuildSettings.METERING) {
            throw new MonitorException(MonitorException.METERING_NOT_SUPPORTED, "Local Monitoring not Available");
        }
        return monitorManager.getCumulativeMonitorReport(monitorFilter);
        
    }
    
    /**
     * {@inheritDoc}
     */
    public void getCumulativeMonitorReport(PeerID peerID, MonitorFilter monitorFilter, MonitorListener monitorListener, long timeout) throws MonitorException {
        remoteMonitorPeerInfoHandler.getCumulativeMonitorReport(peerID, monitorFilter, monitorListener, timeout
                ,
                resolverServicePeerInfoMessenger);
    }
    
    /**
     * {@inheritDoc}
     */
    public long addMonitorListener(MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener) throws MonitorException {
        if (!MeterBuildSettings.METERING) {
            throw new MonitorException(MonitorException.METERING_NOT_SUPPORTED, "Local Monitoring not Available");
        }
        
        return monitorManager.addMonitorListener(monitorFilter, reportRate, includeCumulative, monitorListener);
    }
    
    /**
     * {@inheritDoc}
     */
    public void addRemoteMonitorListener(PeerID peerID, MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener, long lease, long timeout) throws MonitorException {
        remoteMonitorPeerInfoHandler.addRemoteMonitorListener(peerID, monitorFilter, reportRate, includeCumulative
                ,
                monitorListener, lease, timeout, resolverServicePeerInfoMessenger);
    }
    
    /**
     * {@inheritDoc}
     */
    public boolean removeMonitorListener(MonitorListener monitorListener) throws MonitorException {
        
        int numRemoved = monitorManager.removeMonitorListener(monitorListener);

        return numRemoved > 0;
    }
    
    /**
     * {@inheritDoc}
     */
    public void removeRemoteMonitorListener(PeerID peerID, MonitorListener monitorListener, long timeout) throws MonitorException {
        remoteMonitorPeerInfoHandler.removeRemoteMonitorListener(peerID, monitorListener, timeout
                ,
                resolverServicePeerInfoMessenger);
    }
    
    /**
     * {@inheritDoc}
     */
    public void removeRemoteMonitorListener(MonitorListener monitorListener, long timeout) throws MonitorException {
        remoteMonitorPeerInfoHandler.removeRemoteMonitorListener(monitorListener, timeout, resolverServicePeerInfoMessenger);
    }
    
    class PipQueryHandler implements QueryHandler {
        
        /**
         * {@inheritDoc}
         */
        public int processQuery(ResolverQueryMsg query) {
            int queryId = query.getQueryId();
            PeerID requestSourceID = null;

            try {
                requestSourceID = (PeerID) IDFactory.fromURI(new URI(query.getSrc()));
            } catch (Exception e) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "PeerInfoService.processQuery got a bad query, not valid src", e);
                }
                return ResolverService.OK;
            }
            
            XMLDocument doc = null;

            try {
                doc = (XMLDocument)
                        StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, new StringReader(query.getQuery()));
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "PeerInfoService.processQuery got a bad adv", e);
                }
                return ResolverService.OK;
            }
            
            PeerInfoQueryMessage pipquery = new PeerInfoQueryMsg(doc);
            
            Element requestElement = pipquery.getRequest();
            String queryType = (String) requestElement.getKey();
            
            if (queryType != null) {
                PeerInfoHandler peerInfoHandler = getPeerInfoHandler(queryType);
                
                if (peerInfoHandler != null) {
                    peerInfoHandler.processRequest(queryId, requestSourceID, pipquery, requestElement
                            ,
                            resolverServicePeerInfoMessenger);
                } else {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("No registered PeerInfoHandler for this type of request");
                    }
                }
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("No request PeerInfoQueryMessage Request Element found");
                }
            }
            
            return ResolverService.OK;
        }
        
        /**
         * {@inheritDoc}
         */
        public void processResponse(ResolverResponseMsg response) {
            
            int queryId = response.getQueryId();
            
            PeerInfoResponseMessage resp = null;

            try {
                StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8
                        ,
                        new StringReader(response.getResponse()));
                
                resp = new PeerInfoResponseMsg(doc);
            } catch (Exception e) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "PeerInfoService.processResponse got a bad adv", e);
                }
                return;
            }
            
            Element responseElement = resp.getResponse();
            String responseType = (String) responseElement.getKey();
            
            if (responseType != null) {
                PeerInfoHandler peerInfoHandler = getPeerInfoHandler(responseType);
                
                if (peerInfoHandler != null) {
                    peerInfoHandler.processResponse(queryId, resp, responseElement, resolverServicePeerInfoMessenger);
                } else {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("No registered PeerInfoHandler for this type of response");
                    }
                }
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("No request PeerInfoResponseMessage Response Element found");
                }
            }
        }
    }
    

    private class ResolverServicePeerInfoMessenger implements PeerInfoMessenger {
        
        /**
         * {@inheritDoc}
         */
        public void sendPeerInfoResponse(int queryId, PeerID destinationPeerID, String peerInfoHandler, DocumentSerializable response) {
            try {
                PeerInfoResponseMessage peerInfoResponseMessage = new PeerInfoResponseMsg();

                peerInfoResponseMessage.setSourcePid(destinationPeerID);
                peerInfoResponseMessage.setTargetPid(localPeerId);
                
                long now = System.currentTimeMillis();

                peerInfoResponseMessage.setUptime(now - startTime);
                peerInfoResponseMessage.setTimestamp(now);
                
                Element responseElement = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, peerInfoHandler);

                response.serializeTo(responseElement);
                
                peerInfoResponseMessage.setResponse(responseElement);
                
                XMLDocument doc = (XMLDocument) peerInfoResponseMessage.getDocument(MimeMediaType.XMLUTF8);
                String peerInfoResponse = doc.toString();
                
                ResolverResponse resolverResponse = new ResolverResponse(resolverHandlerName, credentialDoc, queryId
                        ,
                        peerInfoResponse);

                resolver.sendResponse(destinationPeerID.toString(), resolverResponse);
            } catch (JxtaException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure building document", e);
                }
            }
        }
        
        /**
         * {@inheritDoc}
         */
        public void sendPeerInfoRequest(int queryID, PeerID destinationPeerID, String peerInfoHandler, DocumentSerializable request) {
            try {
                PeerInfoQueryMsg peerInfoQueryMsg = new PeerInfoQueryMsg();

                peerInfoQueryMsg.setSourcePid(localPeerId);
                peerInfoQueryMsg.setTargetPid(destinationPeerID);
                
                Element requestElement = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, peerInfoHandler);

                request.serializeTo(requestElement);
                
                peerInfoQueryMsg.setRequest(requestElement);
                
                XMLDocument doc = (XMLDocument) peerInfoQueryMsg.getDocument(MimeMediaType.XMLUTF8);
                String peerInfoRequest = doc.toString();
                
                ResolverQuery resolverQuery = new ResolverQuery(resolverHandlerName, credentialDoc, localPeerId.toString()
                        ,
                        peerInfoRequest, queryID);

                resolver.sendQuery(destinationPeerID.toString(), resolverQuery);
            } catch (JxtaException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure to build resolver query", e);
                }
            }
        }
    }
}
