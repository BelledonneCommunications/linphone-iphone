/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util;


import java.util.HashMap;
import java.util.Map;


/**
 * A Cache which is similar to {@link java.util.LinkedHashMap}
 *
 * <p/>LinkedList cannot be used efficiently because it
 * cannot remove an element efficiently from the middle. For that, we need
 * the externally referenced element (the thing to be removed) to
 * be the list entry itself, rather than referenced by an invisible
 * list entry. That is why we use the DLink/Dlist family.
 */
public class Cache {
    
    /**
     * CacheEntryImpl objects are both part of a doubly linked list and
     * inserted in a HashMap. They refer to the thing mapped which is what
     * users of this class want to get, and to the key. The reason is
     * that we need the key to remove from the map
     * an entry that we found in list. The otherway around is made easy by
     * the nature of the dlinked structure.
     **/
    
    class CacheEntryImpl extends Dlink implements CacheEntry {
        
        private final Object value;
        private final Object key;
        
        // The application interface.
        public CacheEntryImpl(Object k, Object v) {
            key = k;
            value = v;
        }
        
        /**
         *  {@inheritDoc}
         **/
        public Object getKey() {
            return key;
        }
        
        /**
         *  {@inheritDoc}
         **/
        public Object getValue() {
            return value;
        }
    }
    
    private final long maxSize;
    private long size;
    private final Map map = new HashMap();
    private final Dlist lru = new Dlist();
    
    private final CacheEntryListener listener;
    
    /**
     * Creates a cache whih will keep at most maxSize purgeable entries.
     * Every new entry is purgeable by default.
     *
     * <p/>Entries that are not purgeable are not counted and are never removed
     * unless clear() or remove() is called. Purgeable entries are removed
     * silently as needed to make room for new entries so that the number
     * of purgeable entries remains < maxSize.
     *
     * <p/>Entries prugeability is controlled by invoking the sticky() method
     * or the stickyCacheEntry() method.
     *
     * <p/>For now, purged entries are abandonned to the GC which is probably not
     * so bad. To permit acceleration of the collection of resources, a
     * purge listener will be added soon.
     */
    public Cache(long maxSize, CacheEntryListener listener) {
        this.maxSize = maxSize;
        this.size = 0;
        this.listener = listener;
    }
    
    /**
     * Empties the cache completely.
     * The entries are abandonned to the GC.
     */
    public void clear() {
        lru.clear();
        map.clear();
    }
    
    /**
     * Purges some of the cache.
     * The entries are cleaned-up properly.
     */
    public void purge(int fraction) {
        if (size == 0) {
            return;
        }
        
        if (fraction == 0) {
            fraction = 1;
        }
        long nbToPurge = size / fraction;

        if (nbToPurge == 0) {
            nbToPurge = 1;
        }
        
        while (nbToPurge-- > 0) {
            CacheEntryImpl toRm = (CacheEntryImpl) lru.next();

            map.remove(toRm.getKey());
            toRm.unlink();
            --size;
            if (listener != null) {
                listener.purged(toRm);
            }
        }
    }
    
    /**
     * Inserts the given cache entry directly.
     * Returns the previous cache entry associated with the given key, if any.
     * Not exposed yet. Should not be a problem to expose it, but it is not
     * needed yet.
     */
    protected CacheEntry putCacheEntry(Object key, CacheEntry value) {
        if (size == maxSize) {
            CacheEntryImpl toRm = (CacheEntryImpl) lru.next();

            map.remove(toRm.getKey());
            toRm.unlink();
            --size;
            if (listener != null) {
                listener.purged(toRm);
            }
        }
        
        lru.putLast((CacheEntryImpl) value);
        ++size;
        
        CacheEntryImpl oldEntry = (CacheEntryImpl) map.put(key, value);

        if (oldEntry == null) {
            return null;
        }
        
        if (oldEntry.isLinked()) {
            oldEntry.unlink();
            --size;
        }
        return oldEntry;
    }
    
    /**
     * Create a cache entry to hold the given value, and insert it.
     * Returns the previous value associated with the given key, if any.
     */
    public Object put(Object key, Object value) {
        CacheEntry oldEntry = putCacheEntry(key, new CacheEntryImpl(key, value));
        
        if (oldEntry == null) {
            return null;
        }
        
        return oldEntry.getValue();
    }
    
    /**
     * Remove the value, if any, and cacheEntry associated with the given key.
     * return the cacheEntry that has been removed.
     * Not exposed yet. Should not be a problem to expose it, but it is not
     * needed yet.
     */
    protected CacheEntry removeCacheEntry(Object key) {
        CacheEntryImpl oldEntry = (CacheEntryImpl) map.remove(key);

        if (oldEntry == null) {
            return null;
        }
        if (oldEntry.isLinked()) {
            oldEntry.unlink();
            --size;
        }
        return oldEntry;
    }
    
    /**
     * Remove the value, if any, and cacheEntry associated with the given key.
     * returns the value that has been removed.
     */
    public Object remove(Object key) {
        CacheEntry oldEntry = removeCacheEntry(key);

        if (oldEntry == null) {
            return null;
        }
        return oldEntry.getValue();
    }
    
    /**
     * Return the cache entry, if any, associated with the given key.
     * This is public; it improves performance by letting the application
     * do a single lookup instead of two when it needs to find an object in
     * the cache and then change its purgeability.
     */
    public CacheEntry getCacheEntry(Object key) {
        CacheEntryImpl foundEntry = (CacheEntryImpl) map.get(key);

        if (foundEntry == null) {
            return null;
        }
        
        // Leave the purgeability status alone but manage lru position if
        // purgeable.
        if (foundEntry.isLinked()) {
            lru.putLast(foundEntry);
        }
        return foundEntry;
    }
    
    /**
     * Return the value, if any associated with the given key.
     */
    public Object get(Object key) {
        CacheEntry foundEntry = getCacheEntry(key);

        if (foundEntry == null) {
            return null;
        }
        return foundEntry.getValue();
    }
    
    /**
     * Change the purgeability of the given cacheEntry.
     * If sticky is true, the entry cannot be purged.
     * Note: if the CacheEntry is known, it is more efficient to use this
     * method than sticky(), since sticky will preform a hashmap lookup
     * to locate the cache entry.
     */
    public void stickyCacheEntry(CacheEntry ce, boolean sticky) {
        CacheEntryImpl target = (CacheEntryImpl) ce;
        
        if (sticky) {
            
            // Stiky => not purgeable.
            
            if (!target.isLinked()) {
                return;
            }
            target.unlink();
            --size;
            
        } else {
            
            // ! Sticky => purgeable.
            
            if (target.isLinked()) {
                return;
            }
            if (size == maxSize) {
                CacheEntryImpl toRm = (CacheEntryImpl) lru.next();

                map.remove(toRm.getKey());
                toRm.unlink();
                if (listener != null) {
                    listener.purged(toRm);
                }
                --size;
            }
            
            lru.putLast(target);
            ++size;
            
        }
    }
    
    /**
     * Force the value associated with the given key to be purgeable or
     * non-purgeable from the cache (non-sticky vs. sticky).
     * Note: Most often, a call to the get() method will be performed
     * before it can be decided to invoke sticky(). Whenever this is the case
     * it is better to invoke getCacheEntry() + getValue() and then
     * stickyCacheEntry() since that eliminates one hashmap lookup.
     */
    public void sticky(Object key, boolean sticky) {
        CacheEntry foundEntry = (CacheEntry) map.get(key);

        if (foundEntry == null) {
            return;
        }
        stickyCacheEntry(foundEntry, sticky);
    }
}
