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


import java.io.ByteArrayOutputStream;
import java.io.FilterOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Collection;


/**
 * @deprecated This class is no longer used by the JXTA core and should not
 *             have been part of the public JXTA API. If you use it, copy it to your own
 *             source base. It will be deleted after June 2007 release.
 */
@Deprecated
public class WatchedOutputStream extends FilterOutputStream
        implements WatchedStream {

    static final int DEFAULT_CHUNK_SIZE = 4096;
    transient Collection watchList = null;
    transient volatile boolean stalled = false;
    transient volatile boolean idle = true;
    transient boolean closed = false;
    transient final int chunkSize;
    transient OutputStream out = null;

    public WatchedOutputStream(OutputStream out, int chunkSize) {
        super(out);
        this.out = out;
        this.chunkSize = chunkSize;
    }

    public WatchedOutputStream(OutputStream out) {
        this(out, DEFAULT_CHUNK_SIZE);
    }

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>Debugging toString.
     */
    @Override
    public String toString() {

        if (null == out) {
            return "closed/" + super.toString();
        } else {
            if (out instanceof ByteArrayOutputStream) {
                // ByteArrayInputStream.toString() prints the entire stream!
                return out.getClass().getName() + "@" + System.identityHashCode(out) + "/" + super.toString();
            } else {
                return out.toString() + "/" + super.toString();
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
    public void flush() throws IOException {
        stalled = false;
        idle = false;
        try {
            out.flush();
        } finally {
            idle = true;
        }
    }

    @Override
    public void write(int b) throws IOException {
        stalled = false;
        idle = false;
        try {
            out.write(b);
        } finally {
            idle = true;
        }
    }

    // We overload it; not trusting that the base classe's method does
    // call write(byte[], int, int) in the future.
    @Override
    public void write(byte[] b) throws IOException {
        write(b, 0, b.length);
    }

    // We overload it; not wanting the base classe's method to
    // call write(int).
    @Override
    public void write(byte[] b, int off, int len) throws IOException {

        // Do the standard checks here. Since we will be calling write
        // repeatedly, errors could be discovered too late per write's
        // contract.

        if ((off < 0) || (len < 0) || ((off + len) > b.length) || ((off + len) < 0)) {

            throw new IndexOutOfBoundsException();

        } else if (len == 0) {
            return;
        }

        stalled = false;
        idle = false;
        try {
            while (len > chunkSize) {
                out.write(b, off, chunkSize);
                stalled = false;
                off += chunkSize;
                len -= chunkSize;
            }
            if (len > 0) {
                out.write(b, off, len);
            }
        } finally {
            idle = true;
        }
    }
}
