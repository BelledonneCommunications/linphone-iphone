

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

package net.jxta.impl.util;


import java.net.URI;
import java.net.URISyntaxException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.TextElement;
import net.jxta.document.XMLDocument;
import net.jxta.id.IDFactory;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleClassAdvertisement;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.ModuleSpecAdvertisement;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PipeAdvertisement;

import net.jxta.impl.peergroup.StdPeerGroup;
import net.jxta.impl.peergroup.StdPeerGroupParamAdv;


/**
 * Advertisements and ID's "cooked" according to recipes lifted
 * from J-C and Frog. Static methods meant for convenience in developing
 * experimental propagation modules (pipe or rendezvous services,
 * rendezvous managers) but maybe generally useful.
 *
 * @author vasha
 */
public class AdvCooker {
    
    /**
     * Reconstructs a ModuleClassID from its String representation
     * as printed by the foregoing recipes.
     *
     * @param url -- the module class id in String form, "urn:jxta:uuid-[the big hex string]"
     * @throws URISyntaxException -- if url is messed up
     * @return -- module class id reconstructed from String
     */
    private static ModuleClassID buildModuleClassID(String uri) throws URISyntaxException {
        return (ModuleClassID) IDFactory.fromURI(new URI(uri));
    }
    
    /** Reconstructs a ModuleSpecID from its String representation
     * as printed by the foregoing recipes.
     *
     * @param url -- the module spec id in String form, "urn:jxta:uuid-[the big hex string]"
     * @throws URISyntaxException -- if url is messed up
     * @return -- module spec id reconstructed from String
     */
    private static ModuleSpecID buildModuleSpecID(String uri) throws URISyntaxException {
        return (ModuleSpecID) IDFactory.fromURI(new URI(uri));
    }
    
    /**
     * The module class advertisement is to simply advertise the
     * existence of a module.
     *
     * @param mcid -- the module class id
     * @param serviceName -- something like "JXTAMOD:JXTA-WIRE-MyNewThing"
     * @param serviceDescription -- something like "JXTA-WIRE MyNewThing Module"
     * @return an appropriate ModuleClassAdvertisement
     */
    public static ModuleClassAdvertisement buildModuleClassAdvertisement(ModuleClassID mcid, String serviceName, String serviceDescription) {
        ModuleClassAdvertisement mcadv = (ModuleClassAdvertisement)
                AdvertisementFactory.newAdvertisement(ModuleClassAdvertisement.getAdvertisementType());

        mcadv.setName(serviceName);
        mcadv.setDescription(serviceDescription);
        mcadv.setModuleClassID(mcid);
        return mcadv;
    }
    
    /**
     * The ModuleSpecAdvertisement has two purposes, to publish
     * the uri of its formal specs for developers and to publish the
     * means of remote access to the module's services if that
     * is appropriate. (See {@link ModuleSpecAdvertisement} )
     * Use this form for a minimal advertisement, suitable
     * for development.
     *
     * @param msid -- the module spec id, "urn:jxta:uuid-[the big hex string]"
     * @param moduleSpecName -- something like "JXTASPEC:JXTA-WIRE-MyNewThing-SPEC"
     * @param moduleSpecDescription -- something like "JXTA-WIRE MyNewThing Specification"
     * @return -- a boilerplate suitable for development.
     */
    public static ModuleSpecAdvertisement buildModuleSpecAdvertisement(ModuleSpecID msid, String moduleSpecName, String moduleSpecDescription) {
        return buildModuleSpecAdvertisement(msid, moduleSpecName, moduleSpecDescription, null, null, null, null, null, null, null);
    }
    
    /**
     * Use this form for production provided remote access is not required.
     *
     * @param msid -- the module spec id, "urn:jxta:uuid-[the big hex string]"
     * @param moduleSpecName -- something like "JXTASPEC:JXTA-WIRE-MyNewThing-SPEC"
     * @param moduleSpecDescription -- something like "JXTA-WIRE MyNewThing Specification"
     * @param creator -- something like "jxta.org"
     * @param version -- something like "Version 1.0"
     * @param specURI -- where to locate the formal specs, e.g. "http://www.jxta.org/MyNewThing"
     * @return -- a fully populated advert suitable if remote access is not required.
     */
    public static ModuleSpecAdvertisement buildModuleSpecAdvertisement(ModuleSpecID msid, String moduleSpecName, String moduleSpecDescription, String creator, String version, String specURI) {
        return buildModuleSpecAdvertisement(msid, moduleSpecName, moduleSpecDescription, creator, version, specURI, null, null
                ,
                null, null);
    }
    
