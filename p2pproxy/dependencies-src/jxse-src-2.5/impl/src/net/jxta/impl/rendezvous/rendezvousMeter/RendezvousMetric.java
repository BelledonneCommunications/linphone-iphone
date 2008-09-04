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

package net.jxta.impl.rendezvous.rendezvousMeter;


import net.jxta.rendezvous.*;
import net.jxta.util.documentSerializable.*;
import net.jxta.document.*;

import java.util.*;


/**
 The Metric corresponding to the state and aggregate information of a Rendezvous Service
 **/
public class RendezvousMetric implements DocumentSerializable {
	
    public static final String EDGE = "edge";
    public static final String RENDEZVOUS = "rendezvous";
    public static final String STOPPED = "stopped"; // but internally represent as null

    private String state = null;
    private long transitionTime = 0;

    private long totalEdgeTime;
    private int numEdgeTransitions;

    private long totalRendezvousTime;
    private long numRendezvousTransitions;

    private int numReceivedProcessedLocally;
    private int numReceivedRepropagatedInGroup;
    private int numReceivedInvalid;
    private int numReceivedDead;
    private int numReceivedLoopback;
    private int numReceivedDuplicate;

    private int numPropagated;
    private int numFailedPropagating;
    private int numRepropagated;
    private int numFailedRepropagating;
	
    private int numPropagatedToPeers;
    private int numFailedPropagatingToPeers;
    private int numPeersPropagatedTo;
	
    private int numPropagatedInGroup;
	
    private int numPropagatedToNeighbors;
    private int numFailedPropagatingToNeighbors;
	
    private int numWalks;
    private int numFailedWalks;
	
    private int numWalkedToPeers;
    private int numFailedWalkToPeers;
    private int numPeersWalkedTo;

    public RendezvousMetric() {}
	
    public RendezvousMetric(RendezvousMetric prototype) {
        if (prototype == null) {
            this.state = STOPPED;
        } else {
            this.state = prototype.state;
            this.transitionTime = prototype.transitionTime;
        }
    }

    /** Get the current state (edge, rendezvous or stopped) 
     * @return RendezvousMetric.EDGE, RendezvousMetric.RENDEZVOUS or RendezvousMetric.STOPPED
     **/
    public String getState() {
        return (state != null) ? state : STOPPED;
    }

    /** Get the time that it entered the current state 
     * @return transition time in ms since January 1, 1970, 00:00:00 GMT
     **/
    public long getTransitionTime() {
        return transitionTime;
    }

    /** Is this Rendezvous currently operating as an Edge **/
    public boolean isEdge() {
        return (state != null) && state.equals(EDGE);
    }

    /** Is this Rendezvous currently operating as an Rendezvous */
    public boolean isRendezvous() {
        return (state != null) && state.equals(RENDEZVOUS);
    }

    /** Get the time that it began operating as an Edge
     * @return time or 0 if it is not currently an Edge 
     **/
    public long getEdgeStartTime() {
        return isEdge() ? transitionTime : 0;
    }

    /** Get the total time it began operating as an Edge.
     * <BR><BR>
     * <B>Note:</B> This does not include the current time as edge (if it is currently an edge)
     * @see #getTotalEdgeTime(long)
     * @return time in ms (see note above)
     **/
    public long getTotalEdgeTime() {
        return totalEdgeTime;
    }

    /** Get the total time it began operating as an Edge.  If it is currently operating as
     * an edge, then the total time is adjusted to include the time since the transition time
     * to an edge until the provided time
     * @param adjustmentTime The time of this metric will be adjusted to
     * @see #getTotalEdgeTime()
     * @return time in ms (see note above)
     **/
    public long getTotalEdgeTime(long adjustmentTime) {
        long result = totalEdgeTime;

        if (isEdge()) { 
            result += (adjustmentTime - this.transitionTime);
        }
			
        return result; 
    }

    /** The number of times the peer has become an edge **/
    public int getNumEdgeTransitions() {
        return numEdgeTransitions;
    }
	
    /** Get the time that it began operating as an Rendezvous
     * @return time or 0 if it is not currently an Rendezvous 
     **/
    public long getRendezvousStartTime() {
        return isRendezvous() ? transitionTime : 0;
    }

