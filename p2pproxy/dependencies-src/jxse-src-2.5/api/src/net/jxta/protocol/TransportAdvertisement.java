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


import net.jxta.document.ExtendableAdvertisement;
import net.jxta.id.ID;


/**
 * This abstract class defines a Transport advertisement. Each peer endpoint
 * protocol is associated with a transport advertisement that describes the
 * protocol and network interface associated with the endpoint transport. For
 * example for TCP endpoint. the following transport information needs to be
 * maintained for this endpoint:
 * <p/>
 * <p>Transport :
 * <ul type-disc>
 * <li><p>Protocol : TCP
 * <li><p>Port : 6001
 * <li><p>MulticastAddr : 224.0.1.85
 * <li><p>MulticastPort : 1234
 * <li><p>MulticastSize : 8192
 * </ul>
 * <p/>
 * <p>This class is an abstract class that needs to be extended by implementation
 * to support the different types of transport advertisements (TCP, HTTP, etc)
 * <p/>
 * <pre>
 * &lt;?xml version="1.0"?>
 *  &lt;TransportAdvertisement type="HTTPAdvertisement">
 *          ..........
 *  &lt;/TransportAdvertisement>
 * </pre>
 *
 * @see net.jxta.protocol.AccessPointAdvertisement
 */


public abstract class TransportAdvertisement extends ExtendableAdvertisement {

    protected String protocol = null;

    /**
     * Returns the identifying type of this Advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:TransportAdvertisement";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     * Return ID for indexing. We don't have one so return the nullID.
     *
     * @return jxta id associated with this advertisement.
     */
    @Override
    public ID getID() {
        return ID.nullID;
    }

    /**
     * Sets the URI scheme to be used for EndpointAddresses of this Message Transport.
     *
     * @return The URI scheme used for EndpointAddresses of this Message Transport.
     */
    public String getProtocol() {
        return protocol;
    }

    /**
     * Returns the URI scheme to be used for EndpointAddresses of this Message Transport.
     *
     * @param protocol The URI scheme used for EndpointAddresses of this Message Transport.
     */
    public void setProtocol(String protocol) {
        this.protocol = protocol;
    }
}
