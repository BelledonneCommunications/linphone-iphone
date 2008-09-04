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

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.exception.ConfiguratorException;
import net.jxta.impl.protocol.PlatformConfig;
import net.jxta.logging.Logging;
import net.jxta.protocol.ConfigParams;

import java.io.*;
import java.net.URI;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * A minimal Platform Configurator. This implementation can load a
 * configuration from an existing PlatformConfig file and also save a
 * configuration to the PlatformConfig file.
 * <p/>
 * This configurator provides no explict validation of the PlatformConfig
 * as it is read from the file (Some is done by the PlatformConfig class) and
 * provides no mechanism for reconfiguration. The NullConfigurator provides a
 * useful base implementation for extending your own Configurator and also
 * provides the minimal implementation needed for applications which perform
 * their own configuration.
 */
public class NullConfigurator implements PlatformConfigurator {

    /**
     * logger
     */
    private final static transient Logger LOG = Logger.getLogger(NullConfigurator.class.getName());

    /**
     * The location in which the configuration files will reside.
     */
    protected final URI jxtaHome;

    /**
     * The file in which contains the platform configurtation.
     */
    protected final URI configFile;

    /**
     * The platform config
     */
    protected PlatformConfig advertisement = null;

    /**
     * Constructor for the NullConfigurator
     *
     * @param homeRoot The location in which the configuration files will reside.
     * @throws ConfiguratorException If there is a problem accessing the configuration information.
     */
    public NullConfigurator(URI homeRoot) throws ConfiguratorException {
        if (!homeRoot.isAbsolute()) {
            throw new IllegalArgumentException("homeRoot must be an absoluteURI");
        }

        jxtaHome = homeRoot;

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            LOG.config("JXTA_HOME = " + jxtaHome.toASCIIString());
        }

        if ("file".equalsIgnoreCase(jxtaHome.getScheme())) {
            File jxtaHomeDir = new File(jxtaHome);

            if (jxtaHomeDir.exists() && !jxtaHomeDir.isDirectory()) {
                throw new IllegalArgumentException("'" + jxtaHomeDir + "' is not a directory.");
            }

            if (!jxtaHomeDir.exists()) {
                if (!jxtaHomeDir.mkdirs()) {
                    throw new IllegalStateException("Could not create '" + jxtaHomeDir + "'.");
                }
            }

            configFile = new File(jxtaHomeDir, "PlatformConfig").toURI();
        } else {
            configFile = jxtaHome.resolve("PlatformConfig");
        }
    }

    /**
     * @inheritDoc
     */
    public PlatformConfig getPlatformConfig() throws ConfiguratorException {
        advertisement = (PlatformConfig) load();

        return advertisement;
    }

    /**
     * @inheritDoc
     */
    public final void setPlatformConfig(PlatformConfig config) {
        advertisement = config;
    }

    /**
     * @inheritDoc
     */
    public ConfigParams getConfigParams() throws ConfiguratorException {
        return getPlatformConfig();
    }

    /**
     * @inheritDoc
     */
    public void setConfigParams(ConfigParams cp) {
        setPlatformConfig((PlatformConfig) cp);
    }

    /**
     * @inheritDoc
     */
    public void setReconfigure(boolean reconfigure) {// This implementation doesn't do configuration so ignores this operation.
    }

    /**
     * @inheritDoc
     */
    public boolean isReconfigure() {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public ConfigParams load() throws ConfiguratorException {
        return load(configFile);
    }

    /**
     * Retrieves the persisted parameters associated with this configuration
     * from the standard location.
     *
     * @param loadFile The location from which the configuration data should be
     *                 loaded.
     * @return The configuration parameters.
     * @throws ConfiguratorException If there was a failure in retrieving the
     *                               persisted parameters. This is normally a chained exception to the
     *                               underlying cause.
     * @deprecated Loading of existing configuration is best accomplished by use
     *             of specific constructors of the implementing configurator. This method
     *             complicates the state management of configuration parameters and may have
     *             unpredictable results depending upon the constructor and configuration
     *             set methods used prior to it's execution.
     */
    @Deprecated
    protected PlatformConfig load(URI loadFile) throws ConfiguratorException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Reading Platform Config from : " + loadFile);
        }

        InputStream advStream = null;

        try {
            advStream = loadFile.toURL().openStream();

            XMLDocument xmlDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, advStream);
            PlatformConfig result = (PlatformConfig) AdvertisementFactory.newAdvertisement(xmlDoc);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Recovered Platform Config from : " + loadFile);
            }

            return result;
        } catch (FileNotFoundException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Platform Config not found : " + loadFile);
            }

            return null;
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to Recover \'" + loadFile + "\' due to : ", e);
            }

            throw new ConfiguratorException("Failed to recover PlatformConfig", e);
        } finally {
            try {
                if (advStream != null) {
                    advStream.close();
                }
                advStream = null;
            } catch (Exception ignored) {// ignored
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public boolean save() throws ConfiguratorException {
        return save(configFile);
    }

    /**
     * Persist the parameters associated with this configuration to the
     * specified location.
     *
     * @param saveFile The location to which the configuration should be saved.
     * @return <code>true</code> if the configuration was successfully saved
     *         otherwise <code>false</code>. If the parameters are not persisted then
     *         <code>false/code> is returned.
     * @throws ConfiguratorException If there was a failure in persisting the
     *                               parameters. This is normally a chained exception to the underlying
     *                               cause.
     */
    protected boolean save(URI saveFile) throws ConfiguratorException {

        // Save the adv as input for future reconfiguration
        OutputStream out = null;

        try {
            XMLDocument aDoc = (XMLDocument) advertisement.getDocument(MimeMediaType.XMLUTF8);

            if ("file".equalsIgnoreCase(saveFile.getScheme())) {
                out = new FileOutputStream(new File(saveFile));
            } else {
                out = saveFile.toURL().openConnection().getOutputStream();
            }

            OutputStreamWriter os = new OutputStreamWriter(out, "UTF-8");

            aDoc.sendToWriter(os);
            os.flush();
        } catch (IOException e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Could not save to : " + saveFile, e);
            }

            throw new ConfiguratorException("Could not save to : " + saveFile, e);
        } finally {
            try {
                if (null != out) {
                    out.close();
                }
            } catch (Exception ignored) {// ignored
            }
            out = null;
        }
        return true;
    }
}
