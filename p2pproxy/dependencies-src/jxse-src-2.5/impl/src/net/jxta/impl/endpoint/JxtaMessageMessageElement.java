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

package net.jxta.impl.endpoint;


import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.MimeMediaType;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.WireFormatMessageFactory;


/**
 * A Message Element using a JXTA Message as the element data
 *
 *  @see net.jxta.endpoint.Message
 *  @see net.jxta.endpoint.MessageElement
 **/
public class JxtaMessageMessageElement extends MessageElement {
    
    /**
     *  Log4J Logger
     **/
    private final static transient Logger LOG = Logger.getLogger(JxtaMessageMessageElement.class.getName());
    
    /**
     *  The Message which is the data for this message element.
     **/
    protected final Message msg;
    
    /**
     *  A serialized form of the message.
     **/
    protected transient net.jxta.endpoint.WireFormatMessage serial;
    
    /**
     * Create a new Message Element. The contents of the provided message are 
     * <b>not</b> copied during construction.
     *
     * @param name Name of the MessageElement. May be the empty string ("") if
     * the MessageElement is not named.
     * @param type Type of the MessageElement. null is the same as specifying
     * the type "Application/Octet-stream".
     * @param msg A message which will be used as the element content for this
     * message.
     * @param sig optional message digest/digital signature element or null if
     * no signature is desired.
     **/
    public JxtaMessageMessageElement(String name, MimeMediaType type, Message msg, MessageElement sig) {
        super(name, type, sig);
        
        this.msg = msg;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        if (target instanceof MessageElement) {
            if (!super.equals(target)) {
                return false;
            }
            
            if (target instanceof JxtaMessageMessageElement) {
                JxtaMessageMessageElement likeMe = (JxtaMessageMessageElement) target;
                
                return super.equals(likeMe) && msg.equals(likeMe.msg);
            } else {
                // have to do a slow stream comparison.
                // XXX 20020615 bondolo@jxta.org the performance of this could be much improved.
                try {
                    MessageElement likeMe = (MessageElement) target;
                    
                    InputStream myStream = getStream();
                    InputStream itsStream = likeMe.getStream();
                    
                    int mine;
                    int its;

                    do {
                        mine = myStream.read();
                        its = itsStream.read();
                        
                        if (mine != its) {
                            return false;
                        }       // content didn't match (includes EOF check).
                        
                    } while (-1 != mine);
                    
                    return true;
                } catch (IOException fatal) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE, "MessageElements could not be compared.", fatal);
                    }
                    
                    IllegalStateException failure = new IllegalStateException("MessageElements could not be compared.");

                    failure.initCause(fatal);
                    
                    throw failure;
                }
            }
        }
        
        return false; // not a message element
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public int hashCode() {
        int result = super.hashCode() * 6037 + // a prime
                msg.hashCode();
        
        return (0 != result) ? result : 1;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public String toString() {
        throw new UnsupportedOperationException("Cannot Generate String for this message element type.");
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public long getByteLength() {
        initSerial();
       
        return serial.getByteLength();
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public byte[] getBytes(boolean copy) {
        initSerial();
       
        ByteArrayOutputStream baos = new ByteArrayOutputStream((int) serial.getByteLength());
        
        try {
            sendToStream(baos);
            
            baos.close();
        } catch (IOException failed) {
            throw new IllegalStateException("failed to generate byte stream");
        }
        
        return baos.toByteArray();
    }
    
    /**
     *  {@inheritDoc}
     **/
    public InputStream getStream() throws IOException {
        initSerial();
        
        return serial.getStream();
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public void sendToStream(OutputStream sendTo) throws IOException {
        initSerial();
        
        serial.sendToStream(sendTo);
    }
    
    /**
     *  Returns a copy of the message which backs this element.
     *
     *  @return Returns a copy of the message which backs this element.
     **/
    public Message getMessage() {
        return msg.clone();
    }
    
    /**
     *  Generates the serialized representation of the message.
     **/
    private synchronized void initSerial() {
        if (null == serial) {
            serial = WireFormatMessageFactory.toWire(msg, type, null);
        }
    }
}
