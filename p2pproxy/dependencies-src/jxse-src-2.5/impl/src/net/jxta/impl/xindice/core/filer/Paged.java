/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 1999 The Apache Software Foundation.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xindice" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 1999-2001, The dbXML
 * Group, L.L.C., http://www.dbxmlgroup.com.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 */
package net.jxta.impl.xindice.core.filer;

import net.jxta.impl.xindice.core.DBException;
import net.jxta.impl.xindice.core.FaultCodes;
import net.jxta.impl.xindice.core.data.Key;
import net.jxta.impl.xindice.core.data.Value;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.lang.ref.WeakReference;
import java.util.Collection;
import java.util.EmptyStackException;
import java.util.HashMap;
import java.util.Map;
import java.util.Stack;
import java.util.WeakHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Paged is a paged file implementation that is foundation for both the
 * BTree class and the HashFiler. It provides flexible paged I/O and
 * page caching functionality.
 * <p/>
 * Page has folowing configuration attributes:
 * <ul>
 * <li><strong>pagesize</strong>: Size of the page used by the paged file.
 * Default page size is 4096 bytes. This parameter can be set only
 * before paged file is created. Once it is created, this parameter
 * can not be changed.</li>
 * <li><strong>maxkeysize</strong>: Maximum allowed size of the key.
 * Default maximum key size is 256 bytes.</li>
 * <li><strong>max-descriptors</strong>: Defines maximum amount of
 * simultaneously opened file descriptors this paged file can have.
 * Several descriptors are needed to provide multithreaded access
 * to the underlying file. Too large number will limit amount of
 * collections you can open. Default value is 16
 * (DEFAULT_DESCRIPTORS_MAX).</li>
 * </ul>
 * <p/>
 * <br>FIXME: Currently it seems that maxkeysize is not used anywhere.
 * <br>TODO: Introduce Paged interface, implementations.
 */
public abstract class Paged {

    /**
     * Logger
     */
    private final static Logger LOG = Logger.getLogger(Paged.class.getName());

    /**
     * The maximum number of pages that will be held in the dirty cache.
     * Once number reaches the limit, pages are flushed to disk.
     */
    private static final int MAX_DIRTY_SIZE = 128;

    // The maximum number of open random access files we can have
    private static final int DEFAULT_DESCRIPTORS_MAX = 16;

    /**
     * Unused page status
     */
    protected static final byte UNUSED = 0;

    /**
     * Overflow page status
     */
    protected static final byte OVERFLOW = 126;

    /**
     * Deleted page status
     */
    protected static final byte DELETED = 127;

    /**
     * Page ID of non-existent page
     */
    protected static final int NO_PAGE = -1;

    /**
     * flag whether to sync DB on every write or not.
     */
    protected boolean sync = true;

    // TODO: This is not a cache right now, but a way to assure that only one page instance at most exists in memory at all times.
    /**
     * Cache of recently read pages.
     * <p/>
     * Cache contains weak references to the Page objects, keys are page numbers (Long objects).
     * Access synchronized by this map itself.
     */
    private final Map<Long, WeakReference<Page>> pages = new WeakHashMap<Long, WeakReference<Page>>();

    /**
     * Cache of modified pages waiting to be written out.
     * Access is synchronized by the {@link #dirtyLock}.
     */
    private Map<Long, Page> dirty = new HashMap<Long, Page>();

    /**
     * Lock for synchronizing access to the {@link #dirty} map.
     */
    private final Object dirtyLock = new Object();

    /**
     * Random access file descriptors cache.
     * Access to it and to {@link #descriptorsCount} is synchronized by itself.
     */
    private final Stack<RandomAccessFile> descriptors = new Stack<RandomAccessFile>();

    /**
     * The number of random access file objects that exist, either in the
     * cache {@link #descriptors}, or currently in use.
     */
    private int descriptorsCount;

    /**
     * The maximum number of random access file objects that can be opened
     * by this paged instance.
     */
    private int descriptorsMax;

    /**
     * Whether the file is opened or not.
     */
    private boolean opened;

    /**
     * The underlying file where the Paged object stores its pages.
     */
    private File file;

    /**
     * Header of this Paged
     */
    private final FileHeader fileHeader;

    public Paged() {
        descriptorsMax = DEFAULT_DESCRIPTORS_MAX;
        fileHeader = createFileHeader();
    }

    public Paged(File file) {
        this();
        setFile(file);
    }

    /**
     * setFile sets the file object for this Paged.
     *
     * @param file The File
     */
    protected final void setFile(final File file) {
        this.file = file;
    }

