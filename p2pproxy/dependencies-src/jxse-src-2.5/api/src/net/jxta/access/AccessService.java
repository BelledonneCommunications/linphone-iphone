/*
 *
 * ====================================================================
 *
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.access;


import net.jxta.credential.Credential;
import net.jxta.credential.PrivilegedOperation;
import net.jxta.document.Element;
import net.jxta.service.Service;


/**
 *  The Access Service is used by JXTA Applications and Services to determine if 
 specific operations are permitted for a particular identity.
 *
 *  <p/>Each Access Service implementation provides a mechanism for determining
 *  if, for a given operation and identity, the operation is permitted.
 **/
public interface AccessService extends Service {
    
    /**
     *  The result of an access check.
     **/
    public enum AccessResult {
        
        /**
         *  State is unknown or could not be established.
         *
         *  <p/><strong>The operation should not be performed.</strong>
         *
         *  <p/>This result may not be used by all Access Service
         *  implementations.
         **/
        UNDETERMINED, /**
         *  Operation is disallowed.
         *
         *  <p/><strong>The operation should not be performed.</strong>
         *
         **/ DISALLOWED, /**
         *  Operation is permitted.
         *
         *  <p/><strong>The operation should be performed.</strong>
         *
         **/ PERMITTED, /**
         *  Operation would be permitted, but one (or more) of the provided
         *  credentials was expired.
         *
         *  <p/><strong>The operation should not be performed.</strong>
         *
         *  <p/>This result may not be used by all Access Service
         *  implementations.
         **/ PERMITTED_EXPIRED
    }
    
    /**
     *  Determine if a privileged operation is permitted for a given identity.
     *  
     *  @param operation The operation which is being requested or {@code null}.
     *  {@code null} signifies that the operation is unimportant though the
     *  credential must be valid.
     *  @param credential The identity which is requesting or {@code null}. A
     *  {@code null} value indicates that no credential is available.
     *  @return the result of the access check.
     **/
    public AccessResult doAccessCheck(PrivilegedOperation operation, Credential credential);
    
    /**
     *  Create a new privileged operation with the specified subject. Each
     *  operation is also associated with an identity, the offerer. Generally
     *  the privileged operation is cryptographically signed by the offerer. 
     *
     *  @see net.jxta.credential.Credential
     *  
     *  @param subject The subject of the operation. This usually identifies
     *  what operation is being requested.
     *  @param offerer The identity which is offering the operation.
     *  @return The privileged operation object
     **/
    public PrivilegedOperation newPrivilegedOperation(Object subject, Credential offerer);
    
    /**
     *  Read a privileged operation from a portion of a structured document.
     *
     *  @param source The root of the document portion containing the serialized
     *  representation of the privileged operation.
     *  @return The privileged operation object.
     **/
    public PrivilegedOperation newPrivilegedOperation(Element source);
}
