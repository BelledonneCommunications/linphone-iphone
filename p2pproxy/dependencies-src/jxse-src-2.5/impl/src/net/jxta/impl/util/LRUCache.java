/*
 *  Copyright 1999-2004 The Apache Software Foundation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package net.jxta.impl.util;

import java.util.Iterator;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This class implements a Generic LRU Cache. The cache is not thread-safe.
 *
 * @author Ignacio J. Ortega
 * @author Mohamed Abdelaziz
 */

public class LRUCache<K, V> {

    private final transient int cacheSize;
    private transient int currentSize;
    private transient CacheNode<K, V> first;
    private transient CacheNode<K, V> last;
    private final transient Map<K, CacheNode<K, V>> nodes;

    /**
     * Constructor for the LRUCache object
     *
     * @param size Description of the Parameter
     */
    public LRUCache(int size) {
        currentSize = 0;
        cacheSize = size;
        nodes = new HashMap<K, CacheNode<K, V>>(size);
    }

    /**
     * clear the cache
     */
    public void clear() {
        first = null;
        last = null;
        nodes.clear();
        currentSize = 0;
    }

    /**
     * returns the number of elements currently in cache
     *
     * @return the number of elements in cache
     */
    public int size() {
        return currentSize;
    }

    /**
     * retrieve an object from cache
     *
     * @param key key
     * @return object
     */
    public V get(K key) {
        CacheNode<K, V> node = nodes.get(key);

        if (node != null) {
            moveToHead(node);
            return node.value;
        }
        return null;
    }

    public boolean contains(K key) {
        return nodes.keySet().contains(key);
    }

    protected Iterator<V> iterator(int size) {
        List<V> list = new ArrayList<V>();

        for (CacheNode<K, V> node : nodes.values()) {
            list.add(node.value);
            if (list.size() >= size) {
                break;
            }
        }
        return list.iterator();
    }

    private void moveToHead(CacheNode<K, V> node) {
        if (node == first) {
            return;
        }
        if (node.prev != null) {
            node.prev.next = node.next;
        }
        if (node.next != null) {
            node.next.prev = node.prev;
        }
        if (last == node) {
            last = node.prev;
        }
        if (first != null) {
            node.next = first;
            first.prev = node;
        }
        first = node;
        node.prev = null;
        if (last == null) {
            last = first;
        }
    }

    /**
     * puts an object into cache
     *
     * @param key   key to store value by
     * @param value object to insert
     */
    public void put(K key, V value) {
        CacheNode<K, V> node = nodes.get(key);

        if (node == null) {
            if (currentSize >= cacheSize) {
                if (last != null) {
                    nodes.remove(last.key);
                }
                removeLast();
            } else {
                currentSize++;
            }
            node = new CacheNode<K, V>(key, value);
        }
        node.value = value;
        moveToHead(node);
        nodes.put(key, node);
    }

    /**
     * remove an object from cache
     *
     * @param key key
     * @return Object removed
     */
    public V remove(K key) {
        CacheNode<K, V> node = nodes.get(key);

        if (node != null) {
            if (node.prev != null) {
                node.prev.next = node.next;
            }
            if (node.next != null) {
                node.next.prev = node.prev;
            }
            if (last == node) {
                last = node.prev;
            }
            if (first == node) {
                first = node.next;
            }
        }
        if (node != null) {
            return node.value;
        } else {
            return null;
        }
    }

    /**
     * removes the last entry from cache
     */
    private void removeLast() {
        if (last != null) {
            if (last.prev != null) {
                last.prev.next = null;
            } else {
                first = null;
            }
            last = last.prev;
        }
    }

    /**
     * cache node object wrapper
     */
    protected class CacheNode<K, V> {
        final K key;
        CacheNode<K, V> next;

        CacheNode<K, V> prev;
        V value;

        /**
         * Constructor for the CacheNode object
         *
         * @param key   key
         * @param value value
         */
        CacheNode(K key, V value) {
            this.key = key;
            this.value = value;
        }
    }
}

