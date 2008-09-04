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

package net.jxta.impl.protocol;


import java.util.Arrays;
import java.util.Enumeration;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.XMLElement;
import net.jxta.protocol.TransportAdvertisement;


/**
 * Provides configuration parameters for the JXTA TCP Message Transport.
 */
public class TCPAdv extends TransportAdvertisement {

    /**
     *  Logger
     */
    private static final Logger LOG = Logger.getLogger(TCPAdv.class.getName());

    private static final String CONFIGMODES[] = { "auto", "manual" };
    private static final String INDEXFIELDS[] = {/* none */};

    private static final String PORT_ELEMENT = "Port";
    private static final String ClientOffTag = "ClientOff";
    private static final String ServerOffTag = "ServerOff";
    private static final String MULTICAST_OFF_TAG = "MulticastOff";
    private static final String FlagsTag = "Flags";
    private static final String PublicAddressOnlyAttr = "PublicAddressOnly";

    private String configMode = CONFIGMODES[0];
    private String interfaceAddress = null;
    private int startPort = -1;
    private int listenPort = -1;
    private int endPort = -1;
    private String publicAddress = null;
    private String multicastaddr = null;
    private int multicastport = -1;
    private int multicastsize = -1;
    private boolean clientEnabled = true;
    private boolean serverEnabled = true;
    private boolean multicastEnabled = true;
    private boolean publicAddressOnly = false;

