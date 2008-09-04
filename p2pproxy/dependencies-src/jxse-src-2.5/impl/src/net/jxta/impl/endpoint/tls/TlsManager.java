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

package net.jxta.impl.endpoint.tls;


import java.io.DataInputStream;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import java.io.IOException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;

import net.jxta.impl.util.TimeUtils;

import net.jxta.impl.endpoint.tls.TlsConn.HandshakeState;


/**
 * Manages the connection pool between peers.
 **/
class TlsManager implements EndpointListener {
    
    /**
     *  Log4J Logger
     **/
    private final static transient Logger LOG = Logger.getLogger(TlsManager.class.getName());
    
    /**
     *  Transport we are working for.
     **/
    private TlsTransport transport = null;
    
    /**
     * Hash table for known connections
     *
     *  <ul>
     *  <li>keys are {@link String } containing {@link net.jxta.peer.PeerID#getUniqueValue() PeerID.getUniqueValue()}</li>
     *  <li>values are {@link TlsConn}<li>
     *  </ul>
     **/
    private Map connections = new HashMap();
    
    /**
     *  The last time at which we printed a warning about discarding messages
     *  due to no authentication.
     **/
    private long lastNonAuthenticatedWarning = 0;
    
    /**
     *  Standard Constructor for TLS Manager
     **/
    TlsManager(TlsTransport tp) {
        this.transport = tp;
    }
    
    /**
     *  Close this manager. This involves closing all registered connections.
     *
     **/
    void close() {
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Shutting down all connections");
        }
        
