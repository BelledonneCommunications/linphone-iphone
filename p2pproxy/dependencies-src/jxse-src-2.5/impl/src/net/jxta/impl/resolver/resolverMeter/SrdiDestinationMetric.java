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


import net.jxta.peer.*;
import net.jxta.endpoint.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import java.util.*;
import net.jxta.impl.meter.*;

import net.jxta.util.*;


/**
 * Metrics for a specific Srdi Destination for a specific handler 
 **/
public class SrdiDestinationMetric implements DocumentSerializable {
    private PeerID peerID;
    private int messageProcessed;
    private int errorWhileProcessing;
    private int srdiToUnregisteredHandler = 0;

    private int messageSentViaUnicast;

    public SrdiDestinationMetric(PeerID peerID) {
        this.peerID = peerID;
    }
	
    public SrdiDestinationMetric(SrdiDestinationMetric prototype) {
        this.peerID = prototype.peerID;		
    }

    public SrdiDestinationMetric() {}

    public PeerID getPeerID() {
        return peerID;
    }

    void messageProcessed() {
        messageProcessed++;
    }

    /** Messages Received and Processed from this destinations **/
    public int getMessagesProcessed() {
        return messageProcessed;
    }
	
    void errorWhileProcessing() {
        errorWhileProcessing++;
    }

    /** Messages Received, but generating errors when processing **/
    public int getErrorsWhileProcessing() {
        return errorWhileProcessing;
    }
	
    void messageSentViaUnicast() {
        messageSentViaUnicast++;
    }

    /** Get Messages Sent via Unicast to this destinations **/
    public int getMessagesSentViaUnicast() {
        return messageSentViaUnicast;
    }
	
    public void srdiToUnregisteredHandler() {
        srdiToUnregisteredHandler++;
    }	

    /** Messages Received, to this when it was not registered **/
    public int getSrdiToUnregisteredHandler() {
        return srdiToUnregisteredHandler;
    }

    public void serializeTo(Element element) throws DocumentSerializationException {
        if (peerID != null) {
            DocumentSerializableUtilities.addString(element, "peerID", peerID.toString());
        }
        if (messageProcessed != 0) {
            DocumentSerializableUtilities.addInt(element, "messageProcessed", messageProcessed);
        }
        if (errorWhileProcessing != 0) {
            DocumentSerializableUtilities.addInt(element, "errorWhileProcessing", errorWhileProcessing);
        }
        if (srdiToUnregisteredHandler != 0) {
            DocumentSerializableUtilities.addInt(element, "srdiToUnregisteredHandler", srdiToUnregisteredHandler);
        }
        if (messageSentViaUnicast != 0) {
            DocumentSerializableUtilities.addInt(element, "messageSentViaUnicast", messageSentViaUnicast);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("peerID")) {
                String peerIDText = DocumentSerializableUtilities.getString(childElement);

                peerID = MetricUtilities.getPeerIdFromString(peerIDText);
            } else if (tagName.equals("messageProcessed")) { 
                messageProcessed = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("errorWhileProcessing")) { 
                errorWhileProcessing = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("srdiToUnregisteredHandler")) { 
                srdiToUnregisteredHandler = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("messageSentViaUnicast")) { 
                messageSentViaUnicast = DocumentSerializableUtilities.getInt(childElement);
            }
        }
    }

    public void mergeMetrics(SrdiDestinationMetric otherSrdiDestinationMetric) {
        this.errorWhileProcessing += otherSrdiDestinationMetric.errorWhileProcessing;
        this.messageProcessed += otherSrdiDestinationMetric.messageProcessed;
        this.messageSentViaUnicast += otherSrdiDestinationMetric.messageSentViaUnicast;
    }

    @Override
    public int hashCode() {
        return peerID.hashCode();
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof SrdiDestinationMetric) {
            SrdiDestinationMetric otherSrdiDestinationMetric = (SrdiDestinationMetric) other;

            return peerID.equals(otherSrdiDestinationMetric.peerID);
        } else {
            return false;
        }
    }		
}
