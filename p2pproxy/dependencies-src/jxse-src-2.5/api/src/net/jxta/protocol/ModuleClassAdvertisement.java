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


import net.jxta.document.Element;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.id.ID;
import net.jxta.platform.ModuleClassID;


/**
 * Formally documents the existence of a module class (identified by the
 * {@link net.jxta.platform.ModuleClassID} and may provide additional
 * descriptive metadata about the Module Class.
 *
 * @see net.jxta.platform.ModuleClassID
 */
public abstract class ModuleClassAdvertisement extends ExtendableAdvertisement implements Cloneable {

    /**
     *  The module class id associated with the is advertisement.
     */
    private ModuleClassID id = null;
    
    /**
     *  Informal, non-canonical name of module.
     */
    private String name = null;

    /**
     *  Descriptive meta-data about this module.
     */
    private Element description = null;

    /**
     *  Returns the identifying type of this Advertisement.
     *
     *  @return The type of advertisement.
     */
    public static String getAdvertisementType() {
        return "jxta:MCA";
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public ModuleClassAdvertisement clone() {
        try {
            ModuleClassAdvertisement clone = (ModuleClassAdvertisement) super.clone();

            clone.setModuleClassID(getModuleClassID());
            clone.setName(getName());
            clone.setDesc(description);

            return clone;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }   
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ID getID() {
        return id;
    }

    /**
     * returns the name of the class
     *
     * @return String name of the class
     */
    public String getName() {
        return name;
    }

    /**
     * sets the name of the class
     *
     * @param name name of the class to be set
     *
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * returns the description
     *
     * @return String the description
     */
    public String getDescription() {
        if (null != description) {
            return (String) description.getValue();
        } else {
            return null;
        }
    }
    
    /**
     * sets the description
     *
     * @param description the description
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
     * returns the description
     *
     * @return the description
     */
    public StructuredDocument getDesc() {
        if (null != description) {
            StructuredDocument newDoc = StructuredDocumentUtils.copyAsDocument(description);
            
            return newDoc;
        } else {
            return null;
        }
    }
    
    /**
     * sets the description
     *
     * @param desc the description
     */
    public void setDesc(Element desc) {
        
        if (null != desc) {
            this.description = StructuredDocumentUtils.copyAsDocument(desc);
        } else {
            this.description = null;
        }
    }
    
    /**
     * returns the id of the class
     *
     * @return ModuleClassID the class id
     */
    public ModuleClassID getModuleClassID() {
        return id;
    }

    /**
     * sets the id of the class
     *
     * @param id The id of the class
     */
    public void setModuleClassID(ModuleClassID id) {
        this.id = id;
    }
}
