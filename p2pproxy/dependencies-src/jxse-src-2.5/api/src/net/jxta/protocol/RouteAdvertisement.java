/*
Copyright (c) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
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
package net.jxta.protocol;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peer.PeerID;
import net.jxta.peergroup.PeerGroupID;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Set;
import java.util.Vector;

/**
 * Advertisement used to represent a route to a peer. Routes are represented in
 * a generic manner as a sequence of hops to reach the destination. Each hop
 * represent a potential relay peer in the route:
 * <p/>
 * <pre> Dest
 *       hop 1
 *       hop 2
 *       hop 3
 *       ....
 *       hop n
 * </pre>
 * <p/>
 * A route can have as many hops as necessary. Hops are implicitly ordered
 * starting from the hop with the shortest route to reach the destination. If a
 * peer cannot reach directly the dest, it should try to reach in descending
 * order one of the listed hops. Some hops may have the same physical distance
 * to the destination. Some hops may define alternative routes.
 * <p/>
 * The destination and hops are defined using a AccessPointAdvertisements.
 *
 * @see net.jxta.protocol.PeerAdvertisement
 * @see net.jxta.protocol.AccessPointAdvertisement
 */
public abstract class RouteAdvertisement extends ExtendableAdvertisement implements Cloneable {

    public static final String DEST_PID_TAG = "DstPID";

