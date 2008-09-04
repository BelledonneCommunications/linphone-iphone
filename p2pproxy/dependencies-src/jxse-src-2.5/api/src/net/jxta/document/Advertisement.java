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


import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.UndeclaredThrowableException;
import java.lang.reflect.Method;

import net.jxta.id.ID;


/**
 *  Advertisements are core JXTA objects that are used to advertise Peers,
 *  PeerGroups, Services, Pipes or other JXTA resources. Advertisements provide
 *  a platform independent representation of core platform objects that can be
 *  exchanged between different platform implementations (Java, C, etc.).
 *
 *  <p>Each Advertisement holds a document that represents the advertisement.
 *  Advertisements are typically represented as a text document (XML). The
 *  {@link Advertisement#getDocument(MimeMediaType) getDocument(mimetype)}
 *  method is used to generate representations of the advertisement. Different
 *  representations are available via mime type selection. Typical mime types
 *  are "text/xml" or "text/plain" that generate textual representations for the
 *  Advertisements.
 *
 *  <p>Advertisements are created via {@link AdvertisementFactory} rather than 
 *  through use of constructors. This is done because public the Advertisement 
 *  sub-classes are typically abstract. The actual implementations are provided 
 *  by private sub-classes.
 *
 *  @see net.jxta.document.AdvertisementFactory
 *  @see net.jxta.document.ExtendableAdvertisement
 *  @see net.jxta.id.ID
 *  @see net.jxta.document.Document
 *  @see net.jxta.document.MimeMediaType
 */
public abstract class Advertisement {
    
    /**
     * {@inheritDoc}
     */
    @Override
    public Advertisement clone() throws CloneNotSupportedException {
        return (Advertisement) super.clone();
    }
    
    /**
     * Return a string representation of this advertisement. The string will
     * contain the advertisement pretty-print formatted as a UTF-8 encoded XML
     * Document.
     *
     * @return A String containing the advertisement.
     */
    @Override
    public String toString() {        
        XMLDocument doc = (XMLDocument) getDocument(MimeMediaType.XMLUTF8);
        
        // Force pretty printing
        doc.addAttribute("xml:space", "default");
            
        return doc.toString();
    }
    
    /**
     *  Returns the identifying type of this Advertisement.
     *
     *  <p/><b>Note:</b> This is a static method. It cannot be used to determine
     *  the runtime type of an advertisement. ie.
     *  </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     *
     *  <p/><b>This is wrong and does not work the way you might expect.</b>
     *  This call is not polymorphic and calls
     *  {@code Advertisement.getAdvertisementType()} no matter what the real
     *  type of the advertisement.
     *
     * @return The type of advertisement.
     */
    public static String getAdvertisementType() {
        throw new UnsupportedOperationException(
                "Advertisement : sub-class failed to override getAdvertisementType. getAdvertisementType() is static and is *not* polymorphic.");
    }
    
    /**
     *  Returns the identifying type of this Advertisement. Unlike
     *  {@link #getAdvertisementType()} this method will return the correct
     *  runtime type of an Advertisement object.
     *  <p/>
     *  This implementation is provided for existing advertisements which do not
     *  provide their own implementation. In most cases you should provide your
     *  own implementation for efficiency reasons.
     *
     *  @since JXSE 2.1.1
     *  @return The identifying type of this Advertisement.
     */
    public String getAdvType() {
        try {
            Method getAdvertisementTypeMethod = this.getClass().getMethod("getAdvertisementType", (Class[]) null);
            String result = (String) getAdvertisementTypeMethod.invoke(null, (Object[]) null);
            
            return result;
        } catch (NoSuchMethodException failed) {
            UnsupportedOperationException failure = new UnsupportedOperationException("Could not get Advertisement type.");

            failure.initCause(failed);
            throw failure;
        } catch (IllegalAccessException failed) {
            SecurityException failure = new SecurityException("Could not get Advertisement type.");

            failure.initCause(failed);
            throw failure;
        } catch (InvocationTargetException failed) {
            UndeclaredThrowableException failure = new UndeclaredThrowableException(failed, "Failed getting Advertisement type.");

            failure.initCause(failed.getCause());
            throw failure;
        }
    }
    
    /**
     *  Write this advertisement into a document of the requested type. Two 
     *  standard document forms are defined. <code>"text/plain"</code> encodes 
     *  the document in a "pretty-print" format for human viewing and 
     *  <code>"text/xml"<code> which provides an XML format.
     *
     *  @param asMimeType MimeMediaType format representation requested.
     *  @return The {@code Advertisement} represented as a {@code Document} of
     *  the requested MIME Media Type.
     */
    public abstract Document getDocument(MimeMediaType asMimeType);
    
    /**
     *  Returns an ID which identifies this {@code Advertisement} as uniquely as 
     *  possible. This ID is typically used as the primary key for indexing of
     *  the Advertisement within databases. 
     *  <p/>
     *  Each advertisement sub-class must choose an appropriate implementation
     *  which returns canonical and relatively unique ID values for it's
     *  instances. Since this ID is commonly used for indexing, the IDs returned
     *  must be as unique as possible to avoid collisions. The value for the ID 
     *  returned can either be:
     *  <p/>
     *  <ul>
     *      <li>An ID which is already part of the advertisement definition
     *      and is relatively unique between advertisements instances. For
     *      example, the Peer Advertisement returns the Peer ID.</li>
     *
     *      <li>A static CodatID which is generated via some canonical process
     *      which will produce the same value each time and different values for
     *      different advertisements of the same type.</li>
     *
     *      <li>ID.nullID for advertisement types which are not readily indexed.
     *      </li>
     *  </ul>
     *  <p/>
     *  For Advertisement types which normally return non-ID.nullID values
     *  no ID should be returned when asked to generate an ID while the
     *  Advertisement is an inconsistent state (example: uninitialized index
     *  fields). Instead {@link java.lang.IllegalStateException} should be
     *  thrown.
     *
     *  @return An ID that relatively uniquely identifies this advertisement 
     *  or {@code ID.nullID} if this advertisement is of a type that is not 
     *  normally indexed.
     */
    public abstract ID getID();
    
    /**
     * Returns the element names on which this advertisement should be indexed.
     *
     * @return The element names on which this advertisement should be indexed.
     */
    public abstract String[] getIndexFields();
}
