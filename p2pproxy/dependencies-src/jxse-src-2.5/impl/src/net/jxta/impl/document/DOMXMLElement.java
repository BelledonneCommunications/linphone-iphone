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

package net.jxta.impl.document;


import net.jxta.document.Attribute;
import net.jxta.document.XMLElement;

import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;


/**
 * This class represent an element of an XML document. XML Documents are formed
 * as a hierarchy of elements. Each element provides a proxy for DOM elements
 * and the text nodes containing values.
 */
public class DOMXMLElement implements XMLElement<DOMXMLElement> {

    protected DOMXMLDocument root;

    /**
     * The DOM node for which this element is a proxy.
     */
    protected Node domNode;

    /**
     * Constructor for associating a DOM node with a StructuredDocument Element.
     *
     * @param root the DOM not which is to be associated with this element.
     * @param node the DOM node
     */
    protected DOMXMLElement(DOMXMLDocument root, Node node) {
        this.root = root;
        domNode = node;
    }

    /**
     * Get the name associated with an element.
     *
     * @return A string containing the key of this element.
     */
    public String getKey() {
        return getName();
    }

    /**
     * Get the value (if any) associated with an element.
     *
     * @return A string containing the value of this element, if any, otherwise null.
     */
    public String getValue() {
        return getTextValue();
    }

    /**
     * Get the name associated with an element.
     *
     * @return A string containing the name of this element.
     */
    public String getName() {
        return getAssocNode().getNodeName();
    }

    /**
     * Get the value (if any) associated with an element.
     *
     * @return A string containing the value of this element, if any, otherwise null.
     */
    public String getTextValue() {
        StringBuilder itsValue = new StringBuilder();
        
        for (Node eachChild = getAssocNode().getFirstChild(); eachChild != null; eachChild = eachChild.getNextSibling()) {
            if (Node.TEXT_NODE == eachChild.getNodeType()) {
                itsValue.append(eachChild.getNodeValue());
            }
        }

        if (0 == itsValue.length()) {
            return null;
        } else {
            return itsValue.toString();
        }
    }

    /**
     * Get the root element of the hierarchy this element belongs to.
     *
     * @return StructuredDocument root of this element's hierarchy.
     */
    public DOMXMLDocument getRoot() {
        return root;
    }

    /**
     * Get the parent of this element. If the element has not been inserted into
     * the Document then null is returned. If this element is the root of the
     * Document then it returns itself.
     */
    public DOMXMLElement getParent() {
        Node node = getAssocNode();

        if (node.getOwnerDocument().equals(node)) {
            return new DOMXMLElement(root, node);
        } else {
            return new DOMXMLElement(root, node.getParentNode());
        }
    }

    /**
     * Add a child element to this element
     *
     * @param element the element to be added as a child
     */
    public void appendChild(DOMXMLElement element) {
        getAssocNode().appendChild(element.getAssocNode());
    }

    /**
     * Returns an enumeration of the immediate children of this element
     *
     * @return An enumeration containing all of the children of this element.
     */
    public Enumeration<DOMXMLElement> getChildren() {
        List<DOMXMLElement> children = new ArrayList<DOMXMLElement>();

        for (Node eachChild = getAssocNode().getFirstChild(); eachChild != null; eachChild = eachChild.getNextSibling()) {
            if (Node.ELEMENT_NODE == eachChild.getNodeType()) {
                children.add(new DOMXMLElement(root, eachChild));
            }
        }

        return Collections.enumeration(children);
    }

    /**
     * Returns an enumeration of the immediate children of this element whose
     * name match the specified string.
     *
     * @param key The key which will be matched against.
     * @return enumeration containing all of the children of this element.
     */
    public Enumeration<DOMXMLElement> getChildren(Object key) {
        if (key instanceof String)
            return getChildren((String) key);
        else
            throw new ClassCastException(key.getClass().getName() + " not supported by getChildren.");
    }

    /**
     * Returns an enumeration of the immediate children of this element whose
     * name match the specified string.
     *
     * @param name The name which will be matched against.
     * @return An enumeration containing all of the children of this element.
     */
    public Enumeration<DOMXMLElement> getChildren(String name) {
        List<DOMXMLElement> children = new ArrayList<DOMXMLElement>();

        for (Node eachChild = getAssocNode().getFirstChild(); eachChild != null; eachChild = eachChild.getNextSibling()) {
            if ((Node.ELEMENT_NODE == eachChild.getNodeType()) && (name.equals(eachChild.getNodeName()))) {
                children.add(new DOMXMLElement(root, eachChild));
            }
        }

        return Collections.enumeration(children);
    }

