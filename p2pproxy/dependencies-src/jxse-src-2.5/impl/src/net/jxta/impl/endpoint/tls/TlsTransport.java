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

package net.jxta.impl.endpoint.tls;


import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.net.URI;
import java.security.cert.X509Certificate;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.ResourceBundle;
import javax.security.auth.x500.X500Principal;

import java.io.IOException;
import java.net.URISyntaxException;
import java.security.InvalidKeyException;
import java.security.KeyStoreException;
import java.security.SignatureException;
import java.util.MissingResourceException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageReceiver;
import net.jxta.endpoint.MessageSender;
import net.jxta.endpoint.Messenger;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.Module;
import net.jxta.protocol.ModuleImplAdvertisement;

import net.jxta.exception.PeerGroupException;

import net.jxta.impl.endpoint.LoopbackMessenger;
import net.jxta.impl.membership.pse.PSECredential;
import net.jxta.impl.membership.pse.PSEMembershipService;
import net.jxta.impl.peergroup.GenericPeerGroup;
import net.jxta.impl.util.TimeUtils;


/**
 *  A JXTA {@link net.jxta.endpoint.MessageTransport} implementation which
 *  uses TLS sockets.
 */
public class TlsTransport implements Module, MessageSender, MessageReceiver {
    
    /**
     *   Logger
     */
    private final static transient Logger LOG = Logger.getLogger(TlsTransport.class.getName());
    
    /**
     *  If true then we can accept incoming connections. Eventually this should
     *  be coming out of the transport advertisement.
     */
    static final boolean ACT_AS_SERVER = true;
    
    private PeerGroup group = null;
    ID assignedID = null;
    ModuleImplAdvertisement implAdvertisement = null;
    
    EndpointService endpoint = null;
    PSEMembershipService membership = null;
    private membershipPCL membershipListener = null;
    
    X509Certificate[] serviceCert = null;
    
    PSECredential credential = null;
    private credentialPCL credentialListener = null;
    
    EndpointAddress localPeerAddr = null;
    EndpointAddress localTlsPeerAddr = null;
    
    /**
     * local peerID
     */
    PeerID localPeerId = null;
    
    /**
     *  Amount of a connection must be idle before a reconnection attempt will
     *  be considered.
     */
    long MIN_IDLE_RECONNECT = 1 * TimeUtils.AMINUTE;
    
    /**
     *  Amount of time after which a connection is considered idle and may be
     *  scavenged.
     */
    long CONNECTION_IDLE_TIMEOUT = 5 * TimeUtils.AMINUTE;
    
    /**
     *  Amount if time which retries may remain queued for retransmission. If
     *  still unACKed after this amount of time then the connection is
     *  considered dead.
     */
    long RETRMAXAGE = 2 * TimeUtils.AMINUTE;
    
    /**
     *  Will manage connections to remote peers.
     */
    private TlsManager manager = null;
    
    /**
     *  This is the thread group into which we will place all of the threads
     *  we create. THIS HAS NO EFFECT ON SCHEDULING. Java thread groups are
     *  only for organization and naming.
     */
    ThreadGroup myThreadGroup = null;
    
    /**
     *  Extends LoopbackMessenger to add a message property to passed messages
     *  so that TLS pipes and other users can be sure that the message
     *  originate with the local TLS transport.
     */
    class TlsLoopbackMessenger extends LoopbackMessenger {
        TlsLoopbackMessenger(EndpointService ep, EndpointAddress src, EndpointAddress dest, EndpointAddress logicalDest) {
            super(group, ep, src, dest, logicalDest);
        }
        
        /**
         *  {@inheritDoc}
         **/
        @Override
        public void sendMessageBImpl(Message message, String service, String serviceParam) throws IOException {
            
            // add a property to the message to indicate it came from us.
            message.setMessageProperty(TlsTransport.class, TlsTransport.this);
            
            super.sendMessageBImpl(message, service, serviceParam);
        }
    }
    
