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

package net.jxta.document;


import java.io.InputStream;
import java.io.Reader;
import java.util.HashMap;
import java.util.Map;

import java.io.IOException;
import java.util.NoSuchElementException;
import java.util.MissingResourceException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.util.ClassFactory;


/**
 * A Factory class for constructing Advertisements. This class abstracts the
 * the implementations used to represent and create advertisements.
 *
 * <p/>Advertisements are core objects that are used to advertise a Peer, a
 * PeerGroup, a Service, a Pipe, etc. The Advertisement class provides a 
 * platform independent representation of core objects that can be exchanged
 * between different implementations (Java, C).
 *
 * <p/>The AdvertisementFactory extends the ClassFactory to register the various
 * types of advertisements into an internal table. The factory is called with
 * the Advertisement type requested to create the corresponding advertisement
 * type.
 *
 * <p/>The set of Advertisements types supported is loaded from the JXTA
 * classpath via the service provider interface.
 *
 * @see net.jxta.document.Advertisement
 * @see net.jxta.document.Document
 * @see net.jxta.document.MimeMediaType
 * @see net.jxta.peergroup.PeerGroup
 * @see net.jxta.protocol.PeerAdvertisement
 * @see net.jxta.protocol.PeerGroupAdvertisement
 * @see net.jxta.protocol.PipeAdvertisement
 */
public class AdvertisementFactory extends ClassFactory<String, AdvertisementFactory.Instantiator> {

    /**
     *  Logger
     */
    private static final Logger LOG = Logger.getLogger(AdvertisementFactory.class.getName());
    
    /**
     *  Interface for instantiators of Advertisements
     */
    public interface Instantiator {
        
        /**
         * Returns the identifying type of this Advertisement.
         *
         * @return String the type of advertisement
         */
        String getAdvertisementType();
        
        /**
         * Constructs an instance of {@link Advertisement} matching the type
         * specified by the <CODE>advertisementType</CODE> parameter.
         *
         *
         * @return The instance of {@link Advertisement}.
         */
        Advertisement newInstance();
        
        /**
         * Constructs an instance of {@link Advertisement} matching the type
         * specified by the <CODE>advertisementType</CODE> parameter.
         *
         * @param root Specifies a portion of a @link StructuredDocument} which
         * will be converted into an Advertisement.
         * @return The instance of {@link Advertisement}.
         */
        Advertisement newInstance(net.jxta.document.Element root);
    }
    
    /**
     *  This class is a singleton. This is the instance that backs the
     *  static methods.
     */
    private final static AdvertisementFactory factory = new AdvertisementFactory();
    
    /**
     *  This is the map of mime-types and constructors used by
     *  {@code newAdvertisement}.
     */
    private final Map<String, Instantiator> encodings = new HashMap<String, Instantiator>();
    
    /**
     *  If true then the pre-defined set of StructuredDocument sub-classes has
     *  been registered from the property containing them.
     */
    private boolean loadedProperty = false;
    
    /**
     *  Private constructor. This class is not meant to be instantiated except
     *  by itself.
     *
     */
    private AdvertisementFactory() {}
    
