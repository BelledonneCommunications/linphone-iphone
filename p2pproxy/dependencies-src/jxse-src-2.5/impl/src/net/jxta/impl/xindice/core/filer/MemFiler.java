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
import net.jxta.impl.xindice.core.data.Record;
import net.jxta.impl.xindice.core.data.RecordSet;

import java.io.File;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * MemFiler is an In-Memory Filer implementation for Xindice.  MemFiler can be
 * used for temporary collections and caching.  It's basically a layering on
 * top of HashMap.
 */
public final class MemFiler implements Filer {
    private Map<Key, Record> hashTable = null;
    private Map<Long, Key> posTable = null;
    private boolean opened = false;
    private boolean readOnly = false;
    private long position = 0;
    public MemFiler() {
        hashTable = Collections.synchronizedMap(new HashMap<Key, Record>());
        posTable = Collections.synchronizedMap(new HashMap<Long, Key>());
    }

    public MemFiler(Map<Key, Record> hashTable, boolean readOnly) {
        this.hashTable = hashTable;
        this.readOnly = readOnly;
    }

    public MemFiler(Map<Key, Record> hashTable) {
        this(hashTable, false);
    }

    public void setLocation(File root, String location) {}

    public String getName() {
        return "MemFiler";
    }

    private void checkOpened() throws DBException {
        if (!opened) {
            throw new FilerException(FaultCodes.COL_COLLECTION_CLOSED, "Filer is closed");
        }
    }

    private void checkReadOnly() throws DBException {
        if (readOnly) {
            throw new FilerException(FaultCodes.COL_COLLECTION_READ_ONLY, "Filer is read-only");
        }
    }

    public boolean create() {
        hashTable.clear();
        return true;
    }

    public boolean open() {
        opened = true;
        return opened;
    }

    public boolean isOpened() {
        return opened;
    }

    public boolean exists() {
        return true;
    }

    public boolean drop() {
        hashTable.clear();
        opened = false;
        return !opened;
    }

    public boolean close() {
        opened = false;
        return !opened;
    }

    public void flush() {}

    public Record readRecord(Key key) throws DBException {
        if (key == null || key.getLength() == 0) {
            return null;
        }
        checkOpened();
        return hashTable.get(key);
    }
   
    public Record readRecord(long pos) throws DBException {
        if (pos < 0) {
            return null;
        }
        checkOpened();
        Key key = posTable.get(pos);

        return hashTable.get(key);
    }
   
    public long writeRecord(Key key, Value value) throws DBException {
        if (key == null || key.getLength() == 0) {
            throw new FilerException(FaultCodes.DBE_CANNOT_CREATE, "Invalid key: '" + key + "'");
        }
        if (value == null) {
            throw new FilerException(FaultCodes.DBE_CANNOT_CREATE, "Invalid null value");
        }
        checkOpened();
        checkReadOnly();
        hashTable.put(key, new Record(key, value));
        posTable.put(position, key);
        long result = position;

        position++;
        return result;
    }
   
    public boolean deleteRecord(Key key) throws DBException {
        if (key == null || key.getLength() == 0) {
            return false;
        }
        checkOpened();
        checkReadOnly();
        return hashTable.remove(key) != null;
    }

    public long getRecordCount() throws DBException {
        checkOpened();
        return hashTable.size();
    }

    public RecordSet getRecordSet() throws DBException {
        checkOpened();
        return new MemRecordSet();
    }

    /**
     * MemRecordSet
     */

    private class MemRecordSet implements RecordSet {
        private Iterator<Record> it = hashTable.values().iterator();

        public synchronized boolean hasMoreRecords() throws DBException {
            return it.hasNext();
        }

        public synchronized Record getNextRecord() throws DBException {
            checkOpened();
            return it.next();
        }

        public synchronized Value getNextValue() throws DBException {
            checkOpened();
            return (it.next()).getValue();
        }

        public synchronized Key getNextKey() {
            return (it.next()).getKey();
        }
    }
}

