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

package net.jxta.impl.resolver.resolverMeter;


import net.jxta.meter.*;

import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.platform.*;
import net.jxta.id.*;
import java.util.*;
import net.jxta.util.*;
import net.jxta.exception.*;
import java.net.*;


/**
 * The Service Monitor Metric for the standard Resolver Service
 **/
public class ResolverServiceMetric implements ServiceMetric {
    private ResolverMetric resolverMetric;
    private LinkedList queryHandlerMetrics = new LinkedList();
    private LinkedList srdiHandlerMetrics = new LinkedList();
    private ModuleClassID moduleClassID = MonitorResources.resolverServiceMonitorClassID;

    public ResolverServiceMetric() {}

    public ResolverServiceMetric(ModuleClassID moduleClassID) {
        init(moduleClassID);
    }

    public void init(ModuleClassID moduleClassID) { 
        this.moduleClassID = moduleClassID;
    }

    public ModuleClassID getModuleClassID() {
        return moduleClassID;
    }

    /**
     * Get the General Resolver Metric
     **/
    public ResolverMetric getResolverMetric() {
        return resolverMetric;
    }
	
    void setResolverMetric(ResolverMetric resolverMetric) { 
        this.resolverMetric = resolverMetric; 
    }
	
    /**
     * Add a Query Handler Metric
     **/
    public void addQueryHandlerMetric(QueryHandlerMetric queryHandlerMetric) {
        queryHandlerMetrics.add(queryHandlerMetric);
    }

    /**
     * Get All Query Handler Metrics as an iterator
     **/
    public Iterator getQueryHandlerMetrics() {
        return queryHandlerMetrics.iterator();
    }

    /**
     * Get Query Handler Metrics for the corresponding handler
     * @return Handler or null if not found
     **/
    public QueryHandlerMetric getQueryHandlerMetric(String handlerName) {
        for (Iterator i = queryHandlerMetrics.iterator(); i.hasNext();) {
            QueryHandlerMetric queryHandlerMetric = (QueryHandlerMetric) i.next();

            if (handlerName.equals(queryHandlerMetric.getHandlerName())) {
                return queryHandlerMetric;
            }
        }

        return null;
    }

    /**
     * Add a Srdi Handler Metric
     **/
    public void addSrdiHandlerMetric(SrdiHandlerMetric srdiHandlerMetric) {
        srdiHandlerMetrics.add(srdiHandlerMetric);
    }

    /**
     * Get All Srdi Handler Metrics as an iterator
     **/
    public Iterator getSrdiHandlerMetrics() {
        return srdiHandlerMetrics.iterator();
    }

    /**
     * Get Srdi Handler Metrics for the corresponding handler
     * @return Handler or null if not found
     **/
    public SrdiHandlerMetric getSrdiHandlerMetric(String handlerName) {
        for (Iterator i = srdiHandlerMetrics.iterator(); i.hasNext();) {
            SrdiHandlerMetric srdiHandlerMetric = (SrdiHandlerMetric) i.next();

            if (handlerName.equals(srdiHandlerMetric.getHandlerName())) {
                return srdiHandlerMetric;
            }
        }

        return null;
    }

