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


import net.jxta.document.Advertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.exception.ConfiguratorException;
import net.jxta.exception.JxtaError;
import net.jxta.exception.PeerGroupException;
import net.jxta.id.ID;
import net.jxta.logging.Logging;
import net.jxta.protocol.ConfigParams;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.net.URI;
import java.net.URLConnection;
import java.util.MissingResourceException;
import java.util.PropertyResourceBundle;
import java.util.ResourceBundle;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * A factory for instantiating the JXTA core peer groups.
 * <p/>
 * JXTA comes with two peergroup implementations:
 * <p/>
 * <dl>
 * <DT><strong>Platform</strong></DT>
 * <DD>Implements the world peer group. Every peer starts by instantiating this
 * peer group and then other peer groups are instantiated as needed. The World
 * Peer Group's ID is invariant.
 * <p/>
 * The world peer group provides the minimum core services needed to find
 * and instantiate other groups on a peer. The <strong>Platform</strong>
 * implementation will assign a new ID to the peer, if it does not already have
 * one.</DD>
 * <p/>
 * <DT><strong>StdPeergroup</strong></DT>
 * <DD>This is currently used to implement all other kinds of peer groups.
 * The first such peer group that it is instantiated after starting is known as
 * <em>The Net Peer Group</em>. When the <strong>Platform</strong> starts it may
 * optionally search for <em>The Net Peer Group</em> on the local network and,
 * if found, instantiate it. Otherwise a default built-in configuration of
 * <em>The Net Peer Group</em> is instantiated.
 * <p/>
 * A non-default configuration of <em>The Net Peer Group</em> may be set-up
 * by the administrator in charge of the network domain inside which the peer
 * is starting. <em>The Net Peer Group</em> is discovered via the Discovery
 * protocol. Many such groups may be configured by an administrator.<br>
 * <p/>
 * <strong>StdPeergroup</strong> may also be used to implement User-defined
 * peer groups--Users can create new peer groups which use their own set of
 * customized services.</DD>
 * </dl>
 *
 * @see net.jxta.peergroup.PeerGroup
 * @deprecated This factory has been deprecated in favour of {@link WorldPeerGroupFactory}
 *             and {@link NetPeerGroupFactory}. See the deprecations for the individual
 *             methods for the specific replacements/alternatives provided by the new
 *             factory classes.
 */
@Deprecated
public final class PeerGroupFactory {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(PeerGroupFactory.class.getName());

    /**
     * Constant for specifying no configurator. This configurator provides no
     * configuration actions but does ensure that a valid configuration exists
     * at the specified location.
     */
    public final static Class NULL_CONFIGURATOR = net.jxta.impl.peergroup.NullConfigurator.class;

    /**
     * Constant for specifying the default configurator. Currently this is the
     * familiar AWT-based dialogue but in future is likely to become the
     * UI-less automatic configurator.
     */
    public final static Class DEFAULT_CONFIGURATOR = net.jxta.impl.peergroup.DefaultConfigurator.class;

    /**
     * The class which will be instantiated as the World Peer Group.
     */
    private static Class worldGroupClass = null;

    /**
     * The ID of the network peer group.
     */
    private static PeerGroupID netPGID = null;

    /**
     * The name of the network peer group.
     */
    private static String netPGName = null;

    /**
     * The description of the network peer group.
     */
    private static String netPGDesc = null;

    /**
     * The class which will be instantiated to configure the World Peer
     * Group.
     */
    private static Class configurator = DEFAULT_CONFIGURATOR;

    /**
     * the location which will serve as the parent for all stored items used
     * by JXTA.
     */
    private static URI storeHome = null;

    /**
     * Static Method to initialize the world peer group class.
     *
     * @param c The Class which will be instantiated for the World Peer Group
     * @deprecated Consider converting to use {@link WorldPeerGroupFactory#WorldPeerGroupFactory(Class,ConfigParams,URI)}.
     */
    @Deprecated
    public static void setPlatformClass(Class c) {
        worldGroupClass = c;
    }