    /** Get the total time it began operating as an Rendezvous.
     * <BR><BR>
     * <B>Note:</B> This does not include the current time as rendezvous (if it is currently an rendezvous)
     * @see #getTotalRendezvousTime(long)
     * @return time in ms (see note above)
     **/
    public long getTotalRendezvousTime() {
        return totalRendezvousTime;
    }

    /** Get the total time it began operating as an Rendezvous.  If it is currently operating as
     * a rendezvous, then the total time is adjusted to include the time since the transition time
     * to an rendezvous until the provided time
     * @param adjustmentTime The time of this metric will be adjusted to
     * @see #getTotalRendezvousTime()
     * @return time in ms (see note above)
     **/
    public long getTotalRendezvousTime(long adjustmentTime) {
        long result = totalRendezvousTime;

        if (isRendezvous()) { 
            result += (adjustmentTime - this.transitionTime);
        }
			
        return result; 
    }

    /** The number of times the peer has become an rendezvous **/
    public long getNumRendezvousTransitions() {
        return numRendezvousTransitions;
    }

    /** The number of messages received that were sent to local listeners **/
    public int getNumReceivedProcessedLocally() {
        return numReceivedProcessedLocally;
    }

    /** The number of messages received that were repropagated to the group **/
    public int getNumReceivedRepropagatedInGroup() {
        return numReceivedRepropagatedInGroup;
    }

    /** The number of invalid messages received **/
    public int getNumReceivedInvalid() {
        return numReceivedInvalid;
    }

    /** The number of TTL Dead messages received **/
    public int getNumReceivedDead() {
        return numReceivedDead;
    }

    /** The number of messages received that originated at peer **/
    public int getNumReceivedLoopback() {
        return numReceivedLoopback;
    }

    /** The number of duplicate messages received **/
    public int getNumReceivedDuplicate() {
        return numReceivedDuplicate;
    }

    /** The total number of inbound messages to the rendezvous service that could not be delivered**/
    public int getTotalReceivedUndelivered() {
        return numReceivedInvalid + numReceivedDead + numReceivedLoopback + numReceivedDuplicate;
    }

    /** The total number of inbound messages to the rendezvous service **/
    public int getTotalReceived() {
        return getTotalReceivedUndelivered() + numReceivedProcessedLocally + numReceivedRepropagatedInGroup;
    }

    /** The number of outbound messages propagated **/
    public int getNumPropagated() {
        return numPropagated;
    }

    /** The number of outbound messages failed during propagation **/
    public int getNumFailedPropagating() {
        return numFailedPropagating;
    }

    /** The number of outbound messages repropagated **/
    public int getNumRepropagated() {
        return numRepropagated;
    }

    /** The number of outbound messages failed during repropagation **/
    public int getNumFailedRepropagating() {
        return numFailedRepropagating;
    }

    /** The number of outbound messages propagated to peers **/
    public int getNumPropagatedToPeers() {
        return numPropagatedToPeers;
    }

    /** The number of outbound messages failed when propagated to peers **/
    public int getNumFailedPropagatingToPeers() {
        return numFailedPropagatingToPeers;
    }

    /** The number of peers that outbound messages were propagated to **/
    public int getNumPeersPropagatedTo() {
        return numPeersPropagatedTo;
    }

    /** The number of outbound messages propagated in group **/
    public int getNumPropagatedInGroup() {
        return numPropagatedInGroup;
    }

    /** The number of outbound messages propagated to neighbors **/
    public int getNumPropagatedToNeighbors() {
        return numPropagatedToNeighbors;
    }

    /** The number of outbound messages failed when propagated to neighbors **/
    public int getNumFailedPropagatingToNeighbors() {
        return numFailedPropagatingToNeighbors;
    }

    /** The number of outbound messages walked  **/
    public int getNumWalks() {
        return numWalks;
    }

    /** The number of outbound messages failed attempting walk **/
    public int getNumFailedWalks() {
        return numFailedWalks;
    }

    /** The number of outbound messages walked to a set of peers **/
    public int getNumWalkedToPeers() {
        return numWalkedToPeers;
    }

    /** The number of outbound messages failed in an attempt to walk to a set of peers **/
    public int getNumFailedWalkToPeers() {
        return numFailedWalkToPeers;
    }

