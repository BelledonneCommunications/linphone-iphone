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
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Map;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.TextElement;
import net.jxta.exception.JxtaException;
import net.jxta.id.ID;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.util.AdvertisementUtilities;
import net.jxta.util.documentSerializable.DocumentSerializableUtilities;


/**
 *   Registration point for types of ServiceMonitors's Advertisements, ServiceMetrics and ServiceMonitorFilters
 *   based upon the ModuleClassID for the ServiceMonitor
 **/

public class MonitorResources {
    public static final String SERVICE_MONITOR_TAG = "serviceMonitor";
    public static final String SERVICE_MONITOR_ADVERTISEMENT_TAG = ModuleImplAdvertisement.getAdvertisementType();
    public static final String CLASS_ID_TAG = "moduleClassID";
    public static final String SERVICE_TITLE_TAG = "serviceTitle";
    public static final String SERVICE_MONITOR_IMPL_TAG = "serviceMonitorImpl";
    public static final String METRIC_CLASS_TAG = "serviceMetric";
    public static final String FILTER_CLASS_TAG = "serviceMonitorFilter";
    
    private static Map<ModuleClassID,ServiceResource> registeredMonitorResources = new Hashtable<ModuleClassID,ServiceResource>();
    
    /**
     *  Prefix string for all of the Well Known IDs declared in this interface.
     **/
    private static final String WK_ID_PREFIX = ID.URIEncodingName + ":" + ID.URNNamespace + ":uuid-DeadBeefDeafBabaFeedBabe";
    
    /**
     * Well known classes for the basic service Monitors.

     * To keep their string representation shorter, we put our small spec
     * or role pseudo unique ID at the front of the second UUID string.
     * Base classes do not need an explicit second UUID string because it is
     * all 0.
     * The type is always the last two characters, nomatter the total length.
     */
    
    /**
     * Well known module class identifier: monitor service
     */
    public static final ModuleClassID monitorServiceClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000011F05"));
    
    /**
     * Well known module class identifier: resolver service
     */
    public static final ModuleClassID resolverServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010205"));
    
    /**
     * Well known module class identifier: discovery service
     */
    public static final ModuleClassID discoveryServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010305"));
    
    /**
     * Well known module class identifier: pipe service
     */
    public static final ModuleClassID pipeServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010405"));
    
    /**
     * Well known module class identifier: membership service
     */
    public static final ModuleClassID membershipServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010505"));
    
    /**
     * Well known module class identifier: rendezvous service
     */
    public static final ModuleClassID rendezvousServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010605"));
    
    /**
     * Well known module class identifier: peerinfo service
     */
    public static final ModuleClassID peerinfoServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010705"));
    
    /**
     * Well known module class identifier: endpoint service
     */
    public static final ModuleClassID endpointServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010805"));
    
    /*
     * FIXME: EndpointProtocols should probably all be of the same class
     * and of different specs and roles... But we'll take a shortcut for now.
     */
    
    /**
     * Well known module class identifier: transport protocol
     */
    public static final ModuleClassID transportServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010905"));
    
    /**
     * Well known module class identifier: router protocol
     */
    public static final ModuleClassID routerServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010B05"));
    
