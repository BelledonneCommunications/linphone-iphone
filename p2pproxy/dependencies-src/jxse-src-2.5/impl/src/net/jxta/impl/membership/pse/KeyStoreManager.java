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

package net.jxta.impl.membership.pse;


import java.io.IOException;
import java.security.KeyStore;
import java.security.KeyStoreException;


/**
 * Abstracts the management of KeyStores. This is commonly used to abstract the
 * location of the KeyStore and the details of creating, loading, saving and
 * deleting the KeyStore.
 * <p/>
 * <p/>Applications should not assume that accesses to KeyStoreManager are
 * thread safe. All access to the KeyStoreManager should be externally
 * synchronized on the KeyStoreManager object.
 */
public interface KeyStoreManager {

    /**
     * Returns <tt>true</tt> if the KeyStore has been initialized (created).
     * Since this method does not provide a passphrase it is really only useful
     * for determining if a new KeyStore needs to be created.
     *
     * @return <tt>true</tt> if the KeyStore has been previously initialized
     *         otherwise <tt>false</tt>.
     * @throws KeyStoreException If the KeyStore is protected by a store
     *                           password that has not been set.
     */
    boolean isInitialized() throws KeyStoreException;

    /**
     * Returns <tt>true</tt> if the Keystore has been initialized (created).
     * This method also ensures that the provided passphrase is valid for the
     * keystore.
     *
     * @param password The KeyStore passphrase.
     * @return <tt>true</tt> if the Keystore has been initialized otherwise
     *         <tt>false</tt>.
     * @throws KeyStoreException If an incorrect KeyStore password is provided.
     */
    boolean isInitialized(char[] password) throws KeyStoreException;

    /**
     * Create the KeyStore using the specified KeyStore passphrase.
     *
     * @param password The KeyStore passphrase.
     * @throws KeyStoreException If an incorrect KeyStore passphrase is provided.
     * @throws IOException       If there is a problem creating the KeyStore.
     */
    void createKeyStore(char[] password) throws IOException, KeyStoreException;

    /**
     * Load the KeyStore.
     *
     * @param password The KeyStore passphrase.
     * @throws KeyStoreException If an incorrect KeyStore password is provided.
     * @throws IOException       If there is a problem loading the KeyStore.
     * @return the keystore
     */
    KeyStore loadKeyStore(char[] password) throws IOException, KeyStoreException;

    /**
     * Save the provided KeyStore using the specified KeyStore passphrase.
     *
     * @param store    The KeyStore to save.
     * @param password The encryption passphrase for the keystore.
     * @throws IOException       Thrown for errors writing the keystore.
     * @throws KeyStoreException Thrown for errors with the provided key or key store.
     */
    void saveKeyStore(KeyStore store, char[] password) throws IOException, KeyStoreException;

    /**
     * Erase the KeyStore. Some KeyStore implementations may not allow the
     * KeyStore container itself to be erased and in some cases specific
     * certificates and keys may be unerasable. All implementations should
     * erase all user provided certificates and keys.
     *
     * @throws IOException If there is a problem erasing the KeyStore or the
     *                     KeyStore cannot be erased.
     */
    void eraseKeyStore() throws IOException;
}
