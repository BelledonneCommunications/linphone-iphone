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

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.XMLDocument;
import net.jxta.impl.util.JxtaHash;
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
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PeerGroupAdvertisement;
import net.jxta.protocol.SrdiMessage;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringWriter;
import java.lang.reflect.UndeclaredThrowableException;
import java.math.BigInteger;
import java.net.URI;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * This class implements a limited document caching mechanism
 * intended to provide cache for services that have a need for cache
 * to search and exchange jxta documents.
 * <p/>
 * Only Core Services are intended to use this mechanism.
 */
public final class Cm implements Runnable {

    /**
     * Logger.
     */
    private final static Logger LOG = Logger.getLogger(Cm.class.getName());

    /**
     * the name we will use for the base directory
     */
    final File ROOTDIRBASE;

    /**
     * adv types
     */
    private static final String[] DIRNAME = {"Peers", "Groups", "Adv", "Raw"};

    // garbage collect once an hour
    public static final long DEFAULT_GC_MAX_INTERVAL = 1 * TimeUtils.ANHOUR;

    /*
     *  expiration db
     */
    private BTreeFiler cacheDB = null;
    private Indexer indexer = null;
    private final static String databaseFileName = "advertisements";

    private boolean stop = false;

    private boolean trackDeltas = false;
    private final Map<String, List<SrdiMessage.Entry>> deltaMap = new HashMap<String, List<SrdiMessage.Entry>>(3);

    /**
     * file descriptor for the root of the cm
     */
    protected File rootDir;

    private Thread gcThread = null;
    private long gcTime = 0;
    private final long gcMinInterval = 1000L * 60L;
    private long gcMaxInterval = DEFAULT_GC_MAX_INTERVAL;
    
    
    private final int maxInconvenienceLevel = 1000;
    private volatile int inconvenienceLevel = 0;

    /**
     * Constructor for cm
     *
     * @param areaName  the name of the cm sub-dir to create
     *                  <p/>
     *                  NOTE: Default garbage interval once an hour
     * @param storeRoot store root dir
     */
    public Cm(URI storeRoot, String areaName) {
        // Default garbage collect once an hour
        this(Thread.currentThread().getThreadGroup(), storeRoot, areaName, DEFAULT_GC_MAX_INTERVAL, false);
    }

    /**
     * Constructor for cm
     *
     * @param threadGroup     the thread group
     * @param storeRoot   persistence location
     * @param gcinterval  garbage collect max interval
     * @param trackDeltas when true deltas are tracked
     * @param areaName    storage area name
     */
    public Cm(ThreadGroup threadGroup, URI storeRoot, String areaName, long gcinterval, boolean trackDeltas) {
        this.trackDeltas = trackDeltas;
        this.gcMaxInterval = gcinterval;
        this.gcTime = System.currentTimeMillis() + gcMaxInterval;

        ROOTDIRBASE = new File(new File(storeRoot), "cm");

        try {
            rootDir = new File(ROOTDIRBASE, areaName);
            rootDir = new File(rootDir.getAbsolutePath());
            if (!rootDir.exists()) {
                // We need to create the directory
                if (!rootDir.mkdirs()) {
                    throw new RuntimeException("Cm cannot create directory " + rootDir);
                }
            }

            /*
             * to avoid inconsistent database state, it is highly recommended that
             * checkpoint is true by default, which causes fd.sync() on every write
             * operation.  In transitory caches such as SrdiCache it makes perfect sense
             */
            boolean chkPoint = true;
            ResourceBundle jxtaRsrcs = ResourceBundle.getBundle("net.jxta.user");
            String checkpointStr = jxtaRsrcs.getString("impl.cm.defferedcheckpoint");

            if (checkpointStr != null) {
                chkPoint = !(checkpointStr.equalsIgnoreCase("true"));
            }

            // Storage
            cacheDB = new BTreeFiler();
            // no deffered checkpoint
            cacheDB.setSync(chkPoint);
            cacheDB.setLocation(rootDir.getAbsolutePath(), databaseFileName);

            if (!cacheDB.open()) {
                cacheDB.create();
                // now open it
                cacheDB.open();
            }

            // Index
            indexer = new Indexer(chkPoint);
            indexer.setLocation(rootDir.getAbsolutePath(), databaseFileName);

            if (!indexer.open()) {
                indexer.create();
                // now open it
                indexer.open();
            }

            if (System.getProperty("net.jxta.impl.cm.index.rebuild") != null) {
                rebuildIndex();
            }
            gcThread = new Thread(threadGroup, this, "CM GC Thread interval : " + gcMinInterval);
            gcThread.setDaemon(true);
            gcThread.start();

            if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                LOG.config("Instantiated Cm for: " + rootDir.getAbsolutePath());
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
            if (e instanceof RuntimeException) {
                throw (RuntimeException) e;
            } else if (e instanceof Error) {
                throw (Error) e;
            } else {
                throw new UndeclaredThrowableException(e, "Unable to create Cm");
            }
        }
    }

