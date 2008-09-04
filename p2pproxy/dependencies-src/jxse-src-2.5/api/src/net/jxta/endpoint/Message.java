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
package net.jxta.endpoint;

import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.PrintWriter;
import java.io.Serializable;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;

import java.io.IOException;
import java.util.ConcurrentModificationException;
import java.util.NoSuchElementException;
import java.util.concurrent.atomic.AtomicInteger;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.MimeMediaType;
import net.jxta.util.AbstractSimpleSelectable;
import net.jxta.util.SimpleSelectable;

import net.jxta.impl.id.UUID.UUID;
import net.jxta.impl.id.UUID.UUIDFactory;

/**
 * Messages are abstract containers for protocol messages within JXTA. Services
 * and applications are expected to use Messages as the basis for any protocols
 * implemented within JXTA. Messages are exchanged through the
 * {@link net.jxta.endpoint.EndpointService} or
 * {@link net.jxta.pipe.PipeService}.
 * <p/>
 * A Message is composed of an ordered list of zero or more
 * {@link net.jxta.endpoint.MessageElement MessageElements}. Each
 * {@link net.jxta.endpoint.MessageElement} is associated with a namespace at
 * the time it is added to the message. Duplicate
 * {@link net.jxta.endpoint.MessageElement MessageElements} are permitted.
 * <p/>
 * <b>Messages are not synchronized. All of the iterators returned by this
 * implementation are "fail-fast". Concurrent modification of messages from
 * multiple threads will produce unexpected results and
 * {@code ConcurrentModificationException}.</b>
 *
 * @see net.jxta.endpoint.MessageElement
 * @see net.jxta.endpoint.EndpointAddress
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.pipe.InputPipe
 * @see net.jxta.pipe.OutputPipe
 * @see net.jxta.pipe.PipeService
 */
public class Message extends AbstractSimpleSelectable implements Serializable {

    /**
     * Logger
     */
    private static final transient Logger LOG = Logger.getLogger(Message.class.getName());

    /**
     * Magic value for this format of serialization version.
     */
    private static final long serialVersionUID = 3418026921074097757L;

    /**
     * If {@code true}, then modification logging be activated. This is a very
     * expensive option as it causes a stack crawl to be captured for every
     * message modification.
     * <p/>
     * To enable modification tracking, set to {@code true} and recompile.
     */
    protected static final boolean LOG_MODIFICATIONS = false;

    /**
     * If {@code true}, then a special tracking element is added to every
     * message. This provides the ability to follow messages throughout the
     * network. If a message has a tracking element then it will be used in
     * the {@code toString()} representation.
     * <p/>
     * The element is currently named "Tracking UUID" and is stored in the
     * "jxta" namespace. The element contains an IETF version 1 UUID in string
     * form.
     * <p/>
     * To enable addition of a tracking element to every message, set the
     * Java System Property {@code net.jxta.endpoint.Message.globalTracking} to
     * {@code true} and restart your JXTA application.
     *
     * @see java.lang.System#setProperty(String,String)
     */
    protected static final boolean GLOBAL_TRACKING_ELEMENT =
            Boolean.getBoolean(Message.class.getName() + ".globalTracking");

    /**
     * Incremented for each standalone message instance. {@see #lineage} for
     * information about how message numbers can be used.
     */
    private static transient AtomicInteger messagenumber = new AtomicInteger(1);

    /**
     * This string identifies the namespace which is assumed when calls are
     * made that don't include a namespace specification.
     */
    protected final String defaultNamespace;

    /**
     * the namespaces in this message and the elements in each.
     */
    protected transient Map<String, List<MessageElement>> namespaces = new HashMap<String, List<MessageElement>>();

    /**
     * List of the elements.
     */
    protected transient List<element> elements = new ArrayList<element>();

    /**
     * Message properties HashMap
     */
    protected transient Map<Object, Object> properties = Collections.synchronizedMap(new HashMap<Object, Object>());

    /**
     * A list of {@link java.lang.Integer} which details the lineage (history
     * of cloning) that produced this message. This message's number is index
     * 0, all of the ancestors are in order at higher indexes.
     * <p/>
     * Message numbers are not part of the message content and are only
     * stored locally. The are useful for following messages throughout their
     * lifetime and is normally shown as part of the <tt>toString()</tt>
     * display for Messages.
     */
    protected transient List<Integer> lineage = new ArrayList<Integer>();

    /**
     * Modification count of this message. Can be used to detect message being
     * concurrently modified when message is shared.
     * <p/>
     * The modification count is part of the {@code toString()} display for
     * Messages.
     */
    protected transient volatile int modCount = 0;

    /**
     * cached aggregate size of all the member elements. Used by
     * {@link #getByteLength()}
     */
    protected transient long cachedByteLength = 0;

