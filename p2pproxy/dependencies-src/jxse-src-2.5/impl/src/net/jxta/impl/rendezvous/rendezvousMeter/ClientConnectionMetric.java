/*
 * Copyright (c) 2001-2007 Sun Micro//Systems, Inc.  All rights reserved.
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


import net.jxta.impl.meter.*;
import net.jxta.rendezvous.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;
import net.jxta.peer.*;
import net.jxta.endpoint.*;
import net.jxta.util.*;
import net.jxta.exception.*;

import java.util.*;


/**
 * The rendezvous's metric for a client peer interacting with a it
 **/
public class ClientConnectionMetric implements DocumentSerializable {
    public static final String CONNECTED = "connected";
    public static final String DISCONNECTED = "disconnected";
    public static final String REFUSED = "refused";

    private PeerID peerID;
	
    private String state = null;
    private long transitionTime;
    private long lastLeaseRenewalTime;
	
    private long lease;
    private int numConnects;
    private int numLeaseRenewals;
    private int numDisconnects;	
    private int numConnectionsRefused;
    private int numErrorsAddingClient;
    private int numUnableToRespondToConnectRequest;

    private long totalTimeConnected;
	
    public ClientConnectionMetric() {}

    public ClientConnectionMetric(PeerID peerID) { 
        this.peerID = peerID;
        this.state = DISCONNECTED;
    }

    public ClientConnectionMetric(ClientConnectionMetric prototype) {
        this.peerID = prototype.peerID;
        this.state = prototype.state;
        this.transitionTime = prototype.transitionTime;
        this.lastLeaseRenewalTime = prototype.lastLeaseRenewalTime;
        this.lease = prototype.lease;
    }

    /** Peer ID of this Client Connection **/
    public PeerID getPeerID() {
        return peerID;
    }

    /**
     * State of Client Connection
     * @return ClientConnectionMetric.CONNECTED, ClientConnectionMetric.DISCONNECTED or ClientConnectionMetric.REFUSED
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
	
    /** Is this client connected **/
    public boolean isConnected() {
        return (state != null) && state.equals(CONNECTED);
    }

    /** Get time Connected
     * @return time or 0 if not connected
     **/
    public long getTimeConnectionEstablished() {
        return isConnected() ? transitionTime : 0;
    }

    /** Get time Connected
     * @return time or 0 if not disconnected
     **/
    public long getDisconnectTime() {
        return isDisconnected() ? transitionTime : 0;
    }

    /** Is this client disconnected **/
    public boolean isDisconnected() {
        return (state != null) && (state.equals(DISCONNECTED) || state.equals(REFUSED));
    }
	
    /** Get Lease time granted for last lease Renewal Request.
     * @see #getLastLeaseRenewalTime()
     **/
    public long getLease() {
        return lease;
    }

    /** Get Last Received Lease Renewal Time 
     * @see #getLease()
     **/
    public long getLastLeaseRenewalTime() {
        return lastLeaseRenewalTime;
    }

    /** Get number of granted connect messages received from this peer **/
    public int getNumConnects() {
        return numConnects;
    }

    /** Get number of granted lease renewal messages received from this peer **/
    public int getNumLeaseRenewals() {
        return numLeaseRenewals;
    }

    /** Get number of disconnect messages received from this peer **/
    public int getNumDisconnects() {
        return numDisconnects;
    }

    /** Get number of refused connect/lease-renewal messages received from this peer **/
    public int getNumConnectionsRefused() {
        return numConnectionsRefused;
    }

    /** Get number of errors when attempting to add this peer as a client **/
    public int getNumErrorsAddingClient() {
        return numErrorsAddingClient;
    }

