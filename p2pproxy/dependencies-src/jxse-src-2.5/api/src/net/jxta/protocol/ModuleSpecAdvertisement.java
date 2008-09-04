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
import net.jxta.platform.ModuleSpecID;


/**
 * Provides the references that describe a module specification. Typically this
 * includes references to the documentation needed in order to create conforming
 * implementations of the specification. A secondary use is, optionally, to make 
 * running instances usable remotely, by publishing any or all of the following:
 * <br><ul>
 *    <li> PipeAdvertisement
 *    <li> ModuleSpecID of a proxy module
 *    <li> ModuleSpecID of an authenticator module
 * </ul>
 * <p/>
 * Not all modules are usable remotely, it is up to the specification creator to
 * make that choice. However, if the specification dictates it, all
 * implementations can be expected to support it.
 * <p/>
 * Note that the Standard PeerGroup implementation included with the JXSE
 * reference implementation does <em>not</em> support replacing a group service 
 * with a pipe to a remote instance. However, nothing prevents a particular
 * implementation of a group from using a proxy module in place of the fully
 * version; provided that the API (and therefore the ClassIDs) of the proxy and
 * local versions are identical.
 * <p/>
 * Note also that in the case of the local+proxy style, it is up to the
 * implementation of both sides to figure-out which pipe to listen to or connect
 * to. The safest method is probably for the full version to seek its own
 * ModuleSpecAdvertisement, and for the proxy version to accept the full
 * version's ModuleSpecAdvertisement as a parameter. Alternatively, if the proxy
 * version is completely dedicated to the specification that it proxies, both
 * sides may have the PipeID and type hard-coded.
 * 
 * @see net.jxta.platform.ModuleSpecID
 * @see net.jxta.protocol.PipeAdvertisement
 * @see net.jxta.protocol.ModuleClassAdvertisement
 * @see net.jxta.protocol.ModuleImplAdvertisement
 * @see net.jxta.document.Advertisement
 */
public abstract class ModuleSpecAdvertisement extends ExtendableAdvertisement implements Cloneable {

    private ModuleSpecID id = null;
    private String name = null;
    private Element description = null;
    private String creator = null;
    private String uri = null;
    private String version = null;
    private PipeAdvertisement pipeAdv = null;
    private ModuleSpecID proxySpecID = null;
    private ModuleSpecID authSpecID = null;
    private StructuredDocument param = null;

    /**
     *  Returns the identifying type of this Advertisement.
     *
     * @return String the type of advertisement
     **/
    public static String getAdvertisementType() {
        return "jxta:MSA";
    }
    
    /**
     * {@inheritDoc}
     **/
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }
    
    /**
     * {@inheritDoc}
     **/
    @Override
    public ModuleSpecAdvertisement clone() {
        try {
            ModuleSpecAdvertisement clone = (ModuleSpecAdvertisement) super.clone();

            clone.setModuleSpecID(getModuleSpecID());
            clone.setName(getName());
            clone.setDesc(getDesc());
            clone.setCreator(getCreator());
            clone.setSpecURI(getSpecURI());
            clone.setVersion(getVersion());
            clone.setPipeAdvertisement(getPipeAdvertisement());
            clone.setProxySpecID(getProxySpecID());
            clone.setAuthSpecID(getAuthSpecID());
            clone.setParam(param);

            return clone;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }
    }

    /**
     * returns a unique id for that adv for the purpose of indexing.
     * The spec id uniquely identifies this advertisement.
     *
     * @return ID the spec id as a basic ID.
     *
     */    
    @Override
    public ID getID() {
        return id;
    }
    
    /**
     * returns the id of the spec
     *
     * @return ModuleSpecID the spec id
     **/
    public ModuleSpecID getModuleSpecID() {
        return id;
    }

    /**
     * sets the id of the spec
     *
     * @param id The id of the spec
     **/
    public void setModuleSpecID(ModuleSpecID id) {
        this.id = id;
    }

    /**
     * returns the name of the module spec
     *
     * @return String name of the module spec
     **/
    public String getName() {
        return name;
    }

    /**
     * sets the name of the module spec
     *
     * @param name name of the module spec to be set
     **/
    public void setName(String name) {
        this.name = name;
    }

    /**
     * returns the description
     *
     * @return String the description
     **/
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
     **/
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
     **/
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
     **/
    public void setDesc(Element desc) {
        
        if (null != desc) {
            this.description = StructuredDocumentUtils.copyAsDocument(desc);
        } else {
            this.description = null;
        }
    }

    /**
     * Returns the creator of the module spec, in case someone cares.
     *
     * @return String the creator.
     **/
    public String getCreator() {
        return creator;
    }

    /**
     * Sets the creator of this module spec.
     * Note: the usefulness of this is unclear.
     *
     * @param creator name of the creator of the module
     **/
    public void setCreator(String creator) {
        this.creator = creator;
    }

    /**
     * returns the uri. This uri normally points at the actual specification
     * that this advertises.
     *
     * @return String uri
     **/
    public String getSpecURI() {
        return uri;
    }
 
    /**
     * sets the uri
     *
     * @param uri string uri
     **/
    public void setSpecURI(String uri) {
        this.uri = uri;
    }

    /**
     * returns the specification version number
     *
     * @return String version number
     **/
    public String getVersion() {
        return version;
    }
  
    /**
     * sets the version of the module
     *
     * @param version  version number
     **/
    public void setVersion(String version) {
        this.version = version;
    }

    /**
     * returns the param element.
     *
     * @return Element parameters as an Element of unspecified content.
     **/
    public StructuredDocument getParam() {
        return (param == null ? null : StructuredDocumentUtils.copyAsDocument(param));
    }
    
    /**
     * Privileged version of {@link #getParam()} that does not clone the elements.
     *
     * @return StructuredDocument A stand-alone structured document of
     * unspecified content.
     **/
    protected StructuredDocument getParamPriv() {
        return param;
    }

    /**
     * sets the param element.
     *
     * @param param Element of an unspecified content.
     **/
    public void setParam(Element param) {
        this.param = (param == null ? null : StructuredDocumentUtils.copyAsDocument(param));
    }

    /**
     * returns the embedded pipe advertisement if any.
     *
     * @return PipeAdvertisement the Pipe Advertisement. null if none exists.
     **/
    public PipeAdvertisement getPipeAdvertisement() {
        return (pipeAdv == null ? null : pipeAdv.clone());
    }

    /**
     * sets an embedded pipe advertisement.
     *
     * @param pipeAdv the Pipe Advertisement. null is authorized.
     **/
    public void setPipeAdvertisement(PipeAdvertisement pipeAdv) {
        this.pipeAdv = (pipeAdv == null ? null : pipeAdv.clone());
    }

    /**
     * returns the specID of a proxy module.
     *
     * @return ModuleSpecID the spec id
     **/
    public ModuleSpecID getProxySpecID() {
        return proxySpecID;
    }

    /**
     * sets a proxy module specID
     *
     * @param proxySpecID The spec id
     **/
    public void setProxySpecID(ModuleSpecID proxySpecID) {
        this.proxySpecID = proxySpecID;
    }

    /**
     * returns the specID of an authenticator module.
     *
     * @return ModuleSpecID the spec id
     **/
    public ModuleSpecID getAuthSpecID() {
        return authSpecID;
    }

    /**
     * sets an authenticator module specID
     *
     * @param authSpecID The spec id
     **/
    public void setAuthSpecID(ModuleSpecID authSpecID) {
        this.authSpecID = authSpecID;
    }
}
