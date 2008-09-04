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

package net.jxta.endpoint;


import net.jxta.document.MimeMediaType;
import net.jxta.logging.Logging;
import net.jxta.util.ClassFactory;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Hashtable;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * This class is a class factory for Wire Format Messages. This class abstracts
 * The implementation of Wire Format Messages and allows for construction based
 * on the MimeType of InputStreams.
 * <p/>
 * The WireFormatMessageFactory extends the ClassFactory to register the
 * various Message wire format implementations into a static hashtable. The
 * factory is called with the Mime type requested to create the corresponding
 * Wire Format type.
 *
 * @see net.jxta.endpoint.Message
 * @see net.jxta.endpoint.WireFormatMessage
 * @see net.jxta.util.ClassFactory
 * @see net.jxta.document.MimeMediaType
 */
public final class WireFormatMessageFactory extends ClassFactory<MimeMediaType, WireFormatMessageFactory.Instantiator> {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(WireFormatMessageFactory.class.getName());

    /**
     * The mime media type of preferred/default wire format.
     */
    public static final MimeMediaType DEFAULT_WIRE_MIME = new MimeMediaType("application/x-jxta-msg").intern();

    /**
     * Interface for instantiators of wire format messages.
     */
    public interface Instantiator {

        /**
         * Returns the list of mime types supported by this serialization. All of
         * mimetypes in this list should have no mime type parameters.
         *
         * @return Returns the list of mime types supported by this serialization.
         */
        public MimeMediaType[] getSupportedMimeTypes();

        /**
         * Returns a list of the content encodings supported by this serialization.
         * These content encodings apply to both the overall coding of the message
         * and to the encoding of individual elements.
         *
         * @return a list of the content encodings supported by this serialization.
         */
        public MimeMediaType[] getSupportedContentEncodings();

        /**
         * Create a WireFormatMessage from an abstract message. It is an error
         * (though lazily enforced) to modify the abstract message during the
         * lifetime of the WireFormatMessage.
         *
         * @param msg                     the message for which a serialization is desired.
         * @param type                    the the serialization form desired. This can include
         *                                mime parameters to control options.
         * @param preferedContentEncoding An array of acceptable message encodings
         *                                in descending order of preference. any or none of these encoding options
         *                                may be used. May be null for unencoded messages.
         * @return a proxy object for the abstract message which is a
         *         representation of the message in its serialized form.
         */
        public WireFormatMessage toWire(Message msg, MimeMediaType type, MimeMediaType[] preferedContentEncoding);

        /**
         * Create an abstract message from a serialization.
         *
         * @param is              The message stream. Message serializations must either use
         *                        internal data or EOF to determine the length of the stream.
         * @param type            Declared message type of the stream including any optional
         *                        configuration parameters.
         * @param contentEncoding Content encoding (including optional parameters)
         *                        which has been applied to the message. May be null for unencoded messages.
         * @return a proxy object for the abstract message which is a
         *         representation of the message in its serialized form.
         * @throws java.io.IOException if an io error occurs
         */
        public Message fromWire(InputStream is, MimeMediaType type, MimeMediaType contentEncoding) throws IOException;

        /**
         * Create an abstract message from a serialization.
         *
         * @param buffer          The byte buffer. Message serializations must either use
         *                        internal data or EOF to determine the length of the stream.
         * @param type            Declared message type of the stream including any optional
         *                        configuration parameters.
         * @param contentEncoding Content encoding (including optional parameters)
         *                        which has been applied to the message. May be null for unencoded messages.
         * @return a proxy object for the abstract message which is a
         *         representation of the message in its serialized form.
         * @throws java.io.IOException if an io error occurs
         */
        public Message fromBuffer(ByteBuffer buffer, MimeMediaType type, MimeMediaType contentEncoding) throws IOException;
    }

    /**
     * This is the map of mime-types and constructors used by
     * <CODE>newStructuredDocument</CODE>.
     */
    private Map<MimeMediaType,Instantiator> encodings = new Hashtable<MimeMediaType,Instantiator>();

    /**
     * If true then the pre-defined set of StructuredDocument sub-classes has
     * been registered from the property containing them.
     */
    private volatile boolean loadedProperty = false;

    /**
     * This class is in fact a singleton. This is the instance that backs the
     * static methods.
     */
    private static WireFormatMessageFactory factory = new WireFormatMessageFactory();

    /**
     * Private constructor. This class is not meant to be instantiated except
     * by itself.
     */
    private WireFormatMessageFactory() {}

