/*
 * Copyright (c) 2003-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util.cm;


import net.jxta.impl.xindice.core.DBException;
import net.jxta.impl.xindice.core.data.Record;
import net.jxta.impl.xindice.core.data.Value;
import net.jxta.impl.xindice.core.filer.BTreeCallback;
import net.jxta.impl.xindice.core.filer.BTreeFiler;
import net.jxta.impl.xindice.core.indexer.IndexQuery;
import net.jxta.impl.xindice.core.indexer.NameIndexer;

import java.io.DataInputStream;
import java.io.File;
import java.io.IOException;


/**
 * A utility to dump the CM databases.
 */

public class DumpCm {

    private static final IndexQuery ANY = new IndexQuery(IndexQuery.ANY, "*");

    public interface DumpCmCallback {
        void println(String val);
    }

    public static void dump(String args[], DumpCmCallback callback) throws IOException {
        String type = null;
        String dir = null;
        String file = null;

        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-type") && i + 1 < args.length) {
                type = args[++i];
            } else if (args[i].equals("-dir") && i + 1 < args.length) {
                dir = args[++i];
            } else if (args[i].equals("-file") && i + 1 < args.length) {
                file = args[++i];
            } else {
                throw new IllegalArgumentException("Incorrect option");
            }
        }

        if (callback == null) {
            throw new IllegalArgumentException("No callback was provided.");
        }

        if (type == null || dir == null || file == null) {
            throw new IllegalArgumentException("Missing mandatory option");
        }

        if (type.equals("index")) {
            dumpIndex(dir, file, callback);
        } else if (type.equals("offsets")) {
            dumpOffsets(dir, file, callback);
        } else if (type.equals("db")) {
            dumpDatabase(dir, file, callback);
        } else {
            throw new IllegalArgumentException("Incorrect type");
        }
    }

    public static void dumpIndex(String dir, String file, DumpCmCallback callback) throws IOException {
        NameIndexer indexer = new NameIndexer();

        // remove the suffix if present (setLocation automatically adds it back)
        final String SUFFIX = ".idx";

        if (file.endsWith(SUFFIX)) {
            file = file.substring(0, file.length() - SUFFIX.length());
        }

        indexer.setLocation(dir, file);
        try {
            if (!indexer.open()) {
                throw new IOException("Failed to open index file " + dir + File.separator + file + SUFFIX);
            }
            callback.println("Index " + dir + File.separator + file + SUFFIX);
            indexer.query(ANY, new IndexCallback(callback));
        } catch (DBException dbe) {
            throw new IOException(dbe.getMessage());
        }
    }

    private static final class IndexCallback implements BTreeCallback {

        private DumpCmCallback callback = null;

        public IndexCallback(DumpCmCallback callback) {
            this.callback = callback;
        }

        public boolean indexInfo(Value val, long pos) {
            callback.println(pos + " \t " + val.toString());
            return true;
        }
    }

    public static void dumpOffsets(String dir, String file, DumpCmCallback callback) throws IOException {
        BTreeFiler filer = new BTreeFiler();

        // remove the suffix if present (setLocation automatically adds it back)
        final String SUFFIX = ".tbl";

        if (file.endsWith(SUFFIX)) {
            file = file.substring(0, file.length() - SUFFIX.length());
        }

        filer.setLocation(dir, file);
        try {
            if (!filer.open()) {
                throw new IOException("Failed to open offsets file " + dir + File.separator + file + SUFFIX);
            }
            callback.println("Offsets " + dir + File.separator + file + SUFFIX);
            filer.query(ANY, new OffsetsCallback(filer, callback));
        } catch (DBException dbe) {
            throw new IOException(dbe.getMessage());
        }
    }

    private static final class OffsetsCallback implements BTreeCallback {

        private BTreeFiler filer = null;
        private DumpCmCallback callback = null;

        public OffsetsCallback(BTreeFiler filer, DumpCmCallback callback) {
            this.filer = filer;
            this.callback = callback;
        }

        public boolean indexInfo(Value val, long pos) {
            Record record = null;

            try {
                record = filer.readRecord(pos);
            } catch (DBException dbe) {
                callback.println("Error reading record: " + dbe.getMessage());
            }

            StringBuilder offsets = new StringBuilder();
            DataInputStream dis = null;

            if (record != null) {
                dis = new DataInputStream(record.getValue().getInputStream());
            }
            try {
                int size = 0;

                if (dis != null) {
                    size = dis.readInt();
                }
                for (int i = 0; i < size; i++) {
                    if (dis != null) {
                        offsets.append(Long.toString(dis.readLong()));
                    }
                    offsets.append(" ");
                }
            } catch (IOException ie) {
                callback.println("Error reading record: " + ie.getMessage());
            }

            callback.println(pos + " \t " + val.toString() + "\n\t " + offsets.toString());
            return true;
        }
    }

    public static void dumpDatabase(String dir, String file, DumpCmCallback callback) throws IOException {
        BTreeFiler filer = new BTreeFiler();

        // remove the suffix if present (setLocation automatically adds it back)
        final String SUFFIX = ".tbl";

        if (file.endsWith(SUFFIX)) {
            file = file.substring(0, file.length() - SUFFIX.length());
        }

        filer.setLocation(dir, file);
        try {
            if (!filer.open()) {
                throw new IOException("Failed to open database file " + dir + File.separator + file + SUFFIX);
            }
            callback.println("Database " + dir + File.separator + file + SUFFIX);
            filer.query(ANY, new DatabaseCallback(filer, callback));
        } catch (DBException dbe) {
            throw new IOException(dbe.getMessage());
        }
    }

    private static final class DatabaseCallback implements BTreeCallback {

        private BTreeFiler filer = null;
        private DumpCmCallback callback = null;

        public DatabaseCallback(BTreeFiler filer, DumpCmCallback callback) {
            this.filer = filer;
            this.callback = callback;
        }

        public boolean indexInfo(Value val, long pos) {
            Record record = null;

            try {
                record = filer.readRecord(pos);
            } catch (DBException dbe) {
                callback.println("Error reading record: " + dbe.getMessage());
            }

            if (record != null) {
                callback.println(pos + " \t " + val.toString() + "\n" + record.getValue().toString());
            }
            return true;
        }
    }
}
