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

package net.jxta.util;


import java.io.*;
import java.util.Collection;


/**
 * @deprecated This class is no longer used by the JXTA core and should not
 *             have been part of the public JXTA API. If you use it, copy it to your own
 *             source base. It will be deleted after June 2007 release.
 */
@Deprecated
public class WatchedInputStream extends FilterInputStream
        implements WatchedStream {

    static final int DEFAULT_CHUNK_SIZE = 4096;
    Collection watchList = null;
    volatile boolean stalled = false;
    volatile boolean idle = true;
    boolean closed = false;
    final int chunkSize;
    InputStream in = null;

    public WatchedInputStream(InputStream in, int chunkSize) {
        super(in);
        this.in = in;
        this.chunkSize = chunkSize;
    }

    public WatchedInputStream(InputStream in) {
        this(in, DEFAULT_CHUNK_SIZE);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>Debugging toString.
     */
    @Override
    public String toString() {
        if (null == in) {
            return "closed/" + super.toString();
        } else {
            if (in instanceof ByteArrayInputStream) {
                // ByteArrayInputStream.toString() prints the entire stream! 
                return in.getClass().getName() + "@" + System.identityHashCode(in) + "/" + super.toString();
            } else {
                return in.toString() + "/" + super.toString();
            }
        }
    }

    /**
     * Sets the watcher list onto which this stream must register
     * when it is not idle (so that it can be watched).
     * This implementation may or may not remain registered while idle.
     * This may affect performance but not functionality.
     * It is assumed that that list is monitored by a watcher task that
     * invokes the watch method as often as needed to monitor progress
     * to its satisfaction.
     *
     * @param watchList The watchList to register with. Must be a
     *                  Synchronized Collection.
     */
    public synchronized void setWatchList(Collection watchList) {
        if (this.watchList != null) {
            this.watchList.remove(this);
        }
        this.watchList = watchList;
        watchList.add(this);
    }

    // This routine may be invoked as often as progress needs to be asserted.
    // After at most two watch cycles stalling is detected.
    public void watch() {

        if (idle) {
            return;
        }

        if (!stalled) {
            stalled = true; // challenge a write method to clear that flag.
            return;
        }

        // It's stalled. The last time around it was not idle, so we set the
        // stalled flag. Now, it is still not idle and the flag is still there.
        // break the stream.
        try {
            close();
        } catch (IOException ioe) {
            ;
        }
    }

    @Override
    public void close() throws IOException {
        idle = true;
        synchronized (this) {
            if (watchList != null) {
                watchList.remove(this);
                watchList = null;
            }

            // Avoid calling close redundantly; some OSes seem to have
            // deadlock capabilities when doing that.

            if (closed) {
                return;
            }
            closed = true;
        }
        super.close();
    }

    @Override
    public int read() throws IOException {
        stalled = false;
        idle = false;
        try {
            return in.read();
        } finally {
            idle = true;
        }
    }

    // We overload it; not trusting that the base classes method does
    // call read(byte[], int, int) in the future.
    @Override
    public int read(byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {

        // Apply the standard checks here; we will call in.read repeatedly
        // which means that errors could be discovered too late per read's
        // contract.

        if ((off < 0) || (len < 0) || ((off + len) > b.length) || ((off + len) < 0)) {

            throw new IndexOutOfBoundsException();

        }

        stalled = false;
        idle = false;
        int left = len;

        try {
            int i;

            while (left > chunkSize) {
                i = in.read(b, off, chunkSize);
                stalled = false;
                if (i <= 0) {
                    if (left == len) {
                        return i;
                    }
                    return len - left;
                }
                off += i;
                left -= i;

                // Must check available only after firt read.
                // first read must wait for at least one byte.

                if (in.available() == 0) {
                    return len - left;
                }
            }

            // Read the left over now. If this is not the first read,
            // available has been checked in the loop above.
            i = in.read(b, off, left);
            if (i <= 0) {
                if (left == len) {
                    return i;
                }
                return len - left;
            }

            return len - left + i;

        } finally {
            idle = true;
        }
    }
}
