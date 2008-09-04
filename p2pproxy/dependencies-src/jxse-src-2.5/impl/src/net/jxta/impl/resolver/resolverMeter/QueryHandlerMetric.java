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

package net.jxta.impl.resolver.resolverMeter;


import net.jxta.resolver.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.endpoint.*;
import net.jxta.exception.*;
import net.jxta.peer.*;
import net.jxta.util.*;
import java.util.*;


/**
 * Metrics for a Registered handler Name
 **/
public class QueryHandlerMetric implements DocumentSerializable {
    public static final String REGISTERED = "registered";
    public static final String UNREGISTERED = "unregistered";
	
    private String handlerName;
    private String registered;
	
    private int numResponses = 0;
    private long responseProcessingTime = 0;
    private long responseTime = 0;
    private int numResponseErrors = 0;

    private int numQueries = 0;
    private int numQueriesRepropagated = 0;
    private long queryProcessingTime = 0;
    private int numQueryErrors = 0;

    private int numQueriesSentInGroup = 0;
    private int numQueriesSentViaWalker = 0;
    private int numQueriesSentViaUnicast = 0;
    private int numErrorsSendingQueries = 0;
    private int numErrorsPropagatingQueries = 0;
    private int numQueriesHopCountDropped = 0;

    private int numPropagationQueriesDropped = 0;	
    private int numPropagatedInGroup = 0;	
    private int numPropagatedViaWalker = 0;
    private int numUnableToPropagate = 0;
	
    private int numResponsesToUnregisteredHandler = 0;
    private int numQueriesToUnregisteredHandler = 0;
	
    private int numResponsesSentInGroup = 0;
    private int numResponsesSentViaWalker = 0;
    private int numResponsesSentViaUnicast = 0;
    private int numErrorsSendingResponses = 0;
    private int numErrorsPropagatingResponses = 0;
	
    private HashMap destinationMetrics = new HashMap();

    public QueryHandlerMetric(String handlerName) {
        this.handlerName = handlerName;
    }

    public QueryHandlerMetric() {}

    public QueryHandlerMetric(QueryHandlerMetric prototype) {
        this.handlerName = prototype.handlerName;
    }

    void responseProcessed(long responseTime, long processingTime) {
        numResponses++;
        responseTime += responseTime;
        responseProcessingTime += processingTime;
    }

    void responseToUnregisteredHandler() {
        numResponsesToUnregisteredHandler++;
    }

    void errorWhileProcessingResponse() {
        numResponseErrors++;
    }

    void queryProcessed(int result, long processingTime) {
        numQueries++;

        if (result == ResolverService.Repropagate) {
            numQueriesRepropagated++;
        }
			
        queryProcessingTime += processingTime;
    }

    void queryToUnregisteredHandler() {
        numQueriesToUnregisteredHandler++;
    }
	
    void errorWhileProcessingQuery() {
        numQueryErrors++;
    }

    void querySentInGroup() {
        numQueriesSentInGroup++;
    }

    void querySentViaWalker() {
        numQueriesSentViaWalker++;
    }

    void querySentViaUnicast(String peer) {
        numQueriesSentViaUnicast++;
    }

    void querySendError() {
        numErrorsSendingQueries++;
    }

    void queryPropagateError() {
        numErrorsPropagatingQueries++;
    }

    void queryHopCountDropped() {
        numQueriesHopCountDropped++;
    }

    void responseSentInGroup() {
        numResponsesSentInGroup++;
    }

    void responseSentViaWalker() {
        numResponsesSentViaWalker++;
    }

    void responseSentViaUnicast() {
        numResponsesSentViaUnicast++;
    }

    void responseSendError() {
        numErrorsSendingResponses++;
    }
	
    void responsePropagateError() {
        numErrorsPropagatingResponses++;
    }

    void propagationQueryDropped() {
        numPropagationQueriesDropped++;
    }

    void queryPropagatedInGroup() {
        numPropagatedInGroup++;
    }

    void queryPropagatedViaWalker() {
        numPropagatedViaWalker++;
    }

