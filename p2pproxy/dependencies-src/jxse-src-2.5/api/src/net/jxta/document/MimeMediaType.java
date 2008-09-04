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


import java.io.*;
import java.lang.ref.Reference;
import java.lang.ref.WeakReference;
import java.util.*;


/**
 * MIME Media Types are used to describe the format of data streams. MIME
 * Media Types are defined by
 * {@link <a href="http://www.ietf.org/rfc/rfc2046.txt" target="_blank">IETF RFC 2046 <i>MIME : Media Types</i></a>}.
 * This class manages parsing of Mime Media Types from strings and piecemeal
 * construction of Mime Media Type descriptors.
 * <p/>
 * <p/>Note : This implementation does not include support for the character
 * encoding techniques described by :
 * {@link <a href="http://www.ietf.org/rfc/rfc2046.txt" target="_blank">IETF RFC 2046 <i>MIME : Media Types</i></a>}.
 *
 * @see net.jxta.document.Document
 * @see net.jxta.document.StructuredDocument
 * @see net.jxta.document.StructuredDocumentFactory
 * @see net.jxta.document.StructuredTextDocument
 */
public class MimeMediaType implements Serializable {

    /**
     * Magic value for this format of serialization version.
     */
    private final static long serialVersionUID = 7546247036878523161L;

    private final static String CTL = "\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007"
            + "\u0008\u0009\n\u000b\u000c\r\u000e\u000f" + "\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017"
            + "\u0018\u0019\u001a\u001b\u001c\u001d\u001e\u001f" + "\u007f";

    private final static String space = "\u0020";
    private final static String LWSP_char = space + "\u0009";
    private final static String param_sep = LWSP_char + ";";
    private final static String tspecials = "()<>@,;:\\\"/[]?=";
    private final static String terminator = CTL + space + tspecials;

    /**
     * A canonical map of Mime Media Types.
     */
    private static final Map<MimeMediaType, Reference<MimeMediaType>> interned = new WeakHashMap<MimeMediaType, Reference<MimeMediaType>>();

    /**
     * Common Mime Media Type for arbitrary unparsed binary data.
     */
    public static final MimeMediaType AOS = new MimeMediaType("application", "octet-stream").intern();

    /**
     * Common Mime Media Type for text encoded using the default character
     * encoding for this JVM. The default character encoding is specified by
     * the JDK System property "<code>file.encoding</code>".
     * <p/>
     * <p/>The default encoding varies with host platform and locale. This
     * media type <b>must not</b> be used for <b>any</b> documents which
     * will be  exchanged with other peers (as they may be using different
     * default character encodings).
     */
    public static final MimeMediaType TEXT_DEFAULTENCODING = new MimeMediaType("text", "plain").intern();

    /**
     * Common Mime Media Type for plain text encoded as UTF-8 characters. This
     * type is used by JXTA for all strings.
     */
    public static final MimeMediaType TEXTUTF8 = new MimeMediaType("text", "plain", "charset=\"UTF-8\"").intern();

    /**
     * Common Mime Media Type for XML encoded using the default character
     * encoding for this JVM. The default character encoding is specified by
     * the JDK System property "<code>file.encoding</code>".
     * <p/>
     * <p/>The default encoding varies with host platform and locale. This
     * media type <b>must not</b> be used for <b>any</b> documents which
     * will be  exchanged with other peers (as they may be using different
     * default character encodings).
     */
    public static final MimeMediaType XML_DEFAULTENCODING = new MimeMediaType("text", "xml").intern();

    /**
     * Common Mime Media Type for XML encoded using the default character
     * encoding for this JVM. The default character encoding is specified by
     * the JDK System property "<code>file.encoding</code>".
     * <p/>
     * <p/>The default encoding varies with host platform and locale. This
     * media type <b>must not</b> be used for <b>any</b> documents which
     * will be  exchanged with other peers (as they may be using different
     * default character encodings).
     */
    public static final MimeMediaType APPLICATION_XML_DEFAULTENCODING = new MimeMediaType("application", "xml").intern();

    /**
     * Common Mime Media Type for XML encoded as UTF-8 characters. This type is
     * used by JXTA for all protocol messages and metadata.
     */
    public static final MimeMediaType XMLUTF8 = new MimeMediaType("text", "xml", "charset=\"UTF-8\"").intern();

