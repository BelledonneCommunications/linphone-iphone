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

package net.jxta.impl.resolver;

import net.jxta.credential.Credential;
import net.jxta.document.Advertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.*;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.endpoint.router.EndpointRouter;
import net.jxta.impl.endpoint.router.RouteControl;
import net.jxta.impl.meter.MonitorManager;
import net.jxta.impl.protocol.ResolverQuery;
import net.jxta.impl.protocol.ResolverResponse;
import net.jxta.impl.protocol.ResolverSrdiMsgImpl;
import net.jxta.impl.resolver.resolverMeter.QueryHandlerMeter;
import net.jxta.impl.resolver.resolverMeter.ResolverMeter;
import net.jxta.impl.resolver.resolverMeter.ResolverMeterBuildSettings;
import net.jxta.impl.resolver.resolverMeter.ResolverServiceMonitor;
import net.jxta.impl.resolver.resolverMeter.SrdiHandlerMeter;
import net.jxta.logging.Logging;
import net.jxta.membership.MembershipService;
import net.jxta.meter.MonitorResources;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.ResolverQueryMsg;
import net.jxta.protocol.ResolverResponseMsg;
import net.jxta.protocol.ResolverSrdiMsg;
import net.jxta.protocol.RouteAdvertisement;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.rendezvous.RendezVousStatus;
import net.jxta.resolver.QueryHandler;
import net.jxta.resolver.ResolverService;
import net.jxta.resolver.SrdiHandler;

import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

/**
 * Implements the {@link net.jxta.resolver.ResolverService} using the standard
 * JXTA Endpoint Resolver Protocol (ERP).
 *
 * @see net.jxta.resolver.ResolverService
 * @see <a href="http://spec.jxta.org/v1.0/docbook/JXTAProtocols.html#proto-erp">JXTA Protocols Specification : Endpoint Resolver Protocol</a>
 */
public class ResolverServiceImpl implements ResolverService {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(ResolverServiceImpl.class.getName());

    /**
     * Resolver query endpoint postfix
     */
    public final static String outQueNameShort = "ORes";

    /**
     * Resolver response endpoint postfix
     */
    public final static String inQueNameShort = "IRes";

    /**
     * Resolver srdi endpoint postfix
     */
    public final static String srdiQueNameShort = "Srdi";

    /**
     * MIME Type for gzipped SRDI messages.
     */
    private final static MimeMediaType GZIP_MEDIA_TYPE = new MimeMediaType("application/gzip").intern();

    private String outQueName = outQueNameShort;
    private String inQueName = inQueNameShort;
    private String srdiQueName = srdiQueNameShort;

    private String handlerName = null;
    private PeerGroup group = null;
    private ModuleImplAdvertisement implAdvertisement = null;
    private EndpointService endpoint = null;
    private MembershipService membership = null;

    private RouteControl routeControl = null;

    private final Map<String, QueryHandler> handlers = Collections.synchronizedMap(new HashMap<String, QueryHandler>(5));
    private final Map<String, SrdiHandler> srdiHandlers = Collections.synchronizedMap(new HashMap<String, SrdiHandler>(5));

    private EndpointListener queryListener = null;
    private EndpointListener responseListener = null;
    private EndpointListener srdiListener = null;

    private ResolverServiceMonitor resolverServiceMonitor;
    private ResolverMeter resolverMeter;

    /**
     * the resolver interface object
     */
    private ResolverService resolverInterface = null;

    /**
     * Encapsulates current Membership Service credential.
     */
    final static class CurrentCredential {

        /**
         * The current default credential
         */
        final Credential credential;

        /**
         * The current default credential in serialized XML form.
         */
        final XMLDocument credentialDoc;

        CurrentCredential(Credential credential, XMLDocument credentialDoc) {
            this.credential = credential;
            this.credentialDoc = credentialDoc;
        }
    }

    /**
     * The current Membership service default credential.
     */
    CurrentCredential currentCredential;

    /**
     * Listener we use for membership property events.
     */
    private class CredentialListener implements PropertyChangeListener {

