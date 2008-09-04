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
package net.jxta.impl.endpoint.servlethttp;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.net.InetAddress;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;

import java.io.IOException;
import java.io.FileNotFoundException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import org.mortbay.http.HttpContext;
import org.mortbay.http.HttpServer;
import org.mortbay.http.SocketListener;
import org.mortbay.http.handler.ResourceHandler;
import org.mortbay.jetty.servlet.ServletHandler;
import org.mortbay.util.InetAddrPort;
import org.mortbay.util.Log;
import org.mortbay.util.LoggerLogSink;

import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.MessageReceiver;
import net.jxta.endpoint.MessengerEvent;
import net.jxta.endpoint.MessengerEventListener;

import net.jxta.exception.PeerGroupException;

import net.jxta.impl.util.TimeUtils;

/**
 * Simple Message Receiver for server side.
 */
class HttpMessageReceiver implements MessageReceiver {

    /**
     *  logger
     */
    private final static transient Logger LOG = Logger.getLogger(HttpMessageReceiver.class.getName());

    /**
     * the relative URI of where the message receiver servlet will be mounted.
     */
    private final static String MSG_RECEIVER_RELATIVE_URI = "/*";

    /**
     * The ServletHttpTransport that created this MessageReceiver.
     */
    final ServletHttpTransport servletHttpTransport;

    /**
     * The public addresses for the this transport.
     */
    private final List<EndpointAddress> publicAddresses;

    /**
     *  The min threads that the HTTP server will use for handling requests.
     */
    private static int MIN_LISTENER_THREADS = 10;
    
    /**
     *  The max threads that the HTTP server will use for handling requests.
     */
    private static int MAX_LISTENER_THREADS = 200;
    
    /**
     *  How long a thread can remain idle until the worker thread is let go.
     */
    private static long MAX_THREAD_IDLE_DURATION = 30 * TimeUtils.ASECOND;
    
    /**
     *  The Jetty HTTP Server instance.
     */
    private final HttpServer server;
    private final ServletHandler handler;
    private final SocketListener listener;

    /**
     * The listener to invoke when making an incoming messenger.
     */
    private MessengerEventListener messengerEventListener;

    public HttpMessageReceiver(ServletHttpTransport servletHttpTransport, List<EndpointAddress> publicAddresses, InetAddress useInterface, int port) throws PeerGroupException {
        this.servletHttpTransport = servletHttpTransport;
        this.publicAddresses = publicAddresses;
       
        // read settings from the properties file
        Properties prop = getJxtaProperties(
                new File(new File(servletHttpTransport.getEndpointService().getGroup().getStoreHome()), "jxta.properties"));
        initFromProperties(prop);

        if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
            StringBuilder configInfo = new StringBuilder("Configuring HTTP Servlet Message Transport : " + servletHttpTransport.assignedID);

            configInfo.append("\n\tMin threads=").append(MIN_LISTENER_THREADS);
            configInfo.append("\n\tMax threads=").append(MAX_LISTENER_THREADS);
            configInfo.append("\n\tMax thread idle time=").append(MAX_THREAD_IDLE_DURATION).append("ms");
            
            LOG.config(configInfo.toString());
        }
        
        // Configure Jetty Logging
        if (!(Logging.SHOW_FINER && LOG.isLoggable(Level.FINER))) {
            Log.instance().disableLog();
        }
        
