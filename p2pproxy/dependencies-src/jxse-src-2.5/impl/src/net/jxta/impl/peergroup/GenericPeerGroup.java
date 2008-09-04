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

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executor;
import java.util.concurrent.ThreadPoolExecutor;
import net.jxta.access.AccessService;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointService;
import net.jxta.exception.PeerGroupException;
import net.jxta.exception.ProtocolNotSupportedException;
import net.jxta.exception.ServiceNotFoundException;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.loader.RefJxtaLoader;
import net.jxta.impl.protocol.PSEConfigAdv;
import net.jxta.impl.protocol.PlatformConfig;
import net.jxta.impl.util.TimeUtils;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peer.PeerInfoService;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.pipe.PipeService;
import net.jxta.platform.JxtaLoader;
import net.jxta.platform.Module;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.resolver.ResolverService;
import net.jxta.service.Service;

import java.io.IOException;
import java.lang.reflect.Method;
import java.lang.reflect.UndeclaredThrowableException;
import java.net.URI;
import java.net.URL;
import java.security.cert.Certificate;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Provides common services for most peer group implementations.
 */
public abstract class GenericPeerGroup implements PeerGroup {
    
    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(GenericPeerGroup.class.getName());
    
    /**
     *  Holder for configuration parameters for groups in the process of being created.
     */
    private final static Map<ID, ConfigParams> group_configs = Collections.synchronizedMap(new HashMap<ID, ConfigParams>());
    
    /**
     * The loader - use the getter and setter for modifying the ClassLoader for
     * a security manager.
     * <p/>
     * This should eventually be group scoped rather than implementation
     * scoped. We are currently allowing classes to loaded into contexts which
     * they should not be known.
     */
    private final static JxtaLoader loader = new RefJxtaLoader(new URL[0], new CompatibilityEquater() {
        public boolean compatible(Element test) {
            return StdPeerGroup.isCompatible(test);
        }
    });
    
    /*
     * Shortcuts to well known services.
     */
    private EndpointService endpoint;
    private ResolverService resolver;
    private DiscoveryService discovery;
    private PipeService pipe;
    private MembershipService membership;
    private RendezVousService rendezvous;
    private PeerInfoService peerinfo;
    private AccessService access;
    
    /**
     * This peer's advertisement in this group.
     */
    private final PeerAdvertisement peerAdvertisement;
    
    /**
     * This group's advertisement.
     */
    private PeerGroupAdvertisement peerGroupAdvertisement = null;
    
    /**
     * This group's implAdvertisement.
     */
    protected ModuleImplAdvertisement implAdvertisement = null;
    
    /**
     * This peer's config advertisement.
     */
    protected ConfigParams configAdvertisement = null;
    
    /**
     * This service implements a group but, being a Service, it runs inside of
     * some group. That's its home group.
     * <p/>
     * Exception: The platform peer group does not have a parent group. It
     * has to be entirely self sufficient.
     */
    protected PeerGroup parentGroup = null;
    
    /**
     * The location of our store
     */
    protected URI jxtaHome = null;
    
    /**
     * The services that do the real work of the Peer Group.
     */
    private final Map<ModuleClassID, Service> services = new HashMap<ModuleClassID, Service>();
    
    /**
     * {@code true} when we have decided to stop this group.
     */
    private volatile boolean stopping = false;
    
    /**
     * {@code true} when the PG adv has been published.
     */
    private boolean published = false; // assume it hasn't
    
    /**
     * Counts the number of times an interface to this group has been given out.
     * This is decremented every time an interface object is GCed or
     * its owner calls unref().
     *
     * <p/>When it reaches zero, if it is time to tear-down the group instance;
     * nomatter what the GC thinks. There are threads that need to be stopped
     * before the group instance object ever becomes un-reachable.
     */
    private int masterRefCount = 0;
    
    /**
     * Is {@code true} when at least one interface object has been generated AFTER
     * initComplete has become true. If true, the group stops when its ref
     * count reaches zero.
     */
    private boolean stopWhenUnreferenced = false;
    
    /**
     * Is set to {@code true} when {@code init()} is completed enough that it
     * makes sense to perform ref-counting.
     */
    protected volatile boolean initComplete = false;
    
    /**
     * The thread group in which threads created by this group or services of
     * this group should live. The thread group is used primarily for debugging
     * and classification purposes--we don't try to use any of the fancy (and
     * mostly useless) ThreadGroup features.
     */
    private ThreadGroup threadGroup = null;
        
    /**
     * The minimum number of Threads our Executor will reserve. Once started
     * these Threads will remain.
     *
     * todo convert these hardcoded settings into group config params.
     */
    private final int COREPOOLSIZE = 5;

    
    /**
     * The number of seconds that Threads above {@code COREPOOLSIZE} will
     * remain idle before terminating.
     *
     * todo convert these hardcoded settings into group config params.
     */
    private final long KEEPALIVETIME = 15;

    
    /**
     * The intended upper bound on the number of threads we will allow our 
     * Executor to create. We will allow the pool to grow to twice this size if
     * we run out of threads.
     *
     * todo convert these hardcoded settings into group config params.
     */
    private final int MAXPOOLSIZE = 100;

    
    /**
     * Queue for tasks waiting to be run by our {@code Executor}.
     */
    private BlockingQueue<Runnable> taskQueue;

    
    /**
     * The PeerGroup ThreadPool
     */
    private ThreadPoolExecutor threadPool;
    
