/*
 * Copyright (c) 2004-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.protocol;


import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.id.ID;
import net.jxta.impl.util.TimeUtils;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.net.URI;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;


/**
 * Contains parameters for configuration of the Reference Implemenation
 * Relay Service.
 * <p/>
 * <p/><pre><code>
 * <p/>
 * </code></pre>
 */
public final class RelayConfigAdv extends ExtendableAdvertisement implements Cloneable {

    /**
     * Log4J Logger
     */
    private static final Logger LOG = Logger.getLogger(RelayConfigAdv.class.getName());

    /**
     * Our DOCTYPE
     */
    private static final String advType = "jxta:RelayConfig";

    private static final String RELAY_CLIENT_ATTR = "client";
    private static final String RELAY_SERVER_ATTR = "server";

    private static final String RELAY_CLIENT_ELEMENT = "client";
    private static final String RELAY_CLIENT_SERVERS_ATTR = "maxRelays";
    private static final String RELAY_CLIENT_LEASE_ATTR = "maxLease";
    private static final String RELAY_CLIENT_POLL_ATTR = "messengerPollInterval";

    private static final String RELAY_CLIENT_SEEDS_ELEMENT = "seeds";
    private static final String USE_ONLY_SEEDS_ATTR = "useOnlySeeds";

    private static final String SEED_RELAY_ADDR_ELEMENT = "addr";
    private static final String SEED_RELAY_ADDR_SEEDING_ATTR = "seeding";

    private static final String RELAY_SERVER_ELEMENT = "server";
    private static final String RELAY_SERVER_CLIENTS_ATTR = "maxClients";
    private static final String RELAY_SERVER_QUEUE_ATTR = "clientQueue";
    private static final String RELAY_SERVER_LEASE_ATTR = "leaseDuration";
    private static final String RELAY_SERVER_STALL_ATTR = "stallTimeout";
    private static final String RELAY_SERVER_ANNOUNCE_ATTR = "announceInterval";
    private static final String ACL_URI = "acl";

    private static final String[] fields = {};

    /**
     * Are we configured as a relay client?
     */
    private boolean clientEnabled = false;

    /**
     * Max Relays
     */
    private int maxRelays = -1;

    /**
     * Max clients lease in relative milliseconds.
     */
    private long maxClientLeaseDuration = -1;

    /**
     * Messenger poll interval in relative milliseconds.
     */
    private long messengerPollInterval = -1;

    /**
     * Use only seeded relays.
     */
    private boolean useOnlySeeds = false;

    /**
     * Seed Relays
     * <p/>
     * <ul>
     * <li>Elements are {@link net.jxta.endpoint.EndpointAddress}</li>
     * </ul>
     */
    private Set<EndpointAddress> seedRelays = new HashSet<EndpointAddress>();

    /**
     * The list of seeding resources.
     * <p/>
     * <p/><ul>
     * <li>The values are {@link java.net.URI}.</li>
     * </ul>
     */
    private Set<URI> seedingURIs = new HashSet<URI>();

    /**
     * Are we configured as a relay server?
     */
    private boolean serverEnabled = false;

    /**
     * Max Clients
     */
    private int maxClients = -1;

    /**
     */
    private int maxClientMessageQueue = -1;

    /**
     * Max Lease offered by server in relative milliseconds.
     */
    private long maxServerLeaseDuration = -1;

    /**
     * Stall timeout in relative milliseconds.
     */
    private long stallTimeout = -1;

    /**
     * Announce interval in relative milliseconds.
     */
    private long announceInterval = -1;

    /**
     * Access control URI
     */
    private URI aclURI = null;

