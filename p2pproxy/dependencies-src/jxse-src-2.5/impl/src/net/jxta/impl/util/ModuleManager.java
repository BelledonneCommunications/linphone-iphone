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


import java.util.Enumeration;
import java.util.Hashtable;
import java.io.IOException;

import net.jxta.platform.Module;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.TextElement;
import net.jxta.protocol.ModuleClassAdvertisement;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.ModuleSpecAdvertisement;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.id.IDFactory;


/**
 * Module Manager.
 *
 * This class allows to manage modules to be loaded, started and stopped
 * within a PeerGroup. Modules that are loaded using the ModuleManager do not need
 * to be listed within the PeerGroup advertisement, nor do they have to have
 * published their ModuleSpec and ModuleImpl advertisements: the ModuleManager
 * takes care of this task. However, other peers which may want to load the Module
 * will also have to use its own loader (or the ModuleManager itself, of course):
 * the ModuleManager only manages Modules on the local peer.
 *
 * The Module Manager allows, as an option, to use an application provided class loader.
 * The default class loader is the PeerGroup class loader.
 *
 * The following example shows how to use the ModuleManager:
 *
 *
 * <pre>
 *      // Get the peergroup
 *      PeerGroup group = getMyPeerGroup();
 *      // Get the ModuleManager
 *      ModuleManager moduleManager = ModuleManager.getModuleManager (group);
 *
 *      // Is the Module already loaded ?
 *      Module module = moduleManager.lookupModule ("SampleModule");
 *      if (module == null) {
 *          // SampleModue is not loaded yet. Load it now.
 *          module = moduleManager.loadModule ( "SampleModule", "net.jxta.app.SampleModule.SampleModule");
 *      }
 *
 *      // Start SampleModule
 *      moduleManager.startModule ("SampleModule", moduleArgs);
 * </pre>
 */

public class ModuleManager {

    private static Hashtable<PeerGroupID, ModuleManager> managers = null;
    private static long LOCAL_ADV_TTL = 5 * 60 * 1000;
    // 5 minutes is more than sufficient
    private static long REMOTE_ADV_TTL = 0;
    // We do not allow remote access of the advertisements.

    private final Hashtable<String, ModuleDesc> modules = new Hashtable<String, ModuleDesc>();
    private final PeerGroup group;

    /**
     * Private constructor that allows to create an instance of the Module Manager for each
     * PeerGroup.
     *
     * @param  group  the PeerGroup for which the ModuleManager needs to allocated a new instance
     * of itself.
     */
    private ModuleManager(PeerGroup group) {
        this.group = group;
    }

    /**
     * startModule
     *
     * This method is invoked by the application to start a previously loaded
     * module.
     *
     * @param  moduleName  is the symbolic name of the module.
     * @param  args        is an array of String containing optional arguments for the module. This
     * array is passed directly to the startApp (String[] ) method of the Module.
     */
    public void startModule(String moduleName, String[] args) {

        ModuleDesc moduleDesc = modules.get(moduleName);

        if (moduleDesc == null) {
            // Cannot find such a module
            return;
        }
        moduleDesc.startApp(args);
    }

    /**
     * stopModule
     *
     * This method is invoked by the application to stop a running module.
     *
     * @param  moduleName  is the symbolic name of the module.
     */
    public void stopModule(String moduleName) {

        ModuleDesc moduleDesc = modules.get(moduleName);

        if (moduleDesc == null) {
            // Cannot find such a module
            return;
        }
        moduleDesc.stopApp();
    }

    /**
     * getModuleManager
     *
     * This method is used in order to get the instance of the ModuleManager for a given
     * PeerGroup. getModuleManager will create a new instance automatically if there is no
     * instance for the given PeerGroup.
     *
     * @param  group  the PeerGroup for which the ModuleManager is asked.
     * @return        the ModuleManager instance for the given PeerGroup.
     */
    public static ModuleManager getModuleManager(PeerGroup group) {

        if (managers == null) {
            // This is the first time the ModuleManager is invoked. Create
            // the Hashtable
            managers = new Hashtable<PeerGroupID, ModuleManager>();
        }
        ModuleManager manager;

        manager = managers.get(group.getPeerGroupID());

        if (manager == null) {
            manager = new ModuleManager(group);
            managers.put(group.getPeerGroupID(), manager);
        }
        return manager;
    }

