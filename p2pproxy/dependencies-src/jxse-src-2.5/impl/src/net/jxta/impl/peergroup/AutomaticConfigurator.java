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


import java.net.InetAddress;
import java.net.URI;
import java.util.Enumeration;
import java.util.Iterator;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.TransportAdvertisement;

import net.jxta.exception.ConfiguratorException;

import net.jxta.impl.endpoint.IPUtils;
import net.jxta.impl.protocol.HTTPAdv;
import net.jxta.impl.protocol.PlatformConfig;
import net.jxta.impl.protocol.PSEConfigAdv;
import net.jxta.impl.protocol.RdvConfigAdv;
import net.jxta.impl.protocol.RelayConfigAdv;
import net.jxta.impl.protocol.TCPAdv;


/**
 * A simple platform configurator. This implementation provides reasonable
 * automatic configuration for edge peers on the JXTA public network.
 * <p/>
 * This implementation will read default values from several Java system
 * properties as appropriate:
 * <p/>
 * jxta.peer.name    --  The peer name to use.
 * jxta.http.port    --  The http port to use.
 * jxta.tcp.port     --  The tcp port to use.
 *
 * @see net.jxta.peergroup.Configurator
 */
public class AutomaticConfigurator extends NullConfigurator {

    /**
     * Log4J logger
     */
    private final static transient Logger LOG = Logger.getLogger(AutomaticConfigurator.class.getName());

    /**
     * Configures the platform using the specified directory.
     * @param jxtaHome store home URI
     * @throws net.jxta.exception.ConfiguratorException if a configuration error occurs
     */
    public AutomaticConfigurator(URI jxtaHome) throws ConfiguratorException {
        super(jxtaHome);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public PlatformConfig getPlatformConfig() throws ConfiguratorException {
        super.getPlatformConfig();

        boolean reconf;

        try {
            reconf = buildPlatformConfig();
        } catch (RuntimeException serious) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Trouble while fixing PlatformConfig. Hope for the best.", serious);
            }

            reconf = true;
        }

        // See if we need a reconf
        if (reconf) {
            throw new IncompleteConfigurationException("Damaged platform configuration.");
        }

        // Save the updated config.
        save();