    /**
     * The primary media type
     */
    private transient String type = null;

    /**
     * The specific media sub-type
     */
    private transient String subtype = null;

    /**
     * The parameters for this media type
     */
    private transient List<parameter> parameters = new ArrayList<parameter>();
    
    /**
     *  The hashcode value for this mime media type.
     */
    private transient int cachedHashCode = 0;

    /**
     * manages a media type parameter.
     */
    private static class parameter implements Comparable<parameter> {

        /**
         * Attribute name.
         */
        final String attribute;

        /**
         * Value for the attribute. <b>Includes quoting characters if they are
         * needed for outputting this value.</b>
         */
        final String value;

        parameter(String attr, String val) {
            attribute = attr;
            value = val;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }

            if (!(obj instanceof parameter)) {
                return false;
            }

            parameter asParameter = (parameter) obj;

            return attribute.equalsIgnoreCase(asParameter.attribute) && asParameter.value.equals(value);

        }

        /**
         * {@inheritDoc}
         */
        @Override
        public int hashCode() {
            return attribute.toLowerCase().hashCode() * 6037 + value.hashCode();
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public String toString() {
            return attribute + "=" + outputForm(value);
        }

        /**
         * {@inheritDoc}
         */
        public int compareTo(parameter asParameter) {
            if (this == asParameter) {
                return 0;
            }

            int result = attribute.compareToIgnoreCase(asParameter.attribute);

            if (0 != result) {
                return result;
            }

            return value.compareTo(asParameter.value);
        }

        private static String outputForm(String val) {
            StringBuilder result = new StringBuilder();

            if (-1 == findNextSeperator(val)) {
                result.append(val);
            } else {
                // needs quoting
                result.append('\"');
                for (int eachChar = 0; eachChar < val.length(); eachChar++) {
                    char aChar = val.charAt(eachChar);

                    if (('\\' == aChar) || ('\"' == aChar) || ('\r' == aChar)) {
                        // needs escaping.
                        result.append('\\');
                    }
                    result.append(aChar);
                }
                result.append('\"');
            }
            return result.toString();
        }
    }

    /**
     * Creates a new MimeMediaType
     *
     * @param mimetype string representing a mime-type
     */
    public MimeMediaType(String mimetype) {

        String cleaned = mimetype.trim();

        if (0 == cleaned.length()) {
            throw new IllegalArgumentException("input cannot be empty");
        }

        // determine the type
        int typeSepAt = findNextSeperator(cleaned);

        if ((-1 == typeSepAt) || (0 == typeSepAt) || ('/' != cleaned.charAt(typeSepAt))) {
            throw new IllegalArgumentException("expected seperator or seperator in unexpected location");
        }

        setType(cleaned.substring(0, typeSepAt));

        // determine the sub-type
        int subtypeSepAt = findNextSeperator(cleaned, typeSepAt + 1);

        String itsParams = "";

        if (-1 == subtypeSepAt) {
            setSubtype(cleaned.substring(typeSepAt + 1));
        } else {
            setSubtype(cleaned.substring(typeSepAt + 1, subtypeSepAt));
            itsParams = cleaned.substring(subtypeSepAt);
            // include the seperator, its significant
        }

        parseParams(itsParams, false);
    }

    /**
     * Creates a new type/subtype MimeMediaType
     *
     * @param type    string representing a mime type
     * @param subtype string representing a mime subtype
     */
    public MimeMediaType(String type, String subtype) {
        this(type, subtype, null);
    }

    /**
     * Creates a new type/subtype MimeMediaType
     *
     * @param type       string representing a mime type
     * @param subtype    string representing a mime subtype
     * @param parameters parameters to the mime-type constructor
     */
    public MimeMediaType(String type, String subtype, String parameters) {
        setType(type);
        setSubtype(subtype);
        if (null != parameters) {
            parseParams(parameters, false);
        }
    }

    /**
     * Creates a new type/subtype MimeMediaType with the specified parameters.
     * The parameters are copied from the source mime type and additional params
     * are added. If replace is true, then the provided params will overwrite
     * the params from the source mime type.
     *
     * @param type    the source mime type
     * @param params  parameters to the mime-type constructor
     * @param replace parameters if true then provided params should replace
     *                existing params else they are accumulated.
     */
    public MimeMediaType(MimeMediaType type, String params, boolean replace) {
        setType(type.getType());
        setSubtype(type.getSubtype());
        parameters.addAll(type.parameters);

        parseParams(params, replace);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }

        if (!(obj instanceof MimeMediaType)) {
            return false;
        }

        MimeMediaType asMMT = (MimeMediaType) obj;

        if (!type.equalsIgnoreCase(asMMT.type)) {
            return false;
        }

        if (!subtype.equalsIgnoreCase(asMMT.subtype)) {
            return false;
        }

        List<parameter> myParams = new ArrayList<parameter>(parameters);
        List<parameter> itsParams = new ArrayList<parameter>(asMMT.parameters);

        Collections.sort(myParams);
        Collections.sort(itsParams);

        return myParams.equals(itsParams);
    }

    /**
     * Similar to {@link #equals(Object)}, but ignores any parameters. Compares
     * only the type and sub-type.
     *
     * @param obj the object to compare
     * @return true if equal
     */
    public boolean equalsIngoringParams(Object obj) {
        if (this == obj) {
            return true;
        }

        if (!(obj instanceof MimeMediaType)) {
            return false;
        }

        MimeMediaType likeMe = (MimeMediaType) obj;

        boolean retValue = getType().equalsIgnoreCase(likeMe.getType()) && getSubtype().equalsIgnoreCase(likeMe.getSubtype());

        return retValue;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        if (0 == cachedHashCode) {
            List<parameter> myParams = new ArrayList<parameter>(parameters);

            Collections.sort(myParams);

            int calcedHash = type.hashCode() * 2467 + subtype.hashCode() * 3943 + myParams.hashCode();

            cachedHashCode = (0 != calcedHash) ? calcedHash : 1;
        }
        
        return cachedHashCode;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        StringBuilder retValue = new StringBuilder(getMimeMediaType());

        for (parameter parameter : parameters) {
            retValue.append(';');
            parameter aParam = parameter;

            retValue.append(aParam.toString());
        }
        return retValue.toString();
    }

    /**
     * Returns the "base" MIME media type of this type. ie. with no parameters.
     *
     * @return The "base" MIME media type of this MimeMediaType.
     */
    public MimeMediaType getBaseMimeMediaType() {
        return new MimeMediaType(type, subtype).intern();
    }

    /**
     * Returns the "base" MIME media type of this type. ie. with no parameters.
     *
     * @return The "base" MIME media type of this type. ie. with no parameters.
     */
    public String getMimeMediaType() {
        StringBuilder retValue = new StringBuilder(type.length() + 1 + subtype.length());

        retValue.append(type);
        retValue.append('/');
        retValue.append(subtype);

        return retValue.toString();
    }

    /**
     * Get type of the mime-type
     *
     * @return type of the mime-type
     */
    public String getType() {
        return type;
    }

    /**
     * Check if the mime-type is for provisional. See Section 2.1 of
     * {@link <a href=http://www.ietf.org/rfc/rfc2048.txt">IETF RFC 2048 <i>MIME : Registration Procedures</i></a>}
     *
     * @return boolean  true if it is a provisional type
     */
    public boolean isExperimentalType() {
        if ((null == type) || (type.length() < 2)) {
            return false;
        }

        if (type.startsWith("x-") || type.startsWith("x.")) {
            return true;
        }

        return null != subtype && subtype.length() >= 2 && (subtype.startsWith("x-") || subtype.startsWith("x."));

    }

    /**
     * Set the type of MimeMediaType
     *
     * @param type type value
     */
    private void setType(String type) {
        if (null == type) {
            throw new IllegalArgumentException("type cannot be null");
        }

        String cleaned = type.trim().toLowerCase(Locale.US);

        if (0 == cleaned.length()) {
            throw new IllegalArgumentException("type cannot be null");
        }

        if (-1 != findNextSeperator(cleaned)) {
            throw new IllegalArgumentException("type cannot contain a seperator");
        }

        this.type = cleaned;
    }

    /**
     * Get the Subtype of the mime-type
     *
     * @return subtype of the mime-type
     */
    public String getSubtype() {
        return subtype;
    }

    /**
     * Check if the mime-type is for debugging. This method will be
     * removed
     *
     * @return boolean  true if it is a debugging type
     */
    public boolean isExperimentalSubtype() {
        if ((null == subtype) || (subtype.length() < 2)) {
            return false;
        }

        return (('x' == subtype.charAt(0)) && ('-' == subtype.charAt(1)));
    }

    /**
     * Set the subtype of MimeMediaType
     *
     * @param subtype subtype value
     */
    private void setSubtype(String subtype) {
        if (null == subtype) {
            throw new IllegalArgumentException("subtype cannot be null");
        }

        String cleaned = subtype.trim().toLowerCase(Locale.US);

        if (0 == cleaned.length()) {
            throw new IllegalArgumentException("subtype cannot be null");
        }

        if (-1 != findNextSeperator(cleaned)) {
            throw new IllegalArgumentException("subtype cannot contain a seperator");
        }

        this.subtype = cleaned;
    }

    /**
     * Get the value of the first occurance of the specified parameter from the
     * parameter list.
     *
     * @param param the parameter to retrieve.
     * @return the value of the specifid parameter or null if the parameter was
     *         not found.
     */
    public String getParameter(String param) {
        for (parameter parameter : parameters) {
            parameter aParam = parameter;

            if (aParam.attribute.equalsIgnoreCase(param)) {
                return aParam.value;
            }
        }
        return null;
    }

    /**
     * Parses the parametes portion of a MIME Media Type specification.
     *
     * @param itsParams parse a string for mime parameters.
     * @param replace   parameters if true then provided params should replace
     *                  existing params else they are accumulated.
     */
    private void parseParams(String itsParams, boolean replace) {
        int currentCharIdx = 0;
        String currentAttribute = null;
        boolean inSeperator = true;
        boolean inComment = false;
        boolean inAttribute = false;
        StringBuffer currentValue = null;
        boolean inValue = false;
        boolean inQuoted = false;
        boolean nextEscaped = false;

        while (currentCharIdx < itsParams.length()) {
            char currentChar = itsParams.charAt(currentCharIdx);

            if (inSeperator) {
                if ('(' == currentChar) {
                    inSeperator = false;
                    inComment = true;
                } else if (-1 == param_sep.indexOf(currentChar)) {
                    inSeperator = false;
                    inAttribute = true;
                    currentCharIdx--; // unget
                }
            } else if (inComment) {
                if (nextEscaped) {
                    nextEscaped = false;
                } else {
                    if ('\\' == currentChar) {
                        nextEscaped = true;
                    } else if (')' == currentChar) {
                        inComment = false;
                        inSeperator = true;
                    } else if ('\r' == currentChar) {
                        throw new IllegalArgumentException("malformed mime parameter at idx = " + currentCharIdx);
                    }
                }
            } else if (inAttribute) {
                int endAttr = findNextSeperator(itsParams, currentCharIdx);

                if ((-1 == endAttr) || ('=' != itsParams.charAt(endAttr)) || (0 == (endAttr - currentCharIdx))) {
                    throw new IllegalArgumentException("malformed mime parameter at idx = " + currentCharIdx);
                }

                currentAttribute = itsParams.substring(currentCharIdx, endAttr).toLowerCase(Locale.US);

                currentCharIdx = endAttr; // skip the equals.
                inAttribute = false;
                inValue = true;
                inQuoted = false;
                nextEscaped = false;
                currentValue = new StringBuffer();
            } else if (inValue) {
                if (inQuoted) {
                    if (nextEscaped) {
                        currentValue.append(currentChar);
                        nextEscaped = false;
                    } else {
                        if ('\\' == currentChar) {
                            nextEscaped = true;
                        } else if ('"' == currentChar) {
                            inQuoted = false;
                        } else if ('\r' == currentChar) {
                            throw new IllegalArgumentException("malformed mime parameter at idx = " + currentCharIdx);
                        } else {
                            currentValue.append(currentChar);
                        }
                    }
                } else if (-1 == terminator.indexOf(currentChar)) {
                    currentValue.append(currentChar);
                } else {
                    if ('"' == currentChar) {
                        inQuoted = true;
                    } else {
                        parameter newparam = new parameter(currentAttribute, currentValue.toString());

                        if (replace) {
                            while (parameters.remove(newparam)) {}
                        }

                        parameters.add(newparam);

                        inValue = false;
                        inSeperator = true;
                        currentCharIdx--; // unget
                    }
                }
            } else {
                throw new IllegalArgumentException("malformed mime parameter at idx = " + currentCharIdx);
            }

            currentCharIdx++;
        }

        // finish off the last value.
        if (inValue) {
            if (nextEscaped || inQuoted) {
                throw new IllegalArgumentException("malformed mime parameter at idx = " + currentCharIdx);
            }

            parameter newparam = new parameter(currentAttribute, currentValue.toString());

            if (replace) {
                while (parameters.remove(newparam)) {}
            }

            parameters.add(newparam);

            inValue = false;
            inSeperator = true;
        }

        if (!inSeperator) {
            throw new IllegalArgumentException("malformed mime parameter at idx = " + currentCharIdx);
        }
    }

    /**
     * Find next separator position  in mime-type
     *
     * @param source source location
     * @return int separator location
     */
    private static int findNextSeperator(String source) {
        return findNextSeperator(source, 0);
    }

    /**
     * Find next separator position  in mime-type
     *
     * @param source source location
     * @param from   from location
     * @return int separator location
     */
    private static int findNextSeperator(String source, int from) {

        int seperator = -1;

        // find a seperator
        for (int eachChar = from; eachChar < source.length(); eachChar++) {
            if (-1 != terminator.indexOf(source.charAt(eachChar))) {
                seperator = eachChar;
                break;
            }
        }

        return seperator;
    }

    /**
     * Returns a MimeMediaType with a value represented by the specified String.
     * <p/>
     * <b>This method may produce better results than using the constructor
     * the same parameter set in that</b>:
     * <p/>
     * <code>
     * new MimeMediaType( string ) != new MimeMediaType( string )
     * </code>
     * <p/>
     * while for common types:
     * <p/>
     * <code>
     * MimeMediaType.valueOf( string ) == MimeMediaType.valueOf( string )
     * </code>
     */
    public static MimeMediaType valueOf(String mimetype) {
        return new MimeMediaType(mimetype).intern();
    }

    /**
     * Read this Object in for Java Serialization
     *
     * @param s The stream from which the Object will be read.
     * @throws IOException            for errors reading from the input stream.
     * @throws ClassNotFoundException if the serialized representation contains
     *                                references to classes which cannot be found.
     */
    private void readObject(ObjectInputStream s) throws IOException, ClassNotFoundException {
        s.defaultReadObject();

        MimeMediaType readType = MimeMediaType.valueOf(s.readUTF());

        type = readType.type;
        subtype = readType.subtype;
        parameters = readType.parameters;
    }

    /**
     *  Return the interned form of the ID.
     */
    private Object readResolve() throws ObjectStreamException {
        return intern();
    }

    /**
     * Write this Object out for Java Serialization
     *
     * @param s The stream to which the Object will be written.
     * @throws IOException for errors writing to the output stream.
     */
    private void writeObject(ObjectOutputStream s) throws IOException {
        s.defaultWriteObject();

        s.writeUTF(toString());
    }

    /**
     * Returns a canonical representation for the MimeMediaType object.
     * <p/>
     * <p/>A pool of MimeMediaType, is maintained privately by the class.
     * <p/>
     * <p/>When the intern method is invoked, if the pool already contains a
     * MimeMediaType equal to this MimeMediaType object as determined by the
     * equals(Object) method, then the MimeMediaType from the pool is returned.
     * Otherwise, this MimeMediaType object is added to the pool and a reference
     * to this MimeMediaType object is returned.
     * <p/>
     * <p/>It follows that for any two MimeMediaType <tt>s</tt> and <tt>t</tt>,
     * <tt>s.intern() == t.intern()</tt> is true if and only if <tt>s.equals(t)</tt>
     * is true.
     *
     * @return a MimeMediaType that has the same value as this type, but is
     *         guaranteed to be from a pool of unique types.
     */
    public MimeMediaType intern() {
        synchronized (MimeMediaType.class) {
            Reference<MimeMediaType> common = interned.get(this);

            MimeMediaType result;

            if (null == common) {
                common = new WeakReference<MimeMediaType>(this);
                interned.put(this, common);
                result = this;
            } else {
                result = common.get();
                if (null == result) {
                    interned.put(this, new WeakReference<MimeMediaType>(this));
                    result = this;
                }
            }

            return result;
        }
    }
}