    /**
     *  Our instantiator
     */
    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return TCPAdv.getAdvertisementType();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new TCPAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(net.jxta.document.Element root) {
            if (!XMLElement.class.isInstance(root)) {
                throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
            }

            return new TCPAdv((XMLElement) root);
        }
    }

    /**
     *  Returns the identifying type of this Advertisement.
     *
     *  <p/><b>Note:</b> This is a static method. It cannot be used to determine
     *  the runtime type of an advertisement. ie.
     *  </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     *
     *  <p/><b>This is wrong and does not work the way you might expect.</b>
     *  This call is not polymorphic and calls
     *  Advertisement.getAdvertisementType() no matter what the real type of the
     *  advertisement.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return "jxta:TCPTransportAdvertisement";
    }

    private TCPAdv() {
        setProtocol("tcp");
    }

    private TCPAdv(XMLElement doc) {
        this();
        
        String doctype = doc.getName();

        String typedoctype = "";
        Attribute itsType = doc.getAttribute("type");

        if (null != itsType) {
            typedoctype = itsType.getValue();
        }

        if (!doctype.equals(getAdvertisementType()) && !getAdvertisementType().equals(typedoctype)) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
        }

        Attribute attr = doc.getAttribute(FlagsTag);

        if (attr != null) {
            String options = attr.getValue();

            publicAddressOnly = (options.indexOf(PublicAddressOnlyAttr) != -1);
        }

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Element: " + elem);
                }
            }
        }

        // Sanity Check!!!
        if (!Arrays.asList(CONFIGMODES).contains(configMode)) {
            throw new IllegalArgumentException("Unsupported configuration mode.");
        }

        if ((listenPort < -1) || (listenPort > 65535)) {
            throw new IllegalArgumentException("Illegal Listen Port Value");
        }

        if ((startPort < -1) || (startPort > 65535)) {
            throw new IllegalArgumentException("Illegal Start Port Value");
        }

        if ((endPort < -1) || (endPort > 65535)) {
            throw new IllegalArgumentException("Illegal End Port Value");
        }

        if ((0 == startPort) && (endPort != 0) || (0 != startPort) && (endPort == 0)) {
            throw new IllegalArgumentException("Port ranges must both be 0 or non-0");
        }

        if ((-1 == startPort) && (endPort != -1) || (-1 != startPort) && (endPort == -1)) {
            throw new IllegalArgumentException("Port ranges must both be -1 or not -1");
        }

        if ((null != publicAddress) && ((-1 != startPort) || (listenPort <= 0))) {
            throw new IllegalArgumentException("Dynamic ports not supported with public address port forwarding.");
        }

        if (getMulticastState() && (null == getMulticastAddr())) {
            throw new IllegalArgumentException("Multicast enabled and no address specified.");
        }

        if (getMulticastState() && (-1 == getMulticastPort())) {
            throw new IllegalArgumentException("Multicast enabled and no port specified.");
        }

        if (getMulticastState() && ((getMulticastPort() <= 0) || (getMulticastPort() > 65536))) {
            throw new IllegalArgumentException("Illegal Multicast Port Value");
        }

        if (getMulticastState() && (-1 == getMulticastSize())) {
            throw new IllegalArgumentException("Multicast enabled and no size specified.");
        }

        if (getMulticastState() && ((getMulticastSize() <= 0) || (getMulticastSize() > 1048575L))) {
            throw new IllegalArgumentException("Illegal Multicast datagram size");
        }

        // XXX 20050118 bondolo Some versions apparently don't initialize this field. Eventually make it required.
        if (null == getProtocol()) {
            setProtocol("tcp");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getAdvType() {
        return getAdvertisementType();
    }

    /**
     * Returns the interface which the TCP transport will use.
     *
     * @return The interface to use. May be a DNS name or an IP Address.
     */
    public String getInterfaceAddress() {
        return interfaceAddress;
    }

    /**
     * Sets the interface which the TCP transport will use.
     *
     * @param ia The interface to use. May be a DNS name or an IP Address.
     */
    public void setInterfaceAddress(String ia) {
        if (null != ia) {
            ia = ia.trim();

            if (0 == ia.length()) {
                ia = null;
            }
        }

        this.interfaceAddress = ia;
    }

    /**
     * Returns the port on which the TCP Transport will listen if configured to
     * do so. If a port range is specified then this the preference. Valid
     * values are <code>-1</code>, <code>0</code> and <code>1-65535</code>.
     * The <code>-1</code> value is used to signify that there is no port
     * preference and any port in range will be used. The <code>0</code>
     * specifies that the Socket API dynamic port allocation should be used.
     * For values <code>1-65535</code> the value specifies the required port on
     * which the TCP transport will listen.
     *
     * @return the port
     */
    public int getPort() {
        return listenPort;
    }

    /**
     * Sets the port on which the TCP Transport will listen if configured to
     * do so. If a port range is specified then this the preference. Valid
     * values are <code>-1</code>, <code>0</code> and <code>1-65535</code>.
     * The <code>-1</code> value is used to signify that there is no port
     * preference and any port in range will be used. The <code>0</code>
     * specifies that the Socket API dynamic port allocation should be used.
     * For values <code>1-65535</code> the value specifies the required port on
     * which the TCP transport will listen.
     *
     * @param port the port on which to listen.
     */
    public void setPort(int port) {
        listenPort = port;
    }

    /**
     * Return the lowest port on which the TCP Transport will listen if
     * configured to do so. Valid values are <code>-1</code>, <code>0</code> and
     * <code>1-65535</code>. The <code>-1</code> value is used to signify that
     * the port range feature should be disabled. The <code>0</code> specifies
     * that the Socket API dynamic port allocation should be used. For values
     * <code>1-65535</code> the value must be equal to or less than the value
     * used for end port.
     *
     * @return the lowest port on which to listen.
     */
    public int getStartPort() {
        return startPort;
    }

    /**
     * Sets the lowest port on which the TCP Transport will listen if configured
     * to do so. Valid values are <code>-1</code>, <code>0</code> and
     * <code>1-65535</code>. The <code>-1</code> value is used to signify that
     * the port range feature should be disabled. The <code>0</code> specifies
     * that the Socket API dynamic port allocation should be used. For values
     * <code>1-65535</code> the value must be equal to or less than the value
     * used for end port.
     *
     * @param start the lowest port on which to listen.
     */
    public void setStartPort(int start) {
        startPort = start;
    }

    /**
     * Return the highest port on which the TCP Transport will listen if
     * configured to do so. Valid values are <code>-1</code>, <code>0</code> and
     * <code>1-65535</code>. The <code>-1</code> value is used to signify that
     * the port range feature should be disabled. The <code>0</code> specifies
     * that the Socket API dynamic port allocation should be used. For values
     * <code>1-65535</code> the value must be equal to or greater than the value
     * used for start port.
     *
     * @return the highest port on which to listen.
     */
    public int getEndPort() {
        return endPort;
    }

    /**
     * Sets the highest port on which the TCP Transport will listen if
     * configured to do so. Valid values are <code>-1</code>, <code>0</code> and
     * <code>1-65535</code>. The <code>-1</code> value is used to signify that
     * the port range feature should be disabled. The <code>0</code> specifies
     * that the Socket API dynamic port allocation should be used. For values
     * <code>1-65535</code> the value must be equal to or greater than the value
     * used for start port.
     *
     * @param end the highest port on which to listen.
     */
    public void setEndPort(int end) {
        endPort = end;
    }

    /**
     * Determine whether multicast if off or not
     *
     * @return boolean  current multicast state
     */
    public boolean getMulticastState() {
        return multicastEnabled;
    }

    /**
     * Enable or disable multicast.
     *
     * @param newState the desired state.
     */
    public void setMulticastState(boolean newState) {
        multicastEnabled = newState;
    }

    /**
     * returns the multicastaddr
     *
     * @return string multicastaddr
     */
    public String getMulticastAddr() {
        return multicastaddr;
    }

    /**
     * set the multicastaddr
     *
     * @param multicastaddr set multicastaddr
     */
    public void setMulticastAddr(String multicastaddr) {
        if (null != multicastaddr) {
            multicastaddr = multicastaddr.trim();

            if (0 == multicastaddr.length()) {
                multicastaddr = null;
            }
        }

        this.multicastaddr = multicastaddr;
    }

    /**
     * returns the multicastport
     *
     * @return string multicastport
     */
    public int getMulticastPort() {
        return multicastport;
    }

    /**
     * set the multicastport
     * @param multicastport set multicastport
     */
    public void setMulticastPort(int multicastport) {
        this.multicastport = multicastport;
    }

    /**
     * returns the multicastsize
     *
     * @return integer containting the multicast size
     */
    public int getMulticastSize() {
        return multicastsize;
    }

    /**
     * set the multicastsize
     *
     * @param multicastsize set multicast size
     */
    public void setMulticastSize(int multicastsize) {
        this.multicastsize = multicastsize;
    }

    /**
     * Returns the public address
     *
     * @return string public address
     */
    public String getServer() {
        return publicAddress;
    }

    /**
     * Set the public address. That is, a the address of a server socket
     * to connect to from the outside. Argument is in the form host:port
     *
     * @param address address
     */
    public void setServer(String address) {
        if (null != address) {
            address = address.trim();

            if (0 == address.length()) {
                address = null;
            }
        }

        this.publicAddress = address;
    }

    /**
     *  Returns the configuration for outbound connections.
     *
     *  @return <code>true</code> if outbound connections are allowed otherwise
     *  <code>false</code>
     */
    public boolean isClientEnabled() {
        return clientEnabled;
    }

    /**
     *  Sets the configuration for outbound connections.
     *
     *  @param enabled <code>true</code> if outbound connections are allowed otherwise
     *  <code>false</code>
     */
    public void setClientEnabled(boolean enabled) {
        clientEnabled = enabled;
    }

    /**
     *  Returns the configuration for inbound connections.
     *
     *  @return <code>true</code> if inbound connections are allowed otherwise
     *  <code>false</code>
     */
    public boolean isServerEnabled() {
        return serverEnabled;
    }

    /**
     *  Sets the configuration for inbound connections.
     *
     *  @param enabled <code>true</code> if inbound connections are allowed otherwise
     *  <code>false</code>
     */
    public void setServerEnabled(boolean enabled) {
        serverEnabled = enabled;
    }

    /**
     * returns the config mode. That is, how the user prefers to configure
     * the interface address: "auto", "manual"
     *
     * @return string config mode
     */
    public String getConfigMode() {
        return configMode;
    }

    /**
     * set the config mode. That is, how the user prefers to configure
     * the interface address: "auto", "manual"
     *
     * This is just a pure config item. It is never in published advs. The TCP
     * transport strips it when it initializes.
     *
     * @param mode Can be "auto", "manual" other settings will act as the default
     * which is "auto".
     */
    public void setConfigMode(String mode) {
        if (!Arrays.asList(CONFIGMODES).contains(mode)) {
            throw new IllegalArgumentException("Unsupported configuration mode.");
        }

        configMode = mode;
    }

    /**
     * Returns the state of whether to only use public address
     * @return boolean true if set to use "Public Address Only"
     */
    public boolean getPublicAddressOnly() {
        return publicAddressOnly;
    }

    /**
     * Sets the state of whether to only use public address
     * @param only true to use "Public Address Only"
     */
    public void setPublicAddressOnly(boolean only) {
        publicAddressOnly = only;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    protected boolean handleElement(Element raw) {

        if (super.handleElement(raw)) {
            return true;
        }

        XMLElement elem = (XMLElement) raw;

        if (elem.getName().equals(MULTICAST_OFF_TAG)) {
            setMulticastState(false);
            return true;
        }

        if (elem.getName().equals(ClientOffTag)) {
            setClientEnabled(false);
            return true;
        }

        if (elem.getName().equals(ServerOffTag)) {
            setServerEnabled(false);
            return true;
        }

        String value = elem.getTextValue();

        if ((null == value) || (0 == value.trim().length())) {
            return false;
        }

        value = value.trim();

        if (elem.getName().equals("Protocol")) {
            setProtocol(value);
            return true;
        }

        if (PORT_ELEMENT.equals(elem.getName())) {
            try {
                setPort(Integer.parseInt(value));
                Attribute startAttr = elem.getAttribute("start");
                Attribute endAttr = elem.getAttribute("end");

                if ((null != startAttr) && (null != endAttr)) {
                    setStartPort(Integer.parseInt(startAttr.getValue().trim()));
                    setEndPort(Integer.parseInt(endAttr.getValue().trim()));
                }
            } catch (NumberFormatException badPort) {
                throw new IllegalArgumentException("Illegal port value : " + value);
            }
            return true;
        }

        if (elem.getName().equals("MulticastAddr")) {
            setMulticastAddr(value);
            return true;
        }

        if (elem.getName().equals("MulticastPort")) {
            try {
                setMulticastPort(Integer.parseInt(value));
            } catch (NumberFormatException badPort) {
                throw new IllegalArgumentException("Illegal multicast port value : " + value);
            }
            return true;
        }

        if (elem.getName().equals("MulticastSize")) {
            try {
                int theMulticastSize = Integer.parseInt(value);

                setMulticastSize(theMulticastSize);
            } catch (NumberFormatException badPort) {
                throw new IllegalArgumentException("Illegal multicast datagram size : " + value);
            }
            return true;
        }

        if (elem.getName().equals("Server")) {
            setServer(value);
            return true;
        }

        if (elem.getName().equals("InterfaceAddress")) {
            setInterfaceAddress(value);
            return true;
        }

        if (elem.getName().equals("ConfigMode")) {
            setConfigMode(value);
            return true;
        }

        return false;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        if (!(adv instanceof Attributable)) {
            throw new IllegalStateException("Only Attributable document types allowed.");
        }

        // XXX 20050118 bondolo Some versions apparently don't initialize this field. Eventually make it required.
        if (null == getProtocol()) {
            setProtocol("tcp");
        }

        if ((listenPort < -1) || (listenPort > 65535)) {
            throw new IllegalStateException("Illegal Listen Port Value");
        }

        if ((startPort < -1) || (startPort > 65535)) {
            throw new IllegalStateException("Illegal Start Port Value");
        }

        if ((endPort < -1) || (endPort > 65535)) {
            throw new IllegalStateException("Illegal End Port Value");
        }

        if ((0 == startPort) && (endPort != 0) || (0 != startPort) && (endPort == 0)) {
            throw new IllegalStateException("Port ranges must both be 0 or non-0");
        }

        if ((-1 == startPort) && (endPort != -1) || (-1 != startPort) && (endPort == -1)) {
            throw new IllegalStateException("Port ranges must both be -1 or not -1");
        }

        if ((null != publicAddress) && ((-1 != startPort) || (listenPort <= 0))) {
            throw new IllegalStateException("Dynamic ports not supported with public address port forwarding.");
        }

        if (getMulticastState() && (null == getMulticastAddr())) {
            throw new IllegalStateException("Multicast enabled and no address specified.");
        }

        if (getMulticastState() && (-1 == getMulticastPort())) {
            throw new IllegalStateException("Multicast enabled and no port specified.");
        }

        if (getMulticastState() && ((getMulticastPort() <= 0) || (getMulticastPort() > 65536))) {
            throw new IllegalStateException("Illegal Multicast Port Value");
        }

        if (getMulticastState() && (-1 == getMulticastSize())) {
            throw new IllegalStateException("Multicast enabled and no size specified.");
        }

        if (getMulticastState() && ((getMulticastSize() <= 0) || (getMulticastSize() > 1048575L))) {
            throw new IllegalStateException("Illegal Multicast datagram size");
        }

        if (adv instanceof Attributable) {
            // Only one flag for now. Easy.
            if (publicAddressOnly) {
                ((Attributable) adv).addAttribute(FlagsTag, PublicAddressOnlyAttr);
            }
        }

        Element e11 = adv.createElement("Protocol", getProtocol());

        adv.appendChild(e11);

        if (!isClientEnabled()) {
            Element e19 = adv.createElement(ClientOffTag);

            adv.appendChild(e19);
        }

        if (!isServerEnabled()) {
            Element e20 = adv.createElement(ServerOffTag);

            adv.appendChild(e20);
        }

        if (getConfigMode() != null) {
            Element e18 = adv.createElement("ConfigMode", getConfigMode());

            adv.appendChild(e18);
        }

        String interfaceAddr = getInterfaceAddress();

        if (null != interfaceAddr) {
            Element e17 = adv.createElement("InterfaceAddress", interfaceAddr);

            adv.appendChild(e17);
        }

        Element e12 = adv.createElement(PORT_ELEMENT, Integer.toString(listenPort));

        adv.appendChild(e12);
        if (adv instanceof Attributable) {
            Attributable attrElem = (Attributable) e12;

            if ((-1 != startPort) && (-1 != endPort)) {
                attrElem.addAttribute("start", Integer.toString(startPort));
                attrElem.addAttribute("end", Integer.toString(endPort));
            }
        }

        String serverAddr = getServer();

        if (null != serverAddr) {
            Element e16 = adv.createElement("Server", serverAddr);

            adv.appendChild(e16);
        }

        if (!getMulticastState()) {
            Element e19 = adv.createElement(MULTICAST_OFF_TAG);

            adv.appendChild(e19);
        }

        if (null != getMulticastAddr()) {
            Element e13 = adv.createElement("MulticastAddr", getMulticastAddr());

            adv.appendChild(e13);
        }

        if (-1 != getMulticastPort()) {
            Element e14 = adv.createElement("MulticastPort", Integer.toString(getMulticastPort()));

            adv.appendChild(e14);
        }

        if (-1 != getMulticastSize()) {
            Element e15 = adv.createElement("MulticastSize", Integer.toString(getMulticastSize()));

            adv.appendChild(e15);
        }

        return adv;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return INDEXFIELDS;
    }
}
