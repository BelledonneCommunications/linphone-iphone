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

package net.jxta.impl.peergroup;


import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLElement;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleImplAdvertisement;

import java.net.URI;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * Not actually an advertisement, but often acts as part of one.
 *
 * @deprecated This internal class will eventually be removed. It has several
 * problems which make it difficult to support. (The most obvious that it 
 * provides poor abstraction and provides references to its' own internal data
 * structures). This class is expected to be replaced by a public API class
 * performing a similar function though such an alternative is not yet available.
 * You are encouraged to copy this code into your own application or service if
 * if you depend upon it.
 */
@Deprecated
public class StdPeerGroupParamAdv {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(StdPeerGroupParamAdv.class.getName());

    private static final String PARAM_TAG = "Parm";
    private static final String PROTO_TAG = "Proto";
    private static final String APP_TAG = "App";
    private static final String SVC_TAG = "Svc";
    private static final String MCID_TAG = "MCID";
    private static final String MSID_TAG = "MSID";
    private static final String MIA_TAG = ModuleImplAdvertisement.getAdvertisementType();

    // In the future we should be able to manipulate all modules regardless of 
    // their kind, but right now it helps to keep them categorized as follows.
    
    /**
     * The services which will be loaded for this peer group.
     * <p/>
     * <ul>
     *     <li>Keys are {@link net.jxta.platform.ModuleClassID}.</li>
     *     <li>Values are {@link net.jxta.platform.ModuleSpecID} or
     *     {@link net.jxta.protocol.ModuleImplAdvertisement}.</li>
     * </ul>
     */    
    private final Map<ModuleClassID, Object> services = new HashMap<ModuleClassID, Object>();
    
    /**
     * The protocols (message transports) which will be loaded for this peer
     * group.
     * <p/>
     * <ul>
     *     <li>Keys are {@link net.jxta.platform.ModuleClassID}.</li>
     *     <li>Values are {@link net.jxta.platform.ModuleSpecID} or
     *     {@link net.jxta.protocol.ModuleImplAdvertisement}.</li>
     * </ul>
     */    
    private final Map<ModuleClassID, Object> transports = new HashMap<ModuleClassID, Object>();
    
    /**
     * The applications which will be loaded for this peer group.
     * <p/>
     * <ul>
     *     <li>Keys are {@link net.jxta.platform.ModuleClassID}.</li>
     *     <li>Values are {@link net.jxta.platform.ModuleSpecID} or
     *     {@link net.jxta.protocol.ModuleImplAdvertisement}.</li>
     * </ul>
     */    
    private final Map<ModuleClassID, Object> apps = new HashMap<ModuleClassID, Object>();

    /**
     * Private constructor for new instances.
     */
    public StdPeerGroupParamAdv() {
    }

    /**
     * Private constructor for serialized instances.
     *
     * @param root the root element
     */
    public StdPeerGroupParamAdv(Element root) {
        if (!(root instanceof XMLElement)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
        }
        initialize((XMLElement) root);
    }

    /**
     * Private constructor for xml serialized instances.
     *
     * @param doc The XML serialization of the advertisement.
     */
    public StdPeerGroupParamAdv(XMLElement doc) {
        initialize(doc);
    }

    /**
     * Add a service to the set of services entries described in this
     * Advertisement.
     *
     * @param mcid   The module class id of the module being added.
     * @param module The module being added.
     */
    public void addService(ModuleClassID mcid, Object module) {
        if(null == mcid) {
            throw new IllegalArgumentException("Illegal ModuleClassID");
        }
        
        if(null == module) {
            throw new IllegalArgumentException("Illegal module");
        }
        
        services.put(mcid, module);
    }

    /**
     * Return the services entries described in this Advertisement.
     * <p/>
     * The result (very unwisely) is the internal map of this
     * Advertisement. Modifying it results in changes to this Advertisement.
     * For safety the Map should be copied before being modified.
     *
     * @return the services entries described in this Advertisement.
     */
    public Map<ModuleClassID, Object> getServices() {
        return services;
    }

