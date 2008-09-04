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


import net.jxta.peer.*;
import net.jxta.endpoint.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.util.*;
import net.jxta.impl.meter.*;

import java.util.*;


/**
 * Metrics for a specific Query Destination for a specific handler 
 **/
public class QueryDestinationMetric implements DocumentSerializable {
    private PeerID peerID;
    private int errorWhileProcessingQuery;
    private int queryProcessed;
    private int errorWhileProcessingResponse;
    private int responseProcessed;
    private int responseSentViaUnicast;
    private int querySentViaUnicast;

    private int queryToUnregisteredHandler;
    private int responseToUnregisteredHandler;
	
    public QueryDestinationMetric(PeerID pid) {
        this.peerID = pid;
    }

    public QueryDestinationMetric(QueryDestinationMetric prototype) {
        this.peerID = prototype.peerID;	
    }

    public QueryDestinationMetric() {}

    public PeerID getPeerID() {
        return peerID;
    }

    void querySentViaUnicast() {
        querySentViaUnicast++;
    }

    /** Get Queries Sent via Unicast to this destinations **/
    public int getQueriesSentViaUnicast() {
        return querySentViaUnicast;
    }
	
    void responseSentViaUnicast() {
        responseSentViaUnicast++;
    }

    /** Get Responses Sent via Unicast to this destinations **/
    public int getResponsesSentViaUnicast() {
        return responseSentViaUnicast;
    }

    void responseToUnregisteredHandler() {
        responseToUnregisteredHandler++;
    }

    /** Get Responses Recieved to this handler when not registered **/
    public int getResponseToUnregisteredHandler() {
        return responseToUnregisteredHandler;
    }
		
    void responseProcessed() {
        responseProcessed++;
    }

    /** Get Responses received and processed locally **/
    public int getResponsesProcessed() {
        return responseProcessed;
    }
	
    void errorWhileProcessingResponse() {
        errorWhileProcessingResponse++;
    }

    /** Get Responses received but failing when processed locally **/
    public int getErrorsWhileProcessingResponse() {
        return errorWhileProcessingResponse;
    }
	
    void queryProcessed() {
        queryProcessed++;
    }

    /** Get Queries received and processed locally **/
    public int getQueriesProcessed() {
        return queryProcessed;
    }

    void queryToUnregisteredHandler() {
        queryToUnregisteredHandler++;
    }

    /** Get Queries Recieved to this handler when not registered **/
    public int getQueryToUnregisteredHandler() {
        return queryToUnregisteredHandler;
    }
	
    void errorWhileProcessingQuery() {
        errorWhileProcessingQuery++;
    }

    /** Get Queries received but failing when processed locally **/
    public int getErrorsWhileProcessingQuery() {
        return errorWhileProcessingQuery;
    }

    public void serializeTo(Element element) throws DocumentSerializationException {
        if (peerID != null) {
            DocumentSerializableUtilities.addString(element, "peerID", peerID.toString());
        }
        if (errorWhileProcessingQuery != 0) {
            DocumentSerializableUtilities.addInt(element, "errorWhileProcessingQuery", errorWhileProcessingQuery);
        }
        if (queryProcessed != 0) {
            DocumentSerializableUtilities.addInt(element, "queryProcessed", queryProcessed);
        }
        if (errorWhileProcessingResponse != 0) {
            DocumentSerializableUtilities.addInt(element, "errorWhileProcessingResponse", errorWhileProcessingResponse);
        }
        if (responseProcessed != 0) {
            DocumentSerializableUtilities.addInt(element, "responseProcessed", responseProcessed);
        }
        if (responseSentViaUnicast != 0) {
            DocumentSerializableUtilities.addInt(element, "responseSentViaUnicast", responseSentViaUnicast);
        }
        if (querySentViaUnicast != 0) {
            DocumentSerializableUtilities.addInt(element, "querySentViaUnicast", querySentViaUnicast);
        }
        if (queryToUnregisteredHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "queryToUnregisteredHandler", queryToUnregisteredHandler);
        }
        if (responseToUnregisteredHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "responseToUnregisteredHandler", responseToUnregisteredHandler);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("peerID")) {
                String peerIDText = DocumentSerializableUtilities.getString(childElement);

                peerID = MetricUtilities.getPeerIdFromString(peerIDText);
            } else if (tagName.equals("errorWhileProcessingQuery")) { 
                errorWhileProcessingQuery = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("queryProcessed")) { 
                queryProcessed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("errorWhileProcessingResponse")) { 
                errorWhileProcessingResponse = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("responseProcessed")) { 
                responseProcessed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("responseSentViaUnicast")) { 
                responseSentViaUnicast = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("querySentViaUnicast")) { 
                querySentViaUnicast = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("queryToUnregisteredHandler")) { 
                queryToUnregisteredHandler = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("responseToUnregisteredHandler")) { 
                responseToUnregisteredHandler = DocumentSerializableUtilities.getInt(childElement);
            }
        }
    }

    public void mergeMetrics(QueryDestinationMetric otherQueryDestinationMetric) {
        this.errorWhileProcessingQuery += otherQueryDestinationMetric.errorWhileProcessingQuery;
        this.queryProcessed += otherQueryDestinationMetric.queryProcessed;
        this.errorWhileProcessingResponse += otherQueryDestinationMetric.errorWhileProcessingResponse;
        this.responseProcessed += otherQueryDestinationMetric.responseProcessed;
        this.responseSentViaUnicast += otherQueryDestinationMetric.responseSentViaUnicast;
        this.querySentViaUnicast += otherQueryDestinationMetric.querySentViaUnicast;
        this.queryToUnregisteredHandler += otherQueryDestinationMetric.queryToUnregisteredHandler;
        this.responseToUnregisteredHandler += otherQueryDestinationMetric.responseToUnregisteredHandler;
    }

    @Override
    public int hashCode() {
        return peerID.hashCode();
    }
	
    @Override
    public boolean equals(Object other) {
        if (other instanceof QueryDestinationMetric) {
            QueryDestinationMetric otherQueryDestinationMetric = (QueryDestinationMetric) other;

            return peerID.equals(otherQueryDestinationMetric.peerID);
        } else {
            return false;
        }
    }	
}