    public void serializeTo(Element element) throws DocumentSerializationException {

        for (Iterator i = queryHandlerMetrics.iterator(); i.hasNext();) {
            QueryHandlerMetric queryHandlerMetric = (QueryHandlerMetric) i.next();

            DocumentSerializableUtilities.addDocumentSerializable(element, "queryHandlerMetric", queryHandlerMetric);		
        }

        for (Iterator i = srdiHandlerMetrics.iterator(); i.hasNext();) {
            SrdiHandlerMetric srdiHandlerMetric = (SrdiHandlerMetric) i.next();

            DocumentSerializableUtilities.addDocumentSerializable(element, "srdiHandlerMetric", srdiHandlerMetric);		
        }

        if (resolverMetric != null) {
            DocumentSerializableUtilities.addDocumentSerializable(element, "resolverMetric", resolverMetric);
        }	

        if (moduleClassID != null) {
            DocumentSerializableUtilities.addString(element, "moduleClassID", moduleClassID.toString());		
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();
			
            if (tagName.equals("queryHandlerMetric")) {
                QueryHandlerMetric queryHandlerMetric = (QueryHandlerMetric) DocumentSerializableUtilities.getDocumentSerializable(
                        childElement, QueryHandlerMetric.class);

                queryHandlerMetrics.add(queryHandlerMetric);
            }

            if (tagName.equals("srdiHandlerMetric")) {
                SrdiHandlerMetric srdiHandlerMetric = (SrdiHandlerMetric) DocumentSerializableUtilities.getDocumentSerializable(
                        childElement, SrdiHandlerMetric.class);

                srdiHandlerMetrics.add(srdiHandlerMetric);
            }

            if (tagName.equals("resolverMetric")) {
                resolverMetric = (ResolverMetric) DocumentSerializableUtilities.getDocumentSerializable(childElement
                        ,
                        ResolverMetric.class);
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

    public void mergeMetrics(ServiceMetric otherOne) {
        mergeMetrics(otherOne, true, true, true);
    }

    /**
     * Make a deep copy of this metric only including the portions designated in the Filter
     * The resulting metric is Safe to modify without danger to the underlying Monitor Metrics
     * @param resolverServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     **/
    public ResolverServiceMetric deepCopy(ResolverServiceMonitorFilter resolverServiceMonitorFilter) {
        ResolverServiceMetric serviceMetric = new ResolverServiceMetric();

        serviceMetric.moduleClassID = moduleClassID;

        serviceMetric.mergeMetrics(this, true, resolverServiceMonitorFilter.isIncludeQueryHandlerMetrics()
                ,
                resolverServiceMonitorFilter.isIncludeSrdiHandlerMetrics());
        return serviceMetric;	
    } 
	
    public void mergeMetrics(ServiceMetric otherOne, boolean includeResolverMetric, boolean includeQueryHandlerMetrics, boolean includeSrdiHandlerMetrics) {
        ResolverServiceMetric otherResolverServiceMetric = (ResolverServiceMetric) otherOne;

        if (includeResolverMetric) {
            ResolverMetric otherResolverMetric = otherResolverServiceMetric.getResolverMetric();

            if ((resolverMetric == null) && (otherResolverMetric != null)) {
                resolverMetric = new ResolverMetric(otherResolverMetric);
            }

            if (otherResolverMetric != null) { 
                resolverMetric.mergeMetrics(otherResolverMetric);
            }
        }

        if (includeQueryHandlerMetrics) {
            for (Iterator i = otherResolverServiceMetric.getQueryHandlerMetrics(); i.hasNext();) {
                QueryHandlerMetric otherQueryHandlerMetric = (QueryHandlerMetric) i.next();
                QueryHandlerMetric queryHandlerMetric = getQueryHandlerMetric(otherQueryHandlerMetric.getHandlerName());
				
                if (queryHandlerMetric == null) {
                    queryHandlerMetric = new QueryHandlerMetric(otherQueryHandlerMetric);
                    addQueryHandlerMetric(queryHandlerMetric);
                }
				 
                queryHandlerMetric.mergeMetrics(otherQueryHandlerMetric);			
            }
        }

        if (includeSrdiHandlerMetrics) {
            for (Iterator i = otherResolverServiceMetric.getSrdiHandlerMetrics(); i.hasNext();) {
                SrdiHandlerMetric otherSrdiHandlerMetric = (SrdiHandlerMetric) i.next();
                SrdiHandlerMetric srdiHandlerMetric = getSrdiHandlerMetric(otherSrdiHandlerMetric.getHandlerName());
					
                if (srdiHandlerMetric == null) {
                    srdiHandlerMetric = new SrdiHandlerMetric(otherSrdiHandlerMetric);
                    addSrdiHandlerMetric(srdiHandlerMetric);
                }
				 
                srdiHandlerMetric.mergeMetrics(otherSrdiHandlerMetric);			
            }
        }
    }

    /**
     * Make a shallow copy of this metric only including the portions designated in the Filter
     * <P> Note: since this is a shallow copy it is dangerous to modify the submetrics
     * @param resolverServiceMonitorFilter Filter designates constituant parts to be included
     * @return a copy of this metric with references to the designated parts
     **/
    public ResolverServiceMetric shallowCopy(ResolverServiceMonitorFilter resolverServiceMonitorFilter) {
        ResolverServiceMetric resolverServiceMetric = new ResolverServiceMetric(moduleClassID);

        resolverServiceMetric.resolverMetric = resolverMetric;
		
        if (resolverServiceMonitorFilter.isIncludeQueryHandlerMetrics()) {
            for (Iterator i = getQueryHandlerMetrics(); i.hasNext();) {
                QueryHandlerMetric queryHandlerMetric = (QueryHandlerMetric) i.next();

                resolverServiceMetric.addQueryHandlerMetric(queryHandlerMetric);
            }
        }

        if (resolverServiceMonitorFilter.isIncludeSrdiHandlerMetrics()) {
            for (Iterator i = getSrdiHandlerMetrics(); i.hasNext();) {
                SrdiHandlerMetric srdiHandlerMetric = (SrdiHandlerMetric) i.next();

                resolverServiceMetric.addSrdiHandlerMetric(srdiHandlerMetric);
            }
        }

        return resolverServiceMetric;	
    }	

    public void diffMetrics(ServiceMetric otherOne) {
        throw new RuntimeException("Not Supported");
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }
}
