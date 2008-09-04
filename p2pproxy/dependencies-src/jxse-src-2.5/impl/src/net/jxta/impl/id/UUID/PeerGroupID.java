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

package net.jxta.impl.id.UUID;


import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;


/**
 *  An implementation of the {@link net.jxta.peergroup.PeerGroupID} ID Type.
 */
public class PeerGroupID extends net.jxta.peergroup.PeerGroupID {

    /**
     *  Location of the group id UUID within the id bytes.
     */
    protected final static int groupIdOffset = 0;
    
    /**
     *  Location of the parent group id UUID within the id bytes.
     */
    protected final static int parentgroupIdOffset = PeerGroupID.groupIdOffset + IDFormat.uuidSize;
    
    /**
     *  Location of the begining of the pad space.
     */
    protected final static int padOffset = PeerGroupID.parentgroupIdOffset + IDFormat.uuidSize;
    
    /**
     *  size of the pad space.
     */
    protected final static int padSize = IDFormat.flagsOffset - PeerGroupID.padOffset;
    
    /**
     *  The id data
     */
    protected IDBytes id;
    
    /**
     * Initializes contents from provided ID.
     *
     * @param id    the ID data
     */
    protected PeerGroupID(IDBytes id) {
        super();
        this.id = id;
    }
    
    /**
     * Creates a PeerGroupID. A PeerGroupID is provided
     *
     * @param groupUUID    the PeerGroupID to use to construct the new PeerGroupID
     */
    protected PeerGroupID(UUID groupUUID) {
        super();
        id = new IDBytes(IDFormat.flagPeerGroupID);
        
        id.longIntoBytes(PeerGroupID.groupIdOffset, groupUUID.getMostSignificantBits());
        id.longIntoBytes(PeerGroupID.groupIdOffset + 8, groupUUID.getLeastSignificantBits());
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newPeerGroupID()}.
     */
    public PeerGroupID() {
        this(UUIDFactory.newUUID());
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newPeerGroupID(net.jxta.peergroup.PeerGroupID)}.
     */
    public PeerGroupID(PeerGroupID parent) {
        this(UUIDFactory.newUUID());

        System.arraycopy(parent.id.bytes, 0, id.bytes, 16, IDFormat.uuidSize);
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newPeerGroupID(byte[])}.
     */
    public PeerGroupID(byte[] seed) {
        super();
        id = new IDBytes(IDFormat.flagPeerGroupID);
        
        for (int copySeed = Math.min(IDFormat.uuidSize, seed.length) - 1; copySeed >= 0; copySeed--) {
            id.bytes[copySeed + PeerGroupID.groupIdOffset] = seed[copySeed];
        }
        
        // make it a valid UUID
        id.bytes[PeerGroupID.groupIdOffset + 6] &= 0x0f;
        id.bytes[PeerGroupID.groupIdOffset + 6] |= 0x40; /* version 4 */
        id.bytes[PeerGroupID.groupIdOffset + 8] &= 0x3f;
        id.bytes[PeerGroupID.groupIdOffset + 8] |= 0x80; /* IETF variant */
        id.bytes[PeerGroupID.groupIdOffset + 10] &= 0x3f;
        id.bytes[PeerGroupID.groupIdOffset + 10] |= 0x80; /* multicast bit */
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newPeerGroupID(net.jxta.peergroup.PeerGroupID,byte[])}.
     */
    public PeerGroupID(PeerGroupID parent, byte[] seed) {
        this(seed);

        System.arraycopy(parent.id.bytes, 0, id.bytes, 16, IDFormat.uuidSize);
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        if (target instanceof PeerGroupID) {
            PeerGroupID peergroupTarget = (PeerGroupID) target;
            
            return id.equals(peergroupTarget.id);
        } else {
            return false;
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return id.hashCode();
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public String getIDFormat() {
        return IDFormat.INSTANTIATOR.getSupportedIDFormat();
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Object getUniqueValue() {
        return getIDFormat() + "-" + (String) id.getUniqueValue();
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public net.jxta.peergroup.PeerGroupID getParentPeerGroupID() {
        boolean zero = true;
        
        for (int eachByte = 0; eachByte < IDFormat.uuidSize; eachByte++) {
            if (id.bytes[eachByte + PeerGroupID.parentgroupIdOffset] != 0) {
                zero = false;
                break;
            }
        }
        
        // if zero, then there is no parent.
        if (zero) {
            return null;
        }
        
        UUID groupUUID = new UUID(id.bytesIntoLong(PeerGroupID.parentgroupIdOffset)
                ,
                id.bytesIntoLong(PeerGroupID.parentgroupIdOffset + 8));
        
        PeerGroupID groupID = new PeerGroupID(groupUUID);
        
        // convert to the generic world PGID as necessary
        return (net.jxta.peergroup.PeerGroupID) IDFormat.translateToWellKnown(groupID);
    }
    
    /**
     *  Returns the UUID associated with this PeerGroupID.
     *
     *  @return The UUID associated with this PeerGroupID.
     */
    protected UUID getUUID() {
        return new UUID(id.bytesIntoLong(PeerGroupID.groupIdOffset), id.bytesIntoLong(PeerGroupID.groupIdOffset + 8));
    }
}
