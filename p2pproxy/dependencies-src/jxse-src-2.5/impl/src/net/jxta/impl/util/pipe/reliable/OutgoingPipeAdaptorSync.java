/*
 * Copyright (c) 2003-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util.pipe.reliable;


import java.io.IOException;
import java.lang.IllegalStateException;

import net.jxta.pipe.OutputPipe;
import net.jxta.endpoint.Message;
import net.jxta.impl.util.TimeUtils;


/**
 *  And ountgoing pipe adaptor which does not use a thread or queue.
 *  Additionally, the pipe does not need to be provided at construction time.
 *  The send() method blocks until the pipe is specified.
 */
public class OutgoingPipeAdaptorSync implements Outgoing {
    
    private OutputPipe pipe = null;
    private long lastAccessed = 0;
    private boolean closed = false;
    
    public OutgoingPipeAdaptorSync(OutputPipe pipe) {
        
        // Null permitted. Send will block until setPipe is called.
        this.pipe = pipe;
        
        // initialize to some reasonable value
        lastAccessed = TimeUtils.timeNow();
    }
    
    public boolean sendNb(Message msg) throws IOException {
        OutputPipe locPipe;
        
        synchronized (this) {
            locPipe = pipe;
        }
        
        if (closed || locPipe == null) {
            return false;
        }
        
        locPipe.send(msg);
        
        return true;
    }
    
    public boolean send(Message msg) throws IOException {
        
        OutputPipe locPipe;
        
        synchronized (this) {
            while (pipe == null && !closed) {
                try {
                    wait();
                } catch (InterruptedException ignore) {}
            }
            if (closed) {
                return false;
            }
            locPipe = pipe;
        }
        
        return locPipe.send(msg);
    }
    
    public void setPipe(OutputPipe pipe) {
        synchronized (this) {
            if (closed || this.pipe != null) {
                throw new IllegalStateException("Cannot change pipe nor re-open");
            }
            this.pipe = pipe;
            notifyAll();
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    public void close() {
        synchronized (this) {
            if (closed) {
                return;
            }
            
            closed = true;
            
            if (pipe != null) {
                pipe.close();
                pipe = null;
            }
            
            notifyAll();            
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    public long getMinIdleReconnectTime() {
        return 10 * TimeUtils.AMINUTE;
    }
    
    /**
     *  {@inheritDoc}
     *
     * <p/>Default should be "never", otherwise, connection closes while not
     * in active use and ReliableOutputStream does NOT reconnect automatically.
     */
    public long getIdleTimeout() {
        return Long.MAX_VALUE;
    }
    
    /**
     *  {@inheritDoc}
     */
    public void setTimeout(int timeout) {}
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>This is the important tunable: how long to wait on a stale connection.
     */
    public long getMaxRetryAge() {
        return 1 * TimeUtils.AMINUTE;
    }
    
    /**
     *  {@inheritDoc}
     */
    public long getLastAccessed() {
        return lastAccessed;
    }
    
    /**
     *  {@inheritDoc}
     */
    public void setLastAccessed(long time) {
        lastAccessed = time;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public String toString() {
        return ((pipe == null) ? "no pipe yet" : pipe.toString()) + " lastAccessed=" + Long.toString(lastAccessed);
    }
}