    /**
     * getFile returns the file object for this Paged.
     *
     * @return The File
     */
    protected final File getFile() {
        return file;
    }

    /**
     * Obtain RandomAccessFile ('descriptor') object out of the pool.
     * If no descriptors available, and maximum amount already allocated,
     * the call will block.
     * @return the file
     * @throws java.io.IOException if an io error occurs
     */
    protected final RandomAccessFile getDescriptor() throws IOException {
        synchronized (descriptors) {
            // If there are descriptors in the cache return one.
            if (!descriptors.empty()) {
                return descriptors.pop();
            }
            // Otherwise we need to get one some other way.

            // First try to create a new one if there's room
            if (descriptorsCount < descriptorsMax) {
                descriptorsCount++;
                return new RandomAccessFile(file, "rw");
            }

            // Otherwise we have to wait for one to be released by another thread.
            while (true) {
                try {
                    descriptors.wait();
                    return descriptors.pop();
                } catch (InterruptedException e) {// Ignore, and continue to wait
                } catch (EmptyStackException e) {// Ignore, and continue to wait
                }
            }
        }
    }

    /**
     * Puts a RandomAccessFile ('descriptor') back into the descriptor pool.
     * @param raf the file to add
     */
    protected final void putDescriptor(RandomAccessFile raf) {
        if (raf != null) {
            synchronized (descriptors) {
                descriptors.push(raf);
                descriptors.notify();
            }
        }
    }

    /**
     * Closes a RandomAccessFile ('descriptor') and removes it from the pool.
     * @param raf the file to close
     */
    protected final void closeDescriptor(RandomAccessFile raf) {
        if (raf != null) {
            try {
                raf.close();
            } catch (IOException e) {// Ignore close exception
            }

            // Synchronization is necessary as decrement operation is not atomic
            synchronized (descriptors) {
                descriptorsCount--;
            }
        }
    }

    /**
     * getPage returns the page specified by pageNum.
     *
     * @param pageNum The Page number
     * @return The requested Page
     * @throws IOException if an Exception occurs
     */
    protected final Page getPage(long pageNum) throws IOException {
        final Long lp = pageNum;
        Page page;

        synchronized (this) {
            // Check if it's in the dirty cache
            // No need to synchronize on dirtyLock thanks to atomic assignment
            page = dirty.get(lp);

            // if not check if it's already loaded in the page cache
            if (page == null) {
                WeakReference<Page> ref = pages.get(lp);

                if (ref != null) {
                    page = ref.get();
                }
            }

            // if still not found we need to create it and add it to the page cache.
            if (page == null) {
                page = new Page(lp);
                pages.put(page.pageNum, new WeakReference<Page>(page));
            }
        }

        // Load the page from disk if necessary
        page.read();
        return page;
    }

    /**
     * readValue reads the multi-Paged Value starting at the specified
     * Page.
     *
     * @param page The starting Page
     * @return The Value
     * @throws IOException if an Exception occurs
     */
    protected final Value readValue(Page page) throws IOException {
        final PageHeader sph = page.getPageHeader();
        ByteArrayOutputStream bos = new ByteArrayOutputStream(sph.getRecordLen());

        // Loop until we've read all the pages into memory.
        Page p = page;

        while (true) {
            PageHeader ph = p.getPageHeader();

            // Add the contents of the page onto the stream
            p.streamTo(bos);

            // Continue following the list of pages until we get to the end.
            long nextPage = ph.getNextPage();

            if (nextPage == NO_PAGE) {
                break;
            }
            p = getPage(nextPage);
        }

        // Return a Value with the collected contents of all pages.
        return new Value(bos.toByteArray());
    }

    /**
     * readValue reads the multi-Paged Value starting at the specified
     * page number.
     *
     * @param page The starting page number
     * @return The Value
     * @throws IOException if an Exception occurs
     */
    protected final Value readValue(long page) throws IOException {
        return readValue(getPage(page));
    }