    /**
     *  Description of the Method
     *
     * @param  moduleName  Description of the Parameter
     * @param  module      Description of the Parameter
     * @return             Description of the Return Value
     */
    private synchronized boolean registerModule(String moduleName, Module module) {

        ModuleDesc moduleDesc = modules.get(moduleName);

        if (moduleDesc != null) {
            // There is already a module registered to that name.
            return false;
        }
        moduleDesc = new ModuleDesc(module);
        modules.put(moduleName, moduleDesc);
        return true;
    }

    /**
     * lookupModule
     *
     * Get the Module from its symbolic name.
     *
     * @param  moduleName  symbolic name of the Module
     * @return             the Module for the given name. null is returned if there is no module
     * of the given name.
     */
    public synchronized Module lookupModule(String moduleName) {

        ModuleDesc moduleDesc = modules.get(moduleName);

        if (moduleDesc == null) {
            // There is not any module registered to that name.
            return null;
        }
        return moduleDesc.module;
    }

    /**
     * loadModule
     *
     * Loads a Module. A class loaded is provided by the application.
     * If the module has already been loaded, the existing Module is returned.
     *
     * @param  moduleName  symbolic name of the Module
     * @param  loader      application provided class loader
     * @return             the Module for the given name. null is returned if the module could not be
     * loaded
     */

    public synchronized Module loadModule(String moduleName, ModuleManagerLoader loader) {

        // First check if the module is already loaded and registered
        Module module = lookupModule(moduleName);

        if (module != null) {
            return module;
        }
        module = loader.loadModule(moduleName);
        if (module != null) {
            // Since this module is not started by the standard
            // JXTA PeerGroup, we need to initialize ourself.
            // Note that the ID and the ModuleImplAdvertisement is
            // then set to null, which is fine, since that has been
            // the decision from the application to actually not use
            // the standard PeerGroup Module loading scheme.
            try {
                module.init(group, null, null);
            } catch (Exception e) {
                // Init failed, the module cannot be initialized
                return null;
            }
            registerModule(moduleName, module);
        }
        return module;
    }

    /**
     * loadModule
     *
     * Loads a Module. The default PeerGroup class loader will be used. The class
     * must be within the CLASSPATH of the platform.
     * If the module has already been loaded, the existing Module is returned.
     *
     * @param  moduleName  symbolic name of the Module
     * @param  moduleCode  the name of the class to be loaded.
     * @return             the Module for the given name. null is returned if the module could not be
     * loaded
     */
    public synchronized Module loadModule(String moduleName, String moduleCode) {

        // First check if the module is already loaded and registered
        Module module = lookupModule(moduleName);

        if (module != null) {
            return module;
        }

        if (!createModuleAdvs(moduleName, null, moduleCode, null, LOCAL_ADV_TTL, REMOTE_ADV_TTL)) {

            // Creation of the module advertisement has failed.
            return null;
        }
        // Get the module. This should always work since the advertisements have
        // just been created.
        module = loadModule(moduleName);
        if (module == null) {
            // There is really nothing more we can do here.
            return null;
        }
        return module;
    }

    /**
     *  Description of the Method
     *
     * @param  moduleName  Description of the Parameter
     * @return             Description of the Return Value
     */
    private synchronized Module loadModule(String moduleName) {

        // First check if the module is already loaded and registered
        Module module = lookupModule(moduleName);

        if (module != null) {
            return module;
        }

        try {
            // Recover the ModuleClassAdvertisement
            Enumeration each = group.getDiscoveryService().getLocalAdvertisements(DiscoveryService.ADV, "Name", moduleName);

            if (!each.hasMoreElements()) {
                // No advertisement.
                return null;
            }

            ModuleClassAdvertisement mcAdv = null;

            while (each.hasMoreElements()) {
                try {
                    mcAdv = (ModuleClassAdvertisement) each.nextElement();
                    break;
                } catch (Exception ez1) {// ignored
                }
            }

            // Revover the Module Specification Advertisement
            each = group.getDiscoveryService().getLocalAdvertisements(DiscoveryService.ADV, "Name", moduleName);
            if (!each.hasMoreElements()) {
                return null;
            }

            ModuleSpecAdvertisement mSpecAdv = null;

            while (each.hasMoreElements()) {
                try {
                    mSpecAdv = (ModuleSpecAdvertisement) each.nextElement();
                    break;
                } catch (Exception ez1) {// ignored
                }
            }

            module = group.loadModule(mcAdv.getModuleClassID(), mSpecAdv.getModuleSpecID(), PeerGroup.Here);

            if (module != null) {
                registerModule(moduleName, module);
            }
            return module;
        } catch (Exception ez2) {
            return null;
        }
    }

