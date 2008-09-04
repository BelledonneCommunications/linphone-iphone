/*
 * Copyright (c) 2002-2004 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util;


import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.ListIterator;
import java.util.Set;

import java.io.IOException;
import java.net.URISyntaxException;
import java.text.MessageFormat;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;
import net.jxta.document.Attribute;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.impl.endpoint.EndpointUtils;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.AccessPointAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.RouteAdvertisement;


/**
 *  A seeding manager that supports both explicit seed peers and loading of
 *  seeds from seeding resources.
 */
public class URISeedingManager extends RdvAdvSeedingManager {
    
    /**
     *  Logger
     */
    private static final transient Logger LOG = Logger.getLogger(URISeedingManager.class.getName());
    
    /**
     *  The minimum amount of time we will wait between attempts to resolve the
     *  seeding URI sources.
     */
    private static final long MINIMUM_SEEDING_REFRESH_INTERVAL = 5 * TimeUtils.AMINUTE;
    
    /**
     *  The standard interval at which we will attempt to refresh from the
     *  seeding URI sources. Also the maximum we will wait between failed
     *  attempts.
     */
    private static final long STANDARD_SEEDING_REFRESH_INTERVAL = 30 * TimeUtils.AMINUTE;
    
    /**
     * Whether we are restricted to using seed rdvs only.
     */
    private boolean allowOnlySeeds = false;
    
    /**
     * These URIs specify location of seed peer lists. The URIs will be resolved
     * via URLConnection and are assumed to refer to plain text lists of
     * absolute URIs or an XML document containing a list of Route Advertisements.
     */
    private final Set<URI> seedingURIs = new HashSet<URI>();
    
    /**
     *  The absolute time in milliseconds after which we will attempt to refresh
     *  the active seeds list using the seeding URIs.
     */
    private long nextSeedingURIrefreshTime = 0;
    
    /**
     *  The number of sequential failures we have encountered while loading
     */
    private int failedSeedingLoads = 0;
    
    /**
     * These are seed peers which were specified as part of the configuration
     * data or programmatically. These seeds are never deleted.
     */
    private final Set<RouteAdvertisement> permanentSeeds = new HashSet<RouteAdvertisement>();
    
    /**
     *  The ranked list of active seed peers. The seed addresses are ranked as
     *  follows :
     *
     *  <ol>
     *      <li>The lists of seed addresses returned by reading from the seeding
     *      addresses in the order they were returned. The seeding addresses are
     *      processed in random order.</li>
     *      <li>The list of permanent seed addresses in random order.</li>
     *  </ol>
     *
     *  Consumers of the seed list should process the list in the order returned
     *  and not request a new list until they have exhausted all entries from
     *  the each returned lists or found an active seed.
     */
    private final List<RouteAdvertisement> activeSeeds = new ArrayList<RouteAdvertisement>();
    
    
    /**
     * Get an instance of URISeedingManager.
     *
     * @param aclLocation The location of the ACL file or {@code null} if no
     * ACL file should be used.
     * @param allowOnlySeeds If {@code true} then the only peers which are part
     * of the seed peer set will be
     */
    public URISeedingManager(URI aclLocation, boolean allowOnlySeeds, PeerGroup group, String serviceName) {
        super(aclLocation, group, serviceName);
        
        this.allowOnlySeeds = allowOnlySeeds;
    }
    
    /**
     * {@inheritDoc}
     */
    public void stop() { 
        super.stop();
    }
    