    /**
     *  Registers the pre-defined set of Advertisement sub-classes so that
     *  this factory can construct them.
     *
     *  @return true if at least one of the Advertisement sub-classes could
     *  be registered otherwise false.
     */
    private synchronized boolean loadProviders() {
        if (!factory.loadedProperty) {
            factory.loadedProperty = registerProviders(Advertisement.class.getName());
        }
        
        return factory.loadedProperty;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    protected Map<String, Instantiator> getAssocTable() {
        return encodings;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Class<Instantiator> getClassOfInstantiators() {
        // our key is the doctype names.
        return Instantiator.class;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Class<String> getClassForKey() {
        // our key is the doctype names.
        return java.lang.String.class;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    protected boolean registerAssoc(String className) {
        boolean registeredSomething = false;
        
        try {
            Class advClass = Class.forName(className + "$Instantiator");
            
            Instantiator instantiator = (Instantiator) advClass.newInstance();
            
            String advType = instantiator.getAdvertisementType();
            
            registeredSomething = registerAdvertisementInstance(advType, instantiator);
        } catch (Exception all) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.log(Level.FINE, "Failed to register \'" + className + "\'", all);
            }
        }
        
        return registeredSomething;
    }
    
    /**
     *  Register an instantiator for and advertisement type to allow instances
     *  of that type to be created.
     *
     *  @param rootType  the identifying value for this advertisement instance
     *   type.
     *  @param instantiator the instantiator to use in constructing objects
     *   of this rootType.
     *  @return boolean  true if the rootType type is registered. If there is
     *   already a constructor for this type then false will be returned.
     */
    public static boolean registerAdvertisementInstance(String rootType, Instantiator instantiator) {
        boolean result = factory.registerAssoc(rootType, instantiator);
        
        return result;
    }
    
    /**
     * Constructs a new instance of {@link Advertisement} matching the type
     * specified by the {@code advertisementType} parameter.
     *
     * @param advertisementType Specifies the type of advertisement to create.
     * @return The instance of {@link Advertisement}.
     * @throws NoSuchElementException if there is no matching advertisement type.
     */
    public static Advertisement newAdvertisement(String advertisementType) {
        factory.loadProviders();
        
        Instantiator instantiator = factory.getInstantiator(advertisementType);
        
        Advertisement a = instantiator.newInstance();
        
        return a;
    }
    
    /**
     * Constructs an instance of {@link Advertisement} from the provided
     * <code>InputStream</code>. The content type of the stream is declared via
     * the <code>mimetype</code> parameter.
     *
     * @deprecated Please convert your code to construct an {@code XMLDocument}
     * using {@code StructuredDocumentFactory} and then call 
     * {@link AdvertisementFactory#newAdvertisement(XMLElement)}. For example :
     * <p/><pre>
     *   XMLDocument xml = (XMLDocument) StructuredDocumentFactory.newStructuredDocument( MimeMediaType.XMLUTF8, is );
     * </pre>
     * <b>or frequently:</b>
     * <p/><pre>
     *   XMLDocument xml = (XMLDocument) StructuredDocumentFactory.newStructuredDocument( msgElement );
     * </pre>
     * <b>followed by:</b>
     * <p/><pre>
     *   Advertisement adv = AdvertisementFactory.newAdvertisement(xml);
     * </pre>
     *
     * @param mimetype Specifies the mime media type of the stream being read.
     * @param stream input stream used to read data to construct the advertisement
     * @return The instance of {@link Advertisement}
     * @throws IOException error reading message from input stream
     * @throws NoSuchElementException if there is no matching advertisement type
     * for the type of document read in.
     */
    @Deprecated
    public static Advertisement newAdvertisement(MimeMediaType mimetype, InputStream stream) throws IOException {
        StructuredDocument doc = StructuredDocumentFactory.newStructuredDocument(mimetype, stream);
        
        if (!(doc instanceof XMLDocument)) {
            throw new IllegalArgumentException("Advertisements must be XML");
        }
        
        return newAdvertisement((XMLDocument) doc);
    }
    
    /**
     * Reconstructs an instance of {@link Advertisement} from the provided
     * <code>Reader</code>. The content type of the reader is declared via the
     * <code>mimetype</code> parameter.
     *
     * @deprecated Please convert your code to construct an {@code XMLDocument}
     * using {@code StructuredDocumentFactory} and then call 
     * {@link AdvertisementFactory#newAdvertisement(XMLElement)}. For example :
     * <p/><pre>
     *   XMLDocument xml = (XMLDocument) StructuredDocumentFactory.newStructuredDocument( MimeMediaType.XMLUTF8, reader );
     * </pre>
     * <b>or frequently:</b>
     * <p/><pre>
     *   XMLDocument xml = (XMLDocument) StructuredDocumentFactory.newStructuredDocument( msgElement );
     * </pre>
     * <b>followed by:</b>
     * <p/><pre>
     *   Advertisement adv = AdvertisementFactory.newAdvertisement(xml);
     * </pre>
     *
     * @param mimetype Specifies the mime media type of the stream being read.
     * @param source used to read data to construct the advertisement.
     * @return The instance of {@link Advertisement}
     * @throws IOException error reading message from input stream
     * @throws NoSuchElementException if there is no matching advertisement type
     * for the type of document read in.
     * @throws UnsupportedOperationException if the specified mime type is not
     *  associated with a text oriented document type.
     */
    @Deprecated
    public static Advertisement newAdvertisement(MimeMediaType mimetype, Reader source) throws IOException {
        StructuredTextDocument doc = (StructuredTextDocument) StructuredDocumentFactory.newStructuredDocument(mimetype, source);
        
        return newAdvertisement(doc);
    }
    
    /**
     * Reconstructs an instance of {@link Advertisement} matching the type
     * specified by the {@code root} parameter.
     *
     * @deprecated Advertisements must be encoded in XML. This is a legacy
     * static constructor. You should convert your code to use the 
     * {@link AdvertisementFactory#newAdvertisement(XMLElement) XMLElement}
     * version.
     *
     * @param root Specifies a portion of a StructuredDocument which will be
     * converted into an Advertisement.
     * @return The instance of {@link Advertisement}.
     * @throws NoSuchElementException if there is no advertisement type
     * matching the type of the root node.
     */
    @Deprecated
    public static Advertisement newAdvertisement(TextElement root) {
        if (!(root instanceof XMLElement)) {
            throw new IllegalArgumentException("Advertisements must be XML");
        }
        
        return newAdvertisement((XMLElement) root);
    }

    /**
     * Reconstructs an instance of {@link Advertisement} matching the type
     * specified by the {@code root} parameter.
     *
     * @param root Specifies a portion of an XMLElement which will be
     * converted into an Advertisement.
     * @return The instance of {@link Advertisement}.
     * @throws NoSuchElementException if there is no advertisement type
     * matching the type of the root node.
     */
    public static Advertisement newAdvertisement(XMLElement root) {
        factory.loadProviders();
        
        Instantiator instantiator = null;
        
        // The base type of the advertisement may be overridden by a type
        // declaration. If this is the case, then we try to use that as the
        // key rather than the root name.
        Attribute type = root.getAttribute("type");

        if (null != type) {
            try {
                instantiator = factory.getInstantiator(type.getValue());
            } catch (NoSuchElementException notThere) {
                // do nothing, its not fatal
                ;
            }
        }
        
        // Don't have an instantiator for the type attribute, try the root name
        if (null == instantiator) {
            instantiator = factory.getInstantiator(root.getName());
        }
        
        Advertisement a = instantiator.newInstance(root);
        
        return a;
    }
}
