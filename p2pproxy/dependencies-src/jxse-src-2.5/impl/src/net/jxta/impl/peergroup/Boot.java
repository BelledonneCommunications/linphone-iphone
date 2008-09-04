/*
Copyright (c) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
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


import net.jxta.peergroup.PeerGroup;
import net.jxta.platform.NetworkManager;
import net.jxta.platform.NetworkConfigurator;

import java.util.logging.Logger;

import java.io.File;
import java.text.MessageFormat;
import java.util.Collections;


/**
 * A default "main" for starting JXTA.
 *
 * @deprecated This code is in no-way dependent upon the implementation and
 *             should not have been located here. Developers are encouraged to copy this
 *             source to their own projects. Consider using alternative JXTA "main"
 *             See NetworkManager tutorial </a>.
 */
@Deprecated
public class Boot {

    /**
     * main
     *
     * @param args command line arguments
     */
    public static void main(String args[]) {
        // Name the main thread. For unknown reasons it usually has a boring name like "thread1"
        Thread.currentThread().setName(Boot.class.getName() + ".main()");
        
        try {
            boolean server;
            // Get the optional location of the directory we should use for cache.
            String jxta_home = System.getProperty("JXTA_HOME");
            File home;
            String instanceName;
            
            if (null != jxta_home) {
                // Use the location from the older JXTA_HOME system property.
                server = false;
                instanceName = "BootCustom";
                home = new File(jxta_home);
            } else {
                // Use the location defined by the newer role convention.
                server = args.length > 0 && ("-server".equalsIgnoreCase(args[0]));
                
                if (server) {
                    instanceName = "BootServer";
                } else {
                    instanceName = "BootEdge";
                }
                
                home = new File(new File(".cache"), instanceName);
            }
            
            // If the home directory doesn't exist, create it.
            if (!home.exists()) {
                home.mkdirs();
            }

            NetworkManager manager;

            if (server) {
                manager = new NetworkManager(NetworkManager.ConfigMode.SUPER, instanceName, home.toURI());

                /*
                 NetworkConfigurator config = manager.getConfigurator();

                 //disable http
                 config.setHttpEnabled(false);
                 //disable seeding
                 config.setRelaySeedURIs(Collections.<String>emptyList());
                 config.setRendezvousSeedURIs(Collections.<String>emptyList());
                 */
            } else {
                manager = new NetworkManager(NetworkManager.ConfigMode.EDGE, instanceName, home.toURI());
            }
            // register a <ctrl-c> hook
            // manager.registerShutdownHook();

            System.out.println(MessageFormat.format("Starting the JXTA platform in mode : {0}", manager.getMode()));
            long startTime = System.currentTimeMillis();

            manager.startNetwork();
            System.out.println(
                    MessageFormat.format("Boot started in {0} millis, in mode : {1}", System.currentTimeMillis() - startTime
                    ,
                    manager.getMode()));

            PeerGroup netPeerGroup = manager.getNetPeerGroup();

            netPeerGroup.startApp(null);

            System.out.println(MessageFormat.format("Boot started in mode : {0}", manager.getMode()));
            if (server) {
                // Put this thread permanently to sleep so that JXTA keeps running.
                Thread.sleep(Long.MAX_VALUE);
            }
        } catch (Throwable e) {
            System.out.flush();
            // make sure output buffering doesn't wreck console display.
            System.err.println("Uncaught Throwable caught by 'main':");
            e.printStackTrace(System.err);
            System.exit(1);
        } finally {
            System.err.flush();
            System.out.flush();
        }
    }
}