    /**
     * Static Method to initialize the std peer group class.
     *
     * @param c The Class which will be instantiated for most peer groups.
     * @deprecated This method previously had no effect and has been removed with no alternatives.
     */
    @Deprecated
    public static void setStdPeerGroupClass(Class c) {
        throw new UnsupportedOperationException("This feature has been removed. (sorry)");
    }

    /**
     * Sets the description which will be used for new net peer group instances.
     *
     * @param desc The description which will be used for new net peer group instances.
     * @deprecated Consider converting to use {@link NetPeerGroupFactory#NetPeerGroupFactory(ConfigParams,URI,ID,String,XMLElement)}
     *             or {@link NetPeerGroupFactory#NetPeerGroupFactory(PeerGroup,ID,String,XMLElement)}.
     */
    @Deprecated
    public static void setNetPGDesc(String desc) {
        netPGDesc = desc;
    }

    /**
     * Sets the name which will be used for new net peer group instances.
     *
     * @param name The name which will be used for new net peer group instances.
     * @deprecated Consider converting to use {@link NetPeerGroupFactory#NetPeerGroupFactory(ConfigParams,URI,ID,String,XMLElement)}
     *             or {@link NetPeerGroupFactory#NetPeerGroupFactory(PeerGroup,ID,String,XMLElement)}.
     */
    @Deprecated
    public static void setNetPGName(String name) {
        netPGName = name;
    }

    /**
     * Sets the ID which will be used for new net peer group instances.
     *
     * @param id The ID which will be used for new net peer group instances.
     * @deprecated Consider converting to use {@link NetPeerGroupFactory#NetPeerGroupFactory(ConfigParams,URI,ID,String,XMLElement)}
     *             or {@link NetPeerGroupFactory#NetPeerGroupFactory(PeerGroup,ID,String,XMLElement)}.
     */
    @Deprecated
    public static void setNetPGID(PeerGroupID id) {
        netPGID = id;
    }

    /**
     * Get the optional configurator class for the world peer group.
     *
     * @return Class configurator class
     * @deprecated Consider converting to use {@link NetPeerGroupFactory}.
     */
    @Deprecated
    public static Class getConfiguratorClass() {
        return configurator;
    }

    /**
     * Set the optional configurator class for the World Peer Group. If present
     * an instance of this class will be used to generate/update the
     * configuration parameters for the World Peer Group whenever
     * {@code newPlatform()} is invoked.
     * <p/>
     * All configuration actions for the World Peer Group may be completely
     * disabled by specify {@code null} as the configurator class. The default
     * configuration class is always initialized to {@code DEFAULT_CONFIGURATOR}.
     *
     * @param c The {@code Class} to use as a configurator for the World Peer
     *          Group.
     * @deprecated Consider converting to use {@link NetPeerGroupFactory} and/or {@link WorldPeerGroupFactory}.
     */
    @Deprecated
    public static void setConfiguratorClass(Class c) {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Setting configurator class to : " + c);
        }

