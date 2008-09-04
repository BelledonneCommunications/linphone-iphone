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
 * This class implements a Pipe ID. Each pipe is assigned a unique id.
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 * @see net.jxta.id.ID
 * @see net.jxta.id.IDFactory
 * @see net.jxta.peergroup.PeerGroupID
 */
public final class PipeBinaryID extends net.jxta.pipe.PipeID {

    /**
     * LOG object for this class.
     */
    private final static transient Logger LOG = Logger.getLogger(PipeBinaryID.class.getName());

    /**
     * The id data
     */
    protected String id;

    /**
     * Used only internally
     */
    protected PipeBinaryID() {
        super();
    }

    /**
     * Creates a ID from a string. Note that the ID is not currently validated.
     *
     * @param id Value of ID.
     */

    protected PipeBinaryID(String id) {
        super();
        this.id = id;

    }

    /**
     * Constructor. Intializes contents from provided ID.
     *
     * @param id the ID data
     */
    PipeBinaryID(BinaryID id) {
        super();
        this.id = id.getID();
    }

    /**
     * Constructor. Creates a PipeID. A PeerGroupID is provided.  Note that only
     * the peer group's primary node is used to  build this node. We don't want
     * to be appending great grand parents.
     *
     * @param parent         the group to which this will belong.
     * @param data           DOCUMENT ME!
     * @param lengthIncluded DOCUMENT ME!
     */
    public PipeBinaryID(net.jxta.peergroup.PeerGroupID parent, byte[] data, boolean lengthIncluded) {
        this();

        String parentStr = IDFormat.childGroup(parent);

        id = BinaryIDFactory.newBinaryID(BinaryID.flagPipeID, data, lengthIncluded).getID() + "." + parentStr.replace('-', '.');
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        return target instanceof PipeBinaryID && getUniqueValue().equals(((PipeBinaryID) target).getUniqueValue());
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

        return new StringBuilder().append(getIDFormat()).append("-").append(id).toString();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public net.jxta.id.ID getPeerGroupID() {
        try {
            if (id == null) {
                return net.jxta.id.ID.nullID;
            }
            String idd = id;
            int parentStart = idd.indexOf('.');

            if (parentStart != -1) {
                idd = idd.substring(parentStart + 1);
            } else {
                return null;
            }

            URI url = new URI("urn:jxta:" + idd.replace('.', '-'));
            net.jxta.peergroup.PeerGroupID peerGroupID = (net.jxta.peergroup.PeerGroupID) net.jxta.id.IDFactory.fromURI(url);

            return peerGroupID;
        } catch (Exception e) {
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "cannot convert sub group. ID value = " + id, e);
            }
            return null;
        }
    }

    /**
     * returns the coded ID without the binaryid tag.
     *
     * @return Returns the raw string used to create the urn!
     */
    protected String getID() {
        return id;
    }
}
