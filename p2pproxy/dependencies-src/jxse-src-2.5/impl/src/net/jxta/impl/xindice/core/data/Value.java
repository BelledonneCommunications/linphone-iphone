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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;

/**
 * Value is the primary base class for all data storing objects.
 * The content window of Value objects are immutable, but the
 * underlying byte array is not.
 *
 */
public class Value implements Comparable {

    protected byte[] data = null;
    protected int pos = 0;
    protected int len = -1;

    public Value(Value value) {
        this.data = value.data;
        this.pos = value.pos;
        this.len = value.len;
    }

    public Value(byte[] data) {
        this.data = data;
        this.len = data.length;
    }

    public Value(byte[] data, int pos, int len) {
        this.data = data;
        this.pos = pos;
        this.len = len;
    }

    public Value(String data) {
        try {
            this.data = data.getBytes("utf-8");
            this.len = this.data.length;
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("Java doesn't support UTF-8 encoding", e);
        }
    }

    /**
     * getData retrieves the data being stored by the Value as a byte array.
     *
     * @return The Data
     */
    public final byte[] getData() {
        if (len != data.length) {
            byte[] b = new byte[len];

            System.arraycopy(data, pos, b, 0, len);
            return b;
        } else {
            return data;
        }
    }

    /**
     * getLength retrieves the length of the data being stored by the Value.
     *
     * @return The Value length
     */
    public final int getLength() {
        return len;
    }

    /**
     * getInputStream returns an InputStream for the Value.
     *
     * @return An InputStream
     */
    public final InputStream getInputStream() {
        return new ByteArrayInputStream(data, pos, len);
    }

    /**
     * streamTo streams the content of the Value to an OutputStream.
     *
     * @param out the OutputStream
     * @throws java.io.IOException if an io error occurs
     */
    public final void streamTo(OutputStream out) throws IOException {
        out.write(data, pos, len);
    }

    public final void copyTo(byte[] tdata, int tpos) {
        System.arraycopy(data, pos, tdata, tpos, len);
    }

    public final void copyTo(byte[] tdata, int tpos, int len) {
        System.arraycopy(data, pos, tdata, tpos, len);
    }

    @Override
    public final String toString() {
        try {
            return new String(data, pos, len, "utf-8");
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("Java doesn't seem to support UTF-8!", e);
        }
    }

    @Override
    public int hashCode() {
        return toString().hashCode();
    }

    public boolean equals(Value value) {
        return len == value.len && compareTo(value) == 0;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj instanceof Value) {
            return equals((Value) obj);
        } else {
            return equals(new Value(obj.toString()));
        }
    }

    public final int compareTo(Value value) {
        byte[] ddata = value.data;
        int dpos = value.pos;
        int dlen = value.len;

        int stop = len > dlen ? dlen : len;

        for (int i = 0; i < stop; i++) {
            byte b1 = data[pos + i];
            byte b2 = ddata[dpos + i];

            if (b1 == b2) {
                // do nothing
            } else {
                short s1 = (short) (b1 >>> 0);
                short s2 = (short) (b2 >>> 0);

                return s1 > s2 ? (i + 1) : -(i + 1);
            }
        }

        if (len == dlen) {
            return 0;
        } else {
            return len > dlen ? stop + 1 : -(stop + 1);
        }
    }

    public final int compareTo(Object obj) {
        if (obj instanceof Value) {
            return compareTo((Value) obj);
        } else {
            return compareTo(new Value(obj.toString()));
        }
    }

    public final boolean startsWith(Value value) {
        if (len < value.len) {
            return false;
        }

        byte[] ddata = value.data;
        int dpos = value.pos;

        for (int i = 0; i < value.len; i++) {
            if (data[i + pos] != ddata[i + dpos]) {
                return false;
            }
        }

        return true;
    }

    public final boolean endsWith(Value value) {
        if (len < value.len) {
            return false;
        }

        byte[] ddata = value.data;
        int dpos = value.pos;

        int offset = len - value.len;

        for (int i = 0; i < value.len; i++) {
            if (data[i + pos + offset] != ddata[i + dpos]) {
                return false;
            }
        }

        return true;
    }

    public final boolean contains(Value value) {
        if (len < value.len) {
            return false;
        }
        boolean match;
        byte[] ddata = value.data;
        int dpos = value.pos;

        for (int offset = 0; offset <= len - value.len; offset++) {
            match = true;
            for (int i = 0; i < value.len; i++) {
                if (data[i + pos + offset] != ddata[i + dpos]) {
                    match = false;
                }
            }
            if (match) {
                return true;
            }
        }
        return false;
    }
}
