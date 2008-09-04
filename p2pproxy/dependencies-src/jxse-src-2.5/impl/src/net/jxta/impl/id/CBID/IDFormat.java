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


import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.impl.id.UUID.UUID;

import java.util.logging.Logger;


/**
 * A general purpose JXTA ID Format implementing all of the six standard ID
 * Types. It was originally created for the Java 2 SE reference implementation.
 * The 'cbid' format uses values generated from secure hash functions as the
 * mechanism for generating canonical values for the ids it provides.
 *
 * @see net.jxta.id.ID
 * @see <a href="http://spec.jxta.org/nonav/v1.0/docbook/JXTAProtocols.html#ids" target="_blank">JXTA Protocols Specification : IDs</a>
 */
public class IDFormat extends net.jxta.impl.id.UUID.IDFormat {

    /**
     * Log4J Logger
     */
    private static final transient Logger LOG = Logger.getLogger(IDFormat.class.getName());

    /**
     * Our local version of the world Peer Group ID. We need this for cases
     * where we have to make ids which are in the world peer group. We only
     * use this ID for those cases and never return this ID.
     */
    static final PeerGroupID worldPeerGroupID = new PeerGroupID(new UUID(0x5961626164616261L, 0x4A78746150325033L)); // YabadabaJXTAP2P!

    /**
     * Our local version of the net Peer Group ID. We need this for cases
     * where we have to make ids which are in the net peer group. We only
     * use this ID for those cases and never return this ID.
     */
    static final PeerGroupID defaultNetPeerGroupID = new PeerGroupID(new UUID(0x5961626164616261L, 0x4E50472050325033L)); // YabadabaNPG P2P!

    /**
     * This table maps our local private versions of the well known ids to the
     * globally known version.
     */
    private final static Object[][] wellKnownIDs = {
        { net.jxta.peergroup.PeerGroupID.worldPeerGroupID, worldPeerGroupID}
                ,
        { net.jxta.peergroup.PeerGroupID.defaultNetPeerGroupID, defaultNetPeerGroupID}
    };

    /**
     * The instantiator for this ID Format which is used by the IDFactory.
     */
    public static final IDFactory.URIInstantiator INSTANTIATOR = new Instantiator();

    /**
     * This class cannot be instantiated.
     */
    protected IDFormat() {}

    /**
     * Translate from well known ID to our locally encoded versions.
     *
     * @param input the id to be translated.
     * @return the translated ID or the input ID if no translation was needed.
     */
    static ID translateFromWellKnown(ID input) {
        for (Object[] wellKnownID : wellKnownIDs) {
            ID aWellKnown = (ID) wellKnownID[0];

            if (aWellKnown.equals(input)) {
                return (ID) wellKnownID[1];
            }
        }

        return input;
    }

    /**
     * Translate from locally encoded versions to the well known versions.
     *
     * @param input the id to be translated.
     * @return the translated ID or the input ID if no translation was needed.
     */
    static ID translateToWellKnown(ID input) {
        for (Object[] wellKnownID : wellKnownIDs) {
            ID aLocalEncoding = (ID) wellKnownID[1];

            if (aLocalEncoding.equals(input)) {
                return (ID) wellKnownID[0];
            }
        }

        return input;
    }
}
