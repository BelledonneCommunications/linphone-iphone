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
package net.jxta.impl.endpoint;

import net.jxta.document.Advertisement;
import net.jxta.endpoint.ChannelMessenger;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.ListenerAdaptor;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageFilterListener;
import net.jxta.endpoint.MessageTransport;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.MessengerEventListener;
import net.jxta.id.ID;
import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.peergroup.StdPeerGroup;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.ModuleImplAdvertisement;

import java.lang.ref.Reference;
import java.lang.ref.WeakReference;
import java.util.Iterator;
import java.util.Map;
import java.util.WeakHashMap;

import net.jxta.platform.Module;

/**
 * Provides an interface object appropriate for applications using the endpoint
 * service. The interface provides a number of convenience features and
 * implementation necessary for legacy features.
 */
class EndpointServiceInterface implements EndpointService {

    /**
     * The service interface that we will be fronting.
     */
    private final EndpointServiceImpl theRealThing;

    /**
     * The number of active instances of this class. We use this for deciding
     * when to instantiate and shutdown the listener adaptor.
     */
    private static int activeInstanceCount = 0;

    /**
     * Provides emulation of the legacy send-message-with-listener and get-messenger-with-listener APIs.
     */
    private static ListenerAdaptor listenerAdaptor;

    /**
     * The cache of channels. If a given owner of this EndpointService interface
     * object requests channels for the same exact destination multiple times,
     * we will return the same channel object as much as possible.  We keep
     * channels in a weak map, so that when channels are discarded, they
     * eventually disappear.  Channels that have messages in them are always
     * referenced. Therefore, this prevents the creation of more than one
     * channel with messages in it for the same destination in the same context
     * (owner of interface object - typically one module). This is required to
     * properly support the common (and convenient) pattern:
     * <p/>
     * <code>m = endpointServiceInterface.getMessenger(); messenger.sendMessage(); m = null;</code>
     * <p/>
     * If that was not kept in check, it would be possible to inadvertently
     * create an infinite number of channels with pending messages, thus an
     * infinite number of messages too.
     */
    private final Map<EndpointAddress, Reference<Messenger>> channelCache = new WeakHashMap<EndpointAddress, Reference<Messenger>>();

