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

package net.jxta.impl.pipe;


import net.jxta.document.Advertisement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.util.TimeUtils;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.OutputPipeEvent;
import net.jxta.pipe.OutputPipeListener;
import net.jxta.pipe.PipeID;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.platform.Module;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.service.Service;
import net.jxta.peer.PeerID;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.text.MessageFormat;


/**
 * A JXTA {@link net.jxta.pipe.PipeService} implementation which implements the
 * standard JXTA Pipe Resolver Protocol (PRP).
 * <p/>
 * This class provides implementation for Unicast, unicast secure and
 * (indirectly) propagate pipes.
 *
 * @see net.jxta.pipe.PipeService
 * @see net.jxta.pipe.InputPipe
 * @see net.jxta.pipe.OutputPipe
 * @see net.jxta.endpoint.Message
 * @see net.jxta.protocol.PipeAdvertisement
 * @see net.jxta.protocol.PipeResolverMessage
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pbp" target="_blank">JXTA Protocols Specification : Pipe Binding Protocol</a>
 */
public class PipeServiceImpl implements PipeService, PipeResolver.Listener {

    /**
     * The Logger
     */
    private final static Logger LOG = Logger.getLogger(PipeServiceImpl.class.getName());

    /**
     * the interval at which we verify that a pipe is still resolved at a
     * remote peer.
     */
    static final long VERIFYINTERVAL = 20 * TimeUtils.AMINUTE;

    /**
     * The group this PipeService is working for.
     */
    private PeerGroup group = null;

    /**
     * Our resolver handler.
     */
    private PipeResolver pipeResolver = null;

    /**
     * Link to wire pipe impl.
     */
    private WirePipeImpl wirePipe = null;

    /**
     * the interface object we will hand out.
     */
    private PipeService myInterface = null;

    /**
     * the impl advertisement for this impl.
     */
    private ModuleImplAdvertisement implAdvertisement = null;

    /**
     * Table of listeners for asynchronous output pipe creation.
     * <p/>
     * <ul>
     * <li>keys are {@link net.jxta.pipe.PipeID}</li>
     * <li>values are {@link java.util.Map}</li>
     * </ul>
     * Within the value Map:
     * <ul>
     * <li>keys are {@link java.lang.Integer} representing queryid</li>
     * <li>values are {@link OutputPipeHolder}</li>
     * </ul>
     */
    private final Map<PipeID, Map<Integer, OutputPipeHolder>> outputPipeListeners = new HashMap<PipeID, Map<Integer, OutputPipeHolder>>();

    /**
     * Has the pipe service been started?
     */
    private volatile boolean started = false;

    /**
     * holds a pipe adv and a listener which will be called for resolutions
     * of the pipe.
     */
    private static class OutputPipeHolder {
        final PipeAdvertisement adv;
        final Set<? extends ID> peers;
        final OutputPipeListener listener;
        final int queryid;

        OutputPipeHolder(PipeAdvertisement adv, Set<? extends ID> peers, OutputPipeListener listener, int queryid) {
            this.adv = adv;
            this.peers = peers;
            this.listener = listener;
            this.queryid = queryid;
        }
    }


    /**
     * A listener useful for implementing synchronous behaviour.
     */
    private static class syncListener implements OutputPipeListener {

        volatile OutputPipeEvent event = null;

        syncListener() {}

        /**
         * Called when a input pipe has been located for a previously registered
         * pipe. The event contains an {@link net.jxta.pipe.OutputPipe} which can
         * be used to communicate with the remote peer.
         *
         * @param event <code>net.jxta.pipe.outputPipeEvent</code> event
         */
        public synchronized void outputPipeEvent(OutputPipeEvent event) {
            // we only accept the first event.
            if (null == this.event) {
                this.event = event;
                notifyAll();
            }
        }
    }

    /**
     * Default Constructor (don't delete)
     */
    public PipeServiceImpl() {// What is reason for this constructor???
        // the same is automatically generated.
    }

    /**
     * {@inheritDoc}
     * <p/>
     * We create only a single interface object and return it over and over
     * again.
     */
    public synchronized PipeService getInterface() {
        if (null == myInterface) {
            myInterface = new PipeServiceInterface(this);
        }
        return myInterface;
    }

