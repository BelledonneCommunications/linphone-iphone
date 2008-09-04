package net.jxta.impl.xindice.core.filer;


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
import net.jxta.impl.xindice.core.DBException;
import net.jxta.impl.xindice.core.FaultCodes;
import net.jxta.impl.xindice.core.data.Key;
import net.jxta.impl.xindice.core.data.Record;
import net.jxta.impl.xindice.core.data.RecordSet;
import net.jxta.impl.xindice.core.data.Value;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

/**
 * BTreeFiler is a Filer implementation based on the BTree class.
 */
public final class BTreeFiler extends BTree implements Filer {

    protected static final byte RECORD = 20;

    private static final short PAGESIZE = 512;
    // TODO: MAXKEYSIZE might need tuning
    private static final short MAXKEYSIZE = 256;

    private BTreeFilerHeader fileHeader;

    private static final int DBE_CANNOT_READ = (int) (572l);
    
    public BTreeFiler() {
        super();
        fileHeader = (BTreeFilerHeader) getFileHeader();
    }

    public void setLocation(String dir, String file) {
        setFile(new File(dir, file + ".tbl"));
    }

    public String getName() {
        return getFile().getName();
    }

    @Override
    public boolean open() throws DBException {
        if (super.open()) {
            // These are the only properties that can be changed after creation
            fileHeader.setMaxKeySize(MAXKEYSIZE);
            return true;
        } else {
            return false;
        }
    }

    @Override
    public boolean create() throws DBException {
        fileHeader.setPageSize(PAGESIZE);
        fileHeader.setMaxKeySize(MAXKEYSIZE);
        return super.create();
    }

    public Record readRecord(Key key) throws DBException {
        if (key == null || key.getLength() == 0) {
            return null;
        }

        checkOpened();
        try {
            long pos = findValue(key);
            Record record = readRecord(pos);

            record.setKey(key);
            return record;
        } catch (BTreeNotFoundException b) {// do nothing
        } catch (BTreeException b) {
            throw b;
        } catch (IOException e) {
            throw new FilerException(DBE_CANNOT_READ, "Can't read record '" + key + "': " + e.getMessage(), e);
        }
        return null;
    }

    public Record readRecord(long pos) throws DBException {
        checkOpened();
        try {
            Page startPage = getPage(pos);
            Value v = readValue(startPage);
            BTreeFilerPageHeader sph = (BTreeFilerPageHeader) startPage.getPageHeader();

            HashMap<String, Long> meta = new HashMap<String, Long>(4);

            meta.put(Record.CREATED, sph.getCreated());
            meta.put(Record.MODIFIED, sph.getModified());
            meta.put(Record.LIFETIME, sph.getLifetime());
            meta.put(Record.EXPIRATION, sph.getExpiration());

            return new Record(null, v, meta);
        } catch (IOException e) {
            throw new FilerException(DBE_CANNOT_READ, "Can't read record : " + e.getMessage(), e);
        }
    }

    public long writeRecord(Key key, Value value) throws DBException {

        return writeRecord(key, value, 0, 0);
    }

    public long writeRecord(Key key, Value value, long lifetime, long expiration) throws DBException {

        if (key == null || key.getLength() == 0) {
            throw new FilerException(FaultCodes.DBE_CANNOT_CREATE, "Invalid key: '" + key + "'");
        }
        if (value == null) {
            throw new FilerException(FaultCodes.DBE_CANNOT_CREATE, "Invalid null value");
        }
        checkOpened();
        try {
            Page p;
            long pos;
            try {
                pos = findValue(key);
                p = getPage(pos);
            } catch (BTreeNotFoundException b) {
                p = getFreePage();
                pos = p.getPageNum();
                addValue(key, p.getPageNum());
                fileHeader.incRecordCount();
            }
            BTreeFilerPageHeader ph = (BTreeFilerPageHeader) p.getPageHeader();

            long t = System.currentTimeMillis();

            if (ph.getStatus() == UNUSED) {
                ph.setCreated(t);
            }
         
            ph.setModified(t);
            ph.setLifetime(lifetime);
            ph.setExpiration(expiration);
            ph.setStatus(RECORD);

            writeValue(p, value);
            flush();
            return pos;
        } catch (IOException e) {
            throw new FilerException(FaultCodes.DBE_CANNOT_CREATE, "Can't write record '" + key + "': " + e.getMessage(), e);
        }
    }

    public long writeRecord(long pos, Value value) throws DBException {

        if (value == null) {
            throw new FilerException(FaultCodes.DBE_CANNOT_CREATE, "Invalid null value");
        }
        checkOpened();
        try {
            writeValue(pos, value);
            flush();
            return pos;
        } catch (IOException e) {
            throw new FilerException(FaultCodes.DBE_CANNOT_CREATE, "Can't write record '" + value + "': " + e.getMessage(), e);
        }
    }

    public boolean deleteRecord(Key key) throws DBException {
        if (key == null || key.getLength() == 0) {
            return false;
        }
        checkOpened();
        try {
            long pos = findValue(key);
            Page p = getPage(pos);

            removeValue(key);
            unlinkPages(p.getPageNum());

            fileHeader.decRecordCount();

            flush();

            return true;
        } catch (BTreeNotFoundException b) {// not found move on
        } catch (IOException e) {
            throw new FilerException(FaultCodes.DBE_CANNOT_DROP, "Can't delete record '" + key + "': " + e.getMessage(), e);
        }
        return false;
    }

