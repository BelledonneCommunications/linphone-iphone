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


import net.jxta.peergroup.PeerGroupID;
import java.util.logging.Logger;
import net.jxta.id.ID;


/**
 * This interface defines a Module Class Identifier.
 * A ModuleClassID uniquely identifies a particular local behaviour, that is,
 * a specific API for each execution environment for which an implementation
 * exists.
 * <p/>
 * <p/>
 * A ModuleClassID has two components: A base class identifier, and a role identifier.
 * The role identifier may be zero. By convention the API uses the ModuleClassID with
 * a zero role identifier to designate the base class in contexts where only the base class
 * is significant. Nonetheless, a ModuleClassID with a zero role identifier is a valid
 * ModulesClassID wherever a full ModuleClassID is expected. In many cases, only one role
 * in a given class is ever used. Using role zero in such cases is an optimization because
 * it may make the string representation of the ModuleClassID shorter.
 * <p/>
 * <p/>
 * Each service of a group, that is, the role it plays in the group, is uniquely identified
 * per the group definition.
 * This identifier may be used by other modules in the group to designate this one, or by the service
 * itself to identify its parameters in a PeerAdvertisement. In addition, by combining its
 * PeerGroupID with its own ModuleClassID, a service may create a predictible identifier unique
 * on their peer, suitable for registering listeners with the EndpointService or other services
 * with similar listener interfaces.
 * <p/>
 * <p/>
 * The standard PeerGroup implementation of the java reference implementation
 * assigns to each service its ModuleClassID as its unique service identifier. Most of the
 * times this ModuleClassID is a base classID, but groups that use the same Module Class
 * for more than one service (same behaviour but playing a different role in the group, such
 * as, for example, a data base engine with a different data base), may define multiple roles
 * identified by the same base class identifier but different role identifiers. The standard
 * PeerGroup implementation of the java reference implementation has the notion of main
 * application: a default application which may be started automatically upon instantiating
 * the group. This application implements Module and, therefore, is assigned a ModuleClassID.
 * However applications are not expected to play any specific role in the group. As a result, they
 * are assigned a role identifier allocated at run-time as need to garantee local unicity. As
 * a result main applications cannot expect a predictible ClassID.
 * <p/>
 * <p/>
 * A ModuleClassID is optionaly described by a published ModuleClassAdvertisement.
 * <p/>
 * <p/>
 * There may be any number of embodiements of a module class. These are module
 * specifications. A module specification represent the network behaviour of a
 * module while its class represents its local behaviour. Different groups
 * may use a common subset of classes, for example, the basic set defined by the platform
 * should always be part of it. Each group may use different and network-incompatible
 * specifications for common classes, optimized for various purposes. The local API of a
 * given class on a given JXTA implementation will be invariant per the spec being used.
 * Therefore, the difference will be transparent to applications which do not depend
 * on the possibly different quality of service.
 * <p/>
 * <p/>
 * A ModuleSpecID embeds a base class identifier, which permits to verify that
 * a given Module specification is suitable for its intended use.
 *
 * @author Daniel Brookshier <a HREF="mailto:turbogeek@cluck.com">turbogeek@cluck.com</a>
 * @see net.jxta.peergroup.PeerGroup
 * @see net.jxta.platform.Module
 * @see net.jxta.platform.ModuleClassID
 * @see net.jxta.protocol.PeerAdvertisement
 * @see net.jxta.protocol.ModuleSpecAdvertisement
 * @see net.jxta.protocol.ModuleClassAdvertisement
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.id.ID
 */

public final class ModuleClassBinaryID extends net.jxta.platform.ModuleClassID {

    /**
     * Log4J categorgy
     */
    private final static transient Logger LOG = Logger.getLogger(ModuleClassBinaryID.class.getName());

    /**
     * The id data
     */
    protected BinaryID classID;
    protected BinaryID parentClassID;
    protected BinaryID roleID;
    protected PeerGroupID peerGroupID;

