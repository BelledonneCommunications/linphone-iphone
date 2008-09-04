/*
 * Copyright (c) 2001-2007 Sun Micro//Systems, Inc.  All rights reserved.
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

package net.jxta.impl.resolver.resolverMeter;


import net.jxta.peer.*;
import net.jxta.peergroup.*;
import net.jxta.resolver.*;
import net.jxta.impl.resolver.*;
import net.jxta.impl.meter.*;
import net.jxta.protocol.*;
import net.jxta.meter.*;
import net.jxta.util.*;

import net.jxta.endpoint.*;
import net.jxta.exception.*;

import java.net.*;
import java.util.*;


public class QueryHandlerMeter {
    private static final int QUERY_CULLING_INTERVAL = 5 * 60 * 1000; // Fix-Me: Five minutes hardcoded for now...
	
    private ResolverServiceMonitor resolverServiceMonitor;
    private String handlerName;

    private QueryHandlerMetric cumulativeMetrics;
    private QueryHandlerMetric deltaMetrics;

    private Hashtable queryDestinationMeters = new Hashtable();

    private Hashtable queryMetricsTable = null;

    private class QueryMetricsTable extends Hashtable {

        void deReference() {
            queryMetricsTable = null;
        }

        @Override
        public String toString() {
            return handlerName;
        }
    }


    private class QueryMetric {
        int queryId;
        long querySentTime = System.currentTimeMillis();
        int numResponsesReceived = 0;
        long lastResponseTime = 0;

        QueryMetric(int queryId) {
            this.queryId = queryId;
        }
    }

    private static LinkedList queryMetricsTables = new LinkedList();
	
    static {
        Thread cullQueries = new Thread(new Runnable() {
            public void run() {
                for (;;) {
                    try {
                        Thread.sleep(QUERY_CULLING_INTERVAL);
                    } catch (Exception e) {}

                    long dormantTime = System.currentTimeMillis() - QUERY_CULLING_INTERVAL;
					
                    synchronized (queryMetricsTables) {
                        LinkedList keysToRemove = new LinkedList();
						
                        for (Iterator i = queryMetricsTables.iterator(); i.hasNext();) {
                            QueryMetricsTable queryMetricsTable = (QueryMetricsTable) i.next();

                            keysToRemove.clear();
							
                            synchronized (queryMetricsTable) {
                                for (Enumeration e = queryMetricsTable.keys(); e.hasMoreElements();) {
                                    Integer key = (Integer) e.nextElement();
                                    QueryMetric queryMetric = (QueryMetric) queryMetricsTable.get(key);
	
                                    if (queryMetric.lastResponseTime < dormantTime) {
                                        keysToRemove.add(key);
                                    }
                                }

                                for (Iterator k = keysToRemove.iterator(); k.hasNext();) {
                                    Integer key = (Integer) k.next();

                                    queryMetricsTable.deReference();
                                    queryMetricsTable.remove(key);
                                }

                                if (queryMetricsTable.size() == 0) {
                                    i.remove();
                                }
                            }
                        }
                    }
                }
            }
        }, "Resolver Query Metrics Culling Thread");

        cullQueries.setDaemon(true);
        cullQueries.start();
    }

    public QueryHandlerMeter(String handlerName, ResolverServiceMonitor resolverServiceMonitor) {
        this.handlerName = handlerName;
        cumulativeMetrics = new QueryHandlerMetric(handlerName);
        this.resolverServiceMonitor = resolverServiceMonitor;
    }

    public QueryHandlerMetric getCumulativeMetrics() {
        return cumulativeMetrics;
    }

    public String getHandlerName() {
        return handlerName;
    }
	
    public synchronized QueryHandlerMetric collectMetrics() {
        QueryHandlerMetric prevDelta = deltaMetrics;

        for (Enumeration e = queryDestinationMeters.elements(); e.hasMoreElements();) {
            QueryDestinationMeter queryDestinationMeter = (QueryDestinationMeter) e.nextElement();

            QueryDestinationMetric queryDestinationMetric = queryDestinationMeter.collectMetrics();

            if (queryDestinationMetric != null) {

                /* Fix-me: while fixing an exception thrown by SrdiHandlerMeter, I noticed that
                 a similar problem might occur here if there can be a case where a destinationMtric
                 is available even though nothing has happened to create the delta.
                 If that is not possible (we need to do a code review!) remove the check for
                 a null delta here.
                 */
                if (prevDelta == null) {
                    createDeltaMetric();
                    prevDelta = deltaMetrics;
                }
                prevDelta.addQueryDestinationMetric(queryDestinationMetric);
            }
        }
        deltaMetrics = null;
        return prevDelta;
    }

    public synchronized QueryDestinationMeter getQueryDestinationMeter(EndpointAddress endpointAddress) {
        PeerID peerID = MetricUtilities.getPeerIdFromEndpointAddress(endpointAddress);

        return getQueryDestinationMeter(peerID);
    }
		
    public synchronized QueryDestinationMeter getQueryDestinationMeter(String peerIdString) {
        PeerID peerID = MetricUtilities.getPeerIdFromString(peerIdString);

        return getQueryDestinationMeter(peerID);
    }
		
    public synchronized QueryDestinationMeter getQueryDestinationMeter(PeerID peerID) {
        QueryDestinationMeter queryDestinationMeter = (QueryDestinationMeter) queryDestinationMeters.get(peerID);

        if (queryDestinationMeter == null) {
            queryDestinationMeter = new QueryDestinationMeter(peerID);
			
            queryDestinationMeters.put(peerID, queryDestinationMeter);
            cumulativeMetrics.addQueryDestinationMetric(queryDestinationMeter.getCumulativeMetrics());
        }

        return queryDestinationMeter;
    }
	
    private void createDeltaMetric() {
        deltaMetrics = new QueryHandlerMetric(handlerName);
    }

    @Override
    public String toString() {
        return "ResolverHandlerMeter(" + handlerName + ")";
    }

    public void setRegistered(boolean registered) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.setRegistered(registered);
        cumulativeMetrics.setRegistered(registered);
    }

    private QueryMetric getQueryMetric(int queryId) {
        Integer key = new Integer(queryId);

        synchronized (queryMetricsTables) {
            if (queryMetricsTable == null) {
                queryMetricsTable = new QueryMetricsTable();
				
                queryMetricsTables.add(queryMetricsTable);
            }
			
            QueryMetric queryMetric = (QueryMetric) queryMetricsTable.get(key);
	
            if (queryMetric == null) {
                queryMetric = new QueryMetric(queryId);
				
                queryMetricsTable.put(key, queryMetric);
            }
	
            return queryMetric;	
        }
    }
	
    // Sent Queries

	
    public void querySentInGroup(ResolverQueryMsg query) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        getQueryMetric(query.getQueryId()); // register the query

        deltaMetrics.querySentInGroup();
        cumulativeMetrics.querySentInGroup();		
    }

    public void querySentViaWalker(ResolverQueryMsg query) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        getQueryMetric(query.getQueryId()); // register the query

        deltaMetrics.querySentViaWalker();
        cumulativeMetrics.querySentViaWalker();		
    }

    public void querySentViaUnicast(String peer, ResolverQueryMsg query) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        getQueryMetric(query.getQueryId()); // register the query
		
        deltaMetrics.querySentViaUnicast(peer);
        cumulativeMetrics.querySentViaUnicast(peer);

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(peer);

        destinationMeter.querySentViaUnicast();
    }

    public void querySendError() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.querySendError();
        cumulativeMetrics.querySendError();		
    }

    public void queryPropagateError() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.queryPropagateError();
        cumulativeMetrics.queryPropagateError();		
    }

    public void queryHopCountDropped() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.queryHopCountDropped();
        cumulativeMetrics.queryHopCountDropped();		
    }

    public void unableToPropagate() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.unableToPropagate();
        cumulativeMetrics.unableToPropagate();		
    }

    // Propagate Query

    public void queryPropagatedInGroup() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.queryPropagatedInGroup();
        cumulativeMetrics.queryPropagatedInGroup();		
    }

    public void queryPropagatedViaWalker() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.queryPropagatedViaWalker();
        cumulativeMetrics.queryPropagatedViaWalker();		
    }

    public void propagationQueryDropped() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.propagationQueryDropped();
        cumulativeMetrics.propagationQueryDropped();		
    }
	
    // Sent Responses

    public void responseSentViaUnicast(String peer, ResolverResponseMsg response) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.responseSentViaUnicast();
        cumulativeMetrics.responseSentViaUnicast();	

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(peer);

        destinationMeter.responseSentViaUnicast();
    }

    public void responseSentViaWalker(ResolverResponseMsg response) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.responseSentViaWalker();
        cumulativeMetrics.responseSentViaWalker();		
    }

    public void responseSentInGroup(ResolverResponseMsg response) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.responseSentInGroup();
        cumulativeMetrics.responseSentInGroup();		
    }		

    public void responseSendError() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.responseSendError();
        cumulativeMetrics.responseSendError();		
    }

    public void responsePropagateError() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.responsePropagateError();
        cumulativeMetrics.responsePropagateError();		
    }

    // Received Responses

    public void responseProcessed(ResolverResponseMsg response, long processTime, EndpointAddress srcAddr) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        QueryMetric queryMetric = getQueryMetric(response.getQueryId());
        long now = System.currentTimeMillis();
        long responseTime = now - queryMetric.querySentTime;
	
        queryMetric.lastResponseTime = now;
        queryMetric.numResponsesReceived++;
		
        deltaMetrics.responseProcessed(responseTime, processTime);
        cumulativeMetrics.responseProcessed(responseTime, processTime);

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(srcAddr);

        destinationMeter.responseProcessed();
    }

    public void responseToUnregisteredHandler(EndpointAddress src) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.responseToUnregisteredHandler();
        cumulativeMetrics.responseToUnregisteredHandler();

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(src);

        destinationMeter.responseToUnregisteredHandler();
    }

    public void errorWhileProcessingResponse(EndpointAddress srcAddr) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.errorWhileProcessingResponse();
        cumulativeMetrics.errorWhileProcessingResponse();

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(srcAddr);

        destinationMeter.errorWhileProcessingResponse();
    }	
	
    // Received Queries	

    public void queryProcessed(ResolverQueryMsg query, int result, long processTime) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.queryProcessed(result, processTime);
        cumulativeMetrics.queryProcessed(result, processTime);

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(query.getSrc());

        destinationMeter.queryProcessed();
    }

    public void queryToUnregisteredHandler(ResolverQueryMsg query) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.queryToUnregisteredHandler();
        cumulativeMetrics.queryToUnregisteredHandler();

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(query.getSrc());

        destinationMeter.queryToUnregisteredHandler();
    }

    public void errorWhileProcessingQuery(ResolverQueryMsg query) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.errorWhileProcessingQuery();
        cumulativeMetrics.errorWhileProcessingQuery();

        QueryDestinationMeter destinationMeter = getQueryDestinationMeter(query.getSrc());

        destinationMeter.errorWhileProcessingQuery();
    }

    public Enumeration getQueryDestinationMeters() {
        return queryDestinationMeters.elements();
    }

    public int getQueryDestinationCount() {
        return queryDestinationMeters.size();
    }
}
