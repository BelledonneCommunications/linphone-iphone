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

package net.jxta.protocol;


import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.id.ID;


/**
 *  Describes a JXTA Pipe. A pipe is described by a pipe id and by a pipe type.
 *  A pipe can also optionally have a name and/or a description.
 *
 *  @see net.jxta.pipe.PipeService
 *  @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pbp" target="_blank">JXTA Protocols Specification : Pipe Binding Protocol</a>
 */
public abstract class PipeAdvertisement extends ExtendableAdvertisement implements Cloneable {

    /**
     * XML tag to store the PipeID
     */
    public static final String IdTag = "Id";

    /**
     * XML tag to store the Pipe Type
     */
    public static final String TypeTag = "Type";

    /**
     * XML tag to store the name of the Pipe
     */
    public static final String NameTag = "Name";

    /**
     * XML tag to store the name of the Pipe
     */
    public static final String descTag = "Desc";

    private transient ID pipeId = ID.nullID;
    private String type = null;
    private String name = null;

    /**
     * Descriptive meta-data about this pipe.
     */
    private Element description = null;

    /**
     *  Returns the identifying type of this Advertisement.
     *
     *  @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:PipeAdvertisement";
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public PipeAdvertisement clone() {
        try {
            PipeAdvertisement likeMe = (PipeAdvertisement) super.clone();

            likeMe.setPipeID(getPipeID());
            likeMe.setType(getType());
            likeMe.setName(getName());
            likeMe.setDesc(getDesc());

            return likeMe;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object obj) {

        if (this == obj) {
            return true;
        }

        if (obj instanceof PipeAdvertisement) {
            PipeAdvertisement likeMe = (PipeAdvertisement) obj;

            if (!getPipeID().equals(likeMe.getPipeID())) {
                return false;
            }

            if (!getType().equals(likeMe.getType())) {
                return false;
            }

            String pipeName = getName();

            if (pipeName == null ? likeMe.getName() != null : !pipeName.equals(likeMe.getName())) {
                return false;
            }

            String pipeDescription = getDescription();

            return !(pipeDescription == null ? likeMe.getDescription() != null : pipeDescription.equals(likeMe.getDescription()));
        }
        return false;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public int hashCode() {

        int result = 17;

        result = 37 * result + getPipeID().hashCode();
        result = 37 * result + getType().hashCode();
        String pipeName = getName();

        if (pipeName != null) {
            result = 37 * result + pipeName.hashCode();
        }
        String pipeDescription = getDescription();

        if (pipeDescription != null) {
            result = 37 * result + pipeDescription.hashCode();
        }
        return result;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     *  {@inheritDoc}
     *
     *  <p/>The PipeID uniquely identifies this ADV.
     */
    @Override
    public ID getID() {
        ID result = getPipeID();

        if (null == result) {
            result = ID.nullID;
        }
        return result;
    }

    /**
     *  Return the pipe ID for the pipe described by this advertisement.
     *
     *  @return The pipe ID for the pipe described by this advertisement.
     */
    public ID getPipeID() {
        return pipeId;
    }

    /**
     *  Set the pipe ID for the pipe described by this advertisement.
     *
     *  @param pipeId The pipe ID for the pipe described by this advertisement.
     */
    public void setPipeID(ID pipeId) {
        this.pipeId = pipeId;
    }

    /**
     *  Return the pipe type for the pipe described by this advertisement.
     *
     *  @return The pipe type for the pipe described by this advertisement.
     */
    public String getType() {
        return type;
    }

    /**
     * Set the pipe type for the pipe described by this advertisement.
     *
     *  @param type The pipe type for the pipe described by this advertisement.
     */
    public void setType(String type) {
        this.type = type;
    }

    /**
     * Return the symbolic name for the pipe described by this advertisement.
     *
     * @return String The symbolic name for the pipe described by this advertisement.
     */
    public String getName() {
        return name;
    }

    /**
     *  Set the symbolic name for the pipe described by this advertisement.
     *
     *  @param name The symbolic name for the pipe described by this advertisement.
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * Returns the description
     *
     * @return String the description
     */
    public String getDescription() {
        if (null != description) {
            return (String) description.getValue();
        }
        return null;
    }

    /**
     * Set the description meta-data for the pipe described by this advertisement.
     *
     * @param description The description meta-data for the pipe described by this advertisement.
     */
    public void setDescription(String description) {

        if (null != description) {
            StructuredDocument newdoc = StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Desc", description);

            setDesc(newdoc);
        } else {
            this.description = null;
        }
    }

    /**
     *  Return the description meta-data for the pipe described by this advertisement.
     *
     *  @return The description meta-data for the pipe described by this advertisement.
     */
    public StructuredDocument getDesc() {
        if (null != description) {
            StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(description);

            return newDoc;
        }
        return null;
    }

    /**
     *  Set the description meta-data for the pipe described by this advertisement.
     *
     *  @param desc The description meta-data for the pipe described by this advertisement.
     */
    public void setDesc(Element desc) {

        if (null != desc) {
            this.description = StructuredDocumentUtils.copyAsDocument(desc);
        } else {
            this.description = null;
        }
    }
}
