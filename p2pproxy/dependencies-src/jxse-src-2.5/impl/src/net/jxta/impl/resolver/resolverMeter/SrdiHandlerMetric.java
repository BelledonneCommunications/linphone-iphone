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


import net.jxta.resolver.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.peer.*;
import net.jxta.impl.meter.*;

import java.util.*;


public class SrdiHandlerMetric implements DocumentSerializable {
    private String handlerName;
    private boolean registered = true;
    private int numProcessed = 0;
    private int numErrorsWhileProcessing = 0;
    private int numToUnregisteredHandler = 0;
	
    private long totalProcessTime = 0;
	
    private int numMessagesSentViaWalker = 0;
    private int numMessagesSentViaUnicast = 0;
    private int numErrorsSendingMessages = 0;
    private int numErrorsPropagatingMessages = 0;
	
    private HashMap destinationMetrics = new HashMap();
		
    public SrdiHandlerMetric(String handlerName) {
        this.handlerName = handlerName;
    }

    public SrdiHandlerMetric() {}

    public SrdiHandlerMetric(SrdiHandlerMetric prototype) {
        this.handlerName = prototype.handlerName;
    }
	
    public void setRegistered(boolean registered) {
        this.registered = registered;
    }

    public String getHandlerName() {
        return handlerName;
    }

    public boolean getRegistered() {
        return registered;
    }

    public int getNumProcessed() {
        return numProcessed;
    }

    public int getNumErrorsWhileProcessing() {
        return numErrorsWhileProcessing;
    }

    public long getTotalProcessTime() {
        return totalProcessTime;
    }

    public int getNumMessagesSentViaWalker() {
        return numMessagesSentViaWalker;
    }

    public int getNumMessagesSentViaUnicast() {
        return numMessagesSentViaUnicast;
    }

    public int getNumErrorsSendingMessages() {
        return numErrorsSendingMessages;
    }

    public int getNumErrorsPropagatingMessages() {
        return numErrorsPropagatingMessages;
    }
	