    /**
     * Add a protocol (message transport) to the set of protocol entries
     * described in this Advertisement.
     *
     * @param mcid   The module class id of the module being added.
     * @param module The module being added.
     */
    public void addProto(ModuleClassID mcid, Object module) {
        if(null == mcid) {
            throw new IllegalArgumentException("Illegal ModuleClassID");
        }
        
        if(null == module) {
            throw new IllegalArgumentException("Illegal module");
        }
        
        transports.put(mcid, module);
    }

    /**
     * Return the protocols (message transports) entries described in this Advertisement.
     * <p/>
     * The result (very unwisely) is the internal map of this Advertisement.
     * Modifying it results in changes to this Advertisement.
     * For safety the Map should be copied before being modified.
     *
     * @return the protocols (message transports) entries described in this Advertisement.
     */
    public Map<ModuleClassID, Object> getProtos() {
        return transports;
    }

    /**
     * Add an application to the set of application entries described in this
     * Advertisement.
     *
     * @param mcid   The module class id of the module being added.
     * @param module The module being added.
     */
    public void addApp(ModuleClassID mcid, Object module) {
        if(null == mcid) {
            throw new IllegalArgumentException("Illegal ModuleClassID");
        }
        
        if(null == module) {
            throw new IllegalArgumentException("Illegal module");
        }
        
        apps.put(mcid, module);
    }

    /**
     * Return the application entries described in this Advertisement.
     * <p/>
     * The result (very unwisely) is the internal map of this Advertisement. 
     * Modifying it results in changes to this Advertisement.
     * For safety the Map should be copied before being modified.
     *
     * @return the application entries described in this Advertisement.
     */
    public Map<ModuleClassID, Object> getApps() {
        return apps;
    }

    /**
     * Replaces the table of services described by this Advertisement. All
     * existing entries are lost.
     *
     * @param servicesTable the services table
     */
    public void setServices(Map<ModuleClassID, Object> servicesTable) {
        if(servicesTable.containsKey(null)) {
            throw new IllegalArgumentException("null key in servicesTable");
        }
        
        if(servicesTable.containsValue(null)) {
            throw new IllegalArgumentException("null value in servicesTable");
        }        
        
        if (servicesTable == this.services) {
            return;
        }

        this.services.clear();

        if (null != servicesTable) {
            this.services.putAll(servicesTable);
        }
    }

    /**
     * Replaces the table of protocols described by this Advertisement. All
     * existing entries are lost.
     *
     * @param protosTable The message transport descriptors for the group.
     */
    public void setProtos(Map<ModuleClassID, Object> protosTable) {
        if(protosTable.containsKey(null)) {
            throw new IllegalArgumentException("null key in protosTable");
        }
        
        if(protosTable.containsValue(null)) {
            throw new IllegalArgumentException("null value in protosTable");
        }        
        
        if (protosTable == this.transports) {
            return;
        }

        this.transports.clear();

        if (null != protosTable) {
            this.transports.putAll(protosTable);
        }
    }

    /**
     * Replaces the table of applications described by this Advertisement. All
     * existing entries are lost.
     *
     * @param appsTable The application descriptors for the group.
     */
    public void setApps(Map<ModuleClassID, Object> appsTable) {
        if(appsTable.containsKey(null)) {
            throw new IllegalArgumentException("null key in appsTable");
        }
        
        if(appsTable.containsValue(null)) {
            throw new IllegalArgumentException("null value in appsTable");
        }        
        
        if (appsTable == this.apps) {
            return;
        }

        this.apps.clear();

        if (null != appsTable) {
            this.apps.putAll(appsTable);
        }
    }