    /**
     *  Default constructor
     **/
    public TlsTransport() {
        
        // initialize connection timeout
        try {
            ResourceBundle jxtaRsrcs = ResourceBundle.getBundle("net.jxta.user");
            
            try {
                String override_str = jxtaRsrcs.getString("impl.endpoint.tls.connection.idletimeout");
                
                if (null != override_str) {
                    long override_long = Long.parseLong(override_str.trim());
                    
                    if (override_long >= 1) {
                        CONNECTION_IDLE_TIMEOUT = override_long * TimeUtils.AMINUTE;
                        
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("Adjusting TLS connection idle timeout to " + CONNECTION_IDLE_TIMEOUT + " millis.");
                        }
                    }
                }
            } catch (NumberFormatException badvalue) {
                ;
            }
            
            try {
                String override_str = jxtaRsrcs.getString("impl.endpoint.tls.connection.minidlereconnect");
                
                if (null != override_str) {
                    long override_long = Long.parseLong(override_str.trim());
                    
                    if (override_long >= 1) {
                        MIN_IDLE_RECONNECT = override_long * TimeUtils.AMINUTE;
                        
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("Adjusting TLS min reconnection idle to " + MIN_IDLE_RECONNECT + " millis.");
                        }
                    }
                }
            } catch (NumberFormatException badvalue) {
                ;
            }
            
