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

package net.jxta.impl.rendezvous.rendezvousMeter;


import net.jxta.meter.*;
import net.jxta.peer.*;

import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.platform.*;
import java.util.*;
import net.jxta.util.*;
import net.jxta.exception.*;
import net.jxta.id.*;
import java.net.*;


/**
 * The Service Monitor Metric for the standard Rendezvous Service
 **/
public class RendezvousServiceMetric implements ServiceMetric {
    private RendezvousMetric rendezvousMetric;
    private LinkedList rendezvousConnectionMetrics = new LinkedList();
    private LinkedList clientConnectionMetrics = new LinkedList();
    private ModuleClassID moduleClassID = MonitorResources.rendezvousServiceMonitorClassID;

    /**
     * Create a Service Metric: No-arg constructor is required 
     **/
    public RendezvousServiceMetric() {}
	
    private RendezvousServiceMetric(ModuleClassID moduleClassID) {
        init(moduleClassID);
    }

    /**
     * 	Initialize the metric with the ModuleClassID of the Monitor
     **/
    public void init(ModuleClassID moduleClassID) { 
        this.moduleClassID = moduleClassID;
    }

    /** Get the ModuleClassID of the Monitor that generated this ServiceMetric **/
    public ModuleClassID getModuleClassID() {
        return moduleClassID;
    }

    /**
     * Get the General Rendezvous Metric
     **/
    public RendezvousMetric getRendezvousMetric() {
        return rendezvousMetric;
    }

    void setRendezvousMetric(RendezvousMetric rendezvousMetric) { 
        this.rendezvousMetric = rendezvousMetric; 
    }

    /**
     *	Append a Client Connection Metric 
     **/
    public void addClientConnectionMetric(ClientConnectionMetric clientConnectionMetric) {
        synchronized (clientConnectionMetrics) {
            clientConnectionMetrics.add(clientConnectionMetric);
        }
    }

    /**
     * Get all the Client Connection Metrics 
     **/
    public Iterator getClientConnectionMetrics() {
        return clientConnectionMetrics.iterator();
    }

    void clearClientConnectionMetrics() {
        clientConnectionMetrics.clear();
    }

    /**
     * Get the Client Connection Metrics for a single Peers ID
     **/
    public ClientConnectionMetric getClientConnectionMetric(PeerID peerId) {
        for (Iterator i = clientConnectionMetrics.iterator(); i.hasNext();) {
            ClientConnectionMetric clientConnectionMetric = (ClientConnectionMetric) i.next();

            if (peerId.equals(clientConnectionMetric.getPeerID())) {
                return clientConnectionMetric;
            }
        }

        return null;
    }

    /**
     *	Append a Rendezvous Connection Metric 
     **/
    public void addRendezvousConnectionMetric(RendezvousConnectionMetric rendezvousConnectionMetric) {
        synchronized (rendezvousConnectionMetrics) {
            rendezvousConnectionMetrics.add(rendezvousConnectionMetric);
        }
    }

    /**
     * Get all the Rendezvous Connection Metrics 
     **/
    public Iterator getRendezvousConnectionMetrics() {
        return rendezvousConnectionMetrics.iterator();
    }

    void clearRendezvousConnectionMetrics() {
        rendezvousConnectionMetrics.clear();
    }

    /**
     * Get the Rendezvous Connection Metrics for each Peers ID
     **/
    public RendezvousConnectionMetric getRendezvousConnectionMetric(PeerID peerID) {
        for (Iterator i = rendezvousConnectionMetrics.iterator(); i.hasNext();) {
            RendezvousConnectionMetric rendezvousConnectionMetric = (RendezvousConnectionMetric) i.next();
			
            if (peerID.equals(rendezvousConnectionMetric.getPeerID())) {
                return rendezvousConnectionMetric;
            }
        }

        return null;
    }

