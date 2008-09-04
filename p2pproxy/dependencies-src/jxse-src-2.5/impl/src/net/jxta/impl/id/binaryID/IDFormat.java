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


import java.util.logging.Logger;


/**
 * The 'BinaryID' format is a general purpose JXTA ID Format. It implements all of
 * the six standard ID types. It was originally created for the Java 2 SE
 * reference implementation. The 'BinaryID' format uses randomly generated BinaryIDs
 * as the mechanism for generating canonical values for the ids it provides.
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 */
public class IDFormat {

    /**
     * LOG4J Logger
     */
    private final static transient Logger LOG = Logger.getLogger(IDFormat.class.getName());

    /**
     * This table maps our local private versions of the well known ids to the
     * globally known version.
     */

    final static Object[][] wellKnownIDs = {
        { net.jxta.peergroup.PeerGroupID.worldPeerGroupID, net.jxta.impl.id.UUID.IDFormat.worldPeerGroupID}
                ,
        { net.jxta.peergroup.PeerGroupID.defaultNetPeerGroupID, net.jxta.impl.id.UUID.IDFormat.defaultNetPeerGroupID}
    };

    /**
     * The instantiator for this ID Format which is used by the IDFactory.
     */
    public static final net.jxta.impl.id.binaryID.Instantiator INSTANTIATOR = new net.jxta.impl.id.binaryID.Instantiator();

    /**
     * Private Constructor. This class cannot be instantiated.
     */
    private IDFormat() {}

    /**
     * Translate from well known ID to our locally encoded versions.
     *
     * @param input the id to be translated.
     * @return the translated ID or the input ID if no translation was needed.
     */

    static net.jxta.id.ID translateFromWellKnown(net.jxta.id.ID input) {
        for (int eachWellKnown = 0; eachWellKnown < wellKnownIDs.length; eachWellKnown++) {
            net.jxta.id.ID aWellKnown = (net.jxta.id.ID) wellKnownIDs[eachWellKnown][0];

            if (aWellKnown.equals(input)) {
                return (net.jxta.id.ID) wellKnownIDs[eachWellKnown][1];
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

    static net.jxta.id.ID translateToWellKnown(net.jxta.id.ID input) {
        for (int eachWellKnown = 0; eachWellKnown < wellKnownIDs.length; eachWellKnown++) {
            net.jxta.id.ID aLocalEncoding = (net.jxta.id.ID) wellKnownIDs[eachWellKnown][1];

            if (aLocalEncoding.equals(input)) {
                return (net.jxta.id.ID) wellKnownIDs[eachWellKnown][0];
            }
        }

        return input;
    }

    /**
     * Utility method used to strip only the most significant peer group ID.
     * This prevents us from continiously appending grandparents to each child.
     * <p/>
     * <p/>
     * This method is used in PipeID and PeerID.
     * </p>
     *
     * @param peerGroupID Peer group ID to pull the child from.
     * @return Child of the peer group.
     */
    public static String childGroup(net.jxta.peergroup.PeerGroupID peerGroupID) {
        String parentStr = (String) peerGroupID.getUniqueValue();
        // Child is the first ID
        int first = parentStr.indexOf('.');
        String child = null;

        if (first != -1) {
            child = parentStr.substring(0, first);

        } else {
            child = parentStr;
        }
        return child;
    }
}
