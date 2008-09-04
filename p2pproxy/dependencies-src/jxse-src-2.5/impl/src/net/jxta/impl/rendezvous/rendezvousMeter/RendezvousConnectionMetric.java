/*
 * Copyright (c) 2001-2007 Sun Microsystem, Inc.  All rights reserved.
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

package net.jxta.impl.rendezvous.rendezvousMeter;


import net.jxta.rendezvous.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.peer.*;
import net.jxta.endpoint.*;
import net.jxta.util.*;
import net.jxta.exception.*;
import net.jxta.impl.meter.*;
import java.util.*;


/**
 * The metrics about a client peer's connection to a rendezvous
 **/
public class RendezvousConnectionMetric implements DocumentSerializable {
    public static final String CONNECTING = "connecting";
    public static final String CONNECTED = "connected";
    public static final String DISCONNECTED = "disconnected";
    public static final String REFUSED = "refused";
	
    private PeerID peerID;
	
    private String state = null;
    private long transitionTime;

    private long lease;
    private int numConnectionsBegun = 0;
    private int numConnectionsEstablished = 0;
    private int numConnectionsRefused = 0;
    private long totalTimesToConnect;
    private long totalTimeConnected;

    private long lastLeaseRenewalTime;
    private int numLeaseRenewals;

    private int numDisconnects;

    public RendezvousConnectionMetric() {}

    public RendezvousConnectionMetric(PeerID peerID) {
        this.peerID = peerID;
        this.state = DISCONNECTED;		
    }

    public RendezvousConnectionMetric(RendezvousConnectionMetric prototype) {
        this.peerID = prototype.peerID;	
        this.state = prototype.state;
        this.transitionTime = prototype.transitionTime;
        this.lastLeaseRenewalTime = prototype.lastLeaseRenewalTime;
        this.lease = prototype.lease;		
    }

    /** Peer ID of  Rendezvous  **/
    public PeerID getPeerID() {
        return peerID;
    }

    /**
     * State of Client Rendezvous
     * @return RendezvousConnectionMetric.CONNECTING, RendezvousConnectionMetric.CONNECTED, RendezvousConnectionMetric.DISCONNECTED or RendezvousConnectionMetric.REFUSED
     **/
    public String getState() {
        return state;
    }

    /** Get the time that it entered the current state 
     * @return transition time in ms since January 1, 1970, 00:00:00 GMT
     **/
    public long getTransitionTime() {
        return transitionTime;
    }
	
    /** Is connecting to Rendezvous **/
    public boolean isConnecting() {
        return (state != null) && state.equals(CONNECTING);
    }

    /** Get time began connecting to Rendezvous 
     * @return time began or 0 if not connecting
     **/
    public long getBeginConnectionTime() {
        return isConnecting() ? transitionTime : 0;
    }

    /** Is connected to Rendezvous **/
    public boolean isConnected() {
        return (state != null) && state.equals(CONNECTED);
    }

    /** Get time connected to Rendezvous 
     * @return time began or 0 if not connected
     **/
    public long getTimeConnectionEstablished() {
        return isConnected() ? transitionTime : 0;
    }

    /** Get lease establised with Rendezvous **/
    public long getLease() {
        return lease;
    }

    /** Get Number of Connections begun with Rendezvous **/
    public int getNumConnectionsBegun() {
        return numConnectionsBegun;
    }

    /** Get Number of Connections established with Rendezvous **/
    public int getNumConnectionsEstablished() {
        return numConnectionsEstablished;
    }

    /** Get Number of Connections refused by Rendezvous **/
    public int getNumConnectionsRefused() {
        return numConnectionsRefused;
    }

    /** Get Sum of times it took to connect **/
    public long getTotalTimesToConnect() {
        return totalTimesToConnect;
    }

    /** Get Last Lease Renewal Time **/
    public long getLastLeaseRenewalTime() {
        return lastLeaseRenewalTime;
    }

    /** Get Number of lease Renewals **/
    public int getNumLeaseRenewals() {
        return numLeaseRenewals;
    }

    /** Get Number of Disconnects **/
    public int getNumDisconnects() {
        return numDisconnects;
    }

    /** Get time disconnected to Rendezvous 
     * @return time began or 0 if not disconnected
     **/
    public long getDisconnectTime() {
        return isDisconnected() ? transitionTime : 0;
    }

    /** Have we disconnected fromthis Rendezvous **/
    public boolean isDisconnected() {
        return (state != null) && (state.equals(DISCONNECTED) || state.equals(REFUSED));
    }
	
    /** Get the total time this peer has been connected.
     * <BR><BR>
     * <B>Note:</B> This does not include the current time connected (if it is currently connected)
     * @see #getTotalTimeConnected(long)
     * @return time in ms (see note above)
     **/
    public long getTotalTimeConnected() {
        return totalTimeConnected;
    }

    /** Get the total time this peer has been connected.  If it is currently 
     * connected, then the total time is adjusted to include the time since the transition time
     * to become connected until the provided time
     * @param adjustmentTime The time of this metric will be adjusted to
     * @see #getTotalTimeConnected()
     * @return time in ms (see note above)
     **/
    public long getTotalTimeConnected(long adjustmentTime) { 
        long result = totalTimeConnected;

        if (isConnected()) { 
            result += (adjustmentTime - this.transitionTime);
        }
			
        return result; 
    }

    /** Get the duration of current connection relative to local clock (from transition time)
     * <BR><BR>
     * <B>Note:</B> This assumes the clocks are in sync with the reporting peer 
     * @see #getTotalTimeConnected(long)
     * @return time in ms (see note above) or 0 if not connected
     **/
    public long getTimeConnected() {
        return getTimeConnected(System.currentTimeMillis());
    }

