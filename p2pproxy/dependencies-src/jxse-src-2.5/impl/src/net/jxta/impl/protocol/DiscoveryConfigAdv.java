/*
 * Copyright (c) 2004-2007 Sun Microsystems, Inc.  All rights reserved.
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


import java.util.Enumeration;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attributable;
import net.jxta.document.Attribute;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.XMLElement;
import net.jxta.id.ID;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 * Contains parameters for configuration of the Reference Implemenation
 * Rendezvous Service.
 *
 * <p/><pre><code>
 *
 * </code></pre>
 *
 */
public final class DiscoveryConfigAdv extends ExtendableAdvertisement {

    /**
     *  Instantiator for DiscoveryConfigAdv
     */
    public static class Instantiator implements AdvertisementFactory.Instantiator {

        /**
         * {@inheritDoc}
         */
        public String getAdvertisementType() {
            return advType;
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance() {
            return new DiscoveryConfigAdv();
        }

        /**
         * {@inheritDoc}
         */
        public Advertisement newInstance(Element root) {
            return new DiscoveryConfigAdv(root);
        }
    }

    /**
     *  Log4J Logger
     */
    private static final Logger LOG = Logger.getLogger(DiscoveryConfigAdv.class.getName());

    /**
     *  Our DOCTYPE
     */
    private static final String advType = "jxta:DiscoConfigAdv";

    private static final String FORWARD_ALWAYS_REPLICA = "forwardAlwaysReplica";
    private static final String FORWARD_BELOW_TRESHOLD = "forwardBelowThreshold";
    private static final String LOCAL_ONLY = "localOnly";

    private static final String[] fields = {};

    /**
     * If true, the discovery service will always forward queries to the replica peer
     * even if there are local responses, unless the replica peer is the local peer.
     */
    private boolean forwardAlwaysReplica = false;

    /**
     * If true, the discovery service will always forward queries if the number of local
     * responses is below the specified threshold. The threshold may be reduced by the number
     * of local responses before forwarding. NOTE: not yet implemented.
     */
    private boolean forwardBelowTreshold = false;

    /**
     * localOnly discovery.
     */
    private boolean localOnly = false;

    /**
     *  Returns the identifying type of this Advertisement.
     *
     *  <p/><b>Note:</b> This is a static method. It cannot be used to determine
     *  the runtime type of an advertisment. ie.
     *  </p><code><pre>
     *      Advertisement adv = module.getSomeAdv();
     *      String advType = adv.getAdvertisementType();
     *  </pre></code>
     *
     *  <p/><b>This is wrong and does not work the way you might expect.</b>
     *  This call is not polymorphic and calls
     *  Advertiement.getAdvertisementType() no matter what the real type of the
     *  advertisment.
     *
     * @return String the type of advertisement
     */
    public static String getAdvertisementType() {
        return advType;
    }

    /**
     *  Use the Instantiator through the factory
     */
    DiscoveryConfigAdv() {}

    /**
     *  Use the Instantiator through the factory
     * @param root the element
     */
    DiscoveryConfigAdv(Element root) {
        if (!XMLElement.class.isInstance(root)) {
            throw new IllegalArgumentException(getClass().getName() + " only supports XLMElement");
        }

        XMLElement doc = (XMLElement) root;

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

        Enumeration eachAttr = doc.getAttributes();

        while (eachAttr.hasMoreElements()) {
            Attribute aDiscoAttr = (Attribute) eachAttr.nextElement();
            String name = aDiscoAttr.getName();
            boolean flag = Boolean.valueOf(aDiscoAttr.getValue().trim());

            if (FORWARD_ALWAYS_REPLICA.equals(name)) {
                forwardAlwaysReplica = flag;
            } else if (FORWARD_BELOW_TRESHOLD.equals(name)) {
                forwardBelowTreshold = flag;
            } else if (LOCAL_ONLY.equals(name)) {
                localOnly = flag;
            } else {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("Unhandled Attribute: " + name);
                }
            }
        }
    }

    /**
     * Make a safe clone of this DiscoveryConfigAdv.
     *
     * @return Object A copy of this DiscoveryConfigAdv
     */
    @Override
    public DiscoveryConfigAdv clone() throws CloneNotSupportedException {

        throw new CloneNotSupportedException("Not implemented");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getAdvType() {
        return getAdvertisementType();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public final String getBaseAdvType() {
        return getAdvertisementType();
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public ID getID() {
        return ID.nullID;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public Document getDocument(MimeMediaType encodeAs) {
        StructuredDocument adv = (StructuredDocument) super.getDocument(encodeAs);

        if (adv instanceof Attributable) {
            Attributable attrDoc = (Attributable) adv;

            // Do not output if false. It is the default value.
            if (forwardAlwaysReplica) {
                attrDoc.addAttribute(FORWARD_ALWAYS_REPLICA, Boolean.toString(forwardAlwaysReplica));
            }
            if (forwardBelowTreshold) {
                attrDoc.addAttribute(FORWARD_BELOW_TRESHOLD, Boolean.toString(forwardBelowTreshold));
            }
            if (localOnly) {
                attrDoc.addAttribute(FORWARD_BELOW_TRESHOLD, Boolean.toString(localOnly));
            }
        }
        return adv;
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public String[] getIndexFields() {
        return fields;
    }

    /**
     * True if this discovery service will forward queries to the replica peer in all cases, rather
     * than only in the absence of local responses.
     *
     * @return The current setting.
     */
    public boolean getForwardAlwaysReplica() {
        return forwardAlwaysReplica;
    }

    /**
     * Specifies if this discovery service will forward queries to the replica peer in all cases, rather than only in the absence
     * of local responses.
     *
     * @param newvalue The new setting.
     */
    public void setForwardAlwaysReplica(boolean newvalue) {
        forwardAlwaysReplica = newvalue;
    }

    /**
     * True if this discovery service will forward queries when the number of local responses
     * is below the specified treshold, rather than only in the absence of local responses.
     *
     * @return The current setting.
     */
    public boolean getForwardBelowTreshold() {
        return forwardBelowTreshold;
    }

    /**
     * Specifies if this discovery service will forward queries when the number of local responses is below the specified
     * treshold, rather than only in the absence of local responses.
     *
     * @param newvalue The new setting.
     */
    public void setForwardBelowTreshold(boolean newvalue) {
        forwardBelowTreshold = newvalue;
    }

    /**
     * True if this discovery service performs only local discovery.
     *
     * @return The current setting.
     */
    public boolean getLocalOnly() {
        return localOnly;
    }

    /**
     * Specifies if this discovery service will perform local only discovery.
     *
     * @param newvalue The new setting.
     */
    public void setLocalOnly(boolean newvalue) {
        localOnly = newvalue;
    }
}
