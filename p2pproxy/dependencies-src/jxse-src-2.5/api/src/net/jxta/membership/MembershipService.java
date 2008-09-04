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

package net.jxta.membership;


import java.beans.PropertyChangeListener;
import java.util.Enumeration;

import net.jxta.credential.AuthenticationCredential;
import net.jxta.credential.Credential;
import net.jxta.document.Element;
import net.jxta.service.Service;

import net.jxta.exception.PeerGroupException;
import net.jxta.exception.ProtocolNotSupportedException;


/**
 *  Allows a peer to establish an identity within a peer group. Identities are 
 *  used by services and applications to determine the capabilities available to 
 *  peers. A peer have any number of identities at one time. Once an identity 
 *  has been established a Credential object is available which allows the peer 
 *  to prove that it rightfully has that identity.
 *
 *  <p/>The sequence for associating an identity with a peer within a peer
 *  group is as follows:
 *
 * <p/><table>
 *  <tr><td valign="top"><b>Apply</b></td>
 * <td>An application or service provides the membership service with an
 * initial credential which may be used by the membership service to determine
 *  the method of authentication to be used for establishing the identity.
 * If the membership service implementations allows authentication using the
 * requested mechanism then an {@link Authenticator} object is returned.
 * </td></tr>
 *
 *  <tr><td valign="top"><b>Join</b></td>
 * <td>The application or service completes the authenticator. This may involve
 * presenting a user interface, completing challenges, etc. How the
 * authenticator is completed depends on the type of membership service and
 * authenticator in use.
 *
 * <p/>Once completed, the authenticator is returned to the membership service.
 * If the authenticator has been correctly completed, a new credential for the
 * new identity will be available to the peer from the membership service.
 * </td></tr>
 *
 *  <tr><td valign="top"><b>Resign</b></td>
 * <td>Whenever the application or service no longer wishes to no longer use the
 * identities it has claimed, it may resign from the peergroup. This will cause
 * any identity credentials held by the membership service to  discarded.
 * </td></tr>
 *  </table>
 *
 *  @see net.jxta.credential.Credential
 *  @see net.jxta.credential.AuthenticationCredential
 *
 */
public interface MembershipService extends Service {
    
    /**
     *  Property name for the default credential bound property.
     */
    public final static String DEFAULT_CREDENTIAL_PROPERTY = "defaultCredential";
    
    /**
     *  Property name for credential addition bound property.
     */
    public final static String ADD_CREDENTIAL_PROPERTY = "addCredential";
    
    /**
     * Request the necessary credentials to join the group with which this
     * service is associated.
     *
     * @param application The authentication Credential associated with this
     * membership application. See
     * {@link net.jxta.credential.AuthenticationCredential}
     * for more information.
     * @return An Authenticator for the membership service.
     *
     * @throws PeerGroupException Thrown in the event of errors.
     * @throws ProtocolNotSupportedException if the authentication method requested
     *       in the application is not supported by this service.
     */
    public Authenticator apply(AuthenticationCredential application) throws PeerGroupException, ProtocolNotSupportedException;
    
    /**
     * Join the group by virtue of the completed authentication provided.
     *
     * @param authenticated the completed authentication.
     * @return Credential the credential for this completed authentication.
     * @throws PeerGroupException Thrown in the event of errors.
     */
    public Credential join(Authenticator authenticated) throws PeerGroupException;
    
    /**
     * Resign all credentials which were previously gained through prior
     * {@link #join(Authenticator) join()} operations.
     *
     * @throws PeerGroupException Thrown in the event of errors. 
     */
    public void resign() throws PeerGroupException;
    
    /**
     *  Returns the default credential for this peer.
     *
     *  @return The current default Credential or {@code null} if there is no
     *  current default.
     *  @throws PeerGroupException Thrown in the event of errors.
     */
    public Credential getDefaultCredential() throws PeerGroupException;
    
    /**
     * Returns the current credentials for this peer.
     *
     * @return Enumeration of the Credentials currently associated with this
     * peer for this peergroup.
     * @throws PeerGroupException Thrown in the event of errors.
     */
    public Enumeration<Credential> getCurrentCredentials() throws PeerGroupException;
    
    /**
     * Returns the authentication credentials which were used to establish the 
     * current identities.
     *
     * @deprecated This interface is being removed in favour of individual
     * Credentials providing their AuthenticationCredential as appropriate.
     *
     * @return Enumeration of the AuthenticationCredentials which were used to
     * establish the current identities.
     * @throws PeerGroupException Thrown in the event of errors.
     */
    @Deprecated
    public Enumeration<AuthenticationCredential> getAuthCredentials() throws PeerGroupException;
    
    /**
     * Given a fragment of a StructuredDocument, reconstruct a Credential object
     * from that fragment.
     *
     * @return Credential The created credential
     * @param element The StructuredDocument fragment to use for building the
     * credential.
     * @throws PeerGroupException Thrown in the event of errors.
     * @throws Exception Thrown in the event of errors.
     */
    public Credential makeCredential(Element element) throws PeerGroupException, Exception;
    
    /**
     *  Add a listener
     *
     *  @param listener the listener
     */
    public void addPropertyChangeListener(PropertyChangeListener listener);
    
    /**
     *  Add a listener. Available properties from all Membership Services are : 
     *      
     *      <p/><ul>
     *          <li>{@code defaultCredential}</li>
     *          <li>{@code addCredential}</li>
     *      </ul>
     *
     *  <p/>Membership Services may offer additional properties.
     *
     *  @param propertyName The property to watch
     *  @param listener The listener
     */
    public void addPropertyChangeListener(String propertyName, PropertyChangeListener listener);
    
    /**
     *  Remove a listener
     *
     *  @param listener the listener
     */
    public void removePropertyChangeListener(PropertyChangeListener listener);
    
    /**
     *  Remove a listener
     *
     *  @param propertyName the property which was watched
     *  @param listener the listener
     */
    public void removePropertyChangeListener(String propertyName, PropertyChangeListener listener);
}