    /**
     * The PeerGroup ScheduledExecutor
     */
    private ScheduledThreadPoolExecutor scheduledExecutor;    

    
    /**
     * {@inheritDoc}
     * <p/>
     * We do not want to count on the invoker to properly unreference the group
     * object that we return; this call is often used in a loop and it is silly
     * to increment and decrement ref-counts for references that are sure to 
     * live shorter than the referee.
     * <p/>
     * On the other hand it is dangerous for us to share our reference object to
     * the parent group. That's where weak interface objects come in handy. We
     * can safely make one and give it away.
     */
    public PeerGroup getParentGroup() {
        if (parentGroup == null) {
            return null;
        }
        return parentGroup.getWeakInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public URI getStoreHome() {
        return jxtaHome;
    }
    
    /**
     * Sets the root location for the store to be used by this peergroup.
     * <p/>
     * This should be set early in the peer group's life and then never
     * changed.
     *
     * @param newHome The new store location.
     */
    protected void setStoreHome(URI newHome) {
        jxtaHome = newHome;
    }

    /**
     * {@inheritDoc}
     */
    public static JxtaLoader getJxtaLoader() {
        return loader;
    }
    
    public GenericPeerGroup() {
        // Start building our peer adv.
        peerAdvertisement = (PeerAdvertisement)
                AdvertisementFactory.newAdvertisement(PeerAdvertisement.getAdvertisementType());
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {        
        if (!(target instanceof PeerGroup)) {
            return false;
        }
        
        PeerGroup targetAsPeerGroup = (PeerGroup) target;
        
        // both null or both non-null.
        if ((null == parentGroup) && (null != targetAsPeerGroup.getParentGroup())) {
            return false;
        }
        
        if ((null != parentGroup) && (null == targetAsPeerGroup.getParentGroup())) {
            return false;
        }
        
        if ((null != parentGroup) && !parentGroup.equals(targetAsPeerGroup.getParentGroup())) {
            return false;
        }
        
        // and same peer ids.
        return getPeerGroupID().equals(targetAsPeerGroup.getPeerGroupID());
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        // before init we must fail.
        if ((null == peerAdvertisement) || (null == getPeerGroupID())) {
            throw new IllegalStateException("PeerGroup not sufficiently initialized");
        }
        
        // XXX 20050907 bondolo including parentGroup would improve the hash.
        return getPeerGroupID().hashCode();
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * An implementation suitable for debugging. <b>Don't try to parse
     * this string!</b> All of the information is available from other sources.
     */
    @Override
    public String toString() {
        if (null == getPeerGroupID()) {
            return super.toString();
        }
        
        StringBuilder result = new StringBuilder();
        
        result.append(getPeerGroupID().toString());
        String peerGroupName = peerGroupAdvertisement.getName();

        if (null != peerGroupName) {
            result.append(" \"");
            result.append(peerGroupName);
            result.append('\"');
        }
        
        result.append('[');
        result.append(masterRefCount);
        result.append(']');
        
        if (null != parentGroup) {
            result.append(" / ");
            result.append(parentGroup.toString());
        }
        
        return result.toString();
    }
    
    /**
     * {@inheritDoc}
     */
    public ThreadGroup getHomeThreadGroup() {
        return threadGroup;
    }
    
    /**
     * Discover advertisements.
     *
     * @param discovery The discovery service to use.
     * @param type      the Discovery advertisement type.
     * @param attr      The attribute to search for or {@code null}.
     * @param value     The attribute value to match or {@code null}.
     * @param seconds   The number of seconds to search.
     * @param thisClass The Advertisement class which the advertisement must
     *                  match.
     * @return a Collection of advertisements
     */
    private Collection<Advertisement> discoverSome(DiscoveryService discovery, int type, String attr, String value, int seconds, Class thisClass) {        
        long discoverUntil = TimeUtils.toAbsoluteTimeMillis(seconds * TimeUtils.ASECOND);
        long lastRemoteAt = 0; // no previous remote discovery made.
        
        List<Advertisement> results = new ArrayList<Advertisement>();
        
        try {
            do {
                Enumeration<Advertisement> res = discovery.getLocalAdvertisements(type, attr, value);

                while (res.hasMoreElements()) {
                    Advertisement a = res.nextElement();
                    
                    if (thisClass.isInstance(a)) {
                        results.add(a);
                    }
                }
                
                if (!results.isEmpty()) {
                    break;
                }
                
                if (TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), lastRemoteAt) > (30 * TimeUtils.ASECOND)) {
                    discovery.getRemoteAdvertisements(null, type, attr, value, 20);
                    lastRemoteAt = TimeUtils.timeNow();
                }
                
                // snooze waiting for responses to come in.
                Thread.sleep(1000);
            } while (TimeUtils.timeNow() < discoverUntil);
        } catch (Exception whatever) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure during discovery", whatever);
            }
        }
        
