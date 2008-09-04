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
package net.jxta.impl.cm;

import net.jxta.impl.xindice.core.DBException;
import net.jxta.impl.xindice.core.data.Key;
import net.jxta.impl.xindice.core.data.Record;
import net.jxta.impl.xindice.core.data.Value;
import net.jxta.impl.xindice.core.filer.BTreeCallback;
import net.jxta.impl.xindice.core.filer.BTreeException;
import net.jxta.impl.xindice.core.filer.BTreeFiler;
import net.jxta.impl.xindice.core.indexer.IndexQuery;
import net.jxta.impl.xindice.core.indexer.NameIndexer;
import net.jxta.logging.Logging;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class Indexer {

    /**
     * The logger
     */
    private final static transient Logger LOG = Logger.getLogger(Indexer.class.getName());

    private final static String listFileName = "offsets";

    private String dir = null;
    private String file = null;
    private final Map<String, NameIndexer> indices = new HashMap<String, NameIndexer>();
    private BTreeFiler listDB = null;
    private boolean sync = true;

    /*
     *      Indexer manages indexes to various advertisement types,
     *      and maintains a listDB which holds records that hold references
     *      to records in advertisments.tbl
     *
     *       -------          -------               /    ------- 
     *      | index | ---->> | listDB |   ------->>  -   | advDB |
     *       -------          -------               \    ------- 
     *
     */
    public Indexer() {}

    /**
     * Creates an indexer
     *
     * @param sync passed through to xindice to determine a lazy checkpoint or not
     *             false == lazy checkpoint
     */
    public Indexer(boolean sync) {
        this.sync = sync;
    }

    public void setLocation(String dir, String file) {
        this.dir = dir;
        this.file = file;

        // upon restart, load existing indices

        File directory = new File(dir);
        File[] indexFiles = directory.listFiles(new FilenameFilter() {
            public boolean accept(File parentDir, String fileName) {
                return fileName.endsWith(".idx");
            }
        });

        for (File indexFile : indexFiles) {
            String indexFileName = indexFile.getName();
            int dash = indexFileName.lastIndexOf("-");
            int dot = indexFileName.lastIndexOf(".idx");

            if (dot > 0 && dash > 0) {
                String name = indexFileName.substring(dash + 1, dot).trim();

                if (indices.get(name) == null) {
                    try {
                        NameIndexer indexer = new NameIndexer();

                        // location should be the same as in
                        // addToIndex below
                        indexer.setLocation(dir, file + "-" + name);
                        indexer.setSync(sync);
                        if (!indexer.open()) {
                            indexer.create();
                            indexer.open();
                        }
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Adding :" + indexFileName + " under " + name);
                        }
                        indices.put(name, indexer);
                    } catch (DBException ignore) {
                        if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                            LOG.log(Level.SEVERE, "Failed to create Index " + name, ignore);
                        }
                    }
                }
            }
        }
        try {
            // record pointers
            listDB = new BTreeFiler();
            listDB.setSync(sync);
            listDB.setLocation(directory.getCanonicalPath(), file + "-" + listFileName);
            if (!listDB.open()) {
                listDB.create();
                // now open it
                listDB.open();
            }
        } catch (DBException dbe) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Failed during listDB Creation", dbe);
            }
        } catch (IOException ie) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Failed during listDB Creation", ie);
            }
        }
    }

    public boolean open() throws DBException {
        return true;
    }

    public boolean create() throws DBException {
        return true;
    }

    public synchronized boolean close() throws DBException {

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Closing Indexer");
        }
                
        Iterator<Map.Entry<String, NameIndexer>> eachIndex = indices.entrySet().iterator();
        while (eachIndex.hasNext()) {
            Map.Entry<String, NameIndexer> anEntry = eachIndex.next();
            
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Closing Index :" + anEntry.getKey());
            }
            
            try {
                anEntry.getValue().close();
            } catch (Exception failed) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failure closing index :" + anEntry.getKey(), failed);
                }
            }
            
            eachIndex.remove();
        }
        
        // clear just in case.
        indices.clear();
        
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Closing listDB");
        }
        
        listDB.close();
        return true;
    }

    /**
     * returns an iteration of index fields (attributes)
     */
    public Map<String, NameIndexer> getIndexers() {
        return Collections.unmodifiableMap(indices);
    }

    /**
     * returns listDB
     */
    public BTreeFiler getListDB() {
        return listDB;
    }

    private static final class EndsWithCallback implements BTreeCallback {

        private int op = IndexQuery.ANY;
        private BTreeCallback callback = null;
        private Value pattern = null;

        EndsWithCallback(int op, BTreeCallback callback, Value pattern) {
            this.op = op;
            this.callback = callback;
            this.pattern = pattern;
        }

        /**
         * {@inheritDoc}
         */
        public boolean indexInfo(Value val, long pos) {

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("value :" + val + " pattern :" + pattern);
            }

            switch (op) {
            case IndexQuery.EW:
                if (val.endsWith(pattern)) {
                    return callback.indexInfo(val, pos);
                }
                break;

            case IndexQuery.NEW:
                if (!val.endsWith(pattern)) {
                    return callback.indexInfo(val, pos);
                }
                break;

            case IndexQuery.BWX:
                if (val.contains(pattern)) {
                    return callback.indexInfo(val, pos);
                }
                break;

            default:
                break;

            }
            return true;
        }
    }

    public void search(IndexQuery query, String name, BTreeCallback callback) throws IOException, BTreeException {

        BTreeCallback cb = new SearchCallback(listDB, callback);

        if (query != null) {
            int op = query.getOperator();
            if (op == IndexQuery.EW || op == IndexQuery.NEW || op == IndexQuery.BWX) {
                query = new IndexQuery(IndexQuery.ANY, query.getValues());
                cb = new EndsWithCallback(op, new SearchCallback(listDB, callback), query.getValue(0));
            }
        }

        if (name == null) {
            if (indices != null) {
                Iterator<NameIndexer> i = indices.values().iterator();

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Searching all indexes");
                }
                while (i.hasNext()) {
                    NameIndexer index = i.next();
                    index.query(query, new SearchCallback(listDB, callback));
                }
            }
        } else {
            NameIndexer indexer = indices.get(name);
            if (indexer == null) {
                return;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Searching Index : " + name);
            }
            indexer.query(query, cb);
        }
    }

    public void addToIndex(Map<String, String> indexables, long pos) throws IOException, DBException {

        if (indexables == null) {
            return;
        }
        // FIXME add indexer name to NameIndexer, to optimize this loop
        for (String name : indexables.keySet()) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("looking up NameIndexer : " + name);
            }
            NameIndexer indexer = indices.get(name);

            if (indexer == null) {
                indexer = new NameIndexer();
                // location should be the same as in setLocation above
                indexer.setLocation(dir, file + "-" + name);
                indexer.setSync(sync);
                if (!indexer.open()) {
                    indexer.create();
                    indexer.open();
                }
                indices.put(name, indexer);
            }

            // we need to make sure that the db key is unique from the
            // the index key to avoid value collision
            Key dbKey = new Key(name + indexables.get(name));
            Key indexKey = new Key(indexables.get(name));
            long listPos = writeRecord(listDB, dbKey, pos);

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                StringBuilder message = new StringBuilder().append("Adding a reference at position :").append(listPos).append(" to ").append(name).append(" index, Key: ").append(
                        indexables.get(name));
                LOG.finer(message.toString());
            }
            indexer.add(indexKey, listPos);
        }
    }

    public void removeFromIndex(Map<String, String> indexables, long pos) throws DBException {

        Collection<String> names;

        if (indexables == null) {
            names = indices.keySet();
        } else {
            names = indexables.keySet();
        }

        Long lpos = pos;

        for (String name : names) {
            NameIndexer indexer = indices.get(name);

            if (indexer != null) {
                // we need to make sure that the db key is unique from the
                // the index key to avoid value collision
                Key dbKey = null;

                if (indexables != null) {
                    dbKey = new Key(name + indexables.get(name));
                }
                Key indexKey = null;

                if (indexables != null) {
                    indexKey = new Key(indexables.get(name));
                }
                synchronized (listDB) {
                    Record record = listDB.readRecord(dbKey);
                    Set<Long> offsets = readRecord(record);

                    if (!offsets.isEmpty()) {
                        if (offsets.contains(lpos)) {
                            offsets.remove(lpos);
                            Value recordValue = new Value(toByteArray(offsets));

                            listDB.writeRecord(dbKey, recordValue);
                        }
                        if (offsets.isEmpty()) {
                            // only we can proceed to remove the entry from the index
                            listDB.deleteRecord(dbKey);
                            indexer.remove(indexKey);
                        }
                    } else {
                        // empty record purge it
                        listDB.deleteRecord(dbKey);
                        indexer.remove(indexKey);
                    }
                }
            }
        }
    }

    /**
     * purge all index entries pointing to a certain record.
     *
     * @param list List of Long position(s) at which the record to be purged is
     *             located in the main database.
     * @throws IOException if an io error occurs
     * @throws BTreeException if an DB error occurs
     */
    public void purge(List<Long> list) throws IOException, BTreeException {
        IndexQuery iq = new IndexQuery(IndexQuery.ANY, "");
        Collection<String> keys = new ArrayList<String>(indices.keySet());

        for (String objKey : keys) {
            NameIndexer index = indices.get(objKey);
            PurgeCallback pc = new PurgeCallback(listDB, index, objKey, list);

            index.query(iq, pc);
        }
    }

    /**
     * purge all index entries pointing to a certain record.
     *
     * @param pos the position at which the record to be purged is
     *            located in the main database.
     * @throws IOException    if an io error occurs
     * @throws BTreeException if an BTree error occurs
     */
    public void purge(long pos) throws IOException, BTreeException {
        purge(Collections.<Long>singletonList(pos));
    }

    private static final class PurgeCallback implements BTreeCallback {

        private final NameIndexer indexer;
        private final List<Long> list;
        private final BTreeFiler listDB;
        private final String indexKey;

        PurgeCallback(BTreeFiler listDB, NameIndexer indexer, String indexKey, List<Long> list) {
            this.listDB = listDB;
            this.indexer = indexer;
            this.indexKey = indexKey;
            this.list = list;
        }

        /**
         * {@inheritDoc}
         */
        public boolean indexInfo(Value val, long pos) {
            // Read record to determine whether there's a refrence to pos
            try {
                synchronized (listDB) {
                    Record record = listDB.readRecord(pos);
                    Set<Long> offsets = readRecord(record);

                    boolean changed = offsets.removeAll(list);
                    if (changed) {
                        if (!offsets.isEmpty()) {
                            Value recordValue = new Value(toByteArray(offsets));

                            listDB.writeRecord(pos, recordValue);
                        } else {
                            listDB.deleteRecord(new Key(indexKey + val));
                            indexer.remove(new Key(val));
                        }
                    }
                }
            } catch (DBException ignore) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "An exception occured", ignore);
                }
            }
            return true;
        }
    }

    private static byte[] toByteArray(Set<Long> offsets) {
        try {
            int size = offsets.size();
            ByteArrayOutputStream bos = new ByteArrayOutputStream((size * 8) + 4);
            DataOutputStream dos = new DataOutputStream(bos);

            dos.writeInt(size);
            for (Long lpos : offsets) {
                dos.writeLong(lpos.longValue());
            }
            dos.close();
            return bos.toByteArray();
        } catch (IOException ie) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception during array to byte array conversion", ie);
            }
        }
        return null;
    }

    public static Set<Long> readRecord(Record record) {
        Set<Long> result = new TreeSet<Long>();

        if (record == null) {
            return result;
        }

        InputStream is = record.getValue().getInputStream();

        try {
            DataInputStream ois = new DataInputStream(is);
            int size = ois.readInt();

            for (int i = 0; i < size; i++) {
                result.add(ois.readLong());
            }
            ois.close();
        } catch (IOException ie) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception while reading Entry", ie);
            }
        }
        return result;
    }

    private static long writeRecord(BTreeFiler listDB, Key key, long pos) throws DBException, IOException {

        synchronized (listDB) {
            Long lpos = pos;
            Record record = listDB.readRecord(key);
            Set<Long> offsets = readRecord(record);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE) && offsets != null) {
                LOG.finer("list.contains " + pos + " : " + offsets.contains(lpos));
            }

            if (offsets != null && !offsets.contains(lpos)) {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("Adding a reference to record at :" + lpos);
                    LOG.finer("Writing :" + offsets.size() + " references");
                }
                offsets.add(lpos);
            }
            Value recordValue = new Value(toByteArray(offsets));

            return listDB.writeRecord(key, recordValue);
        }
    }

    public static final class SearchCallback implements BTreeCallback {

        private BTreeCallback callback = null;
        private BTreeFiler listDB = null;

        public SearchCallback(BTreeFiler listDB, BTreeCallback callback) {
            this.listDB = listDB;
            this.callback = callback;
        }

        /**
         * {@inheritDoc}
         */
        public boolean indexInfo(Value val, long pos) {
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Found " + val.toString() + " at " + pos);
            }
            Record record = null;
            Set<Long> offsets = null;
            boolean result = true;

            try {
                synchronized (listDB) {
                    record = listDB.readRecord(pos);
                    offsets = readRecord(record);
                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.finer("Found " + offsets.size() + " entries");
                    }
                }

                for (Long lpos : offsets) {
                    result &= callback.indexInfo(val, lpos);
                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.finer("Callback result : " + result);
                    }
                }
            } catch (DBException ex) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Exception while reading indexed", ex);
                }
                return false;
            }
            return result;
        }
    }
}
