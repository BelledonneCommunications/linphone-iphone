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
 *    Metric corresponding to a message queue to for outbound messengers based upon an endpoint address
 **/
public class OutboundMetric implements DocumentSerializable {
    private EndpointAddress endpointAddress;

    private int numOutboundQueued;
    private int numOutboundDropped;
    private long timeToDropOutbound;
    private int numOutboundDeQueued;
    private long timeInOutboundQueue;
    private int numOutboundProcessed;
    private long timeToProcessOutbound;
    private int numOutboundFailed;
    private long timeOutboundToFail;

    public OutboundMetric(OutboundMeter outboundMeter) {
        this.endpointAddress = outboundMeter.getEndpointAddress();
    }

    public OutboundMetric(OutboundMetric prototype) {
        this.endpointAddress = prototype.getEndpointAddress();	
    }

    public OutboundMetric() {} 

    /** The Endpoint address for this outbound message queue **/
    public EndpointAddress getEndpointAddress() {
        return endpointAddress;
    }
	
    /** The Number of Outbound Messages Queued **/
    public int getNumOutboundQueued() {
        return numOutboundQueued;
    }

    /** The Number of Outbound Messages Dropped from Queue **/
    public int getNumOutboundDropped() {
        return numOutboundDropped;
    }

    /** The Sum of the times in queue for all dropped messages  **/
    public long getTimeToDropOutbound() {
        return timeToDropOutbound;
    }

    /** The Number of Outbound Messages DeQueued **/
    public int getNumOutboundDeQueued() {
        return numOutboundDeQueued;
    }

    /** The Sum of the times in queue for all messages  **/
    public long getTimeInOutboundQueue() {
        return timeInOutboundQueue;
    }

    /** The Number of Outbound Messages Processed Successfully **/
    public int getNumOutboundProcessed() {
        return numOutboundProcessed;
    }

    /** The Sum of the times from sending to handling by messenger **/
    public long getTimeToProcessOutbound() {
        return timeToProcessOutbound;
    }

    /** The Number of Outbound Messages Failed in sending **/
    public int getNumOutboundFailed() {
        return numOutboundFailed;
    }

    /** The Sum of the times in queue for all failed messages  **/
    public long getTimeOutboundToFail() {
        return timeOutboundToFail;
    }

    /** The Average of the times in queue for all messages  **/
    public long getAverageTimeInOutboundQueue() {
        return  (numOutboundDeQueued == 0) ? 0 : (timeInOutboundQueue / numOutboundDeQueued);
    }

    /** The Average of the times in queue for all dropped messages  **/
    public long getAverageOutboundDropTime() {
        return  (numOutboundDropped == 0) ? 0 : (timeToDropOutbound / numOutboundDropped);
    }

    /** The Average of the times from sending to handling by messenger **/
    public long getAverageOutboundProcessTime() {
        return  (numOutboundProcessed == 0) ? 0 : (timeToProcessOutbound / numOutboundProcessed);
    }
	
    @Override
    public boolean equals(Object obj) {
		
        if (obj instanceof OutboundMetric) {
            OutboundMetric other = (OutboundMetric) obj;
			
            return endpointAddress.equals(other.endpointAddress);
        } else {
            return false;
        }
    }

    public boolean matches(EndpointAddress otherAddress) {
        return getEndpointAddress().equals(otherAddress);
    }

    @Override
    public int hashCode() {
        return endpointAddress.hashCode();
    }	 

    void outboundMessageQueued(Message message) {
        numOutboundQueued++;
    }

    void outboundMessageDropped(Message message, long time) {
        numOutboundDropped++;
        timeToDropOutbound += time;
    }

    void outboundMessageFailed(Message message, long time) {
        numOutboundFailed++;
        timeOutboundToFail += time;
    }

    void outboundMessageDeQueued(Message message, long time) {
        numOutboundDeQueued++;
        timeInOutboundQueue += time;
    }

    void outboundMessageProcessed(Message message, long time) {	
        numOutboundProcessed++;
        timeToProcessOutbound += time;
    }

    public void mergeMetrics(OutboundMetric other) {
	
        numOutboundQueued += other.numOutboundQueued;
        numOutboundDropped += other.numOutboundDropped;
        timeToDropOutbound += other.timeToDropOutbound;
        numOutboundDeQueued += other.numOutboundDeQueued;
        timeInOutboundQueue += other.timeInOutboundQueue;
        numOutboundProcessed += other.numOutboundProcessed;
        timeToProcessOutbound += other.timeToProcessOutbound;
        numOutboundFailed += other.numOutboundFailed;
        timeOutboundToFail += other.timeOutboundToFail;
    }	

    public void serializeTo(Element element) throws DocumentSerializationException {

        DocumentSerializableUtilities.addString(element, "endpointAddress", endpointAddress.toString());

        if (numOutboundQueued != 0) {
            DocumentSerializableUtilities.addInt(element, "numOutboundQueued", numOutboundQueued);
        }
				
        if (numOutboundDropped != 0) {
            DocumentSerializableUtilities.addInt(element, "numOutboundDropped", numOutboundDropped);
        }

        if (timeToDropOutbound != 0) {
            DocumentSerializableUtilities.addLong(element, "timeToDropOutbound", timeToDropOutbound);
        }
		
        if (numOutboundDeQueued != 0) {
            DocumentSerializableUtilities.addInt(element, "numOutboundDeQueued", numOutboundDeQueued);
        }
		
        if (timeInOutboundQueue != 0) {
            DocumentSerializableUtilities.addLong(element, "timeInOutboundQueue", timeInOutboundQueue);
        }
		
        if (numOutboundProcessed != 0) {
            DocumentSerializableUtilities.addInt(element, "numOutboundProcessed", numOutboundProcessed);
        }
				
        if (timeToProcessOutbound != 0) {
            DocumentSerializableUtilities.addLong(element, "timeToProcessOutbound", timeToProcessOutbound);
        }
		
        if (numOutboundFailed != 0) {
            DocumentSerializableUtilities.addInt(element, "numOutboundFailed", numOutboundFailed);
        }

        if (timeOutboundToFail != 0) {
            DocumentSerializableUtilities.addLong(element, "timeOutboundToFail", timeOutboundToFail);
        }
				
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
			
            if (tagName.equals("endpointAddress")) {
                String endpointAddressString = DocumentSerializableUtilities.getString(childElement);	

                endpointAddress = new EndpointAddress(endpointAddressString);
            } else if (tagName.equals("numOutboundQueued")) {
                numOutboundQueued = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numOutboundDropped")) {
                numOutboundDropped = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("timeToDropOutbound")) {
                timeToDropOutbound = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numOutboundDeQueued")) {
                numOutboundDeQueued = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("timeInOutboundQueue")) {
                timeInOutboundQueue = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numOutboundProcessed")) {
                numOutboundProcessed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("timeToProcessOutbound")) {
                timeToProcessOutbound = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numOutboundFailed")) {
                numOutboundFailed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("timeOutboundToFail")) {
                timeOutboundToFail = DocumentSerializableUtilities.getLong(childElement);
            }
        }
    }
}
