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


import net.jxta.peer.*;
import net.jxta.peergroup.*;
import net.jxta.resolver.*;
import net.jxta.impl.resolver.*;
import net.jxta.protocol.*;
import net.jxta.endpoint.*;

import java.net.*;
import java.util.*;


public class ResolverMeter {
    private ResolverServiceMonitor resolverServiceMonitor;
    private ResolverMetric cumulativeMetrics;
    private ResolverMetric deltaMetrics;

    public ResolverMeter(ResolverServiceMonitor resolverServiceMonitor) {
        this.resolverServiceMonitor = resolverServiceMonitor;
        cumulativeMetrics = new ResolverMetric();
    }

    public ResolverMetric getCumulativeMetrics() {
        return cumulativeMetrics;
    }

    public synchronized ResolverMetric collectMetrics() {
        ResolverMetric prevDelta = deltaMetrics;

        deltaMetrics = null;
        return prevDelta;
    }
	
    private void createDeltaMetric() {
        deltaMetrics = new ResolverMetric();
    }

    @Override
    public String toString() {
        return "ResolverMeter";
    }

    public void invalidSrdiMessageDiscarded(EndpointAddress src) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.invalidSrdiMessageDiscarded();
        cumulativeMetrics.invalidSrdiMessageDiscarded();		
    }

    public void unknownHandlerForSrdiMessage(EndpointAddress src, String handlerName) {
        if (handlerName != null) {
            SrdiHandlerMeter srdiHandlerMeter = resolverServiceMonitor.getSrdiHandlerMeter(handlerName);

            srdiHandlerMeter.srdiToUnregisteredHandler(src);
        } else { 
            invalidSrdiDiscarded();
        }			
    }

    public void invalidSrdiDiscarded() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.invalidResponseDiscarded();
        cumulativeMetrics.invalidResponseDiscarded();		
    }	
	
    public void invalidResponseDiscarded(EndpointAddress src) {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.invalidResponseDiscarded();
        cumulativeMetrics.invalidResponseDiscarded();		
    }

    public void unknownHandlerForResponse(EndpointAddress src, ResolverResponseMsg resp) {
        String handlerName = resp.getHandlerName();
		
        if (handlerName != null) {
            QueryHandlerMeter queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(handlerName);

            queryHandlerMeter.responseToUnregisteredHandler(src);
        } else { 
            invalidResponseDiscarded(src);
        }			
    }

    public void invalidQueryDiscarded(EndpointAddress src) {
        invalidQueryDiscarded(); // We aren't tracking on source at this point (or ever?)
    }
	
    public void invalidQueryDiscarded() {
        if (deltaMetrics == null) {	
            createDeltaMetric();
        }

        deltaMetrics.invalidQueryDiscarded();
        cumulativeMetrics.invalidQueryDiscarded();		
    }

    public void unknownHandlerForQuery(ResolverQueryMsg query) {
        String handlerName = query.getHandlerName();
		
        if (handlerName != null) {
            QueryHandlerMeter queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(handlerName);

            queryHandlerMeter.queryToUnregisteredHandler(query);
        } else { 
            invalidQueryDiscarded();
        }		
    }
	
    public void queryPropagatedInGroup(ResolverQueryMsg query) {
        String handlerName = query.getHandlerName();
		
        if (handlerName != null) {
            QueryHandlerMeter queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(handlerName);

            queryHandlerMeter.queryPropagatedInGroup();
        } else { 
            invalidQueryDiscarded();
        }	
    }

    public void queryPropagatedViaWalker(ResolverQueryMsg query) {
        String handlerName = query.getHandlerName();
		
        if (handlerName != null) {
            QueryHandlerMeter queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(handlerName);

            queryHandlerMeter.queryPropagatedViaWalker();
        } else { 
            invalidQueryDiscarded();
        }		
    }

    public void propagationQueryDropped(ResolverQueryMsg query) {
        String handlerName = query.getHandlerName();
		
        if (handlerName != null) {
            QueryHandlerMeter queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(handlerName);

            queryHandlerMeter.propagationQueryDropped();
        } else { 
            invalidQueryDiscarded();
        }
    }

    public void queryPropagationError(ResolverQueryMsg query) {
        String handlerName = query.getHandlerName();
		
        if (handlerName != null) {
            QueryHandlerMeter queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(handlerName);

            queryHandlerMeter.unableToPropagate();
        } else { 
            invalidQueryDiscarded();
        }
    }		
}
