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
 *    Metric corresponding to a propagated messages 
 *    Meter corresponding to propagated to a ServiceName/ServiceParam pair
 **/
public class PropagationMetric implements DocumentSerializable {
    private String serviceName;
    private String serviceParameter;

    private String serviceIdString; // for Hashing
    int numPropagations;
    int numPropagatedTo;
    int numFilteredOut;
    int numErrorsPropagated;
    long propagationTime;

    public PropagationMetric() {
        serviceIdString = serviceName + serviceParameter;		
    }

    public PropagationMetric(PropagationMeter propagationMeter) {
        this.serviceName = propagationMeter.getServiceName();
        this.serviceParameter = propagationMeter.getServiceParameter();

        serviceIdString = serviceName + serviceParameter;		
    }

    public PropagationMetric(PropagationMetric prototype) {
        this.serviceName = prototype.getServiceName();
        this.serviceParameter = prototype.getServiceParameter();

        serviceIdString = serviceName + serviceParameter;		
    }

    void registerPropagateMessageStats(int numPropagatedTo, int numFilteredOut, int numErrorsPropagated, long propagationTime) {
        this.numPropagations++;
        this.numPropagatedTo += numPropagatedTo;
        this.numFilteredOut += numFilteredOut;
        this.numErrorsPropagated += numErrorsPropagated;
        this.propagationTime += propagationTime;
    }

    /** The Endpoint address for this outbound message queue **/
    public String getServiceName() {
        return serviceName;
    }

    /** The Endpoint address for this outbound message queue **/
    public String getServiceParameter() {
        return serviceParameter;
    }

    /** The Number of Propagated Messages  **/
    public int getNumPropagations() {
        return numPropagations;
    }

    /** Total number of transports messagess were propagated to  **/
    public int getNumPropagatedTo() {
        return numPropagatedTo;
    }

    /** The Number of Filtered out  Messages  **/
    
    /** The Average of number of Transports propagated To from propagation to transport **/
    public int getAverageNumTransports() {
        return (numPropagatedTo == 0) ? 0 : (numPropagations / numPropagatedTo);
    }

    public int getNumFilteredOut() {
        return numFilteredOut;
    }

    /** The Number of Errors propagating Messages  **/
    public int getNumErrorsPropagated() {
        return numErrorsPropagated;
    }

    /** The Sum of (clock) times from propagation to transport **/
    public long getPropagationTime() {
        return propagationTime;
    }

    /** The Average of (clock) times from propagation to transport **/
    public long getAveragePropagationTime() {
        return (numPropagatedTo == 0) ? 0 : (propagationTime / numPropagatedTo);
    }

    @Override
    public boolean equals(Object obj) {
		
        if (obj instanceof PropagationMetric) {
            PropagationMetric other = (PropagationMetric) obj;
			
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

    String getServiceIdString() {
        return serviceIdString;
    }	

    public void mergeMetrics(PropagationMetric other) {
        numPropagatedTo += other.numPropagatedTo;
        numFilteredOut += other.numFilteredOut;
        numErrorsPropagated += other.numErrorsPropagated;
        propagationTime += other.propagationTime;
    }	

    public void serializeTo(Element element) throws DocumentSerializationException {

        DocumentSerializableUtilities.addString(element, "serviceName", serviceName);
        DocumentSerializableUtilities.addString(element, "serviceParam", serviceParameter);

        if (numPropagations != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagations", numPropagatedTo);
        }
		
        if (numPropagatedTo != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagatedTo", numPropagatedTo);
        }
		
        if (numFilteredOut != 0) {
            DocumentSerializableUtilities.addInt(element, "numFilteredOut", numFilteredOut);
        }
		
        if (numErrorsPropagated != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsPropagated", numErrorsPropagated);
        }
		
        if (propagationTime != 0) {
            DocumentSerializableUtilities.addLong(element, "propagationTime", propagationTime);
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
            } else if (tagName.equals("numPropagations")) {
                numPropagations = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagatedTo")) {
                numPropagatedTo = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFilteredOut")) {
                numFilteredOut = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("propagationTime")) {
                propagationTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numErrorsPropagated")) {
                numErrorsPropagated = DocumentSerializableUtilities.getInt(childElement);
            }
        }

        serviceIdString = serviceName + serviceParameter;		
    }
}
