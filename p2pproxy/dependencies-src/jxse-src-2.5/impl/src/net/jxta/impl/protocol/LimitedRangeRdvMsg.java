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


import net.jxta.document.*;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peer.PeerID;
import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;


/**
 * The LimitedRangeRdv walk header message.
 * <p/>
 * <p/><pre><code>
 * &lt;xs:simpleType name="WalkDirection">
 *   &lt;xs:restriction base="xs:unsignedInt">
 *     &lt;!-- UP -->
 *     &lt;xs:enumeration value="1" />
 *     &lt;!-- DOWN -->
 *     &lt;xs:enumeration value="2" />
 *     &lt;!-- BOTH -->
 *     &lt;xs:enumeration value="3" />
 *   &lt;/xs:restriction>
 * &lt;/xs:simpleType>
 * <p/>
 * &lt;xs:complexType name="LimitedRangeRdvMessage">
 *   &lt;xs:sequence>
 *     &lt;xs:element name="TTL" type="xs:unsignedInt" />
 *     &lt;xs:element name="Dir" type="jxta:WalkDirection" />
 *     &lt;xs:element name="SrcPeerID" type="jxta:JXTAID" />
 *     &lt;xs:element name="SrcSvcName" type="xs:string" />
 *     &lt;xs:element name="SrcSvcParams" minOccurs="0" type="xs:string" />
 *   &lt;/xs:sequence>
 * &lt;/xs:complexType>
 * </code></pre>
 *
 * @see net.jxta.impl.rendezvous.limited.LimitedRangeWalk
 * @see net.jxta.impl.rendezvous.limited.LimitedRangeWalker
 * @see net.jxta.impl.rendezvous.limited.LimitedRangeGreeter
 * @since JXTA 2.0
 */
public class LimitedRangeRdvMsg {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(LimitedRangeRdvMsg.class.getName());

    private final static String TTL_ELEMENT = "TTL";
    private final static String DIRECTION_ELEMENT = "Dir";
    private final static String SRCPEERID_ELEMENT = "SrcPeerID";
    private final static String SRCSVCNAME_ELEMENT = "SrcSvcName";
    private final static String SRCSVCPARAM_ELEMENT = "SrcSvcParams";

    private int ttl = 0;
    private WalkDirection direction = null;
    private PeerID srcPeerID = null;
    private String srcSvcName = null;
    private String srcSvcParams = null;

    /**
     * Enumeration of possible walk directions.
     */
    public enum WalkDirection {
        UP(1), DOWN(2), BOTH(3);

        /**
         * The protocol integer value associated with this direction.
         */
        private final int proto_direction;

        private WalkDirection(int direction) {
            proto_direction = direction;
        }

        /**
         * Convert a walk code as used by the standard walker protocol to a
         * direction object.
         *
         * @param code the protocol code
         * @return A direction object.
         * @throws IllegalArgumentException For illegal protocol codes.
         */
        public static WalkDirection toWalkDirection(int code) {
            switch (code) {
            case 1:
                return UP;

            case 2:
                return DOWN;

            case 3:
                return BOTH;

            default:
                throw new IllegalArgumentException("Illegal direction");
            }
        }

        /**
         * Return the protocol code used by the standard walker protocol for
         * this WalkDirection.
         *
         * @return the walk direction as a numeric value for use in protocol
         *         messages.
         */
        public int toProtocolCode() {
            return proto_direction;
        }
    }

    /**
     * Constructor
     */
    public LimitedRangeRdvMsg() {}

    /**
     * Construct from a StructuredDocument
     *
     * @param root the element
     */
    public LimitedRangeRdvMsg(Element root) {
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XMLElement");
        }

        XMLElement doc = (XMLElement) root;

        if (!doc.getName().equals(getMessageType())) {
            throw new IllegalArgumentException(
                    "Could not construct : " + getClass().getName() + " from doc containing a '" + doc.getName()
                    + "'. Should be : " + getMessageType());
        }

        Enumeration elements = doc.getChildren();