    /** The number of peers that outbound messages were walked to **/
    public int getNumPeersWalkedTo() {
        return numPeersWalkedTo;
    }

    /** Get the duration of current transition to an edge
     * <BR><BR>
     * <B>Note:</B> This assumes the clocks are in sync with the reporting peer 
     * @see #getTimeAsEdge(long)
     * @return time in ms (see note above) or 0 if not edge
     **/
    public long getTimeAsEdge() {
        return getTimeAsEdge(System.currentTimeMillis());
    }

    /** Get the duration of time became an edge until the specified time
     * @param adjustmentTime The time of this metric will be computed until
     * @see #getTimeAsEdge()
     * @return time in ms (see note above) or 0 if not connected
     **/
    public long getTimeAsEdge(long adjustmentTime) { 
        if (isEdge()) { 
            return (adjustmentTime - this.transitionTime);
        } else {
            return 0;
        }
    }

    /** Get the duration of current transition to a rendezvous
     * <BR><BR>
     * <B>Note:</B> This assumes the clocks are in sync with the reporting peer 
     * @see #getTimeAsRendezvous(long)
     * @return time in ms (see note above) or 0 if not edge
     **/
    public long getTimeAsRendezvous() {
        return getTimeAsRendezvous(System.currentTimeMillis());
    }

    /** Get the duration of time became an rendezvous until the specified time
     * @param adjustmentTime The time of this metric will be computed until
     * @see #getTimeAsRendezvous()
     * @return time in ms (see note above) or 0 if not connected
     **/
    public long getTimeAsRendezvous(long adjustmentTime) { 
        if (isRendezvous()) { 
            return (adjustmentTime - this.transitionTime);
        } else {
            return 0;
        }
    }

    void startEdge(long transitionTime) {
        this.transitionTime = transitionTime;
        this.state = EDGE;
        this.numEdgeTransitions++;
    }

    void stopEdge(long transitionTime, long timeAsEdge) {
        this.state = STOPPED;
        this.transitionTime = transitionTime;
        this.totalEdgeTime += timeAsEdge;
    }

    void startRendezvous(long transitionTime) {
        this.state = RENDEZVOUS;
        this.transitionTime = transitionTime;
        this.numRendezvousTransitions++;
    }

    void stopRendezvous(long transitionTime, long timeAsRendezvous) {
        this.state = STOPPED;
        this.transitionTime = transitionTime;
        this.totalRendezvousTime += timeAsRendezvous;
    }

    void invalidMessageReceived() {
        numReceivedInvalid++;
    } 

    void receivedMessageProcessedLocally() {
        numReceivedProcessedLocally++;
    } 

    void receivedMessageRepropagatedInGroup() {
        numReceivedRepropagatedInGroup++;
    } 

    void receivedDeadMessage() {
        numReceivedDead++;
    } 

    void receivedLoopbackMessage() {
        numReceivedLoopback++;
    } 

    void receivedDuplicateMessage() {
        numReceivedDuplicate++;
    } 
	
    void propagateToPeers(int numPeers) {
        numPropagatedToPeers++;
        numPeersPropagatedTo += numPeers;
    }

    void propagateToNeighbors() {
        numPropagatedToNeighbors++;
    }
	
    void propagateToNeighborsFailed() {
        numFailedPropagatingToNeighbors++;
    } 

    void propagateToGroup() {
        numPropagatedInGroup++;
    } 

    void walk() {
        numWalks++;
    } 

    void walkFailed() {
        numFailedWalks++;
    } 

    void walkToPeers(int numPeers) {
        numWalkedToPeers++;
        numPeersWalkedTo += numPeers;
    } 

    void walkToPeersFailed() {
        numFailedWalkToPeers++;
    } 

