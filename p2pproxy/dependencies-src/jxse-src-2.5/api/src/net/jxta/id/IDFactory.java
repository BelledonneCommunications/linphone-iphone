/*
 *  Copyright (c) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.id;


import java.io.InputStream;
import java.lang.reflect.Field;
import java.net.URI;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;
import java.util.ResourceBundle;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.UnknownServiceException;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.codat.CodatID;
import net.jxta.id.jxta.IDFormat;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.pipe.PipeID;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.util.ClassFactory;


/**
 *  A factory class for creating new ID instances and for constructing ID
 *  instances from external representations such as strings or URIs.
 *
 *  <p/>When possible the factory will create IDs of the same ID Format as any
 *  base ids provided. For example, PipeIDs will be created to be the same ID
 *  Format as the PeerGroupID provided.
 *
 *  <p/>Some ID constructors allow specification of "seed" information. Each ID
 *  Format may use this seed information as it chooses or may require seed
 *  information of a specific form. In some cases the seed information will be
 *  used literally as provided to construct the resulting ID, but ID Formats
 *  may also choose to ignore the seed information entirely, use it as random
 *  number generator seed values, etc. Consult the implementation documentation
 *  for the ID Formats of interest to see how the seed information is used by
 *  each ID Format.
 *
 *  @see net.jxta.id.ID
 *  @see net.jxta.util.ClassFactory
 *  @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#ids" target='_blank'>JXTA Protocols Specification : IDs</a>
 */
public final class IDFactory extends ClassFactory<String, IDFactory.Instantiator> {
    
    /**
     *  Logger
     */
    private static final transient Logger LOG = Logger.getLogger(IDFactory.class.getName());
    
    /**
     *  A map of the ID Formats to instantiators.
     *
     */
    private final Map<String, Instantiator> idFormats = new HashMap<String, Instantiator>();
    
    /**
     *  Identifies the ID format to use when creating new ID instances.
     */
    private final String idNewInstances;
    
    /**
     *  This class is a singleton. This is the instance that backs the
     *  static methods.
     */
    private final static IDFactory factory = new IDFactory();
    
    /**
     *  Interface for instantiators of IDs. Each ID Format registered with the
     *  ID Factory implements a class with this interface.
     */
    public interface Instantiator {
        
        /**
         *  Returns the ID Format value associated with this ID Format
         *
         *  @return String containing the ID format value for this format.
         */
        public String getSupportedIDFormat();
        
        /**
         *  Construct a new ID instance from a JXTA ID contained in a URL.
         *
         *  @deprecated Convert to {@code fromURI}.
         *
         *  @param source  URL which will be decoded to create a new ID instance.
         *  @return  ID containing the new ID instance initialized from the URL.
         *  @throws UnknownServiceException Is thrown if the URL provided is of
         *  a format unrecognized by this JXTA implementation.
         *  @throws MalformedURLException Is thrown if the URL provided is not
         *  a valid, recognized JXTA URL.
         */
        @Deprecated
        public ID fromURL(URL source) throws MalformedURLException, UnknownServiceException;
        
        /**
         *  Construct a new ID instance from a JXTA ID contained in a URI.
         *
         *  @param source  URI which will be decoded to create a new ID instance.
         *  @return  ID containing the new ID instance initialized from the source.
         *  @throws URISyntaxException if the URI provided is not a valid,
         *  recognized JXTA URI.
         */
        public ID fromURI(URI source) throws URISyntaxException;
        
        /**
         *  Construct a new ID instance from the scheme specific portion of a jxta
         *  URN.
         *
         *  @param source  the scheme specific portion of a jxta URN.
         *  @return  ID containing the new ID instance initialized from the source.
         *  @throws URISyntaxException if the URI provided is not a valid,
         *  recognized JXTA URI.
         */
        public ID fromURNNamespaceSpecificPart(String source) throws URISyntaxException;

        /**
         *  Creates a new CodatID Instance. A new random CodatID is created for
         *  the provided Peer Group. This type of CodatID can be used as a
         *  canonical reference for dynamic content.
         *
         *  @see net.jxta.codat.Codat
         *
         *  @param groupID The group to which this content will belong.
         *  @return The newly created CodatID.
         */
        public CodatID newCodatID(PeerGroupID groupID);
        
        /**
         *  Creates a new CodatID instance. A new CodatID is created for the
         *  provided Peer Group. This type of CodatID can be used as a
         *  canonical reference for dynamic content.
         *
         *  <p/>This variant of CodatID allows you to create "Well-known" codats
         *  within the context of diverse groups. This can be useful for common
         *  services that need to do discovery without advertisements or for
         *  network organization services.  Because of the potential for ID
         *  collisions and the difficulties with maintaining common service
         *  interfaces this variant of CodatID should be used with great caution
         *  and pre-planning.
         *
         *  @see net.jxta.codat.Codat
         *
         *  @param groupID The group to which this content will belong.
         *  @param seed The seed information which will be used in creating the
         *  codatID. The seed information should be at least four bytes in
         *  length, though longer values are better.
         *  @return The newly created CodatID.
         */
        public CodatID newCodatID(PeerGroupID groupID, byte[] seed);
        
