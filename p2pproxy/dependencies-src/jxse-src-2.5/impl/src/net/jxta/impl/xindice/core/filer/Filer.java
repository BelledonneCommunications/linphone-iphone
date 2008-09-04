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
import net.jxta.impl.xindice.core.DBObject;
import net.jxta.impl.xindice.core.data.Key;
import net.jxta.impl.xindice.core.data.Record;
import net.jxta.impl.xindice.core.data.RecordSet;
import net.jxta.impl.xindice.core.data.Value;
import net.jxta.impl.xindice.util.Named;

/**
 * Filer is the low-level file management interface for Xindice.  A Filer object
 * is implemented in order to provide a data source to the Xindice Collection
 * class.  Filers are developed to perform transparent storage and retrieval to
 * and from heterogenous data sources (such as FTP, HTTP, RDBMS, etc...)
 */
public interface Filer extends Named, DBObject {

    /**
     * readRecord returns a Record from the Filer based on the specified
     * Key.
     *
     * @param key The Record's Key
     * @return The returned Record
     * @throws net.jxta.impl.xindice.core.DBException if a db exception occurs
     */
    Record readRecord(Key key) throws DBException;

    /**
     * readRecord returns a Record from the Filer at the specified
     * position. The Record's Key will be set to null.
     *
     * @param pos The Record's position
     * @return The returned Record
     * @throws net.jxta.impl.xindice.core.DBException if a db exception occurs
     */
    Record readRecord(long pos) throws DBException;

    /**
     * writeRecord writes a Value to the Filer based on the specified Key.
     *
     * @param key The Record's Key
     * @param value The Record's Value
     * @return 0 if the Record could not be written, the starting
     * offset of the Record otherwise (used for indexing)
     * @throws net.jxta.impl.xindice.core.DBException if a db exception occurs
     */
    long writeRecord(Key key, Value value) throws DBException;

    /**
     * deleteRecord removes a Record from the Filer based on the
     * specified Key.
     *
     * @param key The Record's Key
     * @return Whether or not the Record was deleted
     * @throws net.jxta.impl.xindice.core.DBException if a db exception occurs
     */
    boolean deleteRecord(Key key) throws DBException;

    /**
     * getRecordCount returns the number of Records in the Filer.
     *
     * @return The Record count
     * @throws net.jxta.impl.xindice.core.DBException if a db exception occurs
     */
    long getRecordCount() throws DBException;

    /**
     * getRecordSet returns a RecordSet object for the current Filer.
     *
     * @return The Filer Enumerator
     * @throws net.jxta.impl.xindice.core.DBException if a db exception occurs
     */
    RecordSet getRecordSet() throws DBException;
   
    /**
     * flush forcefully flushes any unwritten buffers to disk.
     * @throws net.jxta.impl.xindice.core.DBException if a db exception occurs
     */
    void flush() throws DBException;
}