    /**
     * Tests two elements for equality. For the XML document the definition of
     * equality is:
     * <p/>
     * <p/><ul>
     * <li>the item compared against must be an XML Element.</li>
     * <p/>
     * <li>The items must belong to the same document.</li>
     * <p/>
     * <li>The items must have the same name.</li>
     * <p/>
     * <li>The items must have the save textual value.</li>
     * </ul>
     *
     * @param element the element to be compared against.
     * @return true if the elements are equal
     */
    @Override
    public boolean equals(Object element) {
        if (this == element) {
            return true;
        }

        if (!(element instanceof DOMXMLElement)) {
            return false;
        }

        DOMXMLElement xmlElement = (DOMXMLElement) element;

        Node me = getAssocNode();
        Node it = xmlElement.getAssocNode();

        if (me == it) {
            return true;
        }

        if (me.getOwnerDocument() != it.getOwnerDocument()) {
            return false;
        }

        if (!getName().equals(xmlElement.getName())) {
            return false;
        }

        String val1 = getTextValue();
        String val2 = xmlElement.getTextValue();

        return (null == val1) && (null == val2) || null != val1 && null != val2 && val1.equals(val2);

    }

    /**
     * Returns the DOM Node associated with this StructuredDocument element.
     *
     * @return Node    The DOM Node associated with this StructuredDocument element.
     */
    protected Node getAssocNode() {
        return domNode;
    }

    // Attributable methods

    /**
     * Adds an attribute with the given name and value. Some implementations
     * may support only a single value for each distinct name. Others may
     * support multiple values for each name. If the value being provided
     * replaces some other value then that value is returned otherwise null
     * is returned.
     *
     * @param name  name of the attribute.
     * @param value value for the attribute.
     * @return String  containing previous value for this name if the value
     *         is being replaced otherwise null.
     */
    public String addAttribute(String name, String value) {
        String oldAttrValue = ((Element) getAssocNode()).getAttribute(name);

        ((Element) getAssocNode()).setAttribute(name, value);
        return (0 == oldAttrValue.length()) ? null : oldAttrValue;
    }

    /**
     * Adds an attribute with the given name and value. Some implementations
     * may support only a single value for each distinct name. Others may
     * support multiple values for each name. If the value being provided
     * replaces some other value then that value is returned otherwise null
     * is returned.
     *
     * @param newAttrib new attribute.
     * @return String  containing previous value for this name if the value
     *         is being replaced otherwise null.
     */
    public String addAttribute(Attribute newAttrib) {
        String oldAttrValue = ((Element) getAssocNode()).getAttribute(newAttrib.getName());

        ((Element) getAssocNode()).setAttribute(newAttrib.getName(), newAttrib.getValue());
        return (0 == oldAttrValue.length()) ? null : oldAttrValue;
    }

    /**
     * Returns an enumerations of the attributes assosicated with this object.
     * Each element is of type Attribute.
     *
     * @return Enumeration the attributes associated with this object.
     */
    public Enumeration<Attribute> getAttributes() {
        NamedNodeMap nmap = getAssocNode().getAttributes();

        if (nmap == null) {
            List<Attribute> noAttrs = Collections.emptyList();

            return Collections.enumeration(noAttrs);
        }

        List<Attribute> attrs = new ArrayList<Attribute>();

        for (int i = 0; i < nmap.getLength(); i++) {
            Node domAttr = nmap.item(i);
            Attribute attr = new Attribute(this, domAttr.getNodeName(), domAttr.getNodeValue());

            attrs.add(attr);
        }

        return Collections.enumeration(attrs);
    }

    /**
     * returns a single attribute which matches the name provided. If no such
     * named attribute exists then null is returned. For impelementations of
     * this interface which support multiple values for each name only the
     * first value will be returned. To access all values for a name you must
     * use getAttributes.
     *
     * @return Attribute the attributes matching the given name.
     */
    public Attribute getAttribute(String name) {
        NamedNodeMap nmap = (getAssocNode()).getAttributes();

        if (nmap == null) {
            return null;
        }

        for (int i = 0; i < nmap.getLength(); i++) {
            Node domAttr = nmap.item(i);

            if (name.equals(domAttr.getNodeName())) {
                Attribute attr = new Attribute(this, domAttr.getNodeName(), domAttr.getNodeValue());

                return attr;
            }
        }

        return null;
    }
}
