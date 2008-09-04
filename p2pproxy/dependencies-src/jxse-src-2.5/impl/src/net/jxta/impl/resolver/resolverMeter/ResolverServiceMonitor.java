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
import net.jxta.impl.meter.*;
import java.util.*;


/**
 * The Service Monitor Metric for the standard Resolver Service
 **/
public class ResolverServiceMonitor extends GenericServiceMonitor {
    public static final String UNKNOWN_HANDLER = "--UNKNOWN-HANDLER--";
	
    private Hashtable queryHandlerMeters = new Hashtable();
    private Hashtable srdiHandlerMeters = new Hashtable();
    private Hashtable destinationMeters = new Hashtable();
    private Hashtable sourceMeters = new Hashtable();
	
    private ResolverMeter resolverMeter = new ResolverMeter(this);
	
    private ResolverServiceMetric cumulativeResolverServiceMetric;

    @Override
    protected void init() {
        cumulativeResolverServiceMetric = (ResolverServiceMetric) getCumulativeServiceMetric();
        cumulativeResolverServiceMetric.setResolverMetric(resolverMeter.getCumulativeMetrics());
    }

    public ResolverMeter getResolverMeter() {
        return resolverMeter;
    }
	
    public synchronized QueryHandlerMeter registerQueryHandlerMeter(String handlerName) {
        QueryHandlerMeter queryHandlerMeter = (QueryHandlerMeter) queryHandlerMeters.get(handlerName);
        
        if (queryHandlerMeter == null) { 
            queryHandlerMeter = addQueryHandlerMeter(handlerName, true);
        }

        queryHandlerMeter.setRegistered(true);

        return queryHandlerMeter;
    }

    public synchronized QueryHandlerMeter addQueryHandlerMeter(String handlerName, boolean registered) {
        QueryHandlerMeter queryHandlerMeter = new QueryHandlerMeter(handlerName, this);

        queryHandlerMeters.put(handlerName, queryHandlerMeter);
        cumulativeResolverServiceMetric.addQueryHandlerMetric(queryHandlerMeter.getCumulativeMetrics());
        queryHandlerMeter.setRegistered(registered);
		
        return queryHandlerMeter;
    }

    public synchronized QueryHandlerMeter unregisterQueryHandlerMeter(String handlerName) {
        QueryHandlerMeter queryHandlerMeter = getQueryHandlerMeter(handlerName);

        queryHandlerMeter.setRegistered(false);

        return queryHandlerMeter;
    }

    public QueryHandlerMeter getQueryHandlerMeter(String handlerName) {
        QueryHandlerMeter queryHandlerMeter = (QueryHandlerMeter) queryHandlerMeters.get(handlerName);
		
        if (queryHandlerMeter == null) {
            queryHandlerMeter = addQueryHandlerMeter(handlerName, false);
        }

        return queryHandlerMeter;
    }

    public synchronized SrdiHandlerMeter registerSrdiHandlerMeter(String handlerName) {
        SrdiHandlerMeter srdiHandlerMeter = (SrdiHandlerMeter) srdiHandlerMeters.get(handlerName);
        
        if (srdiHandlerMeter == null) { 
            srdiHandlerMeter = addSrdiHandlerMeter(handlerName, true);
        }

        srdiHandlerMeter.setRegistered(true);

        return srdiHandlerMeter;
    }

    public synchronized SrdiHandlerMeter addSrdiHandlerMeter(String handlerName, boolean registered) {
        SrdiHandlerMeter srdiHandlerMeter = new SrdiHandlerMeter(handlerName);

        srdiHandlerMeters.put(handlerName, srdiHandlerMeter);
        cumulativeResolverServiceMetric.addSrdiHandlerMetric(srdiHandlerMeter.getCumulativeMetrics());
        srdiHandlerMeter.setRegistered(registered);
		
        return srdiHandlerMeter;
    }