        /**
         *  Creates a new CodatID instance. A new random CodatID is created for
         *  the provided Peer Group and contains a hash value for the Codat data.
         *  This type of Codat ID is most appropriate for static content. By
         *  including a hash value this form of Codat ID provides greater
         *  assurance of the canonical property of IDs. It also allows the
         *  document content returned when this ID is used to be verified to
         *  ensure it has not been altered.
         *
         *  @see net.jxta.codat.Codat
         *
         *  @param groupID The group to which this ID will belong.
         *  @param in The InputStream from which the content hash is calculated.
         *  The stream is read until EOF and then closed.
         *  @return The newly created CodatID.
         *  @throws IOException I/O Error reading document
         */
        public CodatID newCodatID(PeerGroupID groupID, InputStream in) throws IOException;
        
        /**
         *  Creates a new CodatID instance. A new CodatID is created for the
         *  provided Peer Group and contains a hash value for the Codat data.
         *  By including a hash value this form of Codat ID provides greater
         *  assurance of the canonical property of IDs. It also allows the
         *  document content returned when this ID is used to be verified to
         *  ensure it has not been altered. This type of Codat ID is most
         *  appropriate for static content.
         *
         *  <p/>This variant of CodatID allows you to create "Well-known" codats
         *  within the context of diverse groups. This can be useful for common
         *  services that need to do discovery without advertisements or for
         *  network organization services.  Because of the potential for ID
         *  collisions and the difficulties with maintaining common service
         *  interfaces this variant of CodatID should be used with great caution
         *  and pre-planning.
         *
         *  @see net.jxta.codat.Codat
         *
         *  @param groupID The group to which this ID will belong.
         *  @param seed The seed information which will be used in creating the
         *  codat ID. The seed information should be at least four bytes in
         *  length, though longer values are better.
         *  @param in The InputStream from which the content hash is calculated.
         *  The stream is read until EOF and then closed.
         *  @return The newly created CodatID.
         *  @throws IOException I/O Error reading document
         */
        public CodatID newCodatID(PeerGroupID groupID, byte[] seed, InputStream in) throws IOException;
        
        /**
         *  Creates a new PeerID instance. A new random peer id will be generated.
         *  The PeerID will be a member of the provided group.
         *
         *  @see net.jxta.peergroup.PeerGroup
         *
         *  @param groupID    the group to which this PeerID will belong.
         *  @return The newly created PeerID.
         */
        public PeerID newPeerID(PeerGroupID groupID);
        
        /**
         *  Creates a new PeerID instance. A new PeerID will be generated.
         *  The PeerID will be a member of the provided group.
         *
         *  @see net.jxta.peergroup.PeerGroup
         *
         *  @param groupID    the group to which this PeerID will belong.
         *  @param seed   The seed information which will be used in creating the
         *  PeerID. The seed information should be at least four bytes in length,
         *  though longer values are better.
         *  @return The newly created PeerID.
         */
        public PeerID newPeerID(PeerGroupID groupID, byte[] seed);
        
        /**
         *  Creates a new PeerGroupID instance. A new random peer group id will be
         *  generated. The PeerGroupID will be created using the default ID Format.
         *
         *  @see net.jxta.peergroup.PeerGroup
         *
         *  @return The newly created PeerGroupID.
         */
        public PeerGroupID newPeerGroupID();
        
        /**
         *  Creates a new PeerGroupID instance. A new PeerGroupID will be
         *  generated using the provided seed information. The PeerGroupID will
         *  be created using the default ID Format.
         *
         *  <p/>This method allows you to create "Well-known" PeerGroupIDs.
         *  This is similar to how the JXTA "World Peer Group" and "Net
         *  Peer Group". "Well-known" IDs can be useful for common services
         *  that need to do  discovery without advertisements or for network
         *  organization  services. Because of the potential for ID collisions
         *  and the difficulties with maintaining common service interfaces this
         *  variant of PeerGroupID should be used with great caution and
         *  pre-planning.
         *
         *  @see net.jxta.peergroup.PeerGroup
         *
         *  @param seed The seed information which will be used in creating the
         *  PeerGroupID. The seed information should be at least four bytes in
         *  length, though longer values are better.
         *  @return The newly created PeerGroupID.
         */
        public PeerGroupID newPeerGroupID(byte[] seed);
        
