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
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peergroup.PeerGroupID;

import java.io.IOException;


/**
 * The common container for managing content within JXTA. A Codat consists of:
 * <ul>
 * <li>Content data for the Codat in the form of a JXTA
 * {@link net.jxta.document.Document}.</li>
 * <li>A persistent canonical identifier for the Codat in the form of a
 * {@link net.jxta.codat.CodatID}.</li>
 * <li>An optional CodatID for an associated Codat for which this Codat is
 * metadata.</li>
 * </ul>
 *
 * @see net.jxta.codat.CodatID
 * @see net.jxta.document.Document
 */
public class Codat {

    /**
     * CodatID of this Codat. A persistent canonical identifier for this Codat.
     */
    private final CodatID id;

    /**
     * CodatID for an associated Codat for which this Codat is metadata. This
     * may be the CodatId of another Codat in the same Peer Group or
     * <tt>null</tt>.
     */
    private final CodatID metaId;

    /**
     * Contains the data of this Codat.
     */
    private final Document doc;

    /**
     * Constructs a Codat instance with a new CodatId given a PeerGroupID and
     * a Document.
     *
     * @param groupID  The peer group to which this Codat will belong.
     * @param about    The CodatID of an associated Codat for which this Codat is
     *                 metadata or <tt>null</tt> if there is no associated Codat.
     * @param document Document which contains the content data for this Codat.
     * @throws IOException if there is an error accessing the document.
     */
    public Codat(PeerGroupID groupID, CodatID about, Document document) throws IOException {
        this(IDFactory.newCodatID(groupID, document.getStream()), about, document);
    }

    /**
     * Constructs a Codat instance for an existing Codat given it's
     * CodatID and a document.
     * <p/>
     * <p/>This implementation does not verify that the CodatID matches the
     * provided Document.
     *
     * @param id       CodatId of the new Codat.
     * @param about    CodatID of an associated Codat for which this Codat is metadata.
     * @param document Document which contains the content data for this Codat.
     */
    public Codat(CodatID id, CodatID about, Document document) {
        if (null == id) {
            throw new IllegalArgumentException("CodatID may not be null.");
        }

        if (null == document) {
            throw new IllegalArgumentException("Document may not be null.");
        }

        this.id = id;
        this.metaId = about;
        this.doc = document;
    }

    /**
     * Returns the CodatID of this Codat.
     *
     * @return The CodatID of this Codat.
     */
    public ID getCodatID() {
        return id;
    }

    /**
     * Returns the CodatID of an associated Codat for which this Codat is
     * metadata or <tt>null</tt> if there is no associated Codat.
     *
     * @return CodatID The CodatID of an associated Codat for which this Codat
     *         is metadata or <tt>null</tt> if there is no associated Codat.
     */
    public ID getMetaID() {
        return metaId;
    }

    /**
     * Returns a Document containing the data of this Codat.
     *
     * @return A Document containing the data of this Codat.
     */
    public Document getDocument() {
        return doc;
    }
}
