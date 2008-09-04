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
package net.jxta.impl.rendezvous;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Random;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Vector;
import java.util.concurrent.atomic.AtomicBoolean;
import java.io.IOException;
import java.util.Collection;
import java.util.List;
import java.util.NoSuchElementException;

import java.util.logging.Level;

import net.jxta.logging.Logging;

import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.id.ID;
import net.jxta.impl.endpoint.EndpointUtils;
import net.jxta.meter.MonitorResources;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.platform.Module;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.RdvAdvertisement;
import net.jxta.protocol.RouteAdvertisement;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.rendezvous.RendezvousEvent;
import net.jxta.rendezvous.RendezVousStatus;
import net.jxta.rendezvous.RendezvousListener;
import net.jxta.service.Service;

import net.jxta.impl.id.UUID.UUID;
import net.jxta.impl.id.UUID.UUIDFactory;
import net.jxta.impl.meter.MonitorManager;
import net.jxta.impl.protocol.RdvConfigAdv;
import net.jxta.impl.rendezvous.adhoc.AdhocPeerRdvService;
import net.jxta.impl.rendezvous.edge.EdgePeerRdvService;
import net.jxta.impl.rendezvous.rdv.RdvPeerRdvService;
import net.jxta.impl.rendezvous.rendezvousMeter.RendezvousMeterBuildSettings;
import net.jxta.impl.rendezvous.rendezvousMeter.RendezvousServiceMonitor;
import net.jxta.impl.rendezvous.rpv.PeerView;
import net.jxta.impl.rendezvous.rpv.PeerViewElement;
import net.jxta.impl.util.TimeUtils;

/**
 * A JXTA {@link net.jxta.rendezvous.RendezVousService} implementation which
 * implements the standard JXTA Rendezvous Protocol (RVP).
 *
 * @see net.jxta.rendezvous.RendezVousService
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-rvp" target="_blank">JXTA Protocols Specification : Rendezvous Protocol</a>
 */
public final class RendezVousServiceImpl implements RendezVousService {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(RendezVousServiceImpl.class.getName());

    private final static long rdv_watchdog_interval_default = 5 * TimeUtils.AMINUTE; // 5 Minutes

    private static final double DEMOTION_FACTOR = 0.05;
    private static final long DEMOTION_MIN_PEERVIEW_COUNT = 5;
    private static final long DEMOTION_MIN_CLIENT_COUNT = 3;
    protected static final int MAX_MSGIDS = 1000;

    private final static Random random = new Random();

    private PeerGroup group = null;
    private ID assignedID = null;
    private ModuleImplAdvertisement implAdvertisement = null;

    public EndpointService endpoint = null;

    private RendezvousServiceMonitor rendezvousServiceMonitor;

    private Timer timer;
    private RdvWatchdogTask autoRdvTask = null;

    private long rdv_watchdog_interval = 5 * TimeUtils.AMINUTE; // 5 Minutes

    private final Set<RendezvousListener> eventListeners = Collections.synchronizedSet(new HashSet<RendezvousListener>());

    /**
     * The message IDs we have seen. Used for duplicate removal.
     */
    private final List<UUID> msgIds = new ArrayList<UUID>(MAX_MSGIDS);

    /**
     * Total number of messages which have been received.
     */
    private int messagesReceived;

    private RdvConfigAdv.RendezVousConfiguration config = RdvConfigAdv.RendezVousConfiguration.EDGE;
    private boolean autoRendezvous = false;

    private String[] savedArgs = null;

    /**
     * If {@code true} then a rdv provider change is in progress.
     */
    private AtomicBoolean rdvProviderSwitchStatus = new AtomicBoolean(false);

    /**
     * The current provider
     */
    private RendezVousServiceProvider provider = null;

    /**
     * Our interface object. We currently always return the same object.
     */
    private final RendezVousServiceInterface rendezvousInterface = new RendezVousServiceInterface(this);

    /**
     * Constructor for the RendezVousServiceImpl object
     */
    public RendezVousServiceImpl() {
    }