    /**
     * Well known module class identifier: tlsProtocol
     */
    public static final ModuleClassID tlsProtoServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "00000105"));
    
    /**
     * Well known module class identifier: ProxyService
     */
    public static final ModuleClassID proxyServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010E05"));
    
    /**
     * Well known module class identifier: RelayProtocol
     */
    public static final ModuleClassID relayServiceMonitorClassID = (ModuleClassID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010F05"));
    
    /**
     * Well known service specification identifier: the standard monitor
     */
    public static final ModuleSpecID refMonitorServiceSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "0000011F0106"));
    
    /**
     * Well known service specification identifier: the standard resolver
     */
    public static final ModuleSpecID refResolverServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001020106"));
    
    /**
     * Well known service specification identifier: the standard discovery
     */
    public static final ModuleSpecID refDiscoveryServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001030106"));
    
    /**
     * Well known service specification identifier: the standard pipe
     */
    public static final ModuleSpecID refPipeServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001040106"));
    
    /**
     * Well known service specification identifier: the standard membership
     */
    public static final ModuleSpecID refMembershipServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001050106"));
    
    /**
     * Well known service specification identifier: the standard rendezvous
     */
    public static final ModuleSpecID refRendezvousServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001060106"));
    
    /**
     * Well known service specification identifier: the standard peerinfo
     */
    public static final ModuleSpecID refPeerinfoServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001070106"));
    
    /**
     * Well known service specification identifier: the standard endpoint
     */
    public static final ModuleSpecID refEndpointServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001080106"));
    
    /**
     * Well known endpoint protocol specification identifier: the standard
     * transport Service Monitor
     */
    public static final ModuleSpecID refTransportServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "000001090106"));
    
    /**
     * Well known endpoint protocol specification identifier: the standard
     * router
     */
    public static final ModuleSpecID refRouterServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010B0106"));
    
    /**
     * Well known endpoint protocol specification identifier: the standard
     * tls endpoint protocol
     */
    public static final ModuleSpecID refTlsServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010D0106"));
    
    /**
     * Well known application: the Proxy
     */
    public static final ModuleSpecID refProxyServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010E0106"));
    
    /**
     * Well known endpoint protocol specification identifier: the standard
     * relay endpoint protocol
     */
    public static final ModuleSpecID refRelayServiceMonitorSpecID = (ModuleSpecID)
            ID.create(URI.create(WK_ID_PREFIX + "0000010F0106"));
    
    private static ModuleClassID standardServiceMonitorClassIDs[] = new ModuleClassID[] {
        resolverServiceMonitorClassID, rendezvousServiceMonitorClassID, endpointServiceMonitorClassID
                ,
        transportServiceMonitorClassID };
        
    public static ModuleImplAdvertisement getReferenceAllPurposeMonitorServiceImplAdvertisement(boolean includeTransports) {
        ModuleImplAdvertisement moduleImplAdvertisement = AdvertisementUtilities.createModuleImplAdvertisement(
                refMonitorServiceSpecID, "net.jxta.impl.meter.MonitorManager", "Service Monitor");
        StructuredTextDocument param = (StructuredTextDocument) StructuredDocumentFactory.newStructuredDocument(
                MimeMediaType.XMLUTF8, "serviceMonitor");
            
        addServiceMonitorServiceAdvertisement(param, refResolverServiceMonitorSpecID, "Resolver"
                ,
                "net.jxta.impl.resolver.resolverMeter.ResolverServiceMonitor"
                ,
                "net.jxta.impl.resolver.resolverMeter.ResolverServiceMetric"
                ,
                "net.jxta.impl.resolver.resolverMeter.ResolverServiceMonitorFilter");
            
        addServiceMonitorServiceAdvertisement(param, refEndpointServiceMonitorSpecID, "Endpoint"
                ,
                "net.jxta.impl.endpoint.endpointMeter.EndpointServiceMonitor"
                ,
                "net.jxta.impl.endpoint.endpointMeter.EndpointServiceMetric"
                ,
                "net.jxta.impl.endpoint.endpointMeter.EndpointServiceMonitorFilter");
            
        addServiceMonitorServiceAdvertisement(param, refTransportServiceMonitorSpecID, "Transport"
                ,
                "net.jxta.impl.endpoint.transportMeter.TransportServiceMonitor"
                ,
                "net.jxta.impl.endpoint.transportMeter.TransportServiceMetric"
                ,
                "net.jxta.impl.endpoint.transportMeter.TransportServiceMonitorFilter");
            
        addServiceMonitorServiceAdvertisement(param, refRendezvousServiceMonitorSpecID, "Rendezvous"
                ,
                "net.jxta.impl.rendezvous.rendezvousMeter.RendezvousServiceMonitor"
                ,
                "net.jxta.impl.rendezvous.rendezvousMeter.RendezvousServiceMetric"
                ,
                "net.jxta.impl.rendezvous.rendezvousMeter.RendezvousServiceMonitorFilter");
            
        moduleImplAdvertisement.setParam(param);
        return moduleImplAdvertisement;
    }
        
    private static void addServiceMonitorServiceAdvertisement(Element root, ModuleSpecID moduleSpecID, String title, String implClassName, String metricClassName, String filterClassName) {
        ModuleImplAdvertisement moduleImplAdvertisement = createServiceMonitorModuleImplAdvertisement(moduleSpecID, title
                ,
                implClassName, metricClassName, filterClassName);
            
        Element serviceMonitorElement = DocumentSerializableUtilities.createChildElement(root, SERVICE_MONITOR_TAG);
            
        ModuleClassID moduleClassID = moduleSpecID.getBaseClass();

        DocumentSerializableUtilities.addString(serviceMonitorElement, CLASS_ID_TAG, moduleClassID.toString());
            
        Element serviceMonitorAdvertisementElement = DocumentSerializableUtilities.createChildElement(serviceMonitorElement
                ,
                SERVICE_MONITOR_ADVERTISEMENT_TAG);
        Element advDoc = (Element) moduleImplAdvertisement.getDocument(MimeMediaType.XMLUTF8);

        DocumentSerializableUtilities.copyChildren(serviceMonitorAdvertisementElement, advDoc);
    }
        
    public static ModuleImplAdvertisement createServiceMonitorModuleImplAdvertisement(ModuleSpecID moduleSpecID, String title, String implClassName, String metricClassName, String filterClassName) {
        ModuleImplAdvertisement moduleImplAdvertisement = AdvertisementUtilities.createModuleImplAdvertisement(moduleSpecID
                ,
                implClassName, "Service Monitor");
            
        StructuredTextDocument param = (StructuredTextDocument) StructuredDocumentFactory.newStructuredDocument(
                MimeMediaType.XMLUTF8, "serviceMonitor");
            
        DocumentSerializableUtilities.addString(param, SERVICE_TITLE_TAG, title);
        DocumentSerializableUtilities.addString(param, METRIC_CLASS_TAG, metricClassName);
        DocumentSerializableUtilities.addString(param, FILTER_CLASS_TAG, filterClassName);
        moduleImplAdvertisement.setParam(param);
            
        return moduleImplAdvertisement;
    }
        
    public static ModuleImplAdvertisement getServiceMonitorImplAdvertisement(ModuleClassID serviceMonitorModuleClassID, ModuleImplAdvertisement monitorServiceImplAdvertisement) {
            
        String classIDText = serviceMonitorModuleClassID.toString();
            
        StructuredDocument param = monitorServiceImplAdvertisement.getParam();
            
        for (Enumeration e = param.getChildren(SERVICE_MONITOR_TAG); e.hasMoreElements();) {
            Element serviceMonitorElement = (Element) e.nextElement();
                
            String serviceMonitorClassIDText = DocumentSerializableUtilities.getString(serviceMonitorElement, CLASS_ID_TAG, "");
                
            if (classIDText.equals(serviceMonitorClassIDText)) {
                TextElement serviceMonitorAdvertisementElement = (TextElement) DocumentSerializableUtilities.getChildElement(
                        serviceMonitorElement, SERVICE_MONITOR_ADVERTISEMENT_TAG);

                return (ModuleImplAdvertisement) AdvertisementFactory.newAdvertisement(serviceMonitorAdvertisementElement);
            }
        }
            
        return null;
            
    }
        
    private static class ServiceResource {
        String serviceMonitorClassName;
        Class serviceMonitorClass;
        String serviceMonitorFilterClassName;
        Class serviceMonitorFilterClass;
        String serviceMetricClassName;
        Class serviceMetricClass;
            
        ServiceResource(ModuleImplAdvertisement moduleImplAdvertisement) throws JxtaException {
            try {
                serviceMonitorClassName = moduleImplAdvertisement.getCode();
                    
                serviceMonitorFilterClassName = getServiceMonitorFilterClassName(moduleImplAdvertisement);
                serviceMonitorFilterClass = Class.forName(serviceMonitorFilterClassName);
                    
                if (!(ServiceMonitorFilter.class).isAssignableFrom(serviceMonitorFilterClass)) {
                    throw new JxtaException(
                            "Bad ServiceMonitorImplAdvertisment: " + serviceMonitorFilterClassName
                            + " is not a ServiceMonitorFilter");
                }
                    
                serviceMetricClassName = getServiceMetricClassName(moduleImplAdvertisement);
                serviceMetricClass = Class.forName(serviceMetricClassName);
                    
                if (!(ServiceMetric.class).isAssignableFrom(serviceMetricClass)) {
                    throw new JxtaException(
                            "Bad ServiceMonitorImplAdvertisment: " + serviceMetricClassName + " is not a ServiceMetric");
                }
            } catch (Exception e) {
                throw new JxtaException("Bad ServiceMonitorImplAdvertisment: Unable to load constituent parts", e);
            }
        }
    }
        
    /**
     * Register the Implementation Advertisement for a ServiceMonitor Type
     *  This contains the Monitor, Metric and Filter classNames
     */
    public static void registerServiceMonitorModuleImplAdvertisement(ModuleImplAdvertisement moduleImplAdvertisement) throws JxtaException {
        ModuleClassID moduleClassID = moduleImplAdvertisement.getModuleSpecID().getBaseClass();
            
        if (registeredMonitorResources.get(moduleClassID) != null) {
            return;
        }
            
        registeredMonitorResources.put(moduleClassID, new ServiceResource(moduleImplAdvertisement));
    }
        
    /**
     * Create an empty ServiceMonitorFilter for the corresponding ModuleClassID
     *
     * @exception JxtaException  If not found or other errors
     */
    public static ServiceMonitorFilter createServiceMonitorFilter(ModuleClassID moduleClassID) throws MonitorFilterException {
        try {
            ServiceResource serviceResource = registeredMonitorResources.get(moduleClassID);
            ServiceMonitorFilter serviceMonitorFilter = (ServiceMonitorFilter) serviceResource.serviceMonitorFilterClass.newInstance();

            serviceMonitorFilter.init(moduleClassID);
            return serviceMonitorFilter;
        } catch (Exception e) {
            throw new MonitorFilterException("Unable to Create Filter: " + moduleClassID); // this should never happen, we already did our checks
        }
    }
        
    /**
     * Create an empty Service Metric for the corresponding ModuleClassID
     *
     * @exception JxtaException  If not found or other errors
     */
    public static ServiceMetric createServiceMetric(ModuleClassID moduleClassID) throws JxtaException {
        try {
            ServiceResource serviceResource = registeredMonitorResources.get(moduleClassID);
            ServiceMetric serviceMetric = (ServiceMetric) serviceResource.serviceMetricClass.newInstance();

            serviceMetric.init(moduleClassID);
            return serviceMetric;
        } catch (Exception e) {
            throw new JxtaException("Unable to Create Service Metric"); // this should never happen, we already did our checks
        }
    }
        
    /**
     * get ServiceMetric ClassName from its ImplAdvertisement
     */
    public static String getServiceMetricClassName(ModuleImplAdvertisement serviceMonitorModuleImplAdvertisement) {
        Element param = serviceMonitorModuleImplAdvertisement.getParam();

        return DocumentSerializableUtilities.getString(param, METRIC_CLASS_TAG, null);
    }
        
    /**
     * get ServiceMonitorFilter ClassName from its ImplAdvertisement
     */
    public static String getServiceMonitorFilterClassName(ModuleImplAdvertisement serviceMonitorModuleImplAdvertisement) {
        Element param = serviceMonitorModuleImplAdvertisement.getParam();

        return DocumentSerializableUtilities.getString(param, FILTER_CLASS_TAG, null);
    }
        
    /**
     * Get a list of all registered Service Monitor types
     */
    public static ModuleClassID[] getRegisteredModuleClassIDs() {
        return standardServiceMonitorClassIDs.clone();
    }
        
    /**
     * Get the name of standard Monitor Type
     **/
    public static String getMonitorTypeName(ModuleClassID moduleClassID) {
        if (moduleClassID.equals(monitorServiceClassID)) {
            return "monitor";
        }
            
        if (moduleClassID.equals(resolverServiceMonitorClassID)) {
            return "Resolver";
        }
            
        if (moduleClassID.equals(discoveryServiceMonitorClassID)) {
            return "Discovery";
        }
            
        if (moduleClassID.equals(pipeServiceMonitorClassID)) {
            return "Pipe";
        }
            
        if (moduleClassID.equals(membershipServiceMonitorClassID)) {
            return "Membership";
        }
            
        if (moduleClassID.equals(rendezvousServiceMonitorClassID)) {
            return "Rendezvous";
        }
            
        if (moduleClassID.equals(peerinfoServiceMonitorClassID)) {
            return "PeerInfo";
        }
            
        if (moduleClassID.equals(endpointServiceMonitorClassID)) {
            return "Endpoint";
        }
            
        if (moduleClassID.equals(transportServiceMonitorClassID)) {
            return "Transport";
        }
            
        if (moduleClassID.equals(routerServiceMonitorClassID)) {
            return "monitor";
        }
            
        if (moduleClassID.equals(tlsProtoServiceMonitorClassID)) {
            return "Tls";
        }
            
        if (moduleClassID.equals(proxyServiceMonitorClassID)) {
            return "Proxy";
        }
            
        if (moduleClassID.equals(relayServiceMonitorClassID)) {
            return "Relay";
        }
            
        return null;
    }
        
}
