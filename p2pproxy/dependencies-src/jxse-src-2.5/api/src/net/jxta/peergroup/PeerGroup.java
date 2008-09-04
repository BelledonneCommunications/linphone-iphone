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

package net.jxta.peergroup;


import net.jxta.access.AccessService;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.Element;
import net.jxta.endpoint.EndpointService;
import net.jxta.exception.PeerGroupException;
import net.jxta.exception.ProtocolNotSupportedException;
import net.jxta.exception.ServiceNotFoundException;
import net.jxta.id.ID;
import net.jxta.membership.MembershipService;
import net.jxta.peer.PeerID;
import net.jxta.peer.PeerInfoService;
import net.jxta.pipe.PipeService;
import net.jxta.platform.JxtaLoader;
import net.jxta.platform.Module;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.resolver.ResolverService;
import net.jxta.service.Service;

import java.io.IOException;
import java.lang.ref.Reference;
import java.lang.ref.WeakReference;
import java.net.URI;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;


/**
 * Peer groups are formed as a collection of peers that have agreed upon a
 * common set of services. Each peer group is assigned a unique peer group ID
 * and a peer group advertisement. The peer group advertisement contains a
 * ModuleSpecID which refers to a module specification for this peer group.
 * <p/>
 * The peer group specification mandates each of the group services (membership,
 * discovery, resolver, etc). Implementations of that specification are
 * described by ModuleImplAdvertisements which are identified by the group's
 * ModuleSpecID. Implementations are responsible for providing the services mandated
 * by the specification.
 * <p/>
 * The java reference implementation achieves this by loading additional Modules
 * which ModuleSpecIDs are listed by the group implementation advertisement.
 * <p/>
 * In order to fully participate in a group, a peer may need to authenticate
 * with the group using the peer group membership service.
 *
 * @see net.jxta.peergroup.PeerGroupID
 * @see net.jxta.service.Service
 * @see net.jxta.peergroup.PeerGroupFactory
 * @see net.jxta.protocol.PeerGroupAdvertisement
 * @see net.jxta.protocol.ModuleImplAdvertisement
 * @see net.jxta.platform.ModuleSpecID
 * @see net.jxta.platform.ModuleClassID
 */
public interface PeerGroup extends Service {

    /**
     * Look for needed ModuleImplAdvertisement in this group.
     */
    public final static int Here = 0;

    /**
     * Look for needed ModuleImplAdvertisement in the parent group of this group.
     */
    public final static int FromParent = 1;

    /**
     * Look for needed ModuleImplAdvertisement in both this group and its parent.
     */
    public final static int Both = 2;

    /**
     * Default life time for group advertisements in the publisher's cache.
     * (a year)
     */
    // without casting to long we lose precision
    public final static long DEFAULT_LIFETIME = (long) 1000 * (long) 3600 * (long) 24 * 365L;

    /**
     * Default expiration time for discovered group advertisements. (2 weeks)
     */
    // without casting to long we lose precision
    public final static long DEFAULT_EXPIRATION = (long) 1000 * (long) 3600 * (long) 24 * 14L;

    /**
     * Global registry of instantiated peer groups. We allow only a single
     * PeerGroup instance for a specific PeerGroupID within the context of the
     * classloader JXTA is loaded into.
     */
    static class GlobalRegistry {

        private final Map<ID, Reference<PeerGroup>> registry = new HashMap<ID, Reference<PeerGroup>>(8);

        /**
         * Registers a new instance.
         *
         * @param gid the ID of the group of which an instance is being registered.
         * @param pg  the group instance being registered.
         * @return false if the instance could not be registered because there
         *         was already such an instance registered.
         */
        public synchronized boolean registerInstance(PeerGroupID gid, PeerGroup pg) {

            Reference<PeerGroup> ref = registry.get(gid);

            if ((ref != null) && (ref.get() != null)) {
                return false;
            }

            // If the ref is a dead instance, we can also replace it.
            registry.put(gid, new WeakReference<PeerGroup>(pg));
            return true;
        }

