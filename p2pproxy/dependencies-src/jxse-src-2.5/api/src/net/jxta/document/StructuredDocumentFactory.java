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
import java.util.Hashtable;
import java.util.Map;

import java.io.IOException;
import java.util.MissingResourceException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import net.jxta.util.ClassFactory;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.TextMessageElement;


/**
 * A factory for constructing instances of {@link StructuredDocument}.
 * Behind the scenes, it also provides for the registration of the mime-types
 * and constructors needed to accomplish the construction. All supported
 * mime-types will need to register their implementation in this factory.
 *
 * @see         net.jxta.document.Document
 * @see         net.jxta.document.StructuredTextDocument
 * @see         net.jxta.document.StructuredDocument
 * @see         net.jxta.document.MimeMediaType
 */
public final class StructuredDocumentFactory extends ClassFactory<MimeMediaType, StructuredDocumentFactory.Instantiator> {
    
    /**
     * Log4J Logger
     */
    private static final Logger LOG = Logger.getLogger(StructuredDocumentFactory.class.getName());
    
    /**
     * Interface for instantiators of StructuredDocuments
     */
    public interface Instantiator {
        
        /**
         *  For mapping between extensions and MIME types.
         */
        class ExtensionMapping extends Object {

            /**
             * The extension
             */
            private final String extension;
            
            /**
             * MIME type it maps to
             */
            private final MimeMediaType mimetype;
            
            /**
             *  default constructor
             */
            public ExtensionMapping(String extension, MimeMediaType mimetype) {
                this.extension = extension;
                this.mimetype = (null != mimetype) ? mimetype.intern() : null;
            }
            
            /**
             * {@inheritDoc}
             */
            @Override
            public boolean equals(Object target) {
                if (this == target) {
                    return true;
                }
                
                if (target instanceof ExtensionMapping) {
                    ExtensionMapping likeMe = (ExtensionMapping) target;

                    if (!extension.equals(likeMe.extension)) {
                        return false;
                    }
                    
                    if ((null == mimetype) && (null == likeMe.mimetype)) {
                        return true;
                    }
                    
                    if ((null == mimetype) || (null == likeMe.mimetype)) {
                        return false;
                    }
                    
                    return mimetype.equals(likeMe.mimetype);
                } else {
                    return false;
                }
            }
            
            /**
             * {@inheritDoc}
             */
            @Override
            public int hashCode() {
                int hash = extension.hashCode();
                
                if (null != mimetype) {
                    hash ^= mimetype.hashCode();
                }
                
                return hash;
            }
            
            /**
             * {@inheritDoc}
             */
            @Override
            public String toString() {
                return extension + " -> " + ((null != mimetype) ? mimetype.toString() : "<null>");
            }
            
            /**
             * Returns the extension which is part of this mapping.
             *
             * @return the extension which is part of this mapping.
             */
            public String getExtension() {
                return extension;
            }
            
            /**
             * Returns the MIME Media Type which is part of this mapping.
             *
             * @return the MIME Media Type which is part of this mapping.
             */
            public MimeMediaType getMimeMediaType() {
                return mimetype;
            }
        }
        
        /**
         * Returns the MIME Media types supported by this this Document per
         * {@link <a href="http://www.ietf.org/rfc/rfc2046.txt">IETF RFC 2046 <i>MIME : Media Types</i></a>}.
         *
         * <p/>JXTA does not currently support the 'Multipart' or 'Message'
         * media types.
         *
         * @return An array of MimeMediaType objects containing the MIME Media Type
         *  for this Document.
         */
        MimeMediaType[] getSupportedMimeTypes();
        
        /**
         * Returns the mapping of file extension and mime-types for this type
         * of document. The default extension is mapped to the 'null' mime-type
         * and should only be used if no other mapping matches.
         *
         * @return An array of objects containing file extensions
         */
        ExtensionMapping[] getSupportedFileExtensions();
        
        /**
         * Create a new structured document of the type specified by doctype.
         *
         *  @param mimeType The MIME type to be associated with this instance.
         *  the base type must be one of the types returned by
         *  <tt>getSupportedMimeTypes</tt>. Some implementations may accept
         *  parameters in the params section of the MIME type.
         *  @param doctype Type for the base node of the document.
         *  @return StructuredDocument instance.
         */
        StructuredDocument newInstance(MimeMediaType mimeType, String doctype);
        