        org.mortbay.util.Code.setDebug(Logging.SHOW_FINER && LOG.isLoggable(Level.FINER));
        org.mortbay.util.Code.setSuppressWarnings(!(Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)));
        org.mortbay.util.Code.setSuppressStack(!(Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)));

        // Setup the logger to match the rest of JXTA unless explicitly configured.
        // "LOG_CLASSES" is a Jetty thing.
        if (System.getProperty("LOG_CLASSES") == null) {
            LoggerLogSink logSink = new LoggerLogSink();
            Logger jettyLogger = Logger.getLogger(org.mortbay.http.HttpServer.class.getName());

            logSink.setLogger(jettyLogger);
            try {
                logSink.start();
                Log.instance().add(logSink);
            } catch (Exception ex) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Could not configure LoggerLogSink");
                }
            }
        }
        
        // Initialize the Jetty HttpServer
        server = new HttpServer();

        // Create the listener and attach it to server.
        InetAddrPort addrPort = new InetAddrPort(useInterface, port);

        listener = new SocketListener(addrPort);

        listener.setMinThreads(MIN_LISTENER_THREADS);
        listener.setMaxThreads(MAX_LISTENER_THREADS);
        listener.setMaxIdleTimeMs((int) MAX_THREAD_IDLE_DURATION);

        server.addListener(listener);

        // Create a context for the handlers at the root, then add servlet
        // handler for the specified servlet class and add it to the context
        HttpContext handlerContext = server.getContext("/");

        handler = new ServletHandler();

        handler.setUsingCookies(false);
        handler.initialize(handlerContext);

        // Use peer group class loader (useful for HttpMessageServlet)
        handlerContext.setClassLoader(servletHttpTransport.getEndpointService().getGroup().getLoader());
        handlerContext.addHandler(handler);

        // Set up support for downloading midlets.
        if (System.getProperty("net.jxta.http.allowdownload") != null) {
            HttpContext context = server.addContext("/midlets/*");

            context.setResourceBase("./midlets/");
            // context.setDirAllowed(false);
            // context.setServingResources(true);

            // String methods[] = {"GET"};
            ResourceHandler resHandler = new ResourceHandler();

            // resHandler.setAllowedMethods(methods);
            context.addHandler(resHandler);
        }

        handler.addServlet(MSG_RECEIVER_RELATIVE_URI, HttpMessageServlet.class.getName());
    }

    synchronized void start() throws PeerGroupException {
        try {
            server.start();
            handler.getServletContext().setAttribute("HttpMessageReceiver", this);
        } catch (Exception e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Could not start server", e);
            }
            PeerGroupException failure = new PeerGroupException("Could not start server");
            failure.initCause(e);
            throw failure;
        }

        messengerEventListener = servletHttpTransport.getEndpointService().addMessageTransport(this);
        if (messengerEventListener == null) {
            throw new PeerGroupException("Transport registration refused");
        }
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("HTTP Servlet Transport started.");
        }
    }
    
    synchronized void stop() {
        servletHttpTransport.getEndpointService().removeMessageTransport(this);
        messengerEventListener = null;
        
        try {
            server.stop();
        } catch (InterruptedException e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Interrupted during stop()", e);
            }
        }
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("HTTP Servlet Transport stopped.");
        }
    }

    /**
     * {@inheritDoc}
     */
    boolean messengerReadyEvent(HttpServletMessenger newMessenger, EndpointAddress connAddr) {
        MessengerEventListener temp = messengerEventListener;

        return null != temp && temp.messengerReady(new MessengerEvent(this, newMessenger, connAddr));
    }

    /**
     * {@inheritDoc}
     */
    public Iterator<EndpointAddress> getPublicAddresses() {
        return Collections.unmodifiableList(publicAddresses).iterator();
    }

    /**
     * {@inheritDoc}
     */
    public String getProtocolName() {
        return servletHttpTransport.HTTP_PROTOCOL_NAME;
    }

    /**
     * {@inheritDoc}
     */
    public EndpointService getEndpointService() {
        return servletHttpTransport.getEndpointService();
    }
    
    /**
     * {@inheritDoc}
     */
    public Object transportControl(Object operation, Object Value) {
        return null;
    }

    ServletHttpTransport getServletHttpTransport() {
        return servletHttpTransport;
    }

    /**
     * Returns a Properties instance for jxta.properties if the file exists;
     * otherwise, returns null.
     *
     * @param fromFile properties file
     * @return the properties object or null if properties file was not found
     */
    private static Properties getJxtaProperties(File fromFile) {
        Properties prop = new Properties();
        InputStream in = null;
        
        try {
            in = new FileInputStream(fromFile);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Read properties from " + fromFile.getPath());
            }
        } catch (FileNotFoundException e) {
            return null;
        }
        
        try {
            prop.load(in);
        } catch (IOException e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Error reading " + fromFile.getPath(), e);
            }
        } finally {
            try {
                in.close();
            } catch (IOException ignored) {
                //ignored
            }
            in = null;
        }
        return prop;
    }
    
    /**
     * Reads the properties from the jxta.properties file
     *
     * @param prop properties to init from
     */
    private void initFromProperties(Properties prop) {
        
        if (prop == null) {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.fine("jxta.properties not found: using default values");
            }
        } else {
            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("Using jxta.properties to configure HTTP server");
            }
            
            String minThreadsStr = prop.getProperty("HttpServer.MinThreads");
            String maxThreadsStr = prop.getProperty("HttpServer.MaxThreads");
            String maxThreadIdleTimeStr = prop.getProperty("HttpServer.MaxThreadIdleTime");
            
            try {
                if (minThreadsStr != null) {
                    MIN_LISTENER_THREADS = Integer.parseInt(minThreadsStr);
                }
            } catch (NumberFormatException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Invalid HttpServer.MinThreads value; using default");
                }
            }
            
            try {
                if (maxThreadsStr != null) {
                    MAX_LISTENER_THREADS = Integer.parseInt(maxThreadsStr);
                }
            } catch (NumberFormatException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Invalid HttpServer.MaxThreads value; using default");
                }
            }
            
            try {
                if (maxThreadIdleTimeStr != null) {
                    MAX_THREAD_IDLE_DURATION = Integer.parseInt(maxThreadIdleTimeStr);
                }
            } catch (NumberFormatException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Invalid HttpServer.MaxThreadIdleTime value; using default");
                }
            }
        }
    }    
}