        /**
         *  Creates a new PeerGroupID instance with the specified parent group.
         *  A new random peer group id will be generated.
         *
         *  @see net.jxta.peergroup.PeerGroup
         *
         *  @param parent The group which will be the parent of this group.
         *  @return The newly created PeerGroupID.
         */
        public PeerGroupID newPeerGroupID(PeerGroupID parent);
        
        /**
         *  Creates a new PeerGroupID instance with the specified parent group.
         *  A new PeerGroupID will be generated using the provided seed
         *  information.
         *
         *  <p/>This method allows you to create "Well-known" PeerGroupIDs.
         *  This is similar to how the JXTA "World Peer Group" and "Net
         *  Peer Group". "Well-known" IDs can be useful for common services
         *  that need to do  discovery without advertisements or for network
         *  organization  services. Because of the potential for ID collisions
         *  and the difficulties with maintaining common service interfaces this
         *  variant of PeerGroupID should be used with great caution and
         *  pre-planning.
         *
         *  @see net.jxta.peergroup.PeerGroup
         *
         *  @param parent    The group which will be the parent of this group.
         *  @param seed The seed information which will be used in creating the
         *  PeerGroupID. The seed information should be at least four bytes in
         *  length, though longer values are better.
         *  @return The newly created PeerGroupID.
         */
        public PeerGroupID newPeerGroupID(PeerGroupID parent, byte[] seed);
        
        /**
         *  Creates a new PipeID instance. A new random PipeID will be generated.
         *
         *
         *  @param groupID  The group to which this Pipe ID will belong.
         *  @return The newly created PipeID.
         */
        public PipeID newPipeID(PeerGroupID groupID);
        
        /**
         *  Creates a new PipeID instance. A new pipe id will be generated with
         *  the provided seed information. The Pipe ID will be a member of the
         *  provided group.
         *
         *  <p/>This variant of PipeID allows you to create "Well-known" pipes
         *  within the context of diverse groups. This can be useful for common
         *  services that need to do discovery without advertisements or for
         *  network organization services.  Because of the potential for ID
         *  collisions and the difficulties with maintaining common service
         *  interfaces this variant of PipeID should be used with great caution
         *  and pre-planning.
         *
         *
         *  @param groupID  the group to which this Pipe ID will belong.
         *  @param seed The seed information which will be used in creating the
         *  pipeID. The seed information should be at least four bytes in
         *  length, though longer values are better.
         *  @return the newly created PipeID.
         */
        public PipeID newPipeID(PeerGroupID groupID, byte[] seed);
        
        /**
         *  Creates a new ModuleClassID instance. A new random ModuleClassID
         *  will be generated with a zero value role identifier. This form of
         *  ModuleClassID is appropriate for cases where the module does not
         *  need to be distinguished from other instances of the same Module.
         *  The ModuleClassID will be created using the default ID Format.
         *
         *  @see net.jxta.platform.Module
         *
         *  @return The newly created ModuleClassID.
         */
        public ModuleClassID newModuleClassID();
        
        /**
         *  Creates a new ModuleClassID instance. A new random ModuleClassID
         *  will be generated with a a random value role identifier and a base
         *  class of the provided ModuleClassID. This form of ModuleClassID is
         *  appropriate for cases where it is necessary to distinguish instances
         *  of the same service interface.
         *
         *  @see net.jxta.platform.Module
         *
         *  @param  baseClass   The ModuleClassID which will be used as a base
         *  class for this new role value instance.
         *  @return The newly created ModuleClassID.
         */
        public ModuleClassID newModuleClassID(ModuleClassID baseClass);
        
        /**
         *  Creates a new  ModuleSpecID instance. A new random ModuleSpecID will
         *  be generated.
         *
         *  @see net.jxta.platform.Module
         *
         *  @param baseClass   The ModuleClassID which will be used as a base
         *  class for this new ModuleSpecID.
         *  @return The newly created ModuleSpecID.
         */
        public ModuleSpecID newModuleSpecID(ModuleClassID baseClass);
    }
    

    /**
     * @deprecated This interface formerly contained optional URI based
     * construction methods. These have now been moved to the primary
     * instantiator interface in preparation for the removal of the URL
     * based interfaces. This interface will be removed in a future release.
     */
    @Deprecated
    public interface URIInstantiator extends Instantiator {}
    