        /**
         * Create a new structured document of the type specified by doctype.
         *
         *  @param mimeType The MIME type to be associated with this instance.
         *  The base type must be one of the types returned by
         *  <tt>getSupportedMimeTypes</tt>. Some implementations may accept
         *  parameters in the params section of the MIME type.
         *  @param doctype Type for the base node of the document.
         *  @param value Value for the base node of the document.
         *  @return {@link StructuredDocument} instance.
         */
        StructuredDocument newInstance(MimeMediaType mimeType, String doctype, String value);
        
        /**
         *  Create a structured document from a stream containing an appropriately serialized
         *  instance of the same document.
         *
         *  @param mimeType The MIME type to be associated with this instance.
         *  The base type must be one of the types returned by
         *  <tt>getSupportedMimeTypes</tt>. Some implementations may accept
         *  parameters in the params section of the MIME type.
         *  @param source The {@code Inputstream} from which to read the
         *  document.
         *  @return {@link StructuredDocument} instance.
         *  @throws IOException Thrown for problems reading from the source.
         */
        StructuredDocument newInstance(MimeMediaType mimeType, InputStream source) throws IOException;
    }
    

    /**
     *  Interface for instantiators of StructuredTextDocuments
     */
    public interface TextInstantiator extends Instantiator {
        
        /**
         *  Create a structured document from a Reader containing an appropriately serialized
         *  instance of the same document.
         *
         *  @param mimeType The MIME type to be associated with this instance.
         *  The base type must be one of the types returned by
         *  <tt>getSupportedMimeTypes</tt>. Some implementations may accept
         *  parameters in the params section of the MIME type.
         *  @param source {@code Reader} from which to read the instance.
         *  @return {@link StructuredDocument} instance.
         *  @throws IOException Thrown for problems reading from the source.
         */
        StructuredDocument newInstance(MimeMediaType mimeType, Reader source) throws IOException;
    }
    
    /**
     *  This class is a singleton. This is the instance that backs the
     *  static methods.
     */
    private static final StructuredDocumentFactory factory = new StructuredDocumentFactory();
    
    /**
     *  This is the map of mime-types and instantiators used by
     *  <CODE>newStructuredDocument</CODE>.
     */
    private final Map<MimeMediaType, Instantiator> encodings = new HashMap<MimeMediaType, Instantiator>();
    
    /**
     *  This is the map of extensions to mime-types used by
     *  {@link #getMimeTypeForFileExtension(String) }
     */
    private final Map<String, MimeMediaType>  extToMime = new HashMap<String, MimeMediaType>();
    
    /**
     *  This is the map of mime-types to extensions used by
     *  {@link #getFileExtensionForMimeType(MimeMediaType mimetype) }
     */
    private final Map<MimeMediaType, String>  mimeToExt = new HashMap<MimeMediaType, String>();
    
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
    private StructuredDocumentFactory() {}
    
