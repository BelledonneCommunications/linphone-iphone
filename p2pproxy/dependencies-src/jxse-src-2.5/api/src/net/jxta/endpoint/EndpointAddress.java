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
package net.jxta.endpoint;

import net.jxta.id.ID;
import net.jxta.logging.Logging;

import java.lang.ref.SoftReference;
import java.net.URI;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Describes a destination to which JXTA messages may be sent. This may be:
 * <p/>
 *
 *  <ul>
 *      <li>A Pipe</li>
 *      <li>A Peergroup (propagate)</li>
 *      <li>A Peer</li>
 *      <li>A Message Transport for a Peer</li>
 *  </ul>
 *<p/>
 *  An Endpoint Address is a specialized interpretation of a URI.
 *  Wherever it makes sense you should use a URI in preference to an Endpoint
 *  Address. An Endpoint Address is composed of four components: a protocol
 *  (also called a scheme), a protocol address (also called an authority), an
 *  optional service name and optional service parameter.
 *<p/>
 *  <b>The Protocol</b><ul>
 *      <li>Describes the method of addressing used by the remainder of the
 *      endpoint address.</li>
 *      <li>Indicates how the address will be resolved, ie. who will resolve it.</li>
 *      <li>Corresponds to the "scheme" portion of a URI in W3C parlance.
 *      <li><b>May not</b> contain the ":" character.
 *  </ul>
 *<p/>
 *  <b>The Protocol Address</b><ul>
 *      <li>Describes the destination entity of this address.</li>
 *      <li>Form is dependant upon the protocol being used.</li>
 *      <li>Corresponds to the "Authority" portion of a URI in W3C parlance.
 *      <li><b>May not</b> contain the "/" character.
 *  </ul>
 *<p/>
 *  <b>The Service Name</b> (optional)<ul>
 *      <li>Describes the service that is the destination. A service cannot be
 *      a protocol address because a service must have a location; a group or a
 *      specific peer.</li>
 *      <li>Form is dependant upon service intent. This is matched as a UTF8
 *      string.</li>
 *      <li><b>May not</b> contain the "/" character.
 *  </ul>
 *
 *  <p/><b>The Service Parameter</b> (optional)<ul>
 *      <li>Describes parameters for the service.</li>
 *      <li>Form is dependant upon service intent. This is matched as a UTF-8
 *      string (if it is used for matching).</li>
 *  </ul>
 *
 * @see net.jxta.endpoint.EndpointService
 * @see net.jxta.endpoint.MessageTransport
 * @see net.jxta.endpoint.Messenger
 * @see net.jxta.pipe.PipeService
 */
public class EndpointAddress {

    /**
     * Logger
     */
    private static final Logger LOG = Logger.getLogger(EndpointAddress.class.getName());

    /**
     * If {@code true} then endpoint addresses based upon IDs are represented
     * using the "jxta://" form. If false then they are presented using the
     * "urn:jxta:" form. The two forms are meant to be logically equivalent.
     */
    private final static boolean IDS_USE_JXTA_URL_FORM = true;

    /**
     * The default protocol value for Endpoint Addresses based upon JXTA IDs.
     */
    private final static String JXTA_ID_PROTOCOL = ID.URIEncodingName + ":" + ID.URNNamespace;

    /**
     * if true then the address is a url, otherwise its a uri (likely a urn).
     */
    private boolean hierarchical = true;

    /**
     * Describes the method of addressing used by the remainder of the
     * endpoint address.
     */
    private String protocol = null;

    /**
     * Describes the destination entity of this address.
     */
    private String protocolAddress = null;

    /**
     * Describes the service that is the destination.
     */
    private String service = null;

    /**
     * Describes parameters for the service.
     */
    private String serviceParam = null;

    /**
     * cached calculated hash code.
     */
    private transient int cachedHashCode = 0;

    /**
     * cached copy of string representation.
     */
    private transient SoftReference<String> cachedToString = null;

    /**
     * Returns an unmodifiable clone of the provided EndpointAddress.
     *
     * @param address the address to be cloned.
     * @return the unmodifiable address clone.
     * @deprecated All EndpointAddresses are now unmodifiable so this method is
     *             no longer needed.
     */
    @Deprecated
    public static EndpointAddress unmodifiableEndpointAddress(EndpointAddress address) {
        return address;
    }

    /**
     * Builds an Address from a string
     *
     * @param address the string representation of the address.
     */
    public EndpointAddress(String address) {
        parseURI(address);
    }

    /**
     * Create an EndpointAddress whose value is initialized from the provided
     * URI.
     *
     * @param address the URI representation of the address.
     */
    public EndpointAddress(URI address) {
        this(address.toString());
    }

    /**
     * Constructor which builds an endpoint address from a base address and
     * replacement service and params
     *
     * @param base         The EndpointAddress on which the new EndpointAddress will be based
     * @param service      The service name for the endpoint address or
     *                     {@code null} if there is no service name.
     * @param serviceParam The service parameter for the endpoint address or
     *                     {@code null} if there is no parameter.
     */
    public EndpointAddress(EndpointAddress base, String service, String serviceParam) {
        setProtocolName(base.getProtocolName());
        setProtocolAddress(base.getProtocolAddress());
        setServiceName(service);
        setServiceParameter(serviceParam);
    }

