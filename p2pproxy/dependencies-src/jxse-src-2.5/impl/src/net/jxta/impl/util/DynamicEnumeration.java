/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util;


import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

import java.util.NoSuchElementException;


/**
 *  A dynamic {@link java.util.Enumeration} which allows items to added to the
 *  enumeration while it is in use. This is mostly intended for use with
 *  {@link java.io.SequenceInputStream}. When used with {@code SequenceInputStream}
 *  it is important to call {@code close()} on the enumeration before calling
 *  {@code close()} on the stream. Failing to do so will cause the stream to
 *  block as it tries to "drain" the enumeration.
 *
 *  <p/>This class is entirely thread safe. Attempting to use the enumeration
 *  and to add objects from a single thread is not recommended, as it may
 *  deadlock the thread.
 *
 **/
public class DynamicEnumeration implements Enumeration {
    private List sequence = new ArrayList();
    
    private volatile boolean closed = false;
    
    /**
     * Creates a new instance of DynamicEnumeration
     **/
    public DynamicEnumeration() {}
    
    /**
     * Creates a new instance of DynamicEnumeration
     **/
    public DynamicEnumeration(List initial) {
        sequence.addAll(initial);
    }
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>If the Enumeration has not been closed we may have to block until we
     *  have a stream to return.
     **/
    public boolean hasMoreElements() {
        while (!closed) {
            synchronized (sequence) {
                if (!sequence.isEmpty()) {
                    return true;
                }
                
                try {
                    sequence.wait();
                } catch (InterruptedException woken) {
                    Thread.interrupted();
                }
            }
        }
        
        return false;
    }
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>If the Enumeration has not been closed we may have to block until we
     *  have a stream to return.
     **/
    public synchronized Object nextElement() {
        while (!closed) {
            synchronized (sequence) {
                if (sequence.isEmpty()) {
                    try {
                        sequence.wait();
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                    }
                    continue;
                }
                
                Object result = sequence.remove(0);

                return result;
            }
        }
        
        throw new NoSuchElementException("No more elements");
    }
    
    /**
     *  Add another object to the enumeration.
     **/
    public void add(Object add) {
        if (closed) {
            throw new IllegalStateException("Enumeration was closed");
        }
        
        synchronized (sequence) {
            sequence.add(add);
            sequence.notify();
        }
    }
    
    /**
     *  There will be no more objects added.
     **/
    public void close() {
        closed = true;
        synchronized (sequence) {
            sequence.notify();
        }
    }
}
