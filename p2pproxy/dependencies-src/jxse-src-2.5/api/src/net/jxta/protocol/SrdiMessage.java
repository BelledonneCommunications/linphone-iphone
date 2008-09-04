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
package net.jxta.protocol;


import net.jxta.document.Document;
import net.jxta.document.MimeMediaType;
import net.jxta.peer.PeerID;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;


/**
 * A generic message that can be used by services to push shared resources to
 * other peers on the network.
 * <p/>
 */
public abstract class SrdiMessage {

    /**
     * PERSIST Scope is persist only. i.e. A node receiving such message is expected to only persist
     * the indexes contained within.
     */
    public static final int PERSISTONLY = 0;

    /**
     * REPLICATE Scope is persist and replicate. i.e. A node receiving such message is expected
     * to pnly persist the indexes contained within.
     */
    public static final int REPLICATE = 1;
    private final Collection<Entry> entries = new ArrayList<Entry>();
    private PeerID peerid = null;
    private String primaryKey = null;

    /**
     * The scope of SrdiMesage  (0, or 1)
     */
    private int scope = 0;

    /**
     * Write advertisement into a document. asMimeType is a mime media-type
     * specification and provides the form of the document which is being
     * requested. Two standard document forms are defined. 'text/text' encodes
     * the document in a form nice for printing out and 'text/xml' which
     * provides an XML format.
     *
     * @param asMimeType mime-type requested representation for the returned
     *                   document
     * @return Document document representing the advertisement
     */
    public abstract Document getDocument(MimeMediaType asMimeType);

    /**
     * returns the source peerid
     *
     * @return PeerID
     */
    public PeerID getPeerID() {
        return peerid;
    }

    /**
     * get the primary key
     *
     * @return primaryKey
     */
    public String getPrimaryKey() {
        return this.primaryKey;
    }

    /**
     * Returns the current TTL value.
     *
     * @return The current TTL value.
     * @deprecated ttl is a misnomer, it is meant to be defined as scope. use #getScope() instead
     */
    @Deprecated
    public int getTTL() {
        return scope;
    }

    /**
     * sets the ttl
     *
     * @param newTTL The new TTL value.
     * @deprecated ttl is a misnomer, it is meant to be defined as scope. use #setScope() instead
     */
    @Deprecated
    public void setTTL(int newTTL) {
        scope = newTTL;
    }

    /**
     * Returns the current scope of this message.
     * <p/>
     * Scope is either 0 (persist only), or 1 (persist and replicate)
     *
     * @return The current scope value.
     */
    public int getScope() {
        return scope;
    }

    /**
     * Sets the current scope of this message.
     * <p/>
     * Scope is either 0 (persist only), or 1 (persist and replicate)
     * @see #PERSISTONLY
     * @see #REPLICATE
     *
     * @param scope The new TTL value.
     */
    public void setScope(int scope) {
        this.scope = scope;
    }

    /**
     * Decrements TTL
     */
    public void decrementTTL() {

        if (this.scope > 0) {
            this.scope--;
        }
    }

    /**
     * Returns the entries of this SrdiMessage
     *
     * @return the entries of this SrdiMessage
     */
    public Collection<Entry> getEntries() {
        return new ArrayList<Entry>(entries);
    }

    /**
     *  Adds an index entry
     * @param key  key
     * @param value the value
     * @param expiration expiration in millisconds
     */
    public void addEntry(String key, String value, long expiration) {
        addEntry(new SrdiMessage.Entry(key, value, expiration));
    }

    public void addEntry(SrdiMessage.Entry entry) {
        entries.add(entry);
    }

    /**
     * Set the Entries for this SrdiMessage.
     *
     * @param newEntries The entries to be included in this message.
     */
    public void setEntries(Collection<Entry> newEntries) {
        entries.clear();
        entries.addAll(newEntries);
    }

    /**
     * Set the source peerid
     *
     * @param peerid the source PeerID
     */
    public void setPeerID(PeerID peerid) {
        this.peerid = peerid;
    }

    /**
     * Set the primary key
     *
     * @param pkey the primary key
     */
    public void setPrimaryKey(String pkey) {
        this.primaryKey = pkey;
    }

    /**
     * All messages have a type (in xml this is !doctype) which identifies the
     * message, if no expiration is defined Default expiration infinite for
     * entries. This is the amount of time which an entry will live in cache.
     * After this time, the entry is garbage collected, it also worthy to note
     * that it is up to the peer caching these entries to honor the lifetime of
     * the entry, and peer may choose to garbage collect entries based on
     * quotas, relationship with other peers, etc. *
     *
     * @return String "jxta:GenSRDI"
     */
    public static String getMessageType() {
        return "jxta:GenSRDI";
    }

    /**
     * Entries object, which describes each entry described by this message
     */
    public final static class Entry {

        /**
         * Entry Expiration expressed in relative time in ms
         */
        public long expiration;

        /**
         * Entry key attribute
         */
        public String key;

        /**
         * Entry key value
         */
        public String value;

        /**
         * {@inheritDoc}
         * <p/>
         * Expiration time is not considered in calculation.
         */
        @Override
        public boolean equals(Object target) {

            if (this == target) {
                return true;
            }

            if (target instanceof Entry) {
                Entry likeMe = (Entry) target;

                return key.equals(likeMe.key) && value.equals(likeMe.value);
            }

            return false;
        }

        /**
         * {@inheritDoc}
         * <p/>
         * Expiration time is not considered in calculation.
         */
        @Override
        public int hashCode() {
            int result = 0;

            if (null != key) {
                result ^= key.hashCode();
            }

            if (null != value) {
                result ^= value.hashCode();
            }

            return result;
        }

        /**
         * Creates a Entry with key, value, and expiration
         *
         * @param key        key attribute
         * @param value      key value
         * @param expiration expressed in relative time in ms
         */
        public Entry(String key, String value, long expiration) {
            this.key = key;
            this.value = value;
            this.expiration = expiration;
        }
    }
}

