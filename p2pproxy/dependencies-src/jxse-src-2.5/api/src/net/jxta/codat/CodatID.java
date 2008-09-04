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

package net.jxta.codat;


import java.net.URI;
import net.jxta.id.ID;


/**
 * An identifier that enables canonical references to be made to a Codat within
 * the context of a specific peer group.
 * <p/>
 * <p/>A CodatID is formed by the conjuction of:<ul>
 * <li>a PeerGroupID</li>
 * <li>a randomly chosen value that has a high probability of being unique</li>
 * <li>an optional SHA1 cryptographic hash of the Codat contents</li></ul>
 * <p/>
 * <p/>Codats which contain static content will normally include the hash value
 * as part of their CodatID.
 *
 * @see net.jxta.codat.Codat
 * @see net.jxta.peergroup.PeerGroupID
 */
public abstract class CodatID extends ID {

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
    public static CodatID create(URI fromURI) {
        return (CodatID) ID.create(fromURI);
    }
    
    /**
     *  {@inheritDoc}
     */
    public CodatID intern() {
        return (CodatID) super.intern();
    }
    
    /**
     * Returns PeerGroupID of the Peer Group to which this Codat ID belongs.
     *
     * @return PeerGroupID of the Peer Group which this ID is part of.
     */
    public abstract ID getPeerGroupID();

    /**
     * Returns <tt>true</tt> if this CodatID is associated with a static Codat.
     *
     * @return <tt>true</tt> if the codatId is for a Codat with static content
     *         otherwise <tt>false</tt>.
     */
    public abstract boolean isStatic();
}