    /**
     * Constructor which builds an address the four standard constituent parts.
     *
     * @param protocol     The addressing scheme to be used for the endpoint address.
     * @param address      The destination for the endpoint address.
     * @param service      The service name for the endpoint address or
     *                     {@code null} if there is no service name.
     * @param serviceParam The service parameter for the endpoint address or
     *                     {@code null} if there is no parameter.
     */
    public EndpointAddress(String protocol, String address, String service, String serviceParam) {
        setProtocolName(protocol);
        setProtocolAddress(address);
        setServiceName(service);
        setServiceParameter(serviceParam);
    }

    /**
     * Constructor which builds an address from a standard jxta id and a
     * service and param.
     *
     * @param id           the ID which will be the destination of the endpoint address.
     * @param service      The service name for the endpoint address or
     *                     {@code null} if there is no service name.
     * @param serviceParam The service parameter for the endpoint address or
     *                     {@code null} if there is no parameter.
     */
    public EndpointAddress(ID id, String service, String serviceParam) {
        setProtocolName(JXTA_ID_PROTOCOL);
        setProtocolAddress(id.getUniqueValue().toString());
        setServiceName(service);
        setServiceParameter(serviceParam);
    }

    /**
     * {@inheritDoc}
     *
     * @deprecated EndpointAddress objects are immutable and never need to be
     *             cloned.
     */
    @Override
    @Deprecated
    public EndpointAddress clone() {
        return this;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }

        if (target instanceof EndpointAddress) {
            EndpointAddress likeMe = (EndpointAddress) target;

            boolean result = (hierarchical == likeMe.hierarchical) && protocol.equalsIgnoreCase(likeMe.protocol)
                    && protocolAddress.equalsIgnoreCase(likeMe.protocolAddress)
                    && ((service != null)
                    ? ((likeMe.service != null) && service.equals(likeMe.service))
                    : (likeMe.service == null))
                    && ((serviceParam != null)
                    ? ((likeMe.serviceParam != null) && serviceParam.equals(likeMe.serviceParam))
                    : (likeMe.serviceParam == null));

            return result;
        }

        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        if (0 == cachedHashCode) {
            int calcedHashCode = protocol.toLowerCase().hashCode();

            calcedHashCode += protocolAddress.hashCode() * 5741; // a prime
            calcedHashCode += ((service != null) ? service.hashCode() : 1) * 7177; // a prime
            calcedHashCode += ((serviceParam != null) ? serviceParam.hashCode() : 1) * 6733; // a prime

            cachedHashCode = (0 == calcedHashCode) ? 1 : calcedHashCode;
        }

        return cachedHashCode;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized String toString() {
        String result;

        if (null != cachedToString) {
            result = cachedToString.get();

            if (null != result) {
                return result;
            }
        }

        StringBuilder newResult = new StringBuilder(protocol.length() + protocolAddress.length() + 64);

        newResult.append(protocol);

        if (hierarchical) {
            newResult.append("://");
        } else {
            newResult.append(':');
        }

        newResult.append(protocolAddress);

        if (null != service) {
            if (hierarchical) {
                newResult.append('/');
            } else {
                newResult.append('#');
            }
            newResult.append(service);

            if (null != serviceParam) {
                newResult.append('/');
                newResult.append(serviceParam);
            }
        }

        result = newResult.toString();

        cachedToString = new SoftReference<String>(result);

        return result;
    }

    /**
     * Return a URI which represents the endpoint address.
     *
     * @return a URI which represents the endpoint address.
     */
    public URI toURI() {
        return URI.create(toString());
    }

    /**
     * Return a String that contains the name of the protocol
     * contained in the EndpointAddress
     *
     * @return a String containing the protocol name
     */
    public String getProtocolName() {
        return protocol;
    }

    /**
     * Return a String that contains the protocol address contained
     * in the EndpointAddress
     *
     * @return a String containing the protocol address
     */
    public String getProtocolAddress() {
        return protocolAddress;
    }

    /**
     * Return a String that contains the service name contained in
     * the EndpointAddress
     *
     * @return a String containing the service name
     */
    public String getServiceName() {
        return service;
    }

    /**
     * Return a String that contains the service parameter contained
     * in the EndpointAddress
     *
     * @return a String containing the protocol name
     */
    public String getServiceParameter() {
        return serviceParam;
    }

