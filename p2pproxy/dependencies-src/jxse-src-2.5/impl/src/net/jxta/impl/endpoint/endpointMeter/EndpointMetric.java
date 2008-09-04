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
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;

import java.util.*;


/**
 *    Aggregate Metric for Endpoint Monitoring
 **/
public class EndpointMetric implements DocumentSerializable {
    private long endpointStartTime;
    private long endpointUpTime;
    private int invalidIncomingMessage;
    private int noListenerForIncomingMessage;
    private int errorProcessingIncomingMessage;
    private int noDestinationAddressForDemuxMessage;
    private int noSourceAddressForDemuxMessage;
    private int discardedLoopbackDemuxMessage;
    private int incomingMessageFilteredOut;
    private int incomingMessageSentToEndpointListener;
    private int demuxMessageProcessed;
	
    public EndpointMetric() { 
        endpointStartTime = System.currentTimeMillis();
    }

    public EndpointMetric(EndpointMetric prototype) { 
        endpointStartTime = prototype.endpointStartTime;
    }
	
    void invalidIncomingMessage() {
        invalidIncomingMessage++;
    }

    void noListenerForIncomingMessage() {
        noListenerForIncomingMessage++;
    }

    void errorProcessingIncomingMessage() {
        errorProcessingIncomingMessage++;
    }

    void noDestinationAddressForDemuxMessage() {
        noDestinationAddressForDemuxMessage++;
    }

    void noSourceAddressForDemuxMessage() {
        noSourceAddressForDemuxMessage++;
    }

    void discardedLoopbackDemuxMessage() {
        discardedLoopbackDemuxMessage++;
    }

    void incomingMessageFilteredOut() {
        incomingMessageFilteredOut++;
    }

    void incomingMessageSentToEndpointListener() {
        incomingMessageSentToEndpointListener++;
    }

    void demuxMessageProcessed() {
        demuxMessageProcessed++;
    }

    void setEndpointUpTime(long endpointUpTime) {
        this.endpointUpTime = endpointUpTime;
    }

    /** Get the time this Endpoint was created, essentially the boot time of the PeerGroup **/
    public long getEndpointStartTime() {
        return endpointStartTime;
    }

    /** Get the time this Endpoint has been up **/
    public long getEndpointUpTime() {
        return endpointUpTime;
    }

    /** The number of messages received that had invalid formats **/
    public int getInvalidIncomingMessage() {
        return invalidIncomingMessage;
    }

    /** The number of messages received that had no listeners **/
    public int getNoListenerForIncomingMessage() {
        return noListenerForIncomingMessage;
    }

    /** The number of messages whose local listeners threw exceptions **/
    public int getErrorProcessingIncomingMessage() {
        return errorProcessingIncomingMessage;
    }

    /** The number of messages that couldn't be demuxed because there was no destination address **/
    public int getNoDestinationAddressForDemuxMessage() {
        return noDestinationAddressForDemuxMessage;
    }

    /** The number of messages that couldn't be demuxed because there was no source address **/
    public int getNoSourceAddressForDemuxMessage() {
        return noSourceAddressForDemuxMessage;
    }

    /** The number of messages that were discarded because of loopback detection **/
    public int getDiscardedLoopbackDemuxMessage() {
        return discardedLoopbackDemuxMessage;
    }

    /** The number of messages that were discarded because of filtering **/
    public int getIncomingMessageFilteredOut() {
        return incomingMessageFilteredOut;
    }

    /** The number of messages that sent to registered listeners **/
    public int getIncomingMessageSentToEndpointListener() {
        return incomingMessageSentToEndpointListener;
    }

    /** The number of messages that were processed through demux **/
    public int getDemuxMessageProcessed() {
        return demuxMessageProcessed;
    }
		
