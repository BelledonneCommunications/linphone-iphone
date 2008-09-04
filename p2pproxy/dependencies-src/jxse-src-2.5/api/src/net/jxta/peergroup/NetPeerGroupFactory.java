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


import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLElement;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.ModuleImplAdvertisement;

import net.jxta.impl.protocol.PeerGroupConfigAdv;
import net.jxta.impl.peergroup.GenericPeerGroup;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URI;
import java.util.MissingResourceException;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * A factory for instantiating a Network Peer Group instances. The Network Peer
 * Group is the base peer group for applications and services within the JXTA
 * network. Most applications and services will instantiate their own peer
 * groups using the Network Peer Group as a base.
 * <p/>
 * A non-default configuration of <em>The Network Peer Group</em> may be
 * set-up by the administrator in charge of the network domain inside which the
 * peer is starting. <em>The Network Peer Group</em> may be discovered via the
 * JXTA Discovery protocol. Many such groups may be configured by an
 * administrator.
 *
 * @since JXTA JSE 2.4
 *
 * @see net.jxta.peergroup.PeerGroup
 * @see net.jxta.peergroup.WorldPeerGroupFactory
 */
public final class NetPeerGroupFactory {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(NetPeerGroupFactory.class.getName());

    /**
     * Our strong reference to the net peer group.
     */
    private final PeerGroup net;