    /**
     * Set the protocol name.
     *
     * @param name String containing the name of the protocol
     */
    private void setProtocolName(String name) {
        if ((null == name) || (0 == name.length())) {
            throw new IllegalArgumentException("name must be non-null and contain at least one character");
        }

        if (-1 != name.indexOf("/")) {
            throw new IllegalArgumentException("name may not contain '/' character");
        }

        // XXX 20070207 bondolo We explicitly force all use of either "jxta" or "urn:jxta" to our prefered form.
        if (IDS_USE_JXTA_URL_FORM) {
            if (JXTA_ID_PROTOCOL.equals(name)) {
                name = "jxta";
            }
        } else {
            if ("jxta".equals(name)) {
                name = JXTA_ID_PROTOCOL;
            }
        }

        int colonAt = name.indexOf(':');

        if (-1 == colonAt) {
            hierarchical = true;
        } else {
            if (!"urn".equalsIgnoreCase(name.substring(0, colonAt))) {
                throw new IllegalArgumentException("Only urn may contain colon");
            }

            if (colonAt == (name.length() - 1)) {
                throw new IllegalArgumentException("empty urn namespace!");
            }

            hierarchical = false;
        }

        protocol = name;
        cachedToString = null;
    }

    /**
     * Set the protocol address.
     *
     * @param address String containing the peer address.
     */
    private void setProtocolAddress(String address) {
        if ((null == address) || (0 == address.length())) {
            throw new IllegalArgumentException("address must be non-null and contain at least one character");
        }

        if (-1 != address.indexOf("/")) {
            throw new IllegalArgumentException("address may not contain '/' character");
        }

        protocolAddress = address;
        cachedToString = null;
    }

    /**
     * Set the service name.
     *
     * @param name String containing the name of the destination service
     */
    private void setServiceName(String name) {
        if (null != name) {
            if (-1 != name.indexOf("/")) {
                throw new IllegalArgumentException("service name may not contain '/' character");
            }
        }

        service = name;
        cachedToString = null;
    }

    /**
     * Set the service parameter
     *
     * @param param String containing the service parameter
     */
    private void setServiceParameter(String param) {
        serviceParam = param;
        cachedToString = null;
    }

    /**
     * Parse any EndpointAddress from a URI
     *
     * @param addr endpoint address to parse
     */
    private void parseURI(String addr) {
        int index = addr.indexOf("://");

        if (index == -1) {
            parseURN(addr);
        } else {
            parseURL(addr);
        }
    }

    /**
     * Parse an EndpointAddress from a URN
     *
     * @param addr endpoint address to parse
     */
    private void parseURN(String addr) {
        int protocolEnd = addr.indexOf(':');

        if (-1 == protocolEnd) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Address is not a valid URI: " + addr);
            }
            throw new IllegalArgumentException("Address is not a valid URI: " + addr);
        }

        if (!"urn".equalsIgnoreCase(addr.substring(0, protocolEnd))) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Address is unrecognized URI form: " + addr);
            }
            throw new IllegalArgumentException("Address is unrecognized URI form: " + addr);
        }

        if ((addr.length() - 1) == protocolEnd) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Address URN does not have a namespace: " + addr);
            }
            throw new IllegalArgumentException("Address URN does not have a namespace: " + addr);
        }

        // gather the namespace as well.
        int namespaceEnd = addr.indexOf(':', protocolEnd + 1);

        if (-1 == namespaceEnd) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Address URN does not have a namespace: " + addr);
            }
            throw new IllegalArgumentException("Address URN does not have a namespace: " + addr);
        }

        setProtocolName(addr.substring(0, namespaceEnd));

        if ((addr.length() - 1) == namespaceEnd) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Address URN does not have a NSS portion: " + addr);
            }
            throw new IllegalArgumentException("Address URN does not have a NSS portion: " + addr);
        }

        // check for service and param
        int nssEnd = addr.indexOf('#', namespaceEnd + 1);

        if (-1 == nssEnd) {
            setProtocolAddress(addr.substring(namespaceEnd + 1));
        } else {
            setProtocolAddress(addr.substring(namespaceEnd + 1, nssEnd));

            int serviceEnd = addr.indexOf('/', nssEnd + 1);

            if (-1 == serviceEnd) {
                setServiceName(addr.substring(nssEnd + 1));
            } else {
                setServiceName(addr.substring(nssEnd + 1, serviceEnd));

                setServiceParameter(addr.substring(serviceEnd + 1));
            }
        }
    }

    /**
     * Parse and EndpointAddress from a URL
     *
     * @param addr endpoint address to parse
     */
    private void parseURL(String addr) {
        String remainder;

        int index = addr.indexOf("://");

        if (index == -1) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Address is not in absolute form: " + addr);
            }
            throw new IllegalArgumentException("Address is not in absolute form: " + addr);
        }

        if (0 == index) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Protocol is missing: " + addr);
            }
            throw new IllegalArgumentException("Protocol is missing: " + addr);
        }

        try {
            setProtocolName(addr.substring(0, index));
            remainder = addr.substring(index + 3);
        } catch (Exception e) {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Protocol address is missing: " + addr);
            }
            throw new IllegalArgumentException("Protocol address is missing: " + addr);
        }
        index = remainder.indexOf("/");
        if (index == -1) {
            setProtocolAddress(remainder);
            return;
        }

        setProtocolAddress(remainder.substring(0, index));

        remainder = remainder.substring(index + 1);

        index = remainder.indexOf("/");
        if (index == -1) {
            setServiceName(remainder);
            return;
        }

        setServiceName(remainder.substring(0, index));

        remainder = remainder.substring(index + 1);

        setServiceParameter(remainder);
    }
}
