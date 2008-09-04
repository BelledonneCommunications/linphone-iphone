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
package net.jxta.impl.peer;


import java.io.IOException;
import java.util.Enumeration;
import net.jxta.document.Advertisement;
import net.jxta.service.Service;
import net.jxta.id.ID;
import net.jxta.peer.PeerInfoService;
import net.jxta.peer.PeerInfoListener;
import net.jxta.peer.PeerInfoEvent;
import net.jxta.peergroup.PeerGroup;
import net.jxta.exception.PeerGroupException;
import net.jxta.protocol.PeerInfoResponseMessage;
import net.jxta.impl.peer.PeerInfoServiceImpl;
import net.jxta.peer.*;
import net.jxta.meter.*;
import net.jxta.platform.*;


/**
 *  PeerInfoServiceInterface provides a pure interface object that permits
 *  interaction with the actual PeerInfoService implementation without giving
 *  access to the real object.
 */
public class PeerInfoServiceInterface implements PeerInfoService {

    private PeerInfoService impl;

    /**
     * {@inheritDoc}
     * Since THIS is already such an
     *  object, it returns itself. FIXME: it is kind of absurd to have this
     *  method part of the interface but we do not want to define two levels
     *  of Service interface just for that.
     *
     */
    public Service getInterface() {
        return this;
    }

    /**
     * {@inheritDoc}
     *
     */
    public Advertisement getImplAdvertisement() {
        return impl.getImplAdvertisement();
    }

    /**
     *  Only authorized constructor
     *
     *@param  theRealThing
     */
    public PeerInfoServiceInterface(PeerInfoService theRealThing) {
        impl = theRealThing;
    }

    /**
     * {@inheritDoc}
     *  <p/>Initialize the application FIXME: This is meaningless for the
     *  interface object; it is there only to satisfy the requirements of
     *  the interface that we implement. Ultimately, the API should define
     *  two levels of interfaces: one for the real service implementation
     *  and one for the interface object. Right now it feels a bit heavy to
     *  so that since the only different between the two would be init() and
     *  may-be getName().
     *
     */
    public void init(PeerGroup pg, ID assignedID, Advertisement impl) {}

    /**
     * {@inheritDoc}
     *  <p/>This is here for temporary class hierarchy reasons. it is ALWAYS
     *  ignored. By definition, the interface object protects the real
     *  object's start/stop methods from being called
     *
     */
    public int startApp(String[] arg) {
        return 0;
    }

    /**
     * {@inheritDoc}
     *  <p/>This is here for temporary class hierarchy reasons. it is ALWAYS
     *  ignored. By definition, the interface object protects the real
     *  object's start/stop methods from being called This request is
     *  currently ignored.
     */
    public void stopApp() {}

    public boolean isLocalMonitoringAvailable() {
        return (impl.isLocalMonitoringAvailable());
    }

    public boolean isLocalMonitoringAvailable(ModuleClassID moduleClassID) {
        return (impl.isLocalMonitoringAvailable(moduleClassID));
    }
	
    public long[] getSupportedReportRates() {
        return impl.getSupportedReportRates();
    }

    public boolean isSupportedReportRate(long reportRate) {
        return impl.isSupportedReportRate(reportRate);
    }

    public long getBestReportRate(long desiredReportRate) {
        return impl.getBestReportRate(desiredReportRate);
    }

    public PeerMonitorInfo getPeerMonitorInfo() {
        return impl.getPeerMonitorInfo();
    }

    public void getPeerMonitorInfo(PeerID peerID, PeerMonitorInfoListener peerMonitorInfoListener, long timeout) throws MonitorException {
        impl.getPeerMonitorInfo(peerID, peerMonitorInfoListener, timeout);
    } 

    public MonitorReport getCumulativeMonitorReport(MonitorFilter monitorFilter) throws MonitorException {
        return impl.getCumulativeMonitorReport(monitorFilter);
    }
	
    public void getCumulativeMonitorReport(PeerID peerID, MonitorFilter monitorFilter, MonitorListener monitorListener, long timeout) throws MonitorException {
        impl.getCumulativeMonitorReport(peerID, monitorFilter, monitorListener, timeout);
    }

    public long addMonitorListener(MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener) throws MonitorException {
        return impl.addMonitorListener(monitorFilter, reportRate, includeCumulative, monitorListener);
    }	

    public void addRemoteMonitorListener(PeerID peerID, MonitorFilter monitorFilter, long reportRate, boolean includeCumulative, MonitorListener monitorListener, long lease, long timeout) throws MonitorException {	
        impl.addRemoteMonitorListener(peerID, monitorFilter, reportRate, includeCumulative, monitorListener, lease, timeout);
    }

    public boolean removeMonitorListener(MonitorListener monitorListener) throws MonitorException {
        return impl.removeMonitorListener(monitorListener);
    }

    public void removeRemoteMonitorListener(PeerID peerID, MonitorListener monitorListener, long timeout) throws MonitorException {
        impl.removeRemoteMonitorListener(peerID, monitorListener, timeout);
    }

    public void removeRemoteMonitorListener(MonitorListener monitorListener, long timeout) throws MonitorException {
        impl.removeRemoteMonitorListener(monitorListener, timeout);
    }	
}