        /**
         * Standard Constructor
         */
        CredentialListener() {
        }

        /**
         * {@inheritDoc}
         */
        public void propertyChange(PropertyChangeEvent evt) {
            if (MembershipService.DEFAULT_CREDENTIAL_PROPERTY.equals(evt.getPropertyName())) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("New default credential event");
                }

                synchronized (ResolverServiceImpl.this) {
                    Credential cred = (Credential) evt.getNewValue();
                    XMLDocument credentialDoc;

                    if (null != cred) {
                        try {
                            credentialDoc = (XMLDocument) cred.getDocument(MimeMediaType.XMLUTF8);
                            currentCredential = new CurrentCredential(cred, credentialDoc);
                        } catch (Exception all) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Could not generate credential document", all);
                            }
                            currentCredential = null;
                        }
                    } else {
                        currentCredential = null;
                    }
                }
            }
        }
    }

    final CredentialListener membershipCredListener = new CredentialListener();

    /**
     * Convenience method for constructing an endpoint address from an id
     *
     * @param destPeer peer id
     * @param serv     the service name (if any)
     * @param parm     the service param (if any)
     * @return endpointAddress for this peer id.
     */
    private static EndpointAddress mkAddress(ID destPeer, String serv, String parm) {
        return new EndpointAddress("jxta", destPeer.getUniqueValue().toString(), serv, parm);
    }

    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement impl) {
        implAdvertisement = (ModuleImplAdvertisement) impl;

        this.group = group;
        handlerName = assignedID.toString();
        String uniqueStr = group.getPeerGroupID().getUniqueValue().toString();

        outQueName = uniqueStr + outQueNameShort;
        inQueName = uniqueStr + inQueNameShort;
        srdiQueName = uniqueStr + srdiQueNameShort;

        if (ResolverMeterBuildSettings.RESOLVER_METERING) { // Fix-Me: This needs to be moved to startApp() when the load order issue is resolved
            resolverServiceMonitor = (ResolverServiceMonitor) MonitorManager.getServiceMonitor(group,
                    MonitorResources.resolverServiceMonitorClassID);
            if (resolverServiceMonitor != null) {
                resolverMeter = resolverServiceMonitor.getResolverMeter();
            }
        }

        // Tell tell the world about our configuration.
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring Resolver Service : " + assignedID);

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

            configInfo.append("\n\tConfiguration:");
            configInfo.append("\n\t\tIn Queue name: ").append(outQueName);
            configInfo.append("\n\t\tOut Queue name: ").append(inQueName);
            configInfo.append("\n\t\tSRDI Queue name: ").append(srdiQueName);

            LOG.config(configInfo.toString());
        }
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
            return Module.START_AGAIN_STALLED;
        }

        membership = group.getMembershipService();

        if (null == membership) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a membership service");
            }
            return Module.START_AGAIN_STALLED;
        }

        // Register Listeners
        try {
            // Register Query Listener
            queryListener = new DemuxQuery();
            if (!endpoint.addIncomingMessageListener(queryListener, handlerName, outQueName)) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe("Cannot register listener (already registered)");
                }
            }

            // Register Response Listener
            responseListener = new DemuxResponse();
            if (!endpoint.addIncomingMessageListener(responseListener, handlerName, inQueName)) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe("Cannot register listener (already registered)");
                }
            }

            // Register SRDI Listener
            srdiListener = new DemuxSrdi();
            if (!endpoint.addIncomingMessageListener(srdiListener, handlerName, srdiQueName)) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.severe("Cannot register listener (already registered)");
                }
            }
        } catch (Exception e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "failed to add listeners", e);
            }
            return -1;
        }

        synchronized (this) {
            // register our credential listener.
            membership.addPropertyChangeListener(MembershipService.DEFAULT_CREDENTIAL_PROPERTY, membershipCredListener);
            try {
                // set the initial version of the default credential.
                currentCredential = null;
                Credential credential = membership.getDefaultCredential();
                XMLDocument credentialDoc;
                if (null != credential) {
                    credentialDoc = (XMLDocument) credential.getDocument(MimeMediaType.XMLUTF8);
                    currentCredential = new CurrentCredential(credential, credentialDoc);
                }
            } catch (Exception all) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "could not get default credential", all);
                }
            }
        }
        return Module.START_OK;
    }

    /**
     * {@inheritDoc}
     */
    public void stopApp() {
        endpoint.removeIncomingMessageListener(handlerName, outQueName);
        endpoint.removeIncomingMessageListener(handlerName, inQueName);

        if (null != srdiListener) {
            endpoint.removeIncomingMessageListener(handlerName, srdiQueName);
        }

        queryListener = null;
        responseListener = null;
        srdiListener = null;

        membership.removePropertyChangeListener("defaultCredential", membershipCredListener);
        currentCredential = null;

        routeControl = null;
        membership = null;
        group = null;
    }

    /**
     * {@inheritDoc}
     */
    public synchronized ResolverService getInterface() {
        if (resolverInterface == null) {
            resolverInterface = new ResolverServiceInterface(this);
        }
        return resolverInterface;
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
    public QueryHandler registerHandler(String name, QueryHandler handler) {
        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            resolverServiceMonitor.registerQueryHandlerMeter(name);
        }
        return handlers.put(name, handler);
    }

    /**
     * {@inheritDoc}
     */
    public QueryHandler unregisterHandler(String name) {
        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            resolverServiceMonitor.unregisterQueryHandlerMeter(name);
        }
        return handlers.remove(name);
    }

    /**
     * given a name returns the query handler associated with it
     *
     * @param name the handler to lookup
     * @return returns the query handler
     */
    public QueryHandler getHandler(String name) {
        return handlers.get(name);
    }

    /**
     * {@inheritDoc}
     */
    public SrdiHandler registerSrdiHandler(String name, SrdiHandler handler) {
        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            resolverServiceMonitor.registerSrdiHandlerMeter(name);
        }
        return srdiHandlers.put(name, handler);
    }

    /**
     * {@inheritDoc}
     */
    public SrdiHandler unregisterSrdiHandler(String name) {
        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            resolverServiceMonitor.unregisterSrdiHandlerMeter(name);
        }
        return srdiHandlers.remove(name);
    }

    /**
     * given a name returns the srdi handler associated with it
     *
     * @param name the handler to lookup
     * @return returns the SRDI handler
     */
    public SrdiHandler getSrdiHandler(String name) {
        return srdiHandlers.get(name);
    }

    /**
     * {@inheritDoc}
     */
    public void sendQuery(String destPeer, ResolverQueryMsg query) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("sending query to resolver handler: " + query.getHandlerName());
        }

        // NOTE: Add route information about the issuing peer, so the
        // resolver query responding peer can respond to the issuer without
        // requiring any route discovery. In most case the responding peer
        // is unlikely to know the route to the query issuer. This is a good
        // optimization for edge peers. This optimzation is much less
        // important for RDV peers as they are more likely to have a route
        // to peers. Also, there is the concern that adding route info
        // in resolver query exchanged between RDV will increase overhead due
        // to the larger amount of information exchanged between RDV.
        // Only update query if the query does not already contain any route
        // information. We are mostly interested in the original src
        // route information.
        if (query.getSrcPeerRoute() == null) {
            if (getRouteControl() != null) {
                // FIXME tra 20031102 Until the new subscription service
                // is implemented, we use the Router Control IOCTL
                RouteAdvertisement route = routeControl.getMyLocalRoute();

                if (route != null) {
                    query.setSrcPeerRoute(route.clone());
                }

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Sending query with route info to " + destPeer);
                }
            } else {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("No route control--could not set local route on query");
                }
            }
        }

        String queryHandlerName = query.getHandlerName();
        QueryHandlerMeter queryHandlerMeter = null;

        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(queryHandlerName);
        }

        if (destPeer == null) {
            try {
                Message queryMsg = new Message();
                XMLDocument asDoc = (XMLDocument) query.getDocument(MimeMediaType.XMLUTF8);
                MessageElement docElem = new TextDocumentMessageElement(outQueName, asDoc, null);
                queryMsg.addMessageElement("jxta", docElem);
                RendezVousService rendezvous = group.getRendezVousService();

                if (null != rendezvous) {
                    if (rendezvous.getRendezVousStatus() != RendezVousStatus.ADHOC) {
                        // Walk the message
                        rendezvous.walk(queryMsg.clone(), handlerName, outQueName, RendezVousService.DEFAULT_TTL);
                    }
                    // propagate to local net as well
                    rendezvous.propagateToNeighbors(queryMsg, handlerName, outQueName, 2);
                } else {
                    endpoint.propagate(queryMsg, handlerName, outQueName);
                }

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                    queryHandlerMeter.querySentInGroup(query);
                }
            } catch (IOException e) {
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                    queryHandlerMeter.queryPropagateError();
                }
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure during propagate", e);
                }
            }
        } else {
            // unicast instead
            boolean success = sendMessage(destPeer, null, handlerName, outQueName, outQueName,
                    (XMLDocument) query.getDocument(MimeMediaType.XMLUTF8), false);

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                if (success) {
                    queryHandlerMeter.querySentViaUnicast(destPeer, query);
                } else {
                    queryHandlerMeter.querySendError();
                }
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void sendResponse(String destPeer, ResolverResponseMsg response) {

        if (destPeer == null) {
            propagateResponse(response);
        } else {
            QueryHandlerMeter queryHandlerMeter = null;
            try {
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
                    queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(response.getHandlerName());
                }

                // Check if an optional route information is available to send the response
                RouteAdvertisement route = response.getSrcPeerRoute();
                boolean success = sendMessage(destPeer, route, handlerName, inQueName, inQueName,
                        (XMLDocument) response.getDocument(MimeMediaType.XMLUTF8), false);

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                    if (success) {
                        queryHandlerMeter.responseSentViaUnicast(destPeer, response);
                    } else {
                        queryHandlerMeter.responseSendError();
                    }
                }
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Error in sending response", e);
                }

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                    queryHandlerMeter.responseSendError();
                }
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void sendSrdi(String destPeer, ResolverSrdiMsg srdi) {
        String srdiHandlerName = srdi.getHandlerName();
        SrdiHandlerMeter srdiHandlerMeter = null;

        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            srdiHandlerMeter = resolverServiceMonitor.getSrdiHandlerMeter(srdiHandlerName);
        }

        if (destPeer == null) {
            RendezVousService rendezvous = group.getRendezVousService();

            if (rendezvous == null) {
                // no rendezvous service, dump it.
                return;
            }
            Message propagateMsg = new Message();

            try {
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                GZIPOutputStream gos = new GZIPOutputStream(baos);

                srdi.getDocument(MimeMediaType.XMLUTF8).sendToStream(gos);
                gos.finish();
                gos.close();
                byte gzipBytes[] = baos.toByteArray();

                MessageElement zipElem = new ByteArrayMessageElement(srdiQueName, GZIP_MEDIA_TYPE, gzipBytes, null);

                propagateMsg.addMessageElement("jxta", zipElem);

                if (rendezvous.getRendezVousStatus() != RendezVousStatus.ADHOC) {
                    rendezvous.walk(propagateMsg, handlerName, srdiQueName, RendezVousService.DEFAULT_TTL);
                }

                // propagate to local net as well
                rendezvous.propagateToNeighbors(propagateMsg, handlerName, srdiQueName, 2);

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (srdiHandlerMeter != null)) {
                    srdiHandlerMeter.messageSentViaWalker(srdi);
                }
            } catch (IOException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure sending srdi message", e);
                }

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (srdiHandlerMeter != null)) {
                    srdiHandlerMeter.errorPropagatingMessage();
                }
            }
        } else {
            try {
                boolean success = sendMessage(destPeer, null, handlerName, srdiQueName, srdiQueName,
                        (XMLDocument) srdi.getDocument(MimeMediaType.XMLUTF8),
                        // compression
                        true);

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (srdiHandlerMeter != null)) {
                    if (success) {
                        srdiHandlerMeter.messageSentViaUnicast(destPeer, srdi);
                    } else {
                        srdiHandlerMeter.errorSendingMessage();
                    }
                }
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Error in sending srdi message", e);
                }
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (srdiHandlerMeter != null)) {
                    srdiHandlerMeter.errorSendingMessage();
                }
            }
        }
    }

    private void repropagateQuery(Message msg, ResolverQueryMsg query) {
        RendezVousService rendezvous = group.getRendezVousService();

        if ((null != rendezvous) && !group.isRendezvous()) {
            // not a Rendezvous peer? Let someone else forward it.
            return;
        }

        // just in case an excessive of attempt to forward a query
        // hopCount is used to determine forward counts independent of any other TTL
        if (query.getHopCount() > 3) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("discarding ResolverQuery. HopCount exceeded : " + query.getHopCount());
            }

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                resolverMeter.propagationQueryDropped(query);
            }
            return;
        }

        XMLDocument asDoc = (XMLDocument) query.getDocument(MimeMediaType.XMLUTF8);
        MessageElement docElem = new TextDocumentMessageElement(outQueName, asDoc, null);

        msg.replaceMessageElement("jxta", docElem);

        // Re-propagate the message.
        // Loop and TTL control is done in demux and propagate(). The TTL
        // below is just a default it will be reduced appropriately.

        try {
            if (null != rendezvous) {
                if (rendezvous.getRendezVousStatus() != RendezVousStatus.ADHOC) {
                    rendezvous.walk(msg, handlerName, outQueName, RendezVousService.DEFAULT_TTL);
                }
                // propagate to local net as well
                rendezvous.propagateToNeighbors(msg, handlerName, outQueName, 2);
            } else {
                endpoint.propagate(msg, handlerName, inQueName);
            }

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                resolverMeter.queryPropagatedViaWalker(query);
            }
        } catch (IOException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure propagating query", e);

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.queryPropagationError(query);
                }
            }
        }
    }

    /**
     * Process a resolver query.
     *
     * @param query   The query.
     * @param srcAddr Who sent the query to us. May not be the same as the
     *                query originator.
     * @return the query id assigned to the query
     */
    private int processQuery(ResolverQueryMsg query, EndpointAddress srcAddr) {
        String queryHandlerName = query.getHandlerName();
        QueryHandler theHandler = getHandler(queryHandlerName);

        if (query.getHopCount() > 2) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Discarding query #" + query.getQueryId() + " hopCount > 2 : " + query.getHopCount());
            }

            // query has been forwarded too many times stop
            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
                QueryHandlerMeter queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(queryHandlerName);
                if (queryHandlerMeter != null) {
                    queryHandlerMeter.queryHopCountDropped();
                } else {
                    resolverMeter.invalidQueryDiscarded(srcAddr);
                }
            }
            return ResolverService.OK;
        }

        if (theHandler == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Discarding query #" + query.getQueryId() + ", no handler for :" + queryHandlerName);
            }
            // If this peer is a rendezvous peer, it needs to repropagate the query to other rendezvous peer that
            // may have a handler.
            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                resolverMeter.unknownHandlerForQuery(query);
            }
            return ResolverService.Repropagate;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Handing query #" + query.getQueryId() + " to : " + queryHandlerName);
        }

        QueryHandlerMeter queryHandlerMeter = null;
        long startTime = 0;
        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            startTime = System.currentTimeMillis();
            queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(queryHandlerName);
        }

        try {
            int result;
            if (theHandler instanceof InternalQueryHandler) {
                result = ((InternalQueryHandler) theHandler).processQuery(query, srcAddr);
            } else {
                result = theHandler.processQuery(query);
            }

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                queryHandlerMeter.queryProcessed(query, result, System.currentTimeMillis() - startTime);
            }
            return result;
        } catch (Throwable any) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Uncaught Throwable from handler for : " + queryHandlerName, any);
            }
            if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                queryHandlerMeter.errorWhileProcessingQuery(query);
            }
            // stop repropagation
            return ResolverService.OK;
        }
    }

    /**
     * Process a resolver response.
     *
     * @param resp    The response.
     * @param srcAddr Who sent the response. May not be the same as the
     *                originator response.
     */
    private void processResponse(ResolverResponseMsg resp, EndpointAddress srcAddr) {
        String handlerName = resp.getHandlerName();

        if (handlerName == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Missing handlername in response");
            }

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                resolverMeter.invalidResponseDiscarded(srcAddr);
            }
            return;
        }

        QueryHandler theHandler = getHandler(handlerName);
        if (theHandler == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("No handler for :" + handlerName);
            }
            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                resolverMeter.unknownHandlerForResponse(srcAddr, resp);
            }
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Process response to query #" + resp.getQueryId() + " with " + handlerName);
        }

        QueryHandlerMeter queryHandlerMeter = null;
        long startTime = 0;

        if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
            startTime = System.currentTimeMillis();
            queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(handlerName);
        }

        try {
            if (theHandler instanceof InternalQueryHandler) {
                ((InternalQueryHandler) theHandler).processResponse(resp, srcAddr);
            } else {
                theHandler.processResponse(resp);
            }

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                queryHandlerMeter.responseProcessed(resp, System.currentTimeMillis() - startTime, srcAddr);
            }
        } catch (Throwable all) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Uncaught Throwable from handler for: " + handlerName, all);
            }

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                queryHandlerMeter.errorWhileProcessingResponse(srcAddr);
            }
        }
    }

    /**
     * propagate a response
     *
     * @param response response message to propagate
     */
    private void propagateResponse(ResolverResponseMsg response) {

        Message propagateMsg = new Message();

        String queryHandlerName = response.getHandlerName();
        QueryHandlerMeter queryHandlerMeter = null;

        try {
            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
                queryHandlerMeter = resolverServiceMonitor.getQueryHandlerMeter(queryHandlerName);
            }

            XMLDocument responseDoc = (XMLDocument) response.getDocument(MimeMediaType.XMLUTF8);
            MessageElement elemDoc = new TextDocumentMessageElement(inQueName, responseDoc, null);
            propagateMsg.addMessageElement("jxta", elemDoc);
            RendezVousService rendezvous = group.getRendezVousService();

            if (null != rendezvous) {
                rendezvous.walk(propagateMsg, handlerName, inQueName, RendezVousService.DEFAULT_TTL);
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                    queryHandlerMeter.responseSentViaWalker(response);
                }
            } else {
                endpoint.propagate(propagateMsg, handlerName, inQueName);
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                    // FIXME bondolo 20040909 not technically the correct metric
                    queryHandlerMeter.responseSentViaWalker(response);
                }
            }
        } catch (IOException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failure during propagateResponse", e);
            }
            if (ResolverMeterBuildSettings.RESOLVER_METERING && (queryHandlerMeter != null)) {
                queryHandlerMeter.responsePropagateError();
            }
        }
    }

    /**
     * Process an SRDI message.
     *
     * @param srdimsg The SRDI message.
     * @param srcAddr Who sent the message. May not be the same as the
     *                originator of the message.
     */
    private void processSrdi(ResolverSrdiMsgImpl srdimsg, EndpointAddress srcAddr) {
        String handlerName = srdimsg.getHandlerName();

        if (handlerName == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Missing handlername in response");
            }

            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                resolverMeter.invalidSrdiMessageDiscarded(srcAddr);
            }
            return;
        }

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Processing an SRDI msg for : " + handlerName + " in Group ID:" + group.getPeerGroupID());
        }

        SrdiHandler theHandler = getSrdiHandler(handlerName);
        if (theHandler != null) {
            SrdiHandlerMeter srdiHandlerMeter = null;

            try {
                long startTime = 0;
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverServiceMonitor != null)) {
                    startTime = System.currentTimeMillis();
                    srdiHandlerMeter = resolverServiceMonitor.getSrdiHandlerMeter(handlerName);
                }

                theHandler.processSrdi(srdimsg);
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (srdiHandlerMeter != null)) {
                    srdiHandlerMeter.messageProcessed(srdimsg, System.currentTimeMillis() - startTime, srcAddr);
                }
            } catch (Throwable all) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Uncaught Throwable from handler for: " + handlerName, all);
                }

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (srdiHandlerMeter != null)) {
                    srdiHandlerMeter.errorWhileProcessing(srcAddr);
                }
            }
        } else {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING) && group.isRendezvous()) {
                LOG.warning("No srdi handler registered :" + handlerName + " for Group ID:" + group.getPeerGroupID());
            } else if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No srdi handler registered :" + handlerName + " for Group ID:" + group.getPeerGroupID());
            }
            if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                resolverMeter.unknownHandlerForSrdiMessage(srcAddr, handlerName);
            }
        }
    }

    /**
     * Send a resolver message to a peer
     *
     * @param destPeer destination peer
     * @param route    destination route advertisement
     * @param pName    service name on the destination
     * @param pParam   service param on the destination
     * @param tagName  tag name of the message element
     * @param body     the body of the message element
     * @param gzip     If <code>true</code> then encode the message body using gzip.
     * @return {@code true} if successful
     */
    private boolean sendMessage(String destPeer, RouteAdvertisement route, String pName, String pParam, String tagName, XMLDocument body, boolean gzip) {
        // Get the messenger ready
        ID dest;
        try {
            dest = IDFactory.fromURI(new URI(destPeer));
        } catch (URISyntaxException badpeer) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "bad destination peerid : " + destPeer, badpeer);
            }
            
            return false;
        }

        EndpointAddress destAddress = mkAddress(dest, pName, pParam);

        // FIXME add route to responses as well
        Messenger messenger = null;
        if (route == null) {
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("No route info available for " + destPeer);
            }
        } else {
            // ok we have a route let's pass it to the router
            if ((null == getRouteControl()) || (routeControl.addRoute(route) == RouteControl.FAILED)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Failed to add route for " + route.getDestPeerID());
                }
            } else {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("Added route for " + route.getDestPeerID());
                }
            }
        }
        
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Creating a messenger immediate for :" + destAddress);
        }

        messenger = endpoint.getMessengerImmediate(destAddress, route);
        if (null == messenger) {        
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Failed creating messenger for " + destAddress);
            }
            return false;
        }

        Message msg = new Message();
        try {
            MessageElement msgEl;
            if (gzip) {
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                GZIPOutputStream gos = new GZIPOutputStream(baos);

                body.sendToStream(gos);
                gos.finish();
                gos.close();
                byte gzipBytes[] = baos.toByteArray();

                msgEl = new ByteArrayMessageElement(tagName, GZIP_MEDIA_TYPE, gzipBytes, null);
            } else {
                msgEl = new TextDocumentMessageElement(tagName, body, null);
            }
            msg.addMessageElement("jxta", msgEl);
        } catch (Exception ez1) {
            // Not much we can do
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed building message", ez1);
            }
            return false;
        }

        // Send the message
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Sending " + msg + " to " + destAddress + " " + tagName);
        }

        // XXX 20040924 bondolo Convert this to ListenerAdaptor
        messenger.sendMessage(msg, null, null, new FailureListener(dest));
        
        return true;
    }

    private RouteControl getRouteControl() {
        // Obtain the route control object to manipulate route information when sending and receiving resolver queries.
        if (routeControl == null) {
            // insignificant race condition here
            MessageTransport endpointRouter = endpoint.getMessageTransport("jxta");
            if (endpointRouter != null) {
                routeControl = (RouteControl) endpointRouter.transportControl(EndpointRouter.GET_ROUTE_CONTROL, null);
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Failed to get RouteControl object. Resolver will not set route hints.");
                }
            }
        }
        return routeControl;
    }

    /**
     * Inner class to handle incoming queries
     */
    private class DemuxQuery implements EndpointListener {

        /**
         * {@inheritDoc}
         */
        public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Demuxing a query message from " + srcAddr);
            }

            MessageElement element = message.getMessageElement("jxta", outQueName);
            if (element == null) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Message does not contain a query. Discarding message");
                }

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidQueryDiscarded(srcAddr);
                }
                return;
            }

            ResolverQueryMsg query;
            try {
                StructuredDocument asDoc = StructuredDocumentFactory.newStructuredDocument(element);
                query = new ResolverQuery(asDoc);
            } catch (IOException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Ill formatted resolver query, ignoring.", e);
                }
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidQueryDiscarded(srcAddr);
                }
                return;
            } catch (IllegalArgumentException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Ill formatted resolver query, ignoring.", e);
                }
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidQueryDiscarded(srcAddr);
                }
                return;
            }

            int res = processQuery(query, srcAddr);
            if (ResolverService.Repropagate == res) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Repropagating query " + message + " from " + srcAddr);
                }
                repropagateQuery(message, query);
            }
        }
    }

    /**
     * Inner class to handle incoming responses
     */
    private class DemuxResponse implements EndpointListener {

        /**
         * @inheritDoc
         */
        public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Demuxing a response from " + srcAddr);
            }

            MessageElement element = message.getMessageElement("jxta", inQueName);
            if (null == element) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Message does not contain a response. Discarding message");
                }

                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidResponseDiscarded(srcAddr);
                }
                return;
            }

            ResolverResponse resolverResponse;
            try {
                StructuredDocument asDoc = StructuredDocumentFactory.newStructuredDocument(element);
                resolverResponse = new ResolverResponse(asDoc);
            } catch (IOException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Ill formatted resolver response, ignoring.", e);
                }
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidResponseDiscarded(srcAddr);
                }
                return;
            } catch (IllegalArgumentException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Ill formatted resolver response, ignoring.", e);
                }
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidResponseDiscarded(srcAddr);
                }
                return;
            }
            processResponse(resolverResponse, srcAddr);
        }
    }

    /**
     * Inner class to handle SRDI messages
     */
    private class DemuxSrdi implements EndpointListener {

        /**
         * @inheritDoc
         */
        public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Demuxing an SRDI message from : " + srcAddr);
            }

            MessageElement element = message.getMessageElement("jxta", srdiQueName);
            if (element == null) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Message does not contain a SRDI element. Discarding message");
                }
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidSrdiMessageDiscarded(srcAddr);
                }
                return;
            }

            ResolverSrdiMsgImpl srdimsg;
            try {
                if (element.getMimeType().getBaseMimeMediaType().equals(GZIP_MEDIA_TYPE)) {
                    InputStream gzipStream = new GZIPInputStream(element.getStream());
                    StructuredDocument asDoc = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, gzipStream);
                    srdimsg = new ResolverSrdiMsgImpl(asDoc, membership);
                } else {
                    StructuredDocument asDoc = StructuredDocumentFactory.newStructuredDocument(element);
                    srdimsg = new ResolverSrdiMsgImpl(asDoc, membership);
                }
            } catch (IOException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Ill formatted SRDI message, ignoring.", e);
                }
                if (ResolverMeterBuildSettings.RESOLVER_METERING && (resolverMeter != null)) {
                    resolverMeter.invalidSrdiMessageDiscarded(srcAddr);
                }
                return;
            }
            processSrdi(srdimsg, srcAddr);
        }
    }

    /**
     * Listener to find bad destinations and clean srdi tables for them.
     */
    class FailureListener implements OutgoingMessageEventListener {
        final ID dest;

        FailureListener(ID dest) {
            this.dest = dest;
        }

        /**
         * {@inheritDoc}
         */
        public void messageSendFailed(OutgoingMessageEvent event) {
            // Ignore the failure if it's a case of queue overflow.
            if (event.getFailure() == null) {
                return;
            }
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Clearing SRDI tables for failed peer : " + dest);
            }

            for (Object o : Arrays.asList(srdiHandlers.values().toArray())) {
                SrdiHandler theHandler = (SrdiHandler) o;
                try {
                    theHandler.messageSendFailed((PeerID) dest, event);
                } catch (Throwable all) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Uncaught Throwable from handler : " + theHandler, all);
                    }
                }
            }
        }

        /**
         * {@inheritDoc}
         */
        public void messageSendSucceeded(OutgoingMessageEvent event) {// great!
        }
    }
}
