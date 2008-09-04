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
 * Basic Service Metric EndpointService Monitoring
 */
public class EndpointServiceMetric implements ServiceMetric {
    private LinkedList<InboundMetric> inboundMetrics = new LinkedList<InboundMetric>();
    private LinkedList<OutboundMetric> outboundMetrics = new LinkedList<OutboundMetric>();
    private LinkedList<PropagationMetric> propagationMetrics = new LinkedList<PropagationMetric>();
    private EndpointMetric endpointMetric;
    private ModuleClassID moduleClassID = MonitorResources.endpointServiceMonitorClassID;

    public EndpointServiceMetric() {}

    public EndpointServiceMetric(ModuleClassID moduleClassID) {
        this.moduleClassID = moduleClassID;
    }

    public void init(ModuleClassID moduleClassID) {
        this.moduleClassID = moduleClassID;
    }

    public ModuleClassID getModuleClassID() {
        return moduleClassID;
    }

    void addInboundMetric(InboundMetric inboundMetric) {
        inboundMetrics.add(inboundMetric);
    }

    public Iterator<InboundMetric> getInboundMetrics() {
        return inboundMetrics.iterator();
    }

    public InboundMetric getInboundMetric(String serviceName, String serviceParam) {
        for (InboundMetric inboundMetric : inboundMetrics) {
            if (inboundMetric.matches(serviceName, serviceParam)) {
                return inboundMetric;
            }
        }

        return null;
    }

    public Iterator<PropagationMetric> getPropagationMetrics() {
        return propagationMetrics.iterator();
    }

    public PropagationMetric getPropagationMetric(String serviceName, String serviceParam) {
        for (PropagationMetric propagationMetric : propagationMetrics) {
            if (propagationMetric.matches(serviceName, serviceParam)) {
                return propagationMetric;
            }
        }

        return null;
    }

    void addPropagationMetric(PropagationMetric propagationMetric) {
        propagationMetrics.add(propagationMetric);
    }

    void addOutboundMetric(OutboundMetric outboundMetric) {
        outboundMetrics.add(outboundMetric);
    }

    public Iterator<OutboundMetric> getOutboundMetrics() {
        return outboundMetrics.iterator();
    }

    public OutboundMetric getOutboundMetric(EndpointAddress endpointAddress) {
        for (OutboundMetric outboundMetric : outboundMetrics) {
            if (outboundMetric.matches(endpointAddress)) {
                return outboundMetric;
            }
        }

        return null;
    }

    public EndpointMetric getEndpointMetric() {
        return endpointMetric;
    }

    void setEndpointMetric(EndpointMetric endpointMetric) {
        this.endpointMetric = endpointMetric;
    }

