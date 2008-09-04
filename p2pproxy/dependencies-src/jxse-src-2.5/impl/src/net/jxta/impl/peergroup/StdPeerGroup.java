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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.net.URI;
import java.net.URL;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.MessageTransport;
import net.jxta.exception.PeerGroupException;
import net.jxta.exception.ServiceNotFoundException;
import net.jxta.id.ID;
import net.jxta.impl.cm.Cm;
import net.jxta.impl.cm.SrdiIndex;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.JxtaLoader;
import net.jxta.platform.Module;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.service.Service;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import net.jxta.document.TextElement;

/**
 * A subclass of GenericPeerGroup that makes a peer group out of independent
 * plugin services listed in its impl advertisement.
 */
public class StdPeerGroup extends GenericPeerGroup {
    
    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(StdPeerGroup.class.getName());
    
    // A few things common to all ImplAdv for built-in things.
    public static final XMLDocument STD_COMPAT = mkCS();
    public static final String MODULE_IMPL_STD_URI = "http://jxta-jxse.dev.java.net/download/jxta.jar";
    public static final String MODULE_IMPL_STD_PROVIDER = "sun.com";
    
    protected static final String STD_COMPAT_FORMAT = "Efmt";
    
    // FIXME 20061206 bondolo Update this to "JRE1.5" after 2.5 release. 2.4.1 and earlier don't do version comparison correctly.
    
    /**
     * The Specification title and Specification version we require.
     */
    protected static final String STD_COMPAT_FORMAT_VALUE = "JDK1.4.1";
    protected static final String STD_COMPAT_BINDING = "Bind";
    protected static final String STD_COMPAT_BINDING_VALUE = "V2.0 Ref Impl";
    
    static {
        // Initialize the JXTA class loader with the standard modules.
        try {
            Enumeration<URL> allProviderLists = GenericPeerGroup.class.getClassLoader().getResources("META-INF/services/net.jxta.platform.Module");
            for (URL providers : Collections.list(allProviderLists)) {
                registerFromFile(providers);
            }
        } catch (IOException ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to locate provider lists", ex);
            }
        }
        