    private void initialize(XMLElement doc) {
        if (!doc.getName().equals(PARAM_TAG)) {
            throw new IllegalArgumentException("Can not construct " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        // set defaults
        int appCount = 0;
        Enumeration<XMLElement> modules = doc.getChildren();

        while (modules.hasMoreElements()) {
            XMLElement module = modules.nextElement();
            String tagName = module.getName();

            Map<ModuleClassID, Object> theTable;

            if (SVC_TAG.equals(tagName)) {
                theTable = services;
            } else if (APP_TAG.equals(tagName)) {
                theTable = apps;
            } else if (PROTO_TAG.equals(tagName)) {
                theTable = transports;
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Unhandled top-level tag : " + tagName);
                }
                continue;
            }

            ModuleSpecID specID = null;
            ModuleClassID classID = null;
            ModuleImplAdvertisement inLineAdv = null;

            try {
                if (module.getTextValue() != null) {
                    specID = (ModuleSpecID) IDFactory.fromURI(new URI(module.getTextValue()));
                }

                // Check for children anyway.
                Enumeration<XMLElement> fields = module.getChildren();

                while (fields.hasMoreElements()) {
                    XMLElement field = fields.nextElement();

                    String fieldName = field.getName();

                    if (MCID_TAG.equals(fieldName)) {
                        classID = (ModuleClassID) IDFactory.fromURI(new URI(field.getTextValue()));
                    } else if (MSID_TAG.equals(field.getName())) {
                        specID = (ModuleSpecID) IDFactory.fromURI(new URI(field.getTextValue()));
                    } else if (MIA_TAG.equals(field.getName())) {
                        inLineAdv = (ModuleImplAdvertisement) AdvertisementFactory.newAdvertisement(field);
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Unhandled field : " + fieldName);
                        }
                    }
                }
            } catch (Exception any) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Broken entry; skipping", any);
                }
                continue;
            }

            if (inLineAdv == null && specID == null) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Insufficent entry; skipping");
                }
                continue;
            }

            Object theValue;

            if (inLineAdv != null) {
                specID = inLineAdv.getModuleSpecID();
                theValue = inLineAdv;
            } else {
                theValue = specID;
            }

            if (classID == null) {
                classID = specID.getBaseClass();
            }

            // For applications, the role does not matter. We just create a 
            // unique role ID on the fly.
            // When outputing the adv we get rid of it to save space.

            if (theTable == apps) {
                // Only the first (or only) one may use the base class.
                if (classID == PeerGroup.applicationClassID) {
                    if (appCount++ != 0) {
                        classID = IDFactory.newModuleClassID(classID);
                    }
                }
            }
            theTable.put(classID, theValue);
        }
    }

    /**
     * {@inheritDoc}
     */
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(encodeAs, PARAM_TAG);

        outputModules(doc, services, SVC_TAG);
        outputModules(doc, transports, PROTO_TAG);
        outputModules(doc, apps, APP_TAG);

        return doc;
    }

    private void outputModules(StructuredDocument doc, Map<ModuleClassID, Object> modulesTable, String mainTag) {

        for (Map.Entry<ModuleClassID, Object> entry : modulesTable.entrySet()) {
            ModuleClassID mcid = entry.getKey();
            Object val = entry.getValue();
            Element m;

            if(null == mcid) {
                throw new IllegalStateException("null ModuleClassID in " + mainTag );
            }
            
            // For applications, we ignore the role ID. It is not meaningfull,
            // and a new one is assigned on the fly when loading this adv.

            if (val instanceof Advertisement) {
                m = doc.createElement(mainTag);
                doc.appendChild(m);

                if (modulesTable != apps && !mcid.equals(mcid.getBaseClass())) {
                    // It is not an app and there is a role ID. Output it.
                    Element i = doc.createElement(MCID_TAG, mcid.toString());

                    m.appendChild(i);
                }

                StructuredDocument advdoc = (StructuredDocument) ((Advertisement) val).getDocument(doc.getMimeType());

                StructuredDocumentUtils.copyElements(doc, m, advdoc);
            } else if (val instanceof ModuleSpecID) {
                if (modulesTable == apps || mcid.equals(mcid.getBaseClass())) {
                    // Either it is an app or there is no role ID.
                    // So the specId is good enough.
                    m = doc.createElement(mainTag, val.toString());
                    doc.appendChild(m);
                } else {
                    // The role ID matters, so the classId must be separate.
                    m = doc.createElement(mainTag);
                    doc.appendChild(m);

                    Element i;

                    i = doc.createElement(MCID_TAG, mcid.toString());
                    m.appendChild(i);

                    i = doc.createElement(MSID_TAG, val.toString());
                    m.appendChild(i);
                }
            } else {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe("unsupported descriptor for " + mcid + " in " + mainTag +" module table : " + val);
                }
                throw new IllegalStateException("unsupported descriptor for " + mcid + " in " + mainTag +" module table : " + val);
            }
        }
    }
}