    /**
     * {@inheritDoc}
     */
    public RendezVousService getInterface() {
        return rendezvousInterface;
    }

    /**
     * {@inheritDoc}
     */
    public ModuleImplAdvertisement getImplAdvertisement() {
        return implAdvertisement;
    }

    /**
     * Return the assigned ID for this service.
     *
     * @return The assigned ID for this service.
     */
    public ID getAssignedID() {
        return assignedID;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/><b>Note</b>: it is permissible to pass null as the impl parameter
     * when this instance is not being loaded via the module framework.
     */
    public synchronized void init(PeerGroup g, ID assignedID, Advertisement impl) {
        this.group = g;
        this.assignedID = assignedID;
        this.implAdvertisement = (ModuleImplAdvertisement) impl;

        RdvConfigAdv rdvConfigAdv = null;
        ConfigParams confAdv = group.getConfigAdvertisement();

        // Get the config. If we do not have a config, we're done; we just keep
        // the defaults (edge peer/no auto-rdv)
        if (confAdv != null) {
            Advertisement adv = null;

            try {
                XMLDocument configDoc = (XMLDocument) confAdv.getServiceParam(getAssignedID());

                if (null != configDoc) {
                    adv = AdvertisementFactory.newAdvertisement(configDoc);
                }
            } catch (NoSuchElementException failed) {
                //ignored
            }

            if (adv instanceof RdvConfigAdv) {
                rdvConfigAdv = (RdvConfigAdv) adv;
            }
        }

        if (null == rdvConfigAdv) {
            // Make a new advertisement for defaults.
            rdvConfigAdv = (RdvConfigAdv) AdvertisementFactory.newAdvertisement(RdvConfigAdv.getAdvertisementType());
        }

        config = rdvConfigAdv.getConfiguration();

        autoRendezvous = rdvConfigAdv.getAutoRendezvousCheckInterval() > 0;

        rdv_watchdog_interval = rdvConfigAdv.getAutoRendezvousCheckInterval();

        // force AD-HOC config for World Peer Group.
        if (PeerGroupID.worldPeerGroupID.equals(group.getPeerGroupID())) {
            config = RdvConfigAdv.RendezVousConfiguration.AD_HOC;
        }

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring RendezVous Service : " + assignedID);

            if (implAdvertisement != null) {
                configInfo.append("\n\tImplementation :");
                configInfo.append("\n\t\tModule Spec ID: ").append(implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description : ").append(implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI : ").append(implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code : ").append(implAdvertisement.getCode());
            }

            configInfo.append("\n\tGroup Params :");
            configInfo.append("\n\t\tGroup : ").append(group);
            configInfo.append("\n\t\tPeer ID : ").append(group.getPeerID());

            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tRendezVous : ").append(config);
            configInfo.append("\n\t\tAuto RendezVous : ").append(autoRendezvous);
            configInfo.append("\n\t\tAuto-RendezVous Reconfig Interval : ").append(rdv_watchdog_interval);

            LOG.config(configInfo.toString());
        }

        // "start" a rendezvous provider switch. We will finish in startApp()
        rdvProviderSwitchStatus.set(true);
    }

    /**
     * {@inheritDoc}
     */
    public int startApp(String[] arg) {
        endpoint = group.getEndpointService();

        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }

            return START_AGAIN_STALLED;
        }