    public void serializeTo(Element element) throws DocumentSerializationException {
        for (Object outboundMetric1 : outboundMetrics) {
            OutboundMetric outboundMetric = (OutboundMetric) outboundMetric1;

            DocumentSerializableUtilities.addDocumentSerializable(element, "outboundMetric", outboundMetric);
        }
        for (Object inboundMetric1 : inboundMetrics) {
            InboundMetric inboundMetric = (InboundMetric) inboundMetric1;

            DocumentSerializableUtilities.addDocumentSerializable(element, "inboundMetric", inboundMetric);
        }
        for (Object propagationMetric1 : propagationMetrics) {
            PropagationMetric propagationMetric = (PropagationMetric) propagationMetric1;

            DocumentSerializableUtilities.addDocumentSerializable(element, "propagationMetric", propagationMetric);
        }
        if (endpointMetric != null) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "endpointMetric", endpointMetric);
        }

        if (moduleClassID != null) {
            DocumentSerializableUtilities.addString(element, "moduleClassID", moduleClassID.toString());
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("inboundMetric")) {
                InboundMetric inboundMetric = (InboundMetric) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        InboundMetric.class);

                inboundMetrics.add(inboundMetric);
            }
            if (tagName.equals("outboundMetric")) {
                OutboundMetric outboundMetric = (OutboundMetric) DocumentSerializableUtilities.getDocumentSerializable(
                        childElement, OutboundMetric.class);

                outboundMetrics.add(outboundMetric);
            }
            if (tagName.equals("propagationMetric")) {
                PropagationMetric propagationMetric = (PropagationMetric) DocumentSerializableUtilities.getDocumentSerializable(
                        childElement, PropagationMetric.class);

                propagationMetrics.add(propagationMetric);
            }
            if (tagName.equals("endpointMetric")) {
                endpointMetric = (EndpointMetric) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        EndpointMetric.class);
            }
            try {
                if (tagName.equals("moduleClassID")) {
                    moduleClassID = (ModuleClassID) IDFactory.fromURI(
                            new URI(DocumentSerializableUtilities.getString(childElement)));
                }
            } catch (URISyntaxException jex) {
                throw new DocumentSerializationException("Can't decipher ModuleClassID", jex);
            }
        }
    }

    public void mergeMetrics(ServiceMetric otherOne) {
        mergeMetrics(otherOne, true, true, true, true);
    }

    /**
     * Make a deep copy of this metric only including the portions designated in the Filter
     * The resulting metric is Safe to modify without danger to the underlying Monitor Metrics
     *
     * @param endpointServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     */
    public EndpointServiceMetric deepCopy(EndpointServiceMonitorFilter endpointServiceMonitorFilter) {
        EndpointServiceMetric serviceMetric = new EndpointServiceMetric();

        serviceMetric.moduleClassID = moduleClassID;

        serviceMetric.mergeMetrics(this, true, endpointServiceMonitorFilter.isIncludeInboundMetrics()
                ,
                endpointServiceMonitorFilter.isIncludeOutboundMetrics(), endpointServiceMonitorFilter.isIncludePropagateMetrics());
        return serviceMetric;
    }

    public void mergeMetrics(ServiceMetric otherOne, boolean includeEndpointMetrics, boolean includeInboundMetrics, boolean includeOutboundEndpointMetrics, boolean includePropagationMetrics) {
        EndpointServiceMetric otherEndpointServiceMetric = (EndpointServiceMetric) otherOne;

        if (includeEndpointMetrics) {
            EndpointMetric otherEndpointMetric = otherEndpointServiceMetric.getEndpointMetric();

            if ((endpointMetric == null) && (otherEndpointMetric != null)) {
                endpointMetric = new EndpointMetric(otherEndpointMetric);
            }

            if (otherEndpointMetric != null) {
                endpointMetric.mergeMetrics(otherEndpointMetric);
            }
        }

        if (includeInboundMetrics) {
            for (Iterator<InboundMetric> i = otherEndpointServiceMetric.getInboundMetrics(); i.hasNext();) {
                InboundMetric otherInboundMetric = i.next();
                InboundMetric inboundMetric = getInboundMetric(otherInboundMetric.getServiceName()
                        ,
                        otherInboundMetric.getServiceParameter());

                if (inboundMetric == null) {
                    inboundMetric = new InboundMetric(otherInboundMetric);
                    addInboundMetric(inboundMetric);
                }

                inboundMetric.mergeMetrics(otherInboundMetric);
            }
        }

        if (includeOutboundEndpointMetrics) {
            for (Iterator<OutboundMetric> i = otherEndpointServiceMetric.getOutboundMetrics(); i.hasNext();) {
                OutboundMetric otherOutboundMetric = i.next();
                OutboundMetric outboundMetric = getOutboundMetric(otherOutboundMetric.getEndpointAddress());

                if (outboundMetric == null) {
                    outboundMetric = new OutboundMetric(otherOutboundMetric);
                    addOutboundMetric(outboundMetric);
                }

                outboundMetric.mergeMetrics(otherOutboundMetric);
            }
        }

        if (includeOutboundEndpointMetrics) {
            for (Iterator<PropagationMetric> i = otherEndpointServiceMetric.getPropagationMetrics(); i.hasNext();) {
                PropagationMetric otherPropagationMetric = i.next();
                PropagationMetric propagationMetric = getPropagationMetric(otherPropagationMetric.getServiceName()
                        ,
                        otherPropagationMetric.getServiceParameter());

                if (propagationMetric == null) {
                    propagationMetric = new PropagationMetric(otherPropagationMetric);
                    addPropagationMetric(propagationMetric);
                }

                propagationMetric.mergeMetrics(otherPropagationMetric);
            }
        }
    }

    /**
     * Make a shallow copy of this metric only including the portions designated in the Filter
     * <P> Note: since this is a shallow copy it is dangerous to modify the submetrics
     *
     * @param endpointServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     */
    public EndpointServiceMetric shallowCopy(EndpointServiceMonitorFilter endpointServiceMonitorFilter) {
        EndpointServiceMetric endpointServiceMetric = new EndpointServiceMetric(moduleClassID);

        endpointServiceMetric.endpointMetric = endpointMetric;

        if (endpointServiceMonitorFilter.isIncludeInboundMetrics()) {
            for (Iterator<InboundMetric> i = getInboundMetrics(); i.hasNext();) {
                InboundMetric inboundMetric = i.next();

                endpointServiceMetric.addInboundMetric(inboundMetric);
            }
        }

        if (endpointServiceMonitorFilter.isIncludeOutboundMetrics()) {
            for (Iterator<OutboundMetric> i = getOutboundMetrics(); i.hasNext();) {
                OutboundMetric outboundMetric = i.next();

                endpointServiceMetric.addOutboundMetric(outboundMetric);
            }
        }

        if (endpointServiceMonitorFilter.isIncludePropagateMetrics()) {
            for (Iterator<PropagationMetric> i = getPropagationMetrics(); i.hasNext();) {
                PropagationMetric propagationMetric = i.next();

                endpointServiceMetric.addPropagationMetric(propagationMetric);
            }
        }

        return endpointServiceMetric;
    }

    public void diffMetrics(ServiceMetric otherOne) {
        throw new RuntimeException("Not Supported");
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }
}