    /**
     *  Standard Constructor. This class is a singleton so the only constructor
     *  is private.
     *
     *  <p/>Uses net.jxta.impl.config.properties file as the
     *  source for settings.
     *
     * <p/>Example entry from  the file net.jxta.impl.config.properties :
     *
     * <p/><pre><code>
     * #Default type of ID to use when creating an ID (this should not be changed in most implementations).
     * IDNewInstances=uuid
     * </code></pre>
     */
    private IDFactory() {
        // required format
        registerAssoc("net.jxta.id.jxta.IDFormat");
        
        // required by this implementation.
        registerAssoc("net.jxta.impl.id.unknown.IDFormat");
        
        // Register a list of classes for association with an ID format
        registerProviders(ID.class.getName());
        
        try {
            // Get our resource bundle
            ResourceBundle jxtaRsrcs = ResourceBundle.getBundle("net.jxta.impl.config");
            
            // set the default ID Format.
            idNewInstances = jxtaRsrcs.getString("IDNewInstances").trim();
        } catch (MissingResourceException notFound) {
            // This is an error because we can't start without a concept of ID.
            IllegalStateException failure =
		new IllegalStateException("Could not initialize ID defaults", notFound);
            LOG.log(Level.SEVERE, "Cound not initialize IDFactory", failure);
            throw failure;
        }
    }
    
    /**
     *  Used by ClassFactory methods to get the mapping of ID types to constructors.
     *
     *  @return the mapping of ID types to instantiators.
     */
    @Override
    protected Map<String, Instantiator> getAssocTable() {
        return idFormats;
    }
    
    /**
     *  Used by ClassFactory methods to ensure that all keys used with the mapping are
     *  of the correct type.
     *
     *  @return Class object of the key type.
     */
    @Override
    protected Class<String> getClassForKey() {
        return String.class;
    }
    
    /**
     *  Used by ClassFactory methods to ensure that all of the instance classes
     *  which register with this factory have the correct base class
     *
     *  @return Class object of the key type.
     */
    @Override
    protected Class<Instantiator> getClassOfInstantiators() {
        return Instantiator.class;
    }
    
    /**
     *  Register a class with the factory from its class name. We override the
     *  standard implementation to get the id format from the class and
     *  use that as the key to register the class with the factory.
     *
     * @param className The class name which will be registered.
     * @return boolean true if the class was registered otherwise false.
     */
    @Override
    public boolean registerAssoc(String className) {
        boolean registeredSomething = false;
        
        try {
            Class<?> idClass;

            try {
                idClass = Class.forName(className);
                
                if (null == idClass) {
                    throw new ClassNotFoundException("forName() result was null");
                }
            } catch (ClassNotFoundException notThere) {
                LOG.severe("Could not find class named : " + className);
                return false;
            } catch (NoClassDefFoundError notThere) {
                LOG.severe("Could not find class named : " + className);
                return false;
            }
            
            Field instantiatorField;

            try {
                instantiatorField = idClass.getField("INSTANTIATOR");
                
                if (null == instantiatorField) {
                    throw new NoSuchFieldException("getField() result was null for field 'INSTANTIATOR'");
                    // caught locally
                }
            } catch (NoSuchFieldException notThere) {
                LOG.severe("Could not find INSTANTIATOR field in class named : " + className);
                return false;
            }
            
            if (!Instantiator.class.isAssignableFrom(instantiatorField.getType())) {
                throw new ClassCastException("INSTANTIATOR is not of type " + Instantiator.class.getName());
            }
            
            Instantiator instantiator = (Instantiator) instantiatorField.get(null);
            
            if (null == instantiator) {
                LOG.severe("INSTANTIATOR field is null for class  : " + className);
                return false;
            }
            
            String idFormat = instantiator.getSupportedIDFormat();
            
            registeredSomething = registerAssoc(idFormat, instantiator);
        } catch (Exception failed) {
            LOG.log(Level.SEVERE, "Failed to register class : " + className, failed);
        }
        
        return registeredSomething;
    }
    
    /**
     *  Returns a String containing the name of the default ID Format.
     *
     *  @return The current default ID Format.
     */
    public static String getDefaultIDFormat() {
        return factory.idNewInstances;
    }
    
    /**
     *  Construct a new ID instance from a JXTA ID contained in a URI.
     *
     *  @param source  URI which will be decoded to create a new ID instance.
     *  @return  ID containing the new ID instance initialized from the URI.
     *  @throws URISyntaxException If the URI provided is not a valid,
     *  recognized JXTA URI.
     */
    public static ID fromURI(URI source) throws URISyntaxException {
        ID result = null;
        
        // check the protocol
        if (!ID.URIEncodingName.equalsIgnoreCase(source.getScheme())) {
            throw new URISyntaxException(source.toString(), "URI scheme was not as expected.");
        }
        
        String decoded = source.getSchemeSpecificPart();
        
        int colonAt = decoded.indexOf(':');
        
        // There's a colon right?
        if (-1 == colonAt) {
            throw new URISyntaxException(source.toString(), "URN namespace was missing.");
        }
        
        // check the namespace
        if (!net.jxta.id.ID.URNNamespace.equalsIgnoreCase(decoded.substring(0, colonAt))) {
            throw new URISyntaxException(source.toString(),
                    "URN namespace was not as expected. (" +
                    net.jxta.id.ID.URNNamespace + "!=" + decoded.substring(0, colonAt) + ")");
        }
        
        // skip the namespace portion and the colon
        decoded = decoded.substring(colonAt + 1);
        
        int dashAt = decoded.indexOf('-');
        
        // there's a dash, right?
        if (-1 == dashAt) {
            throw new URISyntaxException(source.toString(), "URN jxta namespace IDFormat was missing.");
        }
        
        // get the encoding used for this id
        String format = decoded.substring(0, dashAt);
        
        Instantiator instantiator;

        try {
            instantiator = factory.getInstantiator(format);
        } catch (NoSuchElementException itsUnknown) {
            instantiator = factory.getInstantiator("unknown");
        }
        
        result = instantiator.fromURNNamespaceSpecificPart(decoded);
        
        return result.intern();
    }
    