    public void mergeMetrics(RendezvousMetric otherRendezvousMetric) {	
        if (otherRendezvousMetric == null) {
            return;  
        }

        if (otherRendezvousMetric.state != null) {
            state = otherRendezvousMetric.state;
        }

        if (otherRendezvousMetric.transitionTime != 0) {
            transitionTime = otherRendezvousMetric.transitionTime;
        }
			
        this.totalEdgeTime += otherRendezvousMetric.totalEdgeTime;
        this.numEdgeTransitions += otherRendezvousMetric.numEdgeTransitions;

        this.totalRendezvousTime += otherRendezvousMetric.totalRendezvousTime;
        this.numRendezvousTransitions += otherRendezvousMetric.numRendezvousTransitions;

        this.numReceivedProcessedLocally += otherRendezvousMetric.numReceivedProcessedLocally;
        this.numReceivedRepropagatedInGroup += otherRendezvousMetric.numReceivedRepropagatedInGroup;

        this.numReceivedInvalid += otherRendezvousMetric.numReceivedInvalid;

        this.numReceivedDead += otherRendezvousMetric.numReceivedDead;
        this.numReceivedLoopback += otherRendezvousMetric.numReceivedLoopback;
        this.numReceivedDuplicate += otherRendezvousMetric.numReceivedDuplicate;
	
        this.numPropagated += otherRendezvousMetric.numPropagated;
        this.numFailedPropagating += otherRendezvousMetric.numFailedPropagating;
        this.numRepropagated += otherRendezvousMetric.numRepropagated;
        this.numFailedRepropagating += otherRendezvousMetric.numFailedRepropagating;
		
        this.numPropagatedToPeers += otherRendezvousMetric.numPropagatedToPeers;
        this.numFailedPropagatingToPeers += otherRendezvousMetric.numFailedPropagatingToPeers;
        this.numPeersPropagatedTo += otherRendezvousMetric.numPeersPropagatedTo;
		
        this.numPropagatedInGroup += otherRendezvousMetric.numPropagatedInGroup;
		
        this.numPropagatedToNeighbors += otherRendezvousMetric.numPropagatedToNeighbors;
        this.numFailedPropagatingToNeighbors += otherRendezvousMetric.numFailedPropagatingToNeighbors;
		
        this.numWalks += otherRendezvousMetric.numWalks;
        this.numFailedWalks += otherRendezvousMetric.numFailedWalks;
		
        this.numWalkedToPeers += otherRendezvousMetric.numWalkedToPeers;
        this.numFailedWalkToPeers += otherRendezvousMetric.numFailedWalkToPeers;
        this.numPeersWalkedTo += otherRendezvousMetric.numPeersWalkedTo;
    }
	
    public void serializeTo(Element element) throws DocumentSerializationException {
        if (state != null) {
            DocumentSerializableUtilities.addString(element, "state", state);
        }
        if (transitionTime != 0) {
            DocumentSerializableUtilities.addLong(element, "transitionTime", transitionTime);
        }
        if (totalEdgeTime != 0) {
            DocumentSerializableUtilities.addLong(element, "totalEdgeTime", totalEdgeTime);
        }
        if (numEdgeTransitions != 0) {
            DocumentSerializableUtilities.addInt(element, "numEdgeTransitions", numEdgeTransitions);
        }
        if (totalRendezvousTime != 0) {
            DocumentSerializableUtilities.addLong(element, "totalRendezvousTime", totalRendezvousTime);
        }
        if (numRendezvousTransitions != 0) {
            DocumentSerializableUtilities.addLong(element, "numRendezvousTransitions", numRendezvousTransitions);
        }
        if (numReceivedProcessedLocally != 0) {
            DocumentSerializableUtilities.addInt(element, "numReceivedProcessedLocally", numReceivedProcessedLocally);
        }
        if (numReceivedRepropagatedInGroup != 0) {
            DocumentSerializableUtilities.addInt(element, "numReceivedRepropagatedInGroup", numReceivedRepropagatedInGroup);
        }
        if (numReceivedInvalid != 0) {
            DocumentSerializableUtilities.addInt(element, "numReceivedInvalid", numReceivedInvalid);
        }
        if (numReceivedDead != 0) {
            DocumentSerializableUtilities.addInt(element, "numReceivedDead", numReceivedDead);
        }
        if (numReceivedLoopback != 0) {
            DocumentSerializableUtilities.addInt(element, "numReceivedLoopback", numReceivedLoopback);
        }
        if (numReceivedDuplicate != 0) {
            DocumentSerializableUtilities.addInt(element, "numReceivedDuplicate", numReceivedDuplicate);
        }
        if (numPropagated != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagated", numPropagated);
        }
        if (numFailedPropagating != 0) {
            DocumentSerializableUtilities.addInt(element, "numFailedPropagating", numFailedPropagating);
        }
        if (numRepropagated != 0) {
            DocumentSerializableUtilities.addInt(element, "numRepropagated", numRepropagated);
        }
        if (numFailedRepropagating != 0) {
            DocumentSerializableUtilities.addInt(element, "numFailedRepropagating", numFailedRepropagating);
        }
        if (numPropagatedToPeers != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagatedToPeers", numPropagatedToPeers);
        }
        if (numFailedPropagatingToPeers != 0) {
            DocumentSerializableUtilities.addInt(element, "numFailedPropagatingToPeers", numFailedPropagatingToPeers);
        }
        if (numPeersPropagatedTo != 0) {
            DocumentSerializableUtilities.addInt(element, "numPeersPropagatedTo", numPeersPropagatedTo);
        }
        if (numPropagatedInGroup != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagatedInGroup", numPropagatedInGroup);
        }
        if (numPropagatedToNeighbors != 0) {
            DocumentSerializableUtilities.addInt(element, "numPropagatedToNeighbors", numPropagatedToNeighbors);
        }
        if (numFailedPropagatingToNeighbors != 0) {
            DocumentSerializableUtilities.addInt(element, "numFailedPropagatingToNeighbors", numFailedPropagatingToNeighbors);
        }
        if (numWalks != 0) {
            DocumentSerializableUtilities.addInt(element, "numWalks", numWalks);
        }
        if (numFailedWalks != 0) {
            DocumentSerializableUtilities.addInt(element, "numFailedWalks", numFailedWalks);
        }
        if (numWalkedToPeers != 0) {
            DocumentSerializableUtilities.addInt(element, "numWalkedToPeers", numWalkedToPeers);
        }
        if (numFailedWalkToPeers != 0) {
            DocumentSerializableUtilities.addInt(element, "numFailedWalkToPeers", numFailedWalkToPeers);
        }
        if (numPeersWalkedTo != 0) {
            DocumentSerializableUtilities.addInt(element, "numPeersWalkedTo", numPeersWalkedTo);
        }
    }

