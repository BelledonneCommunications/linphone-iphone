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


import net.jxta.document.*;
import net.jxta.id.*;
import net.jxta.platform.*;
import net.jxta.util.documentSerializable.*;
import java.util.*;
import java.net.*;
import net.jxta.exception.*;
import net.jxta.util.*;


/**
 *		A Monitor Filter specifies which service-specific metrics should be obtained by the Monitor.
 * 	It contains a collection of ServiceMonitor specific ServiceMonitorFilters.  
 *		
 **/
public class MonitorFilter implements DocumentSerializable {
    private String description;
    private Map<ModuleClassID,ServiceMonitorFilter> serviceMonitorFilters = new HashMap<ModuleClassID,ServiceMonitorFilter>();
    private List<ModuleClassID> unknownModuleClassIDs;

    /**
     * MonitorFilter
     *
     **/
    public MonitorFilter() {}
	
    /**
     * MonitorFilter
     *
     * @param description
     **/
    public MonitorFilter(String description) {
        this.description = description;
    }

    /**
     * Add a ServiceMonitorFilter to this MonitorFilter
     *
     * @param serviceMonitorFilter	Service Specific Filter
     * @exception MonitorFilterException
     * @return ServiceMonitorFilter Modified Filter to the capabilities of the service
     **/
    public ServiceMonitorFilter addServiceMonitorFilter(ServiceMonitorFilter serviceMonitorFilter) throws MonitorFilterException {
        ModuleClassID moduleClassID = serviceMonitorFilter.getModuleClassID();

        if (serviceMonitorFilters.get(moduleClassID) != null) {
            throw new MonitorFilterException("Attempt to add a second Monitor Filter for: " + moduleClassID);
        }
        serviceMonitorFilters.put(moduleClassID, serviceMonitorFilter);
        return serviceMonitorFilter;
    }

    /**
     * Get ServiceMonitorFilter subfilter
     *
     * @param moduleClassID	ServiceMonitor's moduleClassID
     * @return ServiceMonitorFilter  SubFilter or null if not found
     **/
    public ServiceMonitorFilter getServiceMonitorFilter(ModuleClassID moduleClassID) {
        return serviceMonitorFilters.get(moduleClassID);
    }

    /**
     * remove ServiceMonitorFilter
     *
     * @param moduleClassID	ServiceMonitor's moduleClassID
     **/
    public void removeServiceMonitorFilter(ModuleClassID moduleClassID) {
        serviceMonitorFilters.remove(moduleClassID);
    }

    /**
     * Get the number of subfilters
     *
     * @return int
     **/
    public int getServiceMonitorFilterCount() {
        return serviceMonitorFilters.size();
    }
		
    /**
     * get ModuleClassIDs of contained subfilters
     *
     * @return Iterator of ServiceMonitor ClassIDs
     **/
    public Iterator getModuleClassIDs() {
        return serviceMonitorFilters.keySet().iterator();
    }

    /**
     * Get Iterator of all ServiceMonitorFilters subfilters
     *
     * @return Iterator of all ServiceMonitorFilters subfilters
     **/
    public Iterator getServiceMonitorFilters() {
        return serviceMonitorFilters.values().iterator();
    }
	
    /**
     * Were any filters removed from this MonitorFilter (particularly when received remotely and deserialized)
     *
     **/
    public boolean isUnknownModuleClassIDs() {
        return (unknownModuleClassIDs != null);
    }

    /**
     * Get a list of ModuleClassIDs for subfilters that could not be deserialized because they weren't registered
     * @see MonitorResources
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
     * Get Description
     *
     **/
    public String getDescription() {
        return description;
    }

    /**
     * {@inheritDoc}
     **/
    public void serializeTo(Element element) throws DocumentSerializationException {
        DocumentSerializableUtilities.addString(element, "description", description);

        for (Iterator i = serviceMonitorFilters.values().iterator(); i.hasNext();) {
            ServiceMonitorFilter serviceMonitorFilter = (ServiceMonitorFilter) i.next();
			
            Element serviceElement = DocumentSerializableUtilities.createChildElement(element, "service");	

            DocumentSerializableUtilities.addString(serviceElement, "moduleClassID"
                    ,
                    serviceMonitorFilter.getModuleClassID().toString());
            DocumentSerializableUtilities.addDocumentSerializable(serviceElement, "serviceFilter", serviceMonitorFilter);
        }
    }

    /**
     * {@inheritDoc}
     **/
    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element serviceElement = (TextElement) e.nextElement();
            String tagName = (String) serviceElement.getKey();
			
            if (tagName.equals("service")) {
                try {
                    ModuleClassID moduleClassID = (ModuleClassID) IDFactory.fromURI(
                            new URI(DocumentSerializableUtilities.getString(serviceElement, "moduleClassID", "ERROR")));
								
                    try {
                        ServiceMonitorFilter serviceMonitorFilter = MonitorResources.createServiceMonitorFilter(moduleClassID);

                        serviceMonitorFilter.init(moduleClassID);
                        Element serviceMonitorFilterElement = DocumentSerializableUtilities.getChildElement(serviceElement
                                ,
                                "serviceFilter");

                        serviceMonitorFilter.initializeFrom(serviceMonitorFilterElement);
                        serviceMonitorFilters.put(moduleClassID, serviceMonitorFilter);
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
