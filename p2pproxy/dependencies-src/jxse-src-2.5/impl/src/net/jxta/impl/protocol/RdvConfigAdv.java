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
import net.jxta.id.ID;
import net.jxta.logging.Logging;

import java.net.URI;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * Contains parameters for configuration of the Reference Implementation
 * Rendezvous Service.
 * <p/>
 * <p/><pre><code>
 * &lt;xs:complexType name="RdvConfig">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="seeds" minOccurs="0" maxOccurs="1">
 *       &lt;xs:complexType>
 *         &lt;xs:sequence>
 *           &lt;xs:element name="addr" minOccurs="1" maxOccurs="unbounded">
 *             &lt;xs:complexType>
 *               &lt;xs:simpleContent>
 *                 &lt;xs:extension base="jxta:JXTAID">
 *                   &lt;xs:attribute name="seeding" type="xs:boolean" default="false"/>
 *                 &lt;/xs:extension>
 *               &lt;/xs:simpleContent>
 *             &lt;/xs:complexType>
 *           &lt;/xs:element>
 *         &lt;/xs:sequence>
 *         &lt;xs:attribute name="useOnlySeeds" type="xs:boolean" default="false"/>
 *         &lt;xs:attribute name="connectDelay" type="xs:unsignedLong"/>
 *       &lt;/xs:complexType>
 *     &lt;/xs:element>
 *   &lt;/xs:sequence>
 *   &lt;xs:attribute name="config" type="jxta:RdvConfiguration"/>
 *   &lt;xs:attribute name="maxTTL" type="xs:unsignedInt"/>
 *   &lt;xs:attribute name="autoRendezvousInterval" type="xs:unsignedInt"/>
 *   &lt;xs:attribute name="probeRelays" type="xs:boolean" default="true"/>
 *   &lt;xs:attribute name="maxClients" type="xs:unsignedInt"/>
 *   &lt;xs:attribute name="leaseDuration" type="xs:unsignedLong"/>
 *   &lt;xs:attribute name="leaseMargin" type="xs:unsignedLong"/>
 *   &lt;xs:attribute name="minHappyPeerView" type="xs:unsignedInt"/>
 * &lt;/xs:complexType>
 * </code></pre>
 *
 * @since JXTA 2.2
 */
public final class RdvConfigAdv extends ExtendableAdvertisement implements Cloneable {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(RdvConfigAdv.class.getName());

    /**
     * Instantiator for RdvConfigAdv
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
            return new RdvConfigAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }

            return new RdvConfigAdv((XMLElement) root);
        }
    }

    /**
     * Our DOCTYPE
     */
    private static final String advType = "jxta:RdvConfig";

    private static final String RDV_CONFIG_ATTR = "config";
    private static final String MAX_TTL_ATTR = "maxTTL";
    private static final String AUTO_RDV_INTERVAL_ATTR = "autoRendezvousInterval";
    private static final String PROBE_RELAYS_ATTR = "probeRelays";
    private static final String MAX_CLIENTS_ATTR = "maxClients";
    private static final String LEASE_DURATION_ATTR = "leaseDuration";
    private static final String LEASE_MARGIN_ATTR = "leaseMargin";
    private static final String MIN_HAPPY_PEERVIEW_ATTR = "minHappyPeerView";

    private static final String SEEDS_RDV_ELEMENT = "seeds";
    private static final String USE_ONLY_SEEDS_ATTR = "useOnlySeeds";
    private static final String CONNECT_DELAY_ATTR = "connectDelay";

    private static final String SEED_RDV_ADDR_ELEMENT = "addr";
    private static final String SEED_RDV_ADDR_SEEDING_ATTR = "seeding";
    private static final String ACL_URI = "acl";

    // This one is deprecated.
    private static final String PROPAGATE_RESPOND_ATTR = "propagateRespondProbability";

    private static final String[] INDEXFIELDS = {};

