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
 *  An element represents a portion of a {@link StructuredDocument}. An element
 *  is identifiable by a <code>key</code> and may also optionally have a
 *  <code>value</code>. Each Element also maintains a collection of references
 *  to other elements, its <emphasis>children</emphasis>. Elements can be
 *  composed into arbitrary hierarchical structures forming complex data
 *  structures.
 *
 *  <p/>Element instances are always associated with a
 *  {@link StructuredDocument}. A {@link StructuredDocument} is a specialized
 *  form of Element with additional features that make it appropriate for
 *  acting as the root of a hierarchy of elements.
 *
 *  @see net.jxta.document.Document
 *  @see net.jxta.document.StructuredDocument
 *  @see net.jxta.document.StructuredDocumentFactory
 *  @see net.jxta.document.StructuredTextDocument
 *  @see net.jxta.document.TextElement
 */
public interface Element<E extends Element<E>> {
    
    /**
     * Get the key associated with this Element.
     *
     * @return The key of this Element.
     */
    Object getKey();
    
    /**
     * Get the value (if any) associated with this Element.
     *
     * @return The value of this element, if any, otherwise null.
     */
    Object getValue();
    
    /**
     *  Get the root document element of the hierarchy this element belongs to.
     *
     *  @return The root document element of this element's hierarchy.
     */
    StructuredDocument getRoot();
    
    /**
     * Get the parent element of this element. If this Element has not been
     * inserted into the Document then <code>null</code> is returned. If this
     * element is the root element of the Document then it returns itself. ie.,
     * <code>this == this.getParent()</code>.
     *
     * @return The parent of this element. If the element has no parent
     * then null will be returned. If the element is the root Element of the
     * hierarchy then it will return itself.
     */
    E getParent();
    
    /**
     *  Add a child element to this element. The child element must be from the
     *  document as the element it is to be added to. Elements are created using
     *  either {@link StructuredDocument#createElement(Object)} or
     *  {@link StructuredDocument#createElement(Object, Object)}.
     *
     *  @param element The element to be added as a child.
     */
    void appendChild(E element);
    
    /**
     *  Returns an enumeration of the immediate children of this element.
     *
     *  @return Enumeration containing all of the children of this element.
     */
    Enumeration<E> getChildren();
    
    /**
     *  Returns an enumeration of the immediate children of this element who
     *  match the specified key.
     *
     *  @param key The key which will be matched against.
     *  @return Enumeration containing all of the matching children of this 
     *  element.
     */
    Enumeration<E> getChildren(Object key);
}