    /**
     *  Construct a new ID instance from a JXTA ID contained in a URI.
     *
     *  @deprecated Use of URLs for representing JXTA IDs and this method are
     *  deprecated. Convert to using {@link #fromURI( URI )} instead.
     *
     *  @param source  URI which will be decoded to create a new ID instance.
     *  @return  ID containing the new ID instance initialized from the URI.
     *  @throws UnknownServiceException Is thrown if the URI provided is of a
     *  format unrecognized by this JXTA implementation.
     *  @throws MalformedURLException Is thrown if the URI provided is not
     *  a valid, recognized JXTA URI.
     */
    @Deprecated
    public static ID fromURL(URL source) throws MalformedURLException, UnknownServiceException {
        
        ID result = null;
        
        // check the protocol
        if (!ID.URIEncodingName.equalsIgnoreCase(source.getProtocol())) {
            throw new UnknownServiceException("URI protocol type was not as expected.");
        }
        
        String encoded = source.getFile();
        
        // Decode the URN to convert any % encodings and convert it from UTF8.
        String decoded = sun.net.www.protocol.urn.Handler.decodeURN(encoded);
        
        int colonAt = decoded.indexOf(':');
        
        // There's a colon right?
        if (-1 == colonAt) {
            throw new MalformedURLException("URN namespace was missing.");
        }
        
        // check the namespace
        if (!net.jxta.id.ID.URNNamespace.equalsIgnoreCase(decoded.substring(0, colonAt))) {
            throw new MalformedURLException(
                    "URN namespace was not as expected. (" + net.jxta.id.ID.URNNamespace + "!=" + decoded.substring(0, colonAt)
                    + ")");
        }
        
        // skip the namespace portion and the colon
        decoded = decoded.substring(colonAt + 1);
        
        int dashAt = decoded.indexOf('-');
        
        // there's a dash, right?
        if (-1 == dashAt) {
            throw new UnknownServiceException("URN Encodingtype was missing.");
        }
        
        // get the encoding used for this id
        decoded = decoded.substring(0, dashAt);
        
        Instantiator instantiator;

        try {
            instantiator = factory.getInstantiator(decoded);
        } catch (NoSuchElementException itsUnknown) {
            instantiator = factory.getInstantiator("unknown");
        }
        
        result = instantiator.fromURL(source);
        
        return result.intern();
    }
    
    /**
     *  Creates a new CodatID Instance. A new random CodatID is created for
     *  the provided Peer Group. This type of CodatID can be used as a
     *  canonical reference for dynamic content.
     *
     *  @see net.jxta.codat.Codat
     *
     *  @param groupID    the group to which this content will belong.
     *  @return The newly created CodatID.
     */
    public static CodatID newCodatID(PeerGroupID groupID) {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newCodatID(groupID).intern();
    }
    
    /**
     *  Creates a new CodatID instance. A new CodatID is created for the
     *  provided Peer Group. This type of CodatID can be used as a
     *  canonical reference for dynamic content.
     *
     *  <p/>This variant of CodatID allows you to create "Well-known" codats
     *  within the context of diverse groups. This can be useful for common
     *  services that need to do discovery without advertisements or for
     *  network organization services.  Because of the potential for ID
     *  collisions and the difficulties with maintaining common service
     *  interfaces this variant of CodatID should be used with great caution
     *  and pre-planning.
     *
     *  @see net.jxta.codat.Codat
     *
     *  @param groupID    the group to which this content will belong.
     *  @param seed The seed information which will be used in creating the
     *  codatID. The seed information should be at least four bytes in length,
     *  though longer values are better.
     *  @return The newly created CodatID.
     */
    public static CodatID newCodatID(PeerGroupID groupID, byte[] seed) {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newCodatID(groupID, seed).intern();
    }
    
