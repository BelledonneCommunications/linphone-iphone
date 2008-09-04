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

package net.jxta.protocol;


import java.io.ByteArrayInputStream;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;

import net.jxta.document.Element;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.logging.Logging;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;


/**
 * Generated when instantiating a group on a peer and contains all the
 * parameters that services need to publish. It is then published within the
 * group.
 */
public abstract class PeerAdvertisement extends ExtendableAdvertisement implements Cloneable {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(PeerAdvertisement.class.getName());

    /**
     * The id of this peer.
     */
    private PeerID pid = null;

    /**
     * The group in which this peer is located.
     */
    private PeerGroupID gid = null;

    /**
     * The name of this peer. Not guaranteed to be unique in any way. May be empty or
     * null.
     */
    private String name = null;

    /**
     * Descriptive meta-data about this peer.
     */
    private Element description = null;

    /**
     * A table of structured documents to be interpreted by each service.
     */
    private final Map<ID, StructuredDocument> serviceParams = new HashMap<ID, StructuredDocument>();

    /**
     * Counts the changes made to this object. We rely on implementations to
     * increment modCount every time something is changed without going through
     * the API.
     */
    private final transient AtomicInteger modCount = new AtomicInteger(0);

    /**
     * Returns the number of times this object has been modified since
     * it was created. This permits to detection of local changes that require
     * refreshing of other data which depends upon the peer advertisement.
     *
     * @return int the current modification count.
     */

    public int getModCount() {
        return modCount.get();
    }

    /**
     * Increments the modification count for this peer advertisement.
     */
    protected int incModCount() {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            Throwable trace = new Throwable("Stack Trace");
            StackTraceElement elements[] = trace.getStackTrace();

            LOG.finer("Modification #" + (getModCount() + 1) + " to PeerAdv@" + Integer.toHexString(System.identityHashCode(this))
                    + " caused by : " + "\n\t" + elements[1] + "\n\t" + elements[2]);
        }

