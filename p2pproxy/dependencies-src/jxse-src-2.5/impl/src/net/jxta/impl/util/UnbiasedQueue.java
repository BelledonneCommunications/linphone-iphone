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


import java.util.ArrayList;
import java.util.List;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.impl.util.TimeUtils;


/**
 *  A generic queue class. This queue is explicitly <b>NOT</b> a synchronized queue.
 *
 *  <p/>FIXME 20020511  bondolo@jxta.org    This could be more efficient with a
 *  circular queue implementation, but its a pain to write since we allow the
 *  queue to be resizable.
 *
 *  <p/>FIXME 20020511  bondolo@jxta.org    Exercise for the reader: Extend this
 *  class so that it does both LIFO and FIFO.
 *
 *  <p/>FIXME 20020910  bondolo@jxta.org    Needs an optional listener callback
 *  for droppped elments.
 *
 *  <p/>FIXME 20020910  bondolo@jxta.org    Needs an optional "strategy" for
 *  element insertion and removal.
 *
 *  @deprecated Please convert all code to use the java.util.concurrent BlockingQueue instead.
 *
 */
@Deprecated
public class UnbiasedQueue {
    
    /**
     *  Logger
     */
    private static final Logger LOG = Logger.getLogger(UnbiasedQueue.class.getName());
    
    /**
     *  Default number of queue elements.
     */
    protected static final int DEFAULT_MAX_OBJECTS = 100;
    
    /**
     *  Default object dropping behaviour
     */
    protected static final boolean DROP_OLDEST_OBJECT = true;
    
    /**
     *  Number of milliseconds between notifications that objects are being dropped.
     */
    protected static final long DROPPED_OBJECT_WARNING_INTERVAL = 10 * TimeUtils.ASECOND;
    
    /**
     *  Contains the objects we currently have queued.
     */
    protected List<Object> queue = null;
    
    /**
     *  The maximum number of objects we will hold in the queue at one time.
     */
    protected int maxObjects;
    
    /**
     *  If true the queue is being closed and is currently in the process of
     *  being flushed. All new "push" requests will be refused.
     */
    protected volatile boolean closeFlag = false;
    
    /**
     *  When we need to drop objects, drop the oldest obj.
     */
    protected boolean dropOldestObject = true;
    
    /**
     * total number of objects which have been enqueued into this queue
     */
    protected long numEnqueued = 0;
    
    /**
     * sum of queue sizes at enqueue time.
     */
    protected long sumOfQueueSizesEnqueue = 0;
    
    /**
     * total number of objects which have been dequeued from this queue
     */
    protected long numDequeued = 0;
    
    /**
     * sum of queue sizes at dequeue time.
     */
    protected long sumOfQueueSizesDequeue = 0;
    
    /**
     * the number of objects we have dropped since we began working.
     */
    protected long numDropped = 0;
    
    /**
     * absolute time in millis when it will be ok to display a obj about
     * dropping objects. We throttle this so that there is a chance to do work
     * rather than just always spewing warnings.
     */
    protected long nextDroppedWarn = 0L;
    
    /**
     *  An inner class for wrapping arbitrary queues with synchronization.
     */
    protected static class SynchronizedQueue extends UnbiasedQueue {
        UnbiasedQueue innerqueue;
        
