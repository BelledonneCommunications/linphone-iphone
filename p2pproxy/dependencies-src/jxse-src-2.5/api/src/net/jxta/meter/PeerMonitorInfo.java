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

import java.net.URISyntaxException;

import net.jxta.document.Element;
import net.jxta.id.IDFactory;
import net.jxta.platform.ModuleClassID;
import net.jxta.util.documentSerializable.DocumentSerializable;
import net.jxta.util.documentSerializable.DocumentSerializableUtilities;
import net.jxta.util.documentSerializable.DocumentSerializationException;

import net.jxta.exception.JxtaException;


/**
 * The Monitoring Capabilities of a Local or Remote Peer as a list of ServiceMonitor ClassIDs and supported Reporting Rates.
 **/ 
public class PeerMonitorInfo implements DocumentSerializable {
    public static final PeerMonitorInfo NO_PEER_MONITOR_INFO = new PeerMonitorInfo(false, null, null, 0, 0);
    private boolean allowsMonitoring;
    private ModuleClassID[] moduleClassIDs;
    private long[] reportRates;
    private long lastResetTime;
    private long runningTime;

    /**
     * PeerMonitorInfo
     *
     */
    public PeerMonitorInfo() {}
	
    /**
     * PeerMonitorInfo
     *
     * @param allowsMonitoring
     * @param moduleClassIDs
     * @param reportRates
     */
    public PeerMonitorInfo(boolean allowsMonitoring, ModuleClassID[] moduleClassIDs, long[] reportRates, long lastResetTime, long runningTime) {
        this.allowsMonitoring = allowsMonitoring;
        this.moduleClassIDs = moduleClassIDs;
        this.reportRates = reportRates;
        this.lastResetTime = lastResetTime;
        this.runningTime = runningTime;
    }

    /**
     * Allows Monitoring
     */
    public boolean allowsMonitoring() {
        return allowsMonitoring;
    }

    /**
     * Get Suported Reporting Rates (in Milliseconds)
     *
     * @return long[]
     */
    public long[] getReportRates() {
        return reportRates;
    }

    /**
     * Get Suported Service Monitors as a list of ModuleClassIDs
     */
    public ModuleClassID[] getModuleClassIDs() {
        return moduleClassIDs;
    }
	
    /**
     * Get Time that the Monitor was last Reset (probably same as startTime)
     */
    public long getLastResetTime() {
        return lastResetTime;
    }

    /**
     * Get the running time since the monitor was reset (probably same as upTime)
     */
    public long getRunningTime() {
        return runningTime;
    }

    /**
     * @inheritDoc
     **/
    public void serializeTo(Element element) throws DocumentSerializationException {
        DocumentSerializableUtilities.addBoolean(element, "allowsMonitoring", allowsMonitoring);
        DocumentSerializableUtilities.addLong(element, "lastResetTime", lastResetTime);
        DocumentSerializableUtilities.addLong(element, "runningTime", runningTime);

        if (allowsMonitoring) {
            DocumentSerializableUtilities.addInt(element, "numModuleClassIDs", moduleClassIDs.length);
            for (int i = 0; i < moduleClassIDs.length; i++) {
                ModuleClassID moduleClassID = moduleClassIDs[i];

                DocumentSerializableUtilities.addString(element, "moduleClassID", moduleClassID.toString());
            }

            DocumentSerializableUtilities.addInt(element, "numReportRates", reportRates.length);			
            for (int i = 0; i < reportRates.length; i++) {
                long reportRate = reportRates[i];

                DocumentSerializableUtilities.addLong(element, "reportRate", reportRate);
            }
        }				
    }

    /**
     * @inheritDoc
     **/
    public void initializeFrom(Element element) throws DocumentSerializationException {
        int numModuleClassIDs = DocumentSerializableUtilities.getInt(element, "numModuleClassIDs", 0);

        moduleClassIDs = new ModuleClassID[numModuleClassIDs];
        int moduleClassIDIndex = 0;		

        int numReportRates = DocumentSerializableUtilities.getInt(element, "numReportRates", 0);

        reportRates = new long[numReportRates];	
        int reportRateIndex = 0;
			
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (Element) e.nextElement();
            String key = (String) childElement.getKey();

            if (key.equals("allowsMonitoring")) { 
                allowsMonitoring = DocumentSerializableUtilities.getBoolean(childElement);
            } else if (key.equals("lastResetTime")) { 
                lastResetTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (key.equals("runningTime")) { 
                runningTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (key.equals("reportRate")) {
                long reportRate = DocumentSerializableUtilities.getLong(childElement);

                reportRates[reportRateIndex] = reportRate;
                reportRateIndex++;
            } else if (key.equals("moduleClassID")) {
                try {
                    ModuleClassID moduleClassID = (ModuleClassID) IDFactory.fromURI(
                            new URI(DocumentSerializableUtilities.getString(childElement)));

                    moduleClassIDs[moduleClassIDIndex] = moduleClassID;
                    moduleClassIDIndex++;
                } catch (URISyntaxException jex) {
                    throw new DocumentSerializationException("Can't get ModuleClassID", jex);
                }
            }
        }
					
    }
}
