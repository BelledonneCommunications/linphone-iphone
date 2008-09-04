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

import net.jxta.id.IDFactory;
import net.jxta.impl.util.TimeUtils;
import net.jxta.impl.xindice.core.DBException;
import net.jxta.impl.xindice.core.data.Key;
import net.jxta.impl.xindice.core.data.Record;
import net.jxta.impl.xindice.core.data.Value;
import net.jxta.impl.xindice.core.filer.BTreeCallback;
import net.jxta.impl.xindice.core.filer.BTreeFiler;
import net.jxta.impl.xindice.core.indexer.IndexQuery;
import net.jxta.impl.xindice.core.indexer.NameIndexer;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.UndeclaredThrowableException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * SrdiIndex
 */
public class SrdiIndex implements Runnable {
    
    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(SrdiIndex.class.getName());
    
    private long interval = 1000 * 60 * 10;
    private volatile boolean stop = false;
    private final Indexer srdiIndexer;
    private final BTreeFiler cacheDB;
    private Thread gcThread = null;
    private final Set<PeerID> gcPeerTBL = new HashSet<PeerID>();
    
    private final String indexName;
    
    /**
     * Constructor for the SrdiIndex
     *
     * @param group     group
     * @param indexName the index name
     */
    public SrdiIndex(PeerGroup group, String indexName) {
        this.indexName = indexName;
        
        try {
            String pgdir = null;
            File storeHome;
            
            if (group == null) {
                pgdir = "srdi-index";
                storeHome = new File(".jxta");
            } else {
                pgdir = group.getPeerGroupID().getUniqueValue().toString();
                storeHome = new File(group.getStoreHome());
            }
            
            File rootDir = new File(new File(storeHome, "cm"), pgdir);
            
            rootDir = new File(rootDir, "srdi");
            if (!rootDir.exists()) {
                // We need to create the directory
                if (!rootDir.mkdirs()) {
                    throw new RuntimeException("Cm cannot create directory " + rootDir);
                }
            }
            // peerid database
            // Storage
            cacheDB = new BTreeFiler();
            // lazy checkpoint
            cacheDB.setSync(false);
            cacheDB.setLocation(rootDir.getCanonicalPath(), indexName);
            
            if (!cacheDB.open()) {
                cacheDB.create();
                // now open it
                cacheDB.open();
            }
            
            // index
            srdiIndexer = new Indexer(false);
            srdiIndexer.setLocation(rootDir.getCanonicalPath(), indexName);
            if (!srdiIndexer.open()) {
                srdiIndexer.create();
                // now open it
                srdiIndexer.open();
            }
            
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("[" + ((group == null) ? "none" : group.toString()) + "] : Initialized " + indexName);
            }
        } catch (DBException de) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Unable to Initialize databases", de);
            }
            
            throw new UndeclaredThrowableException(de, "Unable to Initialize databases");
        } catch (Throwable e) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Unable to create Cm", e);
            }
            
            if (e instanceof Error) {
                throw (Error) e;
            } else if (e instanceof RuntimeException) {
                throw (RuntimeException) e;
            } else {
                throw new UndeclaredThrowableException(e, "Unable to create Cm");
            }
        }
    }
    
    /**
     * Construct a SrdiIndex and starts a GC thread which runs every "interval"
     * milliseconds
     *
     * @param interval  the interval at which the gc will run in milliseconds
     * @param group     group context
     * @param indexName SrdiIndex name
     */
    
    public SrdiIndex(PeerGroup group, String indexName, long interval) {
        this(group, indexName);
        this.interval = interval;
        startGC(group, indexName, interval);
    }
    
    /**
     * Start the GC thread
     *
     * @param group the PeerGroup
     * @param indexName index name
     * @param interval interval in milliseconds
     */
    protected void startGC(PeerGroup group, String indexName, long interval) {
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("[" + ((group == null) ? "none" : group.toString()) + "] : Starting SRDI GC Thread for " + indexName);
        }
        
        gcThread = new Thread(group.getHomeThreadGroup(), this, "SrdiIndex GC :" + indexName + " every " + interval + "ms");
        gcThread.setDaemon(true);
        gcThread.start();
    }
    
    /**
     * Returns the name of this srdi index.
     *
     * @return index name.
     */
    public String getIndexName() {
        return indexName;
    }
    
    /**
     * add an index entry
     *
     * @param primaryKey primary key
     * @param attribute  Attribute String to query on
     * @param value      value of the attribute string
     * @param expiration expiration associated with this entry relative time in
     *                   milliseconds
     * @param pid        peerid reference
     */
    public synchronized void add(String primaryKey, String attribute, String value, PeerID pid, long expiration) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("[" + indexName + "] Adding " + primaryKey + "/" + attribute + " = \'" + value + "\' for " + pid);
        }
        
        try {
            Key key = new Key(primaryKey + attribute + value);
            long expiresin = TimeUtils.toAbsoluteTimeMillis(expiration);
            
            // update the record if it exists
            synchronized (cacheDB) {
                // FIXME hamada 10/14/04 it is possible a peer re-appears with
                // a different set of indexes since it's been marked for garbage
                // collection.  will address this issue in a subsequent patch
                gcPeerTBL.remove(pid);
                
                Record record = cacheDB.readRecord(key);
                List<Entry> old;
                
                if (record != null) {
                    old = readRecord(record).list;
                } else {
                    old = new ArrayList<Entry>();
                }
                Entry entry = new Entry(pid, expiresin);
                
                if (!old.contains(entry)) {
                    old.add(entry);
                } else {
                    // entry exists, replace it (effectively updating expiration)
                    old.remove(old.indexOf(entry));
                    old.add(entry);
                }
                // no sense in keeping expired entries.
                old = removeExpired(old);
                    long t0 = TimeUtils.timeNow();
                byte[] data = getData(key, old);
                
                // if (LOG.isLoggable(Level.FINE)) {
                // LOG.fine("Serialized result in : " + (TimeUtils.timeNow() - t0) + "ms.");
                // }
                if (data == null) {
                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.severe("Failed to serialize data");
                    }
                    return;
                }
                Value recordValue = new Value(data);
                long pos = cacheDB.writeRecord(key, recordValue);
                Map<String, String> indexables = getIndexMap(primaryKey + attribute, value);
                
                srdiIndexer.addToIndex(indexables, pos);
            }
        } catch (IOException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to add SRDI", de);
            }
        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to add SRDI", de);
            }
        }
    }
    
    /**
     * retrieves a record
     *
     * @param pkey  primary key
     * @param skey  secondary key
     * @param value value
     * @return List of Entry objects
     */
    public List<Entry> getRecord(String pkey, String skey, String value) {
        Record record = null;
        
        try {
            Key key = new Key(pkey + skey + value);
            
            synchronized (cacheDB) {
                record = cacheDB.readRecord(key);
            }
        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to retrieve SrdiIndex record", de);
            }
        }
        // if record is null, readRecord returns an empty list
        return readRecord(record).list;
        
    }
    
    /**
     * inserts a pkey into a map with a value of value
     *
     * @param primaryKey primary key
     * @param value      value
     * @return The Map
     */
    
    private Map<String, String> getIndexMap(String primaryKey, String value) {
        if (primaryKey == null) {
            return null;
        }
        if (value == null) {
            value = "";
        }
        Map<String, String> map = new HashMap<String, String>(1);
        
        map.put(primaryKey, value);
        return map;
    }
    
    /**
     * remove entries pointing to peer id from cache
     *
     * @param pid peer id to remove
     */
    public synchronized void remove(PeerID pid) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(" Adding " + pid + " to peer GC table");
        }
        gcPeerTBL.add(pid);
    }
    
    /**
     * Query SrdiIndex
     *
     * @param attribute Attribute String to query on
     * @param value     value of the attribute string
     * @return an enumeration of canonical paths
     * @param primaryKey primary key
     * @param threshold max number of results
     */
    public synchronized List<PeerID> query(String primaryKey, String attribute, String value, int threshold) {
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("[" + indexName + "] Querying for " + threshold + " " + primaryKey + "/" + attribute + " = \'" + value + "\'");
        }
        
        // return nothing
        if (primaryKey == null) {
            return Collections.emptyList();
        }
        
        List<PeerID> res;
        
        // a blind query
        if (attribute == null) {
            res = query(primaryKey);
        } else {
            res = new ArrayList<PeerID>();
            
            IndexQuery iq = Cm.getIndexQuery(value);
            
            try {
                srdiIndexer.search(iq, primaryKey + attribute, new SearchCallback(cacheDB, res, threshold, gcPeerTBL));
            } catch (Exception ex) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Exception while searching in index", ex);
                }
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine( "[" + indexName + "] Returning " + res.size() + " results for " + primaryKey + "/" + attribute + " = \'"
                    + value + "\'");
        }
        
        return res;
    }
    
    /**
     * Query SrdiIndex
     *
     * @param primaryKey primary key
     * @return A list of Peer IDs.
     */
    public synchronized List<PeerID> query(String primaryKey) {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("[" + indexName + "] Querying for " + primaryKey);
        }
        
        List<PeerID> res = new ArrayList<PeerID>();
        
        try {
            Map<String, NameIndexer> map = srdiIndexer.getIndexers();
            
            for (Map.Entry<String, NameIndexer> index : map.entrySet()) {
                String indexName = index.getKey();
                // seperate the index name from attribute
                if (indexName.startsWith(primaryKey)) {
                    NameIndexer idxr = index.getValue();
                    idxr.query(null, new SearchCallback(cacheDB, res, Integer.MAX_VALUE, gcPeerTBL));
                }
            }
        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception while searching in index", ex);
            }
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("[" + indexName + "] Returning " + res.size() + " results for " + primaryKey);
        }
        
        return res;
    }
    
    private static final class SearchCallback implements BTreeCallback {
        private final BTreeFiler cacheDB;
        private final int threshold;
        private final List<PeerID> results;
        private final Set<PeerID> excludeTable;
        
        SearchCallback(BTreeFiler cacheDB, List<PeerID> results, int threshold, Set<PeerID> excludeTable) {
            this.cacheDB = cacheDB;
            this.threshold = threshold;
            this.results = results;
            this.excludeTable = excludeTable;
        }
        
        /**
         * @inheritDoc
         */
        public boolean indexInfo(Value val, long pos) {
            
            if (results.size() >= threshold) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("SearchCallback.indexInfo reached Threshold :" + threshold);
                }
                return false;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Found " + val);
            }
            Record record = null;
            
            try {
                record = cacheDB.readRecord(pos);
            } catch (DBException ex) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Exception while reading indexed", ex);
                }
                return false;
            }
            
            if (record != null) {
                long t0 = TimeUtils.timeNow();
                SrdiIndexRecord rec = readRecord(record);

                if (Logging.SHOW_FINEST && LOG.isLoggable(Level.FINEST)) {
                    LOG.finest("Got result back in : " + (TimeUtils.timeNow() - t0) + "ms.");
                }

                copyIntoList(results, rec.list, excludeTable);
            }
            
            return results.size() < threshold;
        }
    }
    
    
    private static final class GcCallback implements BTreeCallback {
        private final BTreeFiler cacheDB;
        private final Indexer idxr;
        private final List<Long> list;
        private final Set<PeerID> table;
        
        GcCallback(BTreeFiler cacheDB, Indexer idxr, List<Long> list, Set<PeerID> table) {
            this.cacheDB = cacheDB;
            this.idxr = idxr;
            this.list = list;
            this.table = table;
        }
        
        /**
         * @inheritDoc
         */
        public boolean indexInfo(Value val, long pos) {
            
            Record record = null;
            synchronized (cacheDB) {
                try {
                    record = cacheDB.readRecord(pos);
                } catch (DBException ex) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Exception while reading indexed", ex);
                    }
                    return false;
                }
                if (record == null) {
                    return true;
                }
                SrdiIndexRecord rec = readRecord(record);
                List<Entry> res = rec.list;
                boolean changed = false;
                
                Iterator<Entry> eachEntry = res.iterator();
                while(eachEntry.hasNext()) {
                    Entry entry = eachEntry.next();
                    
                    if (entry.isExpired() || table.contains(entry.peerid)) {
                        changed = true;
                        eachEntry.remove();
                    }
                }
                if (changed) {
                    if (res.isEmpty()) {
                        try {
                            cacheDB.deleteRecord(rec.key);
                            list.add(pos);
                        } catch (DBException e) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Exception while deleting empty record", e);
                            }
                        }                        
                    } else {
                        // write it back
                        byte[] data = getData(rec.key, res);
                        Value recordValue = new Value(data);
                        
                        try {
                            cacheDB.writeRecord(pos, recordValue);
                        } catch (DBException ex) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Exception while writing back record", ex);
                            }
                        }
                    }
                }
            }
            return true;
        }
    }
    
    /**
     * copies the content of List into a list. Expired entries are not
     * copied
     *
     * @param to   list to copy into
     * @param from list to copy from
     * @param table table of PeerID's
     */
    private static void copyIntoList(List<PeerID> to, List<Entry> from, Set<PeerID> table) {
        for (Entry entry : from) {
            boolean expired = entry.isExpired();
            
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("Entry peerid : " + entry.peerid + (expired ? " EXPIRED " : (" Expires at : " + entry.expiration)));
            }
            
            if (!to.contains(entry.peerid) && !expired) {
                if (!table.contains(entry.peerid)) {
                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.finer("adding Entry :" + entry.peerid + " to list");
                    }
                    to.add(entry.peerid);
                } else {
                    if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                        LOG.finer("Skipping gc marked entry :" + entry.peerid);
                    }
                }
            } else {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("Skipping expired Entry :" + entry.peerid);
                }
            }
        }
    }
    
    /**
     * Converts a List of {@link Entry} into a byte[]
     *
     * @param key  record key
     * @param list List to convert
     * @return byte []
     */
    private static byte[] getData(Key key, List<Entry> list) {
        try {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(bos);
            
            dos.writeUTF(key.toString());
            dos.writeInt(list.size());
            for (Entry anEntry : list) {
                dos.writeUTF(anEntry.peerid.toString());
                dos.writeLong(anEntry.expiration);
            }
            dos.close();
            return bos.toByteArray();
        } catch (IOException ie) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Exception while reading Entry", ie);
            }
        }
        return null;
    }
    
    /**
     * Reads the content of a record into List
     *
     * @param record Btree Record
     * @return List of entries
     */
    public static SrdiIndexRecord readRecord(Record record) {
        List<Entry> result = new ArrayList<Entry>();
        Key key = null;
        
        if (record == null) {
            return new SrdiIndexRecord(null, result);
        }
        if (record.getValue().getLength() <= 0) {
            return new SrdiIndexRecord(null, result);
        }
        InputStream is = record.getValue().getInputStream();
        
        try {
            DataInputStream ois = new DataInputStream(is);
            
            key = new Key(ois.readUTF());
            int size = ois.readInt();
            
            for (int i = 0; i < size; i++) {
                try {
                    String idstr = ois.readUTF();
                    PeerID pid = (PeerID) IDFactory.fromURI(new URI(idstr));
                    long exp = ois.readLong();
                    Entry entry = new Entry(pid, exp);
                    
                    result.add(entry);
                } catch (URISyntaxException badID) {
                    // ignored
                }
            }
            ois.close();
        } catch (EOFException eofe) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Empty record", eofe);
            }
        } catch (IOException ie) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception while reading Entry", ie);
            }
        }
        return new SrdiIndexRecord(key, result);
    }
    
    /**
     * Empties the index completely.
     * The entries are abandoned to the GC.
     */
    public synchronized void clear() {
        // FIXME changing the behavior a bit
        // instead of dropping all srdi entries, we let them expire
        // if that is not a desired behavior the indexer could be dropped
        // simply close it, and remove all index db created
        try {
            srdiIndexer.close();
            cacheDB.close();
        } catch (Exception e) {
            // bad bits we are done
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to close index", e);
            }
        }
    }
    
    /**
     * Garbage Collect expired entries
     */
    public void garbageCollect() {
        try {
            Map<String, NameIndexer> map = srdiIndexer.getIndexers();
            
            for(NameIndexer idxr : map.values()) {
                List<Long> list = new ArrayList<Long>();
                
                if(stop) {
                    break;
                }
                
                synchronized(this) {
                    idxr.query(null, new GcCallback(cacheDB, srdiIndexer, list, gcPeerTBL));
                    srdiIndexer.purge(list);
                }
            }
            gcPeerTBL.clear();
        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failure during SRDI Garbage Collect", ex);
            }
        }
    }
    
    /**
     * Remove expired entries from a List
     *
     * @param list A list of entries.
     * @return The same list with the expired entries removed.
     */
    private static List<Entry> removeExpired(List<Entry> list) {
        Iterator<Entry> eachEntry = list.iterator();
        
        while(eachEntry.hasNext()) {
            Entry entry = eachEntry.next();
            
            if (entry.isExpired()) {
                eachEntry.remove();
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Removing expired Entry peerid :" + entry.peerid + " Expires at :" + entry.expiration);
                }
            }
        }
        return list;
    }
    
    private static boolean isExpired(long expiration) {
        return (TimeUtils.timeNow() > expiration);
    }
    
    /**
     * stop the current running thread
     */
    public synchronized void stop() {
        if(stop) {
            return;
        }
        
        stop = true;
        
        // wakeup and die
        try {
            Thread temp = gcThread;
            if (temp != null) {
                synchronized (temp) {
                    temp.notify();
                }
            }
        } catch (Exception ignored) {
            // ignored
        }
        
        // Stop the database
        
        try {
            srdiIndexer.close();
            cacheDB.close();
            gcPeerTBL.clear();
        } catch (Exception ex) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Unable to stop the Srdi Indexer", ex);
            }
        }
    }
    
    /**
     * {@inheritDoc}
     * <p/>
     * Periodic thread for GC
     */
    public void run() {
        try {
            while (!stop) {
                try {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Waiting for " + interval + "ms before garbage collection");
                    }
                    synchronized (gcThread) {
                        gcThread.wait(interval);
                    }
                } catch (InterruptedException woken) {
                    // The only reason we are woken is to stop.
                    Thread.interrupted();
                    continue;
                }
                
                if (stop) {
                    break;
                }
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Garbage collection started");
                }
                
                garbageCollect();
                
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Garbage collection completed");
                }
            }
        } catch (Throwable all) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            }
        } finally {
            synchronized (this) {
                gcThread = null;
            }
        }
    }
    
    /**
     * Flushes the Srdi directory for a specified group
     * this method should only be called before initialization of a given group
     * calling this method on a running group would have undefined results
     *
     * @param group group context
     */
    public static void clearSrdi(PeerGroup group) {
        
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Clearing SRDI for " + group.getPeerGroupName());
        }
        
        try {
            String pgdir = null;
            
            if (group == null) {
                pgdir = "srdi-index";
            } else {
                pgdir = group.getPeerGroupID().getUniqueValue().toString();
            }
            File rootDir = null;
            
            if (group != null) {
                rootDir = new File(new File(new File(group.getStoreHome()), "cm"), pgdir);
            }
            
            rootDir = new File(rootDir, "srdi");
            if (rootDir.exists()) {
                // remove it along with it's content
                String[] list = rootDir.list();
                
                for (String aList : list) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Removing : " + aList);
                    }
                    File file = new File(rootDir, aList);
                    
                    if (!file.delete()) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.warning("Unable to delete the file");
                        }
                    }
                }
                rootDir.delete();
            }
        } catch (Throwable t) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Unable to clear Srdi", t);
            }
        }
    }
    
    /**
     * An entry in the index tables.
     */
    public final static class Entry {
        
        public final PeerID peerid;
        public final long expiration;
        
        /**
         * Peer Pointer reference
         *
         * @param peerid     PeerID for this entry
         * @param expiration the expiration for this entry
         */
        public Entry(PeerID peerid, long expiration) {
            this.peerid = peerid;
            this.expiration = expiration;
        }
        
        /**
         * {@inheritDoc}
         */
        @Override
        public boolean equals(Object obj) {
            return obj instanceof Entry && (peerid.equals(((Entry) obj).peerid));
        }
        
        /**
         * {@inheritDoc}
         */
        @Override
        public int hashCode() {
            return peerid.hashCode();
        }
        
        /**
         *  Return the absolute time in milliseconds at which this entry will
         *  expire.
         *
         *  @return The absolute time in milliseconds at which this entry will
         *  expire.
         */
        public long getExpiration() {
            return expiration;
        }
        
        /**
         *  Return {@code true} if this entry is expired.
         *
         *  @return {@code true} if this entry is expired otherwise {@code false}.
         */
        public boolean isExpired() {
            return TimeUtils.timeNow() > expiration;
        }
    }
    
    
    /**
     * an SrdiIndexRecord wrapper
     */
    public final static class SrdiIndexRecord {
        
        public final Key key;
        public final List<Entry> list;
        
        /**
         * SrdiIndex record
         *
         * @param key  record key
         * @param list record entries
         */
        public SrdiIndexRecord(Key key,List<Entry> list) {
            this.key = key;
            this.list = list;
        }
        
        /**
         * {@inheritDoc}
         */
        @Override
        public boolean equals(Object obj) {
            return obj instanceof SrdiIndexRecord && (key.equals(((SrdiIndexRecord) obj).key));
        }
        
        /**
         * {@inheritDoc}
         */
        @Override
        public int hashCode() {
            return key.hashCode();
        }
    }
}

