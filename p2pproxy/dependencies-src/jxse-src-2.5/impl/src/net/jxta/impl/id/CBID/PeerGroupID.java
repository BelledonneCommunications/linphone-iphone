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

package net.jxta.impl.id.CBID;


import net.jxta.impl.id.UUID.IDBytes;
import net.jxta.impl.id.UUID.UUID;
import net.jxta.impl.id.UUID.UUIDFactory;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.ProviderException;
import java.util.logging.Logger;


/**
 * An implementation of the {@link net.jxta.peergroup.PeerGroupID} ID Type.
 */
public class PeerGroupID extends net.jxta.impl.id.UUID.PeerGroupID {

    /**
     * Log4J categorgy
     */
    private static final transient Logger LOG = Logger.getLogger(PeerGroupID.class.getName());

    /**
     * Intializes contents from provided ID.
     *
     * @param id the ID data
     */
    protected PeerGroupID(IDBytes id) {
        super(id);
    }

    /**
     * Creates a PeerGroupID. A PeerGroupID is provided
     *
     * @param groupUUID the PeerGroupID to use to construct the new PeerGroupID
     */
    protected PeerGroupID(UUID groupUUID) {
        super(groupUUID);
    }

    /**
     * See {@link net.jxta.id.IDFactory.Instantiator#newPeerGroupID()}.
     */
    public PeerGroupID() {
        this(UUIDFactory.newUUID());
    }

    /**
     * See {@link net.jxta.id.IDFactory.Instantiator#newPeerGroupID(byte[])}.
     * @param seed the seed
     */
    public PeerGroupID(byte[] seed) {
        super();

        MessageDigest digester = null;

        try {
            digester = MessageDigest.getInstance("SHA-1");
        } catch (NoSuchAlgorithmException caught) {
            digester = null;
        }

        if (digester == null) {
            throw new ProviderException("SHA1 digest algorithm not found");
        }

        byte[] digest = digester.digest(seed);

        // we keep only the 128 most significant bits
        byte[] buf16 = new byte[16];

        System.arraycopy(digest, 0, buf16, 0, 16);

        UUID groupUUID = UUIDFactory.newUUID(buf16);

        id.longIntoBytes(PeerGroupID.groupIdOffset, groupUUID.getMostSignificantBits());
        id.longIntoBytes(PeerGroupID.groupIdOffset + 8, groupUUID.getLeastSignificantBits());
    }

    /**
     * See {@link net.jxta.id.IDFactory.Instantiator#newPeerGroupID(net.jxta.peergroup.PeerGroupID,byte[])}.
     *
     * @param parent Parent PeerGroupID
     * @param seed  the seed
     */
    public PeerGroupID(PeerGroupID parent, byte[] seed) {
        this(seed);

        UUID parentUUID = parent.getUUID();

        id.longIntoBytes(PeerGroupID.parentgroupIdOffset, parentUUID.getMostSignificantBits());
        id.longIntoBytes(PeerGroupID.parentgroupIdOffset + 8, parentUUID.getLeastSignificantBits());
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
    public net.jxta.peergroup.PeerGroupID getParentPeerGroupID() {

        UUID parentUUID = new UUID(id.bytesIntoLong(PeerGroupID.parentgroupIdOffset)
                ,
                id.bytesIntoLong(PeerGroupID.parentgroupIdOffset + 8));

        // if zero, then there is no parent.
        if ((0 == parentUUID.getMostSignificantBits()) && (0 == parentUUID.getLeastSignificantBits())) {
            return null;
        }

        PeerGroupID groupID = new PeerGroupID(parentUUID);

        // convert to the generic world PGID as necessary
        return (net.jxta.peergroup.PeerGroupID) IDFormat.translateToWellKnown(groupID);
    }

    /**
     * Returns the UUID associated with this PeerGroupID.
     *
     * @return The UUID associated with this PeerGroupID.
     */
    @Override
    public UUID getUUID() {
        return super.getUUID();
    }

}