    /**
     * Use this form for a fully populated advert.
     *
     * @param msid -- the module spec id, "urn:jxta:uuid-[the big hex string]"
     * @param moduleSpecName -- something like "JXTASPEC:JXTA-WIRE-MyNewThing-SPEC"
     * @param moduleSpecDescription -- something like "JXTA-WIRE MyNewThing Specification"
     * @param creator -- something like "jxta.org"
     * @param version -- something like "Version 2.0"
     * @param specURI -- where to locate the formal specs, e.g. "http://www.jxta.org/MyNewThing"
     * @param pipeAdv -- to make the module useable remotely (see {@link ModuleSpecAdvertisement})
     * @param proxySpecID -- sometimes required for remote use (see {@link ModuleSpecAdvertisement})
     * @param authorizationSpecID -- sometimes required for remote use (see {@link ModuleSpecAdvertisement})
     * @param param -- anything else
     * @return -- a fully populated advert specifying remote access to module services.
     */
    public static ModuleSpecAdvertisement buildModuleSpecAdvertisement(ModuleSpecID msid, String moduleSpecName, String moduleSpecDescription, String creator, String version, String specURI, PipeAdvertisement pipeAdv, ModuleSpecID proxySpecID, ModuleSpecID authorizationSpecID, StructuredDocument param) {
        ModuleSpecAdvertisement msadv = (ModuleSpecAdvertisement)
                AdvertisementFactory.newAdvertisement(ModuleSpecAdvertisement.getAdvertisementType());

        msadv.setModuleSpecID(msid);
        msadv.setName(moduleSpecName);
        msadv.setDescription(moduleSpecDescription);
        msadv.setCreator(creator == null ? "jxta.org" : creator);
        msadv.setVersion(version == null ? "Version 2.0" : version);
        msadv.setSpecURI(specURI == null ? "http://www.jxta.org/" + moduleSpecName : specURI);
        if (pipeAdv != null) {
            msadv.setPipeAdvertisement(pipeAdv);
        }
        if (proxySpecID != null) {
            msadv.setProxySpecID(proxySpecID);
        }
        if (authorizationSpecID != null) {
            msadv.setAuthSpecID(authorizationSpecID);
        }
        if (param != null) {
            msadv.setParam(param);
        }
        return msadv;
    }
    
    /**
     * Compat's (compatibility statements) serve to narrow the search
     *  for a ModuleImplAdvertisement. Basically you want something
     *  compatible with your group's implementation. Use this form for
     *  compatibilty with the current StdPeerGroup.
     *
     *  @return -- boilerplate compat for StdPeerGroup
     */
    public static XMLDocument buildCompat() {
        
        try {
            PeerGroup wpg = PeerGroup.globalRegistry.lookupInstance(PeerGroupID.worldPeerGroupID);
        
            ModuleImplAdvertisement implAdv = wpg.getAllPurposePeerGroupImplAdvertisement();
            
            wpg.unref();
            
            XMLDocument compat = (XMLDocument) implAdv.getCompat();
            
            return compat;
        } catch (Exception e) {
            // but if it doesn't work default to what was current on Feb 22 2006.
            return buildCompat("JDK1.4.1", "V2.0 Ref Impl");
        }
    }
    
    /**
     * Use this form for customized compatibility statements.
     * Alternatively a group's compat is accessible via
     * group.getCompat()
     *
     * @param efmt -- something like "JDK1.4"
     * @param bind -- something like "V1.0 Ref Impl"
     * @return -- custom compatibility tag
     */
    public static XMLDocument buildCompat(String efmt, String bind) {
        XMLDocument doc = (XMLDocument)
                StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Comp");
        
        Element e = doc.createElement("Efmt", efmt);

        doc.appendChild(e);
        e = doc.createElement("Bind", bind);
        doc.appendChild(e);
        
        return doc;
    }
    
    /**
     * A ModuleImplAdvertisement represents one of any number of
     * published implementations of a given specification. Use this form
     * with for a development boilerplate. Use buildCompat() for a compat
     * boilerplate.
     * (See {@link ModuleImplAdvertisement}.)
     *
     * @param msid -- the module spec id
     * @param code -- the module's fully qualified classname, "net.jxta.impl.wire.MyNewThing"
     * @param compat -- a compatibility statement. Use buildCompat() for a boilerplate.
     * @return -- a development boilerplate with custom compatibility.
     */
    public static ModuleImplAdvertisement buildModuleImplAdvertisement(ModuleSpecID msid, String code, Element compat) {
        ModuleImplAdvertisement miadv = (ModuleImplAdvertisement)
                AdvertisementFactory.newAdvertisement(ModuleImplAdvertisement.getAdvertisementType());

        miadv.setCompat(compat);
        miadv.setModuleSpecID(msid);
        miadv.setCode(code);
        miadv.setDescription(code + " Module, J2SE Implementation");
        miadv.setProvider("jxta.org");
        miadv.setUri("http://download.jxta.org");
        return miadv;
    }
    
