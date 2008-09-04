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

package net.jxta.impl.endpoint.tcp;

import net.jxta.logging.Logging;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.*;
import java.nio.channels.*;
import java.nio.channels.spi.SelectorProvider;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.RejectedExecutionException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * This server handles incoming unicast TCP connections
 */
public class IncomingUnicastServer implements Runnable {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(IncomingUnicastServer.class.getName());

    /**
     * The transport which owns this server.
     */
    private final TcpTransport transport;

    /**
     * The interface address the serverSocket will try to bind to.
     */
    private final InetAddress serverBindLocalInterface;

    /**
     * The beginning of the port range the serverSocket will try to bind to.
     */
    private final int serverBindStartLocalPort;

    /**
     * The port the serverSocket will try to bind to.
     */
    private int serverBindPreferedLocalPort;

    /**
     * The end of the port range the serverSocket will try to bind to.
     */
    private final int serverBindEndLocalPort;

    /**
     * The socket we listen for connections on.
     */
    private ServerSocket serverSocket;

    /**
     * If true then the we are closed or closing.
     */
    private volatile boolean closed = false;

    /**
     * The thread on which connection accepts will take place.
     */
    private Thread acceptThread = null;

    /**
     * Channel Selector
     */
    private Selector acceptSelector = null;

    /**
     * ServerSocket Channel
     */
    private ServerSocketChannel serverSocChannel = null;

    /**
     * Constructor for the TCP server
     *
     * @param owner           the TCP transport we are working for
     * @param serverInterface the network interface to use.
     * @param preferedPort    the port we will be listening on.
     * @param startPort       starting port
     * @param endPort         the endport in port range
     * @throws IOException       if an io severe occurs
     * @throws SecurityException if a security exception occurs
     */
    public IncomingUnicastServer(TcpTransport owner, InetAddress serverInterface, int preferedPort, int startPort, int endPort) throws IOException, SecurityException {
        this.transport = owner;
        serverBindLocalInterface = serverInterface;
        serverBindPreferedLocalPort = preferedPort;
        serverBindStartLocalPort = startPort;
        serverBindEndLocalPort = endPort;

        openServerSocket();
    }

    /**
     * Start this server.
     *
     * @return true if successfully started
     */
    public synchronized boolean start() {

        if (acceptThread != null) {
            return false;
        }

        // Start daemon thread
        acceptThread = new Thread(transport.group.getHomeThreadGroup(), this, "TCP Transport ServerSocket accept for " + transport.getPublicAddress());
        acceptThread.setDaemon(true);
        acceptThread.start();
        return true;
    }