    /**
     *  Creates a new CodatID instance. A new random CodatID is created for
     *  the provided Peer Group and contains a hash value for the Codat data.
     *  This type of Codat ID is most appropriate for static content. By
     *  including a hash value this form of Codat ID provides greater assurance
     *  of the canonical property of IDs. It also allows the document content
     *  returned when this ID is used to be verified to ensure it has not been
     *   altered.
     *
     *  @see net.jxta.codat.Codat
     *
     *  @param  groupID The group to which this ID will belong.
     *  @param  in  The InputStream from which the content hash is calculated.
     *  The stream is read until EOF and then closed.
     *  @return The newly created CodatID.
     *  @throws IOException I/O Error reading document
     */
    public static CodatID newCodatID(PeerGroupID groupID, InputStream in) throws  IOException {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newCodatID(groupID, in).intern();
    }
    
    /**
     *  Creates a new CodatID instance. A new CodatID is created for the
     *  provided Peer Group and contains a hash value for the Codat data.
     *  By including a hash value this form of Codat ID provides greater
     *  assurance of the canonical property of IDs. It also allows the
     *  document content returned when this ID is used to be verified to
     *  ensure it has not been altered. This type of Codat ID is most
     *  appropriate for static content.
     *
     *  <p/>This variant of CodatID allows you to create "Well-known" codats
     *  within the context of diverse groups. This can be useful for common
     *  services that need to do discovery without advertisements or for
     *  network organization services.  Because of the potential for ID
     *  collisions and the difficulties with maintaining common service
     *  interfaces this variant of CodatID should be used with great caution
     *  and pre-planning.
     *
     *  @see net.jxta.codat.Codat
     *
     *  @param  groupID The group to which this ID will belong.
     *  @param seed The seed information which will be used in creating the
     *  codat ID. The seed information should be at least four bytes in length,
     *  though longer values are better.
     *  @param  in  The InputStream from which the content hash is calculated.
     *  The stream is read until EOF and then closed.
     *  @return The newly created CodatID.
     *  @throws IOException I/O Error reading document
     */
    public static CodatID newCodatID(PeerGroupID groupID, byte[] seed, InputStream in) throws  IOException {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newCodatID(groupID, seed, in).intern();
    }
    
    /**
     *  Creates a new PeerID instance. A new random peer id will be generated.
     *  The PeerID will be a member of the provided group.
     *
     *  @see net.jxta.peergroup.PeerGroup
     *
     *  @param groupID    the group to which this PeerID will belong.
     *  @return The newly created PeerID.
     */
    public static PeerID newPeerID(PeerGroupID groupID) {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newPeerID(groupID).intern();
    }
    
    /**
     *  Creates a new PeerID instance. A new PeerID will be generated.
     *  The PeerID will be a member of the provided group.
     *
     *  @see net.jxta.peergroup.PeerGroup
     *
     *  @param groupID    the group to which this PeerID will belong.
     *  @param seed The seed information which will be used in creating the
     *  PeerID. The seed information should be at least four bytes in length,
     *  though longer values are better.
     *  @return The newly created PeerID.
     */
    public static PeerID newPeerID(PeerGroupID groupID, byte[] seed) {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newPeerID(groupID, seed).intern();
    }
    
    /**
     *  Creates a new PeerGroupID instance. A new random peer group id will be
     *  generated. The PeerGroupID will be created using the default ID Format.
     *
     *  @see net.jxta.peergroup.PeerGroup
     *
     *  @return The newly created PeerGroupID.
     */
    public static PeerGroupID newPeerGroupID() {
        return newPeerGroupID(factory.idNewInstances).intern();
    }
    
    /**
     *  Creates a new PeerGroupID instance using the specified ID Format.
     *  A new random peer group id will be generated.
     *
     * @see net.jxta.peergroup.PeerGroup
     *
     * @param idformat The ID Format to be used for crating the Peer Group ID.
     * @return The newly created PeerGroupID.
     */
    public static PeerGroupID newPeerGroupID(String idformat) {
        Instantiator instantiator = factory.getInstantiator(idformat);
        
        return instantiator.newPeerGroupID().intern();
    }
    
    /**
     *  Creates a new PeerGroupID instance. A new PeerGroupID will be
     *  generated using the provided seed information. The PeerGroupID will
     *  be created using the default ID Format.
     *
     *  <p/>This method allows you to create "Well-known" PeerGroupIDs.
     *  This is similar to how the JXTA "World Peer Group" and "Net
     *  Peer Group". "Well-known" IDs can be useful for common services
     *  that need to do  discovery without advertisements or for network
     *  organization  services. Because of the potential for ID collisions
     *  and the difficulties with maintaining common service interfaces this
     *  variant of PeerGroupID should be used with great caution and
     *  pre-planning.
     *
     *  @see net.jxta.peergroup.PeerGroup
     *
     *  @param seed The seed information which will be used in creating the
     *  PeerGroupID. The seed information should be at least four bytes in length,
     *  though longer values are better.
     *  @return The newly created PeerGroupID.
     */
    public static PeerGroupID newPeerGroupID(byte[] seed) {
        return newPeerGroupID(factory.idNewInstances, seed).intern();
    }
    
