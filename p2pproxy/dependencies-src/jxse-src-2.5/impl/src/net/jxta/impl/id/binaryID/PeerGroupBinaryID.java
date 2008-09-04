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
package net.jxta.impl.id.binaryID;


import java.net.URI;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;
import net.jxta.id.ID;


/**
 * This class implements a PeerGroup ID. Each peer group is assigned a unique
 * peer id.BinaryID id are used to implement peer group id. Because this id is
 * built with BinaryID, pulling the parent group requires a little work. The
 * parent group is the first id, with the second following, separated by a
 * dash '-' character.<p>
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 * @see net.jxta.id.ID
 * @see net.jxta.id.IDFactory
 * @see net.jxta.peergroup.PeerGroupID
 */
public final class PeerGroupBinaryID extends net.jxta.peergroup.PeerGroupID {

    /**
     * LOG object for this class.
     */
    private final static transient Logger LOG = Logger.getLogger(PeerGroupBinaryID.class.getName());

    /**
     * This is the id string used in the XML of the id. The format is TX0..Xn where T is the type and X0 through Xn are the base64 encoded id.
     */
    protected String id;

    /**
     * Constructor for creating a new PeerGroupID with a unique ID and a parent.<p>
     * <p/>
     * Note that only the ID for the parent is obtained and not the
     * parent and the grandparent.
     *
     * @param parent         Parent peer group.
     * @param data           data byte array to be used as the id.
     * @param lengthIncluded If true, the first byte in the data array is the length of the remaining bytes.
     */
    public PeerGroupBinaryID(net.jxta.peergroup.PeerGroupID parent, byte[] data, boolean lengthIncluded) {
        this();

        String parentStr = IDFormat.childGroup(parent);

        if (parentStr != null) {
            id = BinaryIDFactory.newBinaryID(BinaryID.flagPeerGroupID, data, lengthIncluded).getID() + "."
                    + parentStr.replace('-', '.');
        } else {
            id = BinaryIDFactory.newBinaryID(BinaryID.flagPeerGroupID, data, lengthIncluded).getID();
        }
    }

    /**
     * Creates a ID from a string. Note that the ID is not currently validated.
     *
     * @param id Value of ID.
     */

    protected PeerGroupBinaryID(String id) {
        super();
        this.id = id;
    }

    /**
     * Constructor for creating a new PeerGroupID with a unique ID and a parent.
     *
     * @param data           DOCUMENT ME!
     * @param lengthIncluded DOCUMENT ME!
     */
    public PeerGroupBinaryID(byte[] data, boolean lengthIncluded) {
        this();
        id = BinaryIDFactory.newBinaryID(BinaryID.flagPeerGroupID, data, lengthIncluded).getID();
    }

    /**
     * Constructor for creating a new PeerGroupID. Note that this creates an
     * invalid ID but is required for serialization.
     */
    public PeerGroupBinaryID() {
        super();
    }

    /**
     * Constructor. Intializes contents from provided ID. This PeerGroupID has
     * no parent.
     *
     * @param id the ID data
     */
    public PeerGroupBinaryID(BinaryID id) {
        super();
        this.id = id.getID();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        return target instanceof PeerGroupBinaryID && getUniqueValue().equals(((PeerGroupBinaryID) target).getUniqueValue());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return getUniqueValue().hashCode();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getIDFormat() {
        return IDFormat.INSTANTIATOR.getSupportedIDFormat();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Object getUniqueValue() {
        if (null == id) { 
            return ID.nullID.getUniqueValue();
        }

        return getIDFormat() + "-" + id;
    }

    /**
     * {@inheritDoc}
     */
    public net.jxta.id.ID getPeerGroupID() {
        // convert to the generic world PGID as necessary
        return IDFormat.translateToWellKnown(this);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public net.jxta.peergroup.PeerGroupID getParentPeerGroupID() {
        net.jxta.peergroup.PeerGroupID result = null;

        try {
            if (id == null) {
                result = (net.jxta.peergroup.PeerGroupID) net.jxta.id.ID.nullID;
            }
            String idd = id;
            int parentStart = idd.indexOf('.');

            if (parentStart != -1) {
                idd = idd.substring(parentStart + 1);
            } else {
                result = null;
            }
            URI url = new URI("urn:jxta:" + idd.replace('.', '-'));
            net.jxta.peergroup.PeerGroupID peerGroupID = (net.jxta.peergroup.PeerGroupID) net.jxta.id.IDFactory.fromURI(url);

            result = (net.jxta.peergroup.PeerGroupID) IDFormat.translateToWellKnown(peerGroupID);
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("cannot convert sub group. ID value = " + id);
            }
            result = null;

        }
        // LOG.error("getParentPeerGroupID():"+result);
        return result;
    }

    /**
     * returns the coded ID without the binaryid tag.
     *
     * @return The coded ID without the binaryid tag.
     */
    protected String getID() {
        return id;
    }
}