    /**
     * Configuration for this peer.
     */
    private RendezVousConfiguration configuration = RendezVousConfiguration.EDGE;

    /**
     * The interval in relative milliseconds at which this peer will re-evaluate
     * it's state as a rendezvous. If <code>0</code> (zero), the default, then
     * the peer will remain in the state of <code>isRendezvous</code>.
     */
    private long autoRendezvousCheckInterval = 0L;

    /**
     * If true then rendezvous clients will probe relay servers for rendezvous.
     */
    private boolean probeRelays = true;

    /**
     * Max Clients
     */
    private int maxClients = -1;

    /**
     * Maximum TTL
     */
    private int maximumTTL = -1;

    /**
     * Lease Duration
     */
    private long leaseDuration = 0L;

    /**
     * Lease Margin
     */
    private long leaseMargin = 0L;

    /**
     * Minimum desirable peerview size.
     */
    private int minHappyPeerView = -1;

    /**
     * If true then this peer will use only seed rendezvous when configured as
     * an edge peer.
     */
    private boolean useOnlySeeds = false;

    /**
     * The interval in relative milliseconds before which this peer will
     * attempt to contact the the seed peers.
     */
    private long seedRendezvousConnectDelay = 0L;

    /**
     * The set of seed rendezvous.
     */
    private final Set<URI> seedRendezvous = new HashSet<URI>();

    /**
     * The set of seeding resources.
     */
    private final Set<URI> seedingURIs = new HashSet<URI>();

    /**
     * Access control URI
     */
    private URI aclURI = null;

    /**
     * Possible Rendezvous configurations.
     */
    public enum RendezVousConfiguration {
        
        /**
         *  Connectionless 
         */
        AD_HOC, 
        
        /**
         *  Edge peer
         */
        EDGE, 
                
        /**
         *  Rendezvous peer
         */
        RENDEZVOUS
    }

    /**
     * Returns the identifying type of this Advertisement.
     * <p/>
     * <p/><b>Note:</b> This is a static method. It cannot be used to determine
     * the runtime type of an advertisement. ie.
     * </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     * <p/>
     * <p/><b>This is wrong and does not work the way you might expect.</b>
     * This call is not polymorphic and calls
     * Advertisement.getAdvertisementType() no matter what the real type of the
     * advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return advType;
    }

    /**
     *  Private constructor for new instances. Use the instantiator.
     */
    private RdvConfigAdv() {}