    /**
     * Stop this server.
     */
    public synchronized void stop() {
        closed = true;

        Thread temp = acceptThread;

        if (null != temp) {
            // interrupt does not seem to have an effect on threads blocked in accept.
            temp.interrupt();
        }

        // Closing the socket works though.
        try {
            serverSocket.close();
        } catch (IOException ignored) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "IO error occured while closing server socket", ignored);
            }
        }

        try {
            acceptSelector.close();
        } catch (IOException io) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "IO error occured while closing Selectors", io);
            }
        }
    }

    /**
     * Get the address of the network interface being used.
     *
     * @return the local socket address
     */
    synchronized InetSocketAddress getLocalSocketAddress() {
        ServerSocket localSocket = serverSocket;

        if (null != localSocket) {
            return (InetSocketAddress) localSocket.getLocalSocketAddress();
        } else {
            return null;
        }
    }

    /**
     * Get the start port range we are using
     *
     * @return starting port range
     */
    int getStartPort() {
        return serverBindStartLocalPort;
    }

    /**
     * Get the end port range we are using
     *
     * @return the ending port range
     */
    int getEndPort() {
        return serverBindEndLocalPort;
    }

    /**
     * Daemon where we wait for incoming connections.
     */
    public void run() {
        try {
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Server is ready to accept connections");
            }

            while (!closed) {
                try {
                    if ((null == serverSocChannel) || !serverSocChannel.isOpen()) {
                        openServerSocket();
                        if (null == serverSocChannel) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.warning("Failed to open Server Channel");
                            }
                            break;
                        }
                    }
                    acceptSelector.select();
                    Iterator<SelectionKey> it = acceptSelector.selectedKeys().iterator();

                    while (it.hasNext()) {
                        SelectionKey key = it.next();

                        // remove it
                        it.remove();
                        if (key.isAcceptable()) {
                            ServerSocketChannel nextReady = (ServerSocketChannel) key.channel();
                            SocketChannel inputSocket = nextReady.accept();

                            if ((inputSocket == null) || (inputSocket.socket() == null)) {
                                continue;
                            }

                            MessengerBuilder builder = new MessengerBuilder(inputSocket, transport);

                            try {
                                transport.executor.execute(builder);
                                transport.incrementConnectionsAccepted();
                            } catch (RejectedExecutionException re) {
                                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                    LOG.log(Level.FINE, MessageFormat.format("Executor rejected task : {0}", builder.toString()), re);
                                }
                            }
                        }
                    }
                } catch (ClosedSelectorException cse) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Accept Selector closed");
                    }
                    if (closed) {
                        break;
                    }
                } catch (InterruptedIOException woken) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Accept Thread woken");
                    }
                } catch (IOException e1) {
                    if (closed) {
                        break;
                    }
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING,
                                "[1] ServerSocket.accept() failed on " + serverSocket.getInetAddress() + ":"+ serverSocket.getLocalPort(), e1);
                    }
                } catch (SecurityException e2) {
                    if (closed) {
                        break;
                    }
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "[2] ServerSocket.accept() failed on " + serverSocket.getInetAddress() + ":" + serverSocket.getLocalPort(), e2);
                    }
                }
            }
        } catch (Throwable all) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            }
        } finally {
            synchronized (this) {
                closed = true;
                ServerSocket temp = serverSocket;

                serverSocket = null;
                if (null != temp) {
                    try {
                        temp.close();
                    } catch (IOException ignored) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Exception occurred while closing server socket", ignored);
                        }
                    }
                }
                acceptThread = null;
            }
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Server has been shut down.");
            }
        }
    }

    public List<Integer> rangeCheckShuffle(int start, int end) {
        if ((start < 1) || (start > 65535)) {
            throw new IllegalArgumentException("Invalid start port");
        }

        if ((end < 1) || (end > 65535) || (end < start)) {
            throw new IllegalArgumentException("Invalid end port");
        }

        // fill the inRange array.
        List<Integer> inRange = new ArrayList<Integer>();

        for (int eachInRange = start; eachInRange < end; eachInRange++) {
            inRange.add(eachInRange);
        }
        Collections.shuffle(inRange);
        return inRange;
    }

    private synchronized void openServerSocket() throws IOException, SecurityException {
        serverSocket = null;
        while (true) {
            try {
                synchronized (this) {
                    acceptSelector = SelectorProvider.provider().openSelector();
                    serverSocChannel = ServerSocketChannel.open();
                    InetSocketAddress bindAddress;

                    if (-1 != serverBindPreferedLocalPort) {
                        bindAddress = new InetSocketAddress(serverBindLocalInterface, serverBindPreferedLocalPort);
                        serverSocket = serverSocChannel.socket();
                        int useBufferSize = Math.max(TcpTransport.RecvBufferSize, serverSocket.getReceiveBufferSize());

                        serverSocket.setReceiveBufferSize(useBufferSize);
                        serverSocket.bind(bindAddress, TcpTransport.MaxAcceptCnxBacklog);
                    } else {
                        List<Integer> rangeList = rangeCheckShuffle(serverBindStartLocalPort, serverBindEndLocalPort);

                        while (!rangeList.isEmpty()) {
                            int tryPort = rangeList.remove(0);

                            if (tryPort > serverBindEndLocalPort) {
                                continue;
                            }

                            try {
                                bindAddress = new InetSocketAddress(serverBindLocalInterface, tryPort);
                                serverSocket = serverSocChannel.socket();
                                int useBufferSize = Math.max(TcpTransport.RecvBufferSize, serverSocket.getReceiveBufferSize());

                                serverSocket.setReceiveBufferSize(useBufferSize);
                                serverSocket.bind(bindAddress, TcpTransport.MaxAcceptCnxBacklog);

                                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                                    LOG.info("ServerSocketChannel bound to " + bindAddress + ":" + tryPort);
                                }
                            } catch (SocketException failed) {
                                // this one is busy. try another.
                            } catch (Error err) {
                                // this can occur on some platforms where 2 instances are listenting on the same port
                            }
                        }
                    }
                }
                try {
                    // set the new channel non-blocking
                    serverSocChannel.configureBlocking(false);
                    serverSocChannel.register(acceptSelector, SelectionKey.OP_ACCEPT);
                } catch (ClosedChannelException cce) {
                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.log(Level.FINER, "Channel closed.", cce);
                    }
                }
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Server will accept connections at " + serverSocket.getLocalSocketAddress());
                }
                return;
            } catch (BindException e0) {
                if (-1 != serverBindStartLocalPort) {
                    serverBindPreferedLocalPort = (0 == serverBindStartLocalPort) ? 0 : -1;
                    continue;
                }
                closed = true;
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Cannot bind ServerSocket on " + serverBindLocalInterface + ":" + serverBindPreferedLocalPort, e0);
                }
                return;
            }
        }
    }

    /**
     * An Executor task that creates a messenger from an incoming SocketChannel
     * object.
     */
    private static class MessengerBuilder implements Runnable {

        private SocketChannel inputSocket;
        private TcpTransport transport;
        TcpMessenger newMessenger;

        MessengerBuilder(SocketChannel inputSocket, TcpTransport transport) {
            assert inputSocket.socket() != null;
            this.inputSocket = inputSocket;
            this.transport = transport;
        }

        /**
         * {@inheritDoc}
         */
        public void run() {
            try {
                if (inputSocket != null && inputSocket.isConnected()) {
                    newMessenger = new TcpMessenger(inputSocket, transport);
                }
            } catch (IOException io) {
                // protect against invalid connections
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "Messenger creation failure", io);
                }
            } catch (Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable", all);
                }
            }
        }
    }
}
