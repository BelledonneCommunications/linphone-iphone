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
package net.jxta.socket;


import net.jxta.document.MimeMediaType;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;

import java.net.SocketAddress;


/**
 * This class implements a JxtaSocket address (PeerGroup ID + Pipe Advertisement
 * + (optional) Peer ID).
 * <p/>
 * It provides an immutable object used by sockets for binding, connecting, or as
 * returned values.
 *
 * @author vwilliams
 * @see net.jxta.socket.JxtaSocket
 * @see net.jxta.socket.JxtaServerSocket
 * @see java.net.SocketAddress
 * @see java.net.Socket
 * @see java.net.ServerSocket
 */
public class JxtaSocketAddress extends SocketAddress {

    private final PeerGroupID peerGroupId;
    private final PipeAdvertisement pipeAdv;
    private final PeerID peerId;
    private final PeerAdvertisement peerAdv;

    private transient String pipeDoc; // convenience, see getPipeDocAsString()

    /**
     * Creates a new instance of JxtaSocketAddress.
     *
     * @param peerGroup peer group within which this socket exists
     * @param pipeAdv   the advertisement of a pipe for the socket to listen on
     */
    public JxtaSocketAddress(PeerGroup peerGroup, PipeAdvertisement pipeAdv) {
        this(peerGroup.getPeerGroupID(), pipeAdv, null);
    }

    /**
     * Creates a new instance of JxtaSocketAddress.
     *
     * @param peerGroup peer group within which this socket exists
     * @param pipeAdv   the advertisement of a pipe for the socket to listen on
     * @param peerId    the ID of a specific peer to be contacted over this socket
     *                  (may be null)
     * @deprecated use the PeerAdvertisement variant instead
     */
    @Deprecated
    public JxtaSocketAddress(PeerGroup peerGroup, PipeAdvertisement pipeAdv, PeerID peerId) {
        this(peerGroup.getPeerGroupID(), pipeAdv, peerId);
    }

    /**
     * Creates a new instance of JxtaSocketAddress.
     *
     * @param peerGroup peer group within which this socket exists
     * @param pipeAdv   the advertisement of a pipe for the socket to listen on
     * @param peerAdv   the PeerAdvertisement (may not be null)
     */
    public JxtaSocketAddress(PeerGroup peerGroup, PipeAdvertisement pipeAdv, PeerAdvertisement peerAdv) {
        if (peerGroup == null) {
            throw new IllegalArgumentException("peerGroupId is required.");
        }
        if (pipeAdv == null) {
            throw new IllegalArgumentException("pipeAdv is required.");
        }
        if (peerAdv == null) {
            throw new IllegalArgumentException("pipeAdv is required.");
        }

        this.pipeAdv = pipeAdv.clone();
        this.peerGroupId = peerGroup.getPeerGroupID();
        this.peerId = peerAdv.getPeerID();
        this.peerAdv = peerAdv.clone();
    }

    /**
     * Creates a new instance of JxtaSocketAddress.
     *
     * @param peerGroupId ID of peer group within which this socket exists
     * @param pipeAdv     the advertisement of a pipe for the socket to listen on
     * @deprecated use the PeerAdvertisement variant instead
     */
    @Deprecated
    public JxtaSocketAddress(PeerGroupID peerGroupId, PipeAdvertisement pipeAdv) {
        this(peerGroupId, pipeAdv, null);
    }

    /**
     * Creates a new instance of JxtaSocketAddress.
     *
     * @param peerGroupId ID of peer group within which this socket exists
     * @param pipeAdv     the advertisement of a pipe for the socket to listen on
     * @param peerId      the ID of a specific peer to be contacted over this socket
     *                    (may be null)
     * @throws IllegalArgumentException if peerGroupId or pipeAdv are null
     * @deprecated use the PeerAdvertisement variant instead
     */
    @Deprecated
    public JxtaSocketAddress(PeerGroupID peerGroupId, PipeAdvertisement pipeAdv, PeerID peerId) {

        if (peerGroupId == null) {
            throw new IllegalArgumentException("peerGroupId is required.");
        }
        if (pipeAdv == null) {
            throw new IllegalArgumentException("pipeAdv is required.");
        }

        this.pipeAdv = pipeAdv.clone();
        this.peerGroupId = peerGroupId;
        this.peerId = peerId;
        this.peerAdv = null;
    }

    /**
     * Returns the PeerGroupID element of the address
     *
     * @return the PeerGroupID
     */
    public PeerGroupID getPeerGroupId() {
        return this.peerGroupId;
    }

    /**
     * Returns the PipeAdvertisement element of the address
     *
     * @return the PipeAdvertisement
     */
    public PipeAdvertisement getPipeAdv() {
        // preserve immutability
        return pipeAdv.clone();
    }

    /**
     * Returns the PeerID element of the address. May be null.
     *
     * @return the PeerID, if there is one, null otherwise
     */
    public PeerID getPeerId() {
        return this.peerId;
    }

    /**
     * Returns the PeerID element of the address. May be null.
     *
     * @return the PeerAdvertisement, if there is one, null otherwise
     */
    public PeerAdvertisement getPeerAdvertisement() {
        return this.peerAdv;
    }

    @Override
    public boolean equals(Object obj) {

        if (this == obj) {
            return true;
        }

        if (obj instanceof JxtaSocketAddress) {
            JxtaSocketAddress addr = (JxtaSocketAddress) obj;

            if (!peerGroupId.equals(addr.getPeerGroupId())) {
                return false;
            }
            if (!pipeAdv.equals(addr.getPipeAdv())) {
                return false;
            }
            if (peerId != null) {
                if (!peerId.equals(addr.getPeerId())) {
                    return false;
                }
            } else if (addr.getPeerId() != null) {
                return false;
            }

            return true;
        }
        return false;
    }

    @Override
    public int hashCode() {

        int result = 17;

        result = 37 * result + peerGroupId.hashCode();
        result = 37 * result + pipeAdv.hashCode();
        if (peerId != null) {
            result = 37 * result + peerId.hashCode();
        }
        return result;
    }

    @Override
    public String toString() {
        StringBuilder result = new StringBuilder();
        String lineSep = System.getProperty("line.separator");

        result.append(lineSep).append("JxtaSocketAdress:").append(lineSep);
        result.append("    PeerGroupID: ").append(peerGroupId.toString()).append(lineSep);
        if (peerId != null) {
            result.append(lineSep).append("    PeerID: ").append(peerId.toString()).append(lineSep);
        }
        result.append("    Pipe Adv: ").append(lineSep).append("    ").append(getPipeDocAsString());
        return result.toString();
    }

    /*
     * A convenience function to lazily-initialize a variable with the string 
     * representaiton of the pipe advertisement and return it as needed.
     */
    private synchronized String getPipeDocAsString() {
        if (pipeDoc == null) {
            // Using plain text to avoid unpredictable white space 
            // nodes. [vwilliams]
            pipeDoc = pipeAdv.getDocument(MimeMediaType.TEXTUTF8).toString();
        }
        return pipeDoc;
    }
}
