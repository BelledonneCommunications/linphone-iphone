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

package net.jxta.pipe;


import java.io.IOException;

import net.jxta.endpoint.Message;
import net.jxta.id.ID;
import net.jxta.protocol.PipeAdvertisement;


/**
 * OuputPipe defines the interface for sending messages from a
 * {@link net.jxta.pipe.PipeService}.
 *
 * <p/>Application that want to send messages onto a Pipe must fist get
 * an {@link net.jxta.pipe.OutputPipe} from the {@link net.jxta.pipe.PipeService}.
 *
 * @see net.jxta.pipe.PipeService
 * @see net.jxta.pipe.InputPipe
 * @see net.jxta.endpoint.Message
 * @see net.jxta.protocol.PipeAdvertisement
 */

public interface OutputPipe {
    
    /**
     * Send a message through the pipe
     *
     * <p/><b>WARNING:</b> The message object used when sending a pipe message
     * should not be reused or modified after the {@link #send(Message)} call is
     * made. Concurrent modification of messages will produce unexpected result.
     *
     * @param msg is the PipeMessage to be sent.
     * @return boolean <code>true</code> if the message has been sent otherwise
     * <code>false</code>. <code>false</code>. is commonly returned for
     * non-error related congestion, meaning that you should be able to send
     * the message after waiting some amount of time.
     * @exception IOException output pipe error
     *
     */
    public boolean send(Message msg) throws IOException;
    
    /**
     * close the pipe
     *
     */
    public void close();
    
    /**
     *  Returns <code>true</code> if this pipe is closed and no longer
     *  accepting messages to be sent. The pipe should be discarded.
     *
     *  @return <code>true</code> if this pipe is closed, otherwise
     *  <code>false</code>.
     */
    boolean isClosed();
    
    /**
     *  Gets the pipe type
     *
     * @return    The type
     */
    public String getType();
    
    /**
     *  Gets the pipe id
     *
     * @return    The type
     */
    public ID getPipeID();
    
    /**
     *  Gets the pipe name
     *
     * @return    The name
     */
    public String getName();
    
    /**
     *  Gets the pipe advertisement
     *
     * @return    The advertisement
     */
    public PipeAdvertisement getAdvertisement();
}

