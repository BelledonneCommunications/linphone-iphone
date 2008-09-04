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

package net.jxta.protocol;


import java.util.Collection;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.peer.PeerID;

import java.util.Enumeration;
import java.util.List;
import java.util.Vector;


/**
 * Provides a simple association of a {@code PeerID} to an ordered list of
 * {@code EndpointAddress} entries. Each {@code EndpointAddress} defines one
 * Message Transport address by which the peer may be reached. The addresses
 * are sorted in the preferred order (which may refer to performance, cost,
 * efficiency, etc.) which they should be used.
 * <p/>
 * The Access Point Advertisement is most commonly used as part of other
 * Advertisements such as {@code RouteAdvertisement}.
 *
 * @see net.jxta.protocol.PeerAdvertisement
 * @see net.jxta.protocol.RouteAdvertisement
 */
public abstract class AccessPointAdvertisement extends ExtendableAdvertisement implements Cloneable {

    /**
     * The peer id of the peer with these endpoints. May be {@code null}
     * if the APA is used as a sub-element of a structure in which the context
     * peerid is already known.
     */
    private PeerID pid = null;

    /**
     * The EndpointAddresses associated with the specified peer in preferred
     * order.
     * <p/>
     * <ul>
     * <li>Values are, sadly, {@link java.lang.String} of
     * {@link net.jxta.endpoint.EndpointAddress}.</li>
     * </ul>
     */
    private Vector<String> endpointAddresses = new Vector<String>();

    /**
     * {@inheritDoc}
     * <p/>
     * <p/>Make a deep copy.
     */
    @Override
    public AccessPointAdvertisement clone() {
        try {
            AccessPointAdvertisement a = (AccessPointAdvertisement) super.clone();

            a.setPeerID(getPeerID());
            a.addEndpointAddresses(endpointAddresses);

            return a;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }
    }

