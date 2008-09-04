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


import net.jxta.peergroup.PeerGroup;
import net.jxta.document.Advertisement;
import net.jxta.id.ID;
import net.jxta.exception.PeerGroupException;


/**
 * Defines the interface for modules loaded by PeerGroups. Message transports,
 * services and applications need to implement this interface if they are
 * to be loaded and started by a PeerGroup. Service and Application extend
 * Module, PeerGroup implements Service and ShellApp implements Application, as
 * a result both implement Module.
 *
 * <p/>Jxta Modules are given their initialization parameters via the init()
 * method rather than a non-default constructor.
 *
 * <p/>Modules are passed the peer group within which they are created.
 * From the peergroup object, Modules can access all the peer group
 * services. The PeerGroup within which a PeerGroup runs is known as its
 * parent.
 *
 * <p/>The initial root peer group is known as the World Peer Group and is
 * implemented by an object of class Platform, a subclass of PeerGroup.
 * The "parent" PeerGroup of the World Peer Group is null.
 *
 * @see net.jxta.protocol.ModuleImplAdvertisement
 * @see net.jxta.platform.ModuleClassID
 * @see net.jxta.peergroup.PeerGroup
 * @see net.jxta.document.Advertisement
 * @see net.jxta.id.ID
 * @see net.jxta.platform.Application
 * @see net.jxta.service.Service
 **/
public interface Module {
    
    /**
     * <code>startApp()</code> completed successfully. This module claims to now
     * be fully functional and no further invocation of startApp is required.
     **/
    public static final int START_OK = 0;
    
    /**
     * This is to be used mostly by co-dependent services when started as
     * a set (such as {@link PeerGroup} services) so that their
     * <code>startApp()</code> method may be invoked multiple times.
     *
     * <p/>This value indicates that startApp must be retried later in order for
     * this module to become fully functional. However, some progress in
     * functionality was accomplished.
     *
     * <p/>This is a strong indication that some other modules may be able
     * to advance or complete their initialization if their
     * <code>startApp()</code> method is invoked again.
     *
     * <p/>The distinction between START_AGAIN_STALLED and START_AGAIN_PROGRESS
     * is only a hint. Each module makes an arbitrary judgment in this
     * respect. It is up to the invoker of startApp to ensure that the
     * starting of a set of modules eventually succeeds or fails.
     **/
    public static final int START_AGAIN_PROGRESS = 1;
    
    /**
     * This is to be used mostly by co-dependent services when started as
     * a set (such as {@link PeerGroup} services) so that their startApp
     * method may be invoked multiple times.
     *
     * <p/>This value indicates that startApp must be retried later in order for
     * this module to become fully functional. However, some progress in
     * functionality was accomplished.
     *
     * <p/>If all modules in a set return this value, it is a strong indication
     * that the modules co-dependency is such that it prevents them
     * collectively from starting.
     *
     * <p/>The distinction between START_AGAIN_STALLED and START_AGAIN_PROGRESS
     * is only a hint. Each module makes an arbitrary judgment in this
     * respect. It is up to the invoker of startApp to ensure that the
     * starting of a set of modules eventually succeeds or fails.
     **/
    public static final int START_AGAIN_STALLED = 2;
        
    /**
     * This return result is used to indicate that the module refuses to start
     * because it has been configured to be disabled or otherwise cannot run
     * (missing hardware, missing system resources, etc.) The module will not be
     * functional and should be discarded but the failure to load may be ignored 
     * by the loader at it's discretion.
     */
    public static final int START_DISABLED = Integer.MIN_VALUE + 100;
    
    /**
     * Initialize the module, passing it its peer group and advertisement.
     *
     * <p/>Note: when subclassing one of the existing PeerGroup implementations
     * (which implement Module), it may not be recommended to overload the init
     * method. See the documentation of the PeerGroup class being subclassed.
     *
     *  @param group The PeerGroup from which this Module can obtain services.
     *  If this module is a Service, this is also the PeerGroup of which this
     *  module is a service.
     *
     *  @param assignedID Identity of Module within group.
     *  modules can use it as a the root of their namespace to create
     *  names that are unique within the group but predictable by the
     *  same module on another peer. This is normally the ModuleClassID
     *  which is also the name under which the module is known by other
     *  modules. For a group it is the PeerGroupID itself.
     *  The parameters of a service, in the Peer configuration, are indexed
     *  by the assignedID of that service, and a Service must publish its
     *  run-time parameters in the Peer Advertisement under its assigned ID.
     *
     *  @param implAdv The implementation advertisement for this
     *  Module. It is permissible to pass null if no implementation
     *  advertisement is available. This may happen if the
     *  implementation was selected by explicit class name rather than
     *  by following an implementation advertisement. Modules are not
     *  required to support that style of loading, but if they do, then
     *  their documentation should mention it.
     *
     *  @exception PeerGroupException This module failed to initialize.
     **/
    public void init(PeerGroup group, ID assignedID, Advertisement implAdv) throws PeerGroupException;
    
    /**
     * Complete any remaining initialization of the module. The module should
     * be fully functional after <code>startApp()</code> is completed. That is
     * also the opportunity to supply arbitrary arguments (mostly to
     * applications).
     *
     * <p/>If this module is a {@link PeerGroup} service, it may be invoked
     * several times depending on its return value.
     *
     * @param args An array of Strings forming the parameters for this
     * Module.
     *
     * @return int A status indication which may be one of
     * {@link #START_OK}, {@link #START_AGAIN_PROGRESS},
     * {@link #START_AGAIN_STALLED}, which indicates partial or complete
     * success, or any other value (negative values are
     * recommended for future compatibility), which indicates failure.
     **/
    public int startApp(String[] args);
    
    /**
     *  Stop a module. This may be called any time after <code>init()</code>
     *  completes and should not assume that <code>startApp()</code> has been
     *  called or completed.
     *
     *  <p/>The Module cannot be forced to comply, but in the future
     *  we might be able to deny it access to anything after some timeout.
     **/
    public void stopApp();
}
