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


import net.jxta.document.*;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.*;
import java.util.concurrent.atomic.AtomicInteger;


/**
 * A container for collections of configuration parameters. Configuration
 * parameters are stored in a Map which is keyed by {@code JXTA ID}s and whose
 * values are {@code Advertisement}s.
 */
public abstract class ConfigParams extends ExtendableAdvertisement implements Cloneable {
    
    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(ConfigParams.class.getName());
    
    private static final String SVC_TAG = "Svc";
    private static final String MCID_TAG = "MCID";
    private static final String PARAM_TAG = "Parm";
    
    /**
     * A table of structured documents to be interpreted by each service.
     * For safe operation these elements should be immutable, but we're helpless
     * if they are not.
     */
    private final Map<ID, StructuredDocument> params = new HashMap<ID, StructuredDocument>();
    
    /**
     * A map of advertisements to be interpreted by each service.
     * For safe operation we clone the advertisements when they are added to the
     * map and only ever return clones of the advertisements.
     */
    private final Map<ID, Advertisement> ads = new HashMap<ID, Advertisement>();
    
    /**
     *  The ids of the advertisements and/or params which have been explicitly
     *  marked as disabled.
     */
    private final Set<ID> disabled = new HashSet<ID>();
    
    /**
     * Counts the changes made to this object. The API increments it every time
     * some change is not proven to be idempotent. We rely on implementations to
     * increment modCount every time something is changed without going through
     * the API.
     */
    protected final transient AtomicInteger modCount = new AtomicInteger(0);
    
    /**
     * Returns the identifying type of this Advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:CP";
    }

    /**
     *  Default Constructor. We want all ConfigParams derived advertisements to
     *  pretty print. 
     */
    protected ConfigParams() {
        super(true);
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public ConfigParams clone() {
        
        try {
            ConfigParams result = (ConfigParams) super.clone();
            
            for (Map.Entry<ID, StructuredDocument> anEntry : params.entrySet()) {
                result.params.put(anEntry.getKey(), StructuredDocumentUtils.copyAsDocument(anEntry.getValue()));
            }
            
            for (Map.Entry<ID, Advertisement> anEntry : ads.entrySet()) {
                result.ads.put(anEntry.getKey(), anEntry.getValue().clone());
            }
            
            result.disabled.addAll(disabled);
            
            return result;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object other) {
        if(this == other) {
            return true;
        }
        
        if(other instanceof ConfigParams) {
            ConfigParams likeMe = (ConfigParams) other;
            
            boolean ep = params.equals(likeMe.params);
            boolean ea = ads.equals(likeMe.ads);
            boolean ed = disabled.equals(likeMe.disabled);            
            
            return ep && ea && ed;
        }
        
        return false;
    }

        
    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    protected boolean handleElement(Element raw) {
        
        if (super.handleElement(raw)) {
            return true;
        }
        
        XMLElement elem = (XMLElement) raw;
        
        if (SVC_TAG.equals(elem.getName())) {
            Attribute disabledAttr = elem.getAttribute("disabled");
            boolean isDisabled = (null != disabledAttr) && Boolean.parseBoolean(disabledAttr.getValue());
            
            Enumeration<XMLElement> elems = elem.getChildren();
            
            ID key = null;
            XMLElement param = null;
            
            while (elems.hasMoreElements()) {
                XMLElement e = elems.nextElement();
                
                if (MCID_TAG.equals(e.getName())) {
                    try {
                        URI mcid = new URI(e.getTextValue());

                        key = IDFactory.fromURI(mcid);
                    } catch (URISyntaxException badID) {
                        throw new IllegalArgumentException("Bad ID in advertisement: " + e.getTextValue());
                    }
                } else if (PARAM_TAG.equals(e.getName())) {
                    param = e;
                } else {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Unrecognized <Svc> tag : " + e.getName());
                    }
                }
            }
            
            if (key != null && param != null) {
                if(!isDisabled) {
                    // Backwards compatibility support.
                    Enumeration<XMLElement> isOff = param.getChildren("isOff");
                    
                    isDisabled = isOff.hasMoreElements();
                }            

                putServiceParam(key, param);
                if(isDisabled) {
                    disabled.add(key);
                }
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Incomplete Service Param : id=" + key + " param=" + param);
                }
                
                return false;
            }
            return true;
        }
        return false;
    }
    