    /**
     * {@inheritDoc}
     */
    public ModuleImplAdvertisement getImplAdvertisement() {
        return implAdvertisement;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void init(PeerGroup group, ID assignedID, Advertisement impl) {

        this.group = group;
        implAdvertisement = (ModuleImplAdvertisement) impl;

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Pipe Service : " + assignedID);

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
            configInfo.append("\n\t\tVerify Interval : " + VERIFYINTERVAL + "ms");
            LOG.config(configInfo.toString());
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Currently this service does not expect arguments.
     */
    public synchronized int startApp(String[] args) {

        Service needed = group.getEndpointService();

        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }
            return START_AGAIN_STALLED;
        }

        needed = group.getResolverService();
        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a resolver service");
            }
            return START_AGAIN_STALLED;
        }

        needed = group.getMembershipService();
        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a membership service");
            }
            return START_AGAIN_STALLED;
        }

        needed = group.getRendezVousService();
        if (null == needed) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a rendezvous service");
            }
            return START_AGAIN_STALLED;
        }

        // create our resolver handler; it will register itself w/ the resolver.
        pipeResolver = new PipeResolver(group);

        // Create the WirePipe (propagated pipe)
        wirePipe = new WirePipeImpl(group, pipeResolver);

        // XXX 20061221 We could check the result of this.
        wirePipe.startApp(args);

        started = true;

        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void stopApp() {
        started = false;

        try {
            if (wirePipe != null) {
                wirePipe.stopApp();
            }
        } catch (Throwable failed) {
            LOG.log(Level.SEVERE, "Failed to stop wire pipe", failed);
        } finally {
            wirePipe = null;
        }

        try {
            if (pipeResolver != null) {
                pipeResolver.stop();
            }
        } catch (Throwable failed) {
            LOG.log(Level.SEVERE, "Failed to stop pipe resolver", failed);
        } finally {
            pipeResolver = null;
        }

        // Avoid cross-reference problem with GC
        group = null;
        myInterface = null;

        // clear outputPipeListeners
        Collection<Map<Integer, OutputPipeHolder>> values = outputPipeListeners.values();

        for (Map<Integer, OutputPipeHolder> value : values) {
            value.clear();
        }
        outputPipeListeners.clear();
    }

    /**
     * {@inheritDoc}
     */
    public InputPipe createInputPipe(PipeAdvertisement adv) throws IOException {
        return createInputPipe(adv, null);
    }

    /**
     * {@inheritDoc}
     */
    public InputPipe createInputPipe(PipeAdvertisement adv, PipeMsgListener listener) throws IOException {

        if (!started) {
            throw new IllegalStateException("Pipe Service has not been started or has been stopped");
        }

        String type = adv.getType();

        if (type == null) {
            throw new IllegalArgumentException("PipeAdvertisement type may not be null");
        }

        PipeID pipeId = (PipeID) adv.getPipeID();

        if (pipeId == null) {
            throw new IllegalArgumentException("PipeAdvertisement PipeID may not be null");
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Create " + type + " InputPipe for " + pipeId);
        }

        InputPipe inputPipe;
        // create an InputPipe.
        if (type.equals(PipeService.UnicastType)) {
            inputPipe = new InputPipeImpl(pipeResolver, adv, listener);
        } else if (type.equals(PipeService.UnicastSecureType)) {
            inputPipe = new SecureInputPipeImpl(pipeResolver, adv, listener);
        } else if (type.equals(PipeService.PropagateType)) {
            if (wirePipe != null) {
                inputPipe = wirePipe.createInputPipe(adv, listener);
            } else {
                throw new IOException("No propagated pipe servive available");
            }
        } else {
            // Unknown type
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Cannot create pipe for unknown type : " + type);
            }
            throw new IOException("Cannot create pipe for unknown type : " + type);
        }
        return inputPipe;
    }

    /**
     * {@inheritDoc}
     */
    public OutputPipe createOutputPipe(PipeAdvertisement pipeAdv, long timeout) throws IOException {
        return createOutputPipe(pipeAdv, Collections.<ID>emptySet(), timeout);
    }

    /**
     * {@inheritDoc}
     */
    public OutputPipe createOutputPipe(PipeAdvertisement adv, Set<? extends ID> resolvablePeers, long timeout) throws IOException {
        // convert zero to max value.
        if (0 == timeout) {
            timeout = Long.MAX_VALUE;
        }

        long absoluteTimeOut = TimeUtils.toAbsoluteTimeMillis(timeout);

        // Make a listener, start async resolution and then wait until the timeout expires.
        syncListener localListener = new syncListener();

        int queryid = PipeResolver.getNextQueryID();

        createOutputPipe(adv, resolvablePeers, localListener, queryid);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Waiting synchronously for " + timeout + "ms to resolve OutputPipe for " + adv.getPipeID());
        }

        try {
            synchronized (localListener) {
                while ((null == localListener.event) && (TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), absoluteTimeOut) < 0)) {
                    try {
                        localListener.wait(TimeUtils.ASECOND);
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                    }
                }
            }
        } finally {
            // remove the listener we installed.
            removeOutputPipeListener(adv.getPipeID().toString(), queryid);
        }

        if (null != localListener.event) {
            return localListener.event.getOutputPipe();
        } else {
            throw new IOException("Output Pipe could not be resolved after " + timeout + "ms.");
        }
    }

    /**
     * {@inheritDoc}
     */
    public void createOutputPipe(PipeAdvertisement pipeAdv, OutputPipeListener listener) throws IOException {
        createOutputPipe(pipeAdv, Collections.<ID>emptySet(), listener);
    }

    /**
     * {@inheritDoc}
     */
    public void createOutputPipe(PipeAdvertisement pipeAdv, Set<? extends ID> resolvablePeers, OutputPipeListener listener) throws IOException {
        createOutputPipe(pipeAdv, resolvablePeers, listener, PipeResolver.getNextQueryID());
    }

    private void createOutputPipe(PipeAdvertisement pipeAdv, Set<? extends ID> resolvablePeers, OutputPipeListener listener, int queryid) throws IOException {

        if (!started) {
            throw new IOException("Pipe Service has not been started or has been stopped");
        }

        // Recover the PipeId from the PipeServiceImpl Advertisement
        PipeID pipeId = (PipeID) pipeAdv.getPipeID();
        String type = pipeAdv.getType();

        if (null == type) {
            IllegalArgumentException failed = new IllegalArgumentException("Pipe type was not set");
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, failed.getMessage(), failed);
            }
            throw failed;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Create " + type + " OutputPipe for " + pipeId);
        }

        if (PipeService.PropagateType.equals(type)) {
            OutputPipe op;

            if (resolvablePeers.size() == 1) {
                op = new BlockingWireOutputPipe(group, pipeAdv, (PeerID) resolvablePeers.iterator().next());
            } else {
                if (wirePipe != null) {
                    op = wirePipe.createOutputPipe(pipeAdv, resolvablePeers);
                } else {
                    throw new IOException("No propagated pipe service available");
                }
            }

            if (null != op) {
                OutputPipeEvent newevent = new OutputPipeEvent(this.getInterface(), op, pipeId.toString(), PipeResolver.ANYQUERY);

                try {
                    listener.outputPipeEvent(newevent);
                } catch (Throwable ignored) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE,
                                "Uncaught Throwable in listener for " + pipeId + " (" + listener.getClass().getName() + ")",
                                ignored);
                    }
                }
            }
        } else if (PipeService.UnicastType.equals(type) || PipeService.UnicastSecureType.equals(type)) {

            addOutputPipeListener(pipeId, new OutputPipeHolder(pipeAdv, resolvablePeers, listener, queryid));
            pipeResolver.addListener(pipeId, this, queryid);
            pipeResolver.sendPipeQuery(pipeAdv, resolvablePeers, queryid);

            // look locally for the pipe
            if (resolvablePeers.isEmpty() || resolvablePeers.contains(group.getPeerID())) {
                InputPipe local = pipeResolver.findLocal(pipeId);

                // if we have a local instance, make sure the local instance is of the same type.
                if (null != local) {
                    if (local.getType().equals(pipeAdv.getType())) {
                        pipeResolver.callListener(queryid, pipeId, local.getType(), group.getPeerID(), false);
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning(MessageFormat.format("rejecting local pipe ({0}) because type is not ({1})", local.getType(),
                                    pipeAdv.getType()));
                        }
                    }
                }
            }
        } else {
            // Unknown type
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("createOutputPipe: cannot create pipe for unknown type : " + type);
            }
            throw new IOException("cannot create pipe for unknown type : " + type);
        }
    }

    /*
     * Add an output listener for the given pipeId.
     */
    private void addOutputPipeListener(PipeID pipeId, OutputPipeHolder pipeHolder) {
        synchronized (outputPipeListeners) {
            Map<Integer, OutputPipeHolder> perpipelisteners = outputPipeListeners.get(pipeId);

            if (perpipelisteners == null) {
                perpipelisteners = new HashMap<Integer, OutputPipeHolder>();
                outputPipeListeners.put(pipeId, perpipelisteners);
            }
            if (perpipelisteners.get(pipeHolder.queryid) != null) {
                LOG.warning("Clobbering output pipe listener for query " + pipeHolder.queryid);
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Adding pipe listener for pipe " + pipeId + " and query " + pipeHolder.queryid);
            }
            perpipelisteners.put(pipeHolder.queryid, pipeHolder);
        }
    }

    /**
     * {@inheritDoc}
     */
    public OutputPipeListener removeOutputPipeListener(String pipeID, OutputPipeListener listener) {
        throw new UnsupportedOperationException("Legacy method not supported. Use interface object if you need this method.");
    }

    /**
     * {@inheritDoc}
     */
    public OutputPipeListener removeOutputPipeListener(ID pipeID, OutputPipeListener listener) {

        // remove all instances of this listener, regardless of queryid
        if (pipeResolver == null) {
            return null;
        }

        if (!(pipeID instanceof PipeID)) {
            throw new IllegalArgumentException("pipeID must be a PipeID.");
        }

        synchronized (outputPipeListeners) {
            Map<Integer, OutputPipeHolder> perpipelisteners = outputPipeListeners.get(pipeID);

            if (perpipelisteners != null) {
                Set<Map.Entry<Integer, OutputPipeHolder>> entries = perpipelisteners.entrySet();

                for (Map.Entry<Integer, OutputPipeHolder> entry : entries) {
                    OutputPipeHolder pl = entry.getValue();

                    if (pl.listener == listener) {
                        pipeResolver.removeListener((PipeID) pipeID, pl.queryid);
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Removing listener for query " + pl.queryid);
                        }
                        perpipelisteners.remove(entry.getKey());
                    }
                }
                // clean up the map if there are no more listeners for the pipe
                if (perpipelisteners.isEmpty()) {
                    outputPipeListeners.remove(pipeID);
                }
            }
        }
        return listener;
    }

    private OutputPipeListener removeOutputPipeListener(String opID, int queryID) {

        if (pipeResolver == null) {
            return null;
        }

        PipeID pipeID;
        try {
            URI aPipeID = new URI(opID);
            pipeID = (PipeID) IDFactory.fromURI(aPipeID);
        } catch (URISyntaxException badID) {
            throw new IllegalArgumentException("Bad pipe ID: " + opID);
        } catch (ClassCastException badID) {
            throw new IllegalArgumentException("id was not a pipe id: " + opID);
        }

        synchronized (outputPipeListeners) {
            Map<Integer, OutputPipeHolder> perpipelisteners = outputPipeListeners.get(pipeID);

            if (perpipelisteners != null) {
                OutputPipeHolder pipeHolder = perpipelisteners.get(queryID);

                perpipelisteners.remove(queryID);
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Removing listener for query " + queryID);
                }
                // clean up the map if there are no more listeners for the pipe
                if (perpipelisteners.isEmpty()) {
                    outputPipeListeners.remove(pipeID);
                }
                pipeResolver.removeListener(pipeID, queryID);
                if (pipeHolder != null) {
                    return pipeHolder.listener;
                }
            }

        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    public boolean pipeResolveEvent(PipeResolver.Event event) {

        try {
            ID peerID = event.getPeerID();
            ID pipeID = event.getPipeID();
            int queryID = event.getQueryID();
            OutputPipeHolder pipeHolder;

            synchronized (outputPipeListeners) {
                Map<Integer, OutputPipeHolder> perpipelisteners = outputPipeListeners.get(pipeID);

                if (perpipelisteners == null) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("No listener for event for pipe " + pipeID);
                    }
                    return false;
                }
                pipeHolder = perpipelisteners.get(queryID);
                if (pipeHolder == null) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("No listener for event for query " + queryID);
                    }
                    return false;
                }
            }

            // check if they wanted a resolve from a specific peer.
            if (!pipeHolder.peers.isEmpty() && !pipeHolder.peers.contains(peerID)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Event was for wrong peer \'" + peerID + "\'. Discarding.");
                }
                return false;
            }

            // create op
            String type = pipeHolder.adv.getType();
            OutputPipe op;

            if (PipeService.UnicastType.equals(type)) {
                op = new NonBlockingOutputPipe(group, pipeResolver, pipeHolder.adv, peerID, pipeHolder.peers);
            } else if (PipeService.UnicastSecureType.equals(type)) {
                op = new SecureOutputPipe(group, pipeResolver, pipeHolder.adv, peerID, pipeHolder.peers);
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Could not create output pipe of type \'" + type + "\'. Discarding.");
                }
                return false;
            }

            // Generate an event when the output pipe was succesfully opened.
            OutputPipeEvent newevent = new OutputPipeEvent(this.getInterface(), op, pipeID.toString(), queryID);
            try {
                pipeHolder.listener.outputPipeEvent(newevent);
            } catch (Throwable ignored) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE
                            ,
                            "Uncaught Throwable in listener for " + pipeID + "(" + pipeHolder.getClass().getName() + ")", ignored);
                }
            }
            removeOutputPipeListener(pipeID.toString(), queryID);
            return true;
        } catch (IOException ie) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Error creating output pipe " + event.getPipeID(), ie);
            }
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("No listener for event for " + event.getPipeID());
        }
        return false;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * We don't do anything with NAKs (yet)
     */
    public boolean pipeNAKEvent(PipeResolver.Event event) {
        return false;
    }
}