    /**
     *  Creates a new PeerGroupID instance. A new PeerGroupID will be
     *  generated using the provided seed information. The PeerGroupID will
     *  be created using the default ID Format.
     *
     *  <p/>This method allows you to create "Well-known" PeerGroupIDs.
     *  This is similar to how the JXTA "World Peer Group" and "Net
     *  Peer Group". "Well-known" IDs can be useful for common services
     *  that need to do  discovery without advertisements or for network
     *  organization  services. Because of the potential for ID collisions
     *  and the difficulties with maintaining common service interfaces this
     *  variant of PeerGroupID should be used with great caution and
     *  pre-planning.
     *
     * @see net.jxta.peergroup.PeerGroup
     *
     * @param idformat The ID Format of the new Peer Group ID.
     * @param seed The seed information which will be used in creating the
     *  PeerGroupID. The seed information should be at least four bytes in length,
     *  though longer values are better.
     * @return The newly created PeerGroupID.
     */
    public static PeerGroupID newPeerGroupID(String idformat, byte[] seed) {
        Instantiator instantiator = factory.getInstantiator(idformat);
        
        return instantiator.newPeerGroupID(seed).intern();
    }
    
    /**
     *  Creates a new PeerGroupID instance with the specified parent group.
     *  A new random peer group id will be generated.
     *
     *  @see net.jxta.peergroup.PeerGroup
     *
     *  @param parent    The group which will be the parent of this group.
     *  @return The newly created PeerGroupID.
     */
    public static PeerGroupID newPeerGroupID(PeerGroupID parent) {
        String useFormat = parent.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newPeerGroupID(parent).intern();
    }
    
    /**
     *  Creates a new PeerGroupID instance with the specified parent group.
     *  A new PeerGroupID will be generated using the provided seed
     *  information.
     *
     *  <p/>This method allows you to create "Well-known" PeerGroupIDs.
     *  This is similar to how the JXTA "World Peer Group" and "Net
     *  Peer Group". "Well-known" IDs can be useful for common services
     *  that need to do  discovery without advertisements or for network
     *  organization  services. Because of the potential for ID collisions
     *  and the difficulties with maintaining common service interfaces this
     *  variant of PeerGroupID should be used with great caution and
     *  pre-planning.
     *
     *  @see net.jxta.peergroup.PeerGroup
     *
     *  @param parent    The group which will be the parent of this group.
     *  @param seed The seed information which will be used in creating the
     *  PeerGroupID. The seed information should be at least four bytes in length,
     *  though longer values are better.
     *  @return The newly created PeerGroupID.
     */
    public static PeerGroupID newPeerGroupID(PeerGroupID parent, byte[] seed) {
        String useFormat = parent.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newPeerGroupID(parent, seed).intern();
    }
    
    /**
     *  Creates a new PipeID instance. A new random PipeID will be generated.
     *
     *  @param groupID  The group to which this Pipe ID will belong.
     *  @return The newly created PipeID.
     */
    public static PipeID newPipeID(PeerGroupID groupID) {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newPipeID(groupID).intern();
    }
    
    /**
     *  Creates a new PipeID instance. A new pipe id will be generated with the
     *  provided seed information. The Pipe ID will be a member of the provided
     *  group.
     *
     *  <p/>This variant of PipeID allows you to create "Well-known" pipes
     *  within the context of diverse groups. This can be useful for common
     *  services that need to do discovery without advertisements or for
     *  network organization services.  Because of the potential for ID
     *  collisions and the difficulties with maintaining common service
     *  interfaces this variant of PipeID should be used with great caution
     *  and pre-planning.
     *
     *  @param groupID  the group to which this Pipe ID will belong.
     *  @param seed The seed information which will be used in creating the
     *  pipeID. The seed information should be at least four bytes in length,
     *  though longer values are better.
     *  @return the newly created PipeID.
     */
    public static PipeID newPipeID(PeerGroupID groupID, byte[] seed) {
        String useFormat = groupID.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newPipeID(groupID, seed).intern();
    }
    
    /**
     *  Creates a new ModuleClassID instance. A new random ModuleClassID will
     *  be generated with a zero value role identifier. This form of
     *  ModuleClassID is appropriate for cases where the module does not
     *  need to be distinguished from other instances of the same Module.
     *  The ModuleClassID will be created using the default ID Format.
     *
     *  @see net.jxta.platform.Module
     *
     *  @return The newly created ModuleClassID.
     */
    public static ModuleClassID newModuleClassID() {
        return newModuleClassID(factory.idNewInstances).intern();
    }
    
