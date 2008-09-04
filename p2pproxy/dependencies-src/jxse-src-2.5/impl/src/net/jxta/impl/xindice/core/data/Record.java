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
package net.jxta.impl.xindice.core.data;

import java.util.Map;

/**
 * Record is a container for Key/Value pairs.  Record also provides
 * metadata for a Value stored in the database.
 */
public final class Record {
    public static final String CREATED = "created";
    public static final String MODIFIED = "modified";
    public static final String LIFETIME = "lifetime";
    public static final String EXPIRATION = "expiration";
    public static final String OWNER = "owner";
    public static final String GROUP = "group";
   
    private Key key = null;
    private Value value = null;
    private Map meta = null;

    public Record() {}

    public Record(Key key, Value value, Map meta) {
        this.key = key;
        this.value = value;
        this.meta = meta;
    }
   
    public Record(Key key, Value value) {
        this.key = key;
        this.value = value;
    }
   
    /**
     * setKey sets the Record's Key.
     *
     * @param key The new key
     */
    public void setKey(Key key) {
        this.key = key;
    }

    /**
     * getKey returns the Record's Key.
     *
     * @return The Record's Key
     */
    public Key getKey() {
        return key;
    }

    /**
     * setValue sets the Record's Value.
     *
     * @param value The new Value
     */
    public void setValue(Value value) {
        this.value = value;
    }

    /**
     * setValue sets the Record's Value.
     *
     * @param value The new Value
     */
    public void setValue(String value) {
        this.value = new Value(value);
    }

    /**
     * getValue returns the Record's Value.
     *
     * @return The Record's Value
     */
    public Value getValue() {
        return value;
    }
   
    /**
     * getMetaData returns metadata about the Value.
     *
     * @param name The property name
     * @return The metadata value
     */
    public Object getMetaData(Object name) {
        return meta != null ? meta.get(name) : null;
    }

    @Override
    public String toString() {
        return 
                (key != null ? key.toString() : "") + (value != null ? " = " + value.toString() : "")
                + (meta != null ? " meta " + meta.toString() : "");
    }
}
