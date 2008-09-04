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

package net.jxta.impl.access;


import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLElement;
import net.jxta.document.XMLDocument;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.UndeclaredThrowableException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;


/**
 * Manages Access Permissions.
 */
public class AccessList {
    
    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(AccessList.class.getName());

    private final static String PEER_TAG = "peer";
    private final static String NAME_TAG = "name";
    private final static String DESCRIPTION_TAG = "description";
    private final static String GRANTALL_TAG = "grantAll";
    private final static String ACCESS_TAG = "access";
    private final static String ACCESS_TAG_DENY_VALUE = "deny";
    private final static String ACCESS_TAG_GRANT_VALUE = "grant";
    
    protected final Map<ID, Entry> accessMap = new HashMap<ID, Entry>();
    
    String description = null;
    boolean grantAll = false;
    
    /**
     * Default Constructor
     */
    public AccessList() {}

    /**
     * Initialize access list from an InputStream
     *
     * @param stream the input stream
     * @throws java.io.IOException if an io error occurs
     */
    public AccessList(InputStream stream) throws IOException {
        init(stream);
    }

    /**
     * Initialize access list from a URI
     * <p/>
     * e.g. file:/export/dist/acl.xml, e.g. http://configserver.net/edge.acl
     *
     * @param uri the URI to the access control list
     * @throws IOException if an i/o error occurs
     */
    public AccessList(URI uri) throws IOException {
        init(uri);
    }

    /**
     * Initialize the access list from a URI
     *
     * @param uri the refresh URI
     * @throws IOException if an io error occurs
     */
    public void init(URI uri) throws IOException {
        InputStream input = getInputStream(uri);

        init(input);
        input.close();
    }

    /**
     * Initialize access list from a file
     *
     * @param fromFile file to init from
     * @throws IOException if an io error occurs
     */
    public void init(File fromFile) throws IOException {
        InputStream is = new FileInputStream(fromFile);

        init(is);
        is.close();
    }

    /**
     * Refresh access list from a file
     *
     * @param file file to refresh from
     * @deprecated use URI variant
     */
    @Deprecated
    public void refresh(File file) {
        if (file.exists()) {
            try {
                InputStream is = new FileInputStream(file);

                refresh(is);
                is.close();
            } catch (IOException io) {// bad input
            }
        }
    }

    /**
     * refresh the access list from a stream
     *
     * @param stream the input stream
     * @throws IOException if an io error occurs
     */
    public void refresh(InputStream stream) throws IOException {
        AccessList tmp = new AccessList(stream);

        refresh(tmp);
    }

    /**
     * refresh the access list from a URI
     *
     * @param uri the refresh URI
     * @throws IOException if an io error occurs
     */
    public void refresh(URI uri) throws IOException {
        InputStream input = getInputStream(uri);
        AccessList tmp = new AccessList(input);

        refresh(tmp);
        input.close();
    }

