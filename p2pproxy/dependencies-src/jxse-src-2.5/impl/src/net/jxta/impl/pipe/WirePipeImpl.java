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

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.id.ID;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.platform.Module;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.rendezvous.RendezVousService;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * The Wire (Propagated) Pipe Service.
 */
public class WirePipeImpl implements EndpointListener {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(WirePipeImpl.class.getName());

    /**
     * Service name we register our listener with.
     */
    final static String WIRE_SERVICE_NAME = "jxta.service.wirepipe";

    /**
     * Service param we register our listener with.
     * <p/>
     * Including the wireParam as part of the destination endpoint address
     * is needed only for backwards compatibility with legacy version of JXTA.
     * The wireParam is no longer registered as part of the endpoint listener
     * address.
     */
    private final String wireParam;

    /**
     * The Message namespace we use for passing the wire header.
     */
    final static String WIRE_HEADER_ELEMENT_NAMESPACE = "jxta";

    /**
     * The Message Element name we use for passing the wire header.
     */
    final static String WIRE_HEADER_ELEMENT_NAME = "JxtaWireHeader";

    /**
     * The wire pipes we know of.
     */
    private final Map<ID, WirePipe> wirePipes = new HashMap<ID, WirePipe>();

    private final PeerGroup group;
    private final PipeResolver pipeResolver;

    private EndpointService endpoint = null;
    private RendezVousService rendezvous = null;

    /**
     * @param group        Description of the Parameter
     * @param pipeResolver Description of the Parameter
     */
    WirePipeImpl(PeerGroup group, PipeResolver pipeResolver) {
        this.group = group;
        this.pipeResolver = pipeResolver;
        this.wireParam = group.getPeerGroupID().getUniqueValue().toString();
    }

    /**
     * To support WirePipe.send(Message, Enumeration)
     *
     * @return The serviceParameter value
     */
    public String getServiceParameter() {
        return wireParam;
    }

    /**
     * Supply arguments and starts this service if it hadn't started by itself.
     * <p/>
     * Currently this service does not expect arguments.
     *
     * @param arg A table of strings arguments.
     * @return int status indication.
     */
    public int startApp(String[] arg) {
        endpoint = group.getEndpointService();

        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }
            return Module.START_AGAIN_STALLED;
        }

        rendezvous = group.getRendezVousService();
        if (null == rendezvous) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a rendezvous service");
            }
            return Module.START_AGAIN_STALLED;
        }

        // Set our Endpoint Listener
        try {
            endpoint.addIncomingMessageListener(this, WIRE_SERVICE_NAME, null);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed registering Endpoint Listener", e);
            }
            throw new IllegalStateException("Failed registering Endpoint Listener");
        }
        return Module.START_OK;
    }

    /**
     * Ask this service to stop.
     */
    public void stopApp() {
        Collection<WirePipe> allWirePipes = new ArrayList<WirePipe>(wirePipes.values());

        for (WirePipe aWirePipe : allWirePipes) {
            // Close all of the wire pipes.
            aWirePipe.close();
        }
        wirePipes.clear();

        // Clear our listener
        endpoint.removeIncomingMessageListener(WIRE_SERVICE_NAME, null);

        endpoint = null;
        rendezvous = null;
    }

    /**
     * create an InputPipe from a pipe Advertisement
     *
     * @param adv      is the advertisement of the PipeServiceImpl.
     * @param listener PipeMsgListener to receive msgs.
     * @return InputPipe InputPipe object created
     * @throws IOException error creating input pipe
     */
    InputPipe createInputPipe(PipeAdvertisement adv, PipeMsgListener listener) throws IOException {
        WirePipe wirePipe = getWirePipe(adv);
        return new InputPipeImpl(wirePipe, adv, listener);
    }

    /**
     * create an OutputPipe from the pipe Advertisement giving a PeerId(s)
     * where the corresponding InputPipe is supposed to be.
     *
     * @param adv   is the advertisement of the NetPipe.
     * @param peers is a set of the PeerId of the peers where to look
     *              for the corresponding Pipes
     * @return OuputPipe corresponding OutputPipe
     */
    NonBlockingWireOutputPipe createOutputPipe(PipeAdvertisement adv, Set<? extends ID> peers) {
        WirePipe wirePipe = getWirePipe(adv);
        return new NonBlockingWireOutputPipe(group, wirePipe, adv, peers);
    }

    /**
     * PropagateType pipes
     *
     * @param adv the pipe adv
     * @return the wire pipe
     */
    private WirePipe getWirePipe(PipeAdvertisement adv) {
        WirePipe wirePipe;

        synchronized (wirePipes) {
            // First see if we have already a WirePipe for this pipe
            wirePipe = wirePipes.get(adv.getPipeID());

            if (null == wirePipe) {
                // No.. There is none. Create a new one.
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Creating new wire pipe for " + adv.getPipeID());
                }
                wirePipe = new WirePipe(group, pipeResolver, this, adv);
                wirePipes.put(adv.getPipeID(), wirePipe);
            }
        }
        return wirePipe;
    }

    /**
     * PropagateType pipes
     *
     * @param pipeID Pipe ID
     * @param create if true create one if one does not exist
     * @return the wire pipe
     */
    private WirePipe getWirePipe(ID pipeID, boolean create) {
        WirePipe wirePipe;

        synchronized (wirePipes) {
            // First see if we have already a WirePipe for this pipe
            wirePipe = wirePipes.get(pipeID);

            if ((null == wirePipe) && create) {
                // No.. There is none. Create a new one.
                // XXX 20031019 bondolo@jxta.org Check for the adv in local discovery maybe?
                PipeAdvertisement adv = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(
                        PipeAdvertisement.getAdvertisementType());

                adv.setPipeID(pipeID);
                adv.setType(PipeService.PropagateType);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Creating new wire pipe for " + adv.getPipeID());
                }
                wirePipe = new WirePipe(group, pipeResolver, this, adv);
                wirePipes.put(pipeID, wirePipe);
            }
        }
        return wirePipe;
    }

    /**
     * Remove a wire pipe from our collection of wire pipes.
     *
     * @param pipeID The ID of the wire pipe to forget.
     * @return {@code true} if the wire pipe had been registered otherwise
     *         {@code false}.
     */
    boolean forgetWirePipe(ID pipeID) {
        synchronized (wirePipes) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Removing wire pipe for " + pipeID);
            }
            return null != wirePipes.remove(pipeID);
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Listener for "jxta.service.wirepipe" / &lt;null&gt;
     */
    public void processIncomingMessage(Message message, EndpointAddress srcAddr, EndpointAddress dstAddr) {
        // Check if there is a JXTA-WIRE header
        MessageElement elem = message.getMessageElement(WIRE_HEADER_ELEMENT_NAMESPACE, WIRE_HEADER_ELEMENT_NAME);

        if (null == elem) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("No JxtaWireHeader element. Discarding " + message);
            }
            return;
        }

        WireHeader header;

        try {
            XMLDocument doc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(elem);
            header = new WireHeader(doc);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "bad wire header for " + message, e);
            }
            return;
        }

        WirePipe wirePipe = getWirePipe(header.getPipeID(), rendezvous.isRendezVous());
        if (null != wirePipe) {
            wirePipe.processIncomingMessage(message, header, srcAddr, dstAddr);
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Ignoring message " + message + " for id " + header.getPipeID());
            }
        }
    }
}