    /**
     * {@inheritDoc}
     **/
    public void serializeTo(Element element) throws DocumentSerializationException {
        if (rendezvousMetric != null) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "rendezvousMetric", rendezvousMetric);
        }

        for (Iterator i = clientConnectionMetrics.iterator(); i.hasNext();) {
            ClientConnectionMetric clientConnectionMetric = (ClientConnectionMetric) i.next();

            DocumentSerializableUtilities.addDocumentSerializable(element, "clientConnectionMetric", clientConnectionMetric);		
        }

        for (Iterator i = rendezvousConnectionMetrics.iterator(); i.hasNext();) {
            RendezvousConnectionMetric rendezvousConnectionMetric = (RendezvousConnectionMetric) i.next();

            DocumentSerializableUtilities.addDocumentSerializable(element, "rendezvousConnectionMetric"
                    ,
                    rendezvousConnectionMetric);		
        }

        if (moduleClassID != null) {
            DocumentSerializableUtilities.addString(element, "moduleClassID", moduleClassID.toString());		
        }

    }

    /**
     * {@inheritDoc}
     **/
    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
			
            if (tagName.equals("clientConnectionMetric")) {
                ClientConnectionMetric clientConnectionMetric = (ClientConnectionMetric) DocumentSerializableUtilities.getDocumentSerializable(
                        childElement, ClientConnectionMetric.class);

                clientConnectionMetrics.add(clientConnectionMetric);
            } else if (tagName.equals("rendezvousConnectionMetric")) {
                RendezvousConnectionMetric rendezvousConnectionMetric = (RendezvousConnectionMetric) DocumentSerializableUtilities.getDocumentSerializable(
                        childElement, RendezvousConnectionMetric.class);

                rendezvousConnectionMetrics.add(rendezvousConnectionMetric);
            } else if (tagName.equals("rendezvousMetric")) {
                rendezvousMetric = (RendezvousMetric) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        RendezvousMetric.class);
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
     * {@inheritDoc}
     **/
    public void mergeMetrics(ServiceMetric otherServiceMetric) {
        mergeMetrics(otherServiceMetric, true, true, true);
    }

    /**
     * Make a deep copy of this metric only including the portions designated in the Filter
     * The resulting metric is Safe to modify without danger to the underlying Monitor Metrics
     * @param rendezvousServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     **/
    public RendezvousServiceMetric deepCopy(RendezvousServiceMonitorFilter rendezvousServiceMonitorFilter) {
        RendezvousServiceMetric serviceMetric = new RendezvousServiceMetric();

        serviceMetric.moduleClassID = moduleClassID;

        serviceMetric.mergeMetrics(this, true, rendezvousServiceMonitorFilter.isIncludeClientConnectionMetrics()
                ,
                rendezvousServiceMonitorFilter.isIncludeRendezvousConnectionMetrics());
        return serviceMetric;	
    } 

    /**
     * {@inheritDoc}
     * <p> This will only merge the designated submetrics
     * @param includeRendezvousMetric Include the basic Rendezvous Metric in the merge
     * @param includeClientConnectionMetrics Include Client Connection Metrics in the merge
     * @param includeRendezvousConnectionMetrics Include Rendezvous Connection Metrics in the merge
     **/
    public void mergeMetrics(ServiceMetric otherServiceMetric, boolean includeRendezvousMetric, boolean includeClientConnectionMetrics, boolean includeRendezvousConnectionMetrics) {
        RendezvousServiceMetric otherRendezvousServiceMetric = (RendezvousServiceMetric) otherServiceMetric;

        if (includeRendezvousMetric) {
            RendezvousMetric otherRendezvousMetric = otherRendezvousServiceMetric.getRendezvousMetric();

            if ((rendezvousMetric == null) && (otherRendezvousMetric != null)) {
                rendezvousMetric = new RendezvousMetric(otherRendezvousMetric);
            }

            if (otherRendezvousMetric != null) { 
                rendezvousMetric.mergeMetrics(otherRendezvousMetric);
            }
        }

        if (includeClientConnectionMetrics) {
            for (Iterator i = otherRendezvousServiceMetric.getClientConnectionMetrics(); i.hasNext();) {
                ClientConnectionMetric otherClientConnectionMetric = (ClientConnectionMetric) i.next();
                ClientConnectionMetric clientConnectionMetric = getClientConnectionMetric(otherClientConnectionMetric.getPeerID());
				
                if (clientConnectionMetric == null) {
                    clientConnectionMetric = new ClientConnectionMetric(otherClientConnectionMetric);
                    addClientConnectionMetric(clientConnectionMetric);
                }
				 
                clientConnectionMetric.mergeMetrics(otherClientConnectionMetric);			
            }
        }

        if (includeRendezvousConnectionMetrics) {
            for (Iterator i = otherRendezvousServiceMetric.getRendezvousConnectionMetrics(); i.hasNext();) {
                RendezvousConnectionMetric otherRendezvousConnectionMetric = (RendezvousConnectionMetric) i.next();
                RendezvousConnectionMetric rendezvousConnectionMetric = getRendezvousConnectionMetric(
                        otherRendezvousConnectionMetric.getPeerID());

                if (rendezvousConnectionMetric == null) {
                    rendezvousConnectionMetric = new RendezvousConnectionMetric(otherRendezvousConnectionMetric);
                    addRendezvousConnectionMetric(rendezvousConnectionMetric);
                }

                rendezvousConnectionMetric.mergeMetrics(otherRendezvousConnectionMetric);			
            }
        }
    }

    /**
     * {@inheritDoc}
     **/
    public void diffMetrics(ServiceMetric otherOne) {
        throw new RuntimeException("Not Supported");
    }

    /**
     * Make a shallow copy of this metric only including the portions designated in the Filter
     * <P> Note: since this is a shallow copy it is dangerous to modify the submetrics
     * @param rendezvousServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     **/
    public RendezvousServiceMetric shallowCopy(RendezvousServiceMonitorFilter rendezvousServiceMonitorFilter) {
        RendezvousServiceMetric rendezvousServiceMetric = new RendezvousServiceMetric(moduleClassID);

        rendezvousServiceMetric.rendezvousMetric = rendezvousMetric;
		
        if (rendezvousServiceMonitorFilter.isIncludeClientConnectionMetrics()) {
            for (Iterator i = getClientConnectionMetrics(); i.hasNext();) {
                ClientConnectionMetric clientConnectionMetric = (ClientConnectionMetric) i.next();

                rendezvousServiceMetric.addClientConnectionMetric(clientConnectionMetric);
            }
        }

        if (rendezvousServiceMonitorFilter.isIncludeRendezvousConnectionMetrics()) {
            for (Iterator i = getRendezvousConnectionMetrics(); i.hasNext();) {
                RendezvousConnectionMetric rendezvousConnectionMetric = (RendezvousConnectionMetric) i.next();

                rendezvousServiceMetric.addRendezvousConnectionMetric(rendezvousConnectionMetric);
            }
        }

        return rendezvousServiceMetric;	
    }	
}
