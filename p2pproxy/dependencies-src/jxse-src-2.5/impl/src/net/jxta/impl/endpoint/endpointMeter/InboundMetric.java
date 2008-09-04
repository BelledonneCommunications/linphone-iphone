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

package net.jxta.impl.endpoint.endpointMeter;


import net.jxta.endpoint.*;
import net.jxta.impl.endpoint.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;

import java.util.*;


/**
 *    Meter corresponding to inbound queue for registered EndpointListeners ServiceName/ServiceParam pair
 **/
public class InboundMetric implements DocumentSerializable {
    private String serviceName;
    private String serviceParameter;

    private String serviceIdString; // internally used to speed up Hashing

    private int numInboundQueued;
    private int numInboundDropped;
    private long timeToDropInbound;
    private int numInboundDeQueued;
    private long timeInInboundQueue;
    private int numInboundProcessed;
    private long timeToProcessInbound;

    public InboundMetric(InboundMeter inboundMeter) {
        this.serviceName = inboundMeter.getServiceName();
        this.serviceParameter = inboundMeter.getServiceParameter();

        serviceIdString = serviceName + serviceParameter;		
    }

    public InboundMetric(InboundMetric prototype) {
        this.serviceName = prototype.getServiceName();
        this.serviceParameter = prototype.getServiceParameter();

        serviceIdString = serviceName + serviceParameter;		
    }

    public InboundMetric() {} 

    /** The Service Name for this Metric **/
    public String getServiceName() {
        return serviceName;
    }

    /** The Service Parameter for this Metric **/
    public String getServiceParameter() {
        return serviceParameter;
    }
	
    /** The Number of Inbound Messages Queued **/
    public int getNumInboundQueued() {
        return numInboundQueued;
    }

    /** The Number of Inbound Messages Dropped **/
    public int getNumInboundDropped() {
        return numInboundDropped;
    }

    /** The Sum of time for all dropped messages from queue **/
    public long getTimeToDropInbound() {
        return timeToDropInbound;
    }

    /** The Number of Inbound Messages Dequeued **/
    public int getNumInboundDeQueued() {
        return numInboundDeQueued;
    }

    /** The Sum of time in queue for messages in queue **/
    public long getTimeInInboundQueue() {
        return timeInInboundQueue;
    }

    public int getNumInboundProcessed() {
        return numInboundProcessed;
    }

    /** The Sum of time for local listeners to process messages **/
    public long getTimeToProcessInbound() {
        return timeToProcessInbound;
    }
	
    /** The Average of time in queue for messages **/
    public long getAverageTimeInInboundQueue() {
        return  (numInboundDeQueued == 0) ? 0 : (timeInInboundQueue / numInboundDeQueued);
    }

    /** The Average of time in queue for dropped messages **/
    public long getAverageInboundDropTime() {
        return   (numInboundDropped == 0) ? 0 : (timeToDropInbound / numInboundDropped);
    }

    /** The Average clock time for local listeners to process messages **/
    public long getAverageInboundProcessTime() {
        return  (numInboundProcessed == 0) ? 0 : (timeToProcessInbound / numInboundProcessed);
    }

    @Override
    public boolean equals(Object obj) {
		
        if (obj instanceof InboundMetric) {
            InboundMetric other = (InboundMetric) obj;
			
            return serviceIdString.equals(other.serviceIdString);
        } else {
            return false;
        }
    }

    public boolean matches(String serviceName, String serviceParam) {
        if (serviceName.equals(getServiceName())) {
            if (serviceParam == null && getServiceParameter() == null) {
                return true;
            } else if (serviceParam != null && getServiceParameter() != null) {
                return serviceParam.equals(getServiceParameter());
            }
        }
        return false;
    }

    @Override
    public int hashCode() {
        return serviceIdString.hashCode();
    }
	
    void inboundMessageQueued(Message message) {
        numInboundQueued++;
    }

    void inboundMessageDropped(Message message, long time) {
        numInboundDropped++;
        timeToDropInbound += time;
    }

    void inboundMessageDeQueued(Message message, long time) {
        numInboundDeQueued++;
        timeInInboundQueue += time;
    }

    void inboundMessageProcessed(Message message, long time) {
        numInboundProcessed++;
        timeToProcessInbound += time;
    }
	
    public void mergeMetrics(InboundMetric other) {
        numInboundQueued += other.numInboundQueued;
        numInboundDropped += other.numInboundDropped;
        timeToDropInbound += other.timeToDropInbound;
        numInboundDeQueued += other.numInboundDeQueued;
        timeInInboundQueue += other.timeInInboundQueue;
        numInboundProcessed += other.numInboundProcessed;
        timeToProcessInbound += other.timeToProcessInbound;
    }	

    public void serializeTo(Element element) throws DocumentSerializationException {

        DocumentSerializableUtilities.addString(element, "serviceName", serviceName);
        DocumentSerializableUtilities.addString(element, "serviceParam", serviceParameter);

        if (numInboundQueued != 0) {
            DocumentSerializableUtilities.addInt(element, "numInboundQueued", numInboundQueued);
        }
		
        if (numInboundDropped != 0) {
            DocumentSerializableUtilities.addInt(element, "numInboundDropped", numInboundDropped);
        }
		
        if (timeToDropInbound != 0) {
            DocumentSerializableUtilities.addLong(element, "timeToDropInbound", timeToDropInbound);
        }
		
        if (numInboundDeQueued != 0) {
            DocumentSerializableUtilities.addInt(element, "numInboundDeQueued", numInboundDeQueued);
        }
		
        if (timeInInboundQueue != 0) {
            DocumentSerializableUtilities.addLong(element, "timeInInboundQueue", timeInInboundQueue);
        }
		
        if (numInboundProcessed != 0) {
            DocumentSerializableUtilities.addInt(element, "numInboundProcessed", numInboundProcessed);
        }
		
        if (timeToProcessInbound != 0) {
            DocumentSerializableUtilities.addLong(element, "timeToProcessInbound", timeToProcessInbound);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
			
            if (tagName.equals("serviceName")) { 
                serviceName = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("serviceParam")) { 
                serviceParameter = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("numInboundQueued")) {
                numInboundQueued = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numInboundDropped")) {
                numInboundDropped = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("timeToDropInbound")) {
                timeToDropInbound = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numInboundDeQueued")) {
                numInboundDeQueued = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("timeInInboundQueue")) {
                timeInInboundQueue = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numInboundProcessed")) {
                numInboundProcessed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("timeToProcessInbound")) {
                timeToProcessInbound = DocumentSerializableUtilities.getLong(childElement);
            }
        }

        serviceIdString = serviceName + serviceParameter;		
    }
}