        return modCount.incrementAndGet();
    }

    /**
     * Returns the identifying type of this Advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:PA";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public PeerAdvertisement clone() {

        try {
            PeerAdvertisement clone = (PeerAdvertisement) super.clone();

            clone.setPeerID(getPeerID());
            clone.setPeerGroupID(getPeerGroupID());
            clone.setName(getName());
            clone.setDesc(getDesc());
            clone.setServiceParams(getServiceParams());

            return clone;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }
    }

    /**
     * returns the name of the peer.
     *
     * @return String name of the peer.
     */

    public String getName() {
        return name;
    }

    /**
     * sets the name of the peer.
     *
     * @param name name of the peer.
     */

    public void setName(String name) {
        this.name = name;
        incModCount();
    }

    /**
     * Returns the id of the peer.
     *
     * @return PeerID the peer id
     */

    public PeerID getPeerID() {
        return pid;
    }

    /**
     * Sets the id of the peer.
     *
     * @param pid the id of this peer.
     */

    public void setPeerID(PeerID pid) {
        this.pid = pid;
        incModCount();
    }

    /**
     * Returns the id of the peergroup this peer advertisement is for.
     *
     * @return PeerGroupID the peergroup id
     */

    public PeerGroupID getPeerGroupID() {
        return gid;
    }

    /**
     * Returns the id of the peergroup this peer advertisement is for.
     *
     * @param gid The id of the peer.
     */

    public void setPeerGroupID(PeerGroupID gid) {
        this.gid = gid;
        incModCount();
    }

    /**
     * Returns a unique ID for that peer X group intersection. This is for
     * indexing purposes only.
     *
     * @return ID the unique ID
     */
    @Override
    public ID getID() {
        // If it is incomplete, there's no meaningful ID that we can return.
        if (gid == null || pid == null) {
            return null;
        }

        try {
            String hashValue = getAdvertisementType() + gid.getUniqueValue().toString() + pid.getUniqueValue().toString();
            byte[] seed = hashValue.getBytes("UTF-8");

            return IDFactory.newCodatID(gid, seed, new ByteArrayInputStream(seed));
        } catch (Exception failed) {
            return null;
        }
    }

    /**
     * returns the description
     *
     * @return String the description
     */
    public String getDescription() {
        if (null != description) {
            return (String) description.getValue();
        } else {
            return null;
        }
    }

    /**
     * sets the description
     *
     * @param description the description
     * @since JXTA 1.0
     */
    public void setDescription(String description) {

        if (null != description) {
            StructuredDocument newdoc = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Desc", description);

            setDesc(newdoc);
        } else {
            this.description = null;
        }

        incModCount();
    }

    /**
     * returns the description
     *
     * @return the description
     */
    public StructuredDocument getDesc() {
        if (null != description) {
            StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(description);

            return newDoc;
        } else {
            return null;
        }
    }

    /**
     * sets the description
     *
     * @param desc the description
     */
    public void setDesc(Element desc) {

        if (null != desc) {
            this.description = StructuredDocumentUtils.copyAsDocument(desc);
        } else {
            this.description = null;
        }

        incModCount();
    }

    /**
     * sets the sets of parameters for all services. This method first makes a
     * deep copy, in order to protect the active information from uncontrolled
     * sharing. This quite an expensive operation. If only a few of the
     * parameters need to be added, it is wise to use putServiceParam()
     * instead.
     *
     * @param params The whole set of parameters.
     */
    public void setServiceParams(Hashtable<ID, ? extends Element> params) {
        serviceParams.clear();

        if (params == null) {
            return;
        }

        for (Map.Entry<ID, ? extends Element> anEntry : params.entrySet()) {
            Element e = anEntry.getValue();
            StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(e);

            serviceParams.put(anEntry.getKey(), newDoc);
        }

        incModCount();
    }

    /**
     * Returns the sets of parameters for all services.
     *
     * <p/>This method returns a deep copy, in order to protect the real
     * information from uncontrolled sharing while keeping it shared as long as
     * it is safe. This quite an expensive operation. If only a few parameters
     * need to be accessed, it is wise to use getServiceParam() instead.
     *
     * @return all of the parameters.
     */
    public Hashtable<ID, StructuredDocument> getServiceParams() {
        Hashtable<ID, StructuredDocument> copy = new Hashtable<ID, StructuredDocument>();

        for (Map.Entry<ID, StructuredDocument> anEntry : serviceParams.entrySet()) {
            Element e = anEntry.getValue();
            StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(e);

            copy.put(anEntry.getKey(), newDoc);
        }

        return copy;
    }

    /**
     * Puts a service parameter in the service parameters table under the given
     * key. The key is of a subclass of ID; usually a ModuleClassID. This
     * method makes a deep copy of the given element into an independent
     * document.
     *
     * @param key   The key.
     * @param param The parameter, as an element. What is stored is a copy as a
     *              standalone StructuredDocument which type is the element's name.
     */
    public void putServiceParam(ID key, Element param) {
        if (param == null) {
            serviceParams.remove(key);
            incModCount();
            return;
        }

        StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(param);

        serviceParams.put(key, newDoc);

        incModCount();
    }

    /**
     * Returns the parameter element that matches the given key from the
     * service parameters table. The key is of a subclass of ID; usually a
     * ModuleClassID.
     *
     * @param key The key.
     * @return StructuredDocument The matching parameter document or null if
     *         none matched. The document type id "Param".
     */
    public StructuredDocument getServiceParam(ID key) {
        StructuredDocument param = serviceParams.get(key);

        if (param == null) {
            return null;
        }

        return StructuredDocumentUtils.copyAsDocument(param);
    }

    /**
     * Removes and returns the parameter element that matches the given key
     * from the service parameters table. The key is of a subclass of ID;
     * usually a ModuleClassID.
     *
     * @param key The key.
     * @return Element the removed parameter element or null if not found.
     *         This is actually a StructureDocument of type "Param".
     */
    public StructuredDocument removeServiceParam(ID key) {
        Element param = serviceParams.remove(key);

        if (param == null) {
            return null;
        }

        incModCount();

        // It sound silly to clone it, but remember that we could be sharing
        // this element with a clone of ours, so we have the duty to still
        // protect it.

        return StructuredDocumentUtils.copyAsDocument(param);
    }
}