    /**
     *  Private constructor for xml serialized instances. Use the instantiator.
     *  
     *  @param doc The XML serialization of the advertisement.
     */
    private RdvConfigAdv(XMLElement doc) {
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
            Attribute aRdvAttr = (Attribute) eachAttr.nextElement();

            if (super.handleAttribute(aRdvAttr)) {// nothing to do
            } else if (RDV_CONFIG_ATTR.equals(aRdvAttr.getName())) {
                String config = aRdvAttr.getValue().trim();

                if ("adhoc".equals(config)) {
                    configuration = RendezVousConfiguration.AD_HOC;
                } else if ("client".equals(config)) {
                    configuration = RendezVousConfiguration.EDGE;
                } else if ("rendezvous".equals(config)) {
                    configuration = RendezVousConfiguration.RENDEZVOUS;
                } else {
                    throw new IllegalArgumentException("Unrecognized Rendezvous configuration :" + config);
                }
            } else if (MAX_TTL_ATTR.equals(aRdvAttr.getName())) {
                maximumTTL = Integer.parseInt(aRdvAttr.getValue().trim());
            } else if (AUTO_RDV_INTERVAL_ATTR.equals(aRdvAttr.getName())) {
                autoRendezvousCheckInterval = Long.parseLong(aRdvAttr.getValue().trim());
            } else if (PROBE_RELAYS_ATTR.equals(aRdvAttr.getName())) {
                probeRelays = Boolean.valueOf(aRdvAttr.getValue().trim());
            } else if (MAX_CLIENTS_ATTR.equals(aRdvAttr.getName())) {
                maxClients = Integer.parseInt(aRdvAttr.getValue().trim());
            } else if (LEASE_DURATION_ATTR.equals(aRdvAttr.getName())) {
                leaseDuration = Long.parseLong(aRdvAttr.getValue().trim());
            } else if (LEASE_MARGIN_ATTR.equals(aRdvAttr.getName())) {
                leaseMargin = Long.parseLong(aRdvAttr.getValue().trim());
            } else if (MIN_HAPPY_PEERVIEW_ATTR.equals(aRdvAttr.getName())) {
                minHappyPeerView = Integer.parseInt(aRdvAttr.getValue().trim());
            } else if (PROPAGATE_RESPOND_ATTR.equals(aRdvAttr.getName())) {// Ignored; deprecated.
            } else if ("Flags".equals(aRdvAttr.getName())) { // deprecated
                boolean onlySeeds = (aRdvAttr.getValue().indexOf("UseOnlySeeds") != -1);

                setUseOnlySeeds(onlySeeds);
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Attribute: " + aRdvAttr.getName());
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

        if ((maximumTTL != -1) && (maximumTTL < 0)) {
            throw new IllegalArgumentException("Maximum TTL must be >= 0");
        }

        if (autoRendezvousCheckInterval < 0) {
            throw new IllegalArgumentException("Auto Rendezvous Check Interval must be >= 0");
        }

        if (seedRendezvousConnectDelay < 0) {
            throw new IllegalArgumentException("Seed Rendezvous Connect Delay must be >= 0");
        }

        if ((-1 != minHappyPeerView) && (minHappyPeerView <= 0)) {
            throw new IllegalArgumentException("Min Happy Peer View must be > 0");
        }

        if ((seedingURIs.isEmpty() && seedRendezvous.isEmpty() && useOnlySeeds) && (configuration == RendezVousConfiguration.EDGE)) {
            throw new IllegalArgumentException("Must specify rendezvous if 'useOnlySeeds' is enabled and configured as client");
        }
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

        if (SEEDS_RDV_ELEMENT.equals(elem.getName())) {

            Enumeration eachSeedElem = elem.getChildren();

            while (eachSeedElem.hasMoreElements()) {
                XMLElement aSeedElem = (XMLElement) eachSeedElem.nextElement();

                if (SEED_RDV_ADDR_ELEMENT.equals(aSeedElem.getName())) {
                    String endpAddrString = aSeedElem.getTextValue();

                    if (null != endpAddrString) {
                        URI endpAddr = URI.create(endpAddrString.trim());

                        Attribute seedingAttr = aSeedElem.getAttribute(SEED_RDV_ADDR_SEEDING_ATTR);

                        if ((null != seedingAttr) && Boolean.valueOf(seedingAttr.getValue().trim())) {
                            seedingURIs.add(endpAddr);
                        } else {
                            seedRendezvous.add(endpAddr);
                        }
                    }
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled Element: " + aSeedElem.getName());
                    }
                }
            }

            Enumeration eachAttr = elem.getAttributes();

            while (eachAttr.hasMoreElements()) {
                Attribute aSeedAttr = (Attribute) eachAttr.nextElement();

                if (USE_ONLY_SEEDS_ATTR.equals(aSeedAttr.getName())) {
                    useOnlySeeds = Boolean.valueOf(aSeedAttr.getValue().trim());
                } else if (CONNECT_DELAY_ATTR.equals(aSeedAttr.getName())) {
                    seedRendezvousConnectDelay = Long.parseLong(aSeedAttr.getValue().trim());
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unhandled Attribute: " + aSeedAttr.getName());
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

        Attributable attrDoc = (Attributable) adv;

        if (RendezVousConfiguration.AD_HOC == configuration) {
            attrDoc.addAttribute(RDV_CONFIG_ATTR, "adhoc");
        } else if (RendezVousConfiguration.EDGE == configuration) {
            attrDoc.addAttribute(RDV_CONFIG_ATTR, "client");
        } else if (RendezVousConfiguration.RENDEZVOUS == configuration) {
            attrDoc.addAttribute(RDV_CONFIG_ATTR, "rendezvous");
        }

        if (maximumTTL != -1) {
            if (maximumTTL < 0) {
                throw new IllegalStateException("Maximum TTL must be >= 0");
            }

            attrDoc.addAttribute(MAX_TTL_ATTR, Integer.toString(maximumTTL));
        }

        if ((seedingURIs.isEmpty() && seedRendezvous.isEmpty() && useOnlySeeds) && (configuration == RendezVousConfiguration.EDGE)) {
            throw new IllegalStateException("Must specify rendezvous if 'useOnlySeeds' is enabled and configured as client");
        }

        if (0 != autoRendezvousCheckInterval) {
            if (autoRendezvousCheckInterval < 0) {
                throw new IllegalStateException("Auto Rendezvous Check Interval must be >= 0");
            }

            attrDoc.addAttribute(AUTO_RDV_INTERVAL_ATTR, Long.toString(autoRendezvousCheckInterval));
        }

        if (!probeRelays) {
            attrDoc.addAttribute(PROBE_RELAYS_ATTR, Boolean.toString(probeRelays));
        }

        if (-1 != maxClients) {
            if (maxClients < 0) {
                throw new IllegalStateException("Max Clients must be >= 0");
            }

            attrDoc.addAttribute(MAX_CLIENTS_ATTR, Integer.toString(maxClients));
        }

        if (0 != leaseDuration) {
            if (leaseDuration < 0) {
                throw new IllegalStateException("Lease Duration must be >= 0");
            }

            attrDoc.addAttribute(LEASE_DURATION_ATTR, Long.toString(leaseDuration));
        }

        if (0 != leaseMargin) {
            if (leaseMargin < 0) {
                throw new IllegalStateException("Lease Margin must be >= 0");
            }

            attrDoc.addAttribute(LEASE_MARGIN_ATTR, Long.toString(leaseMargin));
        }

        if (-1 != minHappyPeerView) {
            if (minHappyPeerView < 0) {
                throw new IllegalStateException("Min Happy Peer View must be > 0");
            }

            attrDoc.addAttribute(MIN_HAPPY_PEERVIEW_ATTR, Integer.toString(minHappyPeerView));
        }

        if (!seedRendezvous.isEmpty() || !seedingURIs.isEmpty()) {
            Element seedsElem = adv.createElement(SEEDS_RDV_ELEMENT);

            adv.appendChild(seedsElem);

            Attributable attrSeeds = (Attributable) seedsElem;

            if (useOnlySeeds) {
                attrSeeds.addAttribute(USE_ONLY_SEEDS_ATTR, Boolean.toString(useOnlySeeds));
            }

            if (0 != seedRendezvousConnectDelay) {
                if (seedRendezvousConnectDelay < 0) {
                    throw new IllegalStateException("Seed Rendezvous Connect Delay must be >= 0");
                }

                attrSeeds.addAttribute(CONNECT_DELAY_ATTR, Long.toString(seedRendezvousConnectDelay));
            }

            for (URI seedRendezvou : seedRendezvous) {
                Element aSeed = adv.createElement(SEED_RDV_ADDR_ELEMENT, seedRendezvou.toString());

                seedsElem.appendChild(aSeed);
            }

            for (URI seedingURI : seedingURIs) {
                Element aSeed = adv.createElement(SEED_RDV_ADDR_ELEMENT, seedingURI.toString());

                seedsElem.appendChild(aSeed);

                Attributable seedAttr = (Attributable) aSeed;

                seedAttr.addAttribute(SEED_RDV_ADDR_SEEDING_ATTR, Boolean.TRUE.toString());
            }
            if (aclURI != null) {
                Element acl = adv.createElement(ACL_URI, aclURI.toString());

                adv.appendChild(acl);
            }
        }

        return adv;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return INDEXFIELDS;
    }

    /**
     * True if this peer is to default to act as a rendezvous.
     *
     * @return True if this peer is to be a rendezvous
     */
    public RendezVousConfiguration getConfiguration() {
        return configuration;
    }

    /**
     * Set the default rendezvous state of this peer.
     *
     * @param newstate if true then this peer should default to acting as a
     *                 rendezvous.
     */
    public void setConfiguration(RendezVousConfiguration newstate) {
        configuration = newstate;
    }

    /**
     * The interval in relative milliseconds at which this peer will re-evaluate
     * it's state as a rendezvous. If <code>0</code> (zero), the default, then
     * the peer will remain in the state of <code>isRendezvous</code>.
     *
     * @return The interval in relative milliseconds at which this peer will re-evaluate
     *         it's state as a rendezvous. If <code>0</code> (zero), the default, then
     *         the peer will remain in the state of <code>isRendezvous</code>.
     */
    public long getAutoRendezvousCheckInterval() {
        return autoRendezvousCheckInterval;
    }

    /**
     * Sets the interval in relative milliseconds at which this peer will re-evaluate
     * it's state as a rendezvous. If <code>0</code> (zero), the default, then
     * the peer will remain in the state of <code>isRendezvous</code>.
     *
     * @param newvalue The interval in relative milliseconds at which this peer
     *                 will re-evaluate it's state as a rendezvous. If <code>0</code> (zero),
     *                 the default, then the peer will remain in the state of
     *                 <code>isRendezvous</code>.
     */
    public void setAutoRendezvousCheckInterval(long newvalue) {
        if (newvalue < 0) {
            throw new IllegalArgumentException("Auto Rendezvous Check Interval must be >= 0");
        }

        autoRendezvousCheckInterval = newvalue;
    }

    /**
     * If true then rendezvous clients will probe relay servers for rendezvous.
     *
     * @return If true then rendezvous clients will probe relay servers for rendezvous.
     */
    public boolean getProbeRelays() {
        return probeRelays;
    }

    /**
     * Set whether rendezvous clients will probe relay servers for rendezvous.
     *
     * @param doProbe If true then rendezvous clients will probe relay servers for rendezvous.
     */
    public void setProbeRelays(boolean doProbe) {
        probeRelays = doProbe;
    }

    public URI[] getSeedRendezvous() {
        return seedRendezvous.toArray(new URI[seedRendezvous.size()]);
    }

    public void addSeedRendezvous(URI addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        seedRendezvous.add(addr);
    }

    public void addSeedRendezvous(String addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        seedRendezvous.add(URI.create(addr));
    }

    public boolean removeSeedRendezvous(URI addr) {
        if (null == addr) {
            throw new IllegalArgumentException("addr may not be null");
        }

        return seedRendezvous.remove(addr);
    }

    public void clearSeedRendezvous() {
        seedRendezvous.clear();
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
     * If true then this peer will use only seed rendezvous when configured as
     * an edge peer.
     *
     * @return If true then this peer will use only seed rendezvous when
     *         configured as an edge peer.
     */
    public boolean getUseOnlySeeds() {
        return useOnlySeeds;
    }

    /**
     * Set whether this peer will use only seed rendezvous when configured as
     * an edge peer.
     *
     * @param onlySeeds If true then this peer will use only seed rendezvous
     *                  when configured as an edge peer.
     */
    public void setUseOnlySeeds(boolean onlySeeds) {
        useOnlySeeds = onlySeeds;
    }

    /**
     * The interval in relative milliseconds before which this peer will
     * attempt to contact the the seed peers.
     *
     * @return The interval in relative milliseconds before which this peer will
     *         attempt to contact the the seed peers.
     */
    public long getSeedRendezvousConnectDelay() {
        return seedRendezvousConnectDelay;
    }

    /**
     * Sets the interval in relative milliseconds before which this peer will
     * attempt to contact the the seed peers.
     *
     * @param newvalue The interval in relative milliseconds before which this peer will
     *                 attempt to contact the the seed peers  or
     *                 <code>-1</code> for the default value.
     */
    public void setSeedRendezvousConnectDelay(long newvalue) {
        if ((-1 != newvalue) && (newvalue < 0)) {
            throw new IllegalArgumentException("Seed Rendezvous Connect Delay must be >= 0");
        }

        seedRendezvousConnectDelay = newvalue;
    }

    /**
     * The interval in relative milliseconds of leases offered by rendezvous
     * peers.
     *
     * @return The interval in relative milliseconds of leases offered by rendezvous
     *         peers.
     */
    public long getLeaseDuration() {
        return leaseDuration;
    }

    /**
     * Sets interval in relative milliseconds of leases to be offered by rendezvous
     * peers.
     *
     * @param newvalue The interval in relative milliseconds of leases to be  offered by rendezvous
     *                 peers  or  <code>-1</code> for the default value.
     */
    public void setLeaseDuration(long newvalue) {
        if ((-1 != newvalue) && (newvalue < 0)) {
            throw new IllegalArgumentException("Lease Duration must be >= 0");
        }

        leaseDuration = newvalue;
    }

    /**
     * The interval in relative milliseconds of leases offered by rendezvous
     * peers.
     *
     * @return The interval in relative milliseconds of leases offered by rendezvous
     *         peers.
     */
    public long getLeaseMargin() {
        return leaseMargin;
    }

    /**
     * Sets interval in relative milliseconds of leases to be offered by rendezvous
     * peers.
     *
     * @param newvalue The interval in relative milliseconds of leases to be  offered by rendezvous
     *                 peers  or
     *                 <code>-1</code> for the default value.
     */
    public void setLeaseMargin(long newvalue) {
        if ((-1 != newvalue) && (newvalue < 0)) {
            throw new IllegalArgumentException("Lease Margin must be >= 0");
        }

        leaseMargin = newvalue;
    }

    /**
     * Return the maximum number of clients.
     *
     * @return The maximum number of clients.
     */
    public int getMaxClients() {
        return maxClients;
    }

    /**
     * Sets he maximum number of clients.
     *
     * @param newvalue The maximum number of clients or
     *                 <code>-1</code> for the default value.
     */
    public void setMaxClients(int newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Max Clients must be > 0");
        }

        maxClients = newvalue;
    }

    /**
     * Returns the maximum TTL for messages propagated by this peer or
     * -1 for the default value.
     *
     * @return the maximum TTL for messages propagated by this peer or
     *         -1 for the default value.
     */
    public int getMaxTTL() {
        return maximumTTL;
    }

    /**
     * Sets the maximum TTL for messages propagated by this peer.
     *
     * @param newvalue the maximum TTL for messages propagated by this peer or
     *                 <code>-1</code> for the default value.
     */
    public void setMaxTTL(int newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Max TTL must be >= 0");
        }

        maximumTTL = newvalue;
    }

    /**
     * Returns the minimum peerview size which the rendezvous service will
     * find sufficient.
     *
     * @return the minimum peerview size.
     */
    public int getMinHappyPeerView() {
        return minHappyPeerView;
    }

    /**
     * Sets the minimum peerview size which the rendezvous service will find
     * sufficient.  If the peerview size is below that threshold, the peerview
     * will more aggressively try to discover additional members. If permitted,
     * the local peer may eventually decide to become a rendezvous in order for
     * the peerview to reach that size.
     *
     * @param newvalue the minimum peerview size, which must be > 0 or
     *                 <code>-1</code> for the default value.
     */
    public void setMinHappyPeerView(int newvalue) {
        if ((-1 != newvalue) && (newvalue <= 0)) {
            throw new IllegalArgumentException("Min Happy Peer View size must be > 0");
        }
        minHappyPeerView = newvalue;
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