    public void initializeFrom(Element element) throws DocumentSerializationException {
        for (Enumeration e = element.getChildren(); e.hasMoreElements();) {
            Element childElement = (TextElement) e.nextElement();
            String tagName = (String) childElement.getKey();

            if (tagName.equals("state")) { 
                state = DocumentSerializableUtilities.getString(childElement);
            } else if (tagName.equals("transitionTime")) { 
                transitionTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("totalEdgeTime")) { 
                totalEdgeTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numEdgeTransitions")) { 
                numEdgeTransitions = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("totalRendezvousTime")) { 
                totalRendezvousTime = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numRendezvousTransitions")) { 
                numRendezvousTransitions = DocumentSerializableUtilities.getLong(childElement);
            } else if (tagName.equals("numReceivedProcessedLocally")) { 
                numReceivedProcessedLocally = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numReceivedRepropagatedInGroup")) { 
                numReceivedRepropagatedInGroup = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numReceivedInvalid")) { 
                numReceivedInvalid = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numReceivedDead")) { 
                numReceivedDead = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numReceivedLoopback")) { 
                numReceivedLoopback = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numReceivedDuplicate")) { 
                numReceivedDuplicate = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagated")) { 
                numPropagated = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFailedPropagating")) { 
                numFailedPropagating = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numRepropagated")) { 
                numRepropagated = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFailedRepropagating")) { 
                numFailedRepropagating = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagatedToPeers")) { 
                numPropagatedToPeers = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFailedPropagatingToPeers")) { 
                numFailedPropagatingToPeers = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPeersPropagatedTo")) { 
                numPeersPropagatedTo = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagatedInGroup")) { 
                numPropagatedInGroup = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPropagatedToNeighbors")) { 
                numPropagatedToNeighbors = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFailedPropagatingToNeighbors")) { 
                numFailedPropagatingToNeighbors = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numWalks")) { 
                numWalks = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFailedWalks")) { 
                numFailedWalks = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numWalkedToPeers")) { 
                numWalkedToPeers = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numFailedWalkToPeers")) { 
                numFailedWalkToPeers = DocumentSerializableUtilities.getInt(childElement);
            } else if (tagName.equals("numPeersWalkedTo")) { 
                numPeersWalkedTo = DocumentSerializableUtilities.getInt(childElement);
            }
        }
    }

}