        /**
         * Unregisters a group instance (normally because the group is being
         * stopped.
         *
         * @param gid the ID of the group of which an instance is unregistered.
         * @param pg  the group instance itself (serves as a credential).
         * @return false if the group could not be unregistered because no such
         *         registration (exact ID, exact object) was not found.
         */
        public synchronized boolean unRegisterInstance(PeerGroupID gid, PeerGroup pg) {

            Reference<PeerGroup> ref = registry.get(gid);

            if (ref == null) {
                return false;
            }

            PeerGroup found = ref.get();

            if (found == null) {
                // Dead instance. Remove from table.
                registry.remove(gid);
                return false;
            }

            // Note the use of "!=", not "!equals()"
            if (pg != found) {
                return false;
            }

            registry.remove(gid);
            return true;
        }

        /**
         * Returns a running instance of the peergroup with given ID if any
         * exists. The instance should be {@link PeerGroup#unref()}ed when it is
         * no longer needed.
         *
         * @param gid the id of the group of which an instance is wanted.
         * @return the group, or {@code null} if no instance exists.
         */
        public synchronized PeerGroup lookupInstance(PeerGroupID gid) {

            Reference<PeerGroup> ref = registry.get(gid);

            if (ref == null) {
                return null;
            }

            PeerGroup pg = ref.get();

            if (pg == null) {
                // Dead instance. remove from table.
                registry.remove(gid);
                return null;
            }

            // Returns an interface object. Therefore a module that got the
            // peergroup through lookup cannot unregister it if the group
            // protects itself by returning an interface object different from
            // the group object. In general only the group itself can
            // unregister when being torn down. Unregistration will also be
            // automatic if the grp object is GC'ed (the references are weak
            // references).
            return (PeerGroup) pg.getInterface();
        }

        /**
         * Returns a running instance of the peergroup with given ID if any
         * exists.
         *
         * @param gid The id of the group of which an instance is wanted.
         * @return The group, or {@code null} if no instance exists.
         */
        synchronized PeerGroup getInstance(PeerGroupID gid) {

            Reference<PeerGroup> ref = registry.get(gid);

            if (ref == null) {
                return null;
            }

            PeerGroup pg = ref.get();

            if (pg == null) {
                // Dead instance. remove from table.
                registry.remove(gid);
                return null;
            }

            return pg;
        }

        /**
         * Returns {@code true} if there is a registered peergroup of the
         * specified ID.
         *
         * @param gid the id of the group of which an instance is wanted.
         * @return {@code} true if the peergroup is currently registered
         *         otherwise false;
         */
        public synchronized boolean registeredInstance(PeerGroupID gid) {

            Reference<PeerGroup> ref = registry.get(gid);

            if (ref == null) {
                return false;
            }

            PeerGroup pg = ref.get();

            if (pg == null) {
                // Dead instance. remove from table.
                registry.remove(gid);
                return false;
            }

            return true;
        }
    }

    /**
     * Well known classes for the basic services.
     *
     * <p/>FIXME: we should make a "well-known ID" encoding implementation that
     * has its own little name space of human readable names...later.
     * To keep their string representation shorter, we put our small spec
     * or role pseudo unique ID at the front of the second UUID string.
     * Base classes do not need an explicit second UUID string because it is
     * all 0.
     *
     * <p/>The type is always the last two characters, no-matter the total length.
     */

    /**
     * Prefix string for all of the Well Known IDs declared in this interface.
     */
    static final String WK_ID_PREFIX = ID.URIEncodingName + ":" + ID.URNNamespace + ":uuid-DeadBeefDeafBabaFeedBabe";

    /**
     * Well known module class identifier: peer group
     */
    public final static ModuleClassID peerGroupClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000105"));

    /**
     * Well known module class identifier: resolver service
     */
    public final static ModuleClassID resolverClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000205"));

    /**
     * Well known module class identifier: discovery service
     */
    public final static ModuleClassID discoveryClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000305"));

    /**
     * Well known module class identifier: pipe service
     */
    public final static ModuleClassID pipeClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000405"));

    /**
     * Well known module class identifier: membership service
     */
    public final static ModuleClassID membershipClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000505"));

    /**
     * Well known module class identifier: rendezvous service
     */
    public final static ModuleClassID rendezvousClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000605"));