    /**
     * Instantiator for RelayConfigAdv
     */
    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return advType;
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new RelayConfigAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            return new RelayConfigAdv(root);
        }
    }

    /**
     * Returns the identifying type of this Advertisement.
     * <p/>
     * <p/><b>Note:</b> This is a static method. It cannot be used to determine
     * the runtime type of an advertisment. ie.
     * </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     * <p/>
     * <p/><b>This is wrong and does not work the way you might expect.</b>
     * This call is not polymorphic and calls
     * Advertiement.getAdvertisementType() no matter what the real type of the
     * advertisment.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return advType;
    }

    private RelayConfigAdv() {}

    private RelayConfigAdv(Element root) {
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;

        String doctype = doc.getName();

        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(getAdvertisementType()) && !getAdvertisementType().equals(typedoctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        Enumeration eachAttr = doc.getAttributes();

        while (eachAttr.hasMoreElements()) {
            Attribute aRelayAttr = (Attribute) eachAttr.nextElement();

            if (super.handleAttribute(aRelayAttr)) {
                // nothing to do
                ;
            } else if (RELAY_CLIENT_ATTR.equals(aRelayAttr.getName())) {
                clientEnabled = Boolean.valueOf(aRelayAttr.getValue().trim());
            } else if (RELAY_SERVER_ATTR.equals(aRelayAttr.getName())) {
                serverEnabled = Boolean.valueOf(aRelayAttr.getValue().trim());
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Attribute: " + aRelayAttr.getName());
                }
            }
        }

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Element: " + elem.toString());
                }
            }
        }

        // Sanity Check!!!
        if ((-1 != maxRelays) && (maxRelays <= 0)) {
            throw new IllegalArgumentException("Max relays must not be negative or zero.");
        }

        if ((-1 != maxClientLeaseDuration) && (maxClientLeaseDuration <= 0)) {
            throw new IllegalArgumentException("Max lease duration must not be negative or zero.");
        }

        if ((-1 != messengerPollInterval) && (messengerPollInterval <= 0)) {
            throw new IllegalArgumentException("Messenger poll interval must not be negative or zero.");
        }

        if (useOnlySeeds && clientEnabled && seedRelays.isEmpty() && seedingURIs.isEmpty()) {
            throw new IllegalArgumentException("Cannot specify 'useOnlySeeds' and no seed relays");
        }

        if ((-1 != maxClients) && (maxClients <= 0)) {
            throw new IllegalArgumentException("Max clients must not be negative or zero.");
        }

        if ((-1 != maxClientMessageQueue) && (maxClientMessageQueue <= 0)) {
            throw new IllegalArgumentException("Max client queue must not be negative or zero.");
        }

        if ((-1 != maxServerLeaseDuration) && (maxServerLeaseDuration <= 0)) {
            throw new IllegalArgumentException("Max lease duration must not be negative or zero.");
        }

        if ((-1 != stallTimeout) && (stallTimeout <= 0)) {
            throw new IllegalArgumentException("Client stall timeout duration must not be negative or zero.");
        }

        if ((-1 != announceInterval) && (announceInterval <= 0)) {
            throw new IllegalArgumentException("Announce interval must not be negative or zero.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public RelayConfigAdv clone() {

        RelayConfigAdv result;
        
        try {
            result = (RelayConfigAdv) super.clone();
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }  

        result.setAnnounceInterval(getAnnounceInterval());
        result.setClientEnabled(isClientEnabled());
        result.setClientLeaseDuration(getClientLeaseDuration());
        result.setClientMessageQueueSize(getClientMessageQueueSize());
        result.setMaxClients(getMaxClients());
        result.setMaxRelays(getMaxRelays());
        result.setMessengerPollInterval(getMessengerPollInterval());
        result.setServerEnabled(isServerEnabled());
        result.setServerLeaseDuration(getServerLeaseDuration());
        result.setStallTimeout(getStallTimeout());
        result.setUseOnlySeeds(getUseOnlySeeds());
        
        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ID getID() {
        return ID.nullID;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected boolean handleElement(Element raw) {

        if (super.handleElement(raw)) {
            return true;
        }

        XMLElement elem = (XMLElement) raw;

        if (RELAY_CLIENT_ELEMENT.equals(elem.getName())) {
            Enumeration eachAttr = elem.getAttributes();

            while (eachAttr.hasMoreElements()) {
                Attribute aRelayAttr = (Attribute) eachAttr.nextElement();

                if (RELAY_CLIENT_SERVERS_ATTR.equals(aRelayAttr.getName())) {
                    maxRelays = Integer.parseInt(aRelayAttr.getValue().trim());
                } else if (RELAY_CLIENT_LEASE_ATTR.equals(aRelayAttr.getName())) {
                    maxClientLeaseDuration = Long.parseLong(aRelayAttr.getValue().trim());
                } else if (RELAY_CLIENT_POLL_ATTR.equals(aRelayAttr.getName())) {
                    messengerPollInterval = Long.parseLong(aRelayAttr.getValue().trim());
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled Attribute: " + aRelayAttr.getName());
                    }
                }
            }

            Enumeration elements = elem.getChildren();

            while (elements.hasMoreElements()) {
                XMLElement seedsElem = (XMLElement) elements.nextElement();

                if (RELAY_CLIENT_SEEDS_ELEMENT.equals(seedsElem.getName())) {
                    Enumeration eachSeedsAttr = seedsElem.getAttributes();

                    while (eachSeedsAttr.hasMoreElements()) {
                        Attribute aRelayAttr = (Attribute) eachSeedsAttr.nextElement();

                        if (USE_ONLY_SEEDS_ATTR.equals(aRelayAttr.getName())) {
                            useOnlySeeds = Boolean.valueOf(aRelayAttr.getValue().trim());
                        } else {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.warning("Unhandled Attribute: " + aRelayAttr.getName());
                            }
                        }
                    }

                    Enumeration addrElements = seedsElem.getChildren();

                    while (addrElements.hasMoreElements()) {
                        XMLElement addrElem = (XMLElement) addrElements.nextElement();

                        if (SEED_RELAY_ADDR_ELEMENT.equals(addrElem.getName())) {
                            String endpAddrString = addrElem.getTextValue();

                            if (null != endpAddrString) {
                                URI endpURI = URI.create(endpAddrString.trim());

                                Attribute seedingAttr = addrElem.getAttribute(SEED_RELAY_ADDR_SEEDING_ATTR);

                                if ((null != seedingAttr) && Boolean.valueOf(seedingAttr.getValue().trim())) {
                                    seedingURIs.add(endpURI);
                                } else {
                                    seedRelays.add(new EndpointAddress(endpURI));
                                }
                            }
                        } else {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.warning("Unhandled Element: " + elem.toString());
                            }
                        }
                    }
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled Element: " + elem.toString());
                    }
                }
            }

            return true;
        } else if (RELAY_SERVER_ELEMENT.equals(elem.getName())) {
            Enumeration eachAttr = elem.getAttributes();

            while (eachAttr.hasMoreElements()) {
                Attribute aRelayAttr = (Attribute) eachAttr.nextElement();

                if (RELAY_SERVER_CLIENTS_ATTR.equals(aRelayAttr.getName())) {
                    maxClients = Integer.parseInt(aRelayAttr.getValue().trim());
                } else if (RELAY_SERVER_QUEUE_ATTR.equals(aRelayAttr.getName())) {
                    maxClientMessageQueue = Integer.parseInt(aRelayAttr.getValue().trim());
                } else if (RELAY_SERVER_LEASE_ATTR.equals(aRelayAttr.getName())) {
                    maxServerLeaseDuration = Long.parseLong(aRelayAttr.getValue().trim());
                } else if (RELAY_SERVER_STALL_ATTR.equals(aRelayAttr.getName())) {
                    stallTimeout = Long.parseLong(aRelayAttr.getValue().trim());
                } else if (RELAY_SERVER_ANNOUNCE_ATTR.equals(aRelayAttr.getName())) {
                    announceInterval = Long.parseLong(aRelayAttr.getValue().trim());
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled Attribute: " + aRelayAttr.getName());
                    }
                }
            }

            return true;
        } else if (ACL_URI.equals(elem.getName())) {
            String addrElement = elem.getTextValue();

            if (null != addrElement) {
                aclURI = URI.create(addrElement.trim());
            }

            return true;
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        if (!(adv instanceof Attributable)) {
            throw new IllegalStateException("Only Attributable documents are supported.");
        }

        if ((-1 != maxRelays) && (maxRelays <= 0)) {
            throw new IllegalStateException("Max relays must not be negative or zero.");
        }

        if ((-1 != maxClientLeaseDuration) && (maxClientLeaseDuration <= 0)) {
            throw new IllegalStateException("Max lease duration must not be negative or zero.");
        }

        if ((-1 != messengerPollInterval) && (messengerPollInterval <= 0)) {
            throw new IllegalStateException("Messenger poll interval must not be negative or zero.");
        }

        if (useOnlySeeds && clientEnabled && seedRelays.isEmpty() && seedingURIs.isEmpty()) {
            throw new IllegalStateException("Cannot specify 'useOnlySeeds' and no seed relays");
        }

        if ((-1 != maxClients) && (maxClients <= 0)) {
            throw new IllegalStateException("Max clients must not be negative or zero.");
        }

        if ((-1 != maxClientMessageQueue) && (maxClientMessageQueue <= 0)) {
            throw new IllegalStateException("Max client queue must not be negative or zero.");
        }

        if ((-1 != maxServerLeaseDuration) && (maxServerLeaseDuration <= 0)) {
            throw new IllegalStateException("Max lease duration must not be negative or zero.");
        }

        if ((-1 != stallTimeout) && (stallTimeout <= 0)) {
            throw new IllegalStateException("Client stall timeout duration must not be negative or zero.");
        }

        if ((-1 != announceInterval) && (announceInterval <= 0)) {
            throw new IllegalStateException("Announce interval must not be negative or zero.");
        }

        Attributable attrDoc = (Attributable) adv;

        if (clientEnabled) {
            attrDoc.addAttribute(RELAY_CLIENT_ATTR, Boolean.TRUE.toString());
        }

        if (serverEnabled) {
            attrDoc.addAttribute(RELAY_SERVER_ATTR, Boolean.TRUE.toString());
        }

        Element clientElem = adv.createElement(RELAY_CLIENT_ELEMENT);

        adv.appendChild(clientElem);

        Attributable attrElem = (Attributable) clientElem;

        if (-1 != maxRelays) {
            attrElem.addAttribute(RELAY_CLIENT_SERVERS_ATTR, Integer.toString(maxRelays));
        }

        if (-1 != maxClientLeaseDuration) {
            attrElem.addAttribute(RELAY_CLIENT_LEASE_ATTR, Long.toString(maxClientLeaseDuration));
        }

        if (-1 != messengerPollInterval) {
            attrElem.addAttribute(RELAY_CLIENT_POLL_ATTR, Long.toString(messengerPollInterval));
        }

        if (!seedRelays.isEmpty() || !seedingURIs.isEmpty()) {
            Element seedsElem = adv.createElement(RELAY_CLIENT_SEEDS_ELEMENT);

            clientElem.appendChild(seedsElem);

            attrElem = (Attributable) seedsElem;

            if (useOnlySeeds) {
                attrElem.addAttribute(USE_ONLY_SEEDS_ATTR, Boolean.TRUE.toString());
            }

            for (EndpointAddress seedRelay : seedRelays) {
                Element addrElement = adv.createElement(SEED_RELAY_ADDR_ELEMENT, seedRelay.toString());

                seedsElem.appendChild(addrElement);
            }

            for (URI seedingURI : seedingURIs) {
                Element addrElement = adv.createElement(SEED_RELAY_ADDR_ELEMENT, seedingURI.toString());

                seedsElem.appendChild(addrElement);

                ((Attributable) addrElement).addAttribute(SEED_RELAY_ADDR_SEEDING_ATTR, Boolean.TRUE.toString());
            }
        }

        Element serverElem = adv.createElement(RELAY_SERVER_ELEMENT);

        adv.appendChild(serverElem);

        attrElem = (Attributable) serverElem;

        if (-1 != maxClients) {
            attrElem.addAttribute(RELAY_SERVER_CLIENTS_ATTR, Integer.toString(maxClients));
        }

        if (-1 != maxClientMessageQueue) {
            attrElem.addAttribute(RELAY_SERVER_QUEUE_ATTR, Integer.toString(maxClientMessageQueue));
        }

        if (-1 != maxServerLeaseDuration) {
            attrElem.addAttribute(RELAY_SERVER_LEASE_ATTR, Long.toString(maxServerLeaseDuration));
        }

        if (-1 != stallTimeout) {
            attrElem.addAttribute(RELAY_SERVER_STALL_ATTR, Long.toString(stallTimeout));
        }

        if (-1 != announceInterval) {
            attrElem.addAttribute(RELAY_SERVER_ANNOUNCE_ATTR, Long.toString(announceInterval));
        }
        if (aclURI != null) {
            Element acl = adv.createElement(ACL_URI, aclURI.toString());

            adv.appendChild(acl);
        }
        return adv;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return fields;
    }

    /**
     * If true then this peer will act as a relay client.
     *
     * @return If true then this peer will act as a relay client.
     */
    public boolean isClientEnabled() {
        return clientEnabled;
    }

    /**
     * If true then this peer will act as a relay client.
     *
     * @param enabled If true then this peer will act as a relay client.
     */
    public void setClientEnabled(boolean enabled) {
        clientEnabled = enabled;
    }

    /**
     * Return the maximum number of relay clients.
     *
     * @return The maximum number of relay clients or <code>-1</code> for
     *         default value.
     */
    public int getMaxRelays() {
        return maxRelays;
    }

    /**
     * Sets the maximum number of relay clients.
     *
     * @param newvalue The maximum number of relay clients or <code>-1</code>
     *                 for default value or <code>-1</code> for default value.
     */
    public void setMaxRelays(int newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Max Relays must be > 0");
        }

        maxRelays = newvalue;
    }

    /**
     * The interval in relative milliseconds of leases accepted by clients.
     *
     * @return The interval in relative milliseconds of leases accepted by
     *         clients or <code>-1</code> for default value.
     */
    public long getClientLeaseDuration() {
        return maxClientLeaseDuration;
    }

    /**
     * Sets interval in relative milliseconds of leases accepted by clients.
     *
     * @param newvalue The interval in relative milliseconds of leases accepted
     *                 by clients or <code>-1</code> for default value.
     */
    public void setClientLeaseDuration(long newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Lease Duration must be > 0");
        }

        maxClientLeaseDuration = newvalue;
    }

    /**
     * The interval in relative milliseconds of at which clients will poll for
     * messengers
     *
     * @return The interval in relative milliseconds of at which clients will
     *         poll for messengers or <code>-1</code> for default value.
     */
    public long getMessengerPollInterval() {
        return messengerPollInterval;
    }

    /**
     * Sets interval in relative milliseconds of at which clients will poll for
     * messengers.
     *
     * @param newvalue The interval in relative milliseconds of at which clients
     *                 will poll for messengers or <code>-1</code> for default value.
     */
    public void setMessengerPollInterval(long newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Poll interval must be > 0");
        }

        messengerPollInterval = newvalue;
    }

    /**
     * If true then this peer will use only seed rendezvous when configured as
     * an edge peer.
     *
     * @return If true then this peer will use only seed rendezvous when configured as
     *         an edge peer.
     */
    public boolean getUseOnlySeeds() {
        return useOnlySeeds;
    }

    /**
     * Set whether this peer will use only seed rendezvous when configured as
     * an edge peer.
     *
     * @param onlySeeds If true then this peer will use only seed rendezvous when configured as
     *                  an edge peer.
     */
    public void setUseOnlySeeds(boolean onlySeeds) {
        useOnlySeeds = onlySeeds;
    }

    public EndpointAddress[] getSeedRelays() {
        return seedRelays.toArray(new EndpointAddress[seedRelays.size()]);
    }

    public void addSeedRelay(EndpointAddress addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        seedRelays.add(addr);
    }

    public void addSeedRelay(String addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        seedRelays.add(new EndpointAddress(addr));
    }

    public boolean removeSeedRelay(EndpointAddress addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        return seedRelays.remove(addr);
    }

    public void clearSeedRelays() {
        seedRelays.clear();
    }

    public URI[] getSeedingURIs() {
        return seedingURIs.toArray(new URI[seedingURIs.size()]);
    }

    public void addSeedingURI(URI addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        seedingURIs.add(addr);
    }

    public void addSeedingURI(String addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        seedingURIs.add(URI.create(addr));
    }

    public boolean removeSeedingURI(URI addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        return seedingURIs.remove(addr);
    }

    public void clearSeedingURIs() {
        seedingURIs.clear();
    }

    /**
     * If true then this peer will act as a relay server.
     *
     * @return If true then this peer will act as a relay server.
     */
    public boolean isServerEnabled() {
        return serverEnabled;
    }

    /**
     * If true then this peer will act as a relay server.
     *
     * @param enabled If true then this peer will act as a relay server.
     */
    public void setServerEnabled(boolean enabled) {
        serverEnabled = enabled;
    }

    /**
     * Return the maximum number of relay clients.
     *
     * @return The maximum number of relay clients or <code>-1</code> for
     *         default value.
     */
    public int getMaxClients() {
        return maxClients;
    }

    /**
     * Sets he maximum number of relay clients.
     *
     * @param newvalue The maximum number of relay clients or <code>-1</code>
     *                 for default value.
     */
    public void setMaxClients(int newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Max Clients must be > 0");
        }

        maxClients = newvalue;
    }

    /**
     * Return the client message queue length size.
     *
     * @return The client message queue length size or <code>-1</code> for default value.
     */
    public int getClientMessageQueueSize() {
        return maxClientMessageQueue;
    }

    /**
     * Sets the client message queue length size.
     *
     * @param newvalue The client message queue length size or <code>-1</code>
     *                 for default value.
     */
    public void setClientMessageQueueSize(int newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Client Message Queue Size must be > 0");
        }

        maxClientMessageQueue = newvalue;
    }

    /**
     * The interval in relative milliseconds of leases offered by servers.
     *
     * @return The interval in relative milliseconds of leases offered by servers.
     */
    public long getServerLeaseDuration() {
        return maxServerLeaseDuration;
    }

    /**
     * Sets interval in relative milliseconds of leases  offered by servers.
     *
     * @param newvalue The interval in relative milliseconds of leases offered
     *                 by servers or <code>-1</code> for default value.
     */
    public void setServerLeaseDuration(long newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Lease Duration must be >= 0");
        }

        maxServerLeaseDuration = newvalue;
    }

    /**
     * The interval in relative milliseconds after which a client is assumed to
     * no longer be connected if it fails to request messages.
     *
     * @return The interval in relative milliseconds after which a client is
     *         assumed to no longer be connected if it fails to request messages or
     *         <code>-1</code> for default value.
     */
    public long getStallTimeout() {
        return stallTimeout;
    }

    /**
     * Sets interval in relative milliseconds after which a client is assumed to
     * no longer be connected if it fails to request messages.
     *
     * @param newvalue The interval in relative milliseconds after which a
     *                 client is  assumed to no longer be connected if it fails to request
     *                 messages or <code>-1</code> for default value.
     */
    public void setStallTimeout(long newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Stall timeout must be > 0");
        }

        stallTimeout = newvalue;
    }

    /**
     * The interval in relative milliseconds at which relay server will
     * announce its presence.
     *
     * @return The interval in relative milliseconds at which relay server will
     *         broadcast its presence or <code>-1</code> for default value.
     */
    public long getAnnounceInterval() {
        return announceInterval;
    }

    /**
     * Sets interval in relative milliseconds at which relay server will
     * announce its presence  or <code>-1</code> for default value.
     *
     * @param newvalue The interval in relative milliseconds at which relay server will
     *                 announce its presence.
     */
    public void setAnnounceInterval(long newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Announce Interval must be > 0");
        }

        announceInterval = newvalue;
    }

    /**
     * Return ACL URI if set
     *
     * @return ACL URI if set, null otherwise
     */
    public URI getAclUri() {
        return aclURI;
    }

    /**
     * Sets ACL URI
     *
     * @param uri URI if set, null otherwise
     */
    public void setAclUri(URI uri) {
        aclURI = uri;
    }
}
