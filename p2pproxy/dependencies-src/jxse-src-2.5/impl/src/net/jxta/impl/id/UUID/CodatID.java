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


import java.io.InputStream;
import java.security.MessageDigest;

import java.io.IOException;
import java.security.ProviderException;
import java.security.NoSuchAlgorithmException;


/**
 *  An implementation of the {@link net.jxta.codat.CodatID} ID Type.
 */
public class CodatID extends net.jxta.codat.CodatID {
       
    /**
     * size of a SHA1 hash. I would use MessageDigest.getDigestLength, but
     * possible exceptions make it difficult to do.
     */
    protected final static int hashSize = 20;
    
    /**
     *  Location of the group id in the byte array.
     */
    protected final static int groupIdOffset = 0;
    
    /**
     *  Location of the randomly chosen portion of the id within the byte array.
     */
    protected final static int idOffset = CodatID.groupIdOffset + IDFormat.uuidSize;
    
    /**
     *  Location of the hash value portion of the id within the byte array.
     */
    protected final static int codatHashOffset = CodatID.idOffset + IDFormat.uuidSize;
    
    /**
     *  Location of the beginning of pad (unused space) within the byte array.
     */
    protected final static int padOffset = CodatID.codatHashOffset + CodatID.hashSize;
    
    /**
     * Size of the pad.
     */
    protected final static int padSize = IDFormat.flagsOffset - CodatID.padOffset;
    
    /**
     *  The id data
     */
    protected IDBytes id;
    
    /**
     * Internal constructor
     */
    protected CodatID() {
        super();
        id = new IDBytes(IDFormat.flagCodatID);
    }
    
    /**
     * Initializes contents from provided bytes.
     *
     * @param id    the ID data
     */
    protected CodatID(IDBytes id) {
        super();
        this.id = id;
    }
    
    protected CodatID(UUID groupUUID, UUID idUUID) {
        this();
        
        id.longIntoBytes(CodatID.groupIdOffset, groupUUID.getMostSignificantBits());
        id.longIntoBytes(CodatID.groupIdOffset + 8, groupUUID.getLeastSignificantBits());
        
        id.longIntoBytes(CodatID.idOffset, idUUID.getMostSignificantBits());
        id.longIntoBytes(CodatID.idOffset + 8, idUUID.getLeastSignificantBits());
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newCodatID(net.jxta.peergroup.PeerGroupID)}.
     */
    public CodatID(PeerGroupID groupID) {
        this(groupID.getUUID(), UUIDFactory.newUUID());
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newCodatID(net.jxta.peergroup.PeerGroupID,byte[])}.
     */
    public CodatID(PeerGroupID groupID, byte[] seed) {
        this();
        
        UUID groupUUID = groupID.getUUID();
        
        id.longIntoBytes(CodatID.groupIdOffset, groupUUID.getMostSignificantBits());
        id.longIntoBytes(CodatID.groupIdOffset + 8, groupUUID.getLeastSignificantBits());
        
        System.arraycopy(seed, 0, id.bytes, CodatID.idOffset, Math.min(IDFormat.uuidSize, seed.length));
        
        // make it a valid UUID
        id.bytes[CodatID.idOffset + 6] &= 0x0f;
        id.bytes[CodatID.idOffset + 6] |= 0x40; /* version 4 */
        id.bytes[CodatID.idOffset + 8] &= 0x3f;
        id.bytes[CodatID.idOffset + 8] |= 0x80; /* IETF variant */
        id.bytes[CodatID.idOffset + 10] &= 0x3f;
        id.bytes[CodatID.idOffset + 10] |= 0x80; /* multicast bit */
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newCodatID(net.jxta.peergroup.PeerGroupID,InputStream)}.
     */
    public CodatID(PeerGroupID groupID, InputStream in) throws IOException {
        this(groupID);
        
        setHash(in);
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newCodatID(net.jxta.peergroup.PeerGroupID,InputStream)}.
     */
    public CodatID(PeerGroupID groupID, byte[] seed, InputStream in) throws IOException {
        this(groupID, seed);
        
        setHash(in);
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        if (target instanceof CodatID) {
            CodatID codatTarget = (CodatID) target;
            
            return id.equals(codatTarget.id);
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
    public net.jxta.id.ID getPeerGroupID() {
        UUID groupUUID = new UUID(id.bytesIntoLong(CodatID.groupIdOffset), id.bytesIntoLong(CodatID.groupIdOffset + 8));
        
        PeerGroupID groupID = new PeerGroupID(groupUUID);
        
        // convert to the generic world PGID as necessary
        return IDFormat.translateToWellKnown(groupID);
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean isStatic() {
        for (int eachHashByte = CodatID.codatHashOffset; eachHashByte < (CodatID.padOffset); eachHashByte++) {
            if (0 != id.bytes[eachHashByte]) {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     *  Calculates the SHA-1 hash of a stream.
     *
     *  @param in The InputStream.
     */
    protected void setHash(InputStream in) throws IOException {
        MessageDigest dig = null;

        try {
            dig = MessageDigest.getInstance("SHA-1");
        } catch (NoSuchAlgorithmException caught) {
            dig = null;
        }
        
        if (dig == null) {
            throw new ProviderException("SHA-1 digest algorithm not found");
        }
        
        dig.reset();
        
        byte[] chunk = new byte[1024];
        
        try {
            do {
                int read = in.read(chunk);

                if (read == -1) {
                    break;
                }
                
                dig.update(chunk, 0, read);
            } while (true);
        } finally {
            in.close();
        }
        
        byte[] result = dig.digest();

        System.arraycopy(result, 0, id.bytes, CodatID.codatHashOffset, CodatID.hashSize);
    }
}