    /**
     * Adds the specified URI to the list of permanent seeds. Even if
     * {@code allowOnlySeeds} is in effect, this seed may now be used, as if it
     * was part of the initial configuration.
     *
     * @param seed The URI of the seed peer.
     */
    public synchronized void addSeed(URI seed) {
        RouteAdvertisement ra = (RouteAdvertisement) 
                AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());
        AccessPointAdvertisement apa = (AccessPointAdvertisement) 
                AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());
        
        ra.addDestEndpointAddress(new EndpointAddress(seed));
        
        permanentSeeds.add(ra);
        activeSeeds.add(ra);
    }
    
    /**
     * Adds the specified URI to the list of permanent seeds. Even if
     * {@code allowOnlySeeds} is in effect, this seed may now be used, as if it
     * was part of the initial configuration.
     *
     * @param seed The RouteAdvertisement of the seed peer.
     */
    public synchronized void addSeed(RouteAdvertisement seed) {
        permanentSeeds.add(seed.clone());
        activeSeeds.add(seed.clone());
    }
    
    /**
     * Adds the specified URI to the list of seeding URIs.
     *
     * @param seeding The URI of the seeding list.
     */
    public synchronized void addSeedingURI(URI seeding) {
        seedingURIs.add(seeding);
        
        // Reset the refresh timer so that our new seeding URI will get used.
        nextSeedingURIrefreshTime = TimeUtils.timeNow();
    }
    
    /**
     *  {@inheritDoc}
     */
    public synchronized URI[] getActiveSeedURIs() {
        List<URI> result = new ArrayList<URI>();
        
        refreshActiveSeeds();
        
        int eaIndex = 0;
        boolean addedEA;
        
        do {
            addedEA = false;
            
            for (RouteAdvertisement aRA : activeSeeds) {
                List<String> eas = aRA.getDest().getVectorEndpointAddresses();
                
                if (eaIndex < eas.size()) {
                    String anEndpointAddress = eas.get(eaIndex);
                    
                    try {
                        result.add(new URI(anEndpointAddress));
                        addedEA = true;
                    } catch (URISyntaxException failed) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "bad address in route : " + anEndpointAddress, failed);
                        }
                    }
                }
            }
            
            // Next loop we use the next most preferred address.
            eaIndex++;
        } while (addedEA);
        
        // Add more primordial seeds.
        if(!allowOnlySeeds) {
            for(URI eachURI : Arrays.asList(super.getActiveSeedURIs())) {
                if(!result.contains(eachURI)) {
                    result.add(eachURI);
                }
            }
        }
        
        return result.toArray(new URI[result.size()]);
    }
    
    /**
     *  {@inheritDoc}
     */
    public synchronized RouteAdvertisement[] getActiveSeedRoutes() {
        
        refreshActiveSeeds();
        
        List<RouteAdvertisement> result = new ArrayList<RouteAdvertisement>(activeSeeds);
                
        // Add more primordial seeds.
        if(!allowOnlySeeds) {
            for(RouteAdvertisement eachRoute : Arrays.asList(super.getActiveSeedRoutes())) {
                if(!result.contains(eachRoute)) {
                    result.add(eachRoute);
                }
            }
        }
        
        return result.toArray(new RouteAdvertisement[result.size()]);
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public synchronized boolean isAcceptablePeer(PeerAdvertisement peeradv) {
        RouteAdvertisement route = EndpointUtils.extractRouteAdv(peeradv);
        
        boolean acceptable = true;
        
        if (allowOnlySeeds) {
            acceptable = isSeedPeer(route);
        }
        
        if (!acceptable) {
            return false;
        }
        
        if (null != route) {
            return isAcceptablePeer(route);
        } else {
            return acl.getGrantAll();
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public synchronized boolean isAcceptablePeer(RouteAdvertisement radv) {
        boolean acceptable = true;
        
        if (allowOnlySeeds) {
            acceptable = isSeedPeer(radv);
        }
        
        return acceptable && super.isAcceptablePeer(radv);
    }
    
    private void refreshActiveSeeds() {
        if (TimeUtils.timeNow() < nextSeedingURIrefreshTime) {
            return;
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Regenerating active seeds list.");
        }
        
        activeSeeds.clear();
        
        if (!seedingURIs.isEmpty()) {
            List<URI> allSeedingURIs = new ArrayList<URI>(seedingURIs);
            boolean allLoadsFailed = true;
            
            Collections.shuffle(allSeedingURIs);
            
            for (URI aSeedingURI : allSeedingURIs) {
                try {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Loading seeding list from : " + aSeedingURI);
                    }
                    
                    RouteAdvertisement ras[] = loadSeeds(aSeedingURI);
                    
                    for (RouteAdvertisement aRA : Arrays.asList(ras)) {
                        if (!activeSeeds.contains(aRA)) {
                            // Only add non-duplicates.
                            activeSeeds.add(aRA);
                            allLoadsFailed = false;
                        }
                    }
                } catch (IOException failed) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Failed loading seeding list from : " + aSeedingURI);
                    }
                }
            }
            
            if (allLoadsFailed) {
                // Allow for an early reload if we couldn't contact any of the
                // seeding URIS.
                failedSeedingLoads++;
                long nextAttemptInterval = Math.min(MINIMUM_SEEDING_REFRESH_INTERVAL * failedSeedingLoads,
                        STANDARD_SEEDING_REFRESH_INTERVAL);

                nextSeedingURIrefreshTime = TimeUtils.toAbsoluteTimeMillis(nextAttemptInterval);
            } else {
                failedSeedingLoads = 0;
                nextSeedingURIrefreshTime = TimeUtils.toAbsoluteTimeMillis(STANDARD_SEEDING_REFRESH_INTERVAL);
            }
        }
        
        // Add the (shuffled) permanent seeds at the last.
        List<RouteAdvertisement> asList = new ArrayList<RouteAdvertisement>(permanentSeeds);

        Collections.shuffle(asList);
        activeSeeds.addAll(asList);
    }
    
    /**
     * Evaluates if the given route corresponds to one of our seeds. This is
     * to support the allowOnlySeeds flag. The test is not completely foolproof
     * since our list of seeds is just transport addresses. We could be given a
     * pve that exhibits an address that corresponds to one of our seeds but is
     * fake. And we might later succeed in connecting to that peer via one
     * the other, real addresses. As a result, allowOnlySeeds is *not* a 
     * security feature, just a convenience for certain kind of deployments. 
     * The remote peer's certificates should be examined in order to fully
     * establish that it an appropriate peer. 
     */
    private boolean isSeedPeer(RouteAdvertisement route) {
        List<?> addrList = route.getDestEndpointAddresses();
        
        ListIterator eachAddr = addrList.listIterator();
        
        // convert each EndpointAddress to a URI to compare with seedHosts
        while (eachAddr.hasNext()) {
            EndpointAddress anAddr = (EndpointAddress) eachAddr.next();
            
            eachAddr.set(anAddr.toURI());
        }
        
        addrList.retainAll(Arrays.asList(getActiveSeedURIs()));
        
        // What's left is the intersection of activeSeeds and the set of
        // endpoint addresses in the given APA. If it is non-empty, then we
        // accept the route as that of a seed host.
        return (!addrList.isEmpty());
    }
    
    /**
     *  Load a list of seed peer RouteAdvertisements from the specified URI.
     *  <p/>
     *  Two formats are supported:
     *  <dl>
     *      <dt>TEXT</dt>
     *      <dd>A simple UTF-8 or US ASCII text file containing one seed 
     *      endpoint address per line. These entries are converted into very 
     *      simple {@code RouteAdvertisement}s.</dd>
     *      <dt>XML</dt>
     *      <dd>A simple XML file containing a sequence of seed
     *      {@code RouteAdvertisement}s. The seed advertisements may be ordered
     *      or unordered.</dd>
     *  </dl>
     *
     *  @param seedingURI The intended source of the {@code RouteAdvertisement}s.
     *  @return The loaded {@code RouteAdvertisement}s.
     *  @throws IOException Thrown for errors encountered loading the seed
     *  RouteAdvertisements.
     */
    static RouteAdvertisement[] loadSeeds(URI seedingURI) throws IOException {
        boolean isXML;
        URL seedingURL = seedingURI.toURL();
        URLConnection connection = seedingURL.openConnection();
        
        connection.setDoInput(true);
        InputStream is = connection.getInputStream();
        
        // Determine if the input file is an XML document or a plain text list.
        // If it is not XML then we assume it is text.
        String content_type = connection.getContentType();
        MimeMediaType type;
        
        if (null == content_type) {
            // If we couldn't get a content-type from the connection then let's
            // try to get it from the URI path.
            String name = seedingURI.getPath();
            int extIdx = name.lastIndexOf('.');
            int sepIdx = name.lastIndexOf('/');
            
            if ((-1 != extIdx) && (extIdx > sepIdx)) {
                String ext = name.substring(extIdx + 1);

                type = StructuredDocumentFactory.getMimeTypeForFileExtension(ext);
            } else {
                // Type is unknown. :-(
                type = MimeMediaType.AOS;
            }
        } else {
            type = new MimeMediaType(content_type);
        }
        
        isXML = MimeMediaType.XML_DEFAULTENCODING.equalsIngoringParams(type)
                || MimeMediaType.APPLICATION_XML_DEFAULTENCODING.equalsIngoringParams(type);
        
        BufferedReader seeds = new BufferedReader(new InputStreamReader(is));
        
        List<RouteAdvertisement> result = new ArrayList<RouteAdvertisement>();
        
        if (isXML) {
            // Read in XML format seeds. (a list of Route Advertisements)
            XMLDocument xmldoc = (XMLDocument) 
                    StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XML_DEFAULTENCODING, seeds);
            
            Enumeration<XMLElement> eachRA = xmldoc.getChildren(RouteAdvertisement.getAdvertisementType());
            
            while (eachRA.hasMoreElements()) {
                XMLElement anRAElement = eachRA.nextElement();
                RouteAdvertisement ra = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(anRAElement);
                
                result.add(ra);
            }
            
            boolean randomize = true;
            
            Attribute ordered = xmldoc.getAttribute("ordered");

            if (null != ordered) {
                randomize = !Boolean.valueOf(ordered.getValue());
            }
            
            if (randomize) {
                Collections.shuffle(result);
            }
        } else {
            // Read in plain text format seeds. A list of Endpoint Addresses
            while (true) {
                String aSeed = seeds.readLine();
                
                if (null == aSeed) {
                    break;
                }
                
                aSeed = aSeed.trim();
                
                if (0 == aSeed.length()) {
                    continue;
                }
                
                try {
                    URI validation = URI.create(aSeed);
                    EndpointAddress ea = new EndpointAddress(validation.toString());
                    
                    RouteAdvertisement ra = (RouteAdvertisement) AdvertisementFactory.newAdvertisement(
                            RouteAdvertisement.getAdvertisementType());
                    
                    ra.addDestEndpointAddress(ea);
                    
                    // Add the world's most pathetic RouteAdvertisement to the result.
                    result.add(ra);
                } catch (IllegalArgumentException badURI) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "bad URI in seeding list : " + aSeed, badURI);
                    }
                }
            }
        }
        
        is.close();
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("Loaded #{0} seeds from : {1}", result.size(), seedingURI));
        }
        
        return result.toArray(new RouteAdvertisement[result.size()]);
    }
}