        /**
         *  Constructs a SynchronizedQueue from an UnbiasedQueue
         *  instance.
         */
        public SynchronizedQueue(UnbiasedQueue queue) {
            innerqueue = queue;
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public boolean isClosed() {
            synchronized (innerqueue.queue) {
                return innerqueue.isClosed();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public void close() {
            synchronized (innerqueue.queue) {
                innerqueue.close();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public void clear() {
            synchronized (innerqueue.queue) {
                innerqueue.clear();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public boolean push(Object obj) {
            synchronized (innerqueue.queue) {
                return innerqueue.push(obj);
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public boolean pushBack(Object obj) {
            synchronized (innerqueue.queue) {
                return innerqueue.pushBack(obj);
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public boolean push(Object obj, long timeout) throws InterruptedException {
            synchronized (innerqueue.queue) {
                return innerqueue.push(obj, timeout);
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public boolean pushBack(Object obj, long timeout) throws InterruptedException {
            synchronized (innerqueue.queue) {
                return innerqueue.pushBack(obj, timeout);
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public Object peek() {
            synchronized (innerqueue.queue) {
                return innerqueue.peek();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public Object pop() {
            synchronized (innerqueue.queue) {
                return innerqueue.pop();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public Object pop(long timeout) throws InterruptedException {
            synchronized (innerqueue.queue) {
                return innerqueue.pop(timeout);
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public Object[] popMulti(int maxObjs) {
            synchronized (innerqueue.queue) {
                return innerqueue.popMulti(maxObjs);
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public int getMaxQueueSize() {
            synchronized (innerqueue.queue) {
                return innerqueue.getMaxQueueSize();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public void setMaxQueueSize(int maxObjs) {
            synchronized (innerqueue.queue) {
                innerqueue.setMaxQueueSize(maxObjs);
            }
        }
        
        @Override
        public int getCurrentInQueue() {
            synchronized (innerqueue.queue) {
                return innerqueue.getCurrentInQueue();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public long getNumEnqueued() {
            synchronized (innerqueue.queue) {
                return innerqueue.getNumEnqueued();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public double getAvgInQueueAtEnqueue() {
            synchronized (innerqueue.queue) {
                return innerqueue.getAvgInQueueAtEnqueue();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public long getNumDequeued() {
            synchronized (innerqueue.queue) {
                return innerqueue.getNumDequeued();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public double getAvgInQueueAtDequeue() {
            synchronized (innerqueue.queue) {
                return innerqueue.getAvgInQueueAtDequeue();
            }
        }
        
        /**
         *  {@inheritDoc}
         */
        @Override
        public long getNumDropped() {
            synchronized (innerqueue.queue) {
                return innerqueue.getNumDropped();
            }
        }
    }
    
    /**
     *  Returns a synchronized (thread-safe) list backed by the specified queue.
     *  Most UnbiasedQueue subclasses are either unsynchronized or internally
     *  synchronized. If you need to do any atomic operations upon
     *  UnbiasedQueues (or subclasses) then this method should be used to
     *  "wrap" the queue with synchronization.
     *
     *  <p/>In order to guarantee serial access, it is critical that all access
     * to the backing queue is accomplished through the returned queue.
     *
     *  @param queue the queue to be "wrapped" in a synchronized queue.
     */
    public static UnbiasedQueue synchronizedQueue(UnbiasedQueue queue) {
        return new SynchronizedQueue(queue);
    }
    
    /**
     * Default constructor. 100 element FIFO queue which drops oldest element
     * when full.
     */
    public UnbiasedQueue() {
        this(DEFAULT_MAX_OBJECTS, DROP_OLDEST_OBJECT);
    }
    
    /**
     * Full featured constructor for creating a new UnBiasedQueue.
     *
     * @param maxsize   Queue will not grow larger than this size. Use
     *  {@link java.lang.Integer#MAX_VALUE} for "unbounded" queue size.
     * @param dropOldest    Controls behaviour of element insertion when the
     * queue is full. If <tt>true</tt> and the queue is full upon a
     * {@link #push(Object) push} operation then the oldest element will be
     * dropped to be replaced with the element currently being pushed. If
     * <tt>false</tt> then the element will not be inserted if the queue is full.
     */
    public UnbiasedQueue(int maxsize, boolean dropOldest) {
        this(maxsize, dropOldest, new ArrayList<Object>());
    }
    
    /**
     * Full featured constructor for creating a new UnBiasedQueue.
     *
     *  @param maxsize   Queue will not grow larger than this size. Use
     *  {@link java.lang.Integer#MAX_VALUE} for "unbounded" queue size.
     *  @param dropOldest    Controls behaviour of element insertion when the
     *  queue is full. If <tt>true</tt> and the queue is full upon a
     *  {@link #push(Object) push} operation then the oldest element will be
     *  dropped to be replaced with the element currently being pushed. If
     *  <tt>false</tt> then the element will not be inserted if the queue is
     * full.
     *  @param queue    the List class instance to use. This does not need to be
     *  a synchronized list class. (and it works more effciently if it isn't).
     */
    public UnbiasedQueue(int maxsize, boolean dropOldest, List<Object> queue) {
        if (maxsize <= 0) {
            throw new IllegalArgumentException("size must be > 0");
        }
        
        if (null == queue) {
            throw new IllegalArgumentException("queue must be non-null");
        }
        
        maxObjects = maxsize;
        this.queue = queue;
        closeFlag = false;
        
        dropOldestObject = dropOldest;
    }
    
    /**
     * {@inheritDoc}
     *
     *  <p/>A diagnostic toString implementation.
     */
    @Override
    public synchronized String toString() {
        
        return this.getClass().getName() + " :" + " size=" + getCurrentInQueue() + " capacity=" + getMaxQueueSize() + " enqueued="
                + getNumEnqueued() + " avgAtEnqueue=" + getAvgInQueueAtEnqueue() + " dequeued=" + getNumDequeued()
                + " avgAtDequeue=" + getAvgInQueueAtDequeue();
    }
    
    /**
     *  Atomically return whether or not this queue has been closed. Closed
     *  queues will not accept "push" requests, but elements will still be
     *  returned with "pop".
     *
     *  @return boolean indicating whether this queue has been closed.
     */
    public boolean isClosed() {
        return closeFlag; // closeFlag is volatile.
    }
    
    /**
     * Close the queue. This will prevent any further objects from being enqueued.
     */
    public void close() {
        closeFlag = true;
        synchronized (queue) {
            queue.notifyAll();
        }
    }
    
    /**
     *  Flush the queue of all pending objects.
     */
    public void clear() {
        numDropped += queue.size();
        queue.clear();
    }
    
    /**
     *  Attempt to push an object onto the queue. If the queue is full then the
     *  object will not be pushed. This method does not use any synchronization
     *  and should not be used if other threads are using {@link #pop(long)} to
     *  retrieve elements.
     *
     *  @param obj object to push
     *  @return true if the obj was pushed, otherwise false.
     */
    public boolean push(Object obj) {
        if (queue.size() >= maxObjects) {
            return false;
        }
        
        numEnqueued++;
        sumOfQueueSizesEnqueue += queue.size();
        queue.add(obj);
        
        return true;
    }
    
    /**
     *  Attempt to push an object back at the head the queue. If the queue is
     *  full then the object will not be pushed. This method does not use any
     * synchronization and should not be used if other threads are using
     * {@link #pop(long)} to retrieve elements.
     *
     *  @param obj object to push
     *  @return true if the obj was pushed, otherwise false.
     */
    public boolean pushBack(Object obj) {
        if (queue.size() >= maxObjects) {
            return false;
        }
        
        numEnqueued++;
        sumOfQueueSizesEnqueue += queue.size();
        queue.add(0, obj);
        
        return true;
    }
    
    /**
     * Push an object onto the queue. If the queue is full then the push will
     *  wait for up to "timeout" milliseconds to push the object. At the end of
     *  "timeout" milliseconds, the push will either return false or remove the
     *  oldest item from the queue and insert "obj". This behaviour is contolled
     *  by the constructor parameter "dropOldest".
     *
     * @param obj Object to be pushed onto the queue
     * @param timeout Time in milliseconds to try to insert the item into a full
     *  queue. Per Java standards, a timeout of "0" (zero) will wait indefinitly.
     *  Negative values force no wait period at all.
     *  @return true if the object was intersted into the queue, otherwise false.
     *  @throws InterruptedException    if the operation is interrupted before
     *      the timeout interval is completed.
     */
    public boolean push(Object obj, long timeout) throws InterruptedException {
        return push3(obj, timeout, false);
    }
    
    /**
     *  Push an object back at the head of the queue. If the queue is full then
     *  the push will wait for up to "timeout" milliseconds to push the object.
     *  At the end of "timeout" milliseconds, the push will either return false
     *  or remove the oldest item from the queue and insert "obj". This behaviour
     *  is contolled by the constructor parameter "dropOldest".
     *
     *  <p/>Timeout control is accomplished via synchronization and
     *  {@link Object#wait(long)}. {@link #pushBack(Object,long)} should only
     *  be used in conjunction with {@link #push(Object,long)} and
     *  {@link #pop(long)}
     *
     * @param obj Object to be pushed onto the queue
     * @param timeout Time in milliseconds to try to insert the item into a full
     *  queue. Per Java standards, a timeout of "0" (zero) will wait indefinitly.
     *  Negative values force no wait period at all.
     *  @return <tt>true</tt> if the object was intersted into the queue,
     *  otherwise <tt>false</tt>.
     *  @throws InterruptedException    if the operation is interrupted before
     *  the timeout interval is completed.
     */
    public boolean pushBack(Object obj, long timeout) throws InterruptedException {
        return push3(obj, timeout, true);
    }
    
    private boolean push3(Object obj, long timeout, boolean atHead) throws InterruptedException {
        if (null == obj) {
            throw new AssertionError("obj is null");
        }

        if (null == queue) {
            throw new AssertionError("queue is null");
        }
       
        if (0 == timeout) {
            timeout = Long.MAX_VALUE;
        }
        
        long absoluteTimeOut = TimeUtils.toAbsoluteTimeMillis(timeout);
        
        synchronized (queue) {
            // this is the loop we stay in until there is space in the queue,
            // the queue becomes closed or we get tired of waiting.
            do {
                // This queue is closed. No additional objects allowed.
                if (isClosed()) {
                    queue.notify(); // inform someone who is waiting. we dont have to tell everyone though.
                    return false;
                }
                
                if (queue.size() >= maxObjects) {
                    long waitfor = TimeUtils.toRelativeTimeMillis(absoluteTimeOut);

                    if (waitfor > 0) {
                        queue.wait(waitfor);
                        
                        // something happened, we check again.
                        continue;
                    }
                    
                    // Queue is full but its time to do something.
                    // discard an element or simply return.
                    if (dropOldestObject) {
                        // Issue a warning if we have not done so recently.
                        long now = TimeUtils.timeNow();

                        if ((now > nextDroppedWarn) && Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Queue full, dropped one or more elements. Now dropped " + numDropped + " elements.");
                            nextDroppedWarn = now + DROPPED_OBJECT_WARNING_INTERVAL;
                        }
                        
                        if (atHead) {
                            // we have chosen to drop this element since it is
                            // the oldest. We can safely return true because we
                            // took the right action for this element.
                            
                            numEnqueued++; // one was queued.
                            numDropped++; // one was dropped.
                            // (happens they are the same)

                            queue.notify(); // inform someone who is waiting. we dont have to tell everyone though.
                            return true;
                        } else {
                            // Due to queue resizing, we have have to drop more than
                            // one element
                            while (queue.size() >= maxObjects) {
                                numDropped++;
                                queue.remove(0);
                            }
                        }
                    } else {
                        queue.notify(); // inform someone who is waiting. we dont have to tell everyone though.
                        return false;
                    }
                    
                } else {
                    break;
                }
            } while (!isClosed());
            
            boolean pushed = (atHead ? pushBack(obj) : push(obj));
            
            queue.notify(); // inform someone who is waiting. we dont have to tell everyone though.
            return pushed;
        }
    }
    
    /**
     * Return the next Object from the queue without removing it.
     *
     * @return Object, null if the queue is empty.
     */
    public Object peek() {
        Object result = null;
        
        if (queue.isEmpty()) {
            return null;
        }
        
        result = queue.get(0);
        
        return  result;
    }
    
    /**
     * Remove and return the next Object from the queue.
     *
     * @return Object, null if the queue is empty.
     */
    public Object pop() {
        Object result = null;
        
        if (queue.isEmpty()) {
            return null;
        }
        
        sumOfQueueSizesDequeue += queue.size();
        numDequeued++;
        
        result = queue.remove(0);
        
        return  result;
    }
    
    /**
     * Gets a Object from the queue. If no Object is immediately available,
     * then wait the specified amount of time for an Object to be inserted.
     *
     * @param timeout   Amount of time to wait in milliseconds for an object to
     * be available. Per Java convention, a timeout of zero (0) means wait an
     * infinite amount of time. Negative values mean do not wait at all.
     * @return The next object in the queue.
     * @throws InterruptedException    if the operation is interrupted before
     * the timeout interval is completed.
     */
    public Object pop(long timeout) throws InterruptedException {
        
        if (0 == timeout) {
            timeout = Long.MAX_VALUE;
        }
        
        long absoluteTimeOut = TimeUtils.toAbsoluteTimeMillis(timeout);
        
        Object result = null;
        
        synchronized (queue) {
            do {

                /*
                 *  Because there may be more than one thread waiting on this
                 *  queue, when we are woken up we do not necessarily get the
                 *  next obj in the queue. In this case, rather than terminating
                 *  because we didn't get the obj we resume waiting, but we
                 *  ensure that we never wait longer than the amount of time
                 *  which was originally requested. (if we fail to get the obj
                 *  after being woken its actually a little less than the
                 *  requested time)
                 */
                result = pop();
                
                if (null != result) {
                    break;
                }          // we have an obj
                
                if (isClosed()) { // we didn't get one and its closed so there
                    break;
                }          // is no chance there will ever be one.
                
                long waitfor = TimeUtils.toRelativeTimeMillis(absoluteTimeOut);
                
                if (waitfor <= 0) { // there is no wait time left.
                    break;
                }
                
                queue.wait(waitfor);
            } while (!isClosed());
            
            // wake someone else who might be waiting. This is apparently better
            // than just letting the scheduler notice the synchro is no longer
            // occupied.
            queue.notify();
        }
        
        return result;
    }
    
    /**
     *  Returns an array of objects, possibly empty, from the queue.
     *
     *  @param maxObjs  the maximum number of items to return.
     *  @return an array of objects, possibly empty containing the returned
     *  queue elements.
     */
    public Object[] popMulti(int maxObjs) {
        if (maxObjs <= 0) {
            throw new IllegalArgumentException("maxObjs must be > 0");
        }
        
        maxObjs = Math.min(maxObjs, queue.size());
        Object[] result = new Object[maxObjs];

        for (int eachElement = 0; eachElement < maxObjs; eachElement++) {
            sumOfQueueSizesDequeue += queue.size();
            numDequeued++;
            result[eachElement] = queue.remove(0);
        }
        
        return result;
    }
    
    /**
     *  How many objects will fit in this queue
     *
     *  @return int indicating how many objects will fit in the queue.
     */
    public int getMaxQueueSize() {
        return maxObjects;
    }
    
    /**
     *  Set how many objects this queue may store. Note that if there are more
     *  objects already in the queue than the specified amount then the queue
     *  will retain its current capacity.
     *
     *  @param maxObjs  The number of objects which the queue must be able to
     *  store.
     */
    public void setMaxQueueSize(int maxObjs) {
        maxObjects = maxObjs;
    }
    
    /**
     *  Return the number of elements currently in the queue. This method is
     *  useful for statistical sampling, but should not be used to determine
     *  program logic due to the multi-threaded behaviour of these queues.
     *
     *  <p/>You should use the return values and timeout behaviour of the
     *  {@link #push(Object)} and {@link #pop(long)} methods to regulate how you
     *  use the queue.
     *
     *  @return the number of elements currently in the queue. Be warned that
     *  even two sequential calls to this method may return different answers
     *  due to activity on other threads.
     */
    public int getCurrentInQueue() {
        return queue.size();
    }
    
    /**
     *  Return the total number of objects which have been enqueued on to this
     *  queue during its existance.
     *
     *  @return how many objects have been queued.
     */
    public long getNumEnqueued() {
        return numEnqueued;
    }
    
    /**
     *  Return the average number of elements in the queue at Enqueue time.
     *
     *  @return average number of elements which were in the queue at during all
     *  of the "push" operations which returned a "true" result. Does not
     *  include the item being pushed. If no elements have ever been enqueued
     *  then "NaN" will be returned.
     */
    public double getAvgInQueueAtEnqueue() {
        if (numEnqueued > 0) {
            return (double) sumOfQueueSizesEnqueue / numEnqueued;
        } else {
            return Double.NaN;
        }
    }
    
    /**
     *  Return the total number of objects which have been dequeued from this
     *  queue during its existance.
     *
     *  @return how many objects have been queued.
     */
    public long getNumDequeued() {
        return numDequeued;
    }
    
    /**
     *  Return the average number of elements in the queue at dequeue time.
     *
     *  @return average number of elements which were in the queue at during all
     *  of the "pop" operations which returned a non-null result. Includes the
     * item being "pop"ed in the average. If no elements have ever been dequeued
     *  then "NaN" will be returned.
     */
    public double getAvgInQueueAtDequeue() {
        if (numDequeued > 0) {
            return (double) sumOfQueueSizesDequeue / numDequeued;
        } else {
            return Double.NaN;
        }
    }
    
    /**
     *  Return the total number of objects which have been dropped by this queue
     *  during its existance.
     *
     *  @return how many objects have been dropped.
     */
    public long getNumDropped() {
        return numDropped;
    }
    
    public void interrupt() {
        synchronized (queue) {
            queue.notify();
        }
    }
    
}