    /** Get the duration of current connection until the specified time
     * @param adjustmentTime The time of this metric will be computed until
     * @see #getTimeConnected()
     * @return time in ms (see note above) or 0 if not connected
     **/
    public long getTimeConnected(long adjustmentTime) { 

        if (isConnected()) { 
            return (adjustmentTime - this.transitionTime);
        } else {	
            return 0;
        } 
    }

    private void resetState(String state, long transitionTime) {
        if (isConnected()) { 
            totalTimeConnected += (System.currentTimeMillis() - this.transitionTime);
        }

        this.state = state;
        this.transitionTime = transitionTime;
    }

    void beginConnection(long transitionTime) {
        resetState(CONNECTING, transitionTime);

        this.numConnectionsBegun++;
    }

    void connectionEstablished(long transitionTime, long timeToConnectTime, long lease) {
        resetState(CONNECTED, transitionTime);
        this.totalTimesToConnect += timeToConnectTime;
        this.numConnectionsEstablished++;
		
        this.lease = lease;	 
    }

    void leaseRenewed(long lastLeaseRenewalTime, long lease) {
        this.numLeaseRenewals++;
		
        this.lastLeaseRenewalTime = lastLeaseRenewalTime;
        this.lease = lease;

        if (!isConnected()) {
            resetState(CONNECTED, lastLeaseRenewalTime);
        }		
    }
	
    void connectionRefused(long transitionTime) {
        resetState(REFUSED, lastLeaseRenewalTime);
        numConnectionsRefused++;
    }
	
    void connectionDisconnected(long transitionTime) {
        resetState(DISCONNECTED, lastLeaseRenewalTime);

        numDisconnects++;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof RendezvousConnectionMetric) {
            RendezvousConnectionMetric other = (RendezvousConnectionMetric) obj;
			
            return (peerID.equals(other.peerID));
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return peerID.hashCode();
    }

    public void serializeTo(Element element) throws DocumentSerializationException {
        if (peerID != null) {
            DocumentSerializableUtilities.addString(element, "peerID", peerID.toString());
        }
        if (state != null) {
            DocumentSerializableUtilities.addString(element, "state", state);
        }
        if (transitionTime != 0) {
            DocumentSerializableUtilities.addLong(element, "transitionTime", transitionTime);
        }
        if (lease != 0) {
            DocumentSerializableUtilities.addLong(element, "lease", lease);
        }
        if (numConnectionsBegun != 0) {
            DocumentSerializableUtilities.addInt(element, "numConnectionsBegun", numConnectionsBegun);
        }
        if (numConnectionsEstablished != 0) {
            DocumentSerializableUtilities.addInt(element, "numConnectionsEstablished", numConnectionsEstablished);
        }
        if (numConnectionsRefused != 0) {
            DocumentSerializableUtilities.addInt(element, "numConnectionsRefused", numConnectionsRefused);
        }
        if (totalTimesToConnect != 0) {
            DocumentSerializableUtilities.addLong(element, "totalTimesToConnect", totalTimesToConnect);
        }
        if (totalTimeConnected != 0) {
            DocumentSerializableUtilities.addLong(element, "totalTimeConnected", totalTimeConnected);
        }
        if (lastLeaseRenewalTime != 0) {
            DocumentSerializableUtilities.addLong(element, "lastLeaseRenewalTime", lastLeaseRenewalTime);
        }
        if (numLeaseRenewals != 0) {
            DocumentSerializableUtilities.addInt(element, "numLeaseRenewals", numLeaseRenewals);
        }
        if (numDisconnects != 0) {
            DocumentSerializableUtilities.addInt(element, "numDisconnects", numDisconnects);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("peerID")) {
                String peerIDText = DocumentSerializableUtilities.getString(childElement);

                peerID = MetricUtilities.getPeerIdFromString(peerIDText);
            } else if (tagName.equals("state")) { 
                state = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("transitionTime")) { 
                transitionTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("lease")) { 
                lease = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numConnectionsBegun")) { 
                numConnectionsBegun = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numConnectionsEstablished")) { 
                numConnectionsEstablished = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numConnectionsRefused")) { 
                numConnectionsRefused = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("totalTimesToConnect")) { 
                totalTimesToConnect = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("totalTimeConnected")) { 
                totalTimeConnected = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("lastLeaseRenewalTime")) { 
                lastLeaseRenewalTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numLeaseRenewals")) { 
                numLeaseRenewals = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numDisconnects")) { 
                numDisconnects = DocumentSerializableUtilities.getInt(childElement);
            }
        }
    }

    public void mergeMetrics(RendezvousConnectionMetric otherRendezvousConnectionMetric) {
        if (otherRendezvousConnectionMetric == null) {
            return; 
        }

        if (otherRendezvousConnectionMetric.state != null) {
            state = otherRendezvousConnectionMetric.state;
        }

        if (otherRendezvousConnectionMetric.transitionTime != 0) {
            transitionTime = otherRendezvousConnectionMetric.transitionTime;
        }

        if (otherRendezvousConnectionMetric.lastLeaseRenewalTime != 0) {
            lastLeaseRenewalTime = otherRendezvousConnectionMetric.transitionTime;
        }
	
        lease = otherRendezvousConnectionMetric.lease;
			
        numConnectionsBegun += otherRendezvousConnectionMetric.numConnectionsBegun;
        numConnectionsEstablished += otherRendezvousConnectionMetric.numConnectionsEstablished;
        numConnectionsRefused += otherRendezvousConnectionMetric.numConnectionsRefused;
        totalTimeConnected += otherRendezvousConnectionMetric.totalTimeConnected;
	
        numLeaseRenewals += otherRendezvousConnectionMetric.numLeaseRenewals;
	
        numDisconnects += otherRendezvousConnectionMetric.numDisconnects;
    }
}