        // XXX Force redefinition of StdPeerGroup implAdvertisement.
        getJxtaLoader().defineClass(getDefaultModuleImplAdvertisement());
    }
    
    /**
     * Register instance classes given a URL to a file containing modules which
     * must be found on the current class path. Each line of the file contains a 
     * module spec ID, the class name and the Module description. The fields are 
     * separated by whitespace. Comments are marked with a '#', the pound sign. 
     * Any text following # on any line in the file is ignored.
     *
     * @param providerList the URL to a file containing a list of modules
     * @return {@code true} if at least one of the instance classes could be
     * registered otherwise {@code false}.
     */
    private static boolean registerFromFile(URL providers) {
        boolean registeredSomething = false;
        InputStream urlStream = null;
        
        try {
            urlStream = providers.openStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(urlStream, "UTF-8"));
            
            String provider;
            while ((provider = reader.readLine()) != null) {
                int comment = provider.indexOf('#');
                
                if (comment != -1) {
                    provider = provider.substring(0, comment);
                }
                
                provider = provider.trim();
                
                if (0 == provider.length()) {
                    continue;
                }
                
                try {
                    String[] parts = provider.split("\\s", 3);
                    
                    if (3 == parts.length) {
                        ModuleSpecID msid = ModuleSpecID.create(URI.create(parts[0]));
                        String code = parts[1];
                        String description = parts[2];
                        
                        ModuleImplAdvertisement moduleImplAdv;
                        
                        try {
                            Class<Module> moduleClass = (Class<Module>) Class.forName(code);
                            
                            Method getImplAdvMethod = moduleClass.getMethod("getDefaultModuleImplAdvertisement");
                            
                            moduleImplAdv = (ModuleImplAdvertisement) getImplAdvMethod.invoke(null);
                        } catch(Exception failed) {
                            // Use default ModuleImplAdvertisement.
                            moduleImplAdv = StdPeerGroup.mkImplAdvBuiltin(msid, code, description);
                        }
                        
                        getJxtaLoader().defineClass(moduleImplAdv);
                        
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Registered Module " + msid + " : " + parts[1]);
                        }
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Failed to register \'" + provider + "\'");
                        }
                    }
                } catch (Exception allElse) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Failed to register \'" + provider + "\'", allElse);
                    }
                }
            }
        } catch (IOException ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to read provider list " + providers, ex);
            }
        } finally {
            if (null != urlStream) {
                try {
                    urlStream.close();
                } catch (IOException ignored) {
                    
                }
            }
        }
        return registeredSomething;
    }
    
    /**
     * If {@code true} then the PeerGroup has been started.
     */
    private volatile boolean started = false;
    
    /**
     * The order in which we started the services.
     */
    private final List<ModuleClassID> moduleStartOrder = new ArrayList<ModuleClassID>();
    
    /**
     * A map of the Message Transports for this group.
     * <p/>
     * <ul>
     * <li>keys are {@link net.jxta.platform.ModuleClassID}</li>
     * <li>values are {@link net.jxta.platform.Module}, but should also be
     * {@link net.jxta.endpoint.MessageTransport}</li>
     * </ul>
     */
    private final Map<ModuleClassID, Object> messageTransports = new HashMap<ModuleClassID, Object>();
    
    /**
     * A map of the applications for this group.
     * <p/>
     * <ul>
     * <li>keys are {@link net.jxta.platform.ModuleClassID}</li>
     * <li>values are {@link net.jxta.platform.Module} or
     * {@link net.jxta.protocol.ModuleImplAdvertisement} or
     * {@link net.jxta.platform.ModuleSpecID}</li>
     * </ul>
     */
    private final Map<ModuleClassID, Object> applications = new HashMap<ModuleClassID, Object>();
    
    /**
     * Cache for this group.
     */
    private Cm cm = null;
    
    private static XMLDocument mkCS() {
        XMLDocument doc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Comp");
        
        XMLElement e = doc.createElement(STD_COMPAT_FORMAT, STD_COMPAT_FORMAT_VALUE);
        
        doc.appendChild(e);
        
        e = doc.createElement(STD_COMPAT_BINDING, STD_COMPAT_BINDING_VALUE);
        doc.appendChild(e);
        return doc;
    }
    
    /**
     * An internal convenience method essentially for bootstrapping.
     * Make a standard ModuleImplAdv for any service that comes builtin this
     * reference implementation.
     * In most cases there are no params, so we do not take that argument.
     * The invoker may add params afterwards.
     *
     * @param specID spec ID
     * @param code   code uri
     * @param descr  description
     * @return a ModuleImplAdvertisement
     */
    static ModuleImplAdvertisement mkImplAdvBuiltin(ModuleSpecID specID, String code, String descr) {
        ModuleImplAdvertisement implAdv = (ModuleImplAdvertisement)
        AdvertisementFactory.newAdvertisement(ModuleImplAdvertisement.getAdvertisementType());
        
        implAdv.setModuleSpecID(specID);
        implAdv.setCompat(STD_COMPAT);
        implAdv.setCode(code);
        implAdv.setUri(MODULE_IMPL_STD_URI);
        implAdv.setProvider(MODULE_IMPL_STD_PROVIDER);
        implAdv.setDescription(descr);
        
        return implAdv;
    }
    
    /**
     *  Create and populate the default module impl Advertisement for this class.
     *
     *  @return The default module impl advertisement for this class.
     */
    private static ModuleImplAdvertisement getDefaultModuleImplAdvertisement() {
        ModuleImplAdvertisement implAdv = mkImplAdvBuiltin(PeerGroup.allPurposePeerGroupSpecID, StdPeerGroup.class.getName(), "General Purpose Peer Group Implementation");
        
        // Create the service list for the group.
        StdPeerGroupParamAdv paramAdv = new StdPeerGroupParamAdv();
        ModuleImplAdvertisement moduleAdv;
        
        // set the services
        
        // core services
        JxtaLoader loader = getJxtaLoader();
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refEndpointSpecID);
        paramAdv.addService(PeerGroup.endpointClassID, moduleAdv);
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refResolverSpecID);
        paramAdv.addService(PeerGroup.resolverClassID, moduleAdv);
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refMembershipSpecID);
        paramAdv.addService(PeerGroup.membershipClassID, moduleAdv);
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refAccessSpecID);
        paramAdv.addService(PeerGroup.accessClassID, moduleAdv);
        
        // standard services
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refDiscoverySpecID);
        paramAdv.addService(PeerGroup.discoveryClassID, moduleAdv);
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refRendezvousSpecID);
        paramAdv.addService(PeerGroup.rendezvousClassID, moduleAdv);
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refPipeSpecID);
        paramAdv.addService(PeerGroup.pipeClassID, moduleAdv);
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refPeerinfoSpecID);
        paramAdv.addService(PeerGroup.peerinfoClassID, moduleAdv);
        
        // Applications
        
        moduleAdv = loader.findModuleImplAdvertisement(PeerGroup.refShellSpecID);
        if (null != moduleAdv) {
            paramAdv.addApp(PeerGroup.applicationClassID, moduleAdv);
        }
        
        // Insert the newParamAdv in implAdv
        XMLElement paramElement = (XMLElement) paramAdv.getDocument(MimeMediaType.XMLUTF8);
        
        implAdv.setParam(paramElement);
        
        return implAdv;
    }
    
    /**
     * constructor
     */
    public StdPeerGroup() {
    }
    
    /**
     * {@inheritDoc}
     */
    // @Override
    public boolean compatible(Element compat) {
        return isCompatible(compat);
    }
    
    /**
     * Evaluates if the given compatibility statement makes the module that
     * bears it is loadable by this group.
     *
     * @param compat The compatibility statement being tested.
     * @return {@code true} if we are compatible with the provided statement
     * otherwise {@code false}.
     */
    static boolean isCompatible(Element compat) {
        boolean formatOk = false;
        boolean bindingOk = false;
        
        if(!(compat instanceof TextElement)) {
            return false;
        }
        
        try {
            Enumeration<TextElement> hisChildren = ((TextElement)compat).getChildren();
            int i = 0;
            while (hisChildren.hasMoreElements()) {
                // Stop after 2 elements; there shall not be more.
                if (++i > 2) {
                    return false;
                }
                
                TextElement e = hisChildren.nextElement();
                String key = e.getKey();
                String val = e.getValue().trim();
                
                if (STD_COMPAT_FORMAT.equals(key)) {
                    Package javaLangPackage = Package.getPackage("java.lang");
                    
                    boolean specMatches;
                    String version;
                    
                    if (val.startsWith("JDK") || val.startsWith("JRE")) {
                        specMatches = true;
                        version = val.substring(3).trim(); // allow for spaces.
                    } else if (val.startsWith(javaLangPackage.getSpecificationTitle())) {
                        specMatches = true;
                        version = val.substring(javaLangPackage.getSpecificationTitle().length()).trim(); // allow for spaces.
                    } else {
                        specMatches = false;
                        version = null;
                    }

                    formatOk = specMatches && javaLangPackage.isCompatibleWith(version);
                } else if (STD_COMPAT_BINDING.equals(key) && STD_COMPAT_BINDING_VALUE.equals(val)) {
                    bindingOk = true;
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Bad element in compatibility statement : " + key);
                    }
                    return false; // Might as well stop right now.
                }
            }
        } catch (Exception any) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure handling compatibility statement", any);
            }
            return false;
        }
        
        return formatOk && bindingOk;
    }
    
    /**
     * Builds a table of modules indexed by their class ID.
     * The values are the loaded modules, the keys are their classId.
     * This routine interprets the parameter list in the advertisement.
     *
     * @param modules    The modules to load
     * @param privileged if true then modules will get a real reference to
     *                   the group loading them, otherwise its an interface object.
     */
    protected void loadAllModules(Map<ModuleClassID, Object> modules, boolean privileged) {
        
        Iterator<Map.Entry<ModuleClassID, Object>> eachModule = modules.entrySet().iterator();
        
        while (eachModule.hasNext()) {
            Map.Entry<ModuleClassID, Object> anEntry = eachModule.next();
            ModuleClassID classID = anEntry.getKey();
            Object value = anEntry.getValue();
            
            // Already loaded.
            if (value instanceof Module) {
                continue;
            }
            
            // Try and load it.
            try {
                Module theModule = null;
                
                if (value instanceof ModuleImplAdvertisement) {
                    // Load module will republish locally but not in the
                    // parent since that adv does not come from there.
                    theModule = loadModule(classID, (ModuleImplAdvertisement) value, privileged);
                } else if (value instanceof ModuleSpecID) {
                    // loadModule will republish both locally and in the parent
                    // Where the module was fetched.
                    theModule = loadModule(classID, (ModuleSpecID) value, FromParent, privileged);
                } else {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("Skipping: " + classID + " Unsupported module descriptor : " + value.getClass().getName());
                    }
                    eachModule.remove();
                    continue;
                }
                
                if (theModule == null) {
                    throw new PeerGroupException("Could not find a loadable implementation for : " + classID);
                }
                
                anEntry.setValue(theModule);
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not load module for class ID : " + classID, e);
                    if (value instanceof ModuleImplAdvertisement) {
                        LOG.log(Level.WARNING, "Will be missing from peer group : " +
                                ((ModuleImplAdvertisement) value).getDescription());
                    } else {
                        LOG.log(Level.WARNING, "Will be missing from peer group: " + value);
                    }
                }
                eachModule.remove();
            }
        }
    }
    
    /**
     * The group does not care for start args, and does not come-up
     * with args to pass to its main app. So, until we decide on something
     * more useful, the args of the group's startApp are passed-on to the
     * group's main app. NB: both the apps init and startApp methods are
     * invoked.
     *
     * @return int Status.
     */
    @Override
    public int startApp(String[] arg) {
        
        if (!initComplete) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Group has not been initialized or init failed.");
            }
            return -1;
        }
        
        // FIXME: maybe concurrent callers should be blocked until the
        // end of startApp(). That could mean forever, though.
        if (started) {
            return Module.START_OK;
        }
        
        started = true;
        
        // Normally does nothing, but we have to.
        int res = super.startApp(arg);
        
        if (Module.START_OK != res) {
            return res;
        }
        
        loadAllModules(applications, false); // Apps are non-privileged;
        
        res = startModules((Map) applications);
        
        return res;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void stopApp() {
        // Shut down the group services and message transports.
        Collections.reverse(moduleStartOrder);
        
        for (ModuleClassID aModule : moduleStartOrder) {
            try {
                if (messageTransports.containsKey(aModule)) {
                    Module theMessageTransport = (Module) messageTransports.remove(aModule);
                    
                    theMessageTransport.stopApp();
                } else {
                    removeService(aModule);
                }
            } catch (Exception any) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to stop module: " + aModule, any);
                }
            }
        }
        
        moduleStartOrder.clear();
        
        if (!messageTransports.isEmpty()) {
            LOG.warning(messageTransports.size() + " message transports could not be shut down during peer group stop.");
        }
        
        messageTransports.clear();
        
        super.stopApp();
        
        if (cm != null) {
            cm.stop();
            cm = null;
        }
    }
    
    /**
     * Given a list of all the modules we need to start attempt to start them.
     * There is an a-priori order, but we'll iterate over the list until all
     * where able to complete their start phase or no progress is made. Since we
     * give modules the opportunity to pretend that they are making progress, we
     * need to have a safeguard: we will not iterate through the list more than
     * N^2 + 1 times without at least one module completing;  N being the number
     * of modules still in the list. This should cover the worst case scenario
     * and still allow the process to eventually fail if it has no chance of
     * success.
     *
     * @param services The services to start.
     */
    private int startModules(Map<ModuleClassID,Module> services) {
        int iterations = 0;
        int maxIterations = services.size() * services.size() + iterations + 1;
        
        boolean progress = true;
        
        while (!services.isEmpty() && (progress || (iterations < maxIterations))) {
            progress = false;
            iterations++;
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("Service startApp() round {0} of {1}(max)", iterations, maxIterations));
            }
            
            Iterator<Map.Entry<ModuleClassID, Module>> eachService = services.entrySet().iterator();
            
            while (eachService.hasNext()) {
                Map.Entry<ModuleClassID, Module> anEntry = eachService.next();
                ModuleClassID mcid = anEntry.getKey();
                Module aModule = anEntry.getValue();
                
                int res;
                
                try {
                    res = aModule.startApp(null);
                } catch (Throwable all) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Exception in startApp() : " + aModule, all);
                    }
                    res = -1;
                }
                
                switch (res) {
                    case Module.START_OK:
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Module started : " + aModule);
                        }
                        if (aModule instanceof Service) {
                            addService(mcid, (Service) aModule);
                        } else {
                            messageTransports.put(mcid, aModule);
                        }
                        
                        moduleStartOrder.add(mcid);
                        eachService.remove();
                        progress = true;
                        break;
                        
                    case Module.START_AGAIN_PROGRESS:
                        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                            LOG.finer("Service made progress during start : " + aModule);
                        }
                        progress = true;
                        break;
                        
                    case Module.START_AGAIN_STALLED:
                        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                            LOG.finer("Service stalled during start : " + aModule);
                        }
                        break;
                        
                    case Module.START_DISABLED:
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Service declined to start : " + aModule);
                        }
                        eachService.remove();
                        progress = true;
                        break;
                        
                    default: // (negative)
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Service failed to start (" + res + ") : " + aModule);
                        }
                        eachService.remove();
                        progress = true;
                        break;
                }
            }
            
            if (progress) {
                maxIterations = services.size() * services.size() + iterations + 1;
            }
        }
        
        // Uh-oh. Services co-dependency prevented them from starting.
        if (!services.isEmpty()) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                StringBuilder failed = new StringBuilder( "No progress is being made in starting services after "
                        + iterations + " iterations. Giving up.");
                
                failed.append("\nThe following services could not be started : ");
                
                for (Map.Entry<ModuleClassID, Module> aService : services.entrySet()) {
                    failed.append("\n\t");
                    failed.append(aService.getKey());
                    failed.append(" : ");
                    failed.append(aService.getValue());
                }
                
                LOG.severe(failed.toString());
            }
            
            return -1;
        }
        
        return Module.START_OK;
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * This method loads and initializes all of the peer group modules
     * described in the provided implementation advertisement. Then, all modules
     * are placed in a list and the list is processed iteratively. During each
     * iteration, the {@link Module#startApp(String[])} method of each module
     * is invoked once. Iterations continue until no progress is being made or
     * the list is empty.
     * <p/>
     * The status returned by the {@link Module#startApp(String[])} method
     * of each module is considered as follows:
     * <p/>
     * <ul>
     * <li>{@link Module#START_OK}: The module is removed from the list of
     * modules to be started and its {@link Module#startApp(String[])}
     * method will not be invoked again.
     * </li>
     * <p/>
     * <li>{@link Module#START_AGAIN_PROGRESS}: The module remains in the
     * list of modules to be started and its {@link Module#startApp(String[])}
     * method will be invoked during the next iteration, if there is one. </li>
     * <p/>
     * <li>{@link Module#START_AGAIN_STALLED}: The module remains in the list
     * of modules to be started and its {@link Module#startApp(String[])}
     * method will be invoked during the next iteration if there is one. </li>
     * <p/>
     * <li>Any other value: The module failed to initialize. Its
     * {@link Module#startApp(String[])} method will not be invoked again.</li>
     * </ul>
     * <p/>
     * Iterations through the list stop when:
     * <ul>
     * <li>The list is empty: the group initialization proceeds.</li>
     * <p/>
     * <li>A complete iteration was performed and all modules returned
     * {@link Module#START_AGAIN_STALLED}: a {@link PeerGroupException}
     * is thrown.</li>
     * <p/>
     * <li>A number of complete iteration completed without any module
     * returning {@link Module#START_OK}: a {@link PeerGroupException}
     * is thrown. The number of complete iterations before that happens is
     * computed as 1 + the square of the number of modules currently in the
     * list.</li>
     * </ul>
     */
    @Override
    protected synchronized void initFirst(PeerGroup parent, ID assignedID, Advertisement impl) throws PeerGroupException {
        
        if (initComplete) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("You cannot initialize a PeerGroup more than once !");
            }
            return;
        }
        
        // Set-up the minimal GenericPeerGroup
        super.initFirst(parent, assignedID, impl);
        
        // assignedID might have been null. It is now the peer group ID.
        assignedID = getPeerGroupID();
        
        // initialize cm before starting services.
        try {
            cm = new Cm(getHomeThreadGroup(),
                    getStoreHome(), assignedID.getUniqueValue().toString(),
                    Cm.DEFAULT_GC_MAX_INTERVAL, false);
        } catch (Exception e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Error during creation of local store", e);
            }
            throw new PeerGroupException("Error during creation of local store", e);
        }
        
        // flush srdi for this group
        SrdiIndex.clearSrdi(this);
        
        // Load the list of peer group services from the impl advertisement params.
        StdPeerGroupParamAdv paramAdv = new StdPeerGroupParamAdv(implAdvertisement.getParam());
        
        Map<ModuleClassID, Object> initServices = new HashMap<ModuleClassID, Object>(paramAdv.getServices());
        
        initServices.putAll(paramAdv.getProtos());
        
        // Remove the modules disabled in the configuration file.
        ConfigParams conf = getConfigAdvertisement();
        
        if(null != conf) {
            Iterator<ModuleClassID> eachModule = initServices.keySet().iterator();
            
            while(eachModule.hasNext()) {
                ModuleClassID aModule = eachModule.next();
                
                if(!conf.isSvcEnabled(aModule)) {
                    // remove disabled module
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Module disabled in configuration : " + aModule);
                    }
                    
                    eachModule.remove();
                }
            }
        }
        
        // We Applications are shelved until startApp()
        applications.putAll(paramAdv.getApps());
        
        if(null != conf) {
            Iterator<ModuleClassID> eachModule = applications.keySet().iterator();
            
            while(eachModule.hasNext()) {
                ModuleClassID aModule = eachModule.next();
                
                if(!conf.isSvcEnabled(aModule)) {
                    // remove disabled module
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Application disabled in configuration : " + aModule);
                    }
                    
                    eachModule.remove();
                }
            }
        }
        
        loadAllModules(initServices, true);
        
        int res = startModules((Map) initServices);
        
        if(Module.START_OK != res) {
            throw new PeerGroupException("Failed to start peer group services. res : " + res);
        }
        
        // Make sure all the required services are loaded.
        try {
            checkServices();
        } catch (ServiceNotFoundException e) {
            LOG.log(Level.SEVERE, "Missing peer group service", e);
            throw new PeerGroupException("Missing peer group service", e);
        }
        
        /*
         * Publish a few things that have not been published in this
         * group yet.
         */
        DiscoveryService discoveryService = getDiscoveryService();
        
        if (discoveryService != null) {
            // It should work but if it does not we can survive.
            try {
                // Discovery service adv could not be published localy,
                // since at that time there was no local discovery to
                // publish to.
                discoveryService.publish(discoveryService.getImplAdvertisement(), DEFAULT_LIFETIME, DEFAULT_EXPIRATION);
                
                // Try to publish our impl adv within this group. (it was published
                // in the parent automatically when loaded.
                discoveryService.publish(implAdvertisement, DEFAULT_LIFETIME, DEFAULT_EXPIRATION);
            } catch (Exception nevermind) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to publish Impl adv within group.", nevermind);
                }
            }
        }
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * Nothing special for now, but we might want to move some steps from
     * initFirst() in the future.
     */
    @Override
    protected synchronized void initLast() throws PeerGroupException {
        
        super.initLast();
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Group : " + getPeerGroupID());
            
            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tCompatibility Statement :\n\t\t\t");
            StringBuilder indent = new StringBuilder(STD_COMPAT.toString().trim());
            int from = indent.length();
            
            while (from > 0) {
                int returnAt = indent.lastIndexOf("\n", from);
                
                from = returnAt - 1;
                if ((returnAt >= 0) && (returnAt != indent.length())) {
                    indent.insert(returnAt + 1, "\t\t\t");
                }
            }
            configInfo.append(indent);
            Iterator eachProto = messageTransports.entrySet().iterator();
            
            if (eachProto.hasNext()) {
                configInfo.append("\n\t\tMessage Transports :");
            }
            while (eachProto.hasNext()) {
                Map.Entry anEntry = (Map.Entry) eachProto.next();
                ModuleClassID aMCID = (ModuleClassID) anEntry.getKey();
                Module anMT = (Module) anEntry.getValue();
                
                configInfo.append("\n\t\t\t").append(aMCID).append("\t").append(
                        (anMT instanceof MessageTransport)
                        ? ((MessageTransport) anMT).getProtocolName()
                        : anMT.getClass().getName());
            }
            Iterator<Map.Entry<ModuleClassID, Object>> eachApp = applications.entrySet().iterator();
            
            if (eachApp.hasNext()) {
                configInfo.append("\n\t\tApplications :");
            }
            while (eachApp.hasNext()) {
                Map.Entry<ModuleClassID, Object> anEntry = eachApp.next();
                ModuleClassID aMCID = anEntry.getKey();
                Object anApp = anEntry.getValue();
                
                if (anApp instanceof ModuleImplAdvertisement) {
                    ModuleImplAdvertisement adv = (ModuleImplAdvertisement) anApp;
                    
                    configInfo.append("\n\t\t\t").append(aMCID).append("\t").append(adv.getCode());
                } else {
                    configInfo.append("\n\t\t\t").append(aMCID).append("\t").append(anApp.getClass().getName());
                }
            }
            LOG.config(configInfo.toString());
        }
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * FIXME 20070801 bondolo To improve compatibility with existing
     * applications the returned {@code ModuleImplAdvertisement} will contain
     * embedded {@code ModuleImplAdvertisement}s for the referenced services as
     * opposed to {@code ModuleSpecID}s. This is because JXSE 2.4.1 and earlier
     * do not handle load failures of modules loaded by spec id correctly.
     * After JXSE 2.5 is released this should be changed to use the better
     * {@code ModuleSpecID} based peer group module specification.
     */
    // @Override
    public ModuleImplAdvertisement getAllPurposePeerGroupImplAdvertisement() {
        JxtaLoader loader = getJxtaLoader();
        
        // grab an impl adv
        ModuleImplAdvertisement implAdv = loader.findModuleImplAdvertisement(PeerGroup.allPurposePeerGroupSpecID);
        
        return implAdv;
    }
    
    
    /**
     * Returns the cache manager associated with this group.
     *
     * @return the cache manager associated with this group.
     */
    public Cm getCacheManager() {
        return cm;
    }
    
    /**
     * Return a map of the applications for this group.
     * <p/>
     * <ul>
     * <li>keys are {@link net.jxta.platform.ModuleClassID}</li>
     * <li>values are {@link net.jxta.platform.Module} or
     * {@link net.jxta.protocol.ModuleImplAdvertisement}</li>
     * </ul>
     *
     * @return a map of the applications for this group.
     */
    public Map<ModuleClassID, Object> getApplications() {
        return Collections.unmodifiableMap(applications);
    }
}
