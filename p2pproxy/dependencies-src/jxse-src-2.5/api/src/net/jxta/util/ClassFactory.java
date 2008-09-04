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

package net.jxta.util;


import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URL;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.ResourceBundle;
import java.util.logging.Level;
import java.util.logging.Logger;

import java.io.IOException;
import java.io.InputStream;
import java.net.URISyntaxException;
import java.util.Arrays;
import java.util.List;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

import net.jxta.logging.Logging;


/**
 * This util class provides methods needed by class construction factories.
 *
 * @see net.jxta.document.StructuredDocumentFactory
 * @see net.jxta.document.AdvertisementFactory
 * @see net.jxta.id.IDFactory
 * @see net.jxta.endpoint.WireFormatMessageFactory
 */
public abstract class ClassFactory<K, I> {

    /**
     *  Logger
     */
    private static final transient Logger LOG = Logger.getLogger(ClassFactory.class.getName());
        
    /**
     *  Standard constructor.
     */
    protected ClassFactory() {}

    /**
     *  Used by ClassFactory methods to get the mapping of keys to constructors.
     *
     *  @return the map containing the mappings.
     */
    protected abstract Map<K, I> getAssocTable();

    /**
     *  Used by ClassFactory methods to ensure that all keys used with the
     *  mapping are of the correct type.
     *
     *  @return Class object of the key type.
     */
    protected abstract Class<K> getClassForKey();

    /**
     *  Return all of the available keys for this factory.
     *
     *  @return Iterator of all the available keys for this factory.
     */
    public Iterator<K> getAvailableKeys() {
        return Collections.unmodifiableSet(getAssocTable().keySet()).iterator();
    }

    /**
     *  Returns an unmodifiable Set containing all of the associations 
     *  stored in this ClassFactory.
     *
     *  @return Set containing all of the available entries for this factory.
     */
    public Set<Map.Entry<K, I>> getEntrySet() {
        return Collections.unmodifiableSet(getAssocTable().entrySet());
    }

    /**
     *  Used by ClassFactory methods to ensure that all of the instance classes
     *  which register with this factory have the correct base class
     *
     *  @return Class object of the "Factory" type.
     */
    protected abstract Class<I> getClassOfInstantiators();

    /**
     *  Given a resource bundle identifier and a property name register instance
     *  classes. The property must be a string containing class names which must
     *  be found on the current class path. The class names are separated by
     *  spaces.
     *
     *  @param resourceName name of the resource bundle
     *  @param propertyName name of the property.
     *  @return boolean true if at least one instance class could be registered 
     *  with this factory.
     *  @exception MissingResourceException if the resource bundle or
     *  property cannot be located.
     */
    protected boolean registerFromResources(String resourceName, String propertyName) throws MissingResourceException {
        ResourceBundle jxtaRsrcs = ResourceBundle.getBundle(resourceName);
        String fromProps = jxtaRsrcs.getString(propertyName).trim();

        return registerFromString(fromProps);
    }