    @Override
    public boolean equals(Object obj) {
        if (obj instanceof SrdiHandlerMetric) {
            SrdiHandlerMetric otherSrdiHandlerMetric = (SrdiHandlerMetric) obj;

            return handlerName.equals((otherSrdiHandlerMetric.handlerName));
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() { 
        return handlerName.hashCode();
    }

    public void messageProcessed(long processTime) {
        numProcessed++;
        totalProcessTime += processTime;
    }

    public void errorWhileProcessing() {
        numErrorsWhileProcessing++;
    }

    public void srdiToUnregisteredHandler() {
        numToUnregisteredHandler++;
    }	

    public void messageSentViaWalker() {
        numMessagesSentViaWalker++;
    }

    public void messageSentViaUnicast() {
        numMessagesSentViaUnicast++;
    }

    public void errorSendingMessage() {
        numErrorsSendingMessages++;
    }

    public void errorPropagatingMessage() {
        numErrorsPropagatingMessages++;
    }	

    SrdiDestinationMetric getSrdiDestinationMetric(EndpointAddress endpointAddress) {
        PeerID peerID = MetricUtilities.getPeerIdFromEndpointAddress(endpointAddress);

        return getSrdiDestinationMetric(peerID);
    }
		
    SrdiDestinationMetric getSrdiDestinationMetric(PeerID peerID) {
		
        SrdiDestinationMetric destinationMetric = (SrdiDestinationMetric) destinationMetrics.get(peerID);
		
        if (destinationMetric == null) {
            destinationMetric = new SrdiDestinationMetric(peerID);
            destinationMetrics.put(peerID, destinationMetric);
        }
        return destinationMetric;
    }

    public Iterator getDestinationMetrics() {

        return destinationMetrics.values().iterator();
    }

    public void addSrdiDestinationMetric(SrdiDestinationMetric srdiDestinationMetric) {
        destinationMetrics.put(srdiDestinationMetric.getPeerID(), srdiDestinationMetric);
    }

    public void serializeTo(Element element) throws DocumentSerializationException {
        if (handlerName != null) {
            DocumentSerializableUtilities.addString(element, "handlerName", handlerName);
        }
        DocumentSerializableUtilities.addBoolean(element, "registered", registered);
        if (numProcessed != 0) {
            DocumentSerializableUtilities.addInt(element, "numProcessed", numProcessed);
        }
        if (numErrorsWhileProcessing != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsWhileProcessing", numErrorsWhileProcessing);
        }
        if (numToUnregisteredHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "numToUnregisteredHandler", numToUnregisteredHandler);
        }
        if (totalProcessTime != 0) {
            DocumentSerializableUtilities.addLong(element, "totalProcessTime", totalProcessTime);
        }
        if (numMessagesSentViaWalker != 0) {
            DocumentSerializableUtilities.addInt(element, "numMessagesSentViaWalker", numMessagesSentViaWalker);
        }
        if (numMessagesSentViaUnicast != 0) {
            DocumentSerializableUtilities.addInt(element, "numMessagesSentViaUnicast", numMessagesSentViaUnicast);
        }
        if (numErrorsSendingMessages != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsSendingMessages", numErrorsSendingMessages);
        }
        if (numErrorsPropagatingMessages != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsPropagatingMessages", numErrorsPropagatingMessages);
        }

        for (Iterator i = destinationMetrics.values().iterator(); i.hasNext();) {
            Element srdiDestinationElement = DocumentSerializableUtilities.createChildElement(element, "destination");
            SrdiDestinationMetric srdiDestinationMetric = (SrdiDestinationMetric) i.next();

            srdiDestinationMetric.serializeTo(srdiDestinationElement);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("handlerName")) { 
                handlerName = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("registered")) { 
                registered = DocumentSerializableUtilities.getBoolean(childElement);
            } else if (tagName.equals("numProcessed")) { 
                numProcessed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsWhileProcessing")) { 
                numErrorsWhileProcessing = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numToUnregisteredHandler")) { 
                numToUnregisteredHandler = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("totalProcessTime")) { 
                totalProcessTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numMessagesSentViaWalker")) { 
                numMessagesSentViaWalker = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numMessagesSentViaUnicast")) { 
                numMessagesSentViaUnicast = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsSendingMessages")) { 
                numErrorsSendingMessages = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsPropagatingMessages")) { 
                numErrorsPropagatingMessages = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("destination")) {
                SrdiDestinationMetric srdiDestinationMetric = new SrdiDestinationMetric();

                srdiDestinationMetric.initializeFrom(childElement);
                addSrdiDestinationMetric(srdiDestinationMetric);
            }
        }
    }

    public void mergeMetrics(SrdiHandlerMetric otherSrdiHandlerMetric) {
        numProcessed += otherSrdiHandlerMetric.numProcessed;
        numErrorsWhileProcessing += otherSrdiHandlerMetric.numErrorsWhileProcessing;
        totalProcessTime += otherSrdiHandlerMetric.totalProcessTime;
        numMessagesSentViaWalker += otherSrdiHandlerMetric.numMessagesSentViaWalker;
        numMessagesSentViaUnicast += otherSrdiHandlerMetric.numMessagesSentViaUnicast;
        numErrorsSendingMessages += otherSrdiHandlerMetric.numErrorsSendingMessages;
        numErrorsPropagatingMessages += otherSrdiHandlerMetric.numErrorsPropagatingMessages;
        for (Iterator i = otherSrdiHandlerMetric.getDestinationMetrics(); i.hasNext();) {
            SrdiDestinationMetric otherSrdiDestinationMetric = (SrdiDestinationMetric) i.next();
            SrdiDestinationMetric ourSrdiDestinationMetric = getSrdiDestinationMetric(otherSrdiDestinationMetric.getPeerID());

            if (ourSrdiDestinationMetric == null) {
                ourSrdiDestinationMetric = new SrdiDestinationMetric(otherSrdiDestinationMetric);
                addSrdiDestinationMetric(ourSrdiDestinationMetric);
            }
				
            ourSrdiDestinationMetric.mergeMetrics(otherSrdiDestinationMetric);
        }
    }
}