    /**
     * modcount at the time the message length was last calculated. Used by
     * {@link #getByteLength()}
     */
    protected transient int cachedByteLengthModCount = -1;

    /**
     * If <tt>true</tt> then the message is modifiable. This is primarily
     * intended as a diagnostic tool for detecting concurrent modification.
     *
     * @deprecated You really should not depend on this feature.
     */
    @Deprecated
    public boolean modifiable = true;

    /**
     * If {@code LOG_MODIFICATIONS} is {@code true} then this will contain
     * the history of modifications this message.
     * <p/>
     * <ul>
     * <li>Values are {@link java.lang.Throwable} with the description
     * field formatted as <code>timeInAbsoluteMillis : threadName</code>.
     * </li>
     * </ul>
     */
    protected transient List<Throwable> modHistory;

    /**
     * A ListIterator for MessageElements which also provides the ability to
     * determine the namespace of the current message element. Message Elements
     * are iterated in the order in which they were added to the Message.
     * <p/>
     * This Iterator returned is not synchronized with the message. If you
     * modify the state of the Message, the iterator will throw
     * ConcurrentModificationException when {@code next()} or
     * {@code previous()} is called.
     */
    public class ElementIterator implements ListIterator<MessageElement> {

        /**
         * The elements being iterated.
         */
        ListIterator<element> list;

        /**
         * The current element
         */
        element current = null;

        /**
         * The modCount at the time when the iterator was created.
         */
        transient int origModCount;

        /**
         * Intialize the iterator from a list iterator.
         *
         * @param list The ListIterator we are managing.
         */
        ElementIterator(ListIterator<element> list) {
            origModCount = Message.this.getMessageModCount();
            this.list = list;
        }