    void unableToPropagate() {
        numUnableToPropagate++;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof QueryHandlerMetric) {
            QueryHandlerMetric otherQueryHandlerMetric = (QueryHandlerMetric) obj;

            return handlerName.equals((otherQueryHandlerMetric.handlerName));
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() { 
        return handlerName.hashCode();
    }

    void setRegistered(boolean isRegistered) {
        this.registered = isRegistered ? REGISTERED : UNREGISTERED;
    }

    /** Get Handler Name for this Metric **/
    public String getHandlerName() {
        return handlerName;
    }

    /** Get State of Handler currently 
     * @return REGISTERED or UNREGISTERED
     **/
    public String getRegistered() {
        return (registered != null) ? registered : UNREGISTERED;
    }

    /** The number of responses received by this Handler **/
    public int getNumResponses() {
        return numResponses;
    }

    /** The total clock time to process all responses received by this Handler **/
    public long getResponseProcessingTime() {
        return responseProcessingTime;
    }

    /** The average clock time to process responses received by this Handler **/
    public long getAverageResponseProcessingTime() {
        return (numResponses == 0) ? 0 : (responseProcessingTime / numResponses);
    }

    /** The total time to receive all responses to queries sent by this Handler **/
    public long getResponseTime() {
        return responseTime;
    }

    /** The average time to receive responses to queries sent by this Handler **/
    public long getAverageResponseTime() {
        return (numResponses == 0) ? 0 : (responseTime / numResponses);
    }

    /** Number of local errors while processing received responses **/
    public int getNumResponseErrors() {
        return numResponseErrors;
    }
	
    /** The number of queries received by this Handler **/
    public int getNumQueries() {
        return numQueries;
    }

    /** The number of queries received and repropagated by this Handler **/
    public int getNumQueriesRepropagated() {
        return numQueriesRepropagated;
    }

    /** The total clock time to process all Queries received by this Handler **/
    public long getQueryProcessingTime() {
        return queryProcessingTime;
    }

    /** The average clock time to process Queries received by this Handler **/
    public long getAverageQueryProcessingTime() {
        return (numQueries == 0) ? 0 : (queryProcessingTime / numQueries);
    }

    /** Number of local errors while processing received Queries **/
    public int getNumQueryErrors() {
        return numQueryErrors;
    }

    /** Number of Queries sent in Group ***/
    public int getNumQueriesSentInGroup() {
        return numQueriesSentInGroup;
    }

    /** Number of Queries sent via Walker ***/
    public int getNumQueriesSentViaWalker() {
        return numQueriesSentViaWalker;
    }

    /** Number of Queries sent via Unicast ***/
    public int getNumQueriesSentViaUnicast() {
        return numQueriesSentViaUnicast;
    }

    /** Number of Errors while sending Queries ***/
    public int getNumErrorsSendingQueries() {
        return numErrorsSendingQueries;
    }

    /** Number of Errors while propagating Queries ***/
    public int getNumErrorsPropagatingQueries() {
        return numErrorsPropagatingQueries;
    }
	
    /** Number of Responses sent in Group ***/
    public int getNumResponsesSentInGroup() {
        return numResponsesSentInGroup;
    }

    /** Number of Responses sent via Walker ***/
    public int getNumResponsesSentViaWalker() {
        return numResponsesSentViaWalker;
    }

    /** Number of Responses sent via Unicast ***/
    public int getNumResponsesSentViaUnicast() {
        return numResponsesSentViaUnicast;
    }

    /** Number of Errors while sending Responses ***/
    public int getNumErrorsSendingResponses() {
        return numErrorsSendingResponses;
    }

    /** Number of Errors while propagating Responses ***/
    public int getNumErrorsPropagatingResponses() {
        return numErrorsPropagatingResponses;
    }

    public int getNumQueriesSent() {
        return (numQueriesSentInGroup + numQueriesSentViaWalker + numQueriesSentViaUnicast);
    }

    public int getNumQuerySendErrors() {
        return (numErrorsSendingQueries + numErrorsPropagatingQueries);
    }

    public int getNumResponsesSent() {
        return (numResponsesSentInGroup + numResponsesSentViaWalker + numResponsesSentViaUnicast);
    }

    public int getNumResponseSendErrors() {
        return (numErrorsSendingResponses + numErrorsPropagatingResponses);
    }
	
    public void addQueryDestinationMetric(QueryDestinationMetric queryDestinationMetric) {
        destinationMetrics.put(queryDestinationMetric.getPeerID(), queryDestinationMetric);
    }
	
    public QueryDestinationMetric getQueryDestinationMetric(PeerID peerID) {
        QueryDestinationMetric destinationMetric = (QueryDestinationMetric) destinationMetrics.get(peerID);
		
        if (destinationMetric == null) {
            destinationMetric = new QueryDestinationMetric(peerID);
            destinationMetrics.put(peerID, destinationMetric);
        }
        return destinationMetric;
    }

    public Iterator getDestinationMetrics() {

        return destinationMetrics.values().iterator();
    }
	
    public void serializeTo(Element element) throws DocumentSerializationException {
        if (handlerName != null) {
            DocumentSerializableUtilities.addString(element, "handlerName", handlerName);
        }
        if (registered != null) {
            DocumentSerializableUtilities.addString(element, "registered", registered);
        }
        if (numResponses != 0) {
            DocumentSerializableUtilities.addInt(element, "numResponses", numResponses);
        }
        if (responseProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "responseProcessingTime", responseProcessingTime);
        }
        if (responseTime != 0) {
            DocumentSerializableUtilities.addLong(element, "responseTime", responseTime);
        }
        if (numResponseErrors != 0) {
            DocumentSerializableUtilities.addInt(element, "numResponseErrors", numResponseErrors);
        }
        if (numQueries != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueries", numQueries);
        }
        if (numQueriesRepropagated != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueriesRepropagated", numQueriesRepropagated);
        }
        if (queryProcessingTime != 0) {
            DocumentSerializableUtilities.addLong(element, "queryProcessingTime", queryProcessingTime);
        }
        if (numQueryErrors != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueryErrors", numQueryErrors);
        }
        if (numQueriesSentInGroup != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueriesSentInGroup", numQueriesSentInGroup);
        }
        if (numQueriesSentViaWalker != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueriesSentViaWalker", numQueriesSentViaWalker);
        }
        if (numQueriesSentViaUnicast != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueriesSentViaUnicast", numQueriesSentViaUnicast);
        }
        if (numErrorsSendingQueries != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsSendingQueries", numErrorsSendingQueries);
        }
        if (numErrorsPropagatingQueries != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsPropagatingQueries", numErrorsPropagatingQueries);
        }
        if (numQueriesHopCountDropped != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueriesHopCountDropped", numQueriesHopCountDropped);
        }
        if (numPropagationQueriesDropped != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagationQueriesDropped", numPropagationQueriesDropped);
        }
        if (numPropagatedInGroup != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagatedInGroup", numPropagatedInGroup);
        }
        if (numPropagatedViaWalker != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagatedViaWalker", numPropagatedViaWalker);
        }
        if (numUnableToPropagate != 0) {
            DocumentSerializableUtilities.addInt(element, "numUnableToPropagate", numUnableToPropagate);
        }
        if (numResponsesToUnregisteredHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "numResponsesToUnregisteredHandler", numResponsesToUnregisteredHandler);
        }
        if (numQueriesToUnregisteredHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueriesToUnregisteredHandler", numQueriesToUnregisteredHandler);
        }
        if (numResponsesSentInGroup != 0) {
            DocumentSerializableUtilities.addInt(element, "numResponsesSentInGroup", numResponsesSentInGroup);
        }
        if (numResponsesSentViaWalker != 0) {
            DocumentSerializableUtilities.addInt(element, "numResponsesSentViaWalker", numResponsesSentViaWalker);
        }
        if (numResponsesSentViaUnicast != 0) {
            DocumentSerializableUtilities.addInt(element, "numResponsesSentViaUnicast", numResponsesSentViaUnicast);
        }
        if (numErrorsSendingResponses != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsSendingResponses", numErrorsSendingResponses);
        }
        if (numErrorsPropagatingResponses != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsPropagatingResponses", numErrorsPropagatingResponses);
        }
	