    /**
     * Return the advertisement as a document.
     *
     *  @param adv the document to add elements to.
     *  @return true if elements were added otherwise false.
     */
    public boolean addDocumentElements(StructuredDocument adv) {
        
        for (Map.Entry<ID, StructuredDocument> anEntry : params.entrySet()) {
            ID anID = anEntry.getKey();
            StructuredDocument aDoc = anEntry.getValue();
            
            Element s = adv.createElement(SVC_TAG);

            adv.appendChild(s);
            
            if(disabled.contains(anID)) {
                ((Attributable)s).addAttribute("disabled", "true");
            }
            
            Element e = adv.createElement(MCID_TAG, anID.toString());

            s.appendChild(e);
            
            StructuredDocumentUtils.copyElements(adv, s, aDoc, PARAM_TAG);
        }
        
        for (Map.Entry<ID, Advertisement> anEntry : ads.entrySet()) {
            ID anID = anEntry.getKey();
            Advertisement anAdv = anEntry.getValue();
            
            Element s = adv.createElement(SVC_TAG);

            adv.appendChild(s);
            
            if(disabled.contains(anID)) {
                ((Attributable)s).addAttribute("disabled", "true");
            }
            
            Element e = adv.createElement(MCID_TAG, anID.toString());

            s.appendChild(e);
            
            StructuredDocument asDoc = (StructuredDocument) anAdv.getDocument(adv.getMimeType());
            
            StructuredDocumentUtils.copyElements(adv, s, asDoc, PARAM_TAG);
        }
        
        return true;
    }
    
    /**
     * Returns the number of times this object has been modified since it was
     * created. This permits the detection of local changes that require
     * refreshing some other data.
     *
     * @return int the current modification count.
     */
    public int getModCount() {
        return modCount.get();
    }
    
    /**
     *  Increases the modification count of this instance.
     *
     * @return modification count
     */
    protected synchronized int incModCount() {
        return modCount.incrementAndGet();
    }
    
    /**
     * Puts a service parameter in the service parameters table
     * under the given key. The key is usually a ModuleClassID. This method
     * makes a clone of the  given element into an independent document.
     *
     * @param key   The key.
     * @param param The parameter document.
     */
    public void putServiceParam(ID key, Element param) {
        incModCount();
        
        params.remove(key);
        ads.remove(key);
        
        if (param == null) {
            return;
        }
        
        boolean isDisabled = false;
        
        if (param instanceof XMLElement) {
            Enumeration<XMLElement> isOff = param.getChildren("isOff");
            
            isDisabled = isOff.hasMoreElements();
            
            Advertisement adv = null;
            
            try {
                adv = AdvertisementFactory.newAdvertisement((XMLElement) param);
            } catch (RuntimeException ignored) {
                // ignored
                ;
            }
            
            if (null != adv) {
                setSvcConfigAdvertisement(key,adv, !isDisabled);
                return;
            }
        }
        
        StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(param);
        
        if(isDisabled) {
            disabled.add(key);
        } else {
            disabled.remove(key);
        }

        params.put(key, newDoc);
    }
    
        
    /**
     * Puts an advertisement into the service parameters table under the given
     * key. The key is usually a {@code ModuleClassID}. This method makes a
     * clone of the advertisement.
     *
     * @param key The key.
     * @param adv The advertisement, a clone of which is stored or {@code null}
     * to forget this key.
     */
    public void setSvcConfigAdvertisement(ID key, Advertisement adv) {
        setSvcConfigAdvertisement(key, adv, true);
    }
    
    /**
     * Puts an advertisement into the service parameters table under the given
     * key. The key is usually a {@code ModuleClassID}. This method makes a
     * clone of the advertisement.
     *
     * @param key The key.
     * @param adv The advertisement, a clone of which is stored or {@code null}
     * to forget this key.
     * @param enabled If true then the service is enabled or disabled if false.
     */
    public void setSvcConfigAdvertisement(ID key, Advertisement adv, boolean enabled) {
        incModCount();
        
        params.remove(key);
        ads.remove(key);
        
        if(enabled) {
            disabled.remove(key);
        } else {
            disabled.add(key);
        }

        if (null == adv) {
            return;
        }
        
        try {
            ads.put(key, adv.clone());
        } catch (CloneNotSupportedException failed) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Unclonable Advertisements may not be used : " + adv.getClass().getName(), failed);
            }
                
