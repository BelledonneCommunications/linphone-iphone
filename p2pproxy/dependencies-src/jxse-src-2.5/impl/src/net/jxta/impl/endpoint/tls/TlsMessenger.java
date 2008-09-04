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


import java.io.*;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.endpoint.*;
import net.jxta.impl.endpoint.*;
import net.jxta.impl.endpoint.tls.TlsConn.HandshakeState;


/**
 * This class implements sending messages through a TLS connection.
 */
public class TlsMessenger extends BlockingMessenger {
    
    private static final Logger LOG = Logger.getLogger(TlsMessenger.class.getName());
    
    private TlsTransport transport = null;
    private TlsConn conn = null;
    
    /**
     *  The source address of messages sent on this messenger.
     */
    private final EndpointAddress srcAddress;
    
    private final MessageElement srcAddressElement;
    
    TlsMessenger(EndpointAddress destAddress, TlsConn conn, TlsTransport tp) {
        
        // No need for self destruction.
        super(tp.getPeerGroup().getPeerGroupID(), destAddress, false);
        
        if (conn == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("null TLS connection!");
            }
            throw new IllegalArgumentException("null TLS connection!");
        }
        
        this.conn = conn;
        this.transport = tp;
        
        this.srcAddress = transport.getPublicAddress();
        
        srcAddressElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NAME, srcAddress.toString()
                ,
                (MessageElement) null);
    }
    
    /*
     * The cost of just having a finalize routine is high. The finalizer is
     * a bottleneck and can delay garbage collection all the way to heap
     * exhaustion. Leave this comment as a reminder to future maintainers.
     * Below is the reason why finalize is not needed here.
     *
     * These messengers never go to the application layer. The endpoint code
     * always invokes close when needed.

     protected void finalize() {
     }

     */
    
    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void closeImpl() {
        super.close();
        conn = null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isIdleImpl() {
        // No need for self destruction.
        return false;
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>The peer that is the destination is the logical address
     */
    @Override
    public EndpointAddress getLogicalDestinationImpl() {
        return new EndpointAddress("jxta", dstAddress.getProtocolAddress(), null, null);
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void sendMessageBImpl(Message message, String service, String serviceParam) throws IOException {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Starting send for " + message);
        }
        
        // check if the connection has died.
        if (HandshakeState.CONNECTIONDEAD == conn.getHandshakeState()) {

            // FIXME - jice@jxta.org 20040413: This will do but it causes the below exception to be shown as the cause of the
            // failure, which is not true: nobody realy closed the messenger before it failed. It failed first.  Also, it used to
            // shutdown this messenger, now it does not. What does is the call to closeImpl() that follows our IOException...(and
            // that's how it should be). Transports should get a deeper retrofit eventually.

            close();
        }
        
        if (isClosed()) {
            IOException failure = new IOException("Messenger is closed, it cannot be used to send messages.");
            
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, failure.getMessage(), failure);
            }
            
            throw failure;
        }
        
        // Set the message with the appropriate src and dest address
        message.replaceMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NS, srcAddressElement);
        
        MessageElement dstAddressElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NAME
                ,
                getDestAddressToUse(service, serviceParam).toString(), (MessageElement) null);
        
        message.replaceMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NS, dstAddressElement);
        
        // Give the message to the TLS connection
        try {
            conn.sendMessage(message);
        } catch (IOException caught) {
            close();
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Message send to \'" + dstAddress + "\' failed for " + message, caught);
            }
            throw caught;
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Message send to \'" + dstAddress + "\' succeeded for " + message);
        }
    }    
}