    public synchronized SrdiHandlerMeter unregisterSrdiHandlerMeter(String handlerName) {
        SrdiHandlerMeter srdiHandlerMeter = getSrdiHandlerMeter(handlerName);

        srdiHandlerMeter.setRegistered(false);

        return srdiHandlerMeter;
    }

    public SrdiHandlerMeter getSrdiHandlerMeter(String handlerName) {
        SrdiHandlerMeter srdiHandlerMeter = (SrdiHandlerMeter) srdiHandlerMeters.get(handlerName);
		
        if (srdiHandlerMeter == null) {
            srdiHandlerMeter = addSrdiHandlerMeter(handlerName, false);
        }

        return srdiHandlerMeter;
    }

    @Override
    protected ServiceMetric collectServiceMetrics() {
        ResolverServiceMetric resolverServiceMetric = (ResolverServiceMetric) createServiceMetric();

        boolean anyData = false;
		
        for (Enumeration e = queryHandlerMeters.elements(); e.hasMoreElements();) {
            QueryHandlerMeter queryHandlerMeter = (QueryHandlerMeter) e.nextElement();
            QueryHandlerMetric queryHandlerMetric = queryHandlerMeter.collectMetrics(); // clears delta from meter

            if (queryHandlerMetric != null) {
                resolverServiceMetric.addQueryHandlerMetric(queryHandlerMetric);
                anyData = true;
            }
        }

        for (Enumeration e = srdiHandlerMeters.elements(); e.hasMoreElements();) {
            SrdiHandlerMeter srdiHandlerMeter = (SrdiHandlerMeter) e.nextElement();
            SrdiHandlerMetric srdiHandlerMetric = srdiHandlerMeter.collectMetrics(); // clears delta from meter

            if (srdiHandlerMetric != null) {
                resolverServiceMetric.addSrdiHandlerMetric(srdiHandlerMetric);
                anyData = true;
            }
        }

        ResolverMetric resolverMetric = resolverMeter.collectMetrics();

        if (resolverMetric != null) {
            resolverServiceMetric.setResolverMetric(resolverMetric);
            anyData = true;
        }

        if (anyData) {
            return resolverServiceMetric;
        } else {
            return null;
        }
    }

    @Override
    public ServiceMetric getServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime, int pulseIndex, long reportRate) {
        int deltaReportRateIndex = monitorManager.getReportRateIndex(reportRate);
        ResolverServiceMetric origMetric = (ResolverServiceMetric) deltaServiceMetrics[deltaReportRateIndex];

        if (origMetric == null) {
            return null;
        } 

        ResolverServiceMonitorFilter resolverServiceMonitorFilter = (ResolverServiceMonitorFilter) serviceMonitorFilter;

        return origMetric.shallowCopy(resolverServiceMonitorFilter);
    }

    @Override
    public ServiceMetric getCumulativeServiceMetric(ServiceMonitorFilter serviceMonitorFilter, long fromTime, long toTime) {
        ResolverServiceMonitorFilter resolverServiceMonitorFilter = (ResolverServiceMonitorFilter) serviceMonitorFilter;
        ResolverServiceMetric origMetric = (ResolverServiceMetric) cumulativeServiceMetric;

        return origMetric.deepCopy(resolverServiceMonitorFilter);
    }

    /*
     private ResolverServiceMetric copy(ResolverServiceMetric origMetric) {
     ResolverServiceMetric resolverServiceMetric = new ResolverServiceMetric();
     
     for (Iterator i = origMetric.getQueryHandlerMetrics(); i.hasNext(); ) {
     QueryHandlerMetric queryHandlerMetric = (QueryHandlerMetric)i.next();
     resolverServiceMetric.addQueryHandlerMetric(queryHandlerMetric);
     }

     for (Iterator i = origMetric.getSrdiHandlerMetrics(); i.hasNext(); ) {
     SrdiHandlerMetric srdiHandlerMetric = (SrdiHandlerMetric)i.next();
     resolverServiceMetric.addSrdiHandlerMetric(srdiHandlerMetric);
     }

     return resolverServiceMetric;	
     }
     */			
}