    /**
     * writeValue writes the multi-Paged Value starting at the specified
     * Page.
     *
     * @param page  The starting Page
     * @param value The Value to write
     * @throws IOException if an Exception occurs
     */
    protected final void writeValue(Page page, Value value) throws IOException {
        if (value == null) {
            throw new IOException("Can't write a null value");
        }

        InputStream is = value.getInputStream();

        // Write as much as we can onto the primary page.
        PageHeader hdr = page.getPageHeader();

        hdr.setRecordLen(value.getLength());
        page.streamFrom(is);

        // Write out the rest of the value onto any needed overflow pages
        while (is.available() > 0) {
            Page lpage = page;
            PageHeader lhdr = hdr;

            // Find an overflow page to use
            long np = lhdr.getNextPage();

            if (np != NO_PAGE) {
                // Use an existing page.
                page = getPage(np);
            } else {
                // Create a new overflow page
                page = getFreePage();
                lhdr.setNextPage(page.getPageNum());
            }

            // Mark the page as an overflow page.
            hdr = page.getPageHeader();
            hdr.setStatus(OVERFLOW);

            // Write some more of the value to the overflow page.
            page.streamFrom(is);
            lpage.write();
        }

        // Cleanup any unused overflow pages. i.e. the value is smaller then the
        // last time it was written.
        long np = hdr.getNextPage();

        if (np != NO_PAGE) {
            unlinkPages(np);
        }

        hdr.setNextPage(NO_PAGE);
        page.write();
    }

    /**
     * writeValue writes the multi-Paged Value starting at the specified
     * page number.
     *
     * @param page  The starting page number
     * @param value The Value to write
     * @throws IOException if an Exception occurs
     */
    protected final void writeValue(long page, Value value) throws IOException {
        writeValue(getPage(page), value);
    }

    /**
     * unlinkPages unlinks a set of pages starting at the specified Page.
     *
     * @param page The starting Page to unlink
     * @throws IOException if an Exception occurs
     */
    protected final void unlinkPages(Page page) throws IOException {
        // Handle the page if it's in primary space by setting its status to
        // DELETED and freeing any overflow pages linked to it.
        if (page.pageNum < fileHeader.pageCount) {
            long nextPage = page.header.nextPage;

            page.header.setStatus(DELETED);
            page.header.setNextPage(NO_PAGE);
            page.write();

            // See if there are any chained pages from the page that was just removed
            if (nextPage == NO_PAGE) {
                page = null;
            } else {
                page = getPage(nextPage);
            }
        }

        // Add any overflow pages to the list of free pages.
        if (page != null) {
            // Get the first and last page in the chain.
            long firstPage = page.pageNum;

            while (page.header.nextPage != NO_PAGE) {
                page = getPage(page.header.nextPage);
            }
            long lastPage = page.pageNum;

            // Free the chain
            synchronized (fileHeader) {
                // If there are already some free pages, add the start of the chain
                // to the list of free pages.
                if (fileHeader.lastFreePage != NO_PAGE) {
                    Page p = getPage(fileHeader.lastFreePage);

                    p.header.setNextPage(firstPage);
                    p.write();
                }

                // Otherwise set the chain as the list of free pages.
                if (fileHeader.firstFreePage == NO_PAGE) {
                    fileHeader.setFirstFreePage(firstPage);
                }

                // Add a reference to the end of the chain.
                fileHeader.setLastFreePage(lastPage);
            }
        }
    }

    /**
     * unlinkPages unlinks a set of pages starting at the specified
     * page number.
     *
     * @param pageNum The starting page number to unlink
     * @throws IOException if an Exception occurs
     */
    protected final void unlinkPages(long pageNum) throws IOException {
        unlinkPages(getPage(pageNum));
    }

    /**
     * getFreePage returns the first free Page from secondary storage.
     * If no Pages are available, the file is grown as appropriate.
     *
     * @return The next free Page
     * @throws IOException if an Exception occurs
     */
    protected final Page getFreePage() throws IOException {
        Page p = null;

        // Synchronize read and write to the fileHeader.firstFreePage
        synchronized (fileHeader) {
            if (fileHeader.firstFreePage != NO_PAGE) {
                // Steal a deleted page
                p = getPage(fileHeader.firstFreePage);
                fileHeader.setFirstFreePage(p.getPageHeader().nextPage);
                if (fileHeader.firstFreePage == NO_PAGE) {
                    fileHeader.setLastFreePage(NO_PAGE);
                }
            }
        }

        if (p == null) {
            // No deleted pages, grow the file
            p = getPage(fileHeader.incTotalCount());
        }

        // Initialize The Page Header (Cleanly)
        p.header.setNextPage(NO_PAGE);
        p.header.setStatus(UNUSED);
        return p;
    }

    /**
     * @throws DBException COL_COLLECTION_CLOSED if paged file is closed
     */
    protected final void checkOpened() throws DBException {
        if (!opened) {
            throw new FilerException(FaultCodes.COL_COLLECTION_CLOSED, "Filer is closed");
        }
    }