    /**
     * Well known module class identifier: peerinfo service
     */
    public final static ModuleClassID peerinfoClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000705"));

    /**
     * Well known module class identifier: endpoint service
     */
    public final static ModuleClassID endpointClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000805"));

    // FIXME: EndpointProtocols should probably all be of the same class
    // and of different specs and roles... But we'll take a shortcut for now.

    /**
     * Well known module class identifier: tcp protocol
     */
    public final static ModuleClassID tcpProtoClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000905"));

    /**
     * Well known module class identifier: http protocol
     */
    public final static ModuleClassID httpProtoClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000A05"));

    /**
     * Well known module class identifier: router protocol
     */
    public final static ModuleClassID routerProtoClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000B05"));

    /**
     * Well known module class identifier: application
     */
    public final static ModuleClassID applicationClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000C05"));

    /**
     * Well known module class identifier: tlsProtocol
     */
    public final static ModuleClassID tlsProtoClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000D05"));

    /**
     * Well known module class identifier: ProxyService
     */
    public final static ModuleClassID proxyClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000E05"));

    /**
     * Well known module class identifier: RelayProtocol
     */
    public final static ModuleClassID relayProtoClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000000F05"));

    /**
     * Well known module class identifier: AccessService
     */
    public final static ModuleClassID accessClassID = 
            ModuleClassID.create(URI.create(WK_ID_PREFIX + "0000001005"));

    /**
     * Well known group specification identifier: the platform
     */
    public final static ModuleSpecID refPlatformSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000010106"));

    /**
     * Well known group specification identifier: the Network Peer Group
     */
    public final static ModuleSpecID refNetPeerGroupSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000010206"));

    /**
     * Well known service specification identifier: the standard resolver
     */
    public final static ModuleSpecID refResolverSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000020106"));

    /**
     * Well known service specification identifier: the standard discovery
     */
    public final static ModuleSpecID refDiscoverySpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000030106"));

    /**
     * Well known service specification identifier: the standard pipe service
     */
    public final static ModuleSpecID refPipeSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000040106"));

    /**
     * Well known service specification identifier: the standard membership
     */
    public final static ModuleSpecID refMembershipSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000050106"));

    /**
     * Well known service specification identifier: the standard rendezvous
     */
    public final static ModuleSpecID refRendezvousSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000060106"));

    /**
     * Well known service specification identifier: the standard peerinfo
     */
    public final static ModuleSpecID refPeerinfoSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000070106"));

    /**
     * Well known service specification identifier: the standard endpoint
     */
    public final static ModuleSpecID refEndpointSpecID =
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000080106"));

    /**
     * Well known endpoint protocol specification identifier: the standard
     * tcp endpoint protocol
     */
    public final static ModuleSpecID refTcpProtoSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000090106"));

