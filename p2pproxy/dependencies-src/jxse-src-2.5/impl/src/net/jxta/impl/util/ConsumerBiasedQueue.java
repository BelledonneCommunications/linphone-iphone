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

package net.jxta.impl.util;


import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import java.util.List;
import java.util.ArrayList;

import net.jxta.impl.util.TimeUtils;


/**
 *  A queue who's implementation is biased towards effciency in removing 
 elements from the queue.
 *
 *  FIXME 20020511  bondolo@jxta.org    This could be more efficient with a
 *  circular queue implementation, but its a pain to write since we allow the
 *  queue to be resizable.
 *
 *  FIXME 20020511  bondolo@jxta.org    Exercise for the reader: Extend this
 *  class so that it does both LIFO and FIFO.
 *
 *  @deprecated Please convert all code to use the java.util.concurrent BlockingQueue instead.
 *
 **/
@Deprecated
public class ConsumerBiasedQueue extends UnbiasedQueue {
    
    /**
     *  Log4J
     **/
    private static final Logger LOG = Logger.getLogger(ConsumerBiasedQueue.class.getName());
    
    /**
     * Default constructor. 100 element FIFO queue which drops oldest element
     * when full.
     */
    public ConsumerBiasedQueue() {
        this(DEFAULT_MAX_OBJECTS, DROP_OLDEST_OBJECT);
    }
    
    /**
     * Full featured constructor for creating a new ConsumerBiasedQueue.
     *
     * @param size    Queue will be not grow larger than this size. Use
     *    Integer.MAX_VALUE for "unbounded" queue size.
     * @param dropOldest  Controls behaviour of element insertion when the queue is
     * full. If "true" and the queue is full upon a push operation then the
     * oldest element will be dropped to be replaced with the element currently
     * being pushed. If "false" then then newest item will be dropped.
     */
    public ConsumerBiasedQueue(int size, boolean dropOldest) {
        super(size, dropOldest, new ArrayList());
    }
    
    /**
     *  Flush the queue of all pending objects.
     **/
    @Override
    public void clear() {
        synchronized (queue) {
            super.clear();
        }
    }
    
    @Override
    public synchronized boolean push(Object obj) {
        synchronized (queue) {
            boolean pushed = super.push(obj);

            queue.notify(); // inform someone who is waiting. we dont have to tell everyone though
            return pushed;
        }
    }
    
    /**
     * Push an object onto the queue. If the queue is full then the push will
     *  wait for up to "timeout" milliseconds to push the object. At the end of
     *  "timeout" milliseconds, the push will either return false or remove the
     *  oldest item from the queue and insert "obj". This behaviour is contolled
     *  by the constructor parameter "dropOldest".
     *
     *  This method, unlike all others is synchronized. This creates a
     *  bottleneck for producers seperate from the primary lock on the "queue"
     *  member. This reduces contention on the primary lock which benefits users
     *  who are popping items from the queue (Consumers).
     *
     * @param obj Object to be pushed onto the queue
     * @param timeout Time in milliseconds to try to insert the item into a full
     *  queue. Per Java standards, a timeout of "0" (zero) will wait indefinitly.
     *  Negative values force no wait period at all.
     *  @return true if the object was intersted into the queue, otherwise false.
     *  @throws InterruptedException    if the operation is interrupted before
     *      the timeout interval is completed.
     **/
    @Override
    public synchronized boolean push(Object obj, long timeout) throws InterruptedException {
        return super.push(obj, timeout);
    }
    
    /**
     * Return next obj in the queue if there is one.
     *
     * @return Object, null if the queue is empty
     **/
    @Override
    public Object pop() {
        synchronized (queue) {
            return super.pop();
        }
    }
    
    /**
     *  Returns an array of objects, possibly empty, from the queue.
     *
     *  @param maxObjs  the maximum number of items to return.
     *  @return an array of objects, possibly empty containing the returned
     *  queue elements.
     **/
    @Override
    public Object[] popMulti(int maxObjs) {
        synchronized (queue) {
            return super.popMulti(maxObjs);
        }
    }
    
    /**
     *  Set how many objects this queue may store. Note that if there are more
     *  objects already in the queue than the specified amount then the queue
     *  will retain its current capacity.
     *
     *  @param maxObjs  The number of objects which the queue must be able to
     *  store.
     **/
    @Override
    public void setMaxQueueSize(int maxObjs) {
        synchronized (queue) {
            super.setMaxQueueSize(maxObjs);
        }
    }
    
    /**
     *  Return the number of elements currently in the queue. This method is
     *  useful for statistical sampling, but should not be used to determine
     *  program logic due to the multi-threaded behaviour of these queues. You
     *  should use the return values and timeout behaviour of the push() and
     * pop() methods to regulate how you use the queue.
     *
     *  @return the number of elements currently in the queue. Be warned that
     *  even two sequential calls to this method may return different answers
     *  due to activity on other threads.
     *
     **/
    @Override
    public int getCurrentInQueue() {
        synchronized (queue) {
            return super.getCurrentInQueue();
        }
    }
    
    /**
     *  Return the average number of elements in the queue at Enqueue time.
     *
     *  @return average number of elements which were in the queue at during all
     *  of the "push" operations which returned a "true" result. Does not
     *  include the item being pushed. If no elements have ever been enqueued
     *  then "NaN" will be returned.
     **/
    @Override
    public double getAvgInQueueAtEnqueue() {
        synchronized (queue) {
            return super.getAvgInQueueAtEnqueue();
        }
    }
    
    /**
     *  Return the average number of elements in the queue at dequeue time.
     *
     *  @return average number of elements which were in the queue at during all
     *  of the "pop" operations which returned a non-null result. Includes the
     * item being "pop"ed in the average. If no elements have ever been dequeued
     *  then "NaN" will be returned.
     **/
    @Override
    public double getAvgInQueueAtDequeue() {
        synchronized (queue) {
            return super.getAvgInQueueAtDequeue();
        }
    }
}
