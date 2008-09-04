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

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.XMLDocument;
import net.jxta.exception.ConfiguratorException;
import net.jxta.exception.JxtaError;
import net.jxta.impl.protocol.PSEConfigAdv;
import net.jxta.impl.protocol.PlatformConfig;
import net.jxta.logging.Logging;
import net.jxta.peergroup.PeerGroup;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URI;
import java.util.NoSuchElementException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * This implementation provides the ability to reconfigure a JXTA PlatformConfig
 * via an AWT based dialog. This is the original JXTA configuration mechanism.
 */
public class DefaultConfigurator extends AutomaticConfigurator {

    /**
     * logger
     */
    private final static transient Logger LOG = Logger.getLogger(DefaultConfigurator.class.getName());

    /**
     * Configures the platform using the specified directory.
     *
     * @param jxtaHome store home URI
     * @throws net.jxta.exception.ConfiguratorException
     *          if a configuration error occurs
     */
    public DefaultConfigurator(URI jxtaHome) throws ConfiguratorException {
        super(jxtaHome);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>Kinda hackish in that we don't really do anything if home is not a file.
     */
    @Override
    public boolean isReconfigure() {
        if (!"file".equalsIgnoreCase(jxtaHome.getScheme())) {
            return false;
        }

        File jxtaHomeDir = new File(jxtaHome);
        try {
            boolean forceReconfig;
            File file = new File(jxtaHomeDir, "reconf");
            forceReconfig = file.exists();

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("force reconfig : " + forceReconfig);
            }

            return forceReconfig;
        } catch (Exception ex1) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Could not check \'reconf\' file. Assuming it exists.", ex1);
            }
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("Reconfig required - error getting \'reconf\' file");
            }
            return true;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setReconfigure(boolean reconfigure) {
        if (!"file".equalsIgnoreCase(jxtaHome.getScheme())) {
            return;
        }

        File jxtaHomeDir = new File(jxtaHome);
        File f = new File(jxtaHomeDir, "reconf");

        if (reconfigure) {
            try {
                f.createNewFile();
            } catch (IOException ex1) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Could not create \'reconf\' file", ex1);
                    LOG.severe("Create the file \'reconf\' by hand before retrying.");
                }
            }
        } else {
            try {
                f.delete();
            } catch (Exception ex1) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Could not remove \'reconf\' file", ex1);
                    LOG.severe("Delete the file \'reconf\' by hand before retrying.");
                }
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public PlatformConfig getPlatformConfig() throws ConfiguratorException {
        boolean needsConfig = isReconfigure();

        if (needsConfig && Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            LOG.config("Reconfig requested - Forced reconfigure");
        }

        try {
            super.getPlatformConfig();

            // Automatic configuration doesn't do any security config. We use
            // this fact to decide if we must do configuration.
            XMLDocument security = (XMLDocument) advertisement.getServiceParam(PeerGroup.membershipClassID);

            if (null == security) {
                needsConfig = true;

                if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                    LOG.config("Reconfig requested - No security info");
                }
            } else {
                Advertisement adv = null;

                try {
                    adv = AdvertisementFactory.newAdvertisement(security);
                } catch (NoSuchElementException notAnAdv) {// that's ok.
                } catch (IllegalArgumentException badAdv) {// that's ok.
                }

                if (adv instanceof PSEConfigAdv) {
                    PSEConfigAdv pseConfig = (PSEConfigAdv) adv;

                    // no certificate? That means we need to make one.
                    needsConfig |= (null == pseConfig.getCertificate());
                } else {
                    needsConfig = true;
                }
            }
        } catch (IncompleteConfigurationException configBad) {
            needsConfig = true;
        }

        if (needsConfig) {
            setReconfigure(true);

            try {
                if (java.awt.EventQueue.isDispatchThread()) {
                    LOG.severe("The JXTA AWT Configuration Dialog cannot be run from the event dispatch thread. It\'s modal nature is fundamentally incompatible with running on the event dispatch thread. Either change to a different Configurator via PeerGroupFactory.setConfiguratorClass() or start JXTA from the application main Thread before starting your GUI via invokeLater().");
                    // cruel but fair, the alternative is a UI deadlock.
                    System.exit(1);
                }
                ConfigDialog configUI = new ConfigDialog(advertisement);

                configUI.untilDone();
                setReconfigure(false);
            } catch (Throwable t) {
                if (t instanceof JxtaError) {
                    throw (JxtaError) t;
                }
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Could not initialize graphical config dialog", t);
                }

                BufferedReader in = new BufferedReader(new InputStreamReader(System.in));

                // clear any type-ahead
                try {
                    while (in.ready()) {
                        in.readLine();
                    }
                } catch (Exception ignored) {// ignored
                }

                System.err.flush();
                System.out.flush();
                System.out.println("The window-based configurator does not seem to be usable.");
                System.out.print("Do you want to stop and edit the current configuration ? [no]: ");
                System.out.flush();
                String answer = "no";

                try {
                    answer = in.readLine();
                } catch (Exception ignored) {// ignored
                }

                // this will cover all the cases of the answer yes, while
                // allowing non-gui/batch type scripts to load the platform
                // see platform issue #45

                if ("yes".equalsIgnoreCase(answer)) {
                    save();
                    System.out.println("Exiting; edit the file \"" + configFile.getPath()
                            + "\", remove the file \"reconf\", and then launch JXTA again.");
                    throw new JxtaError("Manual Configuration Requested");
                } else {
                    System.out.println("Attempting to continue using the current configuration.");
                }
            }
        }
        return advertisement;
    }
}
