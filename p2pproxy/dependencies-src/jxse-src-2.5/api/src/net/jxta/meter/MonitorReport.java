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

package net.jxta.meter;


import java.net.URI;
import java.net.URISyntaxException;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.id.*;
import net.jxta.peer.*;
import net.jxta.platform.*;
import net.jxta.util.*;
import net.jxta.exception.*;

import java.util.*;


/**
 *		A Monitor Report contains  service-specific metrics for each service specified in the
 *		corresponding MonitorFilter (provided when the report was requested).
 *
 **/
public class MonitorReport implements DocumentSerializable {
    private long toTime;
    private long fromTime;
    private boolean isCumulative;
    private Map<ModuleClassID,ServiceMetric> serviceMetrics = new HashMap<ModuleClassID,ServiceMetric>();
    private List<ModuleClassID> unknownModuleClassIDs;
    
    /**
     * Monitor Reports are generally not created by applications, but by the Monitor or PeerInfoService
     *
     **/
    public MonitorReport() {}
    
    /**
     * Monitor Reports are generally not created by applications, but by the Monitor or PeerInfoService
     *
     * @param fromTime
     * @param toTime
     * @param isCumulative
     **/
    public MonitorReport(long fromTime, long toTime, boolean isCumulative) {
        this.fromTime = fromTime;
        this.toTime = toTime;
        this.isCumulative = isCumulative;
    }
    
    /**
     * Begin time that this report is representing
     **/
    public long getFromTime() {
        return fromTime;
    }
    
    /**
     * End time that this report is representing
     *
     * @return long
     **/
    public long getToTime() {
        return toTime;
    }
    
    /**
     * Does this report contain metrics from the last time the monitor was reset or is this a delta report
     **/
    public boolean isCumulative() {
        return isCumulative;
    }
    
    /**
     * Get the contained service-specific ServiceMetrics
     *
     **/
    public Iterator getServiceMetrics() {
        return serviceMetrics.values().iterator();
    }
    
    /**
     * Get the contained service-specific ServiceMetric for the specified ServiceMonitor's classID
     *
     * @param moduleClassID ServiceMonitor's classID
     * @return ServiceMetric  ServiceMetric or null if Not Found
     **/
    public ServiceMetric getServiceMetric(ModuleClassID moduleClassID) {
        return serviceMetrics.get(moduleClassID);
    }
    
    /**
     * addServiceMetric are generally not created by applications, but by the Monitor or PeerInfoService
     **/
    public void addServiceMetric(ServiceMetric serviceMetric) {
        serviceMetrics.put(serviceMetric.getModuleClassID(), serviceMetric);
    }
    
    /**
     * addServiceMetric are generally not created by applications, but by the Monitor or PeerInfoService
     **/
    public void addServiceMetric(ModuleClassID moduleClassID, ServiceMetric serviceMetric) {
        serviceMetrics.put(moduleClassID, serviceMetric);
    }
    
    /**
     * Did this report contain any serviceMetrics for which there weren't registered ServiceMetric classes
     * @see MonitorResources
     **/
    public boolean isUnknownModuleClassIDs() {
        return (unknownModuleClassIDs != null);
    }
    
    /**
     * Get iterator of ModuleClassIDs of serviceMetrics for which there weren't registered ServiceMetric classes
     *
     * @return Iterator
     **/
    public Iterator getUnknownModuleClassIDs() {
        if (unknownModuleClassIDs != null) {
            return unknownModuleClassIDs.iterator();
        } else {
            return 	new LinkedList().iterator();
        }
    }
    
    /**
     * {@inheritDoc}
     **/
    public void serializeTo(Element element) throws DocumentSerializationException {
        DocumentSerializableUtilities.addLong(element, "toTime", toTime);
        DocumentSerializableUtilities.addLong(element, "fromTime", fromTime);
        DocumentSerializableUtilities.addBoolean(element, "isCumulative", isCumulative);
        
        for (Iterator i = serviceMetrics.values().iterator(); i.hasNext();) {
            ServiceMetric serviceMetric = (ServiceMetric) i.next();
            
            Element serviceMetricElement = DocumentSerializableUtilities.createChildElement(element, "service");
            
            DocumentSerializableUtilities.addString(serviceMetricElement, "moduleClassID"
                    ,
                    serviceMetric.getModuleClassID().toString());
            DocumentSerializableUtilities.addDocumentSerializable(serviceMetricElement, "serviceMetric", serviceMetric);
        }
        
    }
    
    /**
     * {@inheritDoc}
     **/
    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
            
            if (tagName.equals("toTime")) {
                toTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("fromTime")) {
                fromTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("isCumulative")) {
                isCumulative = DocumentSerializableUtilities.getBoolean(childElement);
            } else if (tagName.equals("service")) {
                try {
                    ModuleClassID moduleClassID = (ModuleClassID) IDFactory.fromURI(
                            new URI(DocumentSerializableUtilities.getString(childElement, "moduleClassID", "ERROR")));

                    try {
                        ServiceMetric serviceMetric = MonitorResources.createServiceMetric(moduleClassID);

                        serviceMetric.init(moduleClassID);
                        Element serviceMetricElement = DocumentSerializableUtilities.getChildElement(childElement, "serviceMetric");

                        serviceMetric.initializeFrom(serviceMetricElement);
                        serviceMetrics.put(moduleClassID, serviceMetric);
                    } catch (Exception ex) {
                        if (unknownModuleClassIDs == null) {
                            unknownModuleClassIDs = new LinkedList<ModuleClassID>();
                        }
                        
                        unknownModuleClassIDs.add(moduleClassID);
                    }
                } catch (URISyntaxException jex) {
                    throw new DocumentSerializationException("Can't get ModuleClassID", jex);
                }
            }
        }
    }
}
