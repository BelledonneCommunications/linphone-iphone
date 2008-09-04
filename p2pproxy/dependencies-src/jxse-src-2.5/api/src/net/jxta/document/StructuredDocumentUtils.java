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


import java.util.Enumeration;


/**
 *  Provides a number of static utility members which are helpful in
 *  manipluating StructuredDocuments.
 *
 **/
public final class StructuredDocumentUtils {
    
    /**
     *  A singleton class, not meant to be constructed
     **/
    private StructuredDocumentUtils() {
        ;
    }
    
    /**
     * Recursively copy children elements of <code>from</code> into the
     * the element <code>intoElement</code> of document <code>intoDoc</code>.
     *
     * <p/><b>BEWARE</b> that this does NOT copy the TEXTVALUE (if any) of the
     * <code>fromParent</code> element, ONLY CHILDREN. All other elements
     * are fully copied, including their textValue.
     *
     * <p/>It is not possible to copy a textValue in an existing element.
     *
     * @param intoDoc  the document into which the elements will be
     * copied.
     * @param intoElement  the element which will serve as the parent for
     * the elements being copied.
     * @param from the parent element of the elements which will be copied.
     **/
    public static void copyChildren(StructuredDocument intoDoc, Element intoElement, Element from) {
        
        for (Enumeration eachChild = from.getChildren(); eachChild.hasMoreElements();) {
            
            Element aChild = (Element) eachChild.nextElement();
            Element newElement = intoDoc.createElement(aChild.getKey(), aChild.getValue());

            intoElement.appendChild(newElement);
            
            // copy attributes if any
            if ((aChild instanceof Attributable) && (newElement instanceof Attributable)) {
                Enumeration eachAttrib = ((Attributable) aChild).getAttributes();
                
                while (eachAttrib.hasMoreElements()) {
                    Attribute anAttrib = (Attribute) eachAttrib.nextElement();
                    
                    ((Attributable) newElement).addAttribute(anAttrib.getName(), anAttrib.getValue());
                }
            }
            
            // recurse to add the children.
            copyChildren(intoDoc, newElement, aChild);
        }
    }
    
    /**
     *  Recursively copy elements beginnging with <code>from</code> into the
     *  document identified by <code>intoDoc</code>.
     *
     *  @param intoDoc  the document into which the elements which will be
     *  copied.
     *  @param intoElement  the element which will serve as the parent for
     *  the elements being copied.
     *  @param from the root element of the hierarchy which will be copied.
     *  @param newName the root element being copied is renamed
     *  <em>newName</em>.
     *  @return The added element.
     *
     **/
    public static Element copyElements(StructuredDocument intoDoc, Element intoElement, Element from, Object newName) {
        
        Element newElement = intoDoc.createElement(newName, from.getValue());
        
        intoElement.appendChild(newElement);
        
        boolean hasType = false;

        // copy attributes if any
        if (newElement instanceof Attributable) {

            if (from instanceof Attributable) {
                Enumeration eachAttrib = ((Attributable) from).getAttributes();
            
                while (eachAttrib.hasMoreElements()) {
                    Attribute anAttrib = (Attribute) eachAttrib.nextElement();
                    String attribName = anAttrib.getName();

                    if (attribName.equals("type")) {
                        hasType = true;
                    }
                    ((Attributable) newElement).addAttribute(attribName, anAttrib.getValue());
                }
            }

            // If "from" happens to be a document, and if it happens to be renamed, and if it does not have an explicit type
            // attribute, then preserve the document type, which we assume is the original name, converted to string, as a type
            // attribute. It is an XMLism. It may be wrong or at least ineffective for other kinds of structured documents. If
            // it one day becomes an issue, it will have to be resolved by having a method specific to each kind of structured
            // document and dedicated to correcting type loss.

            Object origName = from.getKey().toString();

            if ((!hasType) && (!newName.equals(origName)) && (from instanceof Document)) {
                ((Attributable) newElement).addAttribute("type", origName.toString());
            }
        }

        StructuredDocumentUtils.copyChildren(intoDoc, newElement, from);
        
        return newElement;
    }
    
    /**
     *  Recursively copy elements beginnging with <code>from</code> into the
     *  document identified by <code>intoDoc</code>.
     *
     *  @param intoDoc  the document into which the elements which will be
     *  copied.
     *  @param intoElement  the element which will serve as the parent for
     *  the elements being copied.
     *  @param from the root element of the hierarchy which will be copied.
     *  @return The added element.
     **/
    public static Element copyElements(StructuredDocument intoDoc, Element intoElement, Element from) {
        
        return copyElements(intoDoc, intoElement, from, from.getKey());
    }
    
    /**
     * Copies the specified element or document into a standalone document of
     * same type. The <code>from</code element's name is used as the document
     * type. All child elements are recursively copied.
     *
     * @param from the root element from which to begin copying.
     * @return StructuredDocument the copy
     **/
    public static StructuredDocument copyAsDocument(Element from) {
        
        StructuredDocument result;
        Object value = from.getValue();
        
        if (value == null) {
            result = StructuredDocumentFactory.newStructuredDocument(from.getRoot().getMimeType(), from.getKey().toString());
        } else {
            result = StructuredDocumentFactory.newStructuredDocument(from.getRoot().getMimeType(), from.getKey().toString()
                    ,
                    value.toString());
            value = null;
        }
        
        // copy attributes if any
        if ((from instanceof Attributable) && (result instanceof Attributable)) {
            Enumeration eachAttrib = ((Attributable) from).getAttributes();
            
            while (eachAttrib.hasMoreElements()) {
                Attribute anAttrib = (Attribute) eachAttrib.nextElement();
                
                ((Attributable) result).addAttribute(anAttrib.getName(), anAttrib.getValue());
            }
        }
        
        StructuredDocumentUtils.copyChildren(result, result, from);
        
        return result;
    }
}