        configurator = c;
    }

    /**
     * Returns the location which will serve as the parent for all stored items
     * used by JXTA. This method is intended for use by PeerGroup implementations
     * and is not intended for use by applications. Applications and services
     * should use the PeerGroup method with the same name.
     *
     * @return The location which will serve as the parent for all stored
     *         items used by JXTA.
     * @see PeerGroup#getStoreHome()
     * @deprecated Consider converting to use {@link NetPeerGroupFactory} and/or {@link WorldPeerGroupFactory}.
     */
    @Deprecated
    public static URI getStoreHome() {
        if (null == storeHome) {
            // Establish the default store location via long established hackery.
            String jxta_path = System.getProperty("JXTA_HOME", ".jxta/");

            File jxta_home = new File(jxta_path);

            jxta_home.mkdirs();
            URI defaultHome = jxta_home.toURI();

            return defaultHome;
        }

        return storeHome;
    }

    /**
     * Set the location which will serve as the parent for all stored items used by JXTA.
     *
     * @param newHome The absolute URI location which will serve as the parent
     *                for all stored items used by JXTA. Currently this must be a non-opaque URI.
     *                May also be {@code null} to restore the default value.
     * @deprecated Consider converting to use {@link NetPeerGroupFactory} and/or {@link WorldPeerGroupFactory}.
     */
    @Deprecated
    public static void setStoreHome(URI newHome) {

        if (null != newHome) {
            // Fail if the URI is not absolute.
            if (!newHome.isAbsolute()) {
                throw new IllegalArgumentException("Only absolute URIs accepted for store home location.");
            }

            // Fail if the URI is Opaque.
            if (newHome.isOpaque()) {
                throw new IllegalArgumentException("Only hierarchical URIs accepted for store home location.");
            }

            // Add a trailing slash if necessary. 
            if (!newHome.toString().endsWith("/")) {
                newHome = URI.create(newHome.toString() + "/");
            }
        }

        storeHome = newHome;
    }

    /**
     * Static Method to create a new peer group instance.
     * <p/>
     * After being created the init() method needs to be called, and
     * the startApp() method may be called, at the invoker's discretion.
     *
     * @return PeerGroup instance of a new PeerGroup
     * @deprecated This method was previously unused and has been removed with no alternatives. (it wasn't useful)
     */
    @Deprecated
    public static PeerGroup newPeerGroup() {
        throw new UnsupportedOperationException("This feature has been removed. (sorry)");
    }

    /**
     * Instantiates the World (Platform) Peer Group and can also optionally
     * (re)configure the world peer group before instantiation using the
     * configurator specified via {@link #setConfiguratorClass(Class)}.
     * <p/>
     * Only one instance of  the World Peer Group may be created within the
     * context of the {@code PeerGroupFactory}'s class loader. Invoking this
     * method amounts to creating an instance of JXTA.
     * <p/>
     * The {@link PeerGroup#init(PeerGroup,ID,Advertisement)} method is
     * called automatically. The {@link PeerGroup#startApp(String[])} method
     * is left for the invoker to call if appropriate.
     *
     * @return PeerGroup The World Peer Group instance.
     * @throws JxtaError Thrown for all checked Exceptions which occur during
     *                   construction of the World Peer Group.
     * @deprecated Consider converting to use {@link WorldPeerGroupFactory#WorldPeerGroupFactory()}.
     */
    @Deprecated
    public static PeerGroup newPlatform() {

        Class c = PeerGroupFactory.getConfiguratorClass();

        if (null == c) {
            c = NULL_CONFIGURATOR;
        }

        Configurator configurator;

        try {
            Constructor config_constructor = c.getConstructor(URI.class);

            configurator = (Configurator) config_constructor.newInstance(getStoreHome());
        } catch (InvocationTargetException ie) {
            LOG.log(Level.SEVERE, "Uninstantiatable configurator: " + c, ie);

            throw new JxtaError("Uninstantiatable configurator: " + c, ie);
        } catch (NoSuchMethodException ie) {
            LOG.log(Level.SEVERE, "Uninstantiatable configurator: " + c, ie);

            throw new JxtaError("Uninstantiatable configurator: " + c, ie);
        } catch (InstantiationException ie) {
            LOG.log(Level.SEVERE, "Uninstantiatable configurator: " + c, ie);

            throw new JxtaError("Uninstantiatable configurator: " + c, ie);
        } catch (IllegalAccessException iae) {
            LOG.log(Level.SEVERE, "can\'t instantiate configurator: " + c, iae);

            throw new JxtaError("Can't instantiate configurator: " + c, iae);
        } catch (ClassCastException cce) {
            LOG.log(Level.SEVERE, "Not a Configurator :" + c, cce);

            throw new JxtaError("Not a Configurator :" + c, cce);
        }

        ConfigParams pc;

        try {
            pc = configurator.getConfigParams();
        } catch (ConfiguratorException cce) {
            LOG.log(Level.SEVERE, "Could not retrieve configuration", cce);

            throw new JxtaError("Could not retrieve configuration", cce);
        }

        try {
            WorldPeerGroupFactory wpgf;

            if (null == worldGroupClass) {
                wpgf = new WorldPeerGroupFactory(pc, getStoreHome());
            } else {
                wpgf = new WorldPeerGroupFactory(worldGroupClass, pc, getStoreHome());
            }

            configurator.setConfigParams(pc);
            configurator.save();

            // Forget about the configurator
            configurator = null;

            return wpgf.getInterface();
        } catch (RuntimeException e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "newPlatform failed", e);
            }
            // rethrow
            throw e;
        } catch (Exception e) {
            // should be all other checked exceptions
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "newPlatform failed", e);
            }

            // Simplify exception scheme for caller: any sort of problem wrapped
            // in a PeerGroupException.
            throw new JxtaError("newPlatform failed", e);
        } catch (Error e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "newPlatform failed", e);
            }
            // rethrow
            throw e;
        }
    }

    /**
     * Instantiates the net peer group using the provided parent peer group.
     *
     * @param ppg The parent group.
     * @return PeerGroup The default netPeerGroup
     * @throws PeerGroupException For failures in constructing the Net Peer Group.
     * @deprecated Consider converting to use {@link NetPeerGroupFactory#NetPeerGroupFactory(PeerGroup,ID,String,XMLElement)}.
     */
    @Deprecated
    public static PeerGroup newNetPeerGroup(PeerGroup ppg) throws PeerGroupException {

        try {
            NetPeerGroupFactory npgf;

            NetPeerGroupFactory.NetGroupTunables tunables;

            if (null == netPGID) {
                // Determine net peer group configuration parameters if they
                // have not already been set.
                tunables = new NetPeerGroupFactory.NetGroupTunables(ResourceBundle.getBundle("net.jxta.impl.config")
                        ,
                        new NetPeerGroupFactory.NetGroupTunables());

                // load overides from "${JXTA_HOME}config.properties".
                URI configPropertiesLocation = getStoreHome().resolve("config.properties");

                try {
                    URLConnection configProperties = configPropertiesLocation.toURL().openConnection();

                    ResourceBundle rsrcs = new PropertyResourceBundle(configProperties.getInputStream());

                    tunables = new NetPeerGroupFactory.NetGroupTunables(rsrcs, tunables);

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Loaded defaults from " + rsrcs);
                    }
                } catch (MissingResourceException ignored) {
                    ;
                } catch (IOException ignored) {
                    ;
                } catch (Exception ignored) {
                    ;
                }
            } else {
                tunables = new NetPeerGroupFactory.NetGroupTunables(netPGID, netPGName
                        ,
                        (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "desc", netPGDesc));
            }

            npgf = new NetPeerGroupFactory(ppg, tunables.id, tunables.name, tunables.desc);

            PeerGroup newPg = npgf.getInterface();

            return newPg;
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
            // Simplify exception scheme for caller: any sort of problem wrapped
            // in a PeerGroupException.
            throw new PeerGroupException("newNetPeerGroup failed", e);
        } catch (Error e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "newNetPeerGroup failed", e);
            }

            // rethrow
            throw e;
        }
    }

    /**
     * Instantiates the World Peer Group and then instantiates the Net Peer
     * Group. This simplifies the method by which applications can start JXTA.
     *
     * @return The newly instantiated Net Peer Group.
     * @deprecated Consider converting to use {@link NetPeerGroupFactory#NetPeerGroupFactory()}
     *             or preferably one of the other {@code NetPeerGroupFactory} constructors.
     */
    @Deprecated
    public static PeerGroup newNetPeerGroup() throws PeerGroupException {
        // get/create the World Peer Group.
        PeerGroup wpg = getWorldPeerGroup();

        try {
            PeerGroup npg = newNetPeerGroup(wpg);

            return npg;
        } finally {
            wpg.unref();
        }
    }

    /**
     * Retrieves or constructs a new World Peer Group instance suitable for
     * use as the parent for Net Peer Group instances. This implementation
     * makes an important trade-off worth noting; it will use an existing
     * world peer group instance if available and ignore any changes which have
     * been made to the static configuration methods provided by this class.
     *
     * @return The World Peer Group.
     * @throws PeerGroupException For failures in recovering the World Peer Group.
     */
    private static PeerGroup getWorldPeerGroup() throws PeerGroupException {
        synchronized (PeerGroup.globalRegistry) {
            PeerGroup result = PeerGroup.globalRegistry.lookupInstance(PeerGroupID.worldPeerGroupID);

            if (null != result) {
                return result;
            }

            return newPlatform();
        }
    }
}