    /**
     * {@inheritDoc}
     * <p/>
     * Equals means the same PID and the same endpoint addresses.
     */
    @Override
    public boolean equals(Object target) {

        if (this == target) {
            return true;
        }

        if (!(target instanceof AccessPointAdvertisement)) {
            return false;
        }

        AccessPointAdvertisement ap = (AccessPointAdvertisement) target;

        if ((null == getPeerID()) && (null != ap.getPeerID())) {
            return false;
        }

        if ((null != getPeerID())) {
            if (!getPeerID().equals(ap.getPeerID())) {
                return false;
            }
        }
        if (endpointAddresses.size() != ap.endpointAddresses.size()) {
            return false;
        }

        // XXX 20061127 bondolo This eventually should be an ordered comparison.

        for (String anEA : endpointAddresses) {
            if (!ap.endpointAddresses.contains(anEA)) {
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
        if (null != pid) {
            return pid.hashCode();
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
        return "jxta:APA";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     * Gets the PeerID for this access point.
     *
     * @return PeerID The peer id associated with the endpoint addresses or
     *         {@code null} if no peer has been directly associated.
     */
    public PeerID getPeerID() {
        return pid;
    }

    /**
     * Sets the PeerID for this access point.
     *
     * @param pid The peer id associated with the endpoint addresses or
     *            {@code null} if no peer is directly associated.
     */
    public void setPeerID(PeerID pid) {
        this.pid = pid;
    }

    /**
     *  Add all of the provided EndpointAddresses.
     *
     *  @param addrs Add all of the specified endpoint addresses.
     */
    public void addEndpointAddresses(List<EndpointAddress> addrs) {
        for (EndpointAddress addr : addrs) {
            addEndpointAddress(addr);
        }
    }
    
    /**
     *  Clears all EndpointAddresses.
     */
    public void clearEndpointAddresses() {
        endpointAddresses.clear();
    }
    
    /**
     *  Remove the specified EndpointAddress.
     *
     *  @param addr EndpointAddress to remove.
     */
    public void removeEndpointAddress(EndpointAddress addr) {
        endpointAddresses.remove(addr.toString());
    }
            
    /**
     *  Remove the specified EndpointAddresses.
     *
     *  @param addrs EndpointAddresses to remove.
     */
    public void removeEndpointAddresses(Collection<EndpointAddress> addrs) {
        for (EndpointAddress addr : addrs) {
            endpointAddresses.remove(addr.toString());
        }
    }
            
    /**
     * Returns the endpoint addresses associated with this access point.
     *
     * @return The endpoint addresses associated with this access point
     *         represented as {@link java.lang.String}.
     */
    public Enumeration<String> getEndpointAddresses() {
        return endpointAddresses.elements();
    }

    /**
     * Returns the vector of endpoint addresses associated with this access
     * point. The result is a vector of endpoint addresses represented as
     * {@code String}. <strong>The Vector contains the "live" data of this
     * advertisement. It should be modified only with great care.</strong>
     *
     * @return The endpoint addresses associated with this access point
     *         represented as {@link java.lang.String}.
     * @deprecated Returning the Vector is dangerous and unwise. This feature
     *             will be removed.
     */
    @Deprecated
    public Vector<String> getVectorEndpointAddresses() {
        return endpointAddresses;
    }

    /**
     * Sets the list of endpoint addresses associated with this access point.
     *
     * @param addresses Vector of EndpointAddresses represented as
     *                  {@link java.lang.String}. <b>The Vector is not copied!</b>
     * @deprecated This method causes the AccessPointAdvertisement to reference
     *             the provided array. This means that subsequent changes to the array will
     *             alter the endpoint addresses which are part of the
     *             {@code AcccessPointAdvertisement}.
     */
    @Deprecated
    public void setEndpointAddresses(Vector<String> addresses) {
        endpointAddresses = addresses;
    }

    /**
     * Add a new list of EndpointAddresses to the access point.
     *
     * @param addresses List of EndpointAddresses represented as
     *                  {@link java.lang.String}.
     *
     * @deprecated Use {@link #addEndpointAddresses(List)} instead.
     */
    @Deprecated
    public void addEndpointAddresses(Vector<String> addresses) {
        for (String toAdd : addresses) {
            addEndpointAddress(toAdd);
        }
    }

    /**
     * Add a new EndpointAddresses to the access point
     *
     * @param address An EndpointAddress
     */
    public void addEndpointAddress(EndpointAddress address) {
        String toAdd = address.toString();

        if (!endpointAddresses.contains(toAdd)) {
            endpointAddresses.add(toAdd);
        }
    }

    /**
     * add a new EndpointAddresses to the access point
     *
     * @param address EndpointAddress represented as {@link java.lang.String}.
     */
    public void addEndpointAddress(String address) {
        if (!endpointAddresses.contains(address)) {
            endpointAddresses.add(address);
        }
    }

    /**
     * remove a list of EndpointAddresses from the access point
     *
     * @param addresses List of EndpointAddresses represented as
     *                  {@link java.lang.String}.
     */
    public void removeEndpointAddresses(List<String> addresses) {
        endpointAddresses.removeAll(addresses);
    }

    /**
     * return number of endpoint addresses
     *
     * @return size number of endpointAddress in the hop
     */
    public int size() {
        return endpointAddresses.size();
    }

    /**
     * Check if the EndpointAddress is already associated with this access point
     *
     * @param addr endpoint address to check
     * @return true if the EndpointAddress is already associated with this access point
     */
    public boolean contains(EndpointAddress addr) {
        return endpointAddresses.contains(addr.toString());
    }

    /**
     * Generate a string that displays an access point
     * information for logging or debugging purpose
     *
     * @return String return a string containing the access point advertisement
     */
    public String display() {
        StringBuilder routeBuf = new StringBuilder();

        routeBuf.append("PID=");

        PeerID peerId = getPeerID();

        if (peerId == null) {
            routeBuf.append("<null>");
        } else {
            routeBuf.append(peerId.toString());
        }

        Enumeration e = getEndpointAddresses();

        while (e.hasMoreElements()) {
            routeBuf.append("\n Addr=").append(e.nextElement());
        }
        return routeBuf.toString();
    }
}
