/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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
package net.jxta.impl.rendezvous.limited;

import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.XMLDocument;
import net.jxta.endpoint.EndpointListener;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.peergroup.PeerGroup;

import net.jxta.impl.protocol.LimitedRangeRdvMsg;
import net.jxta.impl.rendezvous.RdvWalk;
import net.jxta.impl.rendezvous.rpv.PeerView;

/**
 * This class is the Limited Walk Policy.
 *
 * @see net.jxta.impl.rendezvous.limited.LimitedRangeWalker
 * @see net.jxta.impl.rendezvous.limited.LimitedRangeGreeter
 */
public class LimitedRangeWalk extends RdvWalk {

    /**
     * The prefix we will use for service name we use for messaging.
     */
    public static final String SERVICENAME = "LR-Greeter";

    /**
     * The name of the message element in which we will store our information.
     */

    public static final String ELEMENTNAME = "LimitedRangeRdvMessage";

    /**
     * The PeerView we walk.
     */
    private final PeerView rpv;

    /**
     * The service name for listener of this walk. All walkers in the same
     * peer group have the same service name.
     */
    private final String walkSvcName;

    /**
     * The service param for listener of this walk. The walker param is
     * composed of the walk service name concated with the walk service param.
     */
    private final String walkSvcParam;

    /**
     * Our walker (sender)
     */
    private LimitedRangeWalker walker = null;

    /**
     * Our greeter (listener)
     */
    private LimitedRangeGreeter greeter = null;

    /**
     * Returns the parsed LimitedRangeRdvMsg from the provided message or
     * {@code null} if the message did not contain an appropriate element or
     * the element couldn't be parsed.
     *
     * @param msg the Message which must contain the LimitedRangeRdvMsg.
     * @return The LimitedRangeRdvMsg from the message or {@code null}.
     */
    static LimitedRangeRdvMsg getRdvMessage(Message msg) {
        MessageElement el = msg.getMessageElement("jxta", ELEMENTNAME);

        if (el == null) {
            // The sender did not use this protocol
            return null;
        }

        try {
            XMLDocument asDoc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(el);

            return new LimitedRangeRdvMsg(asDoc);
        } catch (Exception ez) {
            return null;
        }
    }

    /**
     * Standard constructor
     *
     * @param group           Peergroup in which this walk is running.
     * @param listener        Intended recipient of messages received as part of this walk.
     * @param srcServiceName  Service name used by the client of this walk.
     * @param srcServiceParam Optional service parameter used by the client of this walk.
     * @param rpv             the rendezvous peer PeerView to be used by this walk.
     */
    public LimitedRangeWalk(PeerGroup group, EndpointListener listener, String srcServiceName, String srcServiceParam, PeerView rpv) {
        super(group, listener, srcServiceName, srcServiceParam);

        this.rpv = rpv;

        this.walkSvcName = SERVICENAME + group.getPeerGroupID().toString();
        this.walkSvcParam = srcServiceName + srcServiceParam;

        this.walker = new LimitedRangeWalker(this);
        this.greeter = new LimitedRangeGreeter(this);
    }

    /**
     * Return the Rendezvous peer PeerView used by this walk.
     *
     * @return The rendezvous peer PeerView used by this walk.
     */
    PeerView getPeerView() {
        return rpv;
    }

    /**
     * Return the Service Name used by listener of this walk.
     *
     * @return the Service Name used by listener of this walk.
     */
    String getWalkServiceName() {
        return walkSvcName;
    }

    /**
     * Return the Service Param used by listener of this walk.
     *
     * @return the Service Param used by listener of this walk.
     */
    String getWalkServiceParam() {
        return walkSvcParam;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LimitedRangeWalker getWalker() {
        return this.walker;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LimitedRangeGreeter getGreeter() {
        return this.greeter;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void stop() {
        if (walker != null) {
            walker.stop();
            walker = null;
        }

        if (greeter != null) {
            greeter.stop();
            greeter = null;
        }
    }
}