    /**
     * Instantiates the Net Peer Group using the ConfigParams found in the
     * directory specified by the {@code JXTA_HOME} system property or the
     * "{@code .jxta/}" directory if {@code JXTA_HOME} is not defined.
     * <p/>
     * This constructor is provided primarily for backwards compatibility.
     * Though not deprecated this method should be considered as sample code
     * only and the other constructors should be used whenever possible.
     *
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     */
    public NetPeerGroupFactory() throws PeerGroupException {
        WorldPeerGroupFactory world = new WorldPeerGroupFactory();
        PeerGroup worldGroup = world.getInterface();
        NetGroupTunables tunables;

        try {
            ConfigParams cp = worldGroup.getConfigAdvertisement();
            PeerGroupConfigAdv netGroupConfig = (PeerGroupConfigAdv) cp.getSvcConfigAdvertisement(PeerGroup.peerGroupClassID);
            
            if (null == netGroupConfig) {
                tunables = new NetGroupTunables(ResourceBundle.getBundle("net.jxta.impl.config"), new NetGroupTunables());
                // load overides from "${JXTA_HOME}config.properties".
                URI storeHome = worldGroup.getStoreHome();

                if (null != storeHome) {
                    try {
                        File configProperties = new File(new File(storeHome), "config.properties");
                        ResourceBundle rsrcs = new PropertyResourceBundle(new FileInputStream(configProperties));

                        tunables = new NetGroupTunables(rsrcs, tunables);
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Loaded defaults from " + rsrcs);
                        }
                    } catch (MissingResourceException ignored) {
                        // ingnored
                    } catch (IOException ignored) {
                        // ingnored
                    }
                }
            } else {
                tunables = new NetGroupTunables(netGroupConfig.getPeerGroupID(), netGroupConfig.getName(), netGroupConfig.getDesc());
            }
            
            net = newNetPeerGroup(worldGroup, null, tunables.id, tunables.name, tunables.desc, null);
        } finally {
            worldGroup.unref();
        }
    }

    /**
     * Constructs a Net Peer Group using the specified parent peer group. This
     * is the preferred constructor for constructing a Net Peer Group using the
     * default configuration. The resulting Net Peer Group instance will use
     * the default ID, Name and Description.
     *
     * @param parentGroup The Peer Group which will be the parent of the newly
     * created net peer group. This should normally be the World Peer Group.
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     */
    public NetPeerGroupFactory(PeerGroup parentGroup) throws PeerGroupException {
        ConfigParams cp = parentGroup.getConfigAdvertisement();
        PeerGroupConfigAdv netGroupConfig = (PeerGroupConfigAdv) cp.getSvcConfigAdvertisement(PeerGroup.peerGroupClassID);
        NetGroupTunables tunables;

        if (null == netGroupConfig) {
            tunables = new NetGroupTunables(ResourceBundle.getBundle("net.jxta.impl.config"), new NetGroupTunables());
        } else {
            tunables = new NetGroupTunables(netGroupConfig.getPeerGroupID(), netGroupConfig.getName(), netGroupConfig.getDesc());
        }

        net = newNetPeerGroup(parentGroup, null, tunables.id, tunables.name, tunables.desc, null);
    }
    
    /**
     * Constructs a Net Peer Group and the World Peer Group using the
     * configuration specified by the provided ConfigParams and using the
     * specified storeHome location for persistence. The resulting Net Peer
     * Group instance will use the default ID, Name and Description.
     *
     * @param config The configuration to use for the newly created World Peer
     * Group and Net Peer Groups.
     * @param storeHome The optional location that the World Peer Group, the
     * Net Peer Group and its' services should use for storing persistent and
     * transient information. May be {@code null} if the World Peer Group is
     * not provided a persistent store (though this not currently supported).
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     */
    public NetPeerGroupFactory(ConfigParams config, URI storeHome) throws PeerGroupException {
        WorldPeerGroupFactory world = new WorldPeerGroupFactory(config, storeHome);
        PeerGroup worldGroup = world.getInterface();

        try {
            PeerGroupConfigAdv netGroupConfig = (PeerGroupConfigAdv) config.getSvcConfigAdvertisement(PeerGroup.peerGroupClassID);
            NetGroupTunables tunables;

            if (null == netGroupConfig) {
                tunables = new NetGroupTunables(ResourceBundle.getBundle("net.jxta.impl.config"), new NetGroupTunables());
            } else {
                tunables = new NetGroupTunables(netGroupConfig.getPeerGroupID(), netGroupConfig.getName(), netGroupConfig.getDesc());
            }
            
            net = newNetPeerGroup(worldGroup, config, tunables.id, tunables.name, tunables.desc, null);
        } finally {
            worldGroup.unref();
        }
    }

    /**
     * Constructs a Net Peer Group and the World Peer Group using the
     * configuration specified by the provided ConfigParams and using the
     * specified storeHome location for persistence. The resulting Net Peer
     * Group instance will use the group information provided in the
     * <p/>
     * This constructor is provided in anticipation of other improvements
     * to the peer group instantiation process. Currently it has some
     * unreasonable limitations which keep it from being very useful. In a
     * future release it will be improved.
     *
     * @param config The configuration to use for the newly created World Peer
     * Group and Net Peer Groups.
     * @param storeHome The optional location that the World Peer Group, the
     * Net Peer Group and its' services should use for storing persistent and
     * transient information. May be {@code null} if the World Peer Group is
     * not provided a persistent store (though this not currently supported).
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     * @param parentGroup the parent peer group
     */
    public NetPeerGroupFactory(PeerGroup parentGroup, ConfigParams config, URI storeHome) throws PeerGroupException {

        if (config != parentGroup.getConfigAdvertisement()) {
            throw new IllegalArgumentException("This constructor cannot currently accept group parameters different than the parent group");
        }

        if (null == storeHome) {
            if (null != parentGroup.getStoreHome()) {
                throw new IllegalArgumentException("This constructor cannot currently accept a different store location than the parent group");
            }
        } else {
            if (!storeHome.equals(parentGroup.getStoreHome())) {
                throw new IllegalArgumentException("This constructor cannot currently accept a different store location than the parent group");
            }
        }

        ConfigParams cp = parentGroup.getConfigAdvertisement();
        PeerGroupConfigAdv netGroupConfig = (PeerGroupConfigAdv) cp.getSvcConfigAdvertisement(PeerGroup.peerGroupClassID);
        NetGroupTunables tunables;

        if (null == netGroupConfig) {
            tunables = new NetGroupTunables(ResourceBundle.getBundle("net.jxta.impl.config"), new NetGroupTunables());
        } else {
            tunables = new NetGroupTunables(netGroupConfig.getPeerGroupID(), netGroupConfig.getName(), netGroupConfig.getDesc());
        }
        
        net = newNetPeerGroup(parentGroup, config, tunables.id, tunables.name, tunables.desc, null);
    }

    /**
     * Constructs a Net Peer Group and the World Peer Group using the
     * configuration specified by the provided ConfigParams and using the
     * specified storeHome location for persistence.
     *
     * @deprecated With the addition of support for {@code PeerGroupConfigAdv}
     * this constructor is being deprecated as the precedence of settings is
     * ambiguous.
     *
     * @param config    The configuration to use for the newly created World Peer
     * Group and Net Peer Groups.
     * @param storeHome The optional location that the World Peer Group, the
     * Net Peer Group and its' services should use for storing persistent and
     * transient information. May be {@code null} if the World Peer Group is
     * not provided a persistent store (though this not currently supported).
     * @param id        The PeerGroupID which will be used for the new Net Peer Group
     * instance.
     * @param name      The name which will be used for the new Net Peer Group
     * instance.
     * @param desc      The description which will be used for the new Net Peer Group
     * instance. You can construct an {@code XMLDocument} from a {@code String}
     * via :
     * <p/><pre>
     *     XMLDocument asDoc = StructuredDocumentFactory.newStructuredDocument( MimeMediaType.XMLUTF8, "desc", asString );
     * </pre>
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     */
    @Deprecated
    public NetPeerGroupFactory(ConfigParams config, URI storeHome, ID id, String name, XMLElement desc) throws PeerGroupException {
        WorldPeerGroupFactory world = new WorldPeerGroupFactory(config, storeHome);
        PeerGroup worldGroup = world.getInterface();
        
        try {
            net = newNetPeerGroup(worldGroup, config, id, name, desc, null);
        } finally {
            worldGroup.unref();
        }
    }

    /**
     * Constructs a Net Peer Group instance using the specified parent peer
     * group (normally the World Peer Group). This is the preferred constructor
     * for constructing a private Net Peer Group.
     *
     * @deprecated With the addition of support for {@code PeerGroupConfigAdv}
     * this constructor is being deprecated as the precedence of settings is
     * ambiguous.
     *
     * @param parentGroup The Peer Group which will be the parent of the
     * newly created net peer group. This should normally be the World Peer
     * Group.
     * @param id The PeerGroupID which will be used for the new Net Peer Group
     * instance.
     * @param name The name which will be used for the new Net Peer Group
     * instance.
     * @param desc The description which will be used for the new Net Peer Group
     * instance. You can construct an {@code XMLDocument} from a {@code String}
     * via :
     * <p/><pre>
     *     XMLDocument asDoc = StructuredDocumentFactory.newStructuredDocument( MimeMediaType.XMLUTF8, "desc", asString );
     * </pre>
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     */
    @Deprecated
    public NetPeerGroupFactory(PeerGroup parentGroup, ID id, String name, XMLElement desc) throws PeerGroupException {
        net = newNetPeerGroup(parentGroup, null, id, name, desc, null);
    }

    /**
     * Constructs a Net Peer Group instance using the specified parent peer
     * group (normally the World Peer Group). This is the preferred constructor
     * for constructing a private Net Peer Group with a specific implementation.
     *
     * @deprecated With the addition of support for {@code PeerGroupConfigAdv}
     * this constructor is being deprecated as the precedence of settings is
     * ambiguous.
     *
     * @param parentGroup The Peer Group which will be the parent of the newly
     * created net peer group. This should normally be the World Peer
     *                      
     * @param id The PeerGroupID which will be used for the new Net Peer Group
     * instance.
     * @param name The name which will be used for the new Net Peer Group
     * instance.
     * @param desc The description which will be used for the new Net Peer Group
     * instance. You can construct an {@code XMLDocument} from a {@code String}
     * via :
     * <p/><pre>
     *     XMLDocument asDoc = StructuredDocumentFactory.newStructuredDocument( MimeMediaType.XMLUTF8, "desc", asString );
     * </pre>
     * @param moduleImplAdv The Module Impl Advertisement for the new Net Peer
     * Group instance.
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     */
    @Deprecated
    public NetPeerGroupFactory(PeerGroup parentGroup, ID id, String name, XMLElement desc, ModuleImplAdvertisement moduleImplAdv) throws PeerGroupException {
        net = newNetPeerGroup(parentGroup, null, id, name, desc, moduleImplAdv);
    }

    /**
     * Constructs a Net Peer Group instance using the specified parent peer
     * group (normally the World Peer Group). This is the preferred constructor
     * for constructing a Net Peer Group with a specific implementation.
     *
     * @param parentGroup The Peer Group which will be the parent of the
     * newly created net peer group. This should normally be the World Peer
     * Group.
     * @param config The configuration parameters for the newly created Net Peer
     * Group instance.
     * @param moduleImplAdv The Module Impl Advertisement for the new Net Peer
     * Group instance.
     * @throws PeerGroupException Thrown for problems constructing the Net Peer
     * Group.
     */
    public NetPeerGroupFactory(PeerGroup parentGroup, ConfigParams config, ModuleImplAdvertisement moduleImplAdv) throws PeerGroupException {
        PeerGroupConfigAdv netGroupConfig = (PeerGroupConfigAdv) config.getSvcConfigAdvertisement(PeerGroup.peerGroupClassID);
        NetGroupTunables tunables;

        if (null == netGroupConfig) {
            tunables = new NetGroupTunables(ResourceBundle.getBundle("net.jxta.impl.config"), new NetGroupTunables());
        } else {
            tunables = new NetGroupTunables(netGroupConfig.getPeerGroupID(), netGroupConfig.getName(), netGroupConfig.getDesc());
        }

        net = newNetPeerGroup(parentGroup, config, tunables.id, tunables.name, tunables.desc, moduleImplAdv);
    }
    
    /**
     * Returns a strong (reference counted) interface object for the Net Peer
     * Group instance. This reference should be explicitly unreferenced when it
     * is no longer needed.
     *
     * @return A strong (reference counted) interface object for the Net Peer Group.
     * @see PeerGroup#unref()
     */
    public PeerGroup getInterface() {
        return (PeerGroup) net.getInterface();
    }

    /**
     * Returns a weak (non-reference counted) interface object for the Net Peer Group.
     *
     * @return A weak (non-reference counted) interface object for the Net Peer Group.
     * @see PeerGroup#getWeakInterface()
     */
    public PeerGroup getWeakInterface() {
        return net.getWeakInterface();
    }

    /**
     * Construct the new Net Peer Group instance.
     *
     * @param parentGroup The parent group of the newly created net peer group.
     * @param config Configuration parameters for the newly created net peer group.
     * @param id The name to use for the newly created Net Peer Group.
     * @param name The name to use for the newly created Net Peer Group.
     * @param desc The description to use for the newly created Net Peer Group.
     * @param implAdv The Module Impl Advertisement for the new Net Peer Group
     * instance or {@code null} to use the advertisement returned by
     * {@ link PeerGroup.getAllPurposePeerGroupImplAdvertisement()}.
     * @return the PeerGroup
     * @throws PeerGroupException Thrown for errors instantiating the new Net
     * Peer Group instance.
     */
    private PeerGroup newNetPeerGroup(PeerGroup parentGroup, ConfigParams config, ID id, String name, XMLElement desc, ModuleImplAdvertisement implAdv) throws PeerGroupException {
        synchronized (PeerGroup.globalRegistry) {
            PeerGroup result = PeerGroup.globalRegistry.lookupInstance((PeerGroupID) id);

            if (null != result) {
                result.unref();
                throw new PeerGroupException("Only a single instance of a Peer Group may be instantiated at a single time.");
            }

            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info( "Instantiating net peer group : " + id + 
                        "\n\tParent : " + parentGroup + 
                        "\n\tID : " + id + 
                        "\n\tName : " + name + 
                        "\n\timpl : " + implAdv);
            }

            try {
                if (null == implAdv) {
                    // Use the default Peer Group Impl Advertisement
                    implAdv = parentGroup.getAllPurposePeerGroupImplAdvertisement();
                }

                // Build the group
                GenericPeerGroup.setGroupConfigAdvertisement(id,config);
                
                result = (PeerGroup) parentGroup.loadModule(id, implAdv);
                
                // Set the name and description
                // FIXME 20060217 bondolo How sad, we can't use our XML description.
                if (null != desc) {
                    result.publishGroup(name, desc.getTextValue());
                } else {
                    result.publishGroup(name, null);
                }

                return result;
            } catch (PeerGroupException failed) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "newNetPeerGroup failed", failed);
                }
                // rethrow
                throw failed;
            } catch (RuntimeException e) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "newNetPeerGroup failed", e);
                }
                // rethrow
                throw e;
            } catch (Exception e) {
                // should be all other checked exceptions
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "newNetPeerGroup failed", e);
                }
                // Simplify exception scheme for caller: every sort of problem 
                // wrapped in a PeerGroupException.
                throw new PeerGroupException("newNetPeerGroup failed", e);
            }
        }
    }

    /**
     * Holds the construction tunables for the Net Peer Group. This consists of
     * the peer group id, the peer group name and the peer group description.
     */
    static class NetGroupTunables {

        final ID id;
        final String name;
        final XMLElement desc;

        /**
         * Constructor for loading the default Net Peer Group construction
         * tunables.
         */
        NetGroupTunables() {
            id = PeerGroupID.defaultNetPeerGroupID;
            name = "NetPeerGroup";
            desc = (XMLElement) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "desc", "default Net Peer Group");
        }

        /**
         * Constructor for loading the default Net Peer Group construction
         * tunables.
         *
         * @param pgid the PeerGroupID
         * @param pgname the group name
         * @param pgdesc the group description
         */
        NetGroupTunables(ID pgid, String pgname, XMLElement pgdesc) {
            id = pgid;
            name = pgname;
            desc = pgdesc;
        }

        /**
         * Constructor for loading the Net Peer Group construction
         * tunables from the provided resource bundle.
         *
         * @param rsrcs The resource bundle from which resources will be loaded.
         * @param defaults default values
         */
        NetGroupTunables(ResourceBundle rsrcs, NetGroupTunables defaults) {
            ID idTmp;
            String nameTmp;
            XMLElement descTmp;

            try {
                String idTmpStr = rsrcs.getString("NetPeerGroupID").trim();

                if (idTmpStr.startsWith(ID.URNNamespace + ":")) {
                    idTmpStr = idTmpStr.substring(5);
                }
                idTmp = IDFactory.fromURI(new URI(ID.URIEncodingName + ":" + ID.URNNamespace + ":" + idTmpStr));
                nameTmp = rsrcs.getString("NetPeerGroupName").trim();
                descTmp = (XMLElement) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "desc",
                        rsrcs.getString("NetPeerGroupDesc").trim());
            } catch (Exception failed) {
                if (null != defaults) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "NetPeerGroup tunables not defined or could not be loaded. Using defaults.", failed);
                    }

                    idTmp = defaults.id;
                    nameTmp = defaults.name;
                    descTmp = defaults.desc;
                } else {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "NetPeerGroup tunables not defined or could not be loaded.", failed);
                    }

                    throw new IllegalStateException("NetPeerGroup tunables not defined or could not be loaded.");
                }
            }

            id = idTmp;
            name = nameTmp;
            desc = descTmp;
        }
    }
}