        while (elements.hasMoreElements()) {
            XMLElement elem = (XMLElement) elements.nextElement();

            if (!handleElement(elem)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Unhandled Element: " + elem);
                }
            }
        }

        // Sanity check time!
        if (getTTL() < 1) {
            throw new IllegalArgumentException("Illegal TTL value.");
        }

        if (null == getDirection()) {
            throw new IllegalArgumentException("No Direction specified.");
        }

        if (null == getSrcPeerID()) {
            throw new IllegalArgumentException("No source peer id specified.");
        }

        if (null == getSrcSvcName()) {
            throw new IllegalArgumentException("No source service name specified.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LimitedRangeRdvMsg clone() {

        try {
            LimitedRangeRdvMsg clone = (LimitedRangeRdvMsg) super.clone();

            clone.setSrcPeerID(getSrcPeerID());
            clone.setDirection(getDirection());
            clone.setTTL(getTTL());
            clone.setSrcSvcName(getSrcSvcName());
            clone.setSrcSvcParams(getSrcSvcParams());

            return clone;
        } catch (CloneNotSupportedException impossible) {
            throw new Error("Object.clone() threw CloneNotSupportedException", impossible);
        }
    }

    /**
     * Get the TTL
     *
     * @return Time To Live
     */
    public int getTTL() {
        return ttl;
    }

    /**
     * set the TTL
     *
     * @param ttl TTL
     */
    public void setTTL(int ttl) {
        this.ttl = ttl;
    }

    /**
     * Get the direction the message will take
     *
     * @return UP, DOWN or BOTH
     */
    public WalkDirection getDirection() {
        return direction;
    }

    /**
     * Set the direction the message will take
     *
     * @param dir direction
     */
    public void setDirection(WalkDirection dir) {
        direction = dir;
    }

    /**
     * Get the Source Service Name (listening for the response)
     *
     * @return Source Service Name
     */
    public String getSrcSvcName() {
        return srcSvcName;
    }

    /**
     * Set the Source Service Name (listening for the response)
     *
     * @param srcSvcName Source Service Name
     */
    public void setSrcSvcName(String srcSvcName) {
        this.srcSvcName = srcSvcName;
    }

    /**
     * Get the Source Service Param (listening for the response)
     *
     * @return Source Service Param
     */
    public String getSrcSvcParams() {
        return srcSvcParams;
    }

    /**
     * Set the Source Service Params (listening for the response)
     *
     * @param srcSvcParams Source Service Params
     */
    public void setSrcSvcParams(String srcSvcParams) {
        this.srcSvcParams = srcSvcParams;
    }

    /**
     * Get the Source PeerID (walk originiator)
     *
     * @return Source PeerID
     */
    public ID getSrcPeerID() {
        return srcPeerID;
    }

    /**
     * Set the Source PeerID (walk originiator)
     *
     * @param srcPeerID Source PeerID
     */
    public void setSrcPeerID(ID srcPeerID) {
        this.srcPeerID = (PeerID) srcPeerID;
    }

    /**
     * Our DOCTYPE
     *
     * @return the type of this message.
     */
    public static String getMessageType() {
        return "jxta:LimitedRangeRdvMessage";
    }

    /**
     * Process an individual element from the document during parse. Normally,
     * implementations will allow the base advertisments a chance to handle the
     * element before attempting ot handle the element themselves. ie.
     * <p/>
     * <p/><pre><code>
     *  protected boolean handleElement( Element elem ) {
     * <p/>
     *      if ( super.handleElement() ) {
     *           // it's been handled.
     *           return true;
     *           }
     * <p/>
     *      <i>... handle elements here ...</i>
     * <p/>
     *      // we don't know how to handle the element
     *      return false;
     *      }
     *  </code></pre>
     *
     * @param elem the element to be processed.
     * @return true if the element was recognized, otherwise false.
     */
    protected boolean handleElement(XMLElement elem) {

        String value = elem.getTextValue();

        if (null != value) {
            value = value.trim();

            if (0 == value.length()) {
                value = null;
            }
        }

        if (null == value) {
            return false;
        }

        if (elem.getName().equals(TTL_ELEMENT)) {
            setTTL(Integer.parseInt(value));
            return true;
        }

        if (elem.getName().equals(DIRECTION_ELEMENT)) {
            int direction = Integer.parseInt(value);

            setDirection(LimitedRangeRdvMsg.WalkDirection.toWalkDirection(direction));
            return true;
        }

        if (elem.getName().equals(SRCPEERID_ELEMENT)) {
            try {
                URI srcURI = new URI(value);
                ID srcID = IDFactory.fromURI(srcURI);

                setSrcPeerID(srcID);
            } catch (URISyntaxException badID) {
                IllegalArgumentException iae = new IllegalArgumentException("Bad ID in message");

                iae.initCause(badID);
                throw iae;
            }
            return true;
        }

        if (elem.getName().equals(SRCSVCNAME_ELEMENT)) {
            setSrcSvcName(value);
            return true;
        }

        if (elem.getName().equals(SRCSVCPARAM_ELEMENT)) {
            setSrcSvcParams(value);
            return true;
        }

        return false;
    }

    /**
     * @inheritDoc
     */
    public Document getDocument(MimeMediaType mediaType) {

        if (getTTL() < 1) {
            throw new IllegalStateException("Illegal TTL value.");
        }

        if (null == getDirection()) {
            throw new IllegalStateException("No Direction specified.");
        }

        if (null == getSrcPeerID()) {
            throw new IllegalStateException("No source peer id specified.");
        }

        if (null == getSrcSvcName()) {
            throw new IllegalStateException("No source service name specified.");
        }

        StructuredDocument msg = StructuredDocumentFactory.newStructuredDocument(mediaType, getMessageType());

        if (msg instanceof XMLDocument) {
            ((XMLDocument) msg).addAttribute("xmlns:jxta", "http://jxta.org");
        }

        Element e = msg.createElement(TTL_ELEMENT, Integer.toString(getTTL()));

        msg.appendChild(e);

        e = msg.createElement(DIRECTION_ELEMENT, Integer.toString(getDirection().toProtocolCode()));
        msg.appendChild(e);

        e = msg.createElement(SRCPEERID_ELEMENT, getSrcPeerID().toString());
        msg.appendChild(e);

        e = msg.createElement(SRCSVCNAME_ELEMENT, getSrcSvcName());
        msg.appendChild(e);

        if (getSrcSvcParams() != null) {
            e = msg.createElement(SRCSVCPARAM_ELEMENT, getSrcSvcParams());
            msg.appendChild(e);
        }

        return msg;
    }
}