    /**
     * Builds a new interface object.
     *
     * @param endpointService the endpoint service that we will front.
     */
    public EndpointServiceInterface(EndpointServiceImpl endpointService) {
        theRealThing = endpointService;
        synchronized (this.getClass()) {
            activeInstanceCount++;
            if (1 == activeInstanceCount) {
                listenerAdaptor = new ListenerAdaptor(Thread.currentThread().getThreadGroup(), ((StdPeerGroup) endpointService.getGroup()).getExecutor());
            }
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This is rather heavy-weight if instances are frequently created and
     * discarded since finalization significantly delays GC.
     */
    @Override
    protected void finalize() throws Throwable {
        synchronized (this.getClass()) {
            activeInstanceCount--;
            if (0 == activeInstanceCount) {
                listenerAdaptor.shutdown();
                listenerAdaptor = null;
            }
        }
        super.finalize();
    }

    /**
     * {@inheritDoc}
     * <p/>
     * it is there only to satisfy the requirements of the interface that we
     * implement. Ultimately, the API should define two levels of interfaces :
     * one for the real service implementation and one for the interface object.
     * Right now it feels a bit heavy to so that since the only different
     * between the two would be init() and may-be getName().
     */
    public void init(PeerGroup peerGroup, ID id, Advertisement implAdv) {
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This is here for temporary class hierarchy reasons.
     * it is ALWAYS ignored. By definition, the interface object
     * protects the real object's start/stop methods from being called
     */
    public int startApp(String[] arg) {
        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This is here for temporary class hierarchy reasons.
     * it is ALWAYS ignored. By definition, the interface object
     * protects the real object's start/stop methods from being called
     * <p/>
     * This request is currently ignored.
     */
    public void stopApp() {
    }

    /**
     * {@inheritDoc}
     */
    public ModuleImplAdvertisement getImplAdvertisement() {
        return theRealThing.getImplAdvertisement();
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Sort of absurd but this is part of the API we're implementing.
     * We would not do a two-level API just for that.
     */
    public EndpointService getInterface() {
        return this;
    }

    /**
     * {@inheritDoc}
     */
    public PeerGroup getGroup() {
        return theRealThing.getGroup();
    }

    /**
     * {@inheritDoc}
     */
    public Messenger getCanonicalMessenger(EndpointAddress addr, Object hint) {
        // XXX: maybe we should enforce the stripping of the address here.
        // That would prevent application from making canonical messengers with a variety of service names and
        // service params. On the other hand that would cost useless cloning of endp addrs and prevent future
        // flexibility regarding QOS params, possibly. Be liberal for now.
        return theRealThing.getCanonicalMessenger(addr, hint);
    }

    /**
     * {@inheritDoc}
     */
    public Messenger getMessengerImmediate(EndpointAddress addr, Object hint) {
        // Note: for now, the hint is not used for canonicalization (hint != QOS).
        synchronized (channelCache) {
            Reference<Messenger> existing = channelCache.get(addr);

            if (existing != null) {
                Messenger messenger = existing.get();
                if ((messenger != null) && ((messenger.getState() & Messenger.USABLE) != 0)) {
                    return messenger;
                }
            }
        }
        
        // We do not have a good one at hand. Make a new one.

        // Use the stripped address to get a canonical msngr; not doing so
        // would reduce the sharing to almost nothing.
        EndpointAddress plainAddr = new EndpointAddress(addr, null, null);

        Messenger found = theRealThing.getCanonicalMessenger(plainAddr, hint);

        // Address must not be a supported one.
        if (found == null) {
            return null;
        }

        // Get a channel for that servicename and serviceparam. redirect to this group.

        // NOTE: This assumes that theRealThing.getGroup() is really the group where the application that obtained
        // this interface object lives. This is the case today because all groups have their own endpoint service.
        // In the future, interface objects may refer to a group context that is not necessarily the group where
        // "therealThing" lives. When that happens, this interface object will have to know which group it works
        // for without asking "theRealThing".

        ChannelMessenger res = (ChannelMessenger) found.getChannelMessenger(theRealThing.getGroup().getPeerGroupID(),
                addr.getServiceName(), addr.getServiceParameter());

        synchronized (channelCache) {
            // We have to check again. May be we did all that in parallel with some other thread and it beat
            // us to the finish line. In which case, substitute the existing one and throw ours away.
            Reference<Messenger> existing = channelCache.get(addr);

            if (existing != null) {
                Messenger messenger = existing.get();
                if ((messenger != null) && ((messenger.getState() & Messenger.USABLE) != 0)) {
                    return messenger;
                }
            }
            // The listenerAdaptor of this interface obj is used to support the sendMessage-with-listener API.
            res.setMessageWatcher(listenerAdaptor);
            channelCache.put(res.getDestinationAddress(), new WeakReference<Messenger>(res));
        }
        return res;
    }

    /**
     * {@inheritDoc}
     */
    public Messenger getMessenger(EndpointAddress addr) {
        return getMessenger(addr, null);
    }

    /**
     * {@inheritDoc}
     */
    public Messenger getMessenger(EndpointAddress addr, Object hint) {

        // Get an unresolved messenger (that's immediate).
        Messenger messenger = getMessengerImmediate(addr, hint);

        if (messenger == null) {
            return null;
        }

        // Now ask the messenger to resolve: this legacy blocking API ensures 
        // that only successfully resolved messengers are ever returned.
        messenger.resolve();
        try {
            messenger.waitState(Messenger.RESOLVED | Messenger.TERMINAL, TimeUtils.AMINUTE);
        } catch (InterruptedException ie) {
            Thread.interrupted();
        }

        // check the state
        int state = messenger.getState();

        if ((state & Messenger.TERMINAL) != 0) {
            return null;
        }
        if ((state & Messenger.RESOLVED) == 0) {
            // Not failed yet. But too late for us.
            return null;
        }
        return messenger;
    }

    /**
     * {@inheritDoc}
     */
    public void propagate(Message msg, String serviceName, String serviceParam) {
        theRealThing.propagate(msg, serviceName, serviceParam, Integer.MAX_VALUE);
    }

    /**
     * {@inheritDoc}
     */
    public void propagate(Message msg, String serviceName, String serviceParam, int initialTTL) {
        theRealThing.propagate(msg, serviceName, serviceParam, initialTTL);
    }

    /**
     * {@inheritDoc}
     */
    public void demux(Message msg) {
        theRealThing.demux(msg);
    }

    /**
     * {@inheritDoc}
     */
    public void processIncomingMessage(Message message, EndpointAddress source, EndpointAddress destination) {
        theRealThing.processIncomingMessage(message, source, destination);
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    public boolean ping(EndpointAddress addr) {
        return null != getMessengerImmediate(addr, null);
    }

    /**
     * {@inheritDoc}
     */
    public MessengerEventListener addMessageTransport(MessageTransport transpt) {
        // FIXME TOO: We should probably make the interface refuse to do it.
        // But that will have to wait until we have criteria to decide who
        // gets an interface object and who gets the real thing. In the
        // meantime just do it.
        return theRealThing.addMessageTransport(transpt);
    }

    /**
     * {@inheritDoc}
     */
    public boolean removeMessageTransport(MessageTransport transpt) {
        // FIXME TOO: We should probably make the interface refuse to do it.
        // But that will have to wait until we have criteria to decide who
        // gets an interface object and who gets the real thing. In the
        // meantime just do it.
        return theRealThing.removeMessageTransport(transpt);
    }

    /**
     * {@inheritDoc}
     */
    public Iterator<MessageTransport> getAllMessageTransports() {
        return theRealThing.getAllMessageTransports();
    }

    /**
     * {@inheritDoc}
     */
    public MessageTransport getMessageTransport(String name) {
        return theRealThing.getMessageTransport(name);
    }

    /**
     * {@inheritDoc}
     */
    public boolean addIncomingMessageListener(EndpointListener listener, String serviceName, String serviceParam) {
        return theRealThing.addIncomingMessageListener(listener, serviceName, serviceParam);
    }

    /**
     * {@inheritDoc}
     */
    public EndpointListener getIncomingMessageListener(String serviceName, String serviceParam) {
        return theRealThing.getIncomingMessageListener(serviceName, serviceParam);
    }

    /**
     * {@inheritDoc}
     */
    public void addIncomingMessageFilterListener(MessageFilterListener listener, String namespace, String name) {
        theRealThing.addIncomingMessageFilterListener(listener, namespace, name);
    }

    /**
     * {@inheritDoc}
     */
    public void addOutgoingMessageFilterListener(MessageFilterListener listener, String namespace, String name) {
        theRealThing.addOutgoingMessageFilterListener(listener, namespace, name);
    }

    /**
     * {@inheritDoc}
     */
    public MessageFilterListener removeIncomingMessageFilterListener(MessageFilterListener listener, String namespace, String name) {
        return theRealThing.removeIncomingMessageFilterListener(listener, namespace, name);
    }

    /**
     * {@inheritDoc}
     */
    public MessageFilterListener removeOutgoingMessageFilterListener(MessageFilterListener listener, String namespace, String name) {
        return theRealThing.removeOutgoingMessageFilterListener(listener, namespace, name);
    }

    /**
     * {@inheritDoc}
     */
    public EndpointListener removeIncomingMessageListener(String serviceName, String serviceParam) {
        return theRealThing.removeIncomingMessageListener(serviceName, serviceParam);
    }

    /**
     * {@inheritDoc}
     */
    public boolean addMessengerEventListener(MessengerEventListener listener, int prio) {
        return theRealThing.addMessengerEventListener(listener, prio);
    }

    /**
     * {@inheritDoc}
     */
    public boolean removeMessengerEventListener(MessengerEventListener listener, int prio) {
        return theRealThing.removeMessengerEventListener(listener, prio);
    }

    /**
     * {@inheritDoc}
     *
     * @deprecated legacy support
     */
    @Deprecated
    public boolean getMessenger(MessengerEventListener listener, EndpointAddress addr, Object hint) {
        Messenger messenger = getMessengerImmediate(addr, hint);
        if (messenger == null) {
            return false;
        }

        if (!listenerAdaptor.watchMessenger(listener, messenger)) {
            return false;
        }

        // Make sure that resolution is being attempted if not already in progress.
        messenger.resolve();
        return true;
    }

    /**
     * Returns a Direct Messenger that may be used to send messages via  this endpoint to the specified destination.
     *
     * @param addr the destination address.
     * @param hint the messenger hint, if any, otherwise null.
     * @param exclusive if true avoids caching the messenger
     * @return The messenger or {@code null} is returned if the destination address is not reachable.
     */
    public Messenger getDirectMessenger(EndpointAddress addr, Object hint, boolean exclusive) {
        return theRealThing.getDirectMessenger(addr, hint, exclusive);
    }
}
