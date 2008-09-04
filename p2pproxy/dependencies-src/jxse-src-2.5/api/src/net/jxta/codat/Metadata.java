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


import net.jxta.document.Document;
import net.jxta.peergroup.PeerGroupID;

import java.io.IOException;


/**
 * Medata Codats are special codats that contain information about another
 * Codat. Multiple medata Codats can refer to the same Codat. Medata codats can
 * hold any kind of information about a codat, such as a symbolic name,
 * description, index and searching information, etc.
 *
 * @see net.jxta.codat.Codat
 * @see net.jxta.codat.CodatID
 * @see net.jxta.document.Document
 */
public class Metadata extends Codat {

    /**
     * Constructs a Metadata instance with a new CodatId given a PeerGroupID,
     * the CodatID of the associated Codat and a Document.
     *
     * @param groupID  The peer group to which this Codat will belong.
     * @param about    The CodatID of an associated Codat for which this Codat is
     *                 metadata or <tt>null</tt> if there is no associated Codat.
     * @param document Document which contains the content data for this Codat.
     * @throws IOException if there is an error accessing the document.
     */
    public Metadata(PeerGroupID groupID, CodatID about, Document document) throws IOException {
        super(groupID, about, document);
    }

    /**
     * Constructs a Metadata instance for an existing Codat given it's
     * CodatID, the CodatID of the associated Codat and a Document.
     * <p/>
     * <p/>This implementation does not verify that the CodatID matches the
     * provided Document.
     *
     * @param id       CodatId of the new Codat.
     * @param about    CodatID of an associated Codat for which this Codat is metadata.
     * @param document Document which contains the content data for this Codat.
     */
    public Metadata(CodatID id, CodatID about, Document document) {
        super(id, about, document);
    }
}