    private InputStream getInputStream(URI uri) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Loading ACL : " + uri.toString());
        }

        URL url = uri.toURL();

        URLConnection connection = url.openConnection();

        connection.setDoInput(true);
        return connection.getInputStream();
    }

    private void init(InputStream stream) throws IOException {
        XMLDocument doc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, stream);
        
        initialize(doc);
    }

    /**
     * @param map The map of addresses and permissions.
     */
    public AccessList(Map<ID, Entry> map) {
        this.accessMap.clear();
        this.accessMap.putAll(map);
    }

    /**
     * Construct from a StructuredDocument
     *
     * @param root root element
     */
    public AccessList(Element root) {
        if (!(root instanceof XMLElement)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;

        if (!getAdvertisementType().equals(doc.getName())) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }
        
        initialize(doc);
    }

    /**
     * gets the description of this access control
     *
     * @return the document description
     */
    public String getDescrption() {
        return description;
    }

    /**
     * Determine if all access is granted.
     *
     * @return If {@code true} then all access requests will be granted.
     */
    public boolean getGrantAll() {
        return grantAll;
    }

    /**
     * Allows/denies all access
     *
     * @param grantAll If {@code true} then all access requests will be granted.
     */
    public void setGrantAll(boolean grantAll) {
        this.grantAll = grantAll;
    }

    /**
     * sets the ACL description
     *
     * @param description The new description
     */
    public void setDescrption(String description) {
        this.description = description;
    }

    /**
     * sets the entries list
     *
     * @param map The new access map
     */
    protected void setEntries(Map<ID, Entry> map) {
        accessMap.clear();
        accessMap.putAll(map);
    }

    /**
     * Refreshes the access list
     *
     * @param acl The access list to refresh from
     */
    private void refresh(AccessList acl) {
        grantAll = acl.grantAll;
        description = acl.description;
        accessMap.clear();
        accessMap.putAll(acl.accessMap);
    }

    /**
     * Adds an ACL entry
     *
     * @param entry the entry to add
     */
    public void add(Entry entry) {
        if (!accessMap.containsKey(entry.id)) {
            accessMap.put(entry.id, entry);
        }
    }

    /**
     * Removes an ACL entry
     *
     * @param entry the entry to remove
     */
    public void remove(Entry entry) {
        if (accessMap.containsKey(entry.id)) {
            accessMap.remove(entry.id);
        }
    }

    /**
     * Determines if an entry has access
     * 
     * @param id the PeerID
     * @return ture if access is allowed, always true if grantAll is set
     */
    public boolean isAllowed(ID id) {
        if (grantAll) {
            return true;
        } else if (accessMap.containsKey(id)) {
            Entry entry = accessMap.get(id);

            return entry.access;
        } else {
            return false;
        }
    }

    /**
     * gets the entries list
     *
     * @return List The List containing Entries
     */
    public Map<ID, Entry> getAccessMap() {
        return accessMap;
    }

    /**
     * Returns the Document
     *
     * @param asMimeType mime type encoding
     * @return The document value
     */
    public Document getDocument(MimeMediaType asMimeType) {
        StructuredDocument adv = StructuredDocumentFactory.newStructuredDocument(asMimeType, getAdvertisementType());

        if (adv instanceof XMLDocument) {
            ((XMLDocument) adv).addAttribute("xmlns:jxta", "http://jxta.org");
        }

        Element e;

        if (grantAll) {
            e = adv.createElement(GRANTALL_TAG, Boolean.valueOf(grantAll).toString());
            adv.appendChild(e);
        }
        
        if (description != null) {
            e = adv.createElement(DESCRIPTION_TAG, description);
            adv.appendChild(e);
        }
        
        for (Object o : accessMap.values()) {
            Entry entry = (Entry) o;

            if (entry.id == null && entry.name == null) {
                // skip bad entries
                continue;
            }
            e = adv.createElement(PEER_TAG, entry.id.toString());
            adv.appendChild(e);
            ((Attributable) e).addAttribute(NAME_TAG, entry.name);
            if (entry.access) {
                ((Attributable) e).addAttribute(ACCESS_TAG, ACCESS_TAG_GRANT_VALUE);
            } else {
                ((Attributable) e).addAttribute(ACCESS_TAG, ACCESS_TAG_DENY_VALUE);
            }
        }
        return adv;
    }

    /**
     * Process an individual element from the document.
     *
     * @param doc the element
     */
    protected void initialize(XMLElement doc) {

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();
            
            if (GRANTALL_TAG.equals(elem.getName())) {
                grantAll = Boolean.getBoolean(elem.getTextValue());
                if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                    LOG.config("Grant all access = [ " + grantAll + " ]");
                }

                continue;
            }
            
            if (DESCRIPTION_TAG.equals(elem.getName())) {
                description = elem.getTextValue();
                if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                    LOG.config("Loading [ " + description + " ] access list :");
                }
                
                continue;
            }
            
            if (PEER_TAG.equals(elem.getName())) {
                String name = "NA";
                Attribute nameAttr = elem.getAttribute(NAME_TAG);

                if (nameAttr != null) {
                    name = nameAttr.getValue();
                }
                String access = ACCESS_TAG_GRANT_VALUE;
                Attribute accessAttr = elem.getAttribute(ACCESS_TAG);

                if (accessAttr != null) {
                    access = accessAttr.getValue();
                }
                boolean acl = ACCESS_TAG_GRANT_VALUE.equalsIgnoreCase(access);
                
                ID pid;

                try {
                    URI id = new URI(elem.getTextValue().trim());

                    pid = IDFactory.fromURI(id);
                } catch (URISyntaxException badID) {
                    throw new IllegalArgumentException("unknown ID format in advertisement: " + elem.getTextValue());
                }
                
                Entry entry = new Entry(pid, name, acl);

                if (Logging.SHOW_CONFIG && LOG.isLoggable(Level.CONFIG)) {
                    LOG.config("Adding entry to access list :" + entry);
                }
                
                accessMap.put(entry.id, entry);
                
                continue;
            }
            
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Unrecognized tag : " + elem.getName());
            }            
        }
    }

    /**
     * returns the document string representation of this object
     *
     * @return String representation of the of this message type
     */
    @Override
    public String toString() {

        try {
            XMLDocument doc = (XMLDocument) getDocument(MimeMediaType.XMLUTF8);

            return doc.toString();
        } catch (Throwable e) {
            if (e instanceof Error) {
                throw (Error) e;
            } else if (e instanceof RuntimeException) {
                throw (RuntimeException) e;
            } else {
                throw new UndeclaredThrowableException(e);
            }
        }
    }

    /**
     * All messages have a type (in xml this is &#0033;doctype) which
     * identifies the message
     *
     * @return String "jxta:XACL"
     */
    public static String getAdvertisementType() {
        return "jxta:XACL";
    }

    /**
     * Entries class
     */
    public final static class Entry {

        /**
         * Entry ID entry id
         */
        public final ID id;

        /**
         * Entry name
         */
        public final String name;

        /**
         * Entry name
         */
        public final boolean access;

        /**
         * Creates a Entry with id and name
         *
         * @param id     id
         * @param name   node name
         * @param access access control
         */

        public Entry(ID id, String name, boolean access) {
            this.id = id;
            this.name = name;
            this.access = access;
        }

        @Override
        public String toString() {
            return "[" + name + "  access = " + access + " : " + id.toString() + "]";
        }

        @Override
        public boolean equals(Object obj) {
            return this == obj || (obj != null && id.equals(((Entry) obj).id));
        }

        @Override
        public int hashCode() {
            return id.hashCode();
        }
    }
}