            throw new IllegalArgumentException("Unclonable Advertisements may not be used : " + adv.getClass().getName());
        }        
    }
    
    /**
     * Gets an advertisement from the service parameters table under the given
     * key. The key is usually a {@code ModuleClassID}. This method makes a
     * clone of the advertisement.
     *
     * @param key The key.
     * @return If {@code true} then the service is enabled otherwise {@false} if
     * the service is disabled.
     */
    public boolean isSvcEnabled(ID key) {
        return !disabled.contains(key);
    }
    
    /**
     * Gets an advertisement from the service parameters table under the given
     * key. The key is usually a {@code ModuleClassID}. This method makes a
     * clone of the advertisement.
     *
     * @param key The key.
     * @return The advertisement for the specified key otherwise {@code null}.
     */
    public Advertisement getSvcConfigAdvertisement(ID key) {
        Advertisement adv = ads.get(key);
        
        if (null == adv) {
            if (params.containsKey(key)) {
                throw new IllegalStateException("Unable to return advertisement, params are not an advertisement.");
            }
            
            return null;
        }
        
        try {
            return adv.clone();
        } catch (CloneNotSupportedException failed) {
            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, "Unclonable Advertisements may not be used : " + adv.getClass().getName(), failed);
            }

            throw new IllegalArgumentException("Unclonable Advertisements may not be used : " + adv.getClass().getName());
        }
    }
    
    /**
     * Returns the parameter element that matches the given key from the
     * service parameters table. The key is of a subclass of ID; usually a
     * ModuleClassID.
     *
     * @param key The key.
     * @return StructuredDocument The matching parameter document or null if
     *         none matched.
     */
    public StructuredDocument getServiceParam(ID key) {
        StructuredDocument param = params.get(key);
        
        if (param == null) {
            Advertisement ad = ads.get(key);

            if (null == ad) {
                return null;
            }
            
            return (XMLDocument) ad.getDocument(MimeMediaType.XMLUTF8);
        }
        
        StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(param);
        
        if(disabled.contains(key)) {
            Enumeration<Element> isOffAlready = newDoc.getChildren("isOff");
            
            if(!isOffAlready.hasMoreElements()) {
                newDoc.appendChild(newDoc.createElement("isOff", null));
            }
        }

        return newDoc;
    }
    
    /**
     * Removes and returns the parameter element that matches the given key
     * from the service parameters table. The key is of a subclass of ID;
     * usually a ModuleClassID.
     *
     * @param key The key.
     *
     * @return The removed parameter element or {@code null} if not found.
     */
    public StructuredDocument removeServiceParam(ID key) {
        
        StructuredDocument param = params.remove(key);
        
        if (param == null) {
            Advertisement ad = ads.remove(key);

            if (null == ad) {
                return null;
            }
            
            return (XMLDocument) ad.getDocument(MimeMediaType.XMLUTF8);
        } else {
            ads.remove(key);
        }
        
        incModCount();
        
        // It sound silly to clone it, but remember that we could be sharing
        // this element with a clone of ours, so we have the duty to still
        // protect it.
        
        StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(param);
        
        if(disabled.contains(key)) {
            newDoc.appendChild(newDoc.createElement("isOff", null));
            disabled.remove(key);
        }
        
        return newDoc;
    }
    
    /**
     * Removes any parameters for the given key from the service parameters
     * table.
     *
     * @param key The key.
     */
    public void removeSvcConfigAdvertisement(ID key) {
        incModCount();
        
        params.remove(key);
        ads.remove(key);
    }
    
    /**
     * Returns the set of params held by this object. The parameters are not
     * copied and any changes to the Set are reflected in this object's version.
     * incModCount should be called as appropriate.
     *
     *  @deprecated This method exposes the internal data structures of the
     *  advertisement and will be removed in order to prevent unexpected
     *  behaviour.
     */
    @Deprecated
    public Set<Map.Entry<ID, StructuredDocument>> getServiceParamsEntrySet() {
        Map<ID, StructuredDocument> result = new HashMap<ID, StructuredDocument>();
        
        result.putAll(params);
        
        for (Map.Entry<ID, Advertisement> anEntry : ads.entrySet()) {
            XMLDocument entryDoc = (XMLDocument) anEntry.getValue().getDocument(MimeMediaType.XMLUTF8);
            
            if(disabled.contains(anEntry.getKey())) {
                entryDoc.appendChild(entryDoc.createElement("isOff", null));
            }

            result.put(anEntry.getKey(), entryDoc);
        }
        
        return Collections.unmodifiableSet(result.entrySet());
    }
}