    /**
     * AccessPointAdvertisement of destination peer.
     */
    private transient AccessPointAdvertisement dest = (AccessPointAdvertisement)
            AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());

    /**
     * Semi-ordered list of alternative hops to the destination.
     */
    private transient Vector<AccessPointAdvertisement> hops = new Vector<AccessPointAdvertisement>();

    /**
     * Cached value for {@link #getID()}
     */
    private transient ID hashID = null;

    /**
     * construct a new route
     * <p/>
     * <b>WARNING hops may be MODIFIED.</b>
     *
     * @param destPid  destination
     * @param firsthop first hop node ID
     * @param hops     routes
     * @return the new route
     */
    public static RouteAdvertisement newRoute(PeerID destPid, PeerID firsthop, Vector<AccessPointAdvertisement> hops) {

        if (destPid == null) {
            throw new IllegalArgumentException("Missing destination peer id.");
        }

        for (AccessPointAdvertisement apa : hops) {
            if (null == apa) {
                throw new IllegalArgumentException("Bad route. null APA.");
            }

            if (apa.getPeerID() == null) {
                throw new IllegalArgumentException("Bad route. Incomplete APA.");
            }
        }

        RouteAdvertisement route = (RouteAdvertisement)
                AdvertisementFactory.newAdvertisement(RouteAdvertisement.getAdvertisementType());

        route.setDestPeerID(destPid);

        // set the route hops
        route.setHops(hops);

        // check if the given first hop is already in the route if not add it
        // (note: we do not expect it to be there, but it is acceptable).
        if (firsthop != null) {
            AccessPointAdvertisement ap = route.getFirstHop();

            if (ap == null || !ap.getPeerID().equals(firsthop)) {
                ap = (AccessPointAdvertisement)
                        AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());
                ap.setPeerID(firsthop);
                route.setFirstHop(ap);
            }
        }

        return route;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public RouteAdvertisement clone() {
        try {
            RouteAdvertisement a = (RouteAdvertisement) super.clone();

            a.setDest(getDest());

            // deep copy of the hops
            Vector<AccessPointAdvertisement> clonehops = getVectorHops();

            ListIterator<AccessPointAdvertisement> eachHop = clonehops.listIterator();

            while (eachHop.hasNext()) {
                eachHop.set(eachHop.next().clone());
            }

            a.setHops(clonehops);

            return a;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }
    }

    /**
     * makes a copy of a route advertisement
     * that only contains PID not endpoint addresses
     *
     * @return object clone route advertisement
     */
    public RouteAdvertisement cloneOnlyPIDs() {
        RouteAdvertisement routeAdvertisement;

        try {
            routeAdvertisement = (RouteAdvertisement) super.clone();
            routeAdvertisement.setDestEndpointAddresses(new Vector<String>());
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }

        // deep copy of the hops
        Vector<AccessPointAdvertisement> clonehops = routeAdvertisement.getVectorHops();

        ListIterator<AccessPointAdvertisement> eachHop = clonehops.listIterator();

        while (eachHop.hasNext()) {
            AccessPointAdvertisement aHop = eachHop.next();

            eachHop.set(aHop.clone());
        }

        routeAdvertisement.setHops(clonehops);
        return routeAdvertisement;
    }

    /**
     * Compare if two routes are equals. Equals means same destination with the
     * same endpoint addresses and thee same number of hops and the same
     * endpoint addresses for each hop.
     *
     * @param target the route to compare against
     * @return boolean true if the route is equal to this route otherwise false
     */
    @Override
    public boolean equals(Object target) {

        if (this == target) {
            return true;
        }

        if (!(target instanceof RouteAdvertisement)) {
            return false;
        }

        RouteAdvertisement route = (RouteAdvertisement) target;

        // check the destination
        if (!dest.equals(route.getDest())) {
            return false;
        }

        // check each of the hops

        // routes need to have the same size
        if (hops.size() != route.size()) {
            return false;
        }

        int index = 0;

        for (AccessPointAdvertisement hop : route.hops) {
            if (!hop.equals(hops.get(index++))) {
                return false;
            }
        }

        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        if (null != dest.getPeerID()) {
            return dest.getPeerID().hashCode();
        } else {
            // force all incomplete advertisements to hash to the same place.
            return 1;
        }
    }

    /**
     * Returns the identifying type of this Advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:RA";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized ID getID() {
        if (null == dest.getPeerID()) {
            throw new IllegalStateException("Destination peerID not defined. Incomplete RouteAdvertisement");
        }

        if (hashID == null) {
            try {
                // We have not yet built it. Do it now
                byte[] seed = getAdvertisementType().getBytes("UTF-8");
                InputStream in = new ByteArrayInputStream(dest.getPeerID().toString().getBytes("UTF-8"));

                hashID = IDFactory.newCodatID((PeerGroupID) dest.getPeerID().getPeerGroupID(), seed, in);
            } catch (Exception ez) {
                return ID.nullID;
            }
        }
        return hashID;
    }

    /**
     * Returns the route destination Peer ID
     *
     * @return peerID of the destination of the route
     */
    public PeerID getDestPeerID() {
        return dest.getPeerID();
    }

    /**
     * Sets the route destination peer id.
     *
     * @param pid route destination peerID
     */
    public void setDestPeerID(PeerID pid) {
        if ((null != pid) && (null != dest.getPeerID()) && (!pid.equals(dest.getPeerID()))) {
            throw new IllegalStateException("Changing the peer id of the destination APA." + pid + " != " + dest.getPeerID());
        }

        dest.setPeerID(pid);

        // recalculate hash.
        synchronized (this) {
            hashID = null;
        }
    }

    /**
     * Returns the destination access point. <b>This does <i>NOT</i> copy
     * the AccessPointAdvertisement</b>.
     *
     * @return AccessPointAdvertisement of the destination peer.
     * @deprecated Because this method unsafely exposes destination AccessPointAdvertisement it will be removed.
     */
    @Deprecated
    public AccessPointAdvertisement getDest() {
        return dest;
    }

    /**
     * Sets the access point of the destination. <b>This does <i>NOT</i> copy
     * the AccessPointAdvertisement</b>.
     *
     * @param ap AccessPointAdvertisement of the destination peer
     */
    public void setDest(AccessPointAdvertisement ap) {
        PeerID destPid = dest.getPeerID();

        this.dest = ap.clone();

        if ((null != destPid) && (null != dest.getPeerID()) && (!destPid.equals(dest.getPeerID()))) {
            throw new IllegalStateException("Changed the peer id of the destination APA." + destPid + " != " + dest.getPeerID());
        }

        if (null != destPid) {
            dest.setPeerID(destPid);
        }

        // recalculate hash.
        synchronized (this) {
            hashID = null;
        }
    }

    /**
     * Add a new list of EndpointAddresses to the Route Destination access
     * point
     *
     * @param addresses vector of endpoint addresses to add to the
     *                  destination access point. Warning: The vector of endpoint addresses
     *                  is specified as a vector of String. Each string representing
     *                  one endpoint address.
     * @deprecated Use {@link #addDestEndpointAddresses(List<EndpointAddress>)} instead.
     */
    @Deprecated
    public void addDestEndpointAddresses(Vector<String> addresses) {
        dest.addEndpointAddresses(addresses);
    }

    /**
     * Clears all endpoint addresses associated with the destination peer.
     */
    public void clearDestEndpointAddresses() {
        dest.clearEndpointAddresses();
    }

    /**
     * Add the specified endpoint address to destination peer.
     *
     * @param addr EndpointAddress to add.
     */
    public void addDestEndpointAddress(EndpointAddress addr) {
        dest.addEndpointAddress(addr);
    }

    /**
     * Add all of the specified endpoint addresses to destination peer.
     *
     * @param addrs EndpointAddresses to add.
     */
    public void addDestEndpointAddresses(List<EndpointAddress> addrs) {
        dest.addEndpointAddresses(addrs);
    }

    /**
     * Remove the specified endpoint address to destination peer.
     *
     * @param addr EndpointAddress to add.
     */
    public void removeDestEndpointAddress(EndpointAddress addr) {
        dest.removeEndpointAddress(addr);
    }

    /**
     * Remove the specified endpoint addresses from destination peer.
     *
     * @param addrs EndpointAddress to add.
     */
    public void removeDestEndpointAddresses(Collection<EndpointAddress> addrs) {
        dest.removeEndpointAddresses(addrs);
    }

    /**
     * Remove a list of EndpointAddresses from the Route Destination
     * access point
     *
     * @param addresses vector of endpoint addresses to remove from the
     *                  destination access point.
     * @deprecated Use {@link #removeDestEndpointAddresses(Collection)}.
     */
    @Deprecated
    public void removeDestEndpointAddresses(Vector<String> addresses) {
        dest.removeEndpointAddresses(addresses);
    }

    /**
     * Returns the endpoint addresses of the destination peer in their
     * preferred order.
     *
     * @return The {@code EndpointAddress}es of the destination peer.
     */
    public List<EndpointAddress> getDestEndpointAddresses() {
        List<EndpointAddress> result = new ArrayList<EndpointAddress>();

        Enumeration<String> eachEA = dest.getEndpointAddresses();

        while (eachEA.hasMoreElements()) {
            result.add(new EndpointAddress(eachEA.nextElement()));
        }

        return result;
    }

    /**
     * Set the route destination endpoint addresses
     *
     * @param ea vector of endpoint addresses. Warning: The vector is not copied
     *           and is used directly.
     * @deprecated Use {@link #addDestEndpointAddress(EndpointAddress)} instead.
     */
    @Deprecated
    public void setDestEndpointAddresses(Vector<String> ea) {
        dest.setEndpointAddresses(ea);
    }

    /**
     * returns the list of hops
     *
     * @return Enumeration list of hops as AccessPointAdvertisement
     */
    public Enumeration<AccessPointAdvertisement> getHops() {
        return hops.elements();
    }

    /**
     * returns the list of hops
     *
     * @return Vector list of hops as AccessPointAdvertisement
     */
    public Vector<AccessPointAdvertisement> getVectorHops() {
        return hops;
    }

    /**
     * Sets the list of hops associated with this route.
     *
     * @param newHops AccessPointAdvertisements which form the hops. The
     *                Vector is <b>NOT</b> copied.
     */
    public void setHops(Vector<AccessPointAdvertisement> newHops) {
        // It is legal to set it to null but it is automatically converted
        // to an empty vector. The member hops is NEVER null.
        if (null == newHops) {
            hops = new Vector<AccessPointAdvertisement>();
        } else {
            for (AccessPointAdvertisement hop : newHops) {
                if (null == hop.getPeerID()) {
                    throw new IllegalArgumentException("Bad hop");
                }
            }

            hops = newHops;
        }
    }

    /**
     * Check if the route contains the following hop
     *
     * @param pid peer id of the hop
     * @return boolean true or false if the hop is found in the route
     */
    public boolean containsHop(PeerID pid) {
        for (AccessPointAdvertisement hop : hops) {
            PeerID hid = hop.getPeerID();

            if (pid.equals(hid)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns the AccessPointAdvertisement of first hop. <b>The
     * AccessPointAdvertisement is <i>not</i> cloned.</b>
     *
     * @return AccessPointAdvertisement of first hop.
     */
    public AccessPointAdvertisement getFirstHop() {
        return hops.isEmpty() ? null : hops.firstElement();
    }

    /**
     * Sets the AccessPointAdvertisement for the first hop. <b>The
     * AccessPointAdvertisement is <i>not</i> cloned.</b>
     *
     * @param ap AccessPointAdvertisement of the first hop.
     */
    public void setFirstHop(AccessPointAdvertisement ap) {
        if (null == ap.getPeerID()) {
            throw new IllegalArgumentException("Bad hop");
        }

        hops.add(0, ap);
    }

    /**
     * Returns the access point for the last hop. <b>The
     * AccessPointAdvertisement is <i>not</i> cloned.</b>
     *
     * @return AccessPointAdvertisement last hop.
     */
    public AccessPointAdvertisement getLastHop() {
        return hops.isEmpty() ? null : hops.lastElement();
    }

    /**
     * Sets the AccessPointAdvertisement of the last hop. <b>The
     * AccessPointAdvertisement is <i>not</i> cloned.</b>
     *
     * @param ap AccessPointAdvertisement of the last hop.
     */
    public void setLastHop(AccessPointAdvertisement ap) {
        if (null == ap.getPeerID()) {
            throw new IllegalArgumentException("Bad hop");
        }

        hops.add(ap);
    }

    /**
     * check if the route has a loop
     *
     * @return boolean true or false if the route has a loop
     */
    public boolean hasALoop() {
        // Now check for any other potential loops.

        Set<PeerID> seenPeers = new HashSet<PeerID>(hops.size());

        for (AccessPointAdvertisement anAPA : hops) {
            PeerID pid = anAPA.getPeerID();

            if (seenPeers.contains(pid)) {
                return true; // There is a loop.
            }

            seenPeers.add(pid);
        }
        return false;
    }

    /**
     * return the length of the route
     *
     * @return int size of the route
     */
    public int size() {
        return hops.size();
    }

    /**
     * Return the hop that follows the specified currentHop. <b>The
     * AccessPointAdvertisement is <i>not</i> cloned.</b>
     *
     * @param currentHop PeerID of the current hop
     * @return ap AccessPointAdvertisement of the next Hop
     */
    public AccessPointAdvertisement nextHop(PeerID currentHop) {

        // check if we have a real route
        if (hops.isEmpty()) {
            // Empty vector.
            return null;
        }

        // find the index of the route
        int index = 0;
        boolean found = false;

        for (AccessPointAdvertisement ap : hops) {
            if (currentHop.equals(ap.getPeerID())) {
                found = true;
                break;
            }
            index++;
        }

        AccessPointAdvertisement nextHop = null;

        if (!found) {
            // The peer is not into the list. Since we have got that message,
            // the best we can do is to send it to the first gateway in the
            // forward path.
            nextHop = hops.get(0);
        } else {
            // Found the peer within the vector of hops. Get the next hop.
            if (index < hops.size()) {
                nextHop = hops.get(index);
            }
        }

        return nextHop;
    }

    /**
     * Generate a string that displays the route
     * information for logging or debugging purpose
     *
     * @return String return a string containing the route info
     */
    public String display() {
        StringBuilder routeBuf = new StringBuilder();

        routeBuf.append("Dest APA : ");
        AccessPointAdvertisement dest = getDest();

        routeBuf.append(dest.display());
        routeBuf.append("\n");

        int i = 1;
        Enumeration<AccessPointAdvertisement> e = getHops();

        while (e.hasMoreElements()) {
            AccessPointAdvertisement hop = e.nextElement();

            if (i == 1) {
                routeBuf.append("HOPS = ");
            }
            routeBuf.append("\n\t[").append(i++).append("] ");

            routeBuf.append(hop.display());
        }
        return routeBuf.toString();
    }

    /**
     * Remove a hop from the list of hops.
     *
     * @param pid peer id of the hop
     * @return boolean true or false if the hop is found in the route
     */
    public boolean removeHop(PeerID pid) {
        Iterator<AccessPointAdvertisement> eachHop = hops.iterator();

        while (eachHop.hasNext()) {
            AccessPointAdvertisement hop = eachHop.next();
            PeerID hid = hop.getPeerID();

            if (pid.equals(hid)) {
                eachHop.remove();
                return true;
            }
        }

        return false;
    }

    /**
     * Return a hop from the list of hops.
     *
     * @param pid peer id of the hop
     * @return AccessPointAdvertisement of the corresponding hop
     */
    public AccessPointAdvertisement getHop(PeerID pid) {
        for (AccessPointAdvertisement hop : hops) {
            PeerID hid = hop.getPeerID();

            if (pid.equals(hid)) {
                return hop.clone();
            }
        }
        return null;
    }

    /**
     * Alter the given newRoute (which does not start from here) by using firstLeg, a known route to whence
     * it starts from. So that the complete route goes from here to the end-destination via firstLeg.
     * public static boolean stichRoute(RouteAdvertisement newRoute,
     *
     * @param newRoute the new route
     * @param firstLeg the first route
     * @return true if successful
     */
    public static boolean stichRoute(RouteAdvertisement newRoute, RouteAdvertisement firstLeg) {
        return stichRoute(newRoute, firstLeg, null);
    }

    /**
     * Alter the given newRoute (which does not start from here) by using firstLeg, a known route to whence
     * it starts from. So that the complete route goes from here to the end-destination via firstLeg
     * also shortcut the route by removing the local peer.
     *
     * @param newRoute  the new route
     * @param firstLeg  first hop
     * @param localPeer local PeerID
     * @return true if successful
     */
    public static boolean stichRoute(RouteAdvertisement newRoute, RouteAdvertisement firstLeg, PeerID localPeer) {

        if (newRoute.hasALoop()) {
            return false;
        }

        Vector<AccessPointAdvertisement> hops = newRoute.getVectorHops();

        // Make room
        hops.ensureCapacity(firstLeg.getVectorHops().size() + 1 + hops.size());

        // prepend the routing peer unless the routing peer happens to be
        // in the route already. That happens if the routing peer is the relay.
        // or if the route does not have a first leg
        PeerID routerPid = firstLeg.getDest().getPeerID();

        if (newRoute.size() == 0 || (!newRoute.getFirstHop().getPeerID().equals(routerPid))) {
            AccessPointAdvertisement ap = (AccessPointAdvertisement)
                    AdvertisementFactory.newAdvertisement(AccessPointAdvertisement.getAdvertisementType());

            // prepend the route with the routing peer.
            ap.setPeerID(routerPid);
            hops.add(0, ap);
        }

        // prepend the rest of the route
        hops.addAll(0, firstLeg.getVectorHops());

        // remove any loop from the root
        cleanupLoop(newRoute, localPeer);
        return true;
    }

    /**
     * Remove loops from the route advertisement
     * by shortcutting cycle from the route
     *
     * @param route     the route advertisement
     * @param localPeer local PeerID
     */
    public static void cleanupLoop(RouteAdvertisement route, PeerID localPeer) {

        // Note: we cleanup all enp addresses except for the last hop (which we
        // use to shorten routes often enough).
        // If we end-up removing the last hop, it means that it is the local
        // peer and thus the route ends up with a size 0.

        Vector<AccessPointAdvertisement> hops = route.getVectorHops();
        Vector<AccessPointAdvertisement> newHops = new Vector<AccessPointAdvertisement>(hops.size());
        AccessPointAdvertisement lastHop = null;

        // Replace all by PID-only entries, but keep the last hop on the side.
        if (!hops.isEmpty()) {
            lastHop = hops.get(hops.size() - 1);
        }
        hops = (route.cloneOnlyPIDs()).getVectorHops();

        // remove cycle from the route
        for (int i = 0; i < hops.size(); i++) {
            int loopAt = newHops.indexOf(hops.elementAt(i));

            if (loopAt != -1) { // we found a cycle

                // remove all entries after loopAt
                for (int j = newHops.size(); --j > loopAt;) {
                    newHops.remove(j);
                }
            } else { // did not find it so we add it
                newHops.add(hops.get(i));
            }
        }

        // Remove the local peer in the route if we were given one
        if (localPeer != null) {
            for (int i = newHops.size(); --i >= 0;) {
                if (localPeer.equals(newHops.elementAt(i).getPeerID())) {
                    // remove all the entries up to that point we
                    // need to keep the remaining of the route from that
                    // point
                    for (int j = 0; j <= i; j++) {
                        newHops.remove(0);
                    }
                    break;
                }
            }
        }

        if (lastHop != null && newHops.size() > 0) {
            newHops.setElementAt(lastHop, newHops.size() - 1);
        }

        // update the new hops in the route
        route.setHops(newHops);
    }
}