    /**
     *  Registers the pre-defined set of WireFormatMessage sub-classes so that
     *  this factory can construct them.
     *
     *  @return true if at least one of the WireFormatMessage sub-classes could
     *  be registered otherwise false.
     */
    private synchronized boolean loadProviders() {
        if (!factory.loadedProperty) {
            factory.loadedProperty = registerProviders(WireFormatMessage.class.getName());
        }
        
        return factory.loadedProperty;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    protected Map<MimeMediaType,Instantiator> getAssocTable() {
        return encodings;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Class<Instantiator> getClassOfInstantiators() {
        // our key is the doctype names.
        return Instantiator.class;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Class getClassForKey() {
        // our key is the mime types.
        return MimeMediaType.class;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected boolean registerAssoc(String className) {
        boolean registeredSomething = false;

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Registering : " + className);
        }

        try {
            Class msgClass = Class.forName(className);

            Instantiator instantiator = (Instantiator) (msgClass.getField("INSTANTIATOR").get(null));

            MimeMediaType[] mimeTypes = instantiator.getSupportedMimeTypes();

            for (MimeMediaType mimeType : mimeTypes) {
                if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                    LOG.finer("   Registering Type : " + mimeType);
                }

                registeredSomething |= registerInstantiator(mimeType, instantiator);
            }
        } catch (Exception all) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to register \'" + className + "\'", all);
            }
        }

        return registeredSomething;
    }

    /**
     * Register an instantiator object a mime-type of documents to be
     * constructed.
     *
     * @param mimetype     the mime-type associated.
     * @param instantiator the instantiator that wants to be registered..
     * @return boolean true   if the instantiator for this mime-type is now
     *         registered. If there was already an instantiator this mime-type then
     *         false will be returned.
     * @throws SecurityException there were permission problems registering
     *                           the instantiator.
     */
    public static boolean registerInstantiator(MimeMediaType mimetype, Instantiator instantiator) {
        boolean registered = factory.registerAssoc(mimetype, instantiator);

        return registered;
    }

    /**
     * Constructs an instance of {@link WireFormatMessage} matching the type
     * specified by the <CODE>type</CODE> parameter.
     *
     * @param msg               the message for which a serialization is desired.
     * @param type              the the serialization form desired. This can include
     *                          mime parameters to control options.
     * @param preferedEncodings An array of acceptable message encodings
     *                          in descending order of preference. any or none of these encoding options
     *                          may be used. May be null for unencoded messages.
     * @return a proxy object for the abstract message which is a
     *         representation of the message in its serialized form.
     */
    public static WireFormatMessage toWire(Message msg, MimeMediaType type, MimeMediaType[] preferedEncodings) {
        factory.loadProviders();

        Instantiator instantiator = factory.getInstantiator(type.getBaseMimeMediaType());

        return instantiator.toWire(msg, type, preferedEncodings);
    }

    /**
     * Constructs an instance of <CODE>Message</CODE> from matching the type
     * specified by the <CODE>type</CODE> parameter.
     *
     * @param is              The message stream. Message serializations must either use
     *                        internal data or EOF to determine the length of the stream.
     * @param type            Declared message type of the stream including any optional
     *                        configuration parameters.
     * @param contentEncoding Content encoding (including optional parameters)
     *                        which has been applied to the message. May be null for unencoded messages.
     * @return the new abstract message.
     * @throws java.io.IOException if an io error occurs
     */
    public static Message fromWire(InputStream is, MimeMediaType type, MimeMediaType contentEncoding) throws IOException {
        factory.loadProviders();

        Instantiator instantiator;

        try {
            instantiator = factory.getInstantiator(type.getBaseMimeMediaType());
        } catch (NoSuchElementException badType) {
            throw new IOException("Unable to deserialize message of type: " + type);
        }

        return instantiator.fromWire(is, type, contentEncoding);
    }

    /**
     * Constructs an instance of <CODE>Message</CODE> from matching the type
     * specified by the <CODE>type</CODE> parameter.
     *
     * @param buffer          The message buffer. Message serializations must either use
     *                        internal data or EOF to determine the length of the stream.
     * @param type            Declared message type of the stream including any optional
     *                        configuration parameters.
     * @param contentEncoding Content encoding (including optional parameters)
     *                        which has been applied to the message. May be null for unencoded messages.
     * @return the new abstract message.
     * @throws java.io.IOException if an io error occurs
     */
    public static Message fromBuffer(ByteBuffer buffer, MimeMediaType type, MimeMediaType contentEncoding) throws IOException {
        factory.loadProviders();

        Instantiator instantiator;

        try {
            instantiator = factory.getInstantiator(type.getBaseMimeMediaType());
        } catch (NoSuchElementException badType) {
            throw new IOException("Unable to deserialize message of type: " + type);
        }

        return instantiator.fromBuffer(buffer, type, contentEncoding);
    }
}
