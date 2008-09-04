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


/**
 * Extendable advertisements provide features for allowing inheritance of
 * advertisement types. The core and standard JXTA advertisements all allow
 * extension by extending this class.
 *
 * @see net.jxta.document.Advertisement
 * @see net.jxta.document.AdvertisementFactory
 * @see net.jxta.document.Document
 * @see net.jxta.document.MimeMediaType
 */
public abstract class ExtendableAdvertisement extends Advertisement {

    /**
     *  If {@code true} then the advertisement will be pretty print formatted
     *  when it is output as XML. Currently this must be specified at 
     *  construction time as it is normally a preference per advertisement type.
     */
    private transient boolean formatted;
    
    /**
     *  Default constructor. The advertisement will *not* be pretty-printed when
     *  it is output as XML.
     */
    protected ExtendableAdvertisement() {
        this(false);
    }
    
    /**
     *  @param formatted If {@code true} then the advertisement will be pretty 
     *  print formatted when it is output as XML.
     */
    protected ExtendableAdvertisement(boolean formatted) {
        this.formatted = formatted;
    }
    
    /**
     * Returns the base type of this advertisement hierarchy. Typically, only
     * the most basic advertisement of a type will implement this method and
     * declare it as <code>final</code>.
     *
     * @return String the base type of advertisements in this hierarchy.
     */
    public abstract String getBaseAdvType();

    /**
     * Process an individual element from the document during parse. Normally,
     * implementations will allow the base advertisements a chance to handle
     * the element before attempting to handle the element themselves. ie.
     * <p/>
     * <pre><code>
     *  protected boolean handleElement(Element elem) {
     *
     *      if (super.handleElement()) {
     *           // it's been handled.
     *           return true;
     *           }
     *      <i>... handle elements here ...</i>
     *
     *      // we don't know how to handle the element
     *      return false;
     *      }
     *  </code></pre>
     *
     * @param elem The element to be processed.
     * @return {@code true} if the element was recognized, otherwise false.
     */
    protected boolean handleElement(Element elem) {
        // we don't handle any elements.
        return false;
    }

    /**
     * Process an individual attribute from the document root node. Normally,
     * implementations will allow the base advertisements a chance to handle
     * the attribute before attempting to handle the element themselves. ie.
     * <p/>
     * <p/><pre><code>
     *  protected boolean handleAttribute( Attribute elem ) {
     * <p/>
     *      if (super.handleAttribute()) {
     *           // it's been handled.
     *           return true;
     *           }
     *
     *      <i>... handle attributes here ...</i>
     *
     *      // we don't know how to handle the attribute
     *      return false;
     *      }
     *  </code></pre>
     *
     * @param attrib The attribute to be processed.
     * @return {@code true} if the attribute was recognized, otherwise false.
     */
    protected boolean handleAttribute(Attribute attrib) {

        // "handle" the attributes that we choose to ignore.
        if ("xmlns:jxta".equals(attrib.getName())) {
            return true;
        } else if ("xml:space".equals(attrib.getName())) {
            return true;
        } else if ("type".equals(attrib.getName())) {
            return true;
        }

        return false;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * We don't have any content to add, just build the document instance and 
     * return it to implementations that actually do something with it.
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        Document adv = StructuredDocumentFactory.newStructuredDocument(encodeAs, getBaseAdvType());

        if (!(adv instanceof Attributable)) {
            throw new IllegalArgumentException("Advertisements require document type with attribute support.");
        }

        if (adv instanceof XMLDocument) {
            XMLDocument xmlAdv = (XMLDocument) adv;

            xmlAdv.addAttribute("xmlns:jxta", "http://jxta.org");
            if (!formatted) {
                xmlAdv.addAttribute("xml:space", "preserve");
            }
        }

        if (!getBaseAdvType().equals(getAdvType())) {
            ((Attributable) adv).addAttribute("type", getAdvType());
        }
        
        return adv;
    }
}
