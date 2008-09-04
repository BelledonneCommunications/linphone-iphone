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


import net.jxta.resolver.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;

import java.util.*;


/**
 * The general Metric for this resolver 
 **/
public class ResolverMetric implements DocumentSerializable {
    private int numInvalidSrdiMessages = 0;
    private int numSrdiMessagesToUnknownHandler = 0;
		
    private int numInvalidResponses = 0;
    private int numResponsesToUnknownHandler = 0;
	
    private int numInvalidQueries = 0;
    private int numQueriesToUnknownHandler = 0;

    public ResolverMetric() {}

    public ResolverMetric(ResolverMetric prototype) {}

    /** Number of invalid Srdi Messages received */
    public int getNumInvalidSrdiMessages() {
        return numInvalidSrdiMessages;
    }

    /** Number of Srdi Messages received for unknown handlers */
    public int getNumSrdiMessagesToUnknownHandler() {
        return numSrdiMessagesToUnknownHandler;
    }

    /** Number of invalid Query Response Messages received */
    public int getNumInvalidResponses() {
        return numInvalidResponses;
    }

    /** Number of Response Messages to unknown handlers received */
    public int getNumResponsesToUnknownHandler() {
        return numResponsesToUnknownHandler;
    }

    /** Number of invalid Query Messages received */
    public int getNumInvalidQueries() {
        return numInvalidQueries;
    }

    /** Number of Query Messages to unknown handlers received */
    public int getNumQueriesToUnknownHandler() {
        return numQueriesToUnknownHandler;
    }

    void invalidSrdiMessageDiscarded() {
        numInvalidSrdiMessages++;
    }
	
    void unknownHandlerForSrdiMessage() {
        numSrdiMessagesToUnknownHandler++;
    }

    void invalidResponseDiscarded() {
        numInvalidResponses++;
    }

    void unknownHandlerForResponse() {
        numResponsesToUnknownHandler++;
    }

    void invalidQueryDiscarded() {
        numInvalidQueries++;
    }

    void unknownHandlerForQuery() {
        numQueriesToUnknownHandler++;
    }	

    public void serializeTo(Element element) throws DocumentSerializationException {
        if (numInvalidSrdiMessages != 0) {
            DocumentSerializableUtilities.addInt(element, "numInvalidSrdiMessages", numInvalidSrdiMessages);
        }
        if (numSrdiMessagesToUnknownHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "numSrdiMessagesToUnknownHandler", numSrdiMessagesToUnknownHandler);
        }
        if (numInvalidResponses != 0) {
            DocumentSerializableUtilities.addInt(element, "numInvalidResponses", numInvalidResponses);
        }
        if (numResponsesToUnknownHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "numResponsesToUnknownHandler", numResponsesToUnknownHandler);
        }
        if (numInvalidQueries != 0) {
            DocumentSerializableUtilities.addInt(element, "numInvalidQueries", numInvalidQueries);
        }
        if (numQueriesToUnknownHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "numQueriesToUnknownHandler", numQueriesToUnknownHandler);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("numInvalidSrdiMessages")) { 
                numInvalidSrdiMessages = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numSrdiMessagesToUnknownHandler")) { 
                numSrdiMessagesToUnknownHandler = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numInvalidResponses")) { 
                numInvalidResponses = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numResponsesToUnknownHandler")) { 
                numResponsesToUnknownHandler = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numInvalidQueries")) { 
                numInvalidQueries = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numQueriesToUnknownHandler")) { 
                numQueriesToUnknownHandler = DocumentSerializableUtilities.getInt(childElement);
            }
        }
    }

    public void mergeMetrics(ResolverMetric otherResolverMetric) {	
        if (otherResolverMetric == null) {
            return;  
        }
        this.numInvalidSrdiMessages += otherResolverMetric.numInvalidSrdiMessages;
        this.numSrdiMessagesToUnknownHandler += otherResolverMetric.numSrdiMessagesToUnknownHandler;

        this.numInvalidResponses += otherResolverMetric.numInvalidResponses;
        this.numResponsesToUnknownHandler += otherResolverMetric.numResponsesToUnknownHandler;

        this.numInvalidQueries += otherResolverMetric.numInvalidQueries;
        this.numQueriesToUnknownHandler += otherResolverMetric.numQueriesToUnknownHandler;

    }
}
