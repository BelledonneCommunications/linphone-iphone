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

package net.jxta.impl.membership;


import java.net.MalformedURLException;
import java.net.UnknownServiceException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.AdvertisementFactory;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.platform.JxtaLoader;
import net.jxta.membership.Authenticator;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleImplAdvertisement;

import net.jxta.exception.JxtaError;

import net.jxta.impl.loader.RefJxtaLoader;
import net.jxta.impl.peergroup.StdPeerGroup;


/**
 *  The passwd membership service provides a Membership Service implementation
 *  which is based on a password scheme similar to the unix
 *  <code>/etc/passwd</code> system.</code>
 *
 *  @deprecated This service is intended only as a sample and should not be used
 *  for real membership applications. IT IS NOT SECURE. The implementation has
 *  also moved to {@link net.jxta.impl.membership.passwd.PasswdMembershipService}
 *
 * <p/><strong>This implementation is intended mostly as an example of a
 *  simple Membership Service service and <em>not</em> as a practical secure
 *  Membership Service.<strong>
 *
 * @see net.jxta.membership.MembershipService
 *
 **/
@Deprecated
public class PasswdMembershipService {
    
    /**
     *  Log4J Logger
     **/
    private static final Logger LOG = Logger.getLogger(PasswdMembershipService.class.getName());
    
    /**
     * Well known service specification identifier: password membership
     */
    public static final ModuleSpecID passwordMembershipSpecID = net.jxta.impl.membership.passwd.PasswdMembershipService.passwordMembershipSpecID;
    
    /**
     * Register the "real" password membership service as soon as someone 
     * references this class.
     **/
    static {
        JxtaLoader loader = net.jxta.impl.peergroup.GenericPeerGroup.getJxtaLoader();
        
        ModuleImplAdvertisement implAdv = (ModuleImplAdvertisement) AdvertisementFactory.newAdvertisement(
                ModuleImplAdvertisement.getAdvertisementType());
            
        implAdv.setCode(net.jxta.impl.membership.passwd.PasswdMembershipService.class.getName());
        implAdv.setCompat(StdPeerGroup.STD_COMPAT);
        implAdv.setDescription("Password Membership Service");
        implAdv.setModuleSpecID(passwordMembershipSpecID);
        implAdv.setProvider(StdPeerGroup.MODULE_IMPL_STD_PROVIDER);
        implAdv.setUri(StdPeerGroup.MODULE_IMPL_STD_URI);
 
        loader.defineClass(implAdv);
    }
    
    public abstract static class PasswdAuthenticator implements Authenticator {
        
        public abstract void setAuth1Identity(String who);
        
        public abstract String getAuth1Identity();
        
        public abstract void setAuth2_Password(String secret);
        
        protected abstract String getAuth2_Password();
    }
    
    /**
     *  This is the method used to make the password strings. We only provide
     *  one way encoding since we can compare the encoded strings.
     *
     *  <p/>FIXME 20010402bondolo@jxta.org : switch to use the standard
     *  crypt(3) algorithm for encoding the passwords. The current algorithm has
     *  been breakable since ancient times, crypt(3) is also weak, but harder to
     *  break.
     *
     *   @param source  the string to encode
     *   @return String the encoded version of the password.
     *
     **/
    public static String makePsswd(String source) {

        /**
         *
         * A->D  B->Q  C->K  D->W  E->H  F->R  G->T  H->E  I->N  J->O  K->G  L->X  M->C
         * N->V  O->Y  P->S  Q->F  R->J  S->P  T->I  U->L  V->Z  W->A  X->B  Y->M  Z->U
         *
         **/
        
        final String xlateTable = "DQKWHRTENOGXCVYSFJPILZABMU";
        
        StringBuilder work = new StringBuilder(source);
        
        for (int eachChar = work.length() - 1; eachChar >= 0; eachChar--) {
            char aChar = Character.toUpperCase(work.charAt(eachChar));
            
            int replaceIdx = xlateTable.indexOf(aChar);

            if (-1 != replaceIdx) {
                work.setCharAt(eachChar, (char) ('A' + replaceIdx));
            }
        }
        
        return work.toString();
    }
}