        return results;
    }
    
    /**
     * Discover an advertisement within the local peer group.
     *
     * @param type      the Discovery advertisement type.
     * @param attr      The attribute to search for or {@code null}.
     * @param value     The attribute value to match or {@code null}.
     * @param seconds   The number of seconds to search.
     * @param thisClass The Advertisement class which the advertisement must match.
     * @return a Collection of advertisements
     */
    private Advertisement discoverOne(int type, String attr, String value, int seconds, Class thisClass) {        
        Iterator<Advertisement> res = discoverSome(discovery, type, attr, value, seconds, thisClass).iterator();
        
        if (!res.hasNext()) {
            return null;
        }
        return res.next();
    }
    
    /**
     * Shortcuts to the standard basic services.
     *
     * @param mcid    The Module Class ID of the service.
     * @param service The service instance to set as the shortcut or
     *                {@code null} to clear the shortcut.
     */
    private void setShortCut(ModuleClassID mcid, Service service) {
        if (endpointClassID.equals(mcid)) {
            endpoint = (EndpointService) service;
            return;
        }
        if (resolverClassID.equals(mcid)) {
            resolver = (ResolverService) service;
            return;
        }
        if (discoveryClassID.equals(mcid)) {
            discovery = (DiscoveryService) service;
            return;
        }
        if (pipeClassID.equals(mcid)) {
            pipe = (PipeService) service;
            return;
        }
        if (membershipClassID.equals(mcid)) {
            membership = (MembershipService) service;
            return;
        }
        if (peerinfoClassID.equals(mcid)) {
            peerinfo = (PeerInfoService) service;
            return;
        }
        if (rendezvousClassID.equals(mcid)) {
            rendezvous = (RendezVousService) service;
            return;
        }
        if (accessClassID.equals(mcid)) {
            access = (AccessService) service;
        }
    }
    
    /**
     * Add a service to the collection of known services.
     *
     * @param mcid    The Module Class ID of the service.
     * @param service The service instance to set as the shortcut or
     */
    protected synchronized void addService(ModuleClassID mcid, Service service) {
        if (stopping) {
            return;
        }
        
        if (services.containsKey(mcid)) {
            throw new IllegalStateException("Service" + mcid + " already registered.");
        }
        
        services.put(mcid, service);
        
        setShortCut(mcid, service);
    }
    
    /**
     * {@inheritDoc}
     */
    synchronized public Service lookupService(ID mcid) throws ServiceNotFoundException {
        Service p = services.get(mcid);
        
        if (p == null) {
            throw new ServiceNotFoundException("Not found: " + mcid.toString());
        }
        
        return p.getInterface();
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * Group implementations do not have to support mapping.
     * it would be nice to separate better Interfaces, so that
     * Interface Objects can do things that the real service does
     * not have to implement.
     */
    public Service lookupService(ID mcid, int roleIndex) throws ServiceNotFoundException {
        
        // If the role number is != 0, it can't be honored: we
        // do not have an explicit map.
        
        if (roleIndex != 0) {
            throw new ServiceNotFoundException("Not found: " + mcid + "[" + roleIndex + "]");
        }
        
        return lookupService(mcid);
    }
    
    /**
     * {@inheritDoc}
     */
    public Iterator getRoleMap(ID name) {
        // No translation; use the given name in a singleton.
        return Collections.singletonList(name).iterator();
    }
    
    /**
     * check that all required core services are registered
     *
     * @throws ServiceNotFoundException If a required service was not found.
     */
    protected void checkServices() throws ServiceNotFoundException {
        Service ignored;
        
        ignored = lookupService(endpointClassID);
        ignored = lookupService(resolverClassID);
        ignored = lookupService(membershipClassID);
        ignored = lookupService(accessClassID);
        
        /**
         * ignored = lookupService(discoveryClassID);
         * ignored = lookupService(pipeClassID);
         * ignored = lookupService(rendezvousClassID);
         * ignored = lookupService(peerinfoClassID);
         */
    }
    
    /**
     * Ask a group to unregister and unload a service
     *
     * @param mcid The service to be removed.
     * @throws ServiceNotFoundException if service is not found
     */
    protected synchronized void removeService(ModuleClassID mcid) throws ServiceNotFoundException {
        setShortCut(mcid, null);
        
        Service p = services.remove(mcid);
        
        if (p == null) {
            throw new ServiceNotFoundException("Not found: " + mcid.toString());
        }
        
        p.stopApp();
        
        // service.terminate(); FIXME. We probably need a terminate()
        // method.
        // FIXME: [jice@jxta.org 20011013] to make sure the service is
        // no-longer referenced, we should always return interfaces, and
        // have a way to cut the reference to the real service in the
        // interfaces. One way of doing that would be to have to levels
        // of indirection: we should keep one and return references to it.
        // when we want to cut the service loose, we should clear the
        // reference from the interface that we own before letting it go.
        // We need to study the consequences of doing that before implementing
        // it.
    }
    
    /**
     * {@inheritDoc}
     */
    public Module loadModule(ID assigned, Advertisement impl) throws ProtocolNotSupportedException, PeerGroupException {
        return loadModule(assigned, (ModuleImplAdvertisement) impl, false);
    }
    
    /**
     * Load a Module from a ModuleImplAdv.
     * <p/>
     * Compatibility is checked and load is attempted. If compatible and
     * loaded successfully, the resulting Module is initialized and returned.
     * In most cases the other loadModule() method should be preferred, since
     * unlike this one, it will seek many compatible implementation
     * advertisements and try them all until one works. The home group of the new
     * module (its' parent group if the new Module is a group) will be this group.
     *
     * @param assigned   Id to be assigned to that module (usually its ClassID).
     * @param implAdv    An implementation advertisement for that module.
     * @param privileged If {@code true} then the module is provided the true
     *                   group obj instead of just an interface to the group object. This is
     *                   normally used only for the group's defined services and applications.
     * @return Module the module loaded and initialized.
     * @throws ProtocolNotSupportedException The module is incompatible.
     * @throws PeerGroupException            The module could not be loaded or initialized
     */
    protected Module loadModule(ID assigned, ModuleImplAdvertisement implAdv, boolean privileged) throws ProtocolNotSupportedException, PeerGroupException {
        
        Element compat = implAdv.getCompat();
        
        if (null == compat) {
            throw new IllegalArgumentException("No compatibility statement for : " + assigned);
        }
        
        if (!compatible(compat)) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Incompatible Module : " + assigned);
            }
            
            throw new ProtocolNotSupportedException("Incompatible Module : " + assigned);
        }
        
        Module newMod = null;
        
        if ((null != implAdv.getCode()) && (null != implAdv.getUri())) {
            try {
                // Good one. Try it.
                Class<Module> clazz;
                
                try {
                    clazz = (Class<Module>) loader.findClass(implAdv.getModuleSpecID());
                } catch (ClassNotFoundException notLoaded) {
                    clazz = (Class<Module>) loader.defineClass(implAdv);
                }
                
                if (null == clazz) {
                    throw new ClassNotFoundException("Cannot load class (" + implAdv.getCode() + ") : " + assigned);
                }
                
                newMod = clazz.newInstance();
                
                newMod.init(privileged ? this : (PeerGroup) getInterface(), assigned, implAdv);
                
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info( "Loaded" + (privileged ? " privileged" : "") + 
                            " module : " + implAdv.getDescription() + " (" + implAdv.getCode() + ")");
                }
            } catch (Exception ex) {
                try {
                    newMod.stopApp();
                } catch (Throwable ignored) {
                    // If this does not work, nothing needs to be done.
                }
                throw new PeerGroupException("Could not load module for : " + assigned + " (" + implAdv.getDescription() + ")", ex);
            }            
        } else {
            String error;

            if (null == implAdv.getCode()) {
                error = "ModuleImpAdvertisement missing Code element";
            } else if (null == implAdv.getUri()) {
                error = "ModuleImpAdvertisement missing URI element";
            } else {
                error = "ModuleImpAdvertisement missing both Code and URI elements";
            }
            throw new PeerGroupException("Can not load module : " + error + " for" + assigned);
        }
        
        // Publish or renew the lease of this adv since we're using it.
        try {
            if (discovery != null) {
                discovery.publish(implAdv, DEFAULT_LIFETIME, DEFAULT_EXPIRATION);
            }
        } catch (Exception ignored) {// ignored
        }
        
        // If we reached this point we're done.
        return newMod;
    }
    
    /**
     * {@inheritDoc}
     */
    public Module loadModule(ID assigned, ModuleSpecID specID, int where) {
        return loadModule(assigned, specID, where, false);
    }
    
    /**
     * Load a module from a ModuleSpecID
     * <p/>
     * Advertisement is sought, compatibility is checked on all candidates and
     * load is attempted. The first one that is compatible and loads
     * successfully is initialized and returned.
     * 
     * @param assignedID Id to be assigned to that module (usually its ClassID).
     * @param specID     The specID of this module.
     * @param where      May be one of: {@code Here}, {@code FromParent}, or
     *                   {@code Both}, meaning that the implementation advertisement will be
     *                   searched in this group, its parent or both. As a general guideline, the
     *                   implementation advertisements of a group should be searched in its
     *                   prospective parent (that is Here), the implementation advertisements of a
     *                   group standard service should be searched in the same group than where
     *                   this group's advertisement was found (that is, FromParent), while
     *                   applications may be sought more freely (Both).
     * @param privileged If {@code true} then the module is provided the true
     *                   group obj instead of just an interface to the group object. This is
     *                   normally used only for the group's defined services and applications.
     * @return Module the new module, or {@code null} if no usable implementation was found.
     */
    protected Module loadModule(ID assignedID, ModuleSpecID specID, int where, boolean privileged) {
        
        List<Advertisement> allModuleImplAdvs = new ArrayList<Advertisement>();

        ModuleImplAdvertisement loadedImplAdv = loader.findModuleImplAdvertisement(specID);
        if(null != loadedImplAdv) {
            // We already have a module defined for this spec id. Use that.
            allModuleImplAdvs.add(loadedImplAdv);
        } else {
            // Search for a module to use.
            boolean fromHere = (where == Here || where == Both);
            boolean fromParent = (where == FromParent || where == Both);

            if (fromHere && (null != discovery)) {
                Collection<Advertisement> here = discoverSome(discovery, DiscoveryService.ADV, 
                        "MSID", specID.toString(), 120, ModuleImplAdvertisement.class);

                allModuleImplAdvs.addAll(here);
            }

            if (fromParent && (null != getParentGroup()) && (null != parentGroup.getDiscoveryService())) {
                Collection<Advertisement> parent = discoverSome(parentGroup.getDiscoveryService(), DiscoveryService.ADV, 
                        "MSID", specID.toString(), 120, ModuleImplAdvertisement.class);

                allModuleImplAdvs.addAll(parent);
            }
        }
        
        Throwable recentFailure = null;
        
        for (Advertisement eachAdv : allModuleImplAdvs) {
            if( !(eachAdv instanceof ModuleImplAdvertisement) ) {
                continue;
            }
            
            ModuleImplAdvertisement foundImpl = (ModuleImplAdvertisement) eachAdv;
            
            try {
                // First check that the MSID is really the one we're looking for.
                // It could have appeared somewhere else in the adv than where
                // we're looking, and discovery doesn't know the difference.
                if (!specID.equals(foundImpl.getModuleSpecID())) {
                    continue;
                }
                
                Module newMod = loadModule(assignedID, foundImpl, privileged);
                
                // If we reach that point, the module is good.
                return newMod;
            } catch (ProtocolNotSupportedException failed) {
                // Incompatible implementation.
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "Incompatbile impl adv");
                }
            } catch (PeerGroupException failed) {
                // Initialization failure.
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Initialization failed", failed);
                }
            } catch (Throwable e) {
                recentFailure = e;
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Not a usable impl adv: ", e);
                }
            }
        }
        
        // Throw an exception if there was a recent failure.
        if (null != recentFailure) {
            if (recentFailure instanceof Error) {
                throw (Error) recentFailure;
            } else if (recentFailure instanceof RuntimeException) {
                throw (RuntimeException) recentFailure;
            } else {
                throw new UndeclaredThrowableException(recentFailure);
            }
        }
        
        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
            LOG.warning("Could not find a loadable implementation for SpecID: " + specID);
        }
        
        return null;
    }
    
    /**
     * {@inheritDoc}
     */
    public ConfigParams getConfigAdvertisement() {
        return configAdvertisement;
    }
    
    /**
     * Sets the configuration advertisement for this peer group.
     *
     * @param config The configuration advertisement which will be used for
     * this peer group or {@code null} if no configuration advertisement is to
     * be used.
     */
    protected void setConfigAdvertisement(ConfigParams config) {
        configAdvertisement = config;
    }
    
    /**
     *  Adds configuration parameters for the specified group. The configuration
     *  parameters remain cached until either the specified group is started or
     *  the parameters are replaced.
     *  
     *  @param groupid The group for who's params are being provided.
     *  @param params The parameters to be provided to the peer group when it is
     *  created.
     */
    public static void setGroupConfigAdvertisement(ID groupid, ConfigParams params) {
        if( null != params) {                
            group_configs.put(groupid, params);
        } else {
            group_configs.remove(groupid);
        }
    }
    
    /*
     * Now comes the implementation of the public API, including the
     * API mandated by the Service interface.
     */
    
    /**
     * {@inheritDoc}
     * <p/>
     * It is not recommended to overload this method. Instead, subclassers
     * should overload either or both of
     * {@link #initFirst(PeerGroup,ID,Advertisement)} and {@link #initLast()}.
     * If this method is to be overloaded, the overloading method must
     * invoke <code>super.init</code>.
     * <p/>
     * This method invokes <code>initFirst</code>
     * with identical parameters. <code>initLast</initLast> does not take
     * parameters since the relevant information can be obtained from the
     * group following completion of the <code>initFirst</code> phase.
     * The resulting values may be different from the parameters to
     * <code>initFirst</code> since <code>initFirst</code> may
     * be overLoaded and the overloading method may modify these parameters
     * when calling <code>super.initFirst</code>. (See
     * {@link net.jxta.impl.peergroup.Platform} for an example of such a case).
     * <p/>
     * Upon completion, the group object is marked as completely initialized
     * in all cases. Once a group object is completely initialized, it becomes
     * sensitive to reference counting.
     * <p/>
     * In the future this method may become final.
     */
    public void init(PeerGroup homeGroup, ID assignedID, Advertisement impl) throws PeerGroupException {
        try {
            initFirst(homeGroup, assignedID, impl);
            initLast();
        } finally {
            // This must be done in all cases.
            initComplete = true;
        }
    }
    
    /**
     * Performs all initialization steps that need to be performed
     * before any subclass initialization is performed.
     * <p/>
     * Classes that override this method should always call
     * <code>super.initFirst()</code> <strong>before</strong> doing
     * any of their own work.
     *
     * @param homeGroup  The group that serves as a parent to this group.
     * @param assignedID The unique ID assigned to this module. For
     *                   group this is the group ID or <code>null</code> if a group ID
     *                   has not yet been assigned. If null is passed, GenericPeerGroup
     *                   will generate a new group ID.
     * @param impl       The ModuleImplAdvertisement which defines this
     *                   group's implementation.
     * @throws PeerGroupException if a group initialization error occurs
     */
    protected void initFirst(PeerGroup homeGroup, ID assignedID, Advertisement impl) throws PeerGroupException {
        
        this.implAdvertisement = (ModuleImplAdvertisement) impl;
        this.parentGroup = homeGroup;
        
        if (null != parentGroup) {
            jxtaHome = parentGroup.getStoreHome();
        }
        
        // Set the peer configuration before we start.
        if((null != assignedID) && (null == getConfigAdvertisement())) {
            setConfigAdvertisement(group_configs.remove(assignedID));
        }
        
        try {
            // FIXME 20030919 bondolo@jxta.org This setup doesnt give us any
            // capability to use seed material or parent group.
            if (null == assignedID) {
                if ("cbid".equals(IDFactory.getDefaultIDFormat())) {
                    throw new IllegalStateException("Cannot generate group id for cbid group");
                } else {
                    assignedID = IDFactory.newPeerGroupID();
                }
            } else {
                if (parentGroup != null) {
                    DiscoveryService disco = parentGroup.getDiscoveryService();
                    if (null != disco) {
                        Enumeration found = disco.getLocalAdvertisements(DiscoveryService.GROUP, "GID", assignedID.toString());
                        if (found.hasMoreElements()) {
                            peerGroupAdvertisement = (PeerGroupAdvertisement) found.nextElement();
                        }
                    }
                }
            }
            
            if (!(assignedID instanceof PeerGroupID)) {
                throw new PeerGroupException("assignedID must be a peer group ID");
            }
            
            peerAdvertisement.setPeerGroupID((PeerGroupID) assignedID);
            
            // // make sure the parent group is the required group
            // if (null != peerAdvertisement.getPeerGroupID().getParentPeerGroupID()) {
            // if (null == parentGroup) {
            // throw new PeerGroupException("Group requires parent group : " + peerAdvertisement.getPeerGroupID().getParentPeerGroupID());
            // } else if (!parentGroup.getPeerGroupID().equals(peerAdvertisement.getPeerGroupID().getParentPeerGroupID())) {
            // throw new PeerGroupException("Group requires parent group : " + peerAdvertisement.getPeerGroupID().getParentPeerGroupID() + ". Provided parent was : " + parentGroup.getPeerGroupID());
            // }
            // }
            
            // Do our part of the PeerAdv construction.
            if ((configAdvertisement != null) && (configAdvertisement instanceof PlatformConfig)) {
                PlatformConfig platformConfig = (PlatformConfig) configAdvertisement;
                
                // Normally there will be a peer ID and a peer name in the config.
                PeerID configPID = platformConfig.getPeerID();
                
                if ((null == configPID) || (ID.nullID == configPID)) {
                    if ("cbid".equals(IDFactory.getDefaultIDFormat())) {
                        // Get our peer-defined parameters in the configAdv
                        XMLElement param = (XMLElement) platformConfig.getServiceParam(PeerGroup.membershipClassID);

                        if (null == param) {
                            throw new IllegalArgumentException(PSEConfigAdv.getAdvertisementType() + " could not be located");
                        }
                        
                        Advertisement paramsAdv = null;
                        try {
                            paramsAdv = AdvertisementFactory.newAdvertisement(param);
                        } catch (NoSuchElementException noadv) {// ignored
                        }
                        if (!(paramsAdv instanceof PSEConfigAdv)) {
                            throw new IllegalArgumentException(
                                    "Provided Advertisement was not a " + PSEConfigAdv.getAdvertisementType());
                        }
                        
                        PSEConfigAdv config = (PSEConfigAdv) paramsAdv;
                        Certificate clientRoot = config.getCertificate();
                        byte[] pub_der = clientRoot.getPublicKey().getEncoded();

                        platformConfig.setPeerID(IDFactory.newPeerID((PeerGroupID) assignedID, pub_der));
                    } else {
                        platformConfig.setPeerID(IDFactory.newPeerID((PeerGroupID) assignedID));
                    }
                }
                
                peerAdvertisement.setPeerID(platformConfig.getPeerID());
                peerAdvertisement.setName(platformConfig.getName());
                peerAdvertisement.setDesc(platformConfig.getDesc());
            } else {
                if (null == parentGroup) {
                    // If we did not get a valid peer id, we'll initialize it here.
                    peerAdvertisement.setPeerID(IDFactory.newPeerID((PeerGroupID) assignedID));
                } else {
                    // We're not the world peer group, which is the authoritative source of these values.
                    peerAdvertisement.setPeerID(parentGroup.getPeerAdvertisement().getPeerID());
                    peerAdvertisement.setName(parentGroup.getPeerAdvertisement().getName());
                    peerAdvertisement.setDesc(parentGroup.getPeerAdvertisement().getDesc());
                }
            }
            
            if (peerGroupAdvertisement == null) {
                // No existing gadv. OK then we're creating the group or we're
                // the platform, it seems. Start a grp adv with the essentials
                // that we know.
                peerGroupAdvertisement = (PeerGroupAdvertisement)
                        AdvertisementFactory.newAdvertisement(PeerGroupAdvertisement.getAdvertisementType());
                
                peerGroupAdvertisement.setPeerGroupID((PeerGroupID) assignedID);
                peerGroupAdvertisement.setModuleSpecID(implAdvertisement.getModuleSpecID());
            } else {
                published = true;
            }
            
            // If we still do not have a config adv, make one with the minimal info in it.
            // All groups but the Platform and the netPG are in that case.
            // In theory a plain ConfigParams would be enough for subgroups. But for now
            // GenericPeerGroup always has a full Platformconfig and there is no other concrete
            // ConfigParams subclass.
            if (configAdvertisement == null) {
                PlatformConfig conf = (PlatformConfig) AdvertisementFactory.newAdvertisement(PlatformConfig.getAdvertisementType());

                conf.setPeerID(peerAdvertisement.getPeerID());
                conf.setName(peerAdvertisement.getName());
                conf.setDesc(peerAdvertisement.getDesc());
                configAdvertisement = conf;
            }
            
            // Merge service params with those specified by the group (if any). The only
            // policy, right now, is to give peer params the precedence over group params.
            Hashtable grpParams = peerGroupAdvertisement.getServiceParams();
            Enumeration keys = grpParams.keys();

            while (keys.hasMoreElements()) {
                ID key = (ID) keys.nextElement();
                Element e = (Element) grpParams.get(key);

                if (configAdvertisement.getServiceParam(key) == null) {
                    configAdvertisement.putServiceParam(key, e);
                }
            }
            
            /*
             * Now seems like the right time to attempt to register the group.
             * The only trouble is that it could cause the group to
             * be used before all the services are initialized, but on the
             * other hand, we do not want to let a redundant group go through
             * it's service initialization because that would cause irreparable
             * damage to the legitimate instance. There should be a synchro on
             * on the get<service>() and lookupService() routines.
             */
            if (!globalRegistry.registerInstance((PeerGroupID) assignedID, this)) {
                throw new PeerGroupException("Group already instantiated");
            }
        } catch (Throwable any) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Group init failed", any);
            }
            
            if (any instanceof Error) {
                throw (Error) any;
            } else if (any instanceof RuntimeException) {
                throw (RuntimeException) any;
            } else if (any instanceof PeerGroupException) {
                throw (PeerGroupException) any;
            }
            
            throw new PeerGroupException("Group init failed", any);
        }
        
        ThreadGroup parentThreadGroup = (null != this.parentGroup)
                ? parentGroup.getHomeThreadGroup()
                : Thread.currentThread().getThreadGroup();
        
        threadGroup = new ThreadGroup(parentThreadGroup, "Group " + peerGroupAdvertisement.getPeerGroupID());
        
        taskQueue = new ArrayBlockingQueue<Runnable>(COREPOOLSIZE * 2);
        threadPool = new ThreadPoolExecutor(COREPOOLSIZE, MAXPOOLSIZE, 
                KEEPALIVETIME, TimeUnit.SECONDS, 
                taskQueue,
                new PeerGroupThreadFactory("Executor", getHomeThreadGroup()),
                new CallerBlocksPolicy());
        
        // Try to allow core threads to idle out. (Requires a 1.6 method)
        try {
            Method allowCoreThreadTimeOut = threadPool.getClass().getMethod("allowCoreThreadTimeOut", boolean.class);
            
            allowCoreThreadTimeOut.invoke(threadPool, Boolean.TRUE);            
        } catch(Throwable ohWell) {
            // Our attempt failed.
            if (Logging.SHOW_FINEST && LOG.isLoggable(Level.FINEST)) {
                LOG.log(Level.FINEST, "Failed to enable 'allowCoreThreadTimeOut'", ohWell);
            }
        }
        
        scheduledExecutor = new ScheduledThreadPoolExecutor(1,
            new PeerGroupThreadFactory("Scheduled Executor", getHomeThreadGroup()));
        
        /*
         * The rest of construction and initialization are left to the
         * group subclass, between here and the begining for initLast.
         * That should include instanciating and setting the endpoint, and
         * finally supplying it with endpoint protocols.
         * That also includes instanciating the appropriate services
         * and registering them.
         * For an example, see the StdPeerGroup class.
         */
    }
    
    /**
     * Perform all initialization steps that need to be performed
     * after any subclass initialization is performed.
     * <p/>
     * Classes that override this method should always call super.initLast
     * <strong>after</strong> doing any of their own work.
     * @throws PeerGroupException if a group initialization error occurs
     */
    protected void initLast() throws PeerGroupException {
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Group : " + getPeerGroupID());
            
            if (implAdvertisement != null) {
                configInfo.append("\n\tImplementation :");
                configInfo.append("\n\t\tModule Spec ID: ").append(implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description : ").append(implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI : ").append(implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code : ").append(implAdvertisement.getCode());
            }
            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tModule Spec ID : ").append(implAdvertisement.getModuleSpecID());
            configInfo.append("\n\t\tPeer Group ID : ").append(getPeerGroupID());
            configInfo.append("\n\t\tGroup Name : ").append(getPeerGroupName());
            configInfo.append("\n\t\tPeer ID in Group : ").append(getPeerID());
            configInfo.append("\n\tConfiguration :");
            if (null == parentGroup) {
                configInfo.append("\n\t\tHome Group : (none)");
            } else {
                configInfo.append("\n\t\tHome Group : \"").append(parentGroup.getPeerGroupName()).append("\" / ").append(
                        parentGroup.getPeerGroupID());
            }
            configInfo.append("\n\t\tServices :");
            for (Map.Entry<ModuleClassID, Service> anEntry : services.entrySet()) {
                ModuleClassID aMCID = anEntry.getKey();
                ModuleImplAdvertisement anImplAdv = (ModuleImplAdvertisement) anEntry.getValue().getImplAdvertisement();

                configInfo.append("\n\t\t\t").append(aMCID).append("\t").append(anImplAdv.getDescription());
            }
            LOG.config(configInfo.toString());
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public int startApp(String[] arg) {
        return Module.START_OK;
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * PeerGroupInterface's stopApp() does nothing. Only a real reference to the
     * group object permits to stop it without going through ref counting.
     */
    public void stopApp() {
        stopping = true;
        
        Collection<ModuleClassID> allServices = new ArrayList<ModuleClassID>(services.keySet());
        
        // Stop and remove all remaining services.
        for (ModuleClassID aService : allServices) {
            try {
                removeService(aService);
            } catch (Exception failure) {
                LOG.log(Level.WARNING, "Failure shutting down service : " + aService, failure);
            }
        }
        
        if (!services.isEmpty()) {
            LOG.warning(services.size() + " services could not be shut down during peer group stop.");
        }
        
        // remove everything (just in case);
        services.clear();
        
        globalRegistry.unRegisterInstance(peerGroupAdvertisement.getPeerGroupID(), this);
        
        // Explicitly unreference our parent group in order to allow it
        // to terminate if this group object was itself the last reference
        // to it.
        if (parentGroup != null) {
            parentGroup.unref();
            parentGroup = null;
        }
        
        // shutdown the threadpool
        threadPool.shutdownNow();
        scheduledExecutor.shutdownNow();
        
        // No longer initialized.
        initComplete = false;
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * May be called by a module which has a direct reference to the group
     * object and wants to notify its abandoning it. Has no effect on the real
     * group object.
     */
    public void unref() {}
    
    /**
     * Called every time an interface object that refers to this group
     * goes away, either by being finalized or by its unref() method being
     * invoked explicitly.
     */
    protected void decRefCount() {
        synchronized (this) {
            --masterRefCount;
            
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                Throwable trace = new Throwable("Stack Trace");
                StackTraceElement elements[] = trace.getStackTrace();
                LOG.info("[" + getPeerGroupID() + "] GROUP REF COUNT DECCREMENTED TO: " + masterRefCount + " by\n\t" + elements[2]);
            }
            
            if (masterRefCount != 0) {
                return;
            }
            
            if (!stopWhenUnreferenced) {
                return;
            }
        }
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("[" + getPeerGroupID() + "] STOPPING UNREFERENCED GROUP");
        }
        
        stopApp();
        masterRefCount = Integer.MIN_VALUE;
    }
    
    /*
     * Implement the Service API so that we can make groups services when we
     * decide to.
     */
    
    /**
     * {@inheritDoc}
     */
    public Service getInterface() {
        synchronized (this) {
            ++masterRefCount;
            
            if (masterRefCount < 1) {
                throw new IllegalStateException("Group has been shutdown. getInterface() is not available");
            }
            
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                Throwable trace = new Throwable("Stack Trace");
                StackTraceElement elements[] = trace.getStackTrace();

                LOG.info("[" + getPeerGroupID() + "] GROUP REF COUNT INCREMENTED TO: " + masterRefCount + " by\n\t" + elements[2]);
            }
            
            if (initComplete) {
                // If init is complete the group can become sensitive to
                // its ref count reaching zero. Before there could be
                // transient references before there is a chance to give
                // a permanent reference to the invoker of newGroup.
                stopWhenUnreferenced = true;
            }
        }
        
        return new RefCountPeerGroupInterface(this);
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerGroup getWeakInterface() {
        return new PeerGroupInterface(this);
    }
    
    /**
     * {@inheritDoc}
     */
    public ModuleImplAdvertisement getImplAdvertisement() {
        return implAdvertisement.clone();
    }
    
    /**
     * {@inheritDoc}
     */
    public void publishGroup(String name, String description) throws IOException {
        
        if (published) {
            return;
        }
        
        peerGroupAdvertisement.setName(name);
        peerGroupAdvertisement.setDescription(description);
        
        if (parentGroup == null) {
            return;
        }
        
        DiscoveryService parentDiscovery = parentGroup.getDiscoveryService();
        
        if (null == parentDiscovery) {
            return;
        }
        
        parentDiscovery.publish(peerGroupAdvertisement, DEFAULT_LIFETIME, DEFAULT_EXPIRATION);
        published = true;
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerGroup newGroup(Advertisement pgAdv) throws PeerGroupException {
        
        PeerGroupAdvertisement adv = (PeerGroupAdvertisement) pgAdv;
        PeerGroupID gid = adv.getPeerGroupID();
        
        if ((gid == null) || ID.nullID.equals(gid)) {
            throw new IllegalArgumentException("Advertisement did not contain a peer group ID");
        }
        
        PeerGroup theNewGroup = globalRegistry.lookupInstance(gid);
        
        if (theNewGroup != null) {
            return theNewGroup;
        }
        
        // We do not know if the grp adv had been previously published or not...  Since it may contain information essential to
        // the configuration of services, we need to make sure it is published localy, rather than letting the group publish
        // itself after the fact.
        
        // FIXME jice@jxta.org 20040713 : The downside is that we're publishing the adv even before making sure that this group
        // can really be instantiated. We're basically using the cm as a means to pass parameters to the module because it is a
        // group. We have the same parameter issue with the config adv. Eventually we need to find a clean way of passing
        // parameters specific to a certain types of module.
        
        try {
            discovery.publish(adv, DEFAULT_LIFETIME, DEFAULT_EXPIRATION);
        } catch (Exception any) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not publish the group advertisement: ", any);
            }
        }
        
        theNewGroup = (PeerGroup) loadModule(adv.getPeerGroupID(), adv.getModuleSpecID(), Here, false);
        
        if (theNewGroup == null) {
            throw new PeerGroupException("Could not find group implementation with " + adv.getModuleSpecID());
        }
        
        return (PeerGroup) theNewGroup.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerGroup newGroup(PeerGroupID gid, Advertisement impl, String name, String description) throws PeerGroupException {
        PeerGroup theNewGroup = null;
        
        if (null != gid) {
            theNewGroup = globalRegistry.lookupInstance(gid);
        }
        
        if (theNewGroup != null) {
            return theNewGroup;
        }
        
        try {
            theNewGroup = (PeerGroup) loadModule(gid, (ModuleImplAdvertisement) impl, false);
        } catch (Throwable any) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Could not load group implementation", any);
            }
            
            throw new PeerGroupException("Could not load group implementation", any);
        }
        
        try {
            // The group adv definitely needs to be published.
            theNewGroup.publishGroup(name, description);
        } catch (Exception any) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not publish group or implementation:", any);
            }
        }
        return (PeerGroup) theNewGroup.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerGroup newGroup(PeerGroupID gid) throws PeerGroupException {
        
        if ((gid == null) || ID.nullID.equals(gid)) {
            throw new IllegalArgumentException("Invalid peer group ID");
        }
        
        PeerGroup result = globalRegistry.lookupInstance(gid);
        
        if (result != null) {
            return result;
        }
        
        PeerGroupAdvertisement adv;
        
        try {
            adv = (PeerGroupAdvertisement)
                    discoverOne(DiscoveryService.GROUP, "GID", gid.toString(), 120, PeerGroupAdvertisement.class);
        } catch (Throwable any) {
            throw new PeerGroupException("Failed finding group advertisement for " + gid, any);
        }
        
        if (adv == null) {
            throw new PeerGroupException("Could not find group advertisement for group " + gid);
        }
        
        return newGroup(adv);
    }
    
    /**
     * {@inheritDoc}
     */
    public JxtaLoader getLoader() {
        return loader;
    }
    
    /**
     * {@inheritDoc}
     */
    public String getPeerName() {
        // before init we must fail.
        if (null == peerAdvertisement) {
            throw new IllegalStateException("PeerGroup not sufficiently initialized");
        }
        return peerAdvertisement.getName();
    }
    
    /**
     * {@inheritDoc}
     */
    public String getPeerGroupName() {
        // before init we must fail.
        if (null == peerGroupAdvertisement) {
            throw new IllegalStateException("PeerGroup not sufficiently initialized");
        }
        return peerGroupAdvertisement.getName();
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerGroupID getPeerGroupID() {
        // before init we must fail.
        if (null == peerGroupAdvertisement) {
            throw new IllegalStateException("PeerGroup not sufficiently initialized");
        }
        
        return peerGroupAdvertisement.getPeerGroupID();
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerID getPeerID() {
        // before init we must fail.
        if (null == peerAdvertisement) {
            throw new IllegalStateException("PeerGroup not sufficiently initialized");
        }
        return peerAdvertisement.getPeerID();
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerAdvertisement getPeerAdvertisement() {
        return peerAdvertisement;
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerGroupAdvertisement getPeerGroupAdvertisement() {
        return peerGroupAdvertisement;
    }
    
    /**
     * {@inheritDoc}
     */
    public boolean isRendezvous() {
        if (rendezvous == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Rendezvous service null");
            }
        }
        return (rendezvous != null) && rendezvous.isRendezVous();
    }
    
    /*
     * shortcuts to the well-known services, in order to avoid calls to lookup.
     */
    
    /**
     * {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        if (endpoint == null) {
            return null;
        }
        return (EndpointService) endpoint.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public ResolverService getResolverService() {
        if (resolver == null) {
            return null;
        }
        return (ResolverService) resolver.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public DiscoveryService getDiscoveryService() {
        if (discovery == null) {
            return null;
        }
        return (DiscoveryService) discovery.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public PeerInfoService getPeerInfoService() {
        if (peerinfo == null) {
            return null;
        }
        return (PeerInfoService) peerinfo.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public MembershipService getMembershipService() {
        if (membership == null) {
            return null;
        }
        return (MembershipService) membership.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public PipeService getPipeService() {
        if (pipe == null) {
            return null;
        }
        return (PipeService) pipe.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public RendezVousService getRendezVousService() {
        if (rendezvous == null) {
            return null;
        }
        return (RendezVousService) rendezvous.getInterface();
    }
    
    /**
     * {@inheritDoc}
     */
    public AccessService getAccessService() {
        if (access == null) {
            return null;
        }
        return (AccessService) access.getInterface();
    }

    /**
     * Returns the executor pool
     *
     * @return the executor pool
     */
    public Executor getExecutor() {
        return threadPool;
    }

    /**
     * Returns the scheduled executor. The
     *
     * @return the scheduled executor
     */
    public ScheduledExecutorService getScheduledExecutor() {
        // FIXME 20070815 bondolo We should return a proxy object to disable shutdown()
        return scheduledExecutor;
    }
    
    /**
     * Our rejected execution handler which has the effect of pausing the
     * caller until the task can be queued.
     */
    private static class CallerBlocksPolicy implements RejectedExecutionHandler {
        
        private CallerBlocksPolicy() {
        }
        
        /**
         * {@inheritDoc}
         */
        public void rejectedExecution(Runnable runnable, ThreadPoolExecutor executor) {
            BlockingQueue<Runnable> queue = executor.getQueue();

            while (!executor.isShutdown()) {
                executor.purge();

                try {
                    boolean pushed = queue.offer(runnable, 500, TimeUnit.MILLISECONDS);
                    
                    if (pushed) {
                        break;
                    }
                } catch (InterruptedException woken) {
                    throw new RejectedExecutionException("Interrupted while attempting to enqueue", woken);
                }
            }
        }
    }
    
    /**
     * Our thread factory that adds the threads to our thread group and names
     * the thread to something recognizable.
     */
    static class PeerGroupThreadFactory implements ThreadFactory {
        final AtomicInteger threadNumber = new AtomicInteger(1);
        final String name;
        final ThreadGroup threadgroup;

        PeerGroupThreadFactory(String name, ThreadGroup threadgroup) {
            this.name = name;
            this.threadgroup = threadgroup;
        }

        public Thread newThread(Runnable runnable) {
            Thread thread = new Thread(threadgroup, runnable,  name + " - " + threadNumber.getAndIncrement(), 0);
            if(thread.isDaemon()) {
                thread.setDaemon(false);
            }
            if (thread.getPriority() != Thread.NORM_PRIORITY) {
                thread.setPriority(Thread.NORM_PRIORITY);
            }
            return thread;
        }
    }
}
