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
 * A ModuleSpecID uniquely identifies a particular network behaviour
 * (wire protocol and choregraphy) that may be embodied by a Jxta Module.
 * There may be any number of implementations of a given SpecID. All
 * such implementations are assumed to be network compatible.
 * <p/>
 * <p/>
 * The Specification that corresponds to a given ModuleSpecID may be published
 * in a ModuleSpecAdvertisement. This advertisement is uniquely identified by
 * the ModuleSpecID that it describes.
 * <p/>
 * <p/>
 * The various implementations of a given SpecID may be published in
 * ModuleImplAdvertisements. These advertisements are identified by the
 * ModuleSpecID that they implement and a compatibility statement.
 * ModuleImplAdvertisements baring the same SpecID and compatibility statement
 * are theorethicaly interchangeable. However they may be subsequently discriminated
 * by a Description element.
 * <p/>
 * <p/>
 * A ModuleSpecID embeds a ModuleClassID which uniquely identifies a base Module
 * class. A base module class defines a local behaviour and one API per compatible
 * JXTA implementation.
 * <p/>
 * <p/>
 * A ModuleSpecID therefore uniquely identifies an abstract module, of which an
 * implementation compatible with the local JXTA implementation may be located and
 * instantiated.
 * <p/>
 * <p/>
 * In the standard PeerGroup implementation of the java reference implementation
 * the various services are specified as a list of ModuleSpecID, for each of which
 * the group locates and loads an implementation as part of the group's
 * initialization.
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 * @see net.jxta.peergroup.PeerGroup
 * @see net.jxta.platform.Module
 * @see net.jxta.platform.ModuleClassID
 * @see net.jxta.protocol.ModuleSpecAdvertisement
 * @see net.jxta.protocol.ModuleImplAdvertisement
 * @see net.jxta.id.ID
 * @see net.jxta.document.Advertisement
 */

public final class ModuleSpecBinaryID extends net.jxta.platform.ModuleSpecID {

    /**
     * Log4J categorgy
     */
    private final static transient Logger LOG = Logger.getLogger(ModuleSpecBinaryID.class.getName());

    protected BinaryID classID;
    protected BinaryID baseClassID;
    protected BinaryID specID;

    /**
     * Constructor. Used only internally.
     *
     * @since JXTA  1.0
     */
    protected ModuleSpecBinaryID() {
        super();
        specID = new BinaryID(BinaryID.flagModuleSpecID);
        classID = new BinaryID(BinaryID.flagModuleClassID);
        baseClassID = new BinaryID(BinaryID.flagModuleClassID);
    }

    ;

    /**
     * Constructor.
     * Intializes contents from provided ID.
     *
     * @param id the ID data
     * @since JXTA  1.0
     */
    protected ModuleSpecBinaryID(String id) {
        super();
        int start = id.indexOf('-');
        int parent = id.indexOf(start + 1, '-');
        int spec = id.indexOf(id.indexOf(parent + 1, '-') + 1, '-');

        classID = new BinaryID(id.substring(start + 1, parent));
        baseClassID = new BinaryID(id.substring(parent + 1, spec));
        specID = new BinaryID(id.substring(parent + 1));
    }

    /**
     * Constructor.
     * Creates a ModuleSpecID in a given class, with a given class unique id.
     * A BinaryID of a class and another BinaryID are provided.
     *
     * @param classID     the class to which this will belong.
     * @param baseClassID the unique id of this spec in that class.
     * @param specID      the spec ID
     */
    protected ModuleSpecBinaryID(BinaryID classID, BinaryID baseClassID, BinaryID specID) {
        this.classID = classID;
        this.baseClassID = baseClassID;
        this.specID = specID;
    }

    /*
     * Official constructors. No mention of BinaryID.
     */

    /**
     * Creates a new ModuleSpecID in a given class. A ModuleClassID is
     * provided. A new SpecID in that class is created.
     *
     * @since JXTA  1.0
     *
     * @param classID    the class to which this will belong.
     */
    
    /*
     public ModuleSpecID( net.jxta.platform.ModuleClassID moduleClassID ) {
     this.classID = moduleClassID.getClassID();
     this.baseClassID = getBaseClassID();
     this.specID = getSpecID();
     }
     */

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        if (!(target instanceof ModuleSpecBinaryID)) {
            return false;
        }
        
        ModuleSpecBinaryID targetObj = (ModuleSpecBinaryID) target;
        
        return classID.equals(targetObj.getClassID()) && baseClassID.equals(targetObj.getBaseClassID())
                && specID.equals(targetObj.getSpecID());
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
        return getIDFormat() + "-" + classID + "-" + baseClassID + "_" + specID;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public net.jxta.platform.ModuleClassID getBaseClass() {
        return new ModuleClassBinaryID(baseClassID, new BinaryID(BinaryID.flagModuleClassID)
                ,
                new BinaryID(BinaryID.flagModuleSpecID), new BinaryID(BinaryID.flagModuleSpecID));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isOfSameBaseClass(net.jxta.platform.ModuleClassID classId) {
        return baseClassID.equals(classId.getBaseClass());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isOfSameBaseClass(net.jxta.platform.ModuleSpecID specId) {
        return getClassID().equals(((ModuleSpecBinaryID) specId).getClassID());
    }

    /**
     * Getter for property classID.
     *
     * @return Value of property classID.
     */
    public net.jxta.impl.id.binaryID.BinaryID getClassID() {
        return classID;
    }

    /**
     * Getter for property baseClassID.
     *
     * @return Value of property baseClassID.
     */
    public net.jxta.impl.id.binaryID.BinaryID getBaseClassID() {
        return baseClassID;
    }

    /**
     * Getter for property specID.
     *
     * @return Value of property specID.
     */
    public net.jxta.impl.id.binaryID.BinaryID getSpecID() {
        return specID;
    }

}