        /**
         * {@inheritDoc}
         */
        public boolean hasNext() {
            if (origModCount != Message.this.getMessageModCount()) {
                RuntimeException failure = new ConcurrentModificationException(
                        Message.this + " concurrently modified. Iterator was made at mod " + origModCount);

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE,
                            Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                            + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                            failure);
                }
                throw failure;
            }
            return list.hasNext();
        }

        /**
         * {@inheritDoc}
         */
        public MessageElement next() {
            if (origModCount != Message.this.getMessageModCount()) {
                RuntimeException failure = new ConcurrentModificationException(
                        Message.this + " concurrently modified. Iterator was made at mod " + origModCount);

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE,
                            Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                            + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                            failure);
                }
                throw failure;
            }

            current = list.next();
            return current.element;
        }

        /**
         * {@inheritDoc}
         */
        public int nextIndex() {
            return list.nextIndex();
        }

        /**
         * {@inheritDoc}
         */
        public boolean hasPrevious() {
            if (origModCount != Message.this.getMessageModCount()) {
                RuntimeException failure = new ConcurrentModificationException(
                        Message.this + " concurrently modified. Iterator was made at mod " + origModCount);

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE,
                            Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                            + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                            failure);
                }
                throw failure;
            }

            return list.hasPrevious();
        }

        /**
         * {@inheritDoc}
         */
        public MessageElement previous() {
            if (origModCount != Message.this.getMessageModCount()) {
                RuntimeException failure = new ConcurrentModificationException(
                        Message.this + " concurrently modified. Iterator was made at mod " + origModCount);

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE,
                            Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                            + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                            failure);
                }
                throw failure;
            }

            current = list.previous();
            return current.element;
        }

        /**
         * {@inheritDoc}
         */
        public int previousIndex() {
            return list.previousIndex();
        }

        /**
         * {@inheritDoc}
         * <p/>
         * Not provided because the namespace cannot be specified.
         */
        public void add(MessageElement obj) {
            throw new UnsupportedOperationException("add() not supported");
        }

        /**
         * {@inheritDoc}
         */
        public void remove() {
            if (origModCount != Message.this.getMessageModCount()) {
                RuntimeException failure = new ConcurrentModificationException(
                        Message.this + " concurrently modified. Iterator was made at mod " + origModCount);

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE,
                            Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                            + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                            failure);
                }

                throw failure;
            }

            if (null == current) {
                throw new IllegalStateException("no current element, call next() or previous()");
            }

            ListIterator<element> elsPosition = Message.this.elements.listIterator();
            ListIterator<MessageElement> nsPosition = namespaces.get(current.namespace).listIterator();

            int currentPrevious = list.previousIndex();

            // restart this iterator
            while (list.previousIndex() >= 0) {
                list.previous();
            }

            // readvance to the current position, but track in ns list and master list
            while (list.previousIndex() < currentPrevious) {
                element anElement = list.next();

                try {
                    // advance to the same element in the master list.
                    element anElsElement;

                    do {
                        anElsElement = elsPosition.next();
                    } while (anElement != anElsElement);

                    // advance to the same element in the ns list.
                    MessageElement anNsElement;

                    if (current.namespace.equals(anElement.namespace)) {
                        do {
                            anNsElement = nsPosition.next();
                        } while (anElement.element != anNsElement);
                    }
                } catch (NoSuchElementException ranOut) {
                    RuntimeException failure = new ConcurrentModificationException(
                            Message.this + " concurrently modified. Iterator was made at mod " + origModCount);

                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE,
                                Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                                + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                                failure);
                    }

                    throw failure;
                }
            }

            elsPosition.remove();
            nsPosition.remove();
            list.remove();
            origModCount = Message.this.incMessageModCount();
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(
                        "Removed " + current.namespace + "::" + current.element.getElementName() + "/"
                        + current.element.getClass().getName() + "@" + current.element.hashCode() + " from " + Message.this);
            }
            current = null;
        }

        /**
         * {@inheritDoc}
         * <p/>
         * Replacement MessageElement will be in the same name space as the
         * replaced element.
         */
        public void set(MessageElement obj) {
            if (origModCount != Message.this.getMessageModCount()) {
                RuntimeException failure = new ConcurrentModificationException(
                        Message.this + " concurrently modified. ");

                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE,
                            Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                            + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                            failure);
                }
                throw failure;
            }

            if (null == current) {
                throw new IllegalStateException("no current element, call next() or previous()");
            }

            ListIterator<element> elsPosition = Message.this.elements.listIterator();
            ListIterator<MessageElement> nsPosition = namespaces.get(current.namespace).listIterator();

            int currentPrevious = list.previousIndex();

            // restart this iterator
            while (list.previousIndex() >= 0) {
                list.previous();
            }

            // readvance to the current position, but track in ns list and master list
            while (list.previousIndex() < currentPrevious) {
                element anElement = list.next();

                try {
                    // advance to the same element in the master list.
                    element anElsElement;

                    do {
                        anElsElement = elsPosition.next();
                    } while (anElement != anElsElement);

                    // advance to the same element in the ns list.
                    MessageElement anNsElement;

                    if (current.namespace.equals(anElement.namespace)) {
                        do {
                            anNsElement = nsPosition.next();
                        } while (anElement.element != anNsElement);
                    }
                } catch (NoSuchElementException ranOut) {
                    RuntimeException failure = new ConcurrentModificationException(
                            Message.this + " concurrently modified. Iterator was made at mod " + origModCount);

                    if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                        LOG.log(Level.SEVERE,
                                Message.this + " concurrently modified. iterator mod=" + origModCount + " current mod="
                                + Message.this.getMessageModCount() + "\n" + getMessageModHistory(),
                                failure);
                    }
                    throw failure;
                }
            }

            Message.element newCurrent = new Message.element(current.namespace, obj, null);

            elsPosition.set(newCurrent);
            nsPosition.set(obj);
            list.set(newCurrent);
            origModCount = Message.this.incMessageModCount();
            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(
                        "Replaced " + current.namespace + "::" + current.element.getElementName() + "/"
                        + current.element.getClass().getName() + "@" + current.element.hashCode() + " with "
                        + newCurrent.namespace + "::" + newCurrent.element.getElementName() + "/"
                        + newCurrent.element.getClass().getName() + "@" + newCurrent.element.hashCode() + " in " + Message.this);
            }
            current = newCurrent;
        }

        /**
         * return the namespace of the current element.
         *
         * @return String containing the name space of the current element.
         */
        public String getNamespace() {
            if (null == current) {
                throw new IllegalStateException("no current element, call next() or previous()");
            }

            return current.namespace;
        }

        /**
         * Return the signature element of the current element.
         *
         * @return The signature element of the current element.
         */
        public MessageElement getSignature() {
            if (null == current) {
                throw new IllegalStateException("no current element, call next() or previous()");
            }

            return (null != current.signature) ? current.signature : current.element.getSignature();
        }
    }


    /**
     * Holds an element, its namespace and optionally an override signature
     * element.
     */
    protected static class element {
        final String namespace;
        final MessageElement element;
        final MessageElement signature;

        element(String namespace, MessageElement element, MessageElement signature) {
            this.namespace = namespace;
            this.element = element;
            this.signature = signature;
        }
    }

    /**
     * Standard Constructor for messages. The default namespace will be the
     * empty string ("")
     */
    public Message() {
        this("", false);
    }

    /**
     * Standard Constructor for messages.
     *
     * @param defaultNamespace the namespace which is assumed by methods which
     * do not require a namespace specification.
     */
    protected Message(String defaultNamespace) {
        this(defaultNamespace, false);
    }
    
    /**
     * Standard Constructor for messages.
     *
     * @param defaultNamespace the namespace which is assumed by methods which
     * do not require a namespace specification.
     * @param clone If {@code true} then we are creating a clone.
     */
    private Message(String defaultNamespace, boolean clone) {
        this.defaultNamespace = defaultNamespace;

        lineage.add(messagenumber.getAndIncrement());

        if (LOG_MODIFICATIONS) {
            modHistory = new ArrayList<Throwable>();
            incMessageModCount();
        }
        
        if (!clone && GLOBAL_TRACKING_ELEMENT) {
            UUID tracking = UUIDFactory.newSeqUUID();

            MessageElement trackingElement = new StringMessageElement("Tracking UUID", tracking.toString(), null);

            addMessageElement("jxta", trackingElement);
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Duplicates the Message. The returned duplicate is a real copy. It may
     * be freely modified without causing change to the originally cloned
     * message.
     *
     * @return Message a Message that is a copy of the original message
     */
    @Override
    public Message clone() {
        Message clone = new Message(getDefaultNamespace(), true );

        clone.lineage.addAll(lineage);
        clone.elements.addAll(elements);

        for (String aNamespace : namespaces.keySet()) {
            List<MessageElement> namespaceElements = namespaces.get(aNamespace);

            List<MessageElement> newNamespaceElements = new ArrayList<MessageElement>(namespaceElements.size());

            newNamespaceElements.addAll(namespaceElements);
            clone.namespaces.put(aNamespace, newNamespaceElements);
        }

        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Created clone " + clone + " of " + this);
        }

        return clone;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Compare this Message against another. Returns {@code true} if all of the
     * elements are identical and in the same order. Message properties
     * (setProperty()/getProperty()) are not considered in the calculation.
     *
     * @param target The Message to compare against.
     * @return {@code true} if the elements are identical otherwise
     * {@code false}.
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }

        if (target instanceof Message) {
            Message likeMe = (Message) target;

            ElementIterator myElements = getMessageElements();
            ElementIterator itsElements = likeMe.getMessageElements();

            while (myElements.hasNext()) {
                if (!itsElements.hasNext()) {
                    return false; // it has fewer than i do.
                }

                MessageElement mine = myElements.next();
                MessageElement its = itsElements.next();

                if (!myElements.getNamespace().equals(itsElements.getNamespace())) {
                    return false; // elements not in the same namespace
                }

                if (!mine.equals(its)) {
                    return false; // content didnt match
                }
            }

            return (!itsElements.hasNext()); // ran out at the same time?
        }

        return false; // not a message
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        int result = 0;
        Iterator<MessageElement> eachElement = getMessageElements();

        while (eachElement.hasNext()) {
            MessageElement anElement = eachElement.next();

            result += anElement.hashCode();
            result *= 6037; // a prime
        }

        if (0 == result) {
            result = 1;
        }

        return result;
    }

    /**
     * {@inheritDoc}
     * <p/>
     * This implementation is intended to assist debugging. You should not
     * depend upon the format of the result.
     */
    @Override
    public String toString() {
        StringBuilder toString = new StringBuilder(128);

        toString.append(getClass().getName());
        toString.append('@');
        toString.append(super.hashCode());
        toString.append('(');
        toString.append(modCount);
        toString.append("){");

        Iterator allLineage = getMessageLineage();

        while (allLineage.hasNext()) {
            toString.append(allLineage.next().toString());
            if (allLineage.hasNext()) {
                toString.append(',');
            }
        }

        toString.append('}');

        if (GLOBAL_TRACKING_ELEMENT) {
            toString.append("[");
            Iterator eachUUID = getMessageElements("jxta", "Tracking UUID");

            while (eachUUID.hasNext()) {
                toString.append("[");
                toString.append(eachUUID.next().toString());
                toString.append("]");
                if (eachUUID.hasNext()) {
                    toString.append(',');
                }
            }
            toString.append("]");
        }

        return toString.toString();
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
        // reads defaultNamespace, modifiable flag
        s.defaultReadObject();

        MimeMediaType readType = new MimeMediaType(s.readUTF());

        // XXX bondolo 20040307 Should do something with encoding here.

        Message readMessage = WireFormatMessageFactory.fromWire(s, readType, null);

        namespaces = readMessage.namespaces;
        elements = readMessage.elements;

        if (!namespaces.containsKey(defaultNamespace)) {
            throw new IOException("Corrupted Object--does not contain required namespace.");
        }

        properties = new HashMap<Object, Object>();
        lineage = new ArrayList<Integer>();

        lineage.add(messagenumber.getAndIncrement());

        if (LOG_MODIFICATIONS) {
            modHistory = new ArrayList<Throwable>();
            incMessageModCount();
        }
    }

    /**
     * Write this Object out for Java Serialization
     *
     * @param s The stream to which the Object will be written.
     * @throws IOException for errors writing to the output stream.
     */
    private void writeObject(ObjectOutputStream s) throws IOException {
        s.defaultWriteObject();

        MimeMediaType writeType = WireFormatMessageFactory.DEFAULT_WIRE_MIME;

        s.writeUTF(writeType.toString());

        // XXX bondolo 20040307 Should do something with encoding here.

        WireFormatMessage serialed = WireFormatMessageFactory.toWire(this, writeType, null);

        serialed.sendToStream(s);
    }

    /**
     * Return the default Namespace of this message.
     *
     * @return The default namespace for this message.
     */
    protected String getDefaultNamespace() {
        return defaultNamespace;
    }

    /**
     * Add a MessageElement into the message. The MessageElement is stored in
     * the default namespace.
     *
     * @param add the Element to add to the message.
     */
    public void addMessageElement(MessageElement add) {

        addMessageElement(null, add);
    }

    /**
     * Add a MessageElement into the message using the specified namespace.
     *
     * @param namespace contains the namespace of the element to add. You can
     *                  specify null as a shorthand for the default namespace.
     * @param add       the MessageElement to add to the message.
     */
    public void addMessageElement(String namespace, MessageElement add) {
        addMessageElement(namespace, add, null);
    }

    /**
     * Add a MessageElement into the Message using the specified namespace.
     *
     * @param namespace contains the namespace of the element to add. You can
     *                  specify null as a shorthand for the default namespace.
     * @param add       the MessageElement to add to the message.
     * @param signature The signature MessageElement associated with the
     *                  MessageElement. This allows for an alternate signature element to the
     *                  signature element associated with the message element.
     */
    public void addMessageElement(String namespace, MessageElement add, MessageElement signature) {
        if (null == namespace) {
            namespace = getDefaultNamespace();
        }

        if (null == add) {
            throw new IllegalArgumentException("Message Element must be non-null");
        }

        elements.add(new element(namespace, add, signature));

        List<MessageElement> namespaceElements = namespaces.get(namespace);

        if (null == namespaceElements) {
            namespaceElements = new ArrayList<MessageElement>();
            namespaces.put(namespace, namespaceElements);
        }

        namespaceElements.add(add);
        incMessageModCount();
        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer( "Added " + namespace + "::" + add.getElementName() + "/" + 
                    add.getClass().getName() + "@" + add.hashCode() + " to " + this);
        }
    }

    /**
     * Replace a {@link net.jxta.endpoint.MessageElement} in the message. This
     * method will remove all MessageElement instances in the default namespace
     * which match the specified name (if any) and then insert the replacement
     * element. The existing version of the element is returned, if more than
     * one matching element was removed, a random matching element is returned.
     * <p/>
     * For greatest control over element replacement, use the
     * {@link java.util.ListIterator#set(java.lang.Object)} method as returned
     * by {@link #getMessageElements()},
     * {@link #getMessageElements(java.lang.String)} or
     * {@link #getMessageElementsOfNamespace(java.lang.String)}
     *
     * @param replacement the Element to be inserted into to the message.
     * @return One of the elements which was replaced or null if no existing
     *         matching item was located.
     */
    public MessageElement replaceMessageElement(MessageElement replacement) {
        return replaceMessageElement(null, replacement);
    }

    /**
     * Replace a {@link net.jxta.endpoint.MessageElement} in the message using the specified
     * namespace. This method will remove all MessageElement instances which
     * match the specified name (if any) and then insert the replacement
     * element. The existing version of the element is returned, if more than
     * one matching element was removed, a random matching element is returned.
     * <p/>
     * For greatest control over element replacement, use the
     * {@link java.util.ListIterator#set(java.lang.Object)} method as returned
     * by {@link #getMessageElements()},
     * {@link #getMessageElements(java.lang.String)} or
     * {@link #getMessageElementsOfNamespace(java.lang.String)}
     *
     * @param namespace   contains the namespace of the element to be replaced.
     *                    You can specify null as a shorthand for the default namespace.
     * @param replacement the Element to be inserted into to the message.
     * @return One of the elements which was replaced or null if no existing
     *         matching item was located.
     */
    public MessageElement replaceMessageElement(String namespace, MessageElement replacement) {
        if (null == namespace) {
            namespace = getDefaultNamespace();
        }

        if (null == replacement) {
            throw new IllegalArgumentException("Message Element must be non-null");
        }

        MessageElement removed = null;
        Iterator<MessageElement> allMatching = getMessageElements(namespace, replacement.getElementName());

        while (allMatching.hasNext()) {
            MessageElement anElement = allMatching.next();

            allMatching.remove();
            removed = anElement;
        }

        addMessageElement(namespace, replacement); // updates mod count

        return removed;
    }

    /**
     * Returns an iterator of the namespaces present in this message.
     *
     * @return iterator of strings of the namespaces of this message.
     */
    public Iterator<String> getMessageNamespaces() {
        return Collections.unmodifiableMap(namespaces).keySet().iterator();
    }

    /**
     * Retrieve a message element by name from the message without regard to
     * namespace. If there is more than one message element with this name, the
     * first message element will be returned.
     *
     * @param name The name of the message element to attempt to retrieve.
     * @return Element the element or null if no matching element could be
     *         found.
     */
    public MessageElement getMessageElement(String name) {
        Iterator<element> eachElement = elements.listIterator();

        while (eachElement.hasNext()) {
            element anElement = eachElement.next();

            if (name.equals(anElement.element.getElementName())) {
                return anElement.element;
            }
        }

        return null;
    }

    /**
     * Retrieve a message element by name in the specified namespace from the
     * message. If there is more than one message element matching this name,
     * the first message element will be returned.
     *
     * @param namespace contains the namespace of the element to get. You can
     *                  specify {@code null} as a shorthand for the default namespace.
     * @param name      The name of the message element to retrieve.
     * @return The Message Element or {@code null} if no matching message
     *         element could be found.
     */
    public MessageElement getMessageElement(String namespace, String name) {
        if (null == namespace) {
            namespace = getDefaultNamespace();
        }

        List<MessageElement> namespaceElements = namespaces.get(namespace);

        // no namespace means no element.
        if (null == namespaceElements) {
            return null;
        }

        Iterator<MessageElement> eachElement = namespaceElements.listIterator();

        while (eachElement.hasNext()) {
            MessageElement anElement = eachElement.next();

            if (name.equals(anElement.getElementName())) {
                return anElement;
            }
        }

        return null;
    }

    /**
     * Returns a list iterator of all of the elements contained in this message.
     * Elements from all namespaces are returned.
     * <p/>
     * The iterator returned is not synchronized with the message and will
     * throw {@link java.util.ConcurrentModificationException} if the
     * message is modified.
     *
     * @return Enumeration of Elements.
     */
    public ElementIterator getMessageElements() {
        List<element> theMsgElements = new ArrayList<element>(elements);

        return new ElementIterator(theMsgElements.listIterator());
    }

    /**
     * Returns a list iterator  of all of the elements contained in this
     * message who's name matches the specified name. Elements from all
     * namespaces are returned. Message Elements are iterated in the order in
     * which they were added to the Message.
     * <p/>
     * The iterator returned is not synchronized with the message and will
     * throw {@link java.util.ConcurrentModificationException} if the
     * message is modified.
     *
     * @param name the name of the elements to match against
     * @return iterator of the elements matching the specified name, if any.
     */
    public ElementIterator getMessageElements(String name) {
        List<element> theMsgElements = new ArrayList<element>(elements.size());

        for (element anElement : elements) {
            if (name.equals(anElement.element.getElementName())) {
                theMsgElements.add(anElement);
            }
        }

        return new ElementIterator(theMsgElements.listIterator());
    }

    /**
     * Returns a list iterator of all of the elements contained in this message
     * which match the specified namespace. Message Elements are iterated in
     * the order in which they were added to the Message.
     * <p/>
     * The ListIterator returned is not synchronized with the message. If
     * you modify the state of the Message, the iterator will throw
     * ConcurrentModificationException when {@code next()} or
     * {@code previous()} is called.
     *
     * @param namespace contains the namespace which must be matched in the
     *                  elements returned. You can specify {@code null} as a shorthand for the
     *                  default namespace.
     * @return Iterator of Message Elements matching namespace.
     */
    public ElementIterator getMessageElementsOfNamespace(String namespace) {
        List<element> theMsgElements = new ArrayList<element>(elements.size());

        if (null == namespace) {
            namespace = getDefaultNamespace();
        }

        for (element anElement : elements) {
            if (namespace.equals(anElement.namespace)) {
                theMsgElements.add(anElement);
            }
        }

        return new ElementIterator(theMsgElements.listIterator());
    }

    /**
     * Returns a list iterator  of all of the elements contained in the
     * specified namespace who's name matches the specified name in the order
     * in which they were added to the Message.
     * <p/>
     * The iterator returned is not synchronized with the message and will
     * throw {@link java.util.ConcurrentModificationException} if the message
     * is modified.
     *
     * @param namespace The namespace which must be matched in the elements
     *                  returned. You can specify {@code null} as a shorthand for the default
     *                  namespace.
     * @param name      The name of the elements to retrieve.
     * @return Iterator of Message Elements matching namespace and name.
     */
    public ElementIterator getMessageElements(String namespace, String name) {
        List<element> theMsgElements = new ArrayList<element>(elements.size());

        if (null == namespace) {
            namespace = getDefaultNamespace();
        }

        for (element anElement : elements) {
            if (namespace.equals(anElement.namespace) && name.equals(anElement.element.getElementName())) {
                theMsgElements.add(anElement);
            }
        }

        return new ElementIterator(theMsgElements.listIterator());
    }

    /**
     * Returns a list iterator of all of the elements contained in this message
     * whose mime-type matchs the given in the order they were added to the
     * message. Elements from all namespaces are returned.
     * <p/>
     * The iterator returned is not synchronized with the message and will
     * throw {@link java.util.ConcurrentModificationException} if the
     * message is modified.
     *
     * @param type contains the type of the elements to get
     * @return Iterator of Message Elements matching type.
     */
    public ElementIterator getMessageElements(MimeMediaType type) {
        List<element> theMsgElements = new ArrayList<element>(elements.size());

        ListIterator<element> eachElement = elements.listIterator();

        while (eachElement.hasNext()) {
            element anElement = eachElement.next();

            if (type.equals(anElement.element.getMimeType())) {
                theMsgElements.add(anElement);
            }
        }

        return new ElementIterator(theMsgElements.listIterator());
    }

    /**
     * Returns a list iterator of all of the elements contained in this message
     * whose type matches the given in the order they were added to the message.
     * <p/>
     * The iterator returned is not synchronized with the message and will
     * throw {@link java.util.ConcurrentModificationException} if the
     * message is modified.
     *
     * @param namespace contains the namespace which must be matched in the
     *                  elements returned. You can specify null as a shorthand for the default
     *                  namespace.
     * @param type      contains the type of the elements to get
     * @return Iterator of Message Elements matching namespace and matching
     *         type.
     */
    public ElementIterator getMessageElements(String namespace, MimeMediaType type) {
        List<element> theMsgElements = new ArrayList<element>(elements.size());

        if (null == namespace) {
            namespace = getDefaultNamespace();
        }

        for (element anElement : elements) {
            if (namespace.equals(anElement.namespace) && type.equals(anElement.element.getMimeType())) {
                theMsgElements.add(anElement);
            }
        }

        return new ElementIterator(theMsgElements.listIterator());
    }

    /**
     * Remove an the first occurrence of the provided MessageElement from the
     * message.
     *
     * @param remove the Element to remove from the message.
     * @return boolean returns true if the element was removed, otherwise false.
     */
    public boolean removeMessageElement(MessageElement remove) {
        Iterator<MessageElement> eachElement = getMessageElements();

        while (eachElement.hasNext()) {
            MessageElement anElement = eachElement.next();

            if (remove == anElement) {
                eachElement.remove(); // updates mod count

                return true;
            }
        }

        return false;
    }

    /**
     * Remove the first occurrence of the provided MessageElement within the
     * specified namespace from the message.  You can specify null as a
     * shorthand for the default namespace.
     *
     * @param namespace the namespace from which the element is to be removed.
     * @param remove    the Element to remove from the message.
     * @return boolean returns true if the element was removed, otherwise false.
     */
    public boolean removeMessageElement(String namespace, MessageElement remove) {
        Iterator<MessageElement> eachElement = getMessageElementsOfNamespace(namespace);

        while (eachElement.hasNext()) {
            MessageElement anElement = eachElement.next();

            if (remove == anElement) {
                eachElement.remove(); // updates mod count

                return true;
            }
        }

        return false;
    }

    /**
     * Removes all of the elements in all namespaces from the message. Also
     * clears any properties set for this message.
     */
    public void clear() {
        elements.clear();
        namespaces.clear();
        properties.clear();
        // a cleared message has no ancestors
        lineage.retainAll(Collections.singletonList(lineage.get(0)));

        incMessageModCount();

        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Cleared " + this);
        }
    }

    /**
     * Returns the aggregate size of all the member elements.
     *
     * @return the sum of all element sizes in bytes.
     */
    public synchronized long getByteLength() {
        if (modCount != cachedByteLengthModCount) {
            cachedByteLength = 0;
            Iterator<MessageElement> eachElement = getMessageElements();

            while (eachElement.hasNext()) {
                MessageElement anElement = eachElement.next();

                cachedByteLength += anElement.getByteLength();
            }

            cachedByteLengthModCount = modCount;
        }

        return cachedByteLength;
    }

    /**
     * Returns the modification count of this message. This ever ascending
     * number can be used to determine if the message has been modified by
     * another thread or for use in caching of parts of the message structure.
     *
     * @return the modification count of this message.
     */
    public int getMessageModCount() {
        return modCount;
    }

    /**
     * Returns the modification count of this message. This ever ascending
     * number can be used to determine if the message has been modified by
     * another thread or for use in caching of parts of the message structure.
     *
     * @return the modification count of this message.
     */
    protected synchronized int incMessageModCount() {
        modCount++;

        if (LOG_MODIFICATIONS) {
            modHistory.add(new Throwable(Long.toString(System.currentTimeMillis()) + " : " + Thread.currentThread().getName()));
        }

        if (!modifiable) {
            IllegalStateException failure = new IllegalStateException("Unmodifiable message should not have been modified");

            if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                LOG.log(Level.SEVERE, failure.getMessage(), failure);
            }
            throw failure;
        }

        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
            LOG.finer("Modification to " + this);
        }

        return modCount;
    }

    /**
     * Returns a String containing the modification history for this message
     *
     * @return a String containing the modification history for this message.
     */
    public synchronized String getMessageModHistory() {

        if (LOG_MODIFICATIONS) {
            StringBuilder modHistoryStr = new StringBuilder("Message Modification History for ");

            modHistoryStr.append(toString());
            modHistoryStr.append("\n\n");
            for (int eachMod = modHistory.size() - 1; eachMod >= 0; eachMod--) {
                StringWriter aStackStr = new StringWriter();
                Throwable aStack = modHistory.get(eachMod);

                aStack.printStackTrace(new PrintWriter(aStackStr));
                modHistoryStr.append("Modification #");
                modHistoryStr.append(eachMod + 1);
                modHistoryStr.append(":\n\n");
                modHistoryStr.append(aStackStr.toString());
                modHistoryStr.append("\n");
            }
            return modHistoryStr.toString();
        } else {
            return "Modification history tracking is disabled";
        }
    }

    /**
     * Returns the message number of this message. Message Numbers are intended
     * to assist with debugging and the management of message cloning.
     * <p/>
     * Each message is assigned a unique number upon creation. Message
     * Numbers are monotonically increasing for each message created.
     * <p/>
     * Message Numbers are transient, ie. if the message object is
     * serialized then the message number after deserialization will be
     * probably be a different value. Message numbers should not be used to
     * record permanent relationships between messages.
     *
     * @return int this message's message number.
     */
    public int getMessageNumber() {
        return lineage.get(0);
    }

    /**
     * Returns an iterator which describes the lineage of this message. Each
     * entry is an {@link java.lang.Integer} Message Number. The first entry is
     * this message's number, following entries are the ancestors of this
     * message.
     *
     * @return an Iterator of {@link java.lang.Integer}. Each entry is a
     *         message number.
     */
    public Iterator<Integer> getMessageLineage() {
        return Collections.unmodifiableList(lineage).iterator();
    }

    /**
     * Associate a transient property with this message. if there was a
     * previous value for the key provided then it is returned. This feature is
     * useful for managing the state of messages during processing and for
     * caching. <strong>Message Properties are not transmitted as part of the
     * Message when the message is serialized!</strong>
     * <p/>
     * The setting of particular keys may be controlled by a Java Security
     * Manager. Keys of type 'java.lang.Class' are checked against the caller of
     * this method. Only callers which are instances of the key class may modify
     * the property. This check is not possible through reflection. All other
     * types of keys are unchecked.
     *
     * @param key   the property key
     * @param value the value for the property
     * @return previous value for the property or null if no previous
     */
    public Object setMessageProperty(Object key, Object value) {

        /*
         if( key instanceof java.lang.Class ) {
         Class keyClass = (Class) key;
         SecurityManager secure =  new SecurityManager() {
         public boolean checkCallerOfClass( Class toCheck ) {
         Class [] context = getClassContext();

         return toCheck.isAssignableFrom( context[2] );
         }
         };

         if( !secure.checkCallerOfClass( keyClass ) ) {
         throw new SecurityException( "You can't set that key from this context." );
         }
         }
         */

        Object res = properties.put(key, value);

        // Any property addition (including redundant) is notified. Removals are
        // too, since removal is done by assigning null.

        // Exception: when removing what was not there: no notification.

        if (res != null || value != null) {
            notifyChange();
        }

        return res;
    }

    /**
     * Retrieves a transient property from the set for this message.
     *
     * @param key the property key.
     * @return value for the property or null if no property for this key.
     */
    public Object getMessageProperty(Object key) {

        return properties.get(key);
    }

    /**
     * {@inheritDoc}
     */
    public void itemChanged(SimpleSelectable o) {// For now, messages are not themselves registered with anything.
        // Therefore itemChanged does not do a thing.
    }
}