    /**
     * getFileHeader returns the FileHeader
     *
     * @return The FileHeader
     */
    public FileHeader getFileHeader() {
        return fileHeader;
    }

    public boolean exists() {
        return file.exists();
    }

    public boolean create() throws DBException {
        try {
            createFile();
            fileHeader.write();
            flush();
            return true;
        } catch (Exception e) {
            throw new FilerException(FaultCodes.GEN_CRITICAL_ERROR, "Error creating " + file.getName(), e);
        }
    }

    private void createFile() throws IOException {
        RandomAccessFile raf = null;

        try {
            raf = getDescriptor();
            long o = fileHeader.headerSize + (fileHeader.pageCount + 1) * fileHeader.pageSize - 1;

            raf.seek(o);
            raf.write(0);
        } finally {
            putDescriptor(raf);
        }
    }

    public boolean open() throws DBException {
        RandomAccessFile raf = null;

        try {
            if (exists()) {
                raf = getDescriptor();
                fileHeader.read();
                opened = true;
            } else {
                opened = false;
            }
            return opened;
        } catch (Exception e) {
            throw new FilerException(FaultCodes.GEN_CRITICAL_ERROR, "Error opening " + file.getName(), e);
        } finally {
            putDescriptor(raf);
        }
    }

    public synchronized boolean close() throws DBException {
        if (isOpened()) {
            try {
                // First of all, mark as closed to prevent operations
                opened = false;
                flush();

                synchronized (descriptors) {
                    final int total = descriptorsCount;

                    // Close descriptors in cache
                    while (!descriptors.empty()) {
                        closeDescriptor(descriptors.pop());
                    }
                    // Attempt to close descriptors in use. Max wait time = 0.5s * MAX_DESCRIPTORS
                    int n = descriptorsCount;

                    while (descriptorsCount > 0 && n > 0) {
                        try {
                            descriptors.wait(500);
                        } catch (InterruptedException woken) {
                            Thread.interrupted();
                        }

                        if (descriptors.isEmpty()) {
                            n--;
                        } else {
                            closeDescriptor(descriptors.pop());
                        }
                    }
                    if (descriptorsCount > 0) {
                        LOG.fine(descriptorsCount + " out of " + total + " files were not closed during close.");
                    }
                }
            } catch (Exception e) {
                // Failed to close, leave open
                opened = true;
                throw new FilerException(FaultCodes.GEN_CRITICAL_ERROR, "Error closing " + file.getName(), e);
            }
        }
        return true;
    }

    public boolean isOpened() {
        return opened;
    }

    public boolean drop() throws DBException {
        try {
            close();
            return !exists() || getFile().delete();
        } catch (Exception e) {
            throw new FilerException(FaultCodes.COL_CANNOT_DROP, "Can't drop " + file.getName(), e);
        }
    }

    void addDirty(Page page) throws IOException {
        synchronized (dirtyLock) {
            dirty.put(page.pageNum, page);
            if (dirty.size() > MAX_DIRTY_SIZE) {
                try {
                    // Too many dirty pages... flush them
                    flush();
                } catch (Exception e) {
                    throw new IOException(e.getMessage());
                }
            }
        }
    }

    public void flush() throws DBException {
        // This method is not synchronized

        // Error flag/counter
        int error = 0;

        // Obtain collection of dirty pages
        Collection<Page> pages;

        synchronized (dirtyLock) {
            pages = dirty.values();
            dirty = new HashMap<Long, Page>();
        }

        // Flush dirty pages
        for (Object page : pages) {
            Page p = (Page) page;

            try {
                p.flush();
            } catch (Exception e) {
                LOG.log(Level.WARNING, "Exception while flushing page", e);
                error++;
            }
        }

        // Flush header
        if (fileHeader.dirty) {
            try {
                fileHeader.write();
            } catch (Exception e) {
                LOG.log(Level.WARNING, "Exception while flushing file header", e);
                error++;
            }
        }

        if (error != 0) {
            throw new FilerException(FaultCodes.GEN_CRITICAL_ERROR, "Error performing flush! Failed to flush " + error + " pages!");
        }
    }

    /**
     * createFileHeader must be implemented by a Paged implementation
     * in order to create an appropriate subclass instance of a FileHeader.
     *
     * @return a new FileHeader
     */
    public abstract FileHeader createFileHeader();