    public void serializeTo(Element element) throws DocumentSerializationException {
        if (endpointStartTime != 0) {
            DocumentSerializableUtilities.addLong(element, "endpointStartTime", endpointStartTime);
        }
        if (endpointUpTime != 0) {
            DocumentSerializableUtilities.addLong(element, "endpointUpTime", endpointUpTime);
        }
        if (invalidIncomingMessage != 0) {
            DocumentSerializableUtilities.addInt(element, "invalidIncomingMessage", invalidIncomingMessage);
        }
        if (noListenerForIncomingMessage != 0) {
            DocumentSerializableUtilities.addInt(element, "noListenerForIncomingMessage", noListenerForIncomingMessage);
        }
        if (errorProcessingIncomingMessage != 0) { 	
            DocumentSerializableUtilities.addInt(element, "errorProcessingIncomingMessage", errorProcessingIncomingMessage);
        }
        if (noDestinationAddressForDemuxMessage != 0) {
            DocumentSerializableUtilities.addInt(element, "noDestinationAddressForDemuxMessage"
                    ,
                    noDestinationAddressForDemuxMessage);
        }
        if (noSourceAddressForDemuxMessage != 0) {
            DocumentSerializableUtilities.addInt(element, "noSourceAddressForDemuxMessage", noSourceAddressForDemuxMessage);
        }
        if (discardedLoopbackDemuxMessage != 0) {
            DocumentSerializableUtilities.addInt(element, "discardedLoopbackDemuxMessage", discardedLoopbackDemuxMessage);
        }
        if (incomingMessageFilteredOut != 0) {
            DocumentSerializableUtilities.addInt(element, "incomingMessageFilteredOut", incomingMessageFilteredOut);
        }
        if (incomingMessageSentToEndpointListener != 0) {
            DocumentSerializableUtilities.addInt(element, "incomingMessageSentToEndpointListener"
                    ,
                    incomingMessageSentToEndpointListener);
        }
        if (demuxMessageProcessed != 0) {
            DocumentSerializableUtilities.addInt(element, "demuxMessageProcessed", demuxMessageProcessed);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
			
            if (tagName.equals("endpointStartTime")) {
                endpointStartTime = DocumentSerializableUtilities.getLong(childElement);
            }
            if (tagName.equals("endpointUpTime")) {
                endpointUpTime = DocumentSerializableUtilities.getLong(childElement);
            }
            if (tagName.equals("invalidIncomingMessage")) {
                invalidIncomingMessage = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("noListenerForIncomingMessage")) {
                noListenerForIncomingMessage = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("errorProcessingIncomingMessage")) {
                errorProcessingIncomingMessage = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("noDestinationAddressForDemuxMessage")) {
                noDestinationAddressForDemuxMessage = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("noSourceAddressForDemuxMessage")) {
                noSourceAddressForDemuxMessage = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("invalidIncomingMessage")) {
                invalidIncomingMessage = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("discardedLoopbackDemuxMessage")) {
                discardedLoopbackDemuxMessage = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("incomingMessageFilteredOut")) {
                incomingMessageFilteredOut = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("incomingMessageSentToEndpointListener")) {
                incomingMessageSentToEndpointListener = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("demuxMessageProcessed")) {
                demuxMessageProcessed = DocumentSerializableUtilities.getInt(childElement);
            }
        }

    }

    public void mergeMetrics(EndpointMetric other) {
        if (other == null) {
            return;
        }

        endpointStartTime = other.endpointStartTime;

        if (other.endpointUpTime != 0) {
            endpointUpTime = other.endpointUpTime;
        }
			
        invalidIncomingMessage += other.invalidIncomingMessage;
        noListenerForIncomingMessage += other.noListenerForIncomingMessage;
        errorProcessingIncomingMessage += other.errorProcessingIncomingMessage;
        noDestinationAddressForDemuxMessage += other.noDestinationAddressForDemuxMessage;
        noSourceAddressForDemuxMessage += other.noSourceAddressForDemuxMessage;
        discardedLoopbackDemuxMessage += other.discardedLoopbackDemuxMessage;
        incomingMessageSentToEndpointListener += other.incomingMessageSentToEndpointListener;
        demuxMessageProcessed += other.demuxMessageProcessed;		
    }
}