    /**
     * Constructor.
     * Intializes contents from provided ID.
     *
     * @param id the ID data
     * @since JXTA  1.0
     */
    protected ModuleClassBinaryID(String id) {
        super();
        int start = id.indexOf('-');
        int parent = id.indexOf(start + 1, '-');
        int role = id.indexOf(id.indexOf(parent + 1, '-') + 1, '-');
        int group = id.indexOf(id.indexOf(role + 1, '-') + 1, '-');

        classID = new BinaryID(id.substring(group + 1, parent));
        parentClassID = new BinaryID(id.substring(parent + 1, role));
        roleID = new BinaryID(id.substring(role + 1, group));
        peerGroupID = new PeerGroupBinaryID(new BinaryID(id.substring(group + 1)));
    }

    /**
     * Constructor.
     * Creates a ModuleClassID in a given class, with a given class unique id.
     * A BinaryID of a class and another BinaryID are provided.
     *
     * @param parentClassID the class to which this will belong.
     * @param roleID        the unique id of this role in that class.
     * @param peerGroupID   the peer group ID
     * @param classID       the class ID
     */
    protected ModuleClassBinaryID(BinaryID classID, BinaryID parentClassID, BinaryID roleID, PeerGroupID peerGroupID) {
        super();
        this.classID = classID;
        this.parentClassID = parentClassID;
        this.roleID = roleID;
        this.peerGroupID = peerGroupID;
    }

    protected ModuleClassBinaryID(BinaryID classID, BinaryID parentClassID, BinaryID roleID, BinaryID peerGroupID) {
        super();
        this.classID = classID;
        this.parentClassID = parentClassID;
        this.roleID = roleID;
        this.peerGroupID = new PeerGroupBinaryID(peerGroupID);
    }

    /**
     * Constructor for creating a new ModuleClassID. A new class BinaryID is
     * created. The role ID is left null. This is the only way to create
     * a new class without supplying a new BinaryID explicitly.
     * To create a new role in an existing class, one must use one of
     * the other constructors.
     * Note that a null role is just as valid as any other, it just has a
     * shorter string representation. So it is not mandatory to create a new
     * role in a new class.
     *
     * @since JXTA 1.0
     */
    public ModuleClassBinaryID() {
        this(new BinaryID(BinaryID.flagModuleClassID), new BinaryID(BinaryID.flagModuleClassID)
                ,
                new BinaryID(BinaryID.flagModuleClassRoleID), new BinaryID(BinaryID.flagPeerGroupID));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        } 
        
        if (!(target instanceof ModuleClassBinaryID)) {
            return false;
        }
        
        ModuleClassBinaryID targetObj = (ModuleClassBinaryID) target;
        
        return this.classID.equals(targetObj.getClassID()) && this.parentClassID.equals(targetObj.getBaseClass())
                && this.roleID.equals(targetObj.getRoleID()) && this.peerGroupID.equals(targetObj.getPeerGroupID());
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
        return getIDFormat() + "-" + classID.getID() + "-" + parentClassID.getID() + "-" + roleID.getID() + "-"
                + peerGroupID.getUniqueValue();
    }

    /**
     * Returns the peer group ID
     *
     * @return the peer group ID
     */
    public net.jxta.id.ID getPeerGroupID() {
        return peerGroupID;
    }

    /**
     * returns the coded ID without the binaryid tag.
     *
     * @return string of the contents
     */
    protected String getID() {
        return classID.getID() + "*" + parentClassID.getID() + "*" + roleID.getID() + "*" + peerGroupID.getUniqueValue();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public net.jxta.platform.ModuleClassID getBaseClass() {
        return new ModuleClassBinaryID(parentClassID, new BinaryID(), new BinaryID(), new BinaryID());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isOfSameBaseClass(net.jxta.platform.ModuleClassID classId) {
        return getClass().equals(((ModuleClassBinaryID) classId).getClass());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isOfSameBaseClass(net.jxta.platform.ModuleSpecID specId) {
        return getBaseClassID().equals(((ModuleSpecBinaryID) specId).getBaseClassID());
    }

    /**
     * get the class' unique id
     *
     * @return BinaryID module class' unique id
     * @since JXTA 1.0
     */
    public BinaryID getClassID() {
        return classID;
    }

    /**
     * get the role unique id
     *
     * @return Module role unique id.
     * @since JXTA 1.0
     */
    public BinaryID getRoleID() {
        return roleID;
    }

    /**
     * Getter for property parentClassID.
     *
     * @return Value of property parentClassID.
     */
    public BinaryID getBaseClassID() {
        return parentClassID;
    }

}