    /**
     *  Registers the pre-defined set of StructuredDocument sub-classes so that
     *  this factory can construct them.
     *
     *  @return true if at least one of the StructuredDocument sub-classes could
     *  be registered otherwise false.
     */
    private synchronized boolean loadProviders() {
        if (factory.loadedProperty) {
            return true;
        }

        factory.loadedProperty = registerProviders(StructuredDocument.class.getName());
        
        return factory.loadedProperty;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    protected Map<MimeMediaType, Instantiator> getAssocTable() {
        return encodings;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    protected Class<MimeMediaType> getClassForKey() {
        return MimeMediaType.class;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    protected Class<Instantiator> getClassOfInstantiators() {
        // our key is the doctype names.
        return Instantiator.class;
    }
    
    /**
     *  {@inheritDoc}
     *
     *  <p/>We override the standard implementation to get the MIME type from
     *  the class and use that as the key to register the class with the factory.
     *
     *  @param className The class name which will be registered.
     *  @return boolean true if the class was registered otherwise false.
     */
    @Override
    protected boolean registerAssoc(String className) {
        boolean registeredSomething = false;
        
        LOG.finer("Registering : " + className);
        
        try {
            Class docClass = Class.forName(className);
            
            Instantiator instantiator = (Instantiator) docClass.getField("INSTANTIATOR").get(null);
            
            MimeMediaType[] mimeTypes = instantiator.getSupportedMimeTypes();
            
            for (int eachType = 0; eachType < mimeTypes.length; eachType++) {
                LOG.finer("   Registering Type : " + mimeTypes[eachType].getMimeMediaType());
                registeredSomething |= registerInstantiator(mimeTypes[eachType], instantiator);
            }
        } catch (Exception all) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to register \'" + className + "\'", all);
            }
        }
        
        return registeredSomething;
    }
    
    /**
     *  Returns the preferred extension for a given mime-type. If there is no
     *  mapping or no preferred extension for this MIME type then <tt>null</tt> is
     *  returned.
     *
     *  @param mimetype the MimeMediaType we wish to know the file extension for.
     *  @return String containing the extension or null for mime-types with no
     *  known association.
     */
    public static String getFileExtensionForMimeType(MimeMediaType mimetype) {
        factory.loadProviders();
        
        return factory.mimeToExt.get(mimetype.getBaseMimeMediaType());
    }
    
    /**
     *  Returns the preferred mime-type for a given file extension. If there is
     *  no mapping then <tt>null</tt> is returned.
     *
     *  @param extension The extension we wish to know the mime-type for.
     *  @return MimeMediaType associated with this file extension.
     */
    public static MimeMediaType getMimeTypeForFileExtension(String extension) {
        factory.loadProviders();
        
        MimeMediaType result = factory.extToMime.get(extension);
        
        return result;
    }
    
    /**
     * Register an instantiator object a mime-type of documents to be
     * constructed.
     *
     * @param mimetype   the mime-type associated.
     * @param instantiator the instantiator that wants to be registered..
     * @return boolean true   if the instantiator for this mime-type is now
     * registered. If there was already an instantiator this mime-type then
     * false will be returned.
     * @throws SecurityException   there were permission problems registering
     *  the instantiator.
     */
    public static boolean registerInstantiator(MimeMediaType mimetype, Instantiator instantiator) {
        boolean registered = factory.registerAssoc(mimetype.getBaseMimeMediaType(), instantiator);
        
        if (registered) {
            Instantiator.ExtensionMapping[] extensions = instantiator.getSupportedFileExtensions();
            
            for (int eachExt = 0; eachExt < extensions.length; eachExt++) {
                if (null != extensions[eachExt].getMimeMediaType()) {
                    factory.extToMime.put(extensions[eachExt].getExtension(), extensions[eachExt].getMimeMediaType().intern());
                    
                    factory.mimeToExt.put(extensions[eachExt].getMimeMediaType(), extensions[eachExt].getExtension());
                    
                    // And the base version.
                    factory.mimeToExt.put(extensions[eachExt].getMimeMediaType().getBaseMimeMediaType(), extensions[eachExt].getExtension());
                }
            }
        }
        
        return registered;
    }
    
    /**
     * Constructs an instance of {@link StructuredDocument} matching
     * the mime-type specified by the <CODE>mimetype</CODE> parameter. The
     * <CODE>doctype</CODE> parameter identifies the base type of the
     * {@link StructuredDocument}.
     *
     * @param mimetype Specifies the mime media type to be associated with
     *  the {@link StructuredDocument} to be created.
     * @param doctype Specifies the root type of the {@link StructuredDocument}
     *  to be created.
     * @return StructuredDocument The instance of {@link StructuredDocument}
     *  or null if it could not be created.
     * @throws NoSuchElementException invalid mime-media-type
     */
    public static StructuredDocument newStructuredDocument(MimeMediaType mimetype, String doctype) {
        factory.loadProviders();
        
        Instantiator instantiator = factory.getInstantiator(mimetype.getBaseMimeMediaType());
        
        return instantiator.newInstance(mimetype, doctype);
    }
    
    /**
     * Constructs an instance of {@link StructuredDocument} matching
     * the mime-type specified by the <CODE>mimetype</CODE> parameter. The
     * <CODE>doctype</CODE> parameter identifies the base type of the
     * {@link StructuredDocument}. Value supplies a value for the root
     * element.
     *
     * @param mimetype Specifies the mime media type to be associated with
     *  the {@link StructuredDocument} to be created.
     * @param doctype Specifies the root type of the {@link StructuredDocument}
     *  to be created.
     * @param value Specifies a value for the root element.
     * @return StructuredDocument The instance of {@link StructuredDocument}
     *  or null if it could not be created.
     * @throws NoSuchElementException if the mime-type has not been registered.
     */
    public static StructuredDocument newStructuredDocument(MimeMediaType mimetype, String doctype, String value) {
        factory.loadProviders();
        
        Instantiator instantiator = factory.getInstantiator(mimetype.getBaseMimeMediaType());
        
        return instantiator.newInstance(mimetype, doctype, value);
    }
    
    /**
     * Constructs an instance of {@link StructuredDocument} matching
     * the mime-type specified by the <CODE>mimetype</CODE> parameter. The
     * <CODE>doctype</CODE> parameter identifies the base type of the
     * {@link StructuredDocument}.
     *
     * @param mimetype Specifies the mime media type to be associated with the
     *  {@link StructuredDocument} to be created.
     * @param stream Contains an InputStream from which the document will be
     *  constructed.
     * @return StructuredDocument The instance of {@link StructuredDocument}
     *  or null if it could not be created.
     * @throws IOException If there is a problem reading from the stream.
     * @throws NoSuchElementException if the mime-type has not been registered.
     */
    public static StructuredDocument newStructuredDocument(MimeMediaType mimetype, InputStream stream) throws IOException {
        factory.loadProviders();
        
        Instantiator instantiator = factory.getInstantiator(mimetype.getBaseMimeMediaType());
        
        return instantiator.newInstance(mimetype, stream);
    }
    
    /**
     * Constructs an instance of {@link StructuredDocument} matching
     * the mime-type specified by the <CODE>mimetype</CODE> parameter. The
     * <CODE>doctype</CODE> parameter identifies the base type of the
     * {@link StructuredDocument}.
     *
     * @param mimetype Specifies the mime media type to be associated with the
     * {@link StructuredDocument} to be created.
     * @param reader A Reader from which the document will be constructed.
     * @return StructuredDocument The instance of {@link StructuredDocument}
     * or {@code null} if it could not be created.
     * @throws IOException If there is a problem reading from the stream.
     * @throws NoSuchElementException if the mime-type has not been registered.
     * @throws UnsupportedOperationException if the mime-type provided is not
     * a text oriented MIME type.
     */
    public static StructuredDocument newStructuredDocument(MimeMediaType mimetype, Reader reader) throws IOException {
        factory.loadProviders();
        
        Instantiator instantiator = factory.getInstantiator(mimetype.getBaseMimeMediaType());
        
        if (!(instantiator instanceof TextInstantiator)) {
            // XXX 20020502 bondolo@jxta.org we could probably do something
            // really inefficient that would allow it to work, but better not to.
            // if ReaderInputStream existed, it would be easy to do.
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning( "Document Class \'" + instantiator.getClass().getName() + "\' associated with \'" + mimetype 
                    + "\' is not a text oriented document");
            }
            
            throw new UnsupportedOperationException( "Document Class '" + instantiator.getClass().getName() 
                    + "' associated with '" + mimetype + "' is not a text oriented document");
        }
        
        return ((TextInstantiator) instantiator).newInstance(mimetype, reader);
    }
    
    /**
     * Constructs an instance of {@link StructuredDocument} based upon the
     * content of the provided message element.
     *
     * @param element The message element from which to create the document.
     * @return StructuredDocument The instance of {@link StructuredDocument}
     *  or null if it could not be created.
     * @throws IOException If there is a problem reading from the stream.
     * @throws NoSuchElementException if the mime-type has not been registered.
     */
    public static StructuredDocument newStructuredDocument(MessageElement element) throws IOException {
        factory.loadProviders();
        
        Instantiator instantiator = factory.getInstantiator(element.getMimeType().getBaseMimeMediaType());
        
        if ((instantiator instanceof TextInstantiator) && (element instanceof TextMessageElement)) {
            return ((TextInstantiator) instantiator).newInstance(element.getMimeType(), ((TextMessageElement) element).getReader());
        } else {
            return instantiator.newInstance(element.getMimeType(), element.getStream());
        }
    }
}