    /**
     *  Register instance classes given a string containing class names which
     *  must be found on the current class path. The class names are separated
     *  by spaces.
     *
     *  @param classNamesString The class name list
     *  @return boolean true if at least one of the instance classes could be
     *  registered otherwise false.
     */
    protected boolean registerFromString(String classNamesString) {
        boolean registeredSomething = false;

        if ((null == classNamesString) || (0 == classNamesString.length())) {
            return false;
        }

        // make sure the static initialisers for each instance class are called.
        List<String> instanceClasses = Arrays.asList(classNamesString.split("\\s"));
        
        for (String eachInstanceClass : instanceClasses) {
            try {
                registeredSomething |= registerAssoc(eachInstanceClass);
            } catch (Exception allElse) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.log(Level.WARNING, "Failed to register \'" + eachInstanceClass + "\'", allElse);
                }
            }
        }

        return registeredSomething;
    }

    /**
     * Given an provider interface name register instance classes.  The class
     * path is searched for service provider lists as described by the JAR File
     * specification for service providers
     *
     * @param interfaceName name of the implemented interface.
     * @return boolean true if at least one instance class could be registered
     *         with this factory.
     */
    protected boolean registerProviders(String interfaceName) {
        ClassLoader loader = getClass().getClassLoader();
        boolean registeredSomething = false;

        try {
            Enumeration<URL> providerLists = loader.getResources("META-INF/services/" + interfaceName);

            while (providerLists.hasMoreElements()) {
                try {
                    URI providerList = providerLists.nextElement().toURI();

                    registeredSomething |= registerFromFile(providerList);
                } catch (URISyntaxException badURI) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Failed to convert service URI", badURI);
                    }
                }
            }
        } catch (IOException ex) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Failed to locate provider lists", ex);
            }
        }

        return registeredSomething;
    }

    /**
     * Register instance classes given a URI to a file containing class names
     * which must be found on the current class path. The class names are listed
     * on separate lines.  Comments are marked with a '#', the pound sign and
     * any following text on any line in the file are ignored.
     *
     * @param providerList the URI to a file containing a list of providers
     * @return boolean true if at least one of the instance classes could be
     * registered otherwise false.
     */
    protected boolean registerFromFile(URI providerList) {
        boolean registeredSomething = false;
        InputStream urlStream = null;
        
        try {
            urlStream = providerList.toURL().openStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(urlStream, "UTF-8"));

            String provider;

            while ((provider = reader.readLine()) != null) {
                int comment = provider.indexOf('#');

                if (comment != -1) {
                    provider = provider.substring(0, comment);
                }
                    
                provider = provider.trim();

                if (provider.length() > 0) {
                    try {
                        registeredSomething |= registerAssoc(provider);
                    } catch (Exception allElse) {
                        if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                            LOG.log(Level.WARNING, "Failed to register \'" + provider + "\'", allElse);
                        }
                    }
                }
            }
        } catch (IOException ex) {
            LOG.log(Level.WARNING, "Failed to read provider list " + providerList, ex);
            return false;
        } finally {
            if(null != urlStream) {
                try {
                    urlStream.close();
                } catch(IOException ignored) {
                    
                }
            }
        }

        return registeredSomething;
    }

    /**
     *  Register a class with the factory from its class name. Since class name
     *  doesn't tell us much, we just load the class and hope that something
     *  happens as a result of the class loading. This class is often overridden
     *  in class factories to interogate the instance class before registering
     *  the instance class.
     *
     *  @param className The class name which will be registered.
     *  @return boolean true if the class was registered otherwise false.
     *  @throws Exception   when an error occurs.
     */
    protected boolean registerAssoc(final String className) throws Exception {

        boolean registeredSomething = false;

        try {
            /*
             * This implementation skankily assumes that the class registers 
             * itself as part of class initialization.
             */
                        
            Class ignored = Class.forName(className);
                        
            registeredSomething = true;
        } catch (ClassNotFoundException ignored) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Failed to locate \'" + className + "\'");
            }
        } catch (NoClassDefFoundError ignored) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Failed to locate \'" + className + "\'");
            }
        }

        return registeredSomething;
    }

    /**
     *  Register a key and instance class with the factory.
     *
     *  @param key The key to register.
     *  @param instantiator The instantiator object which will be registered for this key.
     *  @return boolean true if the key was successfully registered otherwise false.
     */
    protected boolean registerAssoc(final K key, final I instantiator) {

        // Check the association table to make sure this key is not already present.
        if (null != getAssocTable().get(key)) {
            return false;
        }

        getAssocTable().put(key, instantiator);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Factory : " + getClass().getName() + " Registered instantiator \'" + instantiator + "\' for \'" + key + "\'");
        }

        return true;
    }

    /**
     *  Return the instantiator associated with the provided key.
     *
     *  @param key The identifier for the Instantiator class to be returned.
     *  @return Instantiator Instantiator matching the provided key
     *  @throws NoSuchElementException if the key has not been registered.
     */
    protected I getInstantiator(final K key) throws NoSuchElementException {

        // Get the constructors for this key.
        I instantiator = getAssocTable().get(key);

        if (null == instantiator) {
            throw new NoSuchElementException("key '" + key + "' not registered.");
        }

        return instantiator;
    }
}
