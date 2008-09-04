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
import net.jxta.id.IDFactory;
import net.jxta.meter.MonitorResources;
import net.jxta.meter.ServiceMonitorFilter;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.util.documentSerializable.DocumentSerializableUtilities;
import net.jxta.util.documentSerializable.DocumentSerializationException;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;
import java.util.LinkedList;


public class TransportServiceMonitorFilter implements ServiceMonitorFilter {
    private ModuleClassID moduleClassID = MonitorResources.transportServiceMonitorClassID;
    private LinkedList<String> includedTransports = new LinkedList<String>();
    private boolean includeAllTransports = true;

    public TransportServiceMonitorFilter() {}

    public void init(ModuleClassID moduleClassID) {
        this.moduleClassID = moduleClassID;
    }

    public ModuleClassID getModuleClassID() {
        return moduleClassID;
    }

    public void removeAllTransports() {
        includedTransports.clear();
        includeAllTransports = false;
    }

    public void includeAllTransports(boolean includeAllTransports) {
        this.includeAllTransports = includeAllTransports;
    }

    public void includeTransport(String protocol) {
        includedTransports.add(protocol);
    }

    public boolean hasTransport(String protocol) {
        if (includeAllTransports) {
            return true;
        }

        for (String includedTransport : includedTransports) {
            if (includedTransport.equals(protocol)) {
                return true;
            }
        }
        return false;
    }

    public void includeTransport(ModuleSpecID transportModuleClassID, String subProtocol) {// todo: how is this used???
    }

    public void serializeTo(Element element) throws DocumentSerializationException {
        DocumentSerializableUtilities.addString(element, "moduleClassID", moduleClassID.toString());

        DocumentSerializableUtilities.addBoolean(element, "includeAllTransports", includeAllTransports);

        for (String includedTransport : includedTransports) {
            DocumentSerializableUtilities.addString(element, "includedTransport", includedTransport);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("moduleClassID")) {
                try {
                    moduleClassID = (ModuleClassID) IDFactory.fromURI(
                            new URI(DocumentSerializableUtilities.getString(childElement)));
                } catch (URISyntaxException jex) {
                    throw new DocumentSerializationException("Can't read moduleClassID", jex);
                }
            }
            if (tagName.equals("includedTransport")) {
                String includedTransport = DocumentSerializableUtilities.getString(childElement);

                includedTransports.add(includedTransport);
            }
            if (tagName.equals("includeAllTransports")) {
                includeAllTransports = DocumentSerializableUtilities.getBoolean(childElement);
            }
        }
    }

}