    /** Get number of errors when attempting to respond to this peer's request**/
    public int getNumUnableToRespondToConnectRequest() {
        return numUnableToRespondToConnectRequest;
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

    void clientConnectionEstablished(long transitionTime, long lease) {
        resetState(CONNECTED, transitionTime);
			
        this.numConnects++;
        this.lease = lease;
    }

    void clientLeaseRenewed(long lastLeaseRenewalTime, long lease) {
        this.numLeaseRenewals++;
        this.lease = lease;
        this.lastLeaseRenewalTime = System.currentTimeMillis();

        if (!isConnected()) {
            resetState(CONNECTED, lastLeaseRenewalTime);
        }
    }

    void errorAddingClient() {
        this.numErrorsAddingClient++;
        ;
    }
	
    void clientConnectionDisconnected(boolean normal, long transitionTime) {
        resetState(DISCONNECTED, transitionTime);
        this.numDisconnects++;
        this.lease = 0;
    }

    void unableToRespondToConnectRequest() {
        this.numUnableToRespondToConnectRequest++;
    }	

    void clientConnectionRefused(long transitionTime) {
        if (!isDisconnected()) {
            clientConnectionDisconnected(false, transitionTime);
        }
			
        this.transitionTime = transitionTime;
        this.numConnectionsRefused++;		
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
        if (lastLeaseRenewalTime != 0) {
            DocumentSerializableUtilities.addLong(element, "lastLeaseRenewalTime", lastLeaseRenewalTime);
        }
        if (lease != 0) {
            DocumentSerializableUtilities.addLong(element, "lease", lease);
        }
        if (numConnects != 0) {
            DocumentSerializableUtilities.addInt(element, "numConnects", numConnects);
        }
        if (numLeaseRenewals != 0) {
            DocumentSerializableUtilities.addInt(element, "numLeaseRenewals", numLeaseRenewals);
        }
        if (numDisconnects != 0) {
            DocumentSerializableUtilities.addInt(element, "numDisconnects", numDisconnects);
        }
        if (numConnectionsRefused != 0) {
            DocumentSerializableUtilities.addInt(element, "numConnectionsRefused", numConnectionsRefused);
        }
        if (numErrorsAddingClient != 0) {
            DocumentSerializableUtilities.addInt(element, "numErrorsAddingClient", numErrorsAddingClient);
        }
        if (numUnableToRespondToConnectRequest != 0) {
            DocumentSerializableUtilities.addInt(element, "numUnableToRespondToConnectRequest", numUnableToRespondToConnectRequest);
        }
        if (totalTimeConnected != 0) {
            DocumentSerializableUtilities.addLong(element, "totalTimeConnected", totalTimeConnected);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        state = null;

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
            } else if (tagName.equals("lastLeaseRenewalTime")) { 
                lastLeaseRenewalTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("lease")) { 
                lease = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numConnects")) { 
                numConnects = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numLeaseRenewals")) { 
                numLeaseRenewals = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numDisconnects")) { 
                numDisconnects = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numConnectionsRefused")) { 
                numConnectionsRefused = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numErrorsAddingClient")) { 
                numErrorsAddingClient = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numUnableToRespondToConnectRequest")) { 
                numUnableToRespondToConnectRequest = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("totalTimeConnected")) { 
                totalTimeConnected = DocumentSerializableUtilities.getLong(childElement);
            }
        }
    }

    public void mergeMetrics(ClientConnectionMetric otherClientConnectionMetric) {
        if (otherClientConnectionMetric == null) {
            return;  
        }

        if (otherClientConnectionMetric.state != null) {
            state = otherClientConnectionMetric.state;
        }

        if (otherClientConnectionMetric.transitionTime != 0) {
            transitionTime = otherClientConnectionMetric.transitionTime;
        }

        lease = otherClientConnectionMetric.lease;

        numConnects += otherClientConnectionMetric.numConnects;
        numLeaseRenewals += otherClientConnectionMetric.numLeaseRenewals;
        numDisconnects += otherClientConnectionMetric.numDisconnects;
        numConnectionsRefused += otherClientConnectionMetric.numConnectionsRefused;
        numErrorsAddingClient += otherClientConnectionMetric.numErrorsAddingClient;
        numUnableToRespondToConnectRequest += otherClientConnectionMetric.numUnableToRespondToConnectRequest;
        totalTimeConnected += otherClientConnectionMetric.totalTimeConnected;
    }
}
