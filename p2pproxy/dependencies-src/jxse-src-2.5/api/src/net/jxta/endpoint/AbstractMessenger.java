/*
 * Copyright (c) 2004-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.endpoint;


import net.jxta.util.AbstractSimpleSelectable;
import net.jxta.util.SimpleSelectable;

import java.io.IOException;
import java.io.InterruptedIOException;


/**
 * An AbstractMessenger is used to implement messengers (for example, by transport modules).
 * It supplies the convenience, bw compatible, obvious, or otherwise rarely changed methods.
 * Many method cannot be overloaded in order to ensure standard behaviour.
 * The rest is left to implementations.
 *
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.endpoint.EndpointAddress
 * @see net.jxta.endpoint.Message
 */
public abstract class AbstractMessenger extends AbstractSimpleSelectable implements Messenger {

    /**
     * The default Maximum Transmission Unit.
     */
    protected static final long DEFAULT_MTU = Long.parseLong(System.getProperty("net.jxta.MTU", "65536"));

    /**
     * The destination address of messages sent on this messenger.
     */
    protected final EndpointAddress dstAddress;

    /**
     * The stateLock that we share with the implementation.
     * This permits to implement waitState in a totally generic way: waitState depends only on the lock
     * (provided at construction), and on getState(), supplied by the implementation.
     */
    private Object stateLock;

    /**
     * Create a new abstract messenger.
     * <p/>
     * Warning: This class needs to know the object on which to waitState must synchronize. It is generally impossible
     * to pass it at construction because it is not yet constructed. Instead implementations MUST call {@link #setStateLock}
     * from their constructor.
     *
     * @param dest who messages should be addressed to
     */
    public AbstractMessenger(EndpointAddress dest) {
        dstAddress = dest;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * A simple implementation for debugging. Do not depend upon the format.
     */
    @Override
    public String toString() {
        return super.toString() + " {" + dstAddress + "}";
    }

    /**
     * Specifies the object on which waitState must synchronize.
     *
     * @param stateLock The object on which waitState must synchronize. This has to be the object that gets notified when the
     *                  implementation changes its state. Changing state is defined as "any operation that causes the result of the
     *                  <code>getState</code> method to change". Implementations that use the MessengerState state machine should typically use the
     *                  MessengerState object as their state lock, but it is not assumed.
     */
    protected void setStateLock(Object stateLock) {
        this.stateLock = stateLock;
    }

    /*
     * Messenger methods implementations.
     */

    /**
     * {@inheritDoc}
     * <p/>
     * This is here for backward compatibility reasons.  The notion of long term unemployment still exists, but is no-longer part
     * of the API.  Self closing for unemployment is now a built-in feature of messengers.
     */
    @Deprecated
    public final boolean isIdle() {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    public final boolean isSynchronous() {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    public final EndpointAddress getDestinationAddress() {
        return dstAddress;
    }

    /**
     * {@inheritDoc}
     */
    @Deprecated
    public final EndpointAddress getDestinationAddressObject() {
        return dstAddress;
    }

    /**
     * {@inheritDoc}
     * <p/>It is not always enforced. At least this much can always be sent. 
     */
    public long getMTU() {
        return DEFAULT_MTU;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This is a minimal implementation. It may not detect closure
     * initiated by the other side unless the messenger was actually used
     * since. A more accurate (but not mandatory implementation) would
     * actually go and check the underlying connection, if relevant...unless
     * breakage initiated by the other side is actually reported asynchronously
     * when it happens. Breakage detection from the other side need not
     * be reported atomically with its occurrence. This not very important
     * since we canonicalize transport messengers and so do not need to
     * aggressively collect closed ones. When not used, messengers die by themselves.
     */
    public boolean isClosed() {
        return (getState() & USABLE) == 0;
    }

    /**
     * {@inheritDoc}
     */
    public final void flush() throws IOException {
        int currentState = 0;

        try {
            currentState = waitState(IDLE, 0);
        } catch (InterruptedException ie) {
            InterruptedIOException iio = new InterruptedIOException("flush() interrupted");

            iio.initCause(ie);
            throw iio;
        }
        
        if ((currentState & (CLOSED | USABLE)) != 0) {
            return;
        }
        
        throw new IOException("Messenger was unexpectedly closed.");
    }

    /**
     * {@inheritDoc}
     */
    public final boolean sendMessage(Message msg) throws IOException {
        return sendMessage(msg, null, null);
    }

    /**
     * {@inheritDoc}
     *
     */
    public void sendMessage(Message msg, String service, String serviceParam, OutgoingMessageEventListener listener) {
        throw new UnsupportedOperationException("This legacy method is not supported by this messenger.");
    }

    /**
     * {@inheritDoc}
     */
    public final boolean sendMessage(Message msg, String rService, String rServiceParam) throws IOException {

        // We have to retrieve the failure from the message and throw it if its an IOException, this is what the API
        // says that this method does.

        if (sendMessageN(msg, rService, rServiceParam)) {
            return true;
        }

        Object failed = msg.getMessageProperty(Messenger.class);

        if ((failed == null) || !(failed instanceof OutgoingMessageEvent)) {
            // huh ?
            return false;
        }

        Throwable t = ((OutgoingMessageEvent) failed).getFailure();

        if (t == null) {
            // Must be saturation, then. (No throw for that).
            return false;
        }

        // Now see how we can manage to throw it.
        if (t instanceof IOException) {
            throw (IOException) t;
        } else if (t instanceof RuntimeException) {
            throw (RuntimeException) t;
        } else if (t instanceof Error) {
            throw (Error) t;
        }

        IOException failure = new IOException("Failure sending message");

        failure.initCause(t);

        throw failure;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This method synchronizes on the lock object supplied at construction.
     */
    public final int waitState(int wantedStates, long timeout) throws InterruptedException {
        synchronized (stateLock) {
            if (timeout == 0) {
                while ((wantedStates & getState()) == 0) {
                    stateLock.wait();
                }
                return getState();
            }

            if (timeout < 0) {
                stateLock.wait(timeout); // let it throw the appropriate error.
            }

            long start = System.currentTimeMillis();
            long end = start + timeout;

            if (end < start) {
                end = Long.MAX_VALUE;
            }
            long left = end - start;

            while ((left > 0) && (wantedStates & getState()) == 0) {

                stateLock.wait(left);

                left = end - System.currentTimeMillis();
            }
            
            return getState();
        }
    }

    /*
     * SimpleSelectable implementation.
     */

    /**
     * Implements a default for all AbstractMessengers: mirror the event to our selectors. This is what is needed by all the
     * known AbstractMessengers that register themselves somewhere. (That is ChannelMessengers).
     * FIXME - jice@jxta.org 20040413: Not sure that this is the best default.
     *
     * @param changedObject Ignored.
     */
    public void itemChanged(SimpleSelectable changedObject) {
        notifyChange();
    }
}