            try {
                String override_str = jxtaRsrcs.getString("impl.endpoint.tls.connection.maxretryage");
                
                if (null != override_str) {
                    long override_long = Long.parseLong(override_str.trim());
                    
                    if (override_long >= 1) {
                        RETRMAXAGE = override_long * TimeUtils.AMINUTE;
                        
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("Adjusting TLS maximum retry queue age to " + RETRMAXAGE + " millis.");
                        }
                    }
                }
            } catch (NumberFormatException badvalue) {
                ;
            }
            
            // reconnect must be less the idle interval.
            
            MIN_IDLE_RECONNECT = Math.min(MIN_IDLE_RECONNECT, CONNECTION_IDLE_TIMEOUT);
            
            // max retry queue age must be less the idle interval.
            RETRMAXAGE = Math.min(RETRMAXAGE, CONNECTION_IDLE_TIMEOUT);
            
        } catch (MissingResourceException notthere) {
            ;
        }
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        if (null == target) {
            return false;
        }
        
        if (target instanceof TlsTransport) {
            TlsTransport likeMe = (TlsTransport) target;
            
            if (!getProtocolName().equals(likeMe.getProtocolName())) {
                return false;
            }
            
            return localTlsPeerAddr.equals(likeMe.localTlsPeerAddr);
        }
        
        return false;
    }
    
    /**
     * {@inheritDoc}
     */
    PeerGroup getPeerGroup() {
        return group;
    }
    
    /**
     * {@inheritDoc}
     */
    public void init(PeerGroup group, ID assignedID, Advertisement impl) throws PeerGroupException {
        
        this.group = group;
        this.assignedID = assignedID;
        this.implAdvertisement = (ModuleImplAdvertisement) impl;
        
        localPeerId = group.getPeerID();
        
        localPeerAddr = mkAddress(group.getPeerID(), null, null);
        
        localTlsPeerAddr = new EndpointAddress(JTlsDefs.tlsPName, localPeerId.getUniqueValue().toString(), null, null);
        
        myThreadGroup = new ThreadGroup(group.getHomeThreadGroup(), "TLSTransport " + localTlsPeerAddr);
        
        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring TLS Transport : " + assignedID);

            if (null != implAdvertisement) {
                configInfo.append("\n\tImplementation:");
                configInfo.append("\n\t\tModule Spec ID: " + implAdvertisement.getModuleSpecID());
                configInfo.append("\n\t\tImpl Description : " + implAdvertisement.getDescription());
                configInfo.append("\n\t\tImpl URI : " + implAdvertisement.getUri());
                configInfo.append("\n\t\tImpl Code : " + implAdvertisement.getCode());
            }
            configInfo.append("\n\tGroup Params:");
            configInfo.append("\n\t\tGroup: " + group.getPeerGroupName());
            configInfo.append("\n\t\tGroup ID: " + group.getPeerGroupID());
            configInfo.append("\n\t\tPeer ID: " + group.getPeerID());
            configInfo.append("\n\tConfiguration :");
            configInfo.append("\n\t\tProtocol: " + JTlsDefs.tlsPName);
            configInfo.append("\n\t\tOutgoing Connections Enabled: " + Boolean.TRUE);
            configInfo.append("\n\t\tIncoming Connections Enabled: " + ACT_AS_SERVER);
            configInfo.append("\n\t\tMinimum idle for reconnect : " + MIN_IDLE_RECONNECT + "ms");
            configInfo.append("\n\t\tConnection idle timeout : " + CONNECTION_IDLE_TIMEOUT + "ms");
            configInfo.append("\n\t\tRetry queue maximum age : " + RETRMAXAGE + "ms");
            configInfo.append("\n\t\tPeerID : " + localPeerId);
            configInfo.append("\n\t\tRoute through : " + localPeerAddr);
            configInfo.append("\n\t\tPublic Address : " + localTlsPeerAddr);
            
            LOG.config(configInfo.toString());
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public synchronized int startApp(String[] args) {
        
        endpoint = group.getEndpointService();
        
        if (null == endpoint) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is an endpoint service");
            }
            
            return START_AGAIN_STALLED;
        }
        
        MembershipService groupMembership = group.getMembershipService();
        
        if (null == groupMembership) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Stalled until there is a membership service");
            }
            
            return START_AGAIN_STALLED;
        }
        
        if (!(groupMembership instanceof PSEMembershipService)) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("TLS Transport requires PSE Membership Service");
            }
            return -1;
        }
        
        if (endpoint.addMessageTransport(this) == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Transport registration refused");
            }
            return -1;
        }
        
        membership = (PSEMembershipService) groupMembership;
        
        PropertyChangeListener mpcl = new membershipPCL();

        membership.addPropertyChangeListener(mpcl);
        
        try {
            serviceCert = membership.getPSEConfig().getTrustedCertificateChain(assignedID);
            
            Enumeration eachCred = membership.getCurrentCredentials();
            
            while (eachCred.hasMoreElements()) {
                PSECredential aCred = (PSECredential) eachCred.nextElement();
                
                // send a fake property change event.
                mpcl.propertyChange(new PropertyChangeEvent(membership, "addCredential", null, aCred));
            }
        } catch (IOException failed) {
            serviceCert = null;
        } catch (KeyStoreException failed) {
            serviceCert = null;
        }
        
        // Create the TLS Manager
        manager = new TlsManager(this);
        
        // Connect ourself to the EndpointService
        try {
            endpoint.addIncomingMessageListener(manager, JTlsDefs.ServiceName, null);
        } catch (Throwable e2) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "TLS could not register listener...as good as dead", e2);
            }
            return -1;
        }
        
        return 0;
    }
    
    /**
     * {@inheritDoc}
     */
    public synchronized void stopApp() {
        if (null != endpoint) {
            endpoint.removeIncomingMessageListener(JTlsDefs.ServiceName, null);
            endpoint.removeMessageTransport(this);
            endpoint = null;
        }
        
        if (null != manager) {
            manager.close();
            manager = null;
        }
        
        if (null != membership) {
            membership.removePropertyChangeListener(membershipListener);
            membershipListener = null;
            membership = null;
        }
        
        PSECredential temp = credential;
        
        if (null != temp) {
            temp.removePropertyChangeListener(credentialListener);
            credentialListener = null;
            credential = null;
        }
    }
    
    /**
     * {@inheritDoc}
     **/
    public boolean isConnectionOriented() {
        
        return true;
    }
    
    /**
     * {@inheritDoc}
     */
    public boolean allowsRouting() {
        // The TLS connection should not be used for default routing
        return false;
    }
    
    /**
     * {@inheritDoc}
     */
    public Object transportControl(Object operation, Object Value) {
        return null;
    }
    
    /**
     * {@inheritDoc}
     */
    public EndpointAddress getPublicAddress() {
        return localTlsPeerAddr;
    }
    
    /**
     *  {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        return endpoint;
    }
    
    /**
     * {@inheritDoc}
     */
    public Iterator getPublicAddresses() {
        return Collections.singletonList(getPublicAddress()).iterator();
    }
    
    /**
     * {@inheritDoc}
     */
    public String getProtocolName() {
        return JTlsDefs.tlsPName;
    }
    
    /**
     *  {@inheritDoc}
     */
    public boolean ping(EndpointAddress addr) {
        
        return null != getMessenger(addr, null);
    }
    
    /**
     *  {@inheritDoc}
     *
     *  XXX bondolo 20040522 The hint could be used in request for the
     * underlying messenger.
     */
    public Messenger getMessenger(EndpointAddress addr, Object hintIgnored) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("getMessenger for " + addr);
        }
        
        EndpointAddress plainAddress = new EndpointAddress(addr, null, null);
        
        // If the dest is the local peer, just loop it back without going
        // through the TLS. Local communication do not use TLS.
        if (plainAddress.equals(localTlsPeerAddr)) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("returning LoopbackMessenger");
            }
            return new TlsLoopbackMessenger(endpoint, plainAddress, addr, localPeerAddr);
        }
        
        // Create a Peer EndpointAddress
        EndpointAddress dstPAddr = mkAddress(ID.URIEncodingName + ":" + ID.URNNamespace + ":" + addr.getProtocolAddress(), null
                ,
                null);
        
        TlsConn conn = manager.getTlsConn(dstPAddr);
        
        if (conn == null) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.severe("Cannot get a TLS connection for " + dstPAddr);
            }
            // No connection was either available or created. Cannot do TLS
            // with the destination address.
            return null;
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("TlsMessanger with TlsConn DONE");
        }
        
        // Build a TlsMessenger around it that will add our header.
        // Right now we do not want to "announce" outgoing messengers because they get pooled and so must
        // not be grabbed by a listener. If "announcing" is to be done, that should be by the endpoint
        // and probably with a subtely different interface.
        return new TlsMessenger(addr, conn, this);
    }
    
    /**
     * processReceivedMessage is invoked by the TLS Manager when a message has been
     * completely received and is ready to be delivered to the service/application
     */
    void processReceivedMessage(final Message msg) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("processReceivedMessage starts");
        }
        
        // add a property to the message to indicate it came from us.
        msg.setMessageProperty(TlsTransport.class, this);
        
        // let the message continue to its final destination.
        try {
            ((GenericPeerGroup)group).getExecutor().execute( new Runnable() {
                public void run() {
                    try {
                        endpoint.demux(msg);
                    } catch(Throwable uncaught) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Failure demuxing an incoming message", uncaught);
                        }
                    }
                }
            });            
        } catch (Throwable e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure demuxing an incoming message", e);
            }
        }
    }
    
    /**
     *  Convenience method for constructing an endpoint address from an id
     *
     *  @param destPeer peer id
     *  @param serv the service name (if any)
     *  @param parm the service param (if any)
     *  @param endpointAddress for this peer id.
     */
    private final static EndpointAddress mkAddress(String destPeer, String serv, String parm) {
        
        ID asID = null;
        
        try {
            asID = IDFactory.fromURI(new URI(destPeer));
        } catch (URISyntaxException caught) {
            throw new IllegalArgumentException(caught.getMessage());
        }
        
        return mkAddress(asID, serv, parm);
    }
    
    /**
     *  Convenience method for constructing an endpoint address from an id
     *
     *  @param destPeer peer id
     *  @param serv the service name (if any)
     *  @param parm the service param (if any)
     *  @param endpointAddress for this peer id.
     */
    private final static EndpointAddress mkAddress(ID destPeer, String serv, String parm) {
        
        EndpointAddress addr = new EndpointAddress("jxta", destPeer.getUniqueValue().toString(), serv, parm);
        
        return addr;
    }
    
    /**
     *  Listener for Property Changed Events on our credential
     **/
    class credentialPCL implements PropertyChangeListener {
        
        /**
         *  {@inheritDoc}
         *
         *  <p/>Handle events on our active credential.
         **/
        public synchronized void propertyChange(PropertyChangeEvent evt) {
            
            if (credential == evt.getSource()) {
                if (!credential.isValid()) {
                    if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                        LOG.info("Clearing credential/certfile ");
                    }
                    
                    credential.removePropertyChangeListener(this);
                    credential = null;
                }
            }
        }
    }
    

    /**
     *  Listener for Property Changed Events on membership service
     **/
    class membershipPCL implements PropertyChangeListener {

        /**
         *  {@inheritDoc}
         **/
        public synchronized void propertyChange(PropertyChangeEvent evt) {
            
            String evtProp = evt.getPropertyName();
            PSECredential cred = (PSECredential) evt.getNewValue();
            
            boolean validCertificate = true;

            if (null != serviceCert) {
                try {
                    serviceCert[0].checkValidity();
                } catch (Exception notValidException) {
                    validCertificate = false;
                }
            }
            
            if ("addCredential".equals(evtProp) && ((null == serviceCert) || !validCertificate)) {
                // no service Cert or Non-valid Cert? Make one.
                Exception failure = null;
                
                try {
                    X509Certificate peerCert = membership.getPSEConfig().getTrustedCertificate(group.getPeerID());
                    
                    X500Principal credSubjectDN = cred.getCertificate().getSubjectX500Principal();
                    X500Principal peerCertSubjectDN = peerCert.getSubjectX500Principal();
                    
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine(
                                "Checking credential cert for match to peer cert" + "\n\tcred subject=" + credSubjectDN
                                + "\n\tpeer subject=" + peerCertSubjectDN);
                    }
                    
                    if (peerCertSubjectDN.equals(credSubjectDN)) {
                        
                        serviceCert = cred.generateServiceCertificate(assignedID);
                        
                    }
                } catch (IOException failed) {
                    failure = failed;
                } catch (KeyStoreException failed) {
                    failure = failed;
                } catch (InvalidKeyException failed) {
                    failure = failed;
                } catch (SignatureException failed) {
                    failure = failed;
                }
                
                if (null != failure) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "Failure building service certificate", failure);
                    }
                    
                    return;
                }
            }
            
            if ("addCredential".equals(evtProp)) {
                Exception failure = null;
                
                try {
                    X509Certificate credCert = cred.getCertificate();
                    
                    X500Principal credSubjectDN = credCert.getSubjectX500Principal();
                    X500Principal serviceIssuerDN = serviceCert[0].getIssuerX500Principal();
                    
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine(
                                "Checking credential cert for match to service issuer cert" + "\n\tcred subject=" + credSubjectDN
                                + "\n\t  svc issuer=" + serviceIssuerDN);
                    }
                    
                    if (credSubjectDN.equals(serviceIssuerDN)) {
                        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                            LOG.info("Setting credential/certfile ");
                        }
                        
                        credential = cred.getServiceCredential(assignedID);
                        
                        if (null != credential) {
                            credentialListener = new credentialPCL();
                            credential.addPropertyChangeListener(credentialListener);
                        }
                    }
                } catch (IOException failed) {
                    failure = failed;
                } catch (PeerGroupException failed) {
                    failure = failed;
                } catch (InvalidKeyException failed) {
                    failure = failed;
                } catch (SignatureException failed) {
                    failure = failed;
                }
                
                if (null != failure) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "Failure building service credential", failure);
                    }
                    
                    return;
                }
            }
        }
    }
}