        Service needed = group.getMembershipService();

        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a membership service");
            }

            return START_AGAIN_STALLED;
        }

        // if( !PeerGroupID.worldPeerGroupID.equals(group.getPeerGroupID())) {
        // MessageTransport router = endpoint.getMessageTransport( "jxta" );
        //
        // if( null == router ) {
        // if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
        // LOG.warning("Stalled until there is a router ");
        // }
        //
        // return START_AGAIN_STALLED;
        // }
        // }
        //

        timer = new Timer("RendezVousServiceImpl Timer for " + group.getPeerGroupID(), true);

        if (!rdvProviderSwitchStatus.compareAndSet(true, true)) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Unable to start rendezvous provider.");
            }

            return -1;
        }

        if (RdvConfigAdv.RendezVousConfiguration.AD_HOC == config) {
            provider = new AdhocPeerRdvService(group, this);
        } else if (RdvConfigAdv.RendezVousConfiguration.EDGE == config) {
            provider = new EdgePeerRdvService(group, this);
        } else if (RdvConfigAdv.RendezVousConfiguration.RENDEZVOUS == config) {
            provider = new RdvPeerRdvService(group, this);
        } else {
            throw new IllegalStateException("Unrecognized rendezvous configuration");
        }

        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING) {
            rendezvousServiceMonitor = (RendezvousServiceMonitor) MonitorManager.getServiceMonitor(group
                    ,
                    MonitorResources.rendezvousServiceMonitorClassID);
            provider.setRendezvousServiceMonitor(rendezvousServiceMonitor);
        }

        provider.startApp(null);

        rdvProviderSwitchStatus.set(false);

        if (autoRendezvous && !PeerGroupID.worldPeerGroupID.equals(group.getPeerGroupID())) {
            startWatchDogTimer();
        }

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Rendezvous Serivce started");
        }

        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void stopApp() {

        // We won't ever release this lock. We are shutting down. There is
        // no reason to switch after stopping is begun.
        rdvProviderSwitchStatus.set(true);

        if (provider != null) {
            provider.stopApp();
            provider = null;
        }

        timer.cancel();
        timer = null;

        msgIds.clear();
        eventListeners.clear();

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Rendezvous Serivce stopped");
        }
    }

    /**
     * {@inheritDoc}
     */
    public boolean isRendezVous() {
        RendezVousStatus currentStatus = getRendezVousStatus();

        return (RendezVousStatus.AUTO_RENDEZVOUS == currentStatus) || (RendezVousStatus.RENDEZVOUS == currentStatus);
    }

    /**
     * @inheritDoc
     */
    public RendezVousStatus getRendezVousStatus() {
        RendezVousServiceProvider currentProvider = provider;

        if (null == currentProvider) {
            return RendezVousStatus.NONE;
        } else if (currentProvider instanceof AdhocPeerRdvService) {
            return RendezVousStatus.ADHOC;
        } else if (currentProvider instanceof EdgePeerRdvService) {
            return autoRendezvous ? RendezVousStatus.AUTO_EDGE : RendezVousStatus.EDGE;
        } else if (currentProvider instanceof RdvPeerRdvService) {
            return autoRendezvous ? RendezVousStatus.AUTO_RENDEZVOUS : RendezVousStatus.RENDEZVOUS;
        } else {
            return RendezVousStatus.UNKNOWN;
        }
    }

    /**
     * {@inheritDoc}
     */
    public boolean setAutoStart(boolean auto) {
        return setAutoStart(auto, rdv_watchdog_interval_default);
    }

    /**
     * {@inheritDoc}
     */
    public synchronized boolean setAutoStart(boolean auto, long period) {
        rdv_watchdog_interval = period;
        boolean old = autoRendezvous;

        autoRendezvous = auto;

        if (auto && !old) {
            startWatchDogTimer();
        } else if (old && !auto) {
            stopWatchDogTimer();
        }
        return old;
    }

    /**
     * Attempt to connect to the specified rendezvous peer.
     *
     * @param addr The endpoint address of the rendezvous peer.
     * @param hint An optional hint which may be {@code null}.
     * @throws IOException If no connection could be made to the specified
     *                     peer.
     */
    private void connectToRendezVous(EndpointAddress addr, RouteAdvertisement hint) throws IOException {
        RendezVousServiceProvider currentProvider = provider;

        if (currentProvider != null) {
            currentProvider.connectToRendezVous(addr, hint);
        } else {
            throw new IOException("Currently switching rendezvous roles.");
        }
    }

    /**
     * {@inheritDoc}
     */
    public void connectToRendezVous(PeerAdvertisement adv) throws IOException {
        EndpointAddress addr = new EndpointAddress("jxta", adv.getPeerID().getUniqueValue().toString(), null, null);
        connectToRendezVous(addr, EndpointUtils.extractRouteAdv(adv));
    }

    /**
     * {@inheritDoc}
     */
    public void connectToRendezVous(EndpointAddress addr) throws IOException {
        connectToRendezVous(addr, null);
    }

    /**
     * {@inheritDoc}
     */
    public void challengeRendezVous(ID peer, long delay) {
        RendezVousServiceProvider currentProvider = provider;

        if (currentProvider != null) {
            currentProvider.challengeRendezVous(peer, delay);
        }
    }

    /**
     * {@inheritDoc}
     */
    public void disconnectFromRendezVous(ID peerId) {

        RendezVousServiceProvider currentProvider = provider;

        if (currentProvider != null) {
            currentProvider.disconnectFromRendezVous(peerId);
        }
    }

    /**
     * {@inheritDoc}
     */
    public Enumeration<ID> getConnectedRendezVous() {
        throw new UnsupportedOperationException("Deprecated opertaion. Use interface if you want to use this operation.");
    }

    /**
     * {@inheritDoc}
     */
    public Enumeration<ID> getDisconnectedRendezVous() {
        throw new UnsupportedOperationException("Deprecated opertaion. Use interface if you want to use this operation.");
    }

    /**
     * {@inheritDoc}
     */
    public Enumeration<ID> getConnectedPeers() {
        throw new UnsupportedOperationException("Deprecated opertaion. Use interface if you want to use this operation.");
    }

    /**
     * {@inheritDoc}
     */
    public Vector<ID> getConnectedPeerIDs() {
        RendezVousServiceProvider currentProvider = provider;

        if (currentProvider != null) {
            return currentProvider.getConnectedPeerIDs();
        }
        return new Vector<ID>();
    }

    /**
     * Gets the rendezvousConnected attribute of the RendezVousServiceImpl object
     *
     * @return true if connected to a rendezvous, false otherwise
     */
    public boolean isConnectedToRendezVous() {
        RendezVousServiceProvider currentProvider = provider;
        return currentProvider != null && currentProvider.isConnectedToRendezVous();
    }

    /**
     * {@inheritDoc}
     */
    public void startRendezVous() {
        try {
            if (isRendezVous() || PeerGroupID.worldPeerGroupID.equals(group.getPeerGroupID())) {
                return;
            }

            if (!rdvProviderSwitchStatus.compareAndSet(false, true)) {
                IOException failed = new IOException("Currently switching rendezvous configuration. try again later.");

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Failed to start rendezvous", failed);
                }
                throw failed;
            }

            // We are at this moment an Edge Peer. First, the current implementation
            // must be stopped.
            if (provider != null) {
                provider.stopApp();
                provider = null;
            }

            config = RdvConfigAdv.RendezVousConfiguration.RENDEZVOUS;

            // Now, a new instance of RdvPeerRdvService must be created and initialized.
            provider = new RdvPeerRdvService(group, this);

            if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING) {
                provider.setRendezvousServiceMonitor(rendezvousServiceMonitor);
            }

            provider.startApp(savedArgs);

            rdvProviderSwitchStatus.set(false);
        } catch (IOException failure) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to start rendezvous", failure);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void stopRendezVous() {

        if (!isRendezVous()) {
            return;
        }

        if (!rdvProviderSwitchStatus.compareAndSet(false, true)) {
            IOException failed = new IOException("Currently switching rendezvous configuration. try again later.");

            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Failed to stop rendezvous", failed);
            }
        }

        // If the service was already started, then it needs to be stopped,
        // and a new instance of an EdgePeerRdvService must be created and initialized and
        // started.

        if (provider != null) {
            provider.stopApp();
            provider = null;
        }

        config = RdvConfigAdv.RendezVousConfiguration.EDGE;

        provider = new EdgePeerRdvService(group, this);

        if (RendezvousMeterBuildSettings.RENDEZVOUS_METERING) {
            provider.setRendezvousServiceMonitor(rendezvousServiceMonitor);
        }

        provider.startApp(savedArgs);

        rdvProviderSwitchStatus.set(false);
    }

    /**
     * {@inheritDoc}
     */
    public boolean addPropagateListener(String serviceName, String serviceParam, EndpointListener listener) {
        if (null == endpoint) {
            throw new IllegalStateException("Unable to register propagate listener. (not started)");
        }

        return endpoint.addIncomingMessageListener(listener, serviceName, serviceParam);
    }

    /**
     * {@inheritDoc}
     */
    public EndpointListener removePropagateListener(String serviceName, String serviceParam, EndpointListener listener) {

        if (null == endpoint) {
            throw new IllegalStateException("Unable to remove propagate listener. (not started)");
        }

        EndpointListener removed = endpoint.removeIncomingMessageListener(serviceName, serviceParam);

        if ((removed != listener) && (null != removed)) {
            // Not the listener we expected.
            // It's kind of bad that we removed it at all, but putting it back should fix things.
            endpoint.addIncomingMessageListener(removed, serviceName, serviceParam);
            return null;
        }
        return listener;
    }

    /**
     * {@inheritDoc}
     */
    public void propagate(Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {

        RendezVousServiceProvider currentProvider = provider;

        if (null == currentProvider) {
            throw new IOException("No RDV provider");
        }
        currentProvider.propagate(msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void propagate(Enumeration<? extends ID> destPeerIDs, Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {

        RendezVousServiceProvider currentProvider = provider;

        if (null == currentProvider) {
            throw new IOException("No RDV provider");
        }
        currentProvider.propagate(destPeerIDs, msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void walk(Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {
        RendezVousServiceProvider currentProvider = provider;

        if (null == currentProvider) {
            throw new IOException("No RDV provider");
        }
        currentProvider.walk(msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void walk(Vector<? extends ID> destPeerIDs, Message msg, String serviceName, String serviceParam, int defaultTTL) throws IOException {

        RendezVousServiceProvider currentProvider = provider;

        if (null == currentProvider) {
            throw new IOException("No RDV provider");
        }
        currentProvider.walk(destPeerIDs, msg, serviceName, serviceParam, defaultTTL);
    }

    /**
     * {@inheritDoc}
     */
    public Vector<RdvAdvertisement> getLocalWalkView() {

        Vector<RdvAdvertisement> result = new Vector<RdvAdvertisement>();

        PeerView currView = getPeerView();

        if (null == currView) {
            return result;
        }

        Collection<PeerViewElement> allPVE = new ArrayList<PeerViewElement>(currView.getView());

        for (PeerViewElement pve : allPVE) {
            RdvAdvertisement adv = pve.getRdvAdvertisement();
            result.add(adv);
        }

        return result;
    }

    /**
     * Returns the PeerView
     *
     * @return the PeerView
     */
    public PeerView getPeerView() {
        RendezVousServiceProvider currentProvider = provider;

        if (currentProvider instanceof RdvPeerRdvService) {
            return ((RdvPeerRdvService) currentProvider).rpv;
        } else {
            return null;
        }
    }

    /**
     * {@inheritDoc}
     */
    public void propagateToNeighbors(Message msg, String serviceName, String serviceParam, int ttl) throws IOException {
        RendezVousServiceProvider currentProvider = provider;

        if (null == currentProvider) {
            throw new IOException("No RDV provider");
        }
        currentProvider.propagateToNeighbors(msg, serviceName, serviceParam, ttl);
    }

    /**
     * {@inheritDoc}
     */
    public void propagateInGroup(Message msg, String serviceName, String serviceParam, int ttl) throws IOException {
        RendezVousServiceProvider currentProvider = provider;
        if (null == currentProvider) {
            throw new IOException("No RDV provider");
        }
        currentProvider.propagateInGroup(msg, serviceName, serviceParam, ttl);
    }

    /**
     * {@inheritDoc}
     */
    public final void addListener(RendezvousListener listener) {
        eventListeners.add(listener);
    }

    /**
     * {@inheritDoc}
     */
    public final boolean removeListener(RendezvousListener listener) {
        return eventListeners.remove(listener);
    }

    /**
     * Creates a rendezvous event and sends it to all registered listeners.
     *
     * @param type      event type
     * @param regarding event peer ID
     */
    public final void generateEvent(int type, ID regarding) {

        Iterator eachListener = Arrays.asList(eventListeners.toArray()).iterator();
        RendezvousEvent event = new RendezvousEvent(getInterface(), type, regarding);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Calling listeners for " + event);
        }

        while (eachListener.hasNext()) {
            RendezvousListener aListener = (RendezvousListener) eachListener.next();

            try {
                aListener.rendezvousEvent(event);
            } catch (Throwable ignored) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Uncaught Throwable in listener (" + aListener + ")", ignored);
                }
            }
        }
    }

    private synchronized void startWatchDogTimer() {
        stopWatchDogTimer();

        autoRdvTask = new RdvWatchdogTask();

        // Now that we have an Auto-switch flag we only use the higher timeout
        // if auto-switch is off .
        // Set a watchdog, so the peer will become rendezvous if, after rdv_watchdog_interval it
        // still has not connected to any rendezvous peer.
        timer.schedule(autoRdvTask, rdv_watchdog_interval, rdv_watchdog_interval);
    }

    private synchronized void stopWatchDogTimer() {
        RdvWatchdogTask tw = autoRdvTask;

        if (tw != null) {
            autoRdvTask.cancel();
            autoRdvTask = null;
        }
    }

    /**
     * Edge Peer mode connection watchdog.
     */
    private class RdvWatchdogTask extends TimerTask {

        /**
         * {@inheritDoc}
         */
        @Override
        public synchronized void run() {
            try {
                int connectedPeers = getConnectedPeerIDs().size();

                if (!isRendezVous()) {
                    if (0 == connectedPeers) {
                        // This peer has not been able to connect to any rendezvous peer.
                        // become one.

                        // become a rendezvous peer.
                        startRendezVous();
                    }
                } else {
                    // Perhaps we can demote ourselves back to an edge

                    int peerViewSize = getLocalWalkView().size();

                    boolean isManyElementsInPeerView = (peerViewSize > DEMOTION_MIN_PEERVIEW_COUNT);
                    boolean isFewClients = (connectedPeers < DEMOTION_MIN_CLIENT_COUNT);

                    if (isManyElementsInPeerView) {
                        if (connectedPeers == 0) {
                            // Demote ourselves if there are no clients and
                            // there are more than the minimum rendezvous around
                            stopRendezVous();
                        } else if (isFewClients && (RendezVousServiceImpl.random.nextDouble() < DEMOTION_FACTOR)) {
                            // Randomly Demote ourselves if there are few clients and
                            // there are many rendezvous
                            stopRendezVous();
                        }
                    }
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in Timer : " + Thread.currentThread().getName(), all);
                }
            }
        }
    }

    public boolean isMsgIdRecorded(UUID id) {

        boolean found;

        synchronized (msgIds) {
            found = msgIds.contains(id);
        }

        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer(id + " = " + found);
        }

        return found;
    }

    /**
     * Checks if a message id has been recorded.
     *
     * @param id message to record.
     * @return {@code true} If message was added otherwise (duplicate)
     *         {@code false}.
     */
    public boolean addMsgId(UUID id) {

        synchronized (msgIds) {
            if (isMsgIdRecorded(id)) {
                // Already there. Nothing to do
                return false;
            }

            if (msgIds.size() < MAX_MSGIDS) {
                msgIds.add(id);
            } else {
                msgIds.set((messagesReceived % MAX_MSGIDS), id);
            }

            messagesReceived++;
        }

        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Added Message ID : " + id);
        }

        return true;
    }

    public UUID createMsgId() {
        return UUIDFactory.newSeqUUID();
    }

    /**
     * Get the current provider. This is for debugging purposes only.
     *
     * @return the provider
     * @deprecated This is private for debugging and diagnostics only.
     */
    @Deprecated
    net.jxta.impl.rendezvous.RendezVousServiceProvider getRendezvousProvider() {
        return provider;
    }
}
