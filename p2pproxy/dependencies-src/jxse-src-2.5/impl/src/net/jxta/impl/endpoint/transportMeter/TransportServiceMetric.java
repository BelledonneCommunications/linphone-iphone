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

package net.jxta.impl.endpoint.transportMeter;


import net.jxta.document.Element;
import net.jxta.document.TextElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.id.IDFactory;
import net.jxta.meter.MonitorResources;
import net.jxta.meter.ServiceMetric;
import net.jxta.platform.ModuleClassID;
import net.jxta.util.documentSerializable.DocumentSerializableUtilities;
import net.jxta.util.documentSerializable.DocumentSerializationException;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.LinkedList;


/**
 * The Service Monitor Metric for the Transport Services
 */
public class TransportServiceMetric implements ServiceMetric {
    private LinkedList<TransportMetric> transportMetrics = new LinkedList<TransportMetric>();
    private ModuleClassID moduleClassID = MonitorResources.transportServiceMonitorClassID;

    public TransportServiceMetric() {}

    /**
     * {@inheritDoc}
     */
    public void init(ModuleClassID moduleClassID) {
        this.moduleClassID = moduleClassID;
    }

    /**
     * {@inheritDoc}
     */
    public ModuleClassID getModuleClassID() {
        return moduleClassID;
    }

    /**
     * Append a Transport Metric
     * @param transportMetric metric to add
     */
    public void addTransportMetric(TransportMetric transportMetric) {
        transportMetrics.add(transportMetric);
    }

    /**
     * Get all Transport Metrics
     * @return iterator of all transport metrics
     */
    public Iterator<TransportMetric> getTransportMetrics() {
        return transportMetrics.iterator();
    }

    /**
     * Get the Transport Metric for a specific Transport Type
     * @param protocol protocol name
     * @param endpointAddress address
     * @return  a Transport Metric for a specific Transport Type
     */
    public TransportMetric getTransportMetric(String protocol, EndpointAddress endpointAddress) {
        for (TransportMetric transportMetric : transportMetrics) {
            if (protocol.equals(transportMetric.getProtocol()) && endpointAddress.equals(transportMetric.getEndpointAddress())) {
                return transportMetric;
            }
        }

        return null;
    }

    /**
     * Get the Transport Metric for a specific Transport Type
     *
     * @param prototype a similar Transport metric object (ie same protocol/endpointAddress)
     * @see #getTransportMetric(String, EndpointAddress)
     * @return a Transport Metric for a specific Transport Type
     */
    public TransportMetric getTransportMetric(TransportMetric prototype) {
        return getTransportMetric(prototype.getProtocol(), prototype.getEndpointAddress());
    }

    /**
     * {@inheritDoc}
     */
    public void serializeTo(Element element) throws DocumentSerializationException {
        for (TransportMetric transportMetric : transportMetrics) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "transportMetric", transportMetric);
        }
        if (moduleClassID != null) {
            DocumentSerializableUtilities.addString(element, "moduleClassID", moduleClassID.toString());
        }
    }

    /**
     * {@inheritDoc}
     */
    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("transportMetric")) {
                TransportMetric transportMetric = (TransportMetric) DocumentSerializableUtilities.getDocumentSerializable(
                        childElement, TransportMetric.class);

                transportMetrics.add(transportMetric);
            }
            if (tagName.equals("moduleClassID")) {
                try {
                    moduleClassID = (ModuleClassID) IDFactory.fromURI(
                            new URI(DocumentSerializableUtilities.getString(childElement)));
                } catch (URISyntaxException jex) {
                    throw new DocumentSerializationException("Can't read moduleClassID", jex);
                }
            }
        }
    }

    /**
     * Make a shallow copy of this metric only including the portions designated in the Filter
     * <P> Note: since this is a shallow copy it is dangerous to modify the submetrics
     *
     * @param transportServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     */
    public TransportServiceMetric shallowCopy(TransportServiceMonitorFilter transportServiceMonitorFilter) {
        TransportServiceMetric serviceMetric = new TransportServiceMetric();

        serviceMetric.moduleClassID = moduleClassID;

        for (Iterator<TransportMetric> i = getTransportMetrics(); i.hasNext();) {
            TransportMetric transportMetric = i.next();
            String protocol = transportMetric.getProtocol();

            if (transportServiceMonitorFilter.hasTransport(protocol)) {
                serviceMetric.addTransportMetric(transportMetric);
            }
        }

        return serviceMetric;
    }

    /**
     * {@inheritDoc}
     */
    public void mergeMetrics(ServiceMetric serviceMetric) {
        mergeMetrics(serviceMetric, null);
    }

    /**
     *
     * @param serviceMetric
     * @param transportServiceMonitorFilter
     */
    public void mergeMetrics(ServiceMetric serviceMetric, TransportServiceMonitorFilter transportServiceMonitorFilter) {
        TransportServiceMetric otherTransportServiceMetric = (TransportServiceMetric) serviceMetric;

        for (Iterator<TransportMetric> i = otherTransportServiceMetric.getTransportMetrics(); i.hasNext();) {
            TransportMetric otherTransportMetric = i.next();
            String protocol = otherTransportMetric.getProtocol();

            if ((transportServiceMonitorFilter == null) || transportServiceMonitorFilter.hasTransport(protocol)) {
                TransportMetric transportMetric = getTransportMetric(otherTransportMetric.getProtocol()
                        ,
                        otherTransportMetric.getEndpointAddress());

                if (transportMetric == null) {
                    transportMetric = new TransportMetric(otherTransportMetric);
                    addTransportMetric(transportMetric);
                }

                transportMetric.mergeMetrics(otherTransportMetric);
            }
        }
    }

    /**
     * Make a deep copy of this metric only including the portions designated in the Filter
     * The resulting metric is Safe to modify without danger to the underlying Monitor Metrics
     *
     * @param transportServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     */
    public TransportServiceMetric deepCopy(TransportServiceMonitorFilter transportServiceMonitorFilter) {
        TransportServiceMetric serviceMetric = new TransportServiceMetric();

        serviceMetric.moduleClassID = moduleClassID;

        serviceMetric.mergeMetrics(this, transportServiceMonitorFilter);
        return serviceMetric;
    }

    /**
     * {@inheritDoc}
     */
    public void diffMetrics(ServiceMetric otherOne) {
        throw new RuntimeException("Not Supported");
    }
}
