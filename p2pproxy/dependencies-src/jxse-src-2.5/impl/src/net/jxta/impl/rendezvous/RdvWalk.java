/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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
package net.jxta.impl.rendezvous;

import net.jxta.endpoint.EndpointListener;
import net.jxta.peergroup.PeerGroup;

/**
 * A Walk implements a particular protocol/behavior policy for sending messages
 * through the Rendezvous Peers. A walk strategy is composed of a Walker and a
 * Greeter. The Walker is used for sending messages according to the strategy.
 * The Greeter receives messages and forwards them to a local listener and may
 * provide the ability to continue a walk.
 * <p/>
 * <p/>Each walk is associated with a source service name and service param.
 * These are the name and optional parameter of the service that uses the
 * RdvWalk.
 *
 * @see net.jxta.impl.rendezvous.RdvWalker
 * @see net.jxta.impl.rendezvous.RdvGreeter
 */
public abstract class RdvWalk {

    /**
     * Peergroup in which this walk is running.
     */
    protected final PeerGroup group;

    /**
     * Intended recipient of messages received as part of this walk.
     */
    protected final EndpointListener listener;

    /**
     * Service name used by the (client) of this walk.
     */
    protected final String srcServiceName;

    /**
     * Optional service parameter used by the client of this walk.
     */
    protected final String srcServiceParam;

    /**
     * Standard constructor
     *
     * @param group           Peergroup in which this walk is running.
     * @param listener        Intended recipient of messages received as part of this walk.
     * @param srcServiceName  Service name used by the client of this walk.
     * @param srcServiceParam Optional service parameter used by the client of this walk.
     */
    public RdvWalk(PeerGroup group, EndpointListener listener, String srcServiceName, String srcServiceParam) {
        this.group = group;
        this.listener = listener;
        this.srcServiceName = srcServiceName;
        this.srcServiceParam = srcServiceParam;
    }

    /**
     * Stop the walk.
     */
    public abstract void stop();

    /**
     * Get/Create a walker to be used with this walk.
     *
     * @return A walker to be used with this walk. {@code null} is returned if
     *         no greeter is available or the walk has been stopped.
     */
    public abstract RdvWalker getWalker();

    /**
     * Get/Create a greeter to be used with this walk.
     *
     * @return A greeter to be used with this walk. {@code null} is returned if
     *         no greeter is available or the walk has been stopped.
     */
    public abstract RdvGreeter getGreeter();

    /**
     * Return the Peer Group in which this walk occurs.
     *
     * @return the Peer Group in which this walk occurs.
     */
    public PeerGroup getPeerGroup() {
        return group;
    }

    /**
     * Return the listener associated with this walk
     *
     * @return The listener associated with this walk.
     */
    public EndpointListener getListener() {
        return listener;
    }

    /**
     * Return the source Service Name for this walk.
     *
     * @return The source Service Name for this walk.
     */
    public String getServiceName() {
        return srcServiceName;
    }

    /**
     * Return the source Service Param for this walk.
     *
     * @return The source Service Param for this walk.
     */
    public String getServiceParam() {
        return srcServiceParam;
    }
}
