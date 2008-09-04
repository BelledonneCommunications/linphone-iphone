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

package net.jxta.impl.id.UUID;


import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;


/**
 *  An implementation of the {@link net.jxta.platform.ModuleSpecID} ID Type.
 */
public class ModuleSpecID extends net.jxta.platform.ModuleSpecID {
    private final static int moduleClassIdOffset = 0;
    private final static int moduleSpecIdOffset = IDFormat.uuidSize;
    private final static int padOffset = ModuleSpecID.moduleSpecIdOffset + IDFormat.uuidSize;
    private final static int padSize = IDFormat.flagsOffset - ModuleSpecID.padOffset;
    
    /**
     *  The id data
     */
    protected IDBytes id;
    
    /**
     *  Constructor. Used only internally.
     */
    protected ModuleSpecID() {
        super();
        id = new IDBytes(IDFormat.flagModuleSpecID);
    }
    
    /**
     * Initializes contents from provided ID.
     *
     * @param id    the ID data
     */
    protected ModuleSpecID(IDBytes id) {
        super();
        this.id = id;
    }
    
    /**
     * Creates a ModuleSpecID in a given class, with a given class unique id.
     * A UUID of a class and another UUID are provided.
     *
     * @param classUUID    the class to which this will belong.
     * @param specUUID     the unique id of this spec in that class.
     */
    protected ModuleSpecID(UUID classUUID, UUID specUUID) {
        
        this();
        id.longIntoBytes(ModuleSpecID.moduleClassIdOffset, classUUID.getMostSignificantBits());
        id.longIntoBytes(ModuleSpecID.moduleClassIdOffset + 8, classUUID.getLeastSignificantBits());
        
        id.longIntoBytes(ModuleSpecID.moduleSpecIdOffset, specUUID.getMostSignificantBits());
        id.longIntoBytes(ModuleSpecID.moduleSpecIdOffset + 8, specUUID.getLeastSignificantBits());
    }
    
    /**
     *  See {@link net.jxta.id.IDFactory.Instantiator#newModuleSpecID(net.jxta.platform.ModuleClassID)}.
     */
    public ModuleSpecID(ModuleClassID classID) {
        this(classID.getClassUUID(), UUIDFactory.newUUID());
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        if (target instanceof ModuleSpecID) {
            ModuleSpecID msidTarget = (ModuleSpecID) target;
            
            return id.equals(msidTarget.id);
        } else {
            return false;
        }
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return id.hashCode();
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public String getIDFormat() {
        return IDFormat.INSTANTIATOR.getSupportedIDFormat();
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public Object getUniqueValue() {
        return getIDFormat() + "-" + (String) id.getUniqueValue();
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public net.jxta.platform.ModuleClassID getBaseClass() {
        return new ModuleClassID(getClassUUID(), new UUID(0L, 0L));
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean isOfSameBaseClass(net.jxta.platform.ModuleClassID classId) {
        return getClassUUID().equals(((ModuleClassID) classId).getClassUUID());
    }
    
    /**
     *  {@inheritDoc}
     */
    @Override
    public boolean isOfSameBaseClass(net.jxta.platform.ModuleSpecID specId) {
        return getClassUUID().equals(((ModuleSpecID) specId).getClassUUID());
    }
    
    /**
     * get the class' unique id
     *
     * @since JXTA 1.0
     *
     * @return UUID module class' unique id
     */
    protected UUID getClassUUID() {
        UUID result = new UUID(id.bytesIntoLong(ModuleSpecID.moduleClassIdOffset)
                ,
                id.bytesIntoLong(ModuleSpecID.moduleClassIdOffset + 8));
        
        return result;
    }
    
    /**
     * get the spec unique id
     *
     * @since JXTA 1.0
     *
     * @return UUID module spec unique id
     */
    protected UUID getSpecUUID() {
        UUID result = new UUID(id.bytesIntoLong(ModuleSpecID.moduleSpecIdOffset)
                ,
                id.bytesIntoLong(ModuleSpecID.moduleSpecIdOffset + 8));
        
        return result;
    }
}