    /**
     * createFileHeader must be implemented by a Paged implementation
     * in order to create an appropriate subclass instance of a FileHeader.
     *
     * @param read If true, reads the FileHeader from disk
     * @return a new FileHeader
     * @throws IOException if an exception occurs
     */
    public abstract FileHeader createFileHeader(boolean read) throws IOException;

    /**
     * createFileHeader must be implemented by a Paged implementation
     * in order to create an appropriate subclass instance of a FileHeader.
     *
     * @param pageCount The number of pages to allocate for primary storage
     * @return a new FileHeader
     */
    public abstract FileHeader createFileHeader(long pageCount);

    /**
     * createFileHeader must be implemented by a Paged implementation
     * in order to create an appropriate subclass instance of a FileHeader.
     *
     * @param pageCount The number of pages to allocate for primary storage
     * @param pageSize  The size of a Page (should be a multiple of a FS block)
     * @return a new FileHeader
     */
    public abstract FileHeader createFileHeader(long pageCount, int pageSize);

    /**
     * createPageHeader must be implemented by a Paged implementation
     * in order to create an appropriate subclass instance of a PageHeader.
     *
     * @return a new PageHeader
     */
    public abstract PageHeader createPageHeader();

    // These are a bunch of utility methods for subclasses

    public static Value[] insertArrayValue(Value[] vals, Value val, int idx) {
        Value[] newVals = new Value[vals.length + 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        newVals[idx] = val;
        if (idx < vals.length) {
            System.arraycopy(vals, idx, newVals, idx + 1, vals.length - idx);
        }
        return newVals;
    }

    public static Value[] deleteArrayValue(Value[] vals, int idx) {
        Value[] newVals = new Value[vals.length - 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        if (idx < newVals.length) {
            System.arraycopy(vals, idx + 1, newVals, idx, newVals.length - idx);
        }
        return newVals;
    }

    public static long[] insertArrayLong(long[] vals, long val, int idx) {
        long[] newVals = new long[vals.length + 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        newVals[idx] = val;
        if (idx < vals.length) {
            System.arraycopy(vals, idx, newVals, idx + 1, vals.length - idx);
        }
        return newVals;
    }

    public static long[] deleteArrayLong(long[] vals, int idx) {
        long[] newVals = new long[vals.length - 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        if (idx < newVals.length) {
            System.arraycopy(vals, idx + 1, newVals, idx, newVals.length - idx);
        }
        return newVals;
    }

    public static int[] insertArrayInt(int[] vals, int val, int idx) {
        int[] newVals = new int[vals.length + 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        newVals[idx] = val;
        if (idx < vals.length) {
            System.arraycopy(vals, idx, newVals, idx + 1, vals.length - idx);
        }
        return newVals;
    }

    public static int[] deleteArrayInt(int[] vals, int idx) {
        int[] newVals = new int[vals.length - 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        if (idx < newVals.length) {
            System.arraycopy(vals, idx + 1, newVals, idx, newVals.length - idx);
        }
        return newVals;
    }

    public static short[] insertArrayShort(short[] vals, short val, int idx) {
        short[] newVals = new short[vals.length + 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        newVals[idx] = val;
        if (idx < vals.length) {
            System.arraycopy(vals, idx, newVals, idx + 1, vals.length - idx);
        }

        return newVals;
    }

    public static short[] deleteArrayShort(short[] vals, int idx) {
        short[] newVals = new short[vals.length - 1];

        if (idx > 0) {
            System.arraycopy(vals, 0, newVals, 0, idx);
        }
        if (idx < newVals.length) {
            System.arraycopy(vals, idx + 1, newVals, idx, newVals.length - idx);
        }

        return newVals;
    }

    /**
     * Paged file's header
     */
    public abstract class FileHeader {
        private boolean dirty = false;
        private int workSize;

        private short headerSize;
        private int pageSize;
        private long pageCount;
        private long totalCount;
        private long firstFreePage = -1;
        private long lastFreePage = -1;
        private byte pageHeaderSize = 64;
        private short maxKeySize = 256;
        private long recordCount;

        public FileHeader() {
            this(1024);
        }

        public FileHeader(long pageCount) {
            this(pageCount, 4096);
        }

        public FileHeader(long pageCount, int pageSize) {
            this.pageSize = pageSize;
            this.pageCount = pageCount;
            totalCount = pageCount;
            headerSize = (short) 4096;
            calculateWorkSize();
        }

        public FileHeader(boolean read) throws IOException {
            if (read) {
                read();
            }
        }

        public synchronized final void read() throws IOException {
            RandomAccessFile raf = null;

            try {
                raf = getDescriptor();
                raf.seek(0);
                read(raf);
                calculateWorkSize();
            } finally {
                putDescriptor(raf);
            }
        }

        public synchronized void read(RandomAccessFile raf) throws IOException {
            headerSize = raf.readShort();
            pageSize = raf.readInt();
            pageCount = raf.readLong();
            totalCount = raf.readLong();
            firstFreePage = raf.readLong();
            lastFreePage = raf.readLong();
            pageHeaderSize = raf.readByte();
            maxKeySize = raf.readShort();
            recordCount = raf.readLong();
        }

        public synchronized final void write() throws IOException {
            if (!dirty) {
                return;
            }

            RandomAccessFile raf = null;

            try {
                raf = getDescriptor();
                raf.seek(0);
                write(raf);
                dirty = false;
            } finally {
                putDescriptor(raf);
            }
        }

        public synchronized void write(RandomAccessFile raf) throws IOException {
            raf.writeShort(headerSize);
            raf.writeInt(pageSize);
            raf.writeLong(pageCount);
            raf.writeLong(totalCount);
            raf.writeLong(firstFreePage);
            raf.writeLong(lastFreePage);
            raf.writeByte(pageHeaderSize);
            raf.writeShort(maxKeySize);
            raf.writeLong(recordCount);
        }

        public synchronized final void setDirty() {
            dirty = true;
        }

        public synchronized final boolean isDirty() {
            return dirty;
        }

        /**
         * The size of the FileHeader. Usually 1 OS Page.
         * This method should be called only while initializing Paged, not during normal processing.
         * @param headerSize the new header size
         */
        public synchronized final void setHeaderSize(short headerSize) {
            this.headerSize = headerSize;
            dirty = true;
        }

        /**
         * The size of the FileHeader.  Usually 1 OS Page
         * @return the header size
         */
        public synchronized final short getHeaderSize() {
            return headerSize;
        }

        /**
         * The size of a page. Usually a multiple of a FS block.
         * This method should be called only while initializing Paged, not during normal processing.
         * @param pageSize the new page size
         */
        public synchronized final void setPageSize(int pageSize) {
            this.pageSize = pageSize;
            calculateWorkSize();
            dirty = true;
        }

        /**
         * The size of a page.  Usually a multiple of a FS block
         * @return the page size
         */
        public synchronized final int getPageSize() {
            return pageSize;
        }

        /**
         * The number of pages in primary storage.
         * This method should be called only while initializing Paged, not during normal processing.
         * @param pageCount the new page count
         */
        public synchronized final void setPageCount(long pageCount) {
            this.pageCount = pageCount;
            dirty = true;
        }

        /**
         * The number of pages in primary storage
         * @return the page count
         */
        public synchronized final long getPageCount() {
            return pageCount;
        }

        /**
         * The number of total pages in the file.
         * This method should be called only while initializing Paged, not during normal processing.
         * @param totalCount the new total count
         */
        public synchronized final void setTotalCount(long totalCount) {
            this.totalCount = totalCount;
            dirty = true;
        }

        public synchronized final long incTotalCount() {
            dirty = true;
            return this.totalCount++;
        }

        /**
         * The number of total pages in the file
         * @return the total count
         */
        public synchronized final long getTotalCount() {
            return totalCount;
        }

        /**
         * The first free page in unused secondary space
         * @param firstFreePage the new first free page
         */
        public synchronized final void setFirstFreePage(long firstFreePage) {
            this.firstFreePage = firstFreePage;
            dirty = true;
        }

        /**
         * The first free page in unused secondary space
         * @return the first free page
         */
        public synchronized final long getFirstFreePage() {
            return firstFreePage;
        }

        /**
         * The last free page in unused secondary space
         * @param lastFreePage sets the last free page
         */
        public synchronized final void setLastFreePage(long lastFreePage) {
            this.lastFreePage = lastFreePage;
            dirty = true;
        }

        /**
         * The last free page in unused secondary space
         * @return the last free page
         */
        public synchronized final long getLastFreePage() {
            return lastFreePage;
        }

        /**
         * Set the size of a page header.
         * <p/>
         * Normally, 64 is sufficient.
         * @param pageHeaderSize the new page header size
         */
        public synchronized final void setPageHeaderSize(byte pageHeaderSize) {
            this.pageHeaderSize = pageHeaderSize;
            calculateWorkSize();
            dirty = true;
        }

        /**
         * Get the size of a page header.
         * <p/>
         * Normally, 64 is sufficient
         * @return the page header size
         */
        public synchronized final byte getPageHeaderSize() {
            return pageHeaderSize;
        }

        /**
         * Set the maximum number of bytes a key can be.
         * <p/>
         * Normally, 256 is good
         * @param maxKeySize the new max key size
         */
        public synchronized final void setMaxKeySize(short maxKeySize) {
            this.maxKeySize = maxKeySize;
            dirty = true;
        }

        /**
         * Get the maximum number of bytes.
         * <p/>
         * Normally, 256 is good.
         * @return max key size
         */
        public synchronized final short getMaxKeySize() {
            return maxKeySize;
        }

        /**
         * Increment the number of records being managed by the file
         */
        public synchronized final void incRecordCount() {
            recordCount++;
            dirty = true;
        }

        /**
         * Decrement the number of records being managed by the file
         */
        public synchronized final void decRecordCount() {
            recordCount--;
            dirty = true;
        }

        /**
         * The number of records being managed by the file (not pages)
         * @return the record count
         */
        public synchronized final long getRecordCount() {
            return recordCount;
        }

        private synchronized void calculateWorkSize() {
            workSize = pageSize - pageHeaderSize;
        }

        public synchronized final int getWorkSize() {
            return workSize;
        }
    }


    /**
     * PageHeader
     */

    public abstract class PageHeader implements Streamable {
        private boolean dirty = false;
        private byte status = UNUSED;
        private int keyLen = 0;
        private int keyHash = 0;
        private int dataLen = 0;
        private int recordLen = 0;
        private long nextPage = -1;

        public PageHeader() {}

        public PageHeader(DataInputStream dis) throws IOException {
            read(dis);
        }

        public synchronized void read(DataInputStream dis) throws IOException {
            status = dis.readByte();
            dirty = false;
            if (status == UNUSED) {
                return;
            }

            keyLen = dis.readInt();
            if (keyLen < 0) {
                // hack for win98/ME - see issue 564
                keyLen = 0;
            }
            keyHash = dis.readInt();
            dataLen = dis.readInt();
            recordLen = dis.readInt();
            nextPage = dis.readLong();
        }

        public synchronized void write(DataOutputStream dos) throws IOException {
            dirty = false;
            dos.writeByte(status);
            dos.writeInt(keyLen);
            dos.writeInt(keyHash);
            dos.writeInt(dataLen);
            dos.writeInt(recordLen);
            dos.writeLong(nextPage);
        }

        public synchronized final boolean isDirty() {
            return dirty;
        }

        public synchronized final void setDirty() {
            dirty = true;
        }

        /**
         * The status of this page (UNUSED, RECORD, DELETED, etc...)
         * @param status the new status
         */
        public synchronized final void setStatus(byte status) {
            this.status = status;
            dirty = true;
        }

        /**
         * The status of this page (UNUSED, RECORD, DELETED, etc...)
         * @return the status
         */
        public synchronized final byte getStatus() {
            return status;
        }

        public synchronized final void setKey(Key key) {
            // setKey WIPES OUT the Page data
            setRecordLen(0);
            dataLen = 0;
            keyHash = key.getHash();
            keyLen = key.getLength();
            dirty = true;
        }

        /**
         * The length of the Key
         * @param keyLen the new key length
         */
        public synchronized final void setKeyLen(int keyLen) {
            this.keyLen = keyLen;
            dirty = true;
        }

        /**
         * The length of the Key
         * @return the key length
         */
        public synchronized final int getKeyLen() {
            return keyLen;
        }

        /**
         * The hashed value of the Key for quick comparisons
         * @param keyHash sets the key hash
         */
        public synchronized final void setKeyHash(int keyHash) {
            this.keyHash = keyHash;
            dirty = true;
        }

        /**
         * The hashed value of the Key for quick comparisons
         * @return the key hash
         */
        public synchronized final int getKeyHash() {
            return keyHash;
        }

        /**
         * The length of the Data
         * @param dataLen sets the data length
         */
        public synchronized final void setDataLen(int dataLen) {
            this.dataLen = dataLen;
            dirty = true;
        }

        /**
         * The length of the Data
         * @return the data length
         */
        public synchronized final int getDataLen() {
            return dataLen;
        }

        /**
         * The length of the Record's value
         * @param recordLen sets the record length
         */
        public synchronized void setRecordLen(int recordLen) {
            this.recordLen = recordLen;
            dirty = true;
        }

        /**
         * The length of the Record's value
         * @return record length
         */
        public synchronized final int getRecordLen() {
            return recordLen;
        }

        /**
         * The next page for this Record (if overflowed)
         * @param nextPage next page
         */
        public synchronized final void setNextPage(long nextPage) {
            this.nextPage = nextPage;
            dirty = true;
        }

        /**
         * The next page for this Record (if overflowed)
         * @return next page
         */
        public synchronized final long getNextPage() {
            return nextPage;
        }
    }


    /**
     * Paged file's page
     */
    public final class Page implements Comparable<Page> {

        /**
         * This page number
         */
        private final Long pageNum;

        /**
         * The Header for this Page
         */
        private final PageHeader header;

        /**
         * The offset into the file that this page starts
         */
        private final long offset;

        /**
         * The data for this page. Null if page is not loaded.
         */
        private byte[] data;

        /**
         * The position (relative) of the Key in the data array
         */
        private int keyPos;

        /**
         * The position (relative) of the Data in the data array
         */
        private int dataPos;

        public Page(Long pageNum) {
            this.header = createPageHeader();
            this.pageNum = pageNum;
            this.offset = fileHeader.headerSize + (pageNum * fileHeader.pageSize);
        }

        /**
         * Reads a page into the memory, once. Subsequent calls are ignored.
         * @throws java.io.IOException if an io error occurs
         */
        public synchronized void read() throws IOException {
            if (data == null) {
                RandomAccessFile raf = null;

                try {
                    byte[] data = new byte[fileHeader.pageSize];

                    raf = getDescriptor();
                    raf.seek(this.offset);
                    raf.read(data);

                    // Read in the header
                    ByteArrayInputStream bis = new ByteArrayInputStream(data);

                    this.header.read(new DataInputStream(bis));

                    this.keyPos = fileHeader.pageHeaderSize;
                    this.dataPos = this.keyPos + this.header.keyLen;

                    // Successfully read all the data
                    this.data = data;
                } finally {
                    putDescriptor(raf);
                }
            }
        }

        /**
         * Writes out the header into the this.data, and adds a page to the set of
         * dirty pages.
         * @throws java.io.IOException if an io error occurs
         */
        public void write() throws IOException {
            // Write out the header into the this.data
            synchronized (this) {
                ByteArrayOutputStream bos = new ByteArrayOutputStream(fileHeader.getPageHeaderSize());

                header.write(new DataOutputStream(bos));
                byte[] b = bos.toByteArray();

                System.arraycopy(b, 0, data, 0, b.length);
            }

            // Add to the list of dirty pages
            Paged.this.addDirty(this);
        }

        /**
         * Flushes content of the dirty page into the file
         * @throws java.io.IOException if an io error occurs
         */
        public synchronized void flush() throws IOException {
            RandomAccessFile raf = null;

            try {
                raf = getDescriptor();
                if (this.offset >= raf.length()) {
                    // Grow the file
                    long o = (fileHeader.headerSize + ((fileHeader.totalCount * 3) / 2) * fileHeader.pageSize)
                            + (fileHeader.pageSize - 1);

                    raf.seek(o);
                    raf.writeByte(0);
                }
                raf.seek(this.offset);
                raf.write(this.data);
                if (sync) {
                    raf.getFD().sync();
                }
            } finally {
                putDescriptor(raf);
            }
        }

        // No synchronization - pageNum is final
        public Long getPageNum() {
            return this.pageNum;
        }

        // No synchronization - header is final
        public PageHeader getPageHeader() {
            return this.header;
        }

        public synchronized void setKey(Key key) {
            header.setKey(key);
            // Insert the key into the data array.
            key.copyTo(this.data, this.keyPos);

            // Set the start of data to skip over the key.
            this.dataPos = this.keyPos + header.keyLen;
        }

        public synchronized Key getKey() {
            if (header.keyLen > 0) {
                return new Key(this.data, this.keyPos, header.keyLen);
            } else {
                return null;
            }
        }

        public synchronized void streamTo(OutputStream os) throws IOException {
            if (header.dataLen > 0) {
                os.write(this.data, this.dataPos, header.dataLen);
            }
        }

        public synchronized void streamFrom(InputStream is) throws IOException {
            int avail = is.available();

            header.dataLen = fileHeader.workSize - header.keyLen;
            if (avail < header.dataLen) {
                header.dataLen = avail;
            }
            if (header.dataLen > 0) {
                is.read(this.data, this.keyPos + header.keyLen, header.dataLen);
            }
        }

        // No synchronization: pageNum is final.
        public int compareTo(Page o) {
            return (int) (this.pageNum - o.pageNum);
        }
    }
}