    /**
     *  Description of the Method
     *
     * @param  moduleName     Description of the Parameter
     * @param  moduleSpecURI  Description of the Parameter
     * @param  moduleCode     Description of the Parameter
     * @param  moduleCodeURI  Description of the Parameter
     * @param  localTTL       Description of the Parameter
     * @param  remoteTTL      Description of the Parameter
     * @return                Description of the Return Value
     */
    private boolean createModuleAdvs(String moduleName, String moduleSpecURI, String moduleCode, String moduleCodeURI, long localTTL, long remoteTTL) {

        DiscoveryService disco = group.getDiscoveryService();

        try {
            // First create the Module class advertisement associated with the module
            // We build the module class advertisement using the advertisement
            // Factory class by passing it the type of the advertisement we
            // want to construct. The Module class advertisement is to be used
            // to simply advertise the existence of the module. This is a
            // a very small advertisement that only advertise the existence
            // of module. In order to access the module, a peer will
            // have to discover the associated module spec advertisement.

            ModuleClassAdvertisement mcadv = (ModuleClassAdvertisement)
                    AdvertisementFactory.newAdvertisement(ModuleClassAdvertisement.getAdvertisementType());

            mcadv.setName(moduleName);
            mcadv.setDescription("Created by ModuleManager: " + moduleName);

            ModuleClassID mcID = IDFactory.newModuleClassID();

            mcadv.setModuleClassID(mcID);

            // Ok the Module Class advertisement was created, just publish
            // it in my local cache and to my peergroup. This
            // is the NetPeerGroup
            disco.publish(mcadv, localTTL, remoteTTL);

            // Create the Module Spec advertisement associated with the module
            // We build the module Spec Advertisement using the advertisement
            // Factory class by passing in the type of the advertisement we
            // want to construct. The Module Spec advertisement will contain
            // all the information necessary for a client to contact the module
            // for instance it will contain a pipe advertisement to
            // be used to contact the module

            ModuleSpecAdvertisement mdadv = (ModuleSpecAdvertisement)
                    AdvertisementFactory.newAdvertisement(ModuleSpecAdvertisement.getAdvertisementType());

            // ModuleManager does not allow to set up any customized
            // information for the Module.

            mdadv.setName(moduleName);
            mdadv.setCreator("jxta.org");
            mdadv.setModuleSpecID(IDFactory.newModuleSpecID(mcID));

            if (moduleSpecURI != null) {
                mdadv.setSpecURI(moduleSpecURI);
            }

            // Ok the Module advertisement was created, just publish
            // it in my local cache and into the NetPeerGroup.
            disco.publish(mdadv, localTTL, remoteTTL);

            // Create the Module Implementation advertisement
            ModuleImplAdvertisement miadv = (ModuleImplAdvertisement)
                    AdvertisementFactory.newAdvertisement(ModuleImplAdvertisement.getAdvertisementType());

            miadv.setModuleSpecID(mdadv.getModuleSpecID());
            if (moduleCode != null) {
                miadv.setCode(moduleCode);
            }

            if (moduleCodeURI != null) {
                miadv.setUri(moduleCodeURI);
            }
            miadv.setDescription("Created by ModuleManager: " + moduleName);

            // Steal the compat, provider, and uri from the
            // group's own impl adv. We DO want them identical in
            // this case.
            ModuleImplAdvertisement pgImpl = (ModuleImplAdvertisement) group.getImplAdvertisement();

            miadv.setCompat(pgImpl.getCompat());
            miadv.setUri(pgImpl.getUri());

            // Ok the Module Class advertisement was created, just publish
            // it in my local cache and to my peergroup. This
            // is the NetPeerGroup
            disco.publish(miadv, localTTL, remoteTTL);
        } catch (Exception ex) {
            return false;
        }
        return true;
    }