        return advertisement;
    }

    /**
     * Makes sure a PlatformConfig is present and if not, creates one.
     * <p/>
     * Performs some checking of PlatformConfig values and will fix some
     * minor configuration problems automatically.
     *
     * @return If <tt>true</tt> then manual reconfiguration (of some form) is required.
     */
    private boolean buildPlatformConfig() {

        boolean reconf = false;

        if (advertisement == null) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("New PlatformConfig Advertisement");
            }
            advertisement = (PlatformConfig) AdvertisementFactory.newAdvertisement(PlatformConfig.getAdvertisementType());
            advertisement.setDescription("Platform Config Advertisement created by : " + AutomaticConfigurator.class.getName());
        }

        // Set the peer name
        String peerName = advertisement.getName();

        if ((null == peerName) || (0 == peerName.trim().length())) {
            String jpn = System.getProperty("jxta.peer.name", "");

            if (0 != jpn.trim().length()) {
                advertisement.setName(jpn);
            }
        }

        // Check the HTTP Message Transport parameters.
        XMLDocument http = (XMLDocument) advertisement.getServiceParam(PeerGroup.httpProtoClassID);
        HTTPAdv httpAdv = null;
        boolean httpEnabled = true;

        if (http != null) {
            try {
                httpEnabled = advertisement.isSvcEnabled(PeerGroup.httpProtoClassID);

                XMLElement param = null;

                Enumeration httpChilds = http.getChildren(TransportAdvertisement.getAdvertisementType());

                // get the HTTPAdv from TransportAdv
                if (httpChilds.hasMoreElements()) {
                    param = (XMLElement) httpChilds.nextElement();
                }

                if (null != param) {
                    httpAdv = (HTTPAdv) AdvertisementFactory.newAdvertisement(param);

                    if (httpEnabled) {
                        // check if the interface address is still valid.
                        String intf = httpAdv.getInterfaceAddress();

                        if ((null != intf) && !isValidInetAddress(intf)) {
                            reconf = true;

                            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                                LOG.config("Reconfig requested - invalid interface address");
                            }
                        }
                    }
                }
            } catch (RuntimeException advTrouble) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "HTTP advertisement corrupted", advTrouble);
                }

                httpAdv = null;
            }
        }

        if (httpAdv == null) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("HTTP advertisement missing, making a new one.");
            }

            int port = 0;
            // get the port from a property
            String httpPort = System.getProperty("jxta.http.port");

            if (httpPort != null) {
                try {
                    int propertyPort = Integer.parseInt(httpPort);

                    if ((propertyPort < 65536) && (propertyPort >= 0)) {
                        port = propertyPort;
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Property \'jxta.http.port\' is not a valid port number : " + propertyPort);
                        }
                    }
                } catch (NumberFormatException ignored) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Property \'jxta.http.port\' was not an integer : " + http);
                    }
                }
            }

            httpAdv = (HTTPAdv) AdvertisementFactory.newAdvertisement(HTTPAdv.getAdvertisementType());
            httpAdv.setProtocol("http");
            httpAdv.setPort(port);
            httpAdv.setServerEnabled(false);
        }

        // Create new param docs that contain the updated adv
        http = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");
        XMLDocument httAdvDoc = (XMLDocument) httpAdv.getDocument(MimeMediaType.XMLUTF8);

        StructuredDocumentUtils.copyElements(http, http, httAdvDoc);
        if (!httpEnabled) {
            http.appendChild(http.createElement("isOff"));
        }
        advertisement.putServiceParam(PeerGroup.httpProtoClassID, http);

        // Check the TCP Message Transport parameters.
        XMLDocument tcp = (XMLDocument) advertisement.getServiceParam(PeerGroup.tcpProtoClassID);
        TCPAdv tcpAdv = null;
        boolean tcpEnabled = true;

        if (tcp != null) {
            try {
                tcpEnabled = advertisement.isSvcEnabled(PeerGroup.tcpProtoClassID);

                XMLElement param = null;

                Enumeration tcpChilds = tcp.getChildren(TransportAdvertisement.getAdvertisementType());

                // get the TransportAdv
                if (tcpChilds.hasMoreElements()) {
                    param = (XMLElement) tcpChilds.nextElement();
                }

                if (null != param) {
                    tcpAdv = (TCPAdv) AdvertisementFactory.newAdvertisement(param);

                    if (tcpEnabled) {
                        String intf = tcpAdv.getInterfaceAddress();

                        if ((null != intf) && !isValidInetAddress(intf)) {
                            reconf = true;

                            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                                LOG.config("Reconfig requested - invalid interface address");
                            }
                        }
                    }
                }
            } catch (RuntimeException advTrouble) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "TCP advertisement corrupted", advTrouble);
                }

                tcpAdv = null;
            }
        }

        if (tcpAdv == null) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("TCP advertisement missing, making a new one.");
            }

            int port = 0;
            // get the port from a property
            String tcpPort = System.getProperty("jxta.tcp.port");

            if (tcpPort != null) {
                try {
                    int propertyPort = Integer.parseInt(tcpPort);

                    if ((propertyPort < 65536) && (propertyPort >= 0)) {
                        port = propertyPort;
                    } else {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Property \'jxta.tcp.port\' is not a valid port number : " + propertyPort);
                        }
                    }
                } catch (NumberFormatException ignored) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Property \'jxta.tcp.port\' was not an integer : " + tcpPort);
                    }
                }
            }

            tcpAdv = (TCPAdv) AdvertisementFactory.newAdvertisement(TCPAdv.getAdvertisementType());

            tcpAdv.setProtocol("tcp");
            tcpAdv.setPort(port);
            tcpAdv.setMulticastAddr("224.0.1.85");
            tcpAdv.setMulticastPort(1234);
            tcpAdv.setMulticastSize(16384);
            tcpAdv.setMulticastState(true);
        }

        tcp = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");

        StructuredDocumentUtils.copyElements(tcp, tcp, (XMLDocument) tcpAdv.getDocument(MimeMediaType.XMLUTF8));
        if (!tcpEnabled) {
            tcp.appendChild(tcp.createElement("isOff"));
        }
        advertisement.putServiceParam(PeerGroup.tcpProtoClassID, tcp);

        // Check the relay config
        RelayConfigAdv relayConfig = null;

        try {
            XMLElement param = (XMLElement) advertisement.getServiceParam(PeerGroup.relayProtoClassID);

            if (param != null) {
                // XXX 20041027 backwards compatibility
                param.addAttribute("type", RelayConfigAdv.getAdvertisementType());

                relayConfig = (RelayConfigAdv) AdvertisementFactory.newAdvertisement(param);
            }
        } catch (Exception failure) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Problem reading relay configuration", failure);
            }
        }

        if (null == relayConfig) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("Relay Config advertisement missing, making a new one.");
            }

            // restore default values.
            relayConfig = (RelayConfigAdv) AdvertisementFactory.newAdvertisement(RelayConfigAdv.getAdvertisementType());

            // Enable relay if any transport doesn't support incoming.
            if (!tcpAdv.isServerEnabled() || !httpAdv.isServerEnabled()) {
                relayConfig.setClientEnabled(true);
            }
        }

        /*
         if( (0 == relayConfig.getSeedingURIs().length) && (0 == relayConfig.getSeedRelays().length) && !relayConfig.isServerEnabled() ) {
         // add the default relay seeding peer.
         relayConfig.addSeedingURI( "http://rdv.jxtahosts.net/cgi-bin/relays.cgi?3" );
         }
         */

        XMLDocument relayDoc = (XMLDocument) relayConfig.getDocument(MimeMediaType.XMLUTF8);

        advertisement.putServiceParam(PeerGroup.relayProtoClassID, relayDoc);

        // Check Rendezvous Configuration
        RdvConfigAdv rdvAdv = null;

        try {
            XMLElement param = (XMLElement) advertisement.getServiceParam(PeerGroup.rendezvousClassID);

            if (param != null) {
                // XXX 20041027 backwards compatibility
                param.addAttribute("type", RdvConfigAdv.getAdvertisementType());

                rdvAdv = (RdvConfigAdv) AdvertisementFactory.newAdvertisement(param);
            }
        } catch (Exception failure) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Problem reading rendezvous configuration", failure);
            }
        }

        if (null == rdvAdv) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("Rdv Config advertisement missing, making a new one.");
            }

            // restore default values.
            rdvAdv = (RdvConfigAdv) AdvertisementFactory.newAdvertisement(RdvConfigAdv.getAdvertisementType());
        }

        /*
         if( (0 == rdvAdv.getSeedingURIs().length) &&
         (0 == rdvAdv.getSeedRendezvous().length) &&
         (RdvConfigAdv.RendezVousConfiguration.RENDEZVOUS != rdvAdv.getConfiguration()) &&
         (RdvConfigAdv.RendezVousConfiguration.AD_HOC != rdvAdv.getConfiguration()) &&
         !relayConfig.isClientEnabled() ) {
         // add the default rendezvous seeding peer if we don't know of any rendezvous, aren't a rendezvous ourselves, aren't in ad-hoc mode or using a relay.
         rdvAdv.addSeedingURI( "http://rdv.jxtahosts.net/cgi-bin/rendezvous.cgi?3" );
         }
         */
        XMLDocument rdvDoc = (XMLDocument) rdvAdv.getDocument(MimeMediaType.XMLUTF8);

        advertisement.putServiceParam(PeerGroup.rendezvousClassID, rdvDoc);

        // if no proxy param section, disable it.
        XMLDocument proxy = (XMLDocument) advertisement.getServiceParam(PeerGroup.proxyClassID);

        if (null == proxy) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("Proxy config advertisement missing, making a new one.");
            }

            proxy = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");
            proxy.appendChild(proxy.createElement("isOff"));
            advertisement.putServiceParam(PeerGroup.proxyClassID, proxy);
        }

        // Check the PSE Configuration
        PSEConfigAdv pseConfig = null;

        try {
            XMLElement param = (XMLElement) advertisement.getServiceParam(PeerGroup.membershipClassID);

            if (param != null) {
                // XXX 20041027 backwards compatibility
                param.addAttribute("type", PSEConfigAdv.getAdvertisementType());

                pseConfig = (PSEConfigAdv) AdvertisementFactory.newAdvertisement(param);
            }
        } catch (Exception failure) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Problem reading pse configuration", failure);
            }
        }

        if (null == pseConfig) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("PSE Config advertisement missing, making a new one.");
            }

            // restore default values.
            pseConfig = (PSEConfigAdv) AdvertisementFactory.newAdvertisement(PSEConfigAdv.getAdvertisementType());
            XMLDocument pseDoc = (XMLDocument) pseConfig.getDocument(MimeMediaType.XMLUTF8);

            advertisement.putServiceParam(PeerGroup.membershipClassID, pseDoc);
        }

        // If we did not modify anything of importance or see anything wrong,
        // leave the adv alone.

        return reconf;
    }

    private boolean isValidInetAddress(String address) {
        boolean found = false;
        boolean loopback;

        InetAddress[] ias;

        try {
            ias = InetAddress.getAllByName(address);
        } catch (java.net.UnknownHostException notfound) {
            return false;
        }

        for (Iterator la = IPUtils.getAllLocalAddresses(); la.hasNext() && !found;) {
            for (InetAddress ia1 : ias) {
                found |= ia1.equals(la.next());
            }
        }

        loopback = true;

        for (InetAddress ia1 : ias) {
            loopback &= ia1.isLoopbackAddress();
        }

        return found || loopback;
    }
}