    /** Use this form to fully populate a ModuleImplAdvertisement.
     * A ModuleImplAdvertisement has an optional field, "param" which is
     * neglected here. If needed it should be set with advert's setParam method.
     * (See {@link ModuleImplAdvertisement}.)
     *
     * @param msid -- the module spec id
     * @param code -- the module's fully qualified classname, "net.jxta.impl.wire.MyNewThing"
     * @param compat -- a compatibility statement
     * @param description -- something like "MyNewThing Module, J2SE Implementation"
     * @param provider -- something like "jxta.org"
     * @param uri -- currently ornamental, eventually where to find binaries.
     * @return -- a custom advert, fully populated except for "param" field.
     */
    public static ModuleImplAdvertisement buildModuleImplAdvertisement(ModuleSpecID msid, String code, Element compat, String description, String provider, String uri) {
        ModuleImplAdvertisement miadv = buildModuleImplAdvertisement(msid, code, compat);

        miadv.setDescription(description);
        miadv.setProvider(provider);
        miadv.setUri(uri);
        return miadv;
    }
    
    /**
     * Modifies a copy of the parent's implementation
     * advertisement to reflect the addition or replacement of
     * services.  The newServices Hashtable must have ModuleClassID
     * keys and ModuleImplAdvertisement values. I've deferred adding
     * applications or protocols for the moment --vasha@jxta.org Dec 3 2001.
     *
     * @return -- advert for the new peergroup which the StdPeerGroup module will implement.
     * @param parent -- a running instance of the new group's parent
     * @param newGroupModuleSpecID -- since new or replacement services are involved
     * @param newDescription -- the new group's reason to be
     * @param newServices -- advert's for the new services
     * @throws IllegalArgumentException -- for a bad key or value type
     * @throws Exception --- if the parent can't produce a copy of its own impl advert
     */
    public static ModuleImplAdvertisement buildPeerGroupImplAdvertisement(StdPeerGroup parent, ModuleSpecID newGroupModuleSpecID, String newDescription, Map newServices) throws Exception {
        Map newApps = null, newProtos = null;

        // illegal types will cause an IllegalArgumentException
        typeCheckKeys(newServices);
        typeCheckValues(newServices);
        // get a copy of parent's general purpose advert as a template
        ModuleImplAdvertisement implAdv = parent.getAllPurposePeerGroupImplAdvertisement();

        implAdv.setDescription(newDescription);
        implAdv.setModuleSpecID(newGroupModuleSpecID);
        // extract embedded ad for standard modules
        TextElement paramElement = (TextElement) implAdv.getParam();
        StdPeerGroupParamAdv paramAdv = new StdPeerGroupParamAdv(paramElement);
        // alter services
        Map services = paramAdv.getServices();

        typeCheckKeys(services);
        // mergeTables will override old services with new if base classes are the same
        services = mergeTables(services, newServices);
        paramAdv.setServices(services);
        paramElement = (TextElement) paramAdv.getDocument(MimeMediaType.XMLUTF8);
        implAdv.setParam(paramElement);
        return implAdv;
    }
    
    public static ModuleImplAdvertisement buildPeerGroupImplAdvertisement(PeerGroup parent, ModuleSpecID newGroupModuleSpecID, String newDescription, Map newServices, Map newApps) throws Exception {
        Map newProtos = null;

        // illegal types will cause an IllegalArgumentException
        typeCheckKeys(newServices);
        typeCheckValues(newServices);
        typeCheckKeys(newApps);
        typeCheckValues(newApps);
        
        // get a copy of parent's general purpose advert as a template
        ModuleImplAdvertisement implAdv = parent.getAllPurposePeerGroupImplAdvertisement();

        implAdv.setDescription(newDescription);
        implAdv.setModuleSpecID(newGroupModuleSpecID);
        // extract embedded ad for standard modules
        TextElement paramElement = (TextElement) implAdv.getParam();
        StdPeerGroupParamAdv paramAdv = new StdPeerGroupParamAdv(paramElement);
        
        // alter services
        Map services = paramAdv.getServices();

        typeCheckKeys(services);
        // mergeTables will override old services with new if base classes are the same
        services = mergeTables(services, newServices);
        paramAdv.setServices(services);
        
        // alter apps
        Map apps = paramAdv.getApps();

        typeCheckKeys(apps);
        apps = mergeTables(apps, newApps);
        paramAdv.setApps(apps);
        
        paramElement = (TextElement) paramAdv.getDocument(MimeMediaType.XMLUTF8);
        implAdv.setParam(paramElement);
        
        return implAdv;
    }
    
