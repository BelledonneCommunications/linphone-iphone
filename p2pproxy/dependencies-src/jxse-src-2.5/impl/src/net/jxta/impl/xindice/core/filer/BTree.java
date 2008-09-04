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
import net.jxta.impl.xindice.core.data.Value;
import net.jxta.impl.xindice.core.indexer.IndexQuery;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.Map;
import java.util.WeakHashMap;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 * BTree represents a Variable Magnitude Simple-Prefix B+Tree File.
 * A BTree is a bit flexible in that it can be used for set or
 * map-based indexing.  HashFiler uses the BTree as a set for
 * producing RecordSet entries.  The Indexers use BTree as a map for
 * indexing entity and attribute values in Documents.
 * <br><br>
 * For those who don't know how a Simple-Prefix B+Tree works, the primary
 * distinction is that instead of promoting actual keys to branch pages,
 * when leaves are split, a shortest-possible separator is generated at
 * the pivot.  That separator is what is promoted to the parent branch
 * (and continuing up the list).  As a result, actual keys and pointers
 * can only be found at the leaf level.  This also affords the index the
 * ability to ignore costly merging and redistribution of pages when
 * deletions occur.  Deletions only affect leaf pages in this
 * implementation, and so it is entirely possible for a leaf page to be
 * completely empty after all of its keys have been removed.
 * <br><br>
 * Also, the Variable Magnitude attribute means that the btree attempts
 * to store as many values and pointers on one page as is possible.
 * <br><br>
 * This implementation supports the notion of nested roots.  This means
 * that you can create a btree where the pointers actually point to the
 * root of a separate btree being managed in the same file.
 */

public class BTree extends Paged {
    private final static Logger LOG = Logger.getLogger(BTree.class.getName());
    protected static final byte LEAF = 1;
    protected static final byte BRANCH = 2;
    protected static final byte STREAM = 3;

    /**
     * Cache of the recently used tree nodes.
     *
     * Cache contains weak references to the BTreeNode objects, keys are page numbers (Long objects).
     * Access synchronized by this map itself.
     */
    private final Map<Long, WeakReference<BTreeNode>> cache = new WeakHashMap<Long, WeakReference<BTreeNode>>();

    private BTreeFileHeader fileHeader;
    private BTreeRootInfo rootInfo;
    private BTreeNode rootNode;

    public BTree() {
        super();
        fileHeader = (BTreeFileHeader) getFileHeader();
    }

    public BTree(File file) {
        this();
        setFile(file);
    }

    /**
     * Setting this option forces all system buffers with the underlying device
     * if sync is set writes return after all modified data and attributes of the DB
     * have been written to the device.
     * by default sync is true.
     * {@link java.io.FileDescriptor}
     * @param sync if true, invokes FD.sync(), an expensive operation, required to ensure high consistency, especially
     * with system failures.
     */
    public void setSync(boolean sync) {
        this.sync = sync;
    }

    @Override
    public boolean open() throws DBException {
        if (super.open()) {
            long p = fileHeader.getRootPage();

            rootInfo = new BTreeRootInfo(p);
            rootNode = getBTreeNode(p, null);
            return true;
        } else {
            return false;
        }
    }