        for (Iterator i = destinationMetrics.values().iterator(); i.hasNext();) {
            Element queryDestinationElement = DocumentSerializableUtilities.createChildElement(element, "destination");
            QueryDestinationMetric queryDestinationMetric = (QueryDestinationMetric) i.next();

            queryDestinationMetric.serializeTo(queryDestinationElement);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("handlerName")) { 
                handlerName = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("registered")) { 
                registered = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("numResponses")) { 
                numResponses = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("responseProcessingTime")) { 
                responseProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("responseTime")) { 
                responseTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numResponseErrors")) { 
                numResponseErrors = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueries")) { 
                numQueries = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueriesRepropagated")) { 
                numQueriesRepropagated = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("queryProcessingTime")) { 
                queryProcessingTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numQueryErrors")) { 
                numQueryErrors = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueriesSentInGroup")) { 
                numQueriesSentInGroup = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueriesSentViaWalker")) { 
                numQueriesSentViaWalker = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueriesSentViaUnicast")) { 
                numQueriesSentViaUnicast = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsSendingQueries")) { 
                numErrorsSendingQueries = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsPropagatingQueries")) { 
                numErrorsPropagatingQueries = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueriesHopCountDropped")) { 
                numQueriesHopCountDropped = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagationQueriesDropped")) { 
                numPropagationQueriesDropped = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagatedInGroup")) { 
                numPropagatedInGroup = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagatedViaWalker")) { 
                numPropagatedViaWalker = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numUnableToPropagate")) { 
                numUnableToPropagate = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numResponsesToUnregisteredHandler")) { 
                numResponsesToUnregisteredHandler = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueriesToUnregisteredHandler")) { 
                numQueriesToUnregisteredHandler = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numResponsesSentInGroup")) { 
                numResponsesSentInGroup = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numResponsesSentViaWalker")) { 
                numResponsesSentViaWalker = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numResponsesSentViaUnicast")) { 
                numResponsesSentViaUnicast = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsSendingResponses")) { 
                numErrorsSendingResponses = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsPropagatingResponses")) { 
                numErrorsPropagatingResponses = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("destination")) {
                QueryDestinationMetric queryDestinationMetric = new QueryDestinationMetric();

                queryDestinationMetric.initializeFrom(childElement);
                addQueryDestinationMetric(queryDestinationMetric);
            }
        }
    }

    public void mergeMetrics(QueryHandlerMetric otherQueryHandlerMetric) {
        this.numResponses += otherQueryHandlerMetric.numResponses;
        this.responseProcessingTime += otherQueryHandlerMetric.responseProcessingTime;
        this.responseTime += otherQueryHandlerMetric.responseTime;
        this.numResponseErrors += otherQueryHandlerMetric.numResponseErrors;

        this.numQueries += otherQueryHandlerMetric.numQueries;
        this.numQueriesRepropagated += otherQueryHandlerMetric.numQueriesRepropagated;
        this.queryProcessingTime += otherQueryHandlerMetric.queryProcessingTime;
        this.numQueryErrors += otherQueryHandlerMetric.numQueryErrors;

        this.numQueriesSentInGroup += otherQueryHandlerMetric.numQueriesSentInGroup;
        this.numQueriesSentViaWalker += otherQueryHandlerMetric.numQueriesSentViaWalker;
        this.numQueriesSentViaUnicast += otherQueryHandlerMetric.numQueriesSentViaUnicast;
        this.numErrorsSendingQueries += otherQueryHandlerMetric.numErrorsSendingQueries;
        this.numErrorsPropagatingQueries += otherQueryHandlerMetric.numErrorsPropagatingQueries;
        this.numQueriesHopCountDropped += otherQueryHandlerMetric.numQueriesHopCountDropped;

        this.numResponsesSentInGroup += otherQueryHandlerMetric.numResponsesSentInGroup;
        this.numResponsesSentViaWalker += otherQueryHandlerMetric.numResponsesSentViaWalker;
        this.numResponsesSentViaUnicast += otherQueryHandlerMetric.numResponsesSentViaUnicast;
        this.numErrorsSendingResponses += otherQueryHandlerMetric.numErrorsSendingResponses;
        this.numErrorsPropagatingResponses += otherQueryHandlerMetric.numErrorsPropagatingResponses;	

        for (Iterator i = otherQueryHandlerMetric.getDestinationMetrics(); i.hasNext();) {
            QueryDestinationMetric otherQueryDestinationMetric = (QueryDestinationMetric) i.next();
            QueryDestinationMetric ourQueryDestinationMetric = getQueryDestinationMetric(otherQueryDestinationMetric.getPeerID());

            if (ourQueryDestinationMetric == null) {
                ourQueryDestinationMetric = new QueryDestinationMetric(otherQueryDestinationMetric);
                addQueryDestinationMetric(ourQueryDestinationMetric);
            }

            ourQueryDestinationMetric.mergeMetrics(otherQueryDestinationMetric);
        }
    }
}