    public static ModuleImplAdvertisement buildPeerGroupImplAdvertisement(PeerGroup parent, ModuleSpecID newGroupModuleSpecID, String newDescription, Map newServices, Map newApps, Map newProtos) throws Exception {
        
        // illegal types will cause an IllegalArgumentException
        typeCheckKeys(newServices);
        typeCheckValues(newServices);
        typeCheckKeys(newApps);
        typeCheckValues(newApps);
        typeCheckKeys(newProtos);
        typeCheckValues(newProtos);
        
        // get a copy of parent's general purpose advert as a template
        ModuleImplAdvertisement implAdv = parent.getAllPurposePeerGroupImplAdvertisement();

        implAdv.setDescription(newDescription);
        implAdv.setModuleSpecID(newGroupModuleSpecID);
        // extract embedded ad for standard modules
        TextElement paramElement = (TextElement) implAdv.getParam();
        StdPeerGroupParamAdv paramAdv = new StdPeerGroupParamAdv(paramElement);
        
        // alter services
        Map services = paramAdv.getServices();

        typeCheckKeys(services);
        // mergeTables will override old services with new if base classes are the same
        services = mergeTables(services, newServices);
        paramAdv.setServices(services);
        
        // alter apps
        Map apps = paramAdv.getApps();

        typeCheckKeys(apps);
        apps = mergeTables(apps, newApps);
        paramAdv.setApps(newApps);
        
        // alter protos
        Map protos = paramAdv.getProtos();

        typeCheckKeys(protos);
        apps = mergeTables(protos, newProtos);
        paramAdv.setProtos(newProtos);
        
        paramElement = (TextElement) paramAdv.getDocument(MimeMediaType.XMLUTF8);
        implAdv.setParam(paramElement);

        return implAdv;
    }
    
    /**
     * Module table vaules must be ModuleImplAdvertisements here.
     * Though StdPeerGroup allows for values of type ModuleSpecID,
     * the context in which they seem to apply is not our context of adding
     * or replacing modules, so I've prohibited them. --vasha@jxta.org dec 3 2001.
     *
     * @param moduleTable -- a Hashtable of services, applications or protocols.
     * @throws IllegalArgumentException -- for an invalid key or value type
     */
    public static void typeCheckValues(Map moduleTable) {
        String badVal = "Module table value not a ModuleImplAdvertisement ";
        Iterator keys = moduleTable.keySet().iterator();

        while (keys.hasNext()) {
            // Tables allow for ModuleSpecID values when, as I understand it,
            // they can load the module from the parent. I'm insisting that
            // NEW or ALTERNATIVE modules supply a ModuleImplAdvertisement.
            Object value = moduleTable.get(keys.next());
            boolean legalValue = value instanceof ModuleImplAdvertisement;

            if (!legalValue) {
                throw(new IllegalArgumentException(badVal + value));
            }
        }
    }
    
    /**
     * Module table keys must be ModuleClassID's.
     *
     * @param moduleTable -- a Hashtable of services, applications or protocols.
     * @throws IllegalArgumentException -- for an invalid key or value type
     */
    public static void typeCheckKeys(Map moduleTable) {
        String badKey = "Module table key not a ModuleClassID ";
        Iterator keys = moduleTable.keySet().iterator();

        while (keys.hasNext()) {
            Object key = keys.next();
            boolean legalKey = key instanceof ModuleClassID;

            if (!legalKey) {
                throw(new IllegalArgumentException(badKey + key));
            }
        }
    }
    
    /**
     * Merge two hashtables of servcices, overwriting old with new if
     * they have the same base class id.
     *
     * @oldServices --service table of a parent group
     * @newServices --services to be added or substituted
     * @return --merged table
     */
    private static Map mergeTables(Map oldServices, Map newServices) {
        // just use brute force; we won't be doing it that often
        Map mergedServices = new HashMap(oldServices);
        Iterator newKeys = newServices.keySet().iterator();

        while (newKeys.hasNext()) {
            ModuleClassID key = (ModuleClassID) newKeys.next();
            Iterator oldKeys = oldServices.keySet().iterator();

            while (oldKeys.hasNext()) {
                ModuleClassID oldkey = (ModuleClassID) oldKeys.next();

                if (oldkey.isOfSameBaseClass(key)) {
                    mergedServices.remove(oldkey);
                }
            }
            mergedServices.put(key, newServices.get(key));
        }
        return mergedServices;
    }
}