    @Override
    public boolean create() throws DBException {
        if (super.create()) {
            try {
                // Don't call this.open() as it will try to read rootNode from the disk
                super.open();

                long p = fileHeader.getRootPage();

                rootInfo = new BTreeRootInfo(p);

                // Initialize root node
                rootNode = new BTreeNode(getPage(p), null, new Value[0], new long[0]);
                rootNode.ph.setStatus(LEAF);
                rootNode.write();
                synchronized (cache) {
                    cache.put(rootNode.page.getPageNum(), new WeakReference<BTreeNode>(rootNode));
                }
                close();
                return true;
            } catch (Exception e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to create BTree, return false", e);
                }
            }
        }
        return false;
    }

    /**
     * addValue adds a Value to the BTree and associates a pointer with
     * it.  The pointer can be used for referencing any type of data, it
     * just so happens that Xindice uses it for referencing pages of
     * associated data in the BTree file or other files.
     *
     * @param value The Value to add
     * @param pointer The pointer to associate with it
     * @return The previous value for the pointer (or -1)
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public long addValue(Value value, long pointer) throws IOException, BTreeException {
        return getRootNode().addValue(value, pointer);
    }

    /**
     * addValue adds a Value to the BTree and associates a pointer with
     * it.  The pointer can be used for referencing any type of data, it
     * just so happens that Xindice uses it for referencing pages of
     * associated data in the BTree file or other files.
     *
     * @param root The BTree's root information (for nested trees)
     * @param value The Value to add
     * @param pointer The pointer to associate with it
     * @return The previous value for the pointer (or -1)
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public long addValue(BTreeRootInfo root, Value value, long pointer) throws IOException, BTreeException {
        return getRootNode(root).addValue(value, pointer);
    }

    /**
     * removeValue removes a Value from the BTree and returns the
     * associated pointer for it.
     *
     * @param value The Value to remove
     * @return The pointer that was associated with it
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public long removeValue(Value value) throws IOException, BTreeException {
        return getRootNode().removeValue(value);
    }

    /**
     * removeValue removes a Value from the BTree and returns the
     * associated pointer for it.
     *
     * @param root The BTree's root information (for nested trees)
     * @param value The Value to remove
     * @return The pointer that was associated with it
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public long removeValue(BTreeRootInfo root, Value value) throws IOException, BTreeException {
        return getRootNode(root).removeValue(value);
    }

    /**
     * findValue finds a Value in the BTree and returns the associated
     * pointer for it.
     *
     * @param value The Value to find
     * @return The pointer that was associated with it
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public long findValue(Value value) throws IOException, BTreeException {
        return getRootNode().findValue(value);
    }

    /**
     * findValue finds a Value in the BTree and returns the associated
     * pointer for it.
     *
     * @param root The BTree's root information (for nested trees)
     * @param value The Value to find
     * @return The pointer that was associated with it
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public long findValue(BTreeRootInfo root, Value value) throws IOException, BTreeException {
        return getRootNode(root).findValue(value);
    }

    /**
     * query performs a query against the BTree and performs callback
     * operations to report the search results.
     *
     * @param query The IndexQuery to use (or null for everything)
     * @param callback The callback instance
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public void query(IndexQuery query, BTreeCallback callback) throws IOException, BTreeException {
        getRootNode().query(query, callback);
    }

    /**
     * query performs a query against the BTree and performs callback
     * operations to report the search results.
     *
     * @param root The BTree's root information (for nested trees)
     * @param query The IndexQuery to use (or null for everything)
     * @param callback The callback instance
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    public void query(BTreeRootInfo root, IndexQuery query, BTreeCallback callback) throws IOException, BTreeException {
        getRootNode(root).query(query, callback);
    }

    /**
     * createBTreeRoot creates a new BTree root node in the BTree file
     * based on the provided value for the main tree.
     *
     * @param v The sub-tree Value to create
     * @return The new BTreeRootInfo instance
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    protected final BTreeRootInfo createBTreeRoot(Value v) throws IOException, BTreeException {
        BTreeNode n = createBTreeNode(BTree.LEAF, null);
        n.write();
        long position = n.page.getPageNum();

        addValue(v, position);
        return new BTreeRootInfo(v, position);
    }

    /**
     * createBTreeRoot creates a new BTree root node in the BTree file
     * based on the provided root information, and value for the tree.
     *
     * @param root The BTreeRootInfo to build off of
     * @param v The sub-tree Value to create
     * @return The new BTreeRootInfo instance
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    protected final BTreeRootInfo createBTreeRoot(BTreeRootInfo root, Value v) throws IOException, BTreeException {
        BTreeNode n = createBTreeNode(BTree.LEAF, null);

        n.write();

        long position = n.page.getPageNum();

        addValue(v, position);
        return new BTreeRootInfo(root, v, position);
    }

    /**
     * findBTreeRoot searches for a BTreeRoot in the file and returns
     * the BTreeRootInfo for the specified value based on the main tree.
     *
     * @param v The sub-tree Value to search for
     * @return The new BTreeRootInfo instance
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    protected final BTreeRootInfo findBTreeRoot(Value v) throws IOException, BTreeException {
        long position = findValue(v);

        return new BTreeRootInfo(v, position);
    }

    /**
     * findBTreeRoot searches for a BTreeRoot in the file and returns
     * the BTreeRootInfo for the specified value based on the provided
     * BTreeRootInfo value.
     *
     * @param root The BTreeRootInfo to search from
     * @param v The sub-tree Value to search for
     * @return The new BTreeRootInfo instance
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    protected final BTreeRootInfo findBTreeRoot(BTreeRootInfo root, Value v) throws IOException, BTreeException {
        long position = findValue(root, v);

        return new BTreeRootInfo(root, v, position);
    }

    /**
     * setRootNode resets the root for the specified root object to the
     * provided BTreeNode's page number.
     *
     * This method is not thread safe.
     *
     * @param root The root to reset
     * @param newRoot the new root node to use
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    protected final void setRootNode(BTreeRootInfo root, BTreeNode newRoot) throws IOException, BTreeException {
        BTreeRootInfo parent = root.getParent();

        if (parent == null) {
            rootNode = newRoot;
            long p = rootNode.page.getPageNum();

            rootInfo.setPage(p);
            fileHeader.setRootPage(p);
        } else {
            long p = newRoot.page.getPageNum();

            root.setPage(p);
            addValue(parent, root.name, p);
        }
    }

    /**
     * setRootNode resets the file's root to the provided
     * BTreeNode's page number.
     *
     * This method is not thread safe.
     *
     * @param rootNode the new root node to use
     * @throws java.io.IOException if an io error occurs
     * @throws BTreeException if a DB exception occurs
     */
    protected final void setRootNode(BTreeNode rootNode) throws IOException, BTreeException {
        setRootNode(rootInfo, rootNode);
    }

    /**
     * getRootNode retreives the BTree node for the specified
     * root object.
     *
     * @param root The root object to retrieve with
     * @return The root node
     */
    protected final BTreeNode getRootNode(BTreeRootInfo root) {
        if (root.page == rootInfo.page) {
            return rootNode;
        } else {
            return getBTreeNode(root.getPage(), null);
        }
    }

    /**
     * getRootNode retreives the BTree node for the file's root.
     *
     * @return The root node
     */
    protected final BTreeNode getRootNode() {
        return rootNode;
    }

    private BTreeNode getBTreeNode(long page, BTreeNode parent) {
        try {
            BTreeNode node = null;

            synchronized (cache) {
                WeakReference<BTreeNode> ref = cache.get(page);

                if (ref != null) {
                    node = ref.get();
                }

                if (node == null) {
                    node = new BTreeNode(getPage(page), parent);
                } else {
                    node.parent = parent;
                }

                cache.put(node.page.getPageNum(), new WeakReference<BTreeNode>(node));
            }

            node.read();
            return node;
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Ignored exception", e);
            }
            return null;
        }
    }

    private BTreeNode createBTreeNode(byte status, BTreeNode parent) throws IOException {
        Page p = getFreePage();
        BTreeNode node = new BTreeNode(p, parent, new Value[0], new long[0]);

        node.ph.setStatus(status);
        synchronized (cache) {
            cache.put(p.getPageNum(), new WeakReference<BTreeNode>(node));
        }
        return node;
    }

    /**
     * BTreeRootInfo
     */
    public final class BTreeRootInfo {
        private final BTreeRootInfo parent;
        private final Value name;
        private long page;

        public BTreeRootInfo(BTreeRootInfo parent, String name, long page) {
            this.parent = parent;
            this.name = new Value(name);
            this.page = page;
        }

        public BTreeRootInfo(BTreeRootInfo parent, Value name, long page) {
            this.parent = parent;
            this.name = name;
            this.page = page;
        }

        public BTreeRootInfo(String name, long page) {
            this.parent = rootInfo;
            this.name = new Value(name);
            this.page = page;
        }

        public BTreeRootInfo(Value name, long page) {
            this.parent = rootInfo;
            this.name = name;
            this.page = page;
        }

        private BTreeRootInfo(long page) {
            parent = null;
            name = null;
            this.page = page;
        }

        public BTreeRootInfo getParent() {
            return parent;
        }

        public Value getName() {
            return name;
        }

        public synchronized long getPage() {
            return page;
        }

        public synchronized void setPage(long page) {
            this.page = page;
        }
    }


    /**
     * BTreeNode
     */
    private final class BTreeNode {
        private final Page page;
        private final BTreePageHeader ph;
        private Value[] values;
        private long[] ptrs;
        private BTreeNode parent;
        private boolean loaded;

        public BTreeNode(Page page) {
            this(page, null);
        }

        public BTreeNode(Page page, BTreeNode parent) {
            this.page = page;
            this.parent = parent;
            this.ph = (BTreePageHeader) page.getPageHeader();
        }

        public BTreeNode(Page page, BTreeNode parent, Value[] values, long[] ptrs) {
            this(page, parent);
            set(values, ptrs);
            this.loaded = true;
        }

        /**
         * Sets values and pointers.
         * Internal (to the BTreeNode) method, not synchronized.
         * @param ptrs  pointers
         * @param values their values
         */
        private void set(Value[] values, long[] ptrs) {
            this.values = values;
            this.ph.setValueCount((short) values.length);
            this.ptrs = ptrs;
        }

        /**
         * Reads node only if it is not loaded yet
         * @throws java.io.IOException if an io error occurs
         */
        public synchronized void read() throws IOException {
            if (!this.loaded) {
                Value v = readValue(page);
                DataInputStream is = new DataInputStream(v.getInputStream());

                // Read in the Values
                values = new Value[ph.getValueCount()];
                for (int i = 0; i < values.length; i++) {
                    short valSize = is.readShort();
                    byte[] b = new byte[valSize];

                    is.read(b);
                    values[i] = new Value(b);
                }

                // Read in the pointers
                ptrs = new long[ph.getPointerCount()];
                for (int i = 0; i < ptrs.length; i++) {
                    ptrs[i] = is.readLong();
                }

                this.loaded = true;
            }
        }

        public synchronized void write() throws IOException {
            ByteArrayOutputStream bos = new ByteArrayOutputStream(fileHeader.getWorkSize());
            DataOutputStream os = new DataOutputStream(bos);

            // Write out the Values
            for (Value value : values) {
                os.writeShort(value.getLength());
                value.streamTo(os);
            }

            // Write out the pointers
            for (long ptr : ptrs) {
                os.writeLong(ptr);
            }

            writeValue(page, new Value(bos.toByteArray()));
        }

        /**
         * Internal (to the BTreeNode) method.
         * Because this method is called only by BTreeNode itself, no synchronization done inside of this method.
         * @param idx  the index
         * @return the BTree node
         */
        private BTreeNode getChildNode(int idx) {
            if (ph.getStatus() == BRANCH && idx >= 0 && idx < ptrs.length) {
                return getBTreeNode(ptrs[idx], this);
            } else {
                return null;
            }
        }

        /* Not used
         private synchronized void getChildStream(int idx, Streamable stream) throws IOException {
         if (ph.getStatus() == LEAF && idx >= 0 && idx < ptrs.length) {
         Value v = readValue(ptrs[idx]);
         DataInputStream dis = new DataInputStream(v.getInputStream());
         stream.read(dis);
         }
         }
         */

        public synchronized long removeValue(Value value) throws IOException, BTreeException {
            int idx = Arrays.binarySearch(values, value);

            switch (ph.getStatus()) {
            case BRANCH:
                idx = idx < 0 ? -(idx + 1) : idx + 1;
                return getChildNode(idx).removeValue(value);

            case LEAF:
                if (idx < 0) {
                    throw new BTreeNotFoundException("Value '" + value + "' doesn't exist");
                } else {
                    long oldPtr = ptrs[idx];

                    set(deleteArrayValue(values, idx), deleteArrayLong(ptrs, idx));

                    write();
                    return oldPtr;
                }

            default:
                throw new BTreeCorruptException("Invalid page type '" + ph.getStatus() + "' in removeValue");
            }
        }

        public synchronized long addValue(Value value, long pointer) throws IOException, BTreeException {
            if (value == null) {
                throw new BTreeException(FaultCodes.DBE_CANNOT_CREATE, "Can't add a null Value");
            }

            int idx = Arrays.binarySearch(values, value);

            switch (ph.getStatus()) {
            case BRANCH:
                idx = idx < 0 ? -(idx + 1) : idx + 1;
                return getChildNode(idx).addValue(value, pointer);

            case LEAF:
                if (idx >= 0) {
                    // Value was found... Overwrite
                    long oldPtr = ptrs[idx];

                    ptrs[idx] = pointer;

                    set(values, ptrs);

                    write();
                    return oldPtr;
                } else {
                    // Value was not found
                    idx = -(idx + 1);

                    // Check to see if we've exhausted the block
                    boolean split = needSplit(value);

                    set(insertArrayValue(values, value, idx), insertArrayLong(ptrs, pointer, idx));

                    if (split) {
                        split();
                    } else {
                        write();
                    }
                }
                return -1;

            default:
                throw new BTreeCorruptException("Invalid Page Type In addValue");
            }
        }

        private synchronized void promoteValue(Value value, long rightPointer) throws IOException, BTreeException {
            // Check to see if we've exhausted the block
            boolean split = needSplit(value);

            int idx = Arrays.binarySearch(values, value);

            idx = idx < 0 ? -(idx + 1) : idx + 1;

            set(insertArrayValue(values, value, idx), insertArrayLong(ptrs, rightPointer, idx + 1));

            if (split) {
                split();
            } else {
                write();
            }
        }

        private Value getSeparator(Value value1, Value value2) {
            int idx = value1.compareTo(value2);
            byte[] b = new byte[Math.abs(idx)];

            value2.copyTo(b, 0, b.length);
            return new Value(b);
        }

        /**
         * Do we need to split this node after adding one more value?
         * @param value the value to split
         * @return true if successful
         */
        private boolean needSplit(Value value) {
            // Do NOT split if just 4 key/values are in the node.
            return this.values.length > 4 && // CurrLength + one Long pointer + value length + one short
                    this.ph.getDataLen() + 8 + value.getLength() + 2 > BTree.this.fileHeader.getWorkSize();
        }

        /**
         * Internal to the BTreeNode method
         * @throws java.io.IOException if an io error occurs
         * @throws BTreeException if a DB exception occurs
         */
        private void split() throws IOException, BTreeException {
            Value[] leftVals;
            Value[] rightVals;
            long[] leftPtrs;
            long[] rightPtrs;
            Value separator;

            short vc = ph.getValueCount();
            int pivot = vc / 2;

            // Split the node into two nodes
            switch (ph.getStatus()) {
            case BRANCH:
                leftVals = new Value[pivot];
                leftPtrs = new long[leftVals.length + 1];
                rightVals = new Value[vc - (pivot + 1)];
                rightPtrs = new long[rightVals.length + 1];

                System.arraycopy(values, 0, leftVals, 0, leftVals.length);
                System.arraycopy(ptrs, 0, leftPtrs, 0, leftPtrs.length);
                System.arraycopy(values, leftVals.length + 1, rightVals, 0, rightVals.length);
                System.arraycopy(ptrs, leftPtrs.length, rightPtrs, 0, rightPtrs.length);

                separator = values[leftVals.length];
                break;

            case LEAF:
                leftVals = new Value[pivot];
                leftPtrs = new long[leftVals.length];
                rightVals = new Value[vc - pivot];
                rightPtrs = new long[rightVals.length];

                System.arraycopy(values, 0, leftVals, 0, leftVals.length);
                System.arraycopy(ptrs, 0, leftPtrs, 0, leftPtrs.length);
                System.arraycopy(values, leftVals.length, rightVals, 0, rightVals.length);
                System.arraycopy(ptrs, leftPtrs.length, rightPtrs, 0, rightPtrs.length);

                separator = getSeparator(leftVals[leftVals.length - 1], rightVals[0]);
                break;

            default:
                throw new BTreeCorruptException("Invalid Page Type In split");
            }

            // Promote the pivot to the parent branch
            if (parent == null) {
                // This can only happen if this is the root
                BTreeNode rNode = createBTreeNode(ph.getStatus(), this);

                rNode.set(rightVals, rightPtrs);

                BTreeNode lNode = createBTreeNode(ph.getStatus(), this);

                lNode.set(leftVals, leftPtrs);

                ph.setStatus(BRANCH);
                set(new Value[] {
                    separator
                }, new long[] {lNode.page.getPageNum(), rNode.page.getPageNum()});

                write();
                rNode.write();
                lNode.write();
            } else {
                set(leftVals, leftPtrs);

                BTreeNode rNode = createBTreeNode(ph.getStatus(), parent);

                rNode.set(rightVals, rightPtrs);

                write();
                rNode.write();
                parent.promoteValue(separator, rNode.page.getPageNum());
            }
        }

        // ///////////////////////////////////////////////////////////////

        public synchronized long findValue(Value value) throws IOException, BTreeException {
            if (value == null) {
                throw new BTreeNotFoundException("Can't search on null Value");
            }

            int idx = Arrays.binarySearch(values, value);

            switch (ph.getStatus()) {
            case BRANCH:
                idx = idx < 0 ? -(idx + 1) : idx + 1;
                return getChildNode(idx).findValue(value);

            case LEAF:
                if (idx < 0) {
                    throw new BTreeNotFoundException("Value '" + value + "' doesn't exist");
                } else {
                    return ptrs[idx];
                }

            default:
                throw new BTreeCorruptException("Invalid page type '" + ph.getStatus() + "' in findValue");
            }
        }

        // query is a BEAST of a method
        public synchronized void query(IndexQuery query, BTreeCallback callback) throws IOException, BTreeException {
            if (query != null && query.getOperator() != IndexQuery.ANY) {
                Value[] qvals = query.getValues();
                int leftIdx = Arrays.binarySearch(values, qvals[0]);
                int rightIdx = qvals.length > 1 ? Arrays.binarySearch(values, qvals[qvals.length - 1]) : leftIdx;

                switch (ph.getStatus()) {
                case BRANCH:
                    leftIdx = leftIdx < 0 ? -(leftIdx + 1) : leftIdx + 1;
                    rightIdx = rightIdx < 0 ? -(rightIdx + 1) : rightIdx + 1;

                    switch (query.getOperator()) {
                    case IndexQuery.BWX:
                    case IndexQuery.BW:
                    case IndexQuery.IN:
                    case IndexQuery.SW:
                        // TODO: Can leftIdx be less than 0 here?
                        if (leftIdx < 0) {
                            leftIdx = 0;
                        }
                        if (rightIdx > ptrs.length - 1) {
                            rightIdx = ptrs.length - 1;
                        }
                        for (int i = leftIdx; i <= rightIdx; i++) {
                            getChildNode(i).query(query, callback);
                        }
                        break;

                    case IndexQuery.NBWX:
                    case IndexQuery.NBW:
                    case IndexQuery.NIN:
                    case IndexQuery.NSW:
                        if (leftIdx > ptrs.length - 1) {
                            leftIdx = ptrs.length - 1;
                        }
                        for (int i = 0; i <= leftIdx; i++) {
                            getChildNode(i).query(query, callback);
                        }
                        if (rightIdx < 0) {
                            rightIdx = 0;
                        }
                        for (int i = rightIdx; i < ptrs.length; i++) {
                            getChildNode(i).query(query, callback);
                        }
                        break;

                    case IndexQuery.EQ:
                        getChildNode(leftIdx).query(query, callback);
                        break;

                    case IndexQuery.LT:
                    case IndexQuery.LEQ:
                        if (leftIdx > ptrs.length - 1) {
                            leftIdx = ptrs.length - 1;
                        }
                        for (int i = 0; i <= leftIdx; i++) {
                            getChildNode(i).query(query, callback);
                        }
                        break;

                    case IndexQuery.GT:
                    case IndexQuery.GEQ:
                        if (rightIdx < 0) {
                            rightIdx = 0;
                        }
                        for (int i = rightIdx; i < ptrs.length; i++) {
                            getChildNode(i).query(query, callback);
                        }
                        break;

                    case IndexQuery.NEQ:
                    default:
                        for (int i = 0; i < ptrs.length; i++) {
                            getChildNode(i).query(query, callback);
                        }
                        break;
                    }
                    break;

                case LEAF:
                    switch (query.getOperator()) {
                    case IndexQuery.EQ:
                        if (leftIdx >= 0) {
                            callback.indexInfo(values[leftIdx], ptrs[leftIdx]);
                        }
                        break;

                    case IndexQuery.NEQ:
                        for (int i = 0; i < ptrs.length; i++) {
                            if (i != leftIdx) {
                                callback.indexInfo(values[i], ptrs[i]);
                            }
                        }
                        break;

                    case IndexQuery.BWX:
                    case IndexQuery.BW:
                    case IndexQuery.SW:
                    case IndexQuery.IN:
                        if (leftIdx < 0) {
                            leftIdx = -(leftIdx + 1);
                        }
                        if (rightIdx < 0) {
                            rightIdx = -(rightIdx + 1);
                        }
                        for (int i = 0; i < ptrs.length; i++) { // FIXME: VG: Optimize this loop
                            if (i >= leftIdx && i <= rightIdx && query.testValue(values[i])) {
                                callback.indexInfo(values[i], ptrs[i]);
                            }
                        }
                        break;

                    case IndexQuery.NBWX:
                    case IndexQuery.NBW:
                    case IndexQuery.NSW:
                        if (leftIdx < 0) {
                            leftIdx = -(leftIdx + 1);
                        }
                        if (rightIdx < 0) {
                            rightIdx = -(rightIdx + 1);
                        }
                        for (int i = 0; i < ptrs.length; i++) {
                            if ((i <= leftIdx || i >= rightIdx) && query.testValue(values[i])) {
                                callback.indexInfo(values[i], ptrs[i]);
                            }
                        }
                        break;

                    case IndexQuery.LT:
                    case IndexQuery.LEQ:
                        if (leftIdx < 0) {
                            leftIdx = -(leftIdx + 1);
                        }
                        for (int i = 0; i < ptrs.length; i++) {
                            if (i <= leftIdx && query.testValue(values[i])) {
                                callback.indexInfo(values[i], ptrs[i]);
                            }
                        }
                        break;

                    case IndexQuery.GT:
                    case IndexQuery.GEQ:
                        if (rightIdx < 0) {
                            rightIdx = -(rightIdx + 1);
                        }

                        for (int i = 0; i < ptrs.length; i++) {
                            if (i >= rightIdx && query.testValue(values[i])) {
                                callback.indexInfo(values[i], ptrs[i]);
                            }
                        }
                        break;

                    case IndexQuery.NIN:
                    default:
                        for (int i = 0; i < ptrs.length; i++) {
                            if (query.testValue(values[i])) {
                                callback.indexInfo(values[i], ptrs[i]);
                            }
                        }
                        break;
                    }
                    break;

                default:
                    throw new BTreeCorruptException("Invalid Page Type In query");
                }

            } else {
                // No Query - Just Walk The Tree
                switch (ph.getStatus()) {
                case BRANCH:
                    for (int i = 0; i < ptrs.length; i++) {
                        getChildNode(i).query(query, callback);
                    }
                    break;

                case LEAF:
                    for (int i = 0; i < values.length; i++) {
                        callback.indexInfo(values[i], ptrs[i]);
                    }
                    break;

                default:
                    throw new BTreeCorruptException("Invalid Page Type In query");
                }
            }
        }
    }

    // //////////////////////////////////////////////////////////////////

    @Override
    public FileHeader createFileHeader() {
        BTreeFileHeader header = new BTreeFileHeader();

        header.setPageCount(1);
        header.setTotalCount(1);
        return header;
    }

    @Override
    public FileHeader createFileHeader(boolean read) throws IOException {
        return new BTreeFileHeader(read);
    }

    @Override
    public FileHeader createFileHeader(long pageCount) {
        return new BTreeFileHeader(pageCount);
    }

    @Override
    public FileHeader createFileHeader(long pageCount, int pageSize) {
        return new BTreeFileHeader(pageCount, pageSize);
    }

    @Override
    public PageHeader createPageHeader() {
        return new BTreePageHeader();
    }

    /**
     * BTreeFileHeader
     */

    protected class BTreeFileHeader extends FileHeader {
        private long rootPage = 0;

        public BTreeFileHeader() {}

        public BTreeFileHeader(long pageCount) {
            super(pageCount);
        }

        public BTreeFileHeader(long pageCount, int pageSize) {
            super(pageCount, pageSize);
        }

        public BTreeFileHeader(boolean read) throws IOException {
            super(read);
        }

        @Override
        public synchronized void read(RandomAccessFile raf) throws IOException {
            super.read(raf);
            rootPage = raf.readLong();
        }

        @Override
        public synchronized void write(RandomAccessFile raf) throws IOException {
            super.write(raf);
            raf.writeLong(rootPage);
        }

        /**
         *  The root page of the storage tree
         * @param rootPage the new root page
         */
        public synchronized final void setRootPage(long rootPage) {
            this.rootPage = rootPage;
            setDirty();
        }

        /**
         * The root page of the storage tree
         * @return the root page
         */
        public synchronized final long getRootPage() {
            return rootPage;
        }
    }


    /**
     * BTreePageHeader
     */

    protected class BTreePageHeader extends PageHeader {
        private short valueCount = 0;

        public BTreePageHeader() {}

        public BTreePageHeader(DataInputStream dis) throws IOException {
            super(dis);
        }

        @Override
        public synchronized void read(DataInputStream dis) throws IOException {
            super.read(dis);
            if (getStatus() == UNUSED) {
                return;
            }

            valueCount = dis.readShort();
        }

        @Override
        public synchronized void write(DataOutputStream dos) throws IOException {
            super.write(dos);
            dos.writeShort(valueCount);
        }

        /** The number of values stored by this page
         * @param valueCount the value count
         */
        public synchronized final void setValueCount(short valueCount) {
            this.valueCount = valueCount;
            setDirty();
        }

        /**
         * The number of values stored by this page
         * @return the value count
         */
        public synchronized final short getValueCount() {
            return valueCount;
        }

        /**
         * The number of pointers stored by this page
         * @return the pointer count
         */
        public synchronized final short getPointerCount() {
            if (getStatus() == BRANCH) {
                return (short) (valueCount + 1);
            } else {
                return valueCount;
            }
        }
    }
}
