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

package net.jxta.platform;


import java.net.URI;
import net.jxta.id.ID;


/**
 * A ModuleClassID uniquely identifies a particular local behaviour, that is,
 * a specific API for each execution environment for which an implementation
 * exists.
 *
 * <p/>A ModuleClassID has two components: A base class identifier, and a role identifier.
 * The role identifier may be zero. By convention the API uses the ModuleClassID with
 * a zero role identifier to designate the base class in contexts where only the base class
 * is significant. Nonetheless, a ModuleClassID with a zero role identifier is a valid
 * ModulesClassID wherever a full ModuleClassID is expected. In many cases, only one role
 * in a given class is ever used. Using role zero in such cases is an optimization because
 * it may make the string representation of the ModuleClassID shorter.
 *
 * <p/>Each service of a group, that is, the role it plays in the group, is uniquely identified
 * per the group definition.
 * This identifier may be used by other modules in the group to designate this one, or by the service
 * itself to identify its parameters in a PeerAdvertisement. In addition, by combining its
 * PeerGroupID with its own ModuleClassID, a service may create a predictible identifier unique
 * on their peer, suitable for registering listeners with the EndpointService or other services
 * with similar listener interfaces.
 *
 * <p/>The standard PeerGroup implementation of the java reference implementation
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
 *
 * <p/>A ModuleClassID is optionaly described by a published ModuleClassAdvertisement.
 *
 * <p/>There may be any number of embodiements of a module class. These are module
 * specifications. A module specification represent the network behaviour of a
 * module while its class represents its local behaviour. Different groups
 * may use a common subset of classes, for example, the basic set defined by the platform
 * should always be part of it. Each group may use different and network-incompatible
 * specifications for common classes, optimized for various purposes. The local API of a
 * given class on a given JXTA implementation will be invariant per the spec being used.
 * Therefore, the difference will be transparent to applications which do not depend
 * on the possibly different quality of service.
 *
 * <p/>A ModuleSpecID embeds a base class identifier, which permits to verify that
 * a given Module specification is suitable for its intended use.
 *
 * @see net.jxta.peergroup.PeerGroup
 * @see net.jxta.platform.Module
 * @see net.jxta.platform.ModuleClassID
 * @see net.jxta.protocol.PeerAdvertisement
 * @see net.jxta.protocol.ModuleSpecAdvertisement
 * @see net.jxta.protocol.ModuleClassAdvertisement
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.id.ID
 *
 */
public abstract class ModuleClassID extends ID {

    /**
     * Creates an ID by parsing the given URI.
     *
     * <p>This convenience factory method works as if by invoking the
     * {@link net.jxta.id.IDFactory#fromURI(URI)} method; any 
     * {@link java.net.URISyntaxException} thrown is caught and wrapped in a 
     * new {@link IllegalArgumentException} object, which is then thrown.  
     *
     * <p> This method is provided for use in situations where it is known that
     * the given string is a legal ID, for example for ID constants declared
     * within in a program, and so it would be considered a programming error
     * for the URI not to parse as such.  The {@link net.jxta.id.IDFactory}, 
     * which throws {@link java.net.URISyntaxException} directly, should be used 
     * situations where a ID is being constructed from user input or from some 
     * other source that may be prone to errors. 
     *
     * @param  fromURI   The URI to be parsed into an ID
     * @return The new ID
     *
     * @throws  NullPointerException If {@code fromURI} is {@code null}.
     * @throws  IllegalArgumentException If the given URI is not a valid ID.
     */
    public static ModuleClassID create(URI fromURI) {
        return (ModuleClassID) ID.create(fromURI);
    }
    
    /**
     *  {@inheritDoc}
     */
    public ModuleClassID intern() {
        return (ModuleClassID) super.intern();
    }
    
    /**
     * Returns true if this ModuleClassID is of the same base class than the
     * given class.
     * Note: This method is NOT named "isOfClass" because a ModuleClassID
     * may have two UUID; one that denotes a "base" class proper,
     * and an optional second one that denotes a "Role", or subclass.
     * Compatibility between ClassIDs is based on the "base" portion, hence the
     * "isOfSame" naming. This routine can be used for comparison with a base class
     * since a base class is just a class which role portion happens to be zero.
     *
     * @param id Module class id to compare with
     * @return boolean true if equals
     */
    
    public abstract boolean isOfSameBaseClass(ModuleClassID id);

    /**
     * Returns true if this ModuleClassID is of the same class than the
     * the given ModuleSpecID.
     *
     * @param id Module spec id to compare with
     * @return boolean true if equals
     */
    
    public abstract boolean isOfSameBaseClass(ModuleSpecID id);

    /**
     * Return a ModuleClassID of the same base class but with the role portion
     * set to zero. aka "the base class".
     *
     * @return ModuleClassID the base class.
     */
    public abstract ModuleClassID getBaseClass();
}