        synchronized (connections) {
            Iterator eachConnection = connections.values().iterator();
            
            while (eachConnection.hasNext()) {
                TlsConn aConnection = (TlsConn) eachConnection.next();
                
                try {
                    aConnection.close(HandshakeState.CONNECTIONDEAD);
                } catch (IOException ignored) {
                    if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                        LOG.info("Non-fatal problem shutting down connection to " + aConnection);
                    }
                }
                
                eachConnection.remove();
            }
        }
    }
    
    /**
     *  Returns or creates a TLS Connection to the specified peer. If an
     *  existing connection exists, it will be returned.
     *
     *  @param dstAddr the EndpointAddress of the remote peer.
     *  @return A TLS Connection or null if the connection could not be opened.
     **/
    TlsConn getTlsConn(EndpointAddress dstAddr) {
        
        if (null == transport.credential) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Not authenticated. Cannot open connections.");
            }
            
            return null;
        }
        
        boolean startHandshake = false;
        
        // see if we have an existing conn, and if so, then reuse it
        // if it has not timed out.
        String paddr = dstAddr.getProtocolAddress();
        
        TlsConn conn = null;
        
        synchronized (connections) {
            conn = (TlsConn) connections.get(paddr);
            
            // remove it if it is dead
            if (null != conn) {
                if ((HandshakeState.CONNECTIONDEAD == conn.getHandshakeState())
                        || (HandshakeState.HANDSHAKEFAILED == conn.getHandshakeState())) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Removing connection for: " + paddr);
                    }
                    connections.remove(paddr);
                    conn = null;
                }
            }
            
            // create the connection info entry as needed
            if (null == conn) {
                try {
                    conn = new TlsConn(transport, dstAddr, true); // true means client
                } catch (Exception failed) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Failed making connection to " + paddr, failed);
                    }
                    
                    return null;
                }
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Adding connection for: " + paddr);
                }
                connections.put(paddr, conn);
                startHandshake = true;
            }
        }
        
        // if we got to be the first one to start the handshake then do it here.
        // We do this outside of the synchro block so that others can enter the
        // state machine.
        if (startHandshake) {
            try {
                // OK. We are originating the connection:
                // Open the connection (returns when handshake is completed)
                // or throws an IOException if a TLS internal error occurs.
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Start of client handshake for " + paddr);
                }
                
                conn.finishHandshake();
            } catch (Throwable e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed making connection to " + paddr, e);
                }
                
                synchronized (connections) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Removing connection for: " + paddr);
                    }
                    connections.remove(paddr);
                }
                try {
                    conn.close(HandshakeState.HANDSHAKEFAILED);
                } catch (IOException ignored) {
                    ;
                }
                
                return null;
            }
        }
        
        do {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("getting " + conn);
            }
            
            synchronized (conn) {
                HandshakeState currentState = conn.getHandshakeState();
                
                if ((HandshakeState.SERVERSTART == currentState) || (HandshakeState.CLIENTSTART == currentState)) {
                    // wait for the handshake to get going on another thread.
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Sleeping until handshake starts for " + paddr);
                    }
                    
                    try {
                        conn.wait(TimeUtils.ASECOND);
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                    }
                    continue;
                } else if (HandshakeState.HANDSHAKESTARTED == currentState) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Handshake in progress for " + paddr);
                    }
                    
                    try {
                        // sleep forever waiting for the state to change.
                        conn.wait(200);
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                    }
                    continue;
                } else if (HandshakeState.HANDSHAKEFINISHED == currentState) {
                    
                    if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                        LOG.info("Returning active connection to " + paddr);
                    }
                    
                    conn.lastAccessed = TimeUtils.timeNow(); // update idle timer
                    
                    return conn;
                } else if (HandshakeState.HANDSHAKEFAILED == currentState) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Handshake failed. " + paddr + " unreachable");
                    }
                    
                    return null;
                } else if (HandshakeState.CONNECTIONDEAD == currentState) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Connection dead for " + paddr);
                    }
                    
                    return null;
                } else if (HandshakeState.CONNECTIONCLOSING == currentState) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Connection closing for " + paddr);
                    }
                    
                    return null;
                } else {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("Unhandled Handshake state: " + currentState);
                    }
                }
            }
        } while (true);
    }
    
    /**
     * Handle an incoming message from the endpoint. This method demultiplexes
     * incoming messages to the connection objects by their source address.
     *
     * <p/>Several types of messages may be received for a connection:
     *
     * <ul>
     * <li>TLS Elements</li>
     *  <li>Element Acknowledgements</li>
     * </ul>
     *
     * @param msg is the incoming message
     * @param srcAddr is the address of the source of the message
     * @param dstAddr is the address of the destination of the message
     **/
    public void processIncomingMessage(Message msg, EndpointAddress srcAddr, EndpointAddress dstAddr) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Starts for " + msg);
        }
        
        if (null == transport.credential) {
            // ignore ALL messages until we are authenticated.
            if (TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), lastNonAuthenticatedWarning) > TimeUtils.AMINUTE) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("NOT AUTHENTICATED--Discarding all incoming messages");
                }
                
                lastNonAuthenticatedWarning = TimeUtils.timeNow();
            }
            
            return;
        }
        
        // determine if its a retry.
        MessageElement retryElement = msg.getMessageElement(JTlsDefs.TLSNameSpace, JTlsDefs.RETR);
        boolean retrans = (null != retryElement);
        
        if (retrans) {
            msg.removeMessageElement(retryElement);
            retryElement = null;
        }
        
        int seqN = getMsgSequenceNumber(msg);
        
        // Extract unique part of source address
        String paddr = srcAddr.getProtocolAddress();
        
        TlsConn conn = null;
        
        boolean serverStart = false;
        
        synchronized (connections) {
            // Will be in our hash table unless this is for a first time
            // incoming connection request
            conn = (TlsConn) connections.get(paddr);
            
            if (null != conn) {
                // check if the connection has idled out and remote is asking for a restart.
                if (TlsTransport.ACT_AS_SERVER && (1 == seqN)) {
                    synchronized (conn) {
                        long idle = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), conn.lastAccessed);
                        
                        if (idle > transport.MIN_IDLE_RECONNECT) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.warning("Restarting : " + conn + " which has been idle for " + idle + " millis");
                            }
                            try {
                                conn.close(HandshakeState.CONNECTIONDEAD);
                            } catch (IOException ignored) {
                                ;
                            }
                        }
                    }
                }
                
                // remove it if it is dead
                if ((HandshakeState.CONNECTIONDEAD == conn.getHandshakeState())
                        || (HandshakeState.HANDSHAKEFAILED == conn.getHandshakeState())) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Removing connection for: " + paddr);
                    }
                    connections.remove(paddr);
                    conn = null;
                }
            }
            
            // we don't have a connection to this destination, make a new connection if seqn#1
            if (null == conn) {
                if (TlsTransport.ACT_AS_SERVER && (1 == seqN)) {
                    try {
                        conn = new TlsConn(transport, srcAddr, false); // false means Server
                    } catch (Exception failed) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Failed making connection for" + paddr, failed);
                        }
                        return;
                    }
                    
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Adding connection for: " + paddr);
                    }
                    connections.put(paddr, conn);
                    serverStart = true;
                } else {
                    // Garbage from an old connection. discard it
                    
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning(msg + " is not start of handshake (seqn#" + seqN + ") for " + paddr);
                    }
                    
                    msg.clear();
                    return;
                }
            }
        }
        
        // if this is a new connection, get it started.
        if (serverStart) {
            try {
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Start of SERVER handshake for " + paddr);
                }
                
                // Queue message up for TlsInputStream on that connection
                conn.tlsSocket.input.queueIncomingMessage(msg);
                
                // Start the TLS Server and complete the handshake
                conn.finishHandshake(); // open the TLS connection
                
                conn.lastAccessed = TimeUtils.timeNow();
                
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Handshake complete for SERVER TLS for: " + paddr);
                }
                
                return;
            } catch (Throwable e) {
                // Handshake failure or IOException
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "TLS Handshake failure for connection: " + paddr, e);
                }
                
                synchronized (connections) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Removing connection for: " + paddr);
                    }
                    connections.remove(paddr);
                }
                try {
                    conn.close(HandshakeState.HANDSHAKEFAILED);
                } catch (IOException ignored) {
                    ;
                }
                
                return;
            }
        }
        
        // handle an ongoing connection.
        do {
            HandshakeState currentState;
            
            synchronized (conn) {
                if (retrans) {
                    conn.retrans++;
                    
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("retrans received, " + conn.retrans + " total.");
                    }
                    retrans = false;
                }
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Process incoming message for " + conn);
                }
                
                currentState = conn.getHandshakeState();
                
                if ((HandshakeState.HANDSHAKESTARTED == currentState) || (HandshakeState.HANDSHAKEFINISHED == currentState)
                        || (HandshakeState.CONNECTIONCLOSING == currentState)) {// we will process the message once we get out of sync.
                } else if (HandshakeState.CONNECTIONDEAD == currentState) {
                    // wait for the handshake to get going on another thread.
                    if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                        LOG.info("Connection failed, discarding msg with seqn#" + seqN + " for " + paddr);
                    }
                    
                    return;
                } else if ((HandshakeState.SERVERSTART == currentState) || (HandshakeState.CLIENTSTART == currentState)) {
                    // wait for the handshake to get going on another thread.
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Sleeping msg with seqn#" + seqN + " until handshake starts for " + paddr);
                    }
                    
                    try {
                        conn.wait(TimeUtils.AMINUTE);
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                    }
                    continue;
                } else if (HandshakeState.HANDSHAKEFAILED == currentState) {
                    // wait for the handshake to get going on another thread.
                    if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                        LOG.info("Handshake failed, discarding msg with seqn#" + seqN + " for " + paddr);
                    }
                    
                    return;
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unexpected state : " + currentState);
                    }
                }
            }
            
            // Process any message outside of the sync on the connection.
            if ((HandshakeState.HANDSHAKESTARTED == currentState) || (HandshakeState.HANDSHAKEFINISHED == currentState)
                    || (HandshakeState.CONNECTIONCLOSING == currentState)) {
                // process any ACK messages.
                Iterator eachACK = msg.getMessageElements(JTlsDefs.TLSNameSpace, JTlsDefs.ACKS);
                
                while (eachACK.hasNext()) {
                    MessageElement elt = (MessageElement) eachACK.next();
                    
                    eachACK.remove();
                    
                    int sackCount = ((int) elt.getByteLength() / 4) - 1;
                    
                    try {
                        DataInputStream dis = new DataInputStream(elt.getStream());
                        
                        int seqack = dis.readInt();
                        
                        int[] sacs = new int[sackCount];
                        
                        for (int eachSac = 0; eachSac < sackCount; eachSac++) {
                            sacs[eachSac] = dis.readInt();
                        }
                        
                        Arrays.sort(sacs);
                        
                        // take care of the ACK here;
                        conn.tlsSocket.output.ackReceived(seqack, sacs);
                    } catch (IOException failed) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Failure processing ACK", failed);
                        }
                    }
                }
                
                if (0 == seqN) {
                    return;
                }
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Queue " + msg + " seqn#" + seqN + " for " + conn);
                }
                
                // Queue message up for TlsInputStream on that connection
                TlsSocket bound = conn.tlsSocket;
                
                if (null != bound) {
                    bound.input.queueIncomingMessage(msg);
                }
                
                return;
            }
        } while (true);
    }
    
    /**
     * getMsgSequenceNumber
     *
     * @param msg  Input message
     * @return int sequence number or 0 (zero) if no tls records in message.
     **/
    private static int getMsgSequenceNumber(Message msg) {
        
        int seqN = 0;
        
        Iterator eachElement = msg.getMessageElements(JTlsDefs.TLSNameSpace, JTlsDefs.BLOCKS);
        
        while (eachElement.hasNext()) {
            MessageElement elt = (MessageElement) eachElement.next();
            
            try {
                seqN = Integer.parseInt(elt.getElementName());
            } catch (NumberFormatException e) {
                // This element was not a TLS element. Get the next one
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Bad tls record name=" + elt.getElementName());
                }
                
                eachElement.remove();
                continue;
            }
            
            break;
        }
        
        return seqN;
    }
}
