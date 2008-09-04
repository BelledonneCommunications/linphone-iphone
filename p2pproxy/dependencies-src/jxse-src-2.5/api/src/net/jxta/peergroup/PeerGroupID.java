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
package net.jxta.peergroup;


import java.net.URI;

import net.jxta.id.ID;
import net.jxta.id.IDFactory;


/**
 *  This class implements a PeerGroup ID. Each peer group is assigned a
 *  unique id.
 *
 *  @see         net.jxta.id.ID
 *  @see         net.jxta.id.IDFactory
 *  @see         net.jxta.peer.PeerID
 *
 * @since JXTA 1.0
 */
public abstract class PeerGroupID extends ID {
    
    /**
     * Creates an ID by parsing the given URI.
     *
     * <p>This convenience factory method works as if by invoking the
     * {@link net.jxta.id.IDFactory#fromURI(URI)} method; any 
     * {@link java.net.URISyntaxException} thrown is caught and wrapped in a 
     * new {@link IllegalArgumentException} object, which is then thrown.  
     *
     * <p> This method is provided for use in situations where it is known that
     * the given string is a legal ID, for example for ID constants declared
     * within in a program, and so it would be considered a programming error
     * for the URI not to parse as such.  The {@link net.jxta.id.IDFactory}, 
     * which throws {@link java.net.URISyntaxException} directly, should be used 
     * situations where a ID is being constructed from user input or from some 
     * other source that may be prone to errors. 
     *
     * @param  fromURI   The URI to be parsed into an ID
     * @return The new ID
     *
     * @throws  NullPointerException If {@code fromURI} is {@code null}.
     * @throws  IllegalArgumentException If the given URI is not a valid ID.
     */
    public static PeerGroupID create(URI fromURI) {
        return (PeerGroupID) ID.create(fromURI);
    }
    
    /**
     *  {@inheritDoc}
     */
    public PeerGroupID intern() {
        return (PeerGroupID) super.intern();
    }
    
    /**
     * The well known Unique Identifier of the world peergroup.
     * This is a singleton within the scope of a VM.
     */
    public final static PeerGroupID worldPeerGroupID = (new WorldPeerGroupID()).intern();
    
    /**
     * The well known Unique Identifier of the net peergroup.
     * This is a singleton within the scope of this VM.
     */
    public final static PeerGroupID defaultNetPeerGroupID = (new NetPeerGroupID()).intern();
    
    /**
     *  Returns the parent peer group id of this peer group id, if any.
     *
     *  @return the id of the parent peergroup or null if this group has no
     *  parent group.
     */
    public abstract PeerGroupID getParentPeerGroupID();
}


final class WorldPeerGroupID extends PeerGroupID {
    
    /**
     * The name associated with this ID Format.
     */
    final static String JXTAFormat = "jxta";
    
    private static final String UNIQUEVALUE = "WorldGroup";
    
    /**
     *  WorldPeerGroupID is not intended to be constructed. You should use the 
     *  {@link PeerGroupID.worldPeerGroupID} constant instead.
     */
    WorldPeerGroupID() {}
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        return (this == target); // worldPeerGroupID is only itself.
    }
    
    /**
     * deserialization has to point back to the singleton in this VM
     */
    private Object readResolve() {
        return PeerGroupID.worldPeerGroupID;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public String getIDFormat() {
        return JXTAFormat;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Object getUniqueValue() {
        return getIDFormat() + "-" + UNIQUEVALUE;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public PeerGroupID getParentPeerGroupID() {
        return null;
    }
}


final class NetPeerGroupID extends PeerGroupID {

    /**
     * The name associated with this ID Format.
     */
    final static String JXTAFormat = "jxta";
    
    private static final String UNIQUEVALUE = "NetGroup";
    
    /**
     *  NetPeerGroupID is not intended to be constructed. You should use the 
     *  {@link PeerGroupID.defaultNetPeerGroupID} constant instead.
     */
    NetPeerGroupID() {}
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        return (this == target); // netPeerGroupID is only itself.
    }
    
    /**
     * deserialization has to point back to the singleton in this VM
     */
    private Object readResolve() {
        return PeerGroupID.defaultNetPeerGroupID;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public String getIDFormat() {
        return JXTAFormat;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Object getUniqueValue() {
        return getIDFormat() + "-" + UNIQUEVALUE;
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public PeerGroupID getParentPeerGroupID() {
        return PeerGroupID.worldPeerGroupID;
    }
}
