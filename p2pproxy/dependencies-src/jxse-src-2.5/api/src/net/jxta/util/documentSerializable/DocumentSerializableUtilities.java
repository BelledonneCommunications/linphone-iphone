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

package net.jxta.util.documentSerializable;


import net.jxta.document.*;
import net.jxta.exception.*;
import net.jxta.util.*;

import java.util.*;
import java.io.*;


/**
 **/
public class DocumentSerializableUtilities {
    // Fix-Me: I didn't implement byte, short or float
    // Fix-Me: I didn't implement arrays, ie addInt(Element element, String tagName, int values[]), etc

    /**
     * Creates a Structured XML Document containing the serialized object
     *
     * @return	The created Document
     * @throws DocumentSerializationException if Unable to parse the serialized object.
     */	
    public static XMLDocument createStructuredXmlDocument(String docType, DocumentSerializable documentSerializable) throws DocumentSerializationException {
        XMLDocument xmlDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, docType);

        documentSerializable.serializeTo(xmlDoc);
        return xmlDoc;
    }

    /**
     * Deeply copy an element into another element
     *
     * @param toElement The target Element
     * @param fromElement The source Element
     */	
    public static void copyChildren(Element toElement, Element fromElement) {
        // for now ... a quicky use of another utility

        StructuredDocument intoDoc = toElement.getRoot();

        StructuredDocumentUtils.copyChildren(intoDoc, toElement, fromElement);
    }

    /**
     *  Add an Element with the specified tagname and value (converted to a String)
     *
     * @param element Parent Element that the new child element will be added to
     * @param tagName TagName to be used for the created Child Element
     * @param documentSerializable This value will be serialized into the created child element
     \ 	 * @throws DocumentSerializationException if Unable to serialized object.
     **/
    public static void addDocumentSerializable(Element element, String tagName, DocumentSerializable documentSerializable)  throws DocumentSerializationException {
        StructuredDocument structuredDocument = element.getRoot();
        Element childElement = structuredDocument.createElement(tagName);

        element.appendChild(childElement);
        documentSerializable.serializeTo(childElement);
    }

    /**
     *  Create an object from its Document Serialized components
     *
     * @param element The relative root element of a Document Serialized Object
     * @param clazz The Class of the resurrected object (must implement DocumentSerializable and have a public no-arg constructor)
     * @return An object of type 'clazz'
     * @throws DocumentSerializationException if Unable to parse the serialized object.
     **/
    public static DocumentSerializable getDocumentSerializable(Element element, Class clazz) throws DocumentSerializationException {
        try {
            return getDocumentSerializable(element, (DocumentSerializable) clazz.newInstance());
        } catch (DocumentSerializationException e) {
            throw e;
        } catch (Exception e) {
            throw new DocumentSerializationException("Class must have a public no-arg constructor", e);
        }
    }
	
    /**
     *  Initialize an object from its Document Serialized components
     *
     * @param element The relative root element of a Document Serialized Object
     * @param documentSerializable The object that will be populated from the Element
     * @return The same parameter passed to it 'documentSerializable'
     * @throws DocumentSerializationException if Unable to parse the serialized object.
     **/
    public static DocumentSerializable getDocumentSerializable(Element element, DocumentSerializable documentSerializable) throws DocumentSerializationException {
        documentSerializable.initializeFrom(element);
        return documentSerializable;
    }

    /**
     *  Create an object from its Document Serialized components
     *
     * @param element The Parent element which has a child Element with the serialized value
     * @param tagName The tagname of the element that contains the relative root element of a Document Serialized Object
     * @param clazz The Class of the resurrected object (must implement DocumentSerializable and have a public no-arg constructor)
     * @return An object of type 'clazz'
     * @throws DocumentSerializationException if Unable to parse the serialized object.
     **/
    public static DocumentSerializable getDocumentSerializable(Element element, String tagName, Class clazz) throws DocumentSerializationException {
        Element childElement = getChildElement(element, tagName);
		
        if (childElement != null) {
            return getDocumentSerializable(childElement, clazz);
        } else {
            return null;
        }
    }	

    /**
     *  Create a copy of any Document Serializable object.
     *  
     *  This is done by serializing and then deserializing the object (ie not very efficient)
     *
     * @param documentSerializable The Object to be copied
     * @return An copy of the presented object
     * @throws DocumentSerializationException if Unable to serialize or parse object.
     **/
    public static DocumentSerializable copyDocumentSerializable(DocumentSerializable documentSerializable) throws DocumentSerializationException {
        StructuredDocument structuredDocument = createStructuredXmlDocument("temp", documentSerializable);

        return getDocumentSerializable(structuredDocument, documentSerializable.getClass());
    }

    /**
     *  Create a child element of the specified tagName
     *  
     *  This is done by serializing and then deserializing the object (ie not very efficient)
     *
     * @param element The Parent Element
     * @param tagName The Tag Name for the new Element
     * @return The created Element
     **/
    public static Element createChildElement(Element element, String tagName) {
        StructuredDocument structuredDocument = element.getRoot();
        Element childElement = structuredDocument.createElement(tagName);

        element.appendChild(childElement);
        return childElement;
    }

    /**
     *  Get a child element of the specified tagName
     *  
     *  This is done by serializing and then deserializing the object (ie not very efficient)
     *
     * @param element The Parent Element
     * @param tagName The Tag Name for the new Element
     * @return The found Element
     **/
    public static Element getChildElement(Element element, String tagName) {
        Enumeration e = element.getChildren(tagName);
		
        if (e.hasMoreElements()) {
            return (Element) e.nextElement();
        } else {
            return null;
        }
    }

    /**
     *  Add an Element with the specified tagname and value (converted to a String)
     *
     * @param element Parent Element that the new element will be added to
     * @param tagName TagName to be used for the created Child Element
     * @param value The value that will be stored in the Element as a String
     **/	 
    public static void addInt(Element element, String tagName, int value) {
        StructuredDocument structuredDocument = element.getRoot();
        Element childElement = structuredDocument.createElement(tagName, Integer.toString(value));

        element.appendChild(childElement);
    }

    /**
     *  Get  the value of an element converted from a String
     *
     * @param element Element that contains the value
     * @return the value converted from a String
     **/
    public static int getInt(Element element) {
        return Integer.parseInt((String) element.getValue());
    }
	
    /**
     *  Get the value of a Child Element 
     *
     * @param element The Parant Element
     * @param tagName The Tag Name of the Child Element that will contain the value
     * @param defaultValue The return value if there is no Child Element with that Tag Name
     * @return the value converted from a String
     **/
    public static int getInt(Element element, String tagName, int defaultValue) {
        Element childElement = getChildElement(element, tagName);
		
        if (childElement != null) {
            return getInt(childElement);
        } else {
            return defaultValue;
        }
    }	
		
    /**
     *  Add an Element with the specified tagname and value (converted to a String)
     *
     * @param element Parent Element that the new element will be added to
     * @param tagName TagName to be used for the created Child Element
     * @param value The value that will be stored in the Element as a String
     **/
    public static void addLong(Element element, String tagName, long value) {
        StructuredDocument structuredDocument = element.getRoot();
        Element childElement = structuredDocument.createElement(tagName, Long.toString(value));

        element.appendChild(childElement);
    }

    /**
     *  Get  the value of an element converted from a String
     *
     * @param element Element that contains the value
     * @return the value converted from a String
     **/
    public static long getLong(Element element) {
        return Long.parseLong((String) element.getValue());
    }

    /**
     *  Get the value of a Child Element 
     *
     * @param element The Parant Element
     * @param tagName The Tag Name of the Child Element that will contain the value
     * @param defaultValue The return value if there is no Child Element with that Tag Name
     * @return the value converted from a String
     **/
    public static long getLong(Element element, String tagName, long defaultValue) {
        Element childElement = getChildElement(element, tagName);
		
        if (childElement != null) {
            return getLong(childElement);
        } else {
            return defaultValue;
        }
    }	
		
    /**
     *  Add an Element with the specified tagname and value (converted to a String)
     *
     * @param element Parent Element that the new element will be added to
     * @param tagName TagName to be used for the created Child Element
     * @param value The value that will be stored in the Element as a String
     **/
    public static void addDouble(Element element, String tagName, double value) {
        StructuredDocument structuredDocument = element.getRoot();
        Element childElement = structuredDocument.createElement(tagName, Double.toString(value));

        element.appendChild(childElement);
    }

    /**
     *  Get  the value of an element converted from a String
     *
     * @param element Element that contains the value
     * @return the value converted from a String
     **/
    public static double getDouble(Element element) {
        return Double.parseDouble((String) element.getValue());
    }

    /**
     *  Get the value of a Child Element 
     *
     * @param element The Parant Element
     * @param tagName The Tag Name of the Child Element that will contain the value
     * @param defaultValue The return value if there is no Child Element with that Tag Name
     * @return the value converted from a String
     **/
    public static double getDouble(Element element, String tagName, double defaultValue) {
        Element childElement = getChildElement(element, tagName);
		
        if (childElement != null) {
            return getDouble(childElement);
        } else {
            return defaultValue;
        }
    }	
		
    /**
     *  Add an Element with the specified tagname and value (converted to a String)
     *
     * @param element Parent Element that the new element will be added to
     * @param tagName TagName to be used for the created Child Element
     * @param value The value that will be stored in the Element as a String
     **/
    public static void addBoolean(Element element, String tagName, boolean value) {
        StructuredDocument structuredDocument = element.getRoot();
        Element childElement = structuredDocument.createElement(tagName, value ? "true" : "false");

        element.appendChild(childElement);
    }

    /**
     *  Get  the value of an element converted from a String ("true" or "false")
     *
     * @param element Element that contains the value
     * @return the value converted from a String
     **/
    public static boolean getBoolean(Element element) {
        return "true".equals((String) element.getValue());
    }

    /**
     *  Get the value of a Child Element 
     *
     * @param element The Parant Element
     * @param tagName The Tag Name of the Child Element that will contain the value
     * @param defaultValue The return value if there is no Child Element with that Tag Name
     * @return the value converted from a String
     **/
    public static boolean getBoolean(Element element, String tagName, boolean defaultValue) {
        Element childElement = getChildElement(element, tagName);
		
        if (childElement != null) {
            return getBoolean(childElement);
        } else {
            return defaultValue;
        }
    }	

    /**
     *  Add an Element with the specified tagname and value
     *
     * @param element Parent Element that the new element will be added to
     * @param tagName TagName to be used for the created Child Element
     * @param value The value that will be stored in the Element
     **/
    public static void addString(Element element, String tagName, String value) {
        StructuredDocument structuredDocument = element.getRoot();
        Element childElement = structuredDocument.createElement(tagName, value);

        element.appendChild(childElement);
    }

    /**
     *  Get  the value of an element as a String
     *
     * @param element Element that contains the value
     * @return the value converted from a String
     **/
    public static String getString(Element element) {
        return (String) element.getValue();
    }

    /**
     *  Get the value of a Child Element 
     *
     * @param element The Parant Element
     * @param tagName The Tag Name of the Child Element that will contain the value
     * @param defaultValue The return value if there is no Child Element with that Tag Name
     * @return The value found in the Element
     **/
    public static String getString(Element element, String tagName, String defaultValue) {
        Element childElement = getChildElement(element, tagName);
		
        if (childElement != null) {
            return getString(childElement);
        } else {
            return defaultValue;
        }
    }

    /**
     *  Convert a DocumentSerializable object to its XML representation as a String
     *  
     *  The Root TagName will be 'documentSerializable' by default
     *  
     * @param documentSerializable The Object to be converted to an XML Document
     * @return The String representation of an XML Document
     * @throws DocumentSerializationException if Unable to serialize object.
     **/
    public static String toXmlString(DocumentSerializable documentSerializable) throws DocumentSerializationException {
        return toXmlString(documentSerializable, "documentSerializable");
    }

    /**
     *  Convert a DocumentSerializable object to its XML representation as a String
     *  
     *  The Root TagName will be 'documentSerializable' by default
     *  
     * @param documentSerializable The Object to be converted to an XML Document
     * @param rootTagName The Root tagName for the XML Document
     * @return The String representation of an XML Document
     * @throws DocumentSerializationException if Unable to serialize object.
     **/
    public static String toXmlString(DocumentSerializable documentSerializable, String rootTagName) throws DocumentSerializationException {
        try {
            StringWriter bout = new StringWriter();
            XMLDocument document = DocumentSerializableUtilities.createStructuredXmlDocument(rootTagName, documentSerializable);

            document.sendToWriter(bout);
            bout.close();
                        
            return bout.toString();
        } catch (IOException e) {
            throw new DocumentSerializationException("Error converting to String", e);
        }
    }
	
    /**
     *  Write a DocumentSerializable object as an XML Document to a Stream
     *  
     *  The Root TagName will be 'documentSerializable' by default
     *  
     * @param out The Stream to write the document to
     * @param documentSerializable The Object to be converted to an XML Document
     * @throws DocumentSerializationException if Unable to serialize object.
     * @throws IOException if I/O error while writing
     **/
    public static void writeAsXmlString(OutputStream out, DocumentSerializable documentSerializable) throws IOException, DocumentSerializationException {
        writeAsXmlString(out, documentSerializable, "documentSerializable");
    }
	
    /**
     *  Write a DocumentSerializable object as an XML Document to a Stream
     *  
     *  The Root TagName will be 'documentSerializable' by default
     *  
     * @param out The Stream to write the document to
     * @param rootTagName The Root tagName for the XML Document
     * @param documentSerializable The Object to be converted to an XML Document
     * @throws DocumentSerializationException if Unable to serialize object.
     * @throws IOException if I/O error while writing
     **/
    public static void writeAsXmlString(OutputStream out, DocumentSerializable documentSerializable, String rootTagName) throws IOException, DocumentSerializationException {
        StructuredDocument document = DocumentSerializableUtilities.createStructuredXmlDocument(rootTagName, documentSerializable);

        document.sendToStream(out);
    }

    /**
     *  Write a DocumentSerializable object as an XML Document to StdErr
     *  
     *  The Root TagName will be 'documentSerializable' by default
     *  
     * @param documentSerializable The DocumentSerializable to be printed.
     **/
    public static void printAsXmlString(DocumentSerializable documentSerializable) {
        try {
            if (documentSerializable == null) {
                System.err.println("<null DocumentSerializable>");
            } else {
                writeAsXmlString(System.err, documentSerializable);
            }
        } catch (Exception e) {
            System.err.println("<Error converting DocumentSerializable to XML doc: " + e);
        }
    }
	
    /**
     *  Create a DocumentSerializable Object from an XML Document
     *  
     * @param buf The XML document contained in a String
     * @param clazz The Class of the resurrected object (must implement DocumentSerializable and have a public no-arg constructor)
     * @return An object of type 'clazz'
     * @throws DocumentSerializationException if Unable to parse object.
     **/
    public static DocumentSerializable getDocumentSerializableFromXml(String buf, Class clazz) throws DocumentSerializationException {
        try {
            XMLDocument xmlDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8
                    ,
                    new StringReader(buf));

            return getDocumentSerializable(xmlDoc.getRoot(), clazz);			
        } catch (IOException readErr) {
            throw new DocumentSerializationException("Unable to read the document", readErr);
        } catch (JxtaException e) {
            throw new DocumentSerializationException("Unable to get the document", e);
        }
    }

    /**
     *  Create a DocumentSerializable Object from an XML Document
     *  
     * @param buf The XML document contained in a byte buffer
     * @param clazz The Class of the resurrected object (must implement DocumentSerializable and have a public no-arg constructor)
     * @return An object of type 'clazz'
     * @throws DocumentSerializationException if Unable to parse object.
     **/
    public static DocumentSerializable getDocumentSerializableFromXml(byte buf[], Class clazz) throws DocumentSerializationException {
        return getDocumentSerializableFromXml(new ByteArrayInputStream(buf), clazz);
    }

    /**
     *  Create a DocumentSerializable Object from an XML Document
     *  
     * @param in The Stream containing an XML Document to be read
     * @param clazz The Class of the resurrected object (must implement DocumentSerializable and have a public no-arg constructor)
     * @return An object of type 'clazz'
     * @throws DocumentSerializationException if Unable to parse object.
     **/
    public static DocumentSerializable getDocumentSerializableFromXml(InputStream in, Class clazz) throws DocumentSerializationException {
        try {
            XMLDocument xmlDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, in);

            return getDocumentSerializable(xmlDoc.getRoot(), clazz);			
        } catch (IOException readErr) {
            throw new DocumentSerializationException("Unable to read the document", readErr);
        } catch (JxtaException e) {
            throw new DocumentSerializationException("Unable to get the document", e);
        }
    }

}