    @Override
    public String toString() {
        return "CM for " + rootDir.getAbsolutePath() + "[" + super.toString() + "]";
    }

    private static String getDirName(Advertisement adv) {
        if (adv instanceof PeerAdvertisement) {
            return DIRNAME[DiscoveryService.PEER];
        } else if (adv instanceof PeerGroupAdvertisement) {
            return DIRNAME[DiscoveryService.GROUP];
        }
        return DIRNAME[DiscoveryService.ADV];
    }

    /**
     * Generates a random file name using doc hashcode
     *
     * @param doc to hash to generate a unique name
     * @return String a random file name
     */
    public static String createTmpName(StructuredTextDocument doc) {
        try {
            StringWriter out = new StringWriter();

            doc.sendToWriter(out);
            out.close();

            JxtaHash digester = new JxtaHash(out.toString());
            BigInteger hash = digester.getDigestInteger();

            if (hash.compareTo(BigInteger.ZERO) < 0) {
                hash = hash.negate();
            }
            return "cm" + hash.toString(16);
        } catch (IOException ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception creating tmp name: ", ex);
            }
            throw new IllegalStateException("Could not generate name from document");
        }
    }

    /**
     * Gets the list of all the files into the given folder
     *
     * @param dn          contains the name of the folder
     * @param threshold   the max number of results
     * @param expirations List to contain expirations
     * @return List Strings containing the name of the
     *         files
     */
    public List<InputStream> getRecords(String dn, int threshold, List<Long> expirations) {
        return getRecords(dn, threshold, expirations, false);
    }

    public synchronized List<InputStream> getRecords(String dn, int threshold, List<Long> expirations, boolean purge) {
        List<InputStream> res = new ArrayList<InputStream>();

        if (dn == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("null directory name");
            }
            return res;
        } else {
            IndexQuery iq = new IndexQuery(IndexQuery.SW, new Value(dn));
            try {
                cacheDB.query(iq, new SearchCallback(cacheDB, indexer, res, expirations, threshold, purge));
            } catch (DBException dbe) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "Exception during getRecords(): ", dbe);
                }
            } catch (IOException ie) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.log(Level.FINE, "Exception during getRecords(): ", ie);
                }
            }
            return res;
        }
    }

    public void garbageCollect() {
        // calling getRecords is good enough since it removes
        // expired entries
        Map map = indexer.getIndexers();
        Iterator it = map.keySet().iterator();
        long t0;

        while (it != null && it.hasNext()) {
            t0 = System.currentTimeMillis();
            String indexName = (String) it.next();
            getRecords(indexName, Integer.MAX_VALUE, null, true);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Cm garbageCollect :" + indexName + " in :" + (System.currentTimeMillis() - t0));
            }
        }
    }

    /**
     * Returns the relative time in milliseconds at which the file
     * will expire.
     *
     * @param dn contains the name of the folder
     * @param fn contains the name of the file
     * @return the absolute time in milliseconds at which this
     *         document will expire. -1 is returned if the file is not
     *         recognized or already expired.
     */
    public synchronized long getLifetime(String dn, String fn) {
        try {
            Key key = new Key(dn + "/" + fn);
            Record record = cacheDB.readRecord(key);

            if (record == null) {
                return -1;
            }
            Long life = (Long) record.getMetaData(Record.LIFETIME);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Lifetime for :" + fn + "  " + life.toString());
            }
            if (life < System.currentTimeMillis()) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Removing expired record :" + fn);
                }
                try {
                    remove(dn, fn);
                } catch (IOException e) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Failed to remove record", e);
                    }
                }
            }
            return TimeUtils.toRelativeTimeMillis(life);
        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to remove " + dn + "/" + fn, de);
            }
            return -1;
        }
    }

    /**
     * Returns the maximum duration in milliseconds for which this
     * document should cached by those other than the publisher. This
     * value is either the cache lifetime or the remaining lifetime
     * of the document, whichever is less.
     *
     * @param dn contains the name of the folder
     * @param fn contains the name of the file
     * @return number of milliseconds until the file expires or -1 if the
     *         file is not recognized or already expired.
     */
    public synchronized long getExpirationtime(String dn, String fn) {
        try {
            Key key = new Key(dn + "/" + fn);
            Record record = cacheDB.readRecord(key);
            long expiration = calcExpiration(record);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Expiration for :" + fn + "  " + expiration);
            }
            if (expiration < 0) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Removing expired record :" + fn);
                }
                try {
                    remove(dn, fn);
                } catch (IOException e) {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Failed to remove record", e);
                    }
                }
            }
            return expiration;
        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to get " + dn + "/" + fn, de);
            }
            return -1;
        }
    }

    /**
     * Figures out expiration
     *
     * @param record record
     * @return expiration in ms
     */
    private static long calcExpiration(Record record) {
        if (record == null) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Record is null returning expiration of -1");
            }
            return -1;
        }
        Long exp = (Long) record.getMetaData(Record.EXPIRATION);
        Long life = (Long) record.getMetaData(Record.LIFETIME);
        long expiresin = life - System.currentTimeMillis();

        if (expiresin <= 0) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(
                        MessageFormat.format("Record expired lifetime   : {0} expiration: {1} expires in: {2}", life, exp
                                ,
                                expiresin));
                LOG.fine(MessageFormat.format("Record expires on :{0}", new Date(life)));
            }
            return -1;
        } else {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("Record lifetime: {0} expiration: {1} expires in: {2}", life, exp, expiresin));
                LOG.fine(MessageFormat.format("Record expires on :{0}", new Date(life)));
            }
            return Math.min(expiresin, exp.longValue());
        }
    }

    /**
     * Returns the inputStream of a specified file, in a specified dir
     *
     * @param dn directory name
     * @param fn file name
     * @return The inputStream value
     * @throws IOException if an I/O error occurs
     */
    public InputStream getInputStream(String dn, String fn) throws IOException {
        Key key = new Key(dn + "/" + fn);
        try {
            Record record = cacheDB.readRecord(key);
            if (record == null) {
                return null;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Restored record for " + key);
            }
            Value val = record.getValue();

            if (val != null) {
                return val.getInputStream();
            } else {
                return null;
            }
        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to restore record for " + key, de);
            }
            IOException failure = new IOException("Failed to restore record for " + key);
            failure.initCause(de);
            throw failure;
        }
    }

    /**
     * Remove a file
     *
     * @param dn directory name
     * @param fn file name
     * @throws IOException if an I/O error occurs
     */
    public synchronized void remove(String dn, String fn) throws IOException {

        try {
            if (fn == null) {
                return;
            }
            Key key = new Key(dn + "/" + fn);
            Record record = cacheDB.readRecord(key);
            long removePos = cacheDB.findValue(key);

            cacheDB.deleteRecord(key);
            if (record != null) {
                try {
                    if (calcExpiration(record) > 0) {
                        InputStream is = record.getValue().getInputStream();
                        XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, is);
                        Advertisement adv = AdvertisementFactory.newAdvertisement(asDoc);
                        Map<String, String> indexables = getIndexfields(adv.getIndexFields(), asDoc);

                        indexer.removeFromIndex(addKey(dn, indexables), removePos);
                        // add it to deltas to expire it in srdi
                        addDelta(dn, indexables, 0);
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("removed " + record);
                        }
                    }
                } catch (Exception e) {
                    // bad bits we are done
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "failed to remove " + dn + "/" + fn, e);
                    }
                }
            }
        } catch (DBException de) {
            // entry does not exist
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("failed to remove " + dn + "/" + fn);
            }
        }
    }

    /**
     * Restore a saved StructuredDocument.
     *
     * @param dn directory name
     * @param fn file name
     * @return StructuredDocument containing the file
     * @throws IOException if an I/O error occurs
     *                     was not possible.
     */
    public StructuredDocument restore(String dn, String fn) throws IOException {
        InputStream is = getInputStream(dn, fn);
        return StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, is);
    }

    /**
     * Restore an advetisement into a byte array.
     *
     * @param dn directory name
     * @param fn file name
     * @return byte [] containing the file
     * @throws IOException if an I/O error occurs
     */
    public synchronized byte[] restoreBytes(String dn, String fn) throws IOException {

        try {
            Key key = new Key(dn + "/" + fn);
            Record record = cacheDB.readRecord(key);

            if (record == null) {
                return null;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("restored " + record);
            }
            Value val = record.getValue();

            if (val != null) {
                return val.getData();
            } else {
                return null;
            }
        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "failed to restore " + dn + "/" + fn, de);
            }
            IOException failure = new IOException("failed to restore " + dn + "/" + fn);
            failure.initCause(de);
            throw failure;
        }
    }

    /**
     * Stores a StructuredDocument in specified dir, and file name
     *
     * @param dn  directory name
     * @param fn  file name
     * @param adv Advertisement to store
     * @throws IOException if an I/O error occurs
     */
    public void save(String dn, String fn, Advertisement adv) throws IOException {
        save(dn, fn, adv, DiscoveryService.INFINITE_LIFETIME, DiscoveryService.NO_EXPIRATION);
    }

    /**
     * Stores a StructuredDocument in specified dir, and file name, and
     * associated doc timeouts
     *
     * @param dn         directory name
     * @param fn         file name
     * @param adv        Advertisement to save
     * @param lifetime   Document (local) lifetime in relative ms
     * @param expiration Document (global) expiration time in relative ms
     * @throws IOException Thrown if there is a problem saving the document.
     */
    public synchronized void save(String dn, String fn, Advertisement adv, long lifetime, long expiration) throws IOException {

        try {
            if (expiration < 0 || lifetime <= 0) {
                throw new IllegalArgumentException("Bad expiration or lifetime.");
            }
            XMLDocument doc;

            try {
                doc = (XMLDocument) adv.getDocument(MimeMediaType.XMLUTF8);
            } catch (RuntimeException e) {
                IOException failure = new IOException("Advertisement couldn't be saved");
                failure.initCause(e);
                throw failure;
            }

            Key key = new Key(dn + "/" + fn);
            // save the new version
            ByteArrayOutputStream baos = new ByteArrayOutputStream();

            doc.sendToStream(baos);
            baos.close();
            Value value = new Value(baos.toByteArray());
            Long oldLife = null;
            Record record = cacheDB.readRecord(key);

            if (record != null) {
                // grab the old lifetime
                oldLife = (Long) record.getMetaData(Record.LIFETIME);
            }

            long absoluteLifetime = TimeUtils.toAbsoluteTimeMillis(lifetime);

            if (oldLife != null) {
                if (absoluteLifetime < oldLife) {
                    // make sure we don't override the original value
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine(MessageFormat.format("Overriding attempt to decrease adv lifetime from : {0} to :{1}",
                                                     new Date(oldLife), new Date(absoluteLifetime)));
                    }
                    absoluteLifetime = oldLife;
                }
            }
            // make sure expiration does not exceed lifetime
            if (expiration > lifetime) {
                expiration = lifetime;
            }
            long pos = cacheDB.writeRecord(key, value, absoluteLifetime, expiration);
            Map<String, String> indexables = getIndexfields(adv.getIndexFields(), doc);
            Map<String, String> keyedIdx = addKey(dn, indexables);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Indexing " + keyedIdx + " at " + pos);
            }
            indexer.addToIndex(keyedIdx, pos);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                // too noisy
                // LOG.debug("Wrote " + key + " = " + value);
                LOG.fine("Stored " + indexables + " at " + pos);
            }

            if (expiration > 0) {
                // Update for SRDI with our caches lifetime only if we are prepared to share the advertisement with others.
                addDelta(dn, indexables, TimeUtils.toRelativeTimeMillis(absoluteLifetime));
            }

        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, MessageFormat.format("Failed to write {0}/{1} {2} {3}", dn, fn, lifetime, expiration), de);
            }
            IOException failure = new IOException("Failed to write " + dn + "/" + fn + " " + lifetime + " " + expiration);
            failure.initCause(de);
            throw failure;
        }
    }

    /**
     * Store some bytes in specified dir, and file name, and
     * associated doc timeouts
     *
     * @param dn         directory name
     * @param fn         file name
     * @param data       byte array to save
     * @param lifetime   Document (local) lifetime in relative ms
     * @param expiration Document (global) expiration time in relative ms
     * @throws IOException Thrown if there is a problem saving the document.
     */
    public synchronized void save(String dn, String fn, byte[] data, long lifetime, long expiration) throws IOException {

        try {
            if (expiration < 0 || lifetime <= 0) {
                throw new IllegalArgumentException("Bad expiration or lifetime.");
            }

            Key key = new Key(dn + "/" + fn);
            Value value = new Value(data);
            Long oldLife = null;
            Record record = cacheDB.readRecord(key);

            if (record != null) {
                // grab the old lifetime
                oldLife = (Long) record.getMetaData(Record.LIFETIME);
            }

            // save the new version

            long absoluteLifetime = TimeUtils.toAbsoluteTimeMillis(lifetime);

            if (oldLife != null) {
                if (absoluteLifetime < oldLife) {
                    // make sure we don't override the original value
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine(MessageFormat.format("Overriding attempt to decrease adv lifetime from : {0} to :{1}",
                                                            new Date(oldLife), new Date(absoluteLifetime)));
                    }
                    absoluteLifetime = oldLife;
                }
            }

            // make sure expiration does not exceed lifetime
            if (expiration > lifetime) {
                expiration = lifetime;
            }

            cacheDB.writeRecord(key, value, absoluteLifetime, expiration);
        } catch (DBException de) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to write " + dn + "/" + fn + " " + lifetime + " " + expiration, de);
            }

            IOException failure = new IOException("Failed to write " + dn + "/" + fn + " " + lifetime + " " + expiration);
            failure.initCause(de);
            throw failure;
        }
    }

    private static Map<String, String> getIndexfields(String[] fields, StructuredDocument doc) {
        Map<String, String> map = new HashMap<String, String>();

        if (doc == null) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Null document");
            }
            return map;
        }
        if (fields == null) {
            return map;
        }
        for (String field : fields) {
            Enumeration en = doc.getChildren(field);
            while (en.hasMoreElements()) {
                String val = (String) ((Element) en.nextElement()).getValue();
                if (val != null) {
                    map.put(field, val);
                }
            }
        }
        return map;
    }

    /* adds a primary index 'dn' to indexables */
    private static Map<String, String> addKey(String dn, Map<String, String> map) {
        if (map == null) {
            return null;
        }

        Map<String, String> tmp = new HashMap<String, String>();
        if (map.size() > 0) {
            Iterator<String> it = map.keySet().iterator();

            while (it != null && it.hasNext()) {
                String name = it.next();

                tmp.put(dn + name, map.get(name));
            }
        }
        return tmp;
    }

    private static final class EntriesCallback implements BTreeCallback {

        private BTreeFiler cacheDB = null;
        private int threshold;
        private List<SrdiMessage.Entry> results;
        private String key;

        EntriesCallback(BTreeFiler cacheDB, List<SrdiMessage.Entry> results, String key, int threshold) {
            this.cacheDB = cacheDB;
            this.results = results;
            this.key = key;
            this.threshold = threshold;
        }

        /**
         * {@inheritDoc}
         */
        public boolean indexInfo(Value val, long pos) {
            if (results.size() >= threshold) {
                return false;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Found " + val.toString() + " at " + pos);
            }

            Record record;
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
            long exp = calcExpiration(record);

            if (exp <= 0) {
                // skip expired and private entries
                return true;
            }
            Long life = (Long) record.getMetaData(Record.LIFETIME);
            SrdiMessage.Entry entry = new SrdiMessage.Entry(key, val.toString(), life - System.currentTimeMillis());

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(" key [" + entry.key + "] value [" + entry.value + "] exp [" + entry.expiration + "]");
            }
            results.add(entry);
            return true;
        }
    }


    private final class SearchCallback implements BTreeCallback {

        private BTreeFiler cacheDB = null;
        private Indexer indexer = null;
        private int threshold;
        private List<InputStream> results;
        private List<Long> expirations;
        private boolean purge;

        SearchCallback(BTreeFiler cacheDB, Indexer indexer, List<InputStream> results, List<Long> expirations, int threshold) {
            this(cacheDB, indexer, results, expirations, threshold, false);
        }

        SearchCallback(BTreeFiler cacheDB, Indexer indexer, List<InputStream> results, List<Long> expirations, int threshold, boolean purge) {
            this.cacheDB = cacheDB;
            this.indexer = indexer;
            this.results = results;
            this.threshold = threshold;
            this.expirations = expirations;
            this.purge = purge;
        }

        /**
         * {@inheritDoc}
         */
        public boolean indexInfo(Value val, long pos) {
            if (results.size() >= threshold) {
                return false;
            }
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Found " + val.toString() + " at " + pos);
            }

            Record record;
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

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINEST)) {
                LOG.finest("Search callback record " + record.toString());
            }
            long exp = calcExpiration(record);
            if (exp < 0) {
                if (purge) {
                    try {
                        indexer.purge(pos);
                        cacheDB.deleteRecord(record.getKey());
                    } catch (DBException ex) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Exception while reading indexed", ex);
                        }
                    } catch (IOException ie) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Exception while reading indexed", ie);
                        }
                    }
                } else {
                    ++inconvenienceLevel;
                }
                return true;
            }

            if (expirations != null) {
                expirations.add(exp);
            }
            results.add(record.getValue().getInputStream());
            return true;
        }
    }

    protected static IndexQuery getIndexQuery(String value) {

        int operator;

        if (value == null) {
            return null;
        } else if (value.length() == 0 || "*".equals(value)) {
            return null;
        } else if (value.indexOf("*") < 0) {
            operator = IndexQuery.EQ;
        } else if (value.charAt(0) == '*' && value.charAt(value.length() - 1) != '*') {
            operator = IndexQuery.EW;
            value = value.substring(1, value.length());
        } else if (value.charAt(value.length() - 1) == '*' && value.charAt(0) != '*') {
            operator = IndexQuery.SW;
            value = value.substring(0, value.length() - 1);
        } else {
            operator = IndexQuery.BWX;
            value = value.substring(1, value.length() - 1);
        }
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Index query operator :" + operator);
        }
        return new IndexQuery(operator, new Value(value));
    }

    /**
     * Search and recovers documents that contains at least
     * a macthing pair of tag/value.
     *
     * @param dn          contains the name of the folder on which to
     *                    perform the search
     * @param value       contains the value to search on.
     * @param attribute   attribute to search on
     * @param threshold   threshold
     * @param expirations List to contain expirations
     * @return Enumeration containing of all the documents names
     */
    public synchronized List<InputStream> search(String dn, String attribute, String value, int threshold, List<Long> expirations) {
        List<InputStream> res = new ArrayList<InputStream>();
        IndexQuery iq = getIndexQuery(value);
        try {
            indexer.search(iq, dn + attribute, new SearchCallback(cacheDB, indexer, res, expirations, threshold));
        } catch (Exception ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Exception while searching in index", ex);
            }
        }
        return res;
    }

    /**
     * returns all entries that are cached
     *
     * @param dn          the relative dir name
     * @param clearDeltas if true clears the delta cache
     * @return SrdiMessage.Entries
     */
    public synchronized List<SrdiMessage.Entry> getEntries(String dn, boolean clearDeltas) {
        List<SrdiMessage.Entry> res = new ArrayList<SrdiMessage.Entry>();
        try {
            Map map = indexer.getIndexers();
            BTreeFiler listDB = indexer.getListDB();
            Iterator it = map.keySet().iterator();

            while (it != null && it.hasNext()) {
                String indexName = (String) it.next();
                // seperate the index name from attribute
                if (indexName.startsWith(dn)) {
                    String attr = indexName.substring((dn).length());
                    NameIndexer idxr = (NameIndexer) map.get(indexName);
                    idxr.query(null, new Indexer.SearchCallback(listDB, new EntriesCallback(cacheDB, res, attr, Integer.MAX_VALUE)));
                }
            }
        } catch (Exception ex) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Exception while searching in index", ex);
            }
        }

        if (clearDeltas) {
            clearDeltas(dn);
        }
        return res;
    }

    /**
     * returns all entries that are added since this method was last called
     *
     * @param dn the relative dir name
     * @return SrdiMessage.Entries
     */
    public synchronized List<SrdiMessage.Entry> getDeltas(String dn) {
        List<SrdiMessage.Entry> result = new ArrayList<SrdiMessage.Entry>();
        List<SrdiMessage.Entry> deltas = deltaMap.get(dn);

        if (deltas != null) {
            result.addAll(deltas);
            deltas.clear();
        }
        return result;
    }

    private synchronized void clearDeltas(String dn) {

        List<SrdiMessage.Entry> deltas = deltaMap.get(dn);

        if (deltas == null) {
            return;
        }
        deltas.clear();
    }

    private synchronized void addDelta(String dn, Map<String, String> indexables, long exp) {

        if (trackDeltas) {
            Iterator<Map.Entry<String, String>> eachIndex = indexables.entrySet().iterator();

            if (eachIndex.hasNext()) {
                List<SrdiMessage.Entry> deltas = deltaMap.get(dn);

                if (deltas == null) {
                    deltas = new ArrayList<SrdiMessage.Entry>();
                    deltaMap.put(dn, deltas);
                }
                while (eachIndex.hasNext()) {
                    Map.Entry<String, String> anEntry = eachIndex.next();
                    String attr = anEntry.getKey();
                    String value = anEntry.getValue();
                    SrdiMessage.Entry entry = new SrdiMessage.Entry(attr, value, exp);

                    deltas.add(entry);
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Added entry  :" + entry + " to deltas");
                    }
                }
            }
        }
    }

    public synchronized void setTrackDeltas(boolean trackDeltas) {
        this.trackDeltas = trackDeltas;
        if (!trackDeltas) {
            deltaMap.clear();
        }
    }

    /**
     * stop the cm
     */
    public synchronized void stop() {
        try {
            cacheDB.close();
            indexer.close();
            stop = true;
            notify();
        } catch (DBException ex) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Unable to close advertisments.tbl", ex);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public synchronized void run() {
        try {
            while (!stop) {
                try {
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("waiting " + gcMinInterval + "ms before garbage collection");
                    }
                    wait(gcMinInterval);
                } catch (InterruptedException woken) {
                    Thread.interrupted();

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.log(Level.FINE, "Thread interrupted", woken);
                    }
                }

                if (stop) {
                    // if asked to stop, exit
                    break;
                }

                if ((inconvenienceLevel > maxInconvenienceLevel) || (System.currentTimeMillis() > gcTime)) {

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Garbage collection started");
                    }
                    garbageCollect();
                    inconvenienceLevel = 0;
                    gcTime = System.currentTimeMillis() + gcMaxInterval;
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Garbage collection completed");
                    }
                }
            }
        } catch (Throwable all) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Uncaught Throwable in thread :" + Thread.currentThread().getName(), all);
            }
        } finally {
            gcThread = null;
        }
    }

    private synchronized void rebuildIndex() throws DBException, IOException {

        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("Rebuilding indices");
        }

        String pattern = "*";
        IndexQuery any = new IndexQuery(IndexQuery.ANY, pattern);

        cacheDB.query(any, new RebuildIndexCallback(cacheDB, indexer));
    }

    private static final class RebuildIndexCallback implements BTreeCallback {

        private BTreeFiler database = null;
        private Indexer index = null;

        RebuildIndexCallback(BTreeFiler database, Indexer index) {
            this.database = database;
            this.index = index;
        }

        /**
         * {@inheritDoc}
         */
        public boolean indexInfo(Value val, long pos) {
            try {
                Record record = database.readRecord(pos);

                if (record == null) {
                    return true;
                }

                InputStream is = record.getValue().getInputStream();
                XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, is);
                Advertisement adv = AdvertisementFactory.newAdvertisement(asDoc);
                Map<String, String> indexables = getIndexfields(adv.getIndexFields(), asDoc);

                String dn = getDirName(adv);
                Map<String, String> keyedIdx = addKey(dn, indexables);

                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Restoring index " + keyedIdx + " at " + pos);
                }
                index.addToIndex(keyedIdx, pos);
            } catch (Exception ex) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Exception rebuilding index  at " + pos, ex);
                }
                return true;
            }
            return true;
        }
    }
}