    // FIXME this method should be refactored
    /**
     *  Creates a Module Class, Spec, and Impl advertisements, and adds the service
     *  Advertisement as part of the Module Impl Advertisement, and publishes the advertisements
     *  in local cache
     *
     * @param  group            group 
     * @param  moduleName       module name
     * @param  description      module description 
     * @param  moduleSpecURI    module spec uri
     * @param  moduleCode       module code
     * @param  moduleCodeURI    module code uri 
     * @param  mcID             module class id
     * @param  msID             module spec id
     * @param code              module code
     * @param  serviceAdv       service advertisement
     * @param  localTTL         local cache lifetime in ms
     * @param  remoteTTL        remote cache lifetime in ms
     * @exception  IOException  if an io error occurs
     */
    public void createServiceAdvertisement(PeerGroup group, String moduleName, String description, String moduleSpecURI, String moduleCode, String moduleCodeURI, ModuleClassID mcID, ModuleSpecID msID, String code, Advertisement serviceAdv, long localTTL, long remoteTTL) throws IOException {

        DiscoveryService discovery = group.getDiscoveryService();
        // Create module class advertisement
        ModuleClassAdvertisement mcadv = (ModuleClassAdvertisement)
                AdvertisementFactory.newAdvertisement(ModuleClassAdvertisement.getAdvertisementType());

        mcadv.setModuleClassID(mcID);
        mcadv.setName(moduleName);
        mcadv.setDescription(description);

        // Module spec advertisement
        ModuleSpecAdvertisement mdspec = (ModuleSpecAdvertisement)
                AdvertisementFactory.newAdvertisement(ModuleSpecAdvertisement.getAdvertisementType());

        mdspec.setModuleSpecID(msID);
        mdspec.setName(moduleName);
        mdspec.setSpecURI(moduleSpecURI);

        // Module implementation advertisement
        ModuleImplAdvertisement miadv = (ModuleImplAdvertisement)
                AdvertisementFactory.newAdvertisement(ModuleImplAdvertisement.getAdvertisementType());

        miadv.setModuleSpecID(mdspec.getModuleSpecID());
        miadv.setDescription(description);
        if (moduleCodeURI != null) {
            miadv.setUri(moduleCodeURI);
        }
        if (moduleCode != null) {
            miadv.setCode(moduleCode);
        }
        // Steal the compat, provider, and uri from the
        // group's own impl adv. We DO want them identical in
        // this case.
        ModuleImplAdvertisement pgImpl = (ModuleImplAdvertisement) group.getImplAdvertisement();

        miadv.setCompat(pgImpl.getCompat());
        miadv.setCode(code);
        Element pEl = (Element) serviceAdv.getDocument(MimeMediaType.XMLUTF8);
        StructuredDocument svcParm = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");

        StructuredDocumentUtils.copyElements(svcParm, svcParm, pEl);
        miadv.setParam(svcParm);
        // publish the advertisements
        discovery.publish(mcadv, localTTL, remoteTTL);
        discovery.publish(mdspec, localTTL, remoteTTL);
        discovery.publish(miadv, localTTL, remoteTTL);
    }

    /**
     *  Retreives a Service Advertisement from a module impl advertisement
     * @param  group             peer group
     * @param  mia               ModuleImplAdvertisement
     * @param  advertismentType  service advertisment string Type
     * @return                   The service Advertisement 
     * @exception  IOException   if an io error occurs
     */
    public Advertisement getServiceAdvertisement(PeerGroup group, ModuleImplAdvertisement mia, String advertismentType) throws IOException {
        Element param = mia.getParam();
        Element pel = null;

        if (param != null) {
            Enumeration list = param.getChildren(advertismentType);

            if (list.hasMoreElements()) {
                pel = (Element) list.nextElement();
            }
        }
        Advertisement adv = AdvertisementFactory.newAdvertisement((TextElement) pel);

        return adv;
    }

    /**
     *  Description of the Class
     */
    private class ModuleDesc {

        /**
         *  Description of the Field
         */
        protected Module module = null;
        private boolean started = false;
        private boolean stopped = true;

        /**
         *Constructor for the ModuleDesc object
         *
         * @param  module  Description of the Parameter
         */
        public ModuleDesc(Module module) {
            this.module = module;
        }

        /**
         *  Description of the Method
         *
         * @param  args  Description of the Parameter
         */
        public void startApp(String[] args) {
            if (module == null) {
                return;
            }
            if (started) {
                // Already started - nothing to do
                return;
            }
            module.startApp(args);
            started = true;
            stopped = false;
        }

        /**
         *  Description of the Method
         */
        public void stopApp() {
            if (module == null) {
                return;
            }
            if (stopped) {
                // Already stopped - nothing to do
                return;
            }
            module.stopApp();
            stopped = true;
            started = false;
        }
    }


    /**
     * ModuleManagerLoader interface.
     * This interface is used by the application in order to provide its own
     * class loader instead of using the standard PeerGroup loader.
     */

    public interface ModuleManagerLoader {

        /**
         * This method is invoked by the ModuleManager when it is time to load
         * the class associated to the module. The name of the module is provided,
         * which allows the application provided loader to be able to load a variety
         * of modules, if that is necessary for the application. Note that the ModuleManager
         * assumes that the module which is loaded by the provided loader is not started:
         * loading and starting a module are two different operations for the ModuleManager.
         *
         * @param  moduleName  is the symbolic name of the Module.
         * @return             Module the object that has been loaded.
         */
        public Module loadModule(String moduleName);
    }

}