    public long getRecordCount() throws DBException {
        checkOpened();
        return fileHeader.getRecordCount();
    }

    public RecordSet getRecordSet() throws DBException {
        checkOpened();
        return new BTreeFilerRecordSet();
    }

    /**
     * BTreeFilerRecordSet
     */

    private class BTreeFilerRecordSet implements RecordSet, BTreeCallback {
        private List<Key> keys = new ArrayList<Key>();
        private Iterator<Key> it;

        public BTreeFilerRecordSet() throws DBException {
            try {
                query(null, this);
                it = keys.iterator();
            } catch (IOException e) {
                throw new FilerException(FaultCodes.GEN_CRITICAL_ERROR, "Error generating RecordSet", e);
            }
        }

        public synchronized boolean indexInfo(Value value, long pointer) {
            keys.add(new Key(value));
            return true;
        }

        public synchronized Key getNextKey() {
            return it.next();
        }

        public synchronized Record getNextRecord() throws DBException {
            return readRecord(it.next());
        }

        public synchronized Value getNextValue() throws DBException {
            return getNextRecord().getValue();
        }

        public synchronized boolean hasMoreRecords() {
            return it.hasNext();
        }
    }

    // //////////////////////////////////////////////////////////////////

    @Override
    public FileHeader createFileHeader() {
        return new BTreeFilerHeader();
    }

    @Override
    public FileHeader createFileHeader(boolean read) throws IOException {
        return new BTreeFilerHeader(read);
    }

    @Override
    public FileHeader createFileHeader(long pageCount) {
        return new BTreeFilerHeader(pageCount);
    }

    @Override
    public FileHeader createFileHeader(long pageCount, int pageSize) {
        return new BTreeFilerHeader(pageCount, pageSize);
    }

    @Override
    public PageHeader createPageHeader() {
        return new BTreeFilerPageHeader();
    }

    /**
     * BTreeFilerHeader
     */

    private final class BTreeFilerHeader extends BTreeFileHeader {
        private long totalBytes = 0;

        public BTreeFilerHeader() {}

        public BTreeFilerHeader(long pageCount) {
            super(pageCount);
        }

        public BTreeFilerHeader(long pageCount, int pageSize) {
            super(pageCount, pageSize);
        }

        public BTreeFilerHeader(boolean read) throws IOException {
            super(read);
        }

        @Override
        public synchronized void read(RandomAccessFile raf) throws IOException {
            super.read(raf);
            totalBytes = raf.readLong();
        }

        @Override
        public synchronized void write(RandomAccessFile raf) throws IOException {
            super.write(raf);
            raf.writeLong(totalBytes);
        }

        /**
         * The total number of bytes in use by the file
         * @param totalBytes the new total number of bytes
         */
        public synchronized void setTotalBytes(long totalBytes) {
            this.totalBytes = totalBytes;
            setDirty();
        }

        /**
         * The total number of bytes in use by the file
         * @return the total number of bytes
         */
        public synchronized long getTotalBytes() {
            return totalBytes;
        }
    }


    /**
     * BTreeFilerPageHeader
     */

    private final class BTreeFilerPageHeader extends BTreePageHeader {
        private long created = 0;
        private long modified = 0;
        private long lifetime = 0;
        private long expiration = 0;

        public BTreeFilerPageHeader() {}

        public BTreeFilerPageHeader(DataInputStream dis) throws IOException {
            super(dis);
        }

        @Override
        public synchronized void read(DataInputStream dis) throws IOException {
            super.read(dis);

            if (getStatus() == UNUSED) {
                return;
            }

            created = dis.readLong();
            modified = dis.readLong();
            lifetime = dis.readLong();
            expiration = dis.readLong();
        }

        @Override
        public synchronized void write(DataOutputStream dos) throws IOException {
            super.write(dos);
            dos.writeLong(created);
            dos.writeLong(modified);
            dos.writeLong(lifetime);
            dos.writeLong(expiration);
        }

        @Override
        public synchronized void setRecordLen(int recordLen) {
            fileHeader.setTotalBytes((fileHeader.totalBytes - getRecordLen()) + recordLen);
            super.setRecordLen(recordLen);
        }

        /**
         * UNIX-time when this record was created
         * @param created creation time
         */
        public synchronized void setCreated(long created) {
            this.created = created;
            setDirty();
        }

        /**
         * UNIX-time when this record was created
         * @return creation time
         */
        public synchronized long getCreated() {
            return created;
        }

        /**
         * UNIX-time when this record was last modified
         * @param modified modified time
         */
        public synchronized void setModified(long modified) {
            this.modified = modified;
            setDirty();
        }

        /**
         *  UNIX-time when this record was last modified
         * @return modified time
         */
        public synchronized long getModified() {
            return modified;
        }

        /**
         *  JXTA-lifetime this record's lifetime
         * @param lifetime the new record lifetime
         */
        public synchronized void setLifetime(long lifetime) {
            this.lifetime = lifetime;
            setDirty();
        }

        /**
         * JXTA-lifetime this record's lifetime
         * @return the record lifetime
         */
        public synchronized long getLifetime() {
            return lifetime;
        }

        /**
         * JXTA-expiration this record's expiration
         * @param expiration the record expiration time
         */
        public synchronized void setExpiration(long expiration) {
            this.expiration = expiration;
            setDirty();
        }

        /**
         * JXTA-expiration this record's expiration
         * @return the record expiration time
         */
        public synchronized long getExpiration() {
            return expiration;
        }
    }
}
