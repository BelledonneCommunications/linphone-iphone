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

package net.jxta.discovery;


import java.util.EventListener;


/**
 *  The listener interface for receiving {@link DiscoveryEvent}s from the
 *  Discovery Service.
 *
 *  The following 2 examples illustrate how to implement a 
 *  <code>DiscoverListener</code>:
 *
 * <p/><b>Example 1:</b>
 * <pre>
 * DiscoveryListener myListener = new DiscoveryListener() {
 *   public void discoveryEvent(DiscoveryEvent e) {
 *     DiscoveryResponseMsg msg = e.getResponse();
 *     if (myQueryID == e.getQueryID()) {
 *       int advCount = msg.getResponseCount();
 *     }
 *   }
 *   discovery.addDiscoveryListener(myListener);
 *   int myQueryID = discovery.getRemoteAdvertisements(address,
 *     discovery.GROUP, attr, val,threshold);
 * </pre>
 *
 * <p/><b>Example 2:</b>
 * <pre>
 * public class JxtaAppDemo implements Runnable, DiscoveryListener {
 *          ..
 *          ..
 *  public void discoveryEvent(DiscoveryEvent ev) {
 *
 *     DiscoveryResponseMsg res = ev.getResponse();
 *     ..
 *     ..
 *     // see tutorial example 2 for full source
 *     // <a href="http://www.jxta.org/project/www/Tutorials.html">JXTA Tutorials</a>
 *     }
 * }
 * </pre>
 *
 *  @see net.jxta.discovery.DiscoveryService
 *  @see net.jxta.discovery.DiscoveryEvent
 **/
public interface DiscoveryListener extends EventListener {

    /**
     * Called to handle an event from the Discovery Service.
     *
     * @param event the discovery event
     */
    void discoveryEvent(DiscoveryEvent event);
}