    /**
     *  Creates a new ModuleClassID instance using the specified ID Format.
     *  A new random ModuleClassID will be generated with a zero value role
     *  identifier. This form of ModuleClassID is appropriate for cases
     *  where the module does not need to be distinguished from other
     *  instances of the same Module.
     *
     * @see net.jxta.platform.Module
     *
     * @param idformat The ID Format of the new ModuleClassID.
     * @return The newly created ModuleClassID.
     */
    public static ModuleClassID newModuleClassID(String idformat) {
        Instantiator instantiator = factory.getInstantiator(idformat);
        
        return instantiator.newModuleClassID().intern();
    }
    
    /**
     *  Creates a new ModuleClassID instance. A new random ModuleClassID will
     *  be generated with a a random value role identifier and a base class of
     *  the provided ModuleClassID. This form of ModuleClassID is
     *  appropriate for cases where it is necessary to distinguish instances
     *  of the same service interface.
     *
     *  @see net.jxta.platform.Module
     *
     *  @param  baseClass   The ModuleClassID which will be used as a base
     *  class for this new role value instance.
     *  @return The newly created ModuleClassID.
     */
    public static ModuleClassID newModuleClassID(ModuleClassID baseClass) {
        String useFormat = baseClass.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newModuleClassID(baseClass).intern();
    }
    
    /**
     *  Creates a new  ModuleSpecID instance. A new random ModuleSpecID will
     *  be generated.
     *
     *  @see net.jxta.platform.Module
     *
     *  @param baseClass   The ModuleClassID which will be used as a base
     *  class for this new ModuleSpecID.
     *  @return The newly created ModuleSpecID.
     */
    public static ModuleSpecID newModuleSpecID(ModuleClassID baseClass) {
        String useFormat = baseClass.getIDFormat();
        
        // is the group netpg or worldpg?
        if (IDFormat.INSTANTIATOR.getSupportedIDFormat().equals(useFormat)) {
            useFormat = factory.idNewInstances;
        }
        
        Instantiator instantiator = factory.getInstantiator(useFormat);
        
        return instantiator.newModuleSpecID(baseClass).intern();
    }
    
    /**
     *  This method should be used instead of using
     *  {@code new java.net.URL( )} to create URLs for use with IDFactory.
     *  URL construction can cause classes to be loaded using the system
     *  classLoader (JXTA IDs require the class
     *  {@code sun.net.www.protocol.urn.Handler} for Sun JVMs).
     *  This class loading will fail in many environments such as web-servers,
     *  servlet containers, application servers, or java web start where a user
     *  class loader is used to load all JXTA resources.
     *
     *  @deprecated You should convert code which creates JXTA IDs to instead
     *  use {@code IDFactory.fromURI( new URI(...) )}. This method was only
     *  provided to overcome problems with registration of URL handlers in
     *  foreign class loader environments (Servlets, Applets, JNLP, etc.).
     *
     *  @param protocol   The protocol for this URL
     *  @param host       The host for this URL
     *  @param file       The file for this URL
     *  @return a newly created URL for the resource specified.
     *  @throws  MalformedURLException  if an unknown protocol is specified.
     */
    @Deprecated
    public static URL jxtaURL(String protocol, String host, String file) throws MalformedURLException {
        return new URL(protocol, host, -1, file, sun.net.www.protocol.urn.Handler.handler);
    }
    
    /**
     *  This method should be used instead of using
     *  {@code new java.net.URL( )} to create URLs for use with IDFactory.
     *  URL construction can cause classes to be loaded using the system
     *  classLoader (JXTA IDs require the class
     *  {@code sun.net.www.protocol.urn.Handler} for Sun JVMs).
     *  This class loading will fail in many environments such as web-servers,
     *  servlet containers, application servers, or java web start where a user
     *  class loader is used to load all JXTA resources.
     *
     *  @deprecated You should convert code which creates JXTA IDs from strings
     *  to instead use {IDFactory.fromURI( new URI(String) )}. This
     *  method was only provided to overcome problems with registration of URL
     *  handlers in foreign class loader environments (Servlets, Applets, JNLP,
     *  etc.).
     *
     *  @param uri The {@code String} to parse as a URL.
     *  @return a newly created URL for the resource specified.
     *  @throws  MalformedURLException  if an unknown protocol is specified.
     */
    @Deprecated
    public static URL jxtaURL(String uri) throws MalformedURLException {
        final String file = net.jxta.id.ID.URNNamespace + ":";
        URL urlCnxt = jxtaURL(net.jxta.id.ID.URIEncodingName, "", file);
        
        return new URL(urlCnxt, uri);
    }
}