    /**
     * Well known endpoint protocol specification identifier: the standard
     * http endpoint protocol
     */
    public final static ModuleSpecID refHttpProtoSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "0000000A0106"));

    /**
     * Well known endpoint protocol specification identifier: the standard
     * router
     */
    public final static ModuleSpecID refRouterProtoSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "0000000B0106"));

    /**
     * Well known endpoint protocol specification identifier: the standard
     * tls endpoint protocol
     */
    public final static ModuleSpecID refTlsProtoSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "0000000D0106"));

    /**
     * Well known group specification identifier: an all purpose peer group
     * specification. The java reference implementation implements it with
     * the StdPeerGroup class and all the standard platform services and no
     * endpoint protocols.
     */
    public final static ModuleSpecID allPurposePeerGroupSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000010306"));

    /**
     * Well known application: the shell
     */
    public final static ModuleSpecID refShellSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "0000000C0206"));

    /**
     * Well known application: the Proxy
     */
    public final static ModuleSpecID refProxySpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "0000000E0106"));

    /**
     * Well known endpoint protocol specification identifier: the standard
     * relay endpoint protocol
     */
    public final static ModuleSpecID refRelayProtoSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "0000000F0106"));

    /**
     * Well known access specification identifier: the standard
     * access service
     */
    public final static ModuleSpecID refAccessSpecID = 
            ModuleSpecID.create(URI.create(WK_ID_PREFIX + "000000100106"));

    /**
     * The global registry of Peer Group instances. Operations involving the
     * instantiation or orderly shutdown of Peer Groups should synchronize upon
     * this object.
     */
    final static GlobalRegistry globalRegistry = new GlobalRegistry();

    /**
     * Returns the Thread Group in which threads for this peer group will live.
     * This is currently used only for debugging purposes so that the source of
     * a thread can be determined.
     *
     * @return ThreadGroup
     */
    public ThreadGroup getHomeThreadGroup();

    /**
     * Returns the class loader for this group.
     *
     * @return JxtaLoader The JXTA Class loader used by this group.
     */
    public JxtaLoader getLoader();

    /**
     * Returns the whether the group member is a Rendezvous peer for the group.
     *
     * @return boolean true if the peer is a rendezvous for the group.
     */
    public boolean isRendezvous();

    /**
     * Return the PeerGroupAdvertisement for this group.
     *
     * @return PeerGroupAdvertisement this Group's advertisement.
     */
    public PeerGroupAdvertisement getPeerGroupAdvertisement();

    /**
     * Return the PeerAdvertisement of the local Peer within this Peer Group.
     *
     * @return the PeerAdvertisement of the local Peer within this Peer Group.
     */
    public PeerAdvertisement getPeerAdvertisement();

    /**
     * Lookup for a service by name.
     *
     * @param name the service identifier.
     * @return Service, the Service registered by that name
     * @throws ServiceNotFoundException could not find the service requested
     */
    public Service lookupService(ID name) throws ServiceNotFoundException;

    /**
     * Lookup for a service by class ID and index in a map.
     * <p/>
     * More than one service in a group may be of a given ModuleClass.
     * However each of them has a unique assigned ID which serves as the
     * index in the map of services. In most cases, there is only one
     * service of each given Module Class, and the ID of that Module Class
     * is the assigned ID. Otherwise, the group may have a list of existing
     * assigned ID per base class. This routine may be used to retrieve
     * services of the given Module Class and index in that list.
     * In the absence of a mapping, index 0 is still valid and
     * corresponds to the service which assigned ID is exactly the
     * given ID.
     * Group objects with a map are normally wrappers tailored
     * specially by the loader of a module (often the group itself) in order
     * to provide a map appropriate for that module. Modules that do not use
     * more than one service of a given base class normally never need to call
     * this method; lookupService(ID) is equivalent to lookupService(ID, 0)
     * and will transparently remap index 0 to whatever the group's
     * structure defines as the default for the invoking service.
     * <p/>
     * Note: traditionally, the given ID is expected to be a base Module
     * Class ID, and the assigned ID of a Module is a Class ID of the
     * same base class with a role suffix to make it unique. If the given
     * ID already contains a role suffix, there may exist an entry for
     * it in the map anyway, if not (which is the expected use pattern),
     * then only index 0 exists and the given ID is used whole and
     * untranslated.
     *
     * @param name      the service identifier
     * @param roleIndex the index in the list of assigned IDs that match
     *                  that identifier.
     * @return Service, the corresponding Service
     * @throws ServiceNotFoundException Could not find the service requested.
     * @since JXTA 2.3.1
     */
    public Service lookupService(ID name, int roleIndex) throws ServiceNotFoundException;

    /**
     * Returns the map of the assigned IDs currently associated with the given
     * ModuleClassID by this PeerGroup object. The IDs are returned in the order
     * of their index in the map. So the first ID returned will be identical to
     * what would be returned by the lookup method for the given ID and index 0.
     * If there is no explicit such map, this method will return a singleton
     * containing the given ID as this is the default mapping.  There is no
     * guarantee that any of the returned IDs correspond to an actually
     * registered service. This method only maps IDs.
     *
     * @param name The ModuleClassID for which the map is desired.
     * @return Iterator An iterator on a collection of the IDs to which the given ID maps.
     * @since JXTA 2.3.1
     */
    public Iterator getRoleMap(ID name);

    /**
     * Return true if the provided compatibility statement is compatible with this group.
     *
     * @param compat compatibility element
     * @return boolean True if the statement is compatible.
     */
    public boolean compatible(Element compat);

    /**
     * Load a Module from a ModuleImplAdv.
     * <p/>
     * Compatibility is checked and load is attempted. If compatible and
     * loaded successfully, the resulting Module is initialized and returned.
     * In most cases the other loadModule() method should be preferred, since
     * unlike this one, it will seek many compatible implementation
     * advertisements and try them all until one works. The home group of the new
     * module (its' parent group if the new Module is a group) will be this group.
     *
     * @param assignedID Id to be assigned to that module (usually its ClassID).
     * @param impl       An implementation advertisement for that module.
     * @return Module the module loaded and initialized.
     * @throws ProtocolNotSupportedException The implementation described by the
     *                                       advertisement is incompatible with this peer. The module cannot be loaded.
     * @throws PeerGroupException            The module could not be loaded or initialized
     */
    public Module loadModule(ID assignedID, Advertisement impl) throws ProtocolNotSupportedException, PeerGroupException;

    /**
     * Load a module from a ModuleSpecID
     * <p/>
     * Advertisement is sought, compatibility is checked on all candidates
     * and load is attempted. The first one that is compatible and loads
     * successfully is initialized and returned.
     *
     * @param assignedID Id to be assigned to that module (usually its ClassID).
     * @param specID     The specID of this module.
     * @param where      May be one of: {@code Here}, {@code FromParent}, or
     *                   {@code Both}, meaning that the implementation advertisement will be
     *                   searched in this group, its parent or both. As a general guideline, the
     *                   implementation advertisements of a group should be searched in its
     *                   prospective parent (that is {@code Here}), the implementation
     *                   advertisements of a group standard service should be searched in the same
     *                   group than where this group's advertisement was found (that is,
     *                   {@code FromParent}), while applications may be sought more freely
     *                   ({@code Both}).
     * @return Module the new module, or null if no usable implementation was
     *         found.
     */
    public Module loadModule(ID assignedID, ModuleSpecID specID, int where);

    /**
     * Publish this group's Peer Group Advertisement. The Advertisement will be
     * published using the parent peer group's Discovery service.
     * <p/>
     * Calling this method is only useful if the group is being created
     * from scratch and the PeerGroup advertisement has not been
     * created beforehand. In such a case, the group has never been named or
     * described. Therefore this information has to be supplied here.
     *
     * @param name        The name of this group.
     * @param description The description of this group.
     * @throws IOException The publication could not be accomplished
     *                     because of a network or storage failure.
     */
    public void publishGroup(String name, String description) throws IOException;

    /*
     * Valuable application helpers: Various methods to instantiate
     * groups.
     */

    /**
     * Instantiate a peer group from the provided advertisement. This peer
     * group will be the parent of the newly instantiated peer group.
     * <p/>
     * The pgAdv itself may be all new and unpublished. Therefore, the two
     * typical uses of this routine are:
     * <p/>
     * <ul>
     * <li>Creating an all new group with a new ID while using an existing
     * and published implementation. (Possibly a new one published for
     * that purpose). The information should first be gathered in a new
     * PeerGroupAdvertisement which is then passed to this method.</li>
     * <p/>
     * <li>Instantiating a group which advertisement has already been
     * discovered (therefore there is no need to find it by groupID
     * again).</li>
     * </ul>
     *
     * @param pgAdv The advertisement for the group to be instantiated.
     * @return PeerGroup the initialized (but not started) peergroup.
     * @throws PeerGroupException For problems instantiating the peer group.
     */
    public PeerGroup newGroup(Advertisement pgAdv) throws PeerGroupException;

    /**
     * Instantiates a peer group from its elementary pieces
     * and publishes the corresponding PeerGroupAdvertisement.
     * The pieces are: the groups implementation adv, the group id,
     * the name and description.
     * <p/>
     * The typical use of this routine is creating a whole new group based
     * on a newly created and possibly unpublished implementation adv.
     * <p/>
     * This is a convenience method equivalent to either:
     * <p/>
     * <pre>
     * newGrp = thisGroup.loadModule(gid, impl);
     * newGrp.publishGroup(name, description);
     * </pre>
     * or, but only if the implementation advertisement has been published:
     * <p/>
     * <pre>
     * newPGAdv = AdvertisementFactory.newAdvertisement(
     *                 PeerGroupAdvertisement.getAdvertisementType());
     * newPGAdv.setPeerGroupID(gid);
     * newPGAdv.setModuleSpecID(impl.getModuleSpecID());
     * newPGAdv.setName(name);
     * newPGAdv.setDescription(description);
     * newGrp = thisGroup.newGroup(newPGAdv);
     * </pre>
     *
     * @param gid         The ID of that group. If <code>null</code> then a new group ID
     *                    will be chosen.
     * @param impl        The advertisement of the implementation to be used.
     * @param name        The name of the group.
     * @param description A description of this group.
     * @return PeerGroup the initialized (but not started) peergroup.
     * @throws PeerGroupException Thrown if the group could not be instantiated.
     */
    public PeerGroup newGroup(PeerGroupID gid, Advertisement impl, String name, String description) throws PeerGroupException;

    /**
     * Instantiate a group from its Peer Group ID only. Use this when using a
     * group that has already been published and discovered.
     * <p/>
     * The typical uses of this routine are therefore:
     * <p/>
     * <ul>
     * <li>Instantiating a peer group which is assumed to exist and whose Peer
     * Group ID is already known.</li>
     * <p/>
     * <li>Creating a new peer group instance using an already published
     * Group advertisement, typically published for that purpose. All other
     * referenced advertisements must also be available.</li>
     * </ul>
     * <p/>
     * To create a group from a known implAdv, just use
     * {@link #loadModule(ID,Advertisement)} or even:<p>
     * <p/>
     * <code>
     * grp = new GroupSubClass();
     * grp.init(parentGroup, gid, impladv);
     * </code>
     * <p/>
     * then, <strong>REMEMBER TO PUBLISH THE GROUP IF IT IS ALL NEW.</strong>
     *
     * @param gid the groupID.
     * @return PeerGroup the initialized (but not started) peergroup.
     * @throws PeerGroupException Thrown if the group could not be instantiated.
     */
    public PeerGroup newGroup(PeerGroupID gid) throws PeerGroupException;

    /*
     * Shortcuts to the well-known services, in order to avoid calls to
     * {@link #lookupService(ID)}.
     */

    /**
     * Return the Rendezvous Service for this Peer Group. This service is
     * optional and may not be present in all groups.
     *
     * @return The Rendezvous Service for this Peer Group or <code>null</code>
     *         if there is no Rendezvous Service in this Peer Group.
     */
    public RendezVousService getRendezVousService();

    /**
     * Return the Endpoint Service for this Peer Group. This service is
     * present in every Peer Group.
     *
     * @return EndpointService The Endpoint Service for this Peer Group.
     */
    public EndpointService getEndpointService();

    /**
     * Return the Resolver Service for this Peer Group. This service is
     * present in every Peer Group.
     *
     * @return ResolverService The Resolver Service for this Peer Group.
     */
    public ResolverService getResolverService();

    /**
     * Return the Discovery Service for this Peer Group.
     *
     * @return The Discovery Service for this Peer Group or <code>null</code>
     *         if there is no PeerInfo Service in this Peer Group.
     */
    public DiscoveryService getDiscoveryService();

    /**
     * Return the PeerInfo Service for this Peer Group.
     *
     * @return The PeerInfo Service for this Peer Group or <code>null</code>
     *         if there is no PeerInfo Service in this Peer Group.
     */
    public PeerInfoService getPeerInfoService();

    /**
     * Return the Membership Service for this Peer Group. This service is
     * present in every Peer Group.
     *
     * @return MembershipService The Membership Service for this Peer Group.
     */
    public MembershipService getMembershipService();

    /**
     * Return the Pipe Service for this Peer Group.
     *
     * @return The Pipe Service for this Peer Group or <code>null</code> if
     *         there is no Pipe Service in this Peer Group.
     */
    public PipeService getPipeService();

    /**
     * Return the Access Service for this Peer Group. This service is
     * present in every Peer Group.
     *
     * @return AccessService The Access Service for this Peer Group.
     * @since JXTA 2.1
     */
    public AccessService getAccessService();

    // A few convenience methods. This information is available from the peer and peergroup advertisement.

    /**
     * Return the Peer Group ID of this Peer Group.
     *
     * @return PeerGroupId The Peer Group ID of this Peer Group.
     */
    public PeerGroupID getPeerGroupID();

    /**
     * Return the Peer ID by which this Peer is known within this Peer Group.
     *
     * @return the Peer ID by which this Peer is known within this Peer Group.
     */
    public PeerID getPeerID();

    /**
     * Return the Name of this group. This name is not canonical, meaning that
     * there may be other groups with the same name.
     *
     * @return String This groups's name or <code>null</code>  if no name was
     *         specified.
     */
    public String getPeerGroupName();

    /**
     * Return the name of the local peer within this group. This name is not
     * canonical, meaning that there may be other peers with the same name.
     *
     * @return String This peer's name or <code>null</code> if no name was
     *         specified.
     */
    public String getPeerName();

    /**
     * Returns the config advertisement for this peer in this group (if any).
     *
     * @return The advertisement or <code>null</code> if none is available.
     */
    public ConfigParams getConfigAdvertisement();

    /**
     * Get an all purpose peerGroup ModuleImplAdvertisement that is compatible
     * with this group. This impl adv can be used to create any group that
     * relies only on the standard services. Or to derive other impl advs, using
     * this impl advertisement as a basis.
     * <p/>
     * This defines a peergroup implementation that can be used for
     * many purposes, and from which one may derive slightly different
     * peergroup implementations.
     * <p/>
     * This definition is always the same and has a well known ModuleSpecID.
     * It includes the basic service, no protocols and the shell for main
     * application.
     * <p/>
     * The user must remember to change the specID if the set of services
     * protocols or applications is altered before use.
     *
     * @return ModuleImplAdvertisement The new peergroup impl adv.
     * @throws Exception if an error occurs while creating the implementation advertisement
     */
    public ModuleImplAdvertisement getAllPurposePeerGroupImplAdvertisement() throws Exception;

    /**
     * Explicitly notifies a group interface that it will no-longer be
     * used (similar to dispose). Does nothing to a real group object,
     * only has an effect on a group interface.
     */
    public void unref();

    /**
     * Returns a weak interface object that represents this
     * group.
     * <p/>
     * A weak interface object has no life-cycle privileges over
     * the group that it represents and therefore its users have
     * no accountability. A weak interface object is safe to
     * give away but holds no promise of sustained validity.
     * <p/>
     * Whatever code gave away a weak interface object retains
     * the power of terminating the group object from which it
     * was obtained, thereby making the weak interface
     * object invalid.
     * <p/>
     * A weak interface object is immutable; its unref and stopApp
     * methods do nothing. Its validity is exactly that of the
     * group or interface object from which it was obtained.
     * <p/>
     * A weak interface object can be obtained from an interface
     * object, or from a real group object, or from a weak interface
     * object. In the later case, the object returned may be
     * the original weak interface object since such objects
     * are immutable.
     * <p/>
     * Whatever code obtains a weak interface object from a group object
     * or regular interface object, remains entirely liable for invoking unref
     * on the initial object before discarding it. Giving away a weak interface
     * object is not equivalent to transferring ownership of the original.
     *
     * @return PeerGroup A weak interface object that represents this
     *         PeerGroup object.
     * @since JXTA 2.2
     */
    public PeerGroup getWeakInterface();

    /**
     * Returns the parent group of this group. Not all groups have parents and
     * some implementations may not reveal their parents.
     *
     * @return PeerGroup the parent group or <code>null</code> if a parent group
     *         if not available.
     * @since JXTA 2.3
     */
    public PeerGroup getParentGroup();

    /**
     * Returns the location of the parent of all items that this peer group is
     * using for persistently storing its preferences, cache, persistent store,
     * properties, etc. May be {@code null} if the peergroup has no defined
     * location for storing persistent data.
     *
     * @return The location of the parent of all persistent items stored by
     *         this peer group.
     * @since JXTA 2.3.7
     */
    public URI getStoreHome();    
}
