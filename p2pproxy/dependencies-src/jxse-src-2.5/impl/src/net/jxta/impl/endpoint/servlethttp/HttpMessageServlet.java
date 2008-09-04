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
package net.jxta.impl.endpoint.servlethttp;

import java.io.InputStream;
import java.io.OutputStream;
// import java.util.Enumeration;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Enumeration;
import java.util.NoSuchElementException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;

import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.Cookie;
// import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.linphone.p2pproxy.core.jxtaext.EndpointRegistry;

import net.jxta.document.MimeMediaType;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.EndpointService;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.Messenger;
import net.jxta.endpoint.WireFormatMessage;
import net.jxta.endpoint.WireFormatMessageFactory;

import net.jxta.impl.endpoint.EndpointServiceImpl;
import net.jxta.impl.endpoint.transportMeter.TransportBindingMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeterBuildSettings;
import net.jxta.impl.util.TimeUtils;

/**
 *  This is a simple servlet that accepts JXTA Messages from clients using HTTP
 *  via {@code POST}. In addition to receiving messages via {@code POST}
 *  responses clients can also poll for messages using {@code GET}.
 *
 *  <p/>It also supports a ping operation. When the URI is <tt>/</tt> the
 *  response consists of the unique value portion of the local peer id.
 */
public class HttpMessageServlet extends HttpServlet {

    /**
     * Logger
     */
    private final static transient Logger LOG = Logger.getLogger(HttpMessageServlet.class.getName());

    /**
     *  The maximum duration in milliseconds we will keep a connection alive
     *  while generating a response.
     */
    private final static long MAXIMUM_RESPONSE_DURATION = 2 * TimeUtils.AMINUTE;

    /**
     *  Owner of this servlet.
     */
    private HttpMessageReceiver owner = null;

    /**
     *  The endpoint that the owner message transport is registered with.
     */
    private EndpointService endpoint = null;

    /**
     *  Our address.
     */
    private EndpointAddress localAddress = null;
    private byte[] pingResponseBytes;

    private ServletHttpTransport servletHttpTransport = null;

    /**
     *  If {@code true} then this servlet has been (or is being) destroyed.
     */
    private volatile boolean destroyed = false;

    /**
     *  Recovers the Message Transport which owns this servlet from the context
     *  information.
     */
    @Override
    public void init(ServletConfig config) throws ServletException {
        super.init(config);

        try {
            owner = (HttpMessageReceiver) getServletContext().getAttribute("HttpMessageReceiver");
            if (owner == null) {
                throw new ServletException("Servlet Context did not contain 'HttpMessageReceiver'");
            }
        } catch (ClassCastException e) {
            throw new ServletException("'HttpMessageReceiver' attribute was not of the proper type in the Servlet Context");
        }

        servletHttpTransport = owner.servletHttpTransport;
        endpoint = owner.getEndpointService();

        String peerId = endpoint.getGroup().getPeerID().getUniqueValue().toString();

        localAddress = new EndpointAddress("jxta", peerId, null, null);

        try {
            pingResponseBytes = peerId.getBytes("UTF-8");
        } catch (java.io.UnsupportedEncodingException never) {
            // UTF-8 is always available.
        }
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public void doGet(HttpServletRequest req, HttpServletResponse res) throws ServletException, IOException {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("GET " + req.getRequestURI() + " thread = " + Thread.currentThread());
        }

        processRequest(req, res);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("GET done for thread = " + Thread.currentThread());
        }
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public void doPost(HttpServletRequest req, HttpServletResponse res) throws ServletException, IOException {

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("POST " + req.getRequestURI() + " thread = " + Thread.currentThread());
        }

        processRequest(req, res);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("POST done for thread = " + Thread.currentThread());
        }
    }

    /**
     *  {@inheritDoc}
     */
    @Override
    public synchronized void destroy() {
        // All we need to do is wakeup the threads that are waiting. (In truth
        // we'll miss those that are waiting on a messenger, but that'll do for
        // now, because we do that only when shutting down the group and then
        // the relay will be shutdown as well, which will take care of the
        // messengers.
        destroyed = true;
        notifyAll();
    }

    /**
     *  Handle a request and optionally generate a response.
     *
     *  @param req The request we are being asked to process.
     *  @param res The response we should use for any response.
     *  @throws IOException For failures in IO processing.
     */
    private void processRequest(HttpServletRequest req, HttpServletResponse res) throws IOException {

        long lastReadWriteTime;
        int requestSize = 0;
        TransportBindingMeter transportBindingMeter = null;

        if (Logging.SHOW_FINEST && LOG.isLoggable(Level.FINEST)) {
            printRequest(req);
        }

        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            int contentLength = req.getContentLength();

            requestSize += (contentLength != -1) ? contentLength : 0;
        }

        JxtaRequest currentRequest = new JxtaRequest(req);

        /*
         *  We accept three request formats:
         *
         *  o PING : no Message and no Requestor defined.
         *  o POLL : Requestor defined, positive response timeout and destination address.
         *  o SEND : Requestor defined, positive response timeout or no destination address.
         */

        // check if this is a ping request, no requestor peerId or incoming message
        if (null == currentRequest.requestorAddr && !currentRequest.messageContent) {
            // this is only a ping request
            pingResponse(res);

            if (TransportMeterBuildSettings.TRANSPORT_METERING) {
                long connectionTime = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), currentRequest.requestStartTime);
                EndpointAddress sourceAddress = new EndpointAddress("http", req.getRemoteHost(), null, null); //

                transportBindingMeter = servletHttpTransport.getTransportBindingMeter(null, sourceAddress);
                if (transportBindingMeter != null) {
                    transportBindingMeter.connectionEstablished(false, currentRequest.requestStartTime);
                    transportBindingMeter.dataReceived(false, requestSize);
                    transportBindingMeter.dataSent(false, 0);
                    transportBindingMeter.pingReceived();
                    transportBindingMeter.connectionClosed(false, connectionTime);
                }
            }

            return;
        }

        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            lastReadWriteTime = TimeUtils.timeNow();
            long connectTime = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), currentRequest.requestStartTime);
            EndpointAddress sourceAddress = new EndpointAddress("http", req.getRemoteHost(), null, null); //

            if (null != currentRequest.requestorAddr) {
                transportBindingMeter = servletHttpTransport.getTransportBindingMeter(currentRequest.requestorAddr.toString(),
                        sourceAddress);
            } else {
                transportBindingMeter = servletHttpTransport.getTransportBindingMeter(req.getRemoteHost(), sourceAddress);
            }
            if (transportBindingMeter != null) {
                transportBindingMeter.connectionEstablished(false, connectTime);
                transportBindingMeter.dataReceived(false, requestSize);
            }
        }

        // check if the request included polling (valid requestor peerId and timeout not -1)
        HttpServletMessenger messenger = null;

        if ((null != currentRequest.requestorAddr) && (currentRequest.responseTimeout >= 0) && (null != currentRequest.destAddr)) {
            // create the back channel messenger
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine( "Creating back channel messenger for " + currentRequest.requestorAddr + " (" + currentRequest.destAddr + ")");
            }

            long messengerAliveFor;

            if (0 == currentRequest.responseTimeout) {
                messengerAliveFor = 0;
            } else {
                messengerAliveFor = Math.max(currentRequest.responseTimeout, currentRequest.extraResponsesTimeout);
            }

            messenger = new HttpServletMessenger(owner.servletHttpTransport.group.getPeerGroupID(), localAddress,
                    currentRequest.requestorAddr, messengerAliveFor);
            boolean taken = owner.messengerReadyEvent(messenger, currentRequest.destAddr);

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Incoming messenger to: " + currentRequest.requestorAddr + " taken=" + taken);
            }

            if (!taken) {
                // nobody cares. Just destroy it.
                messenger.close();
                messenger = null;
            }
        }

        // We may later decide that contentLength should not be set after all
        // if we use chunking. Otherwise we must set it; specially to zero, so
        // that jetty does not forcefully close the connection after each
        // message in order to complete the transaction http-1.0-style.

        boolean mustSetContentLength = true;

        try {
            // get the incoming message is there is one
            if (currentRequest.messageContent) {
                Message incomingMessage;

                // read the stream
                InputStream in = req.getInputStream();

                // construct the message. Send BAD_REQUEST if the message construction
                // fails
                try {
                    String contentType = req.getContentType();

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Reading message from request : " + contentType);
                    }

                    MimeMediaType contentMimeType = EndpointServiceImpl.DEFAULT_MESSAGE_TYPE;

                    if (null != contentType) {
                        contentMimeType = MimeMediaType.valueOf(contentType);
                    }

                    // FIXME 20040927 bondolo Should get message encoding from http header.
                    try {
                        incomingMessage = WireFormatMessageFactory.fromWire(in, contentMimeType, null);
                    } catch (NoSuchElementException noValidWireFormat) {
                        IOException failure = new IOException("Unrecognized content type MIME type : " + contentType);

                        failure.initCause(noValidWireFormat);
                        throw failure;
                    }

                    if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                        lastReadWriteTime = TimeUtils.timeNow();
                        long receiveTime = TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), currentRequest.requestStartTime);

                        transportBindingMeter.messageReceived(false, incomingMessage, receiveTime, 0); // size=0 since it was already incorporated in the request size
                    }
                } catch (IOException e) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Malformed JXTA message, responding with BAD_REQUEST", e);
                    }

                    res.sendError(HttpServletResponse.SC_BAD_REQUEST, "Message was not a valid JXTA message");

                    if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                        transportBindingMeter.connectionDropped(false,
                                TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), currentRequest.requestStartTime));
                    }
                    return;
                }

                // post the incoming message to the endpoint demux
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Handing " + incomingMessage + " to the endpoint.");
                }

                try {
                    endpoint.demux(incomingMessage);
                } catch (Throwable e) {
                    if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.log(Level.WARNING, "Failure demuxing an incoming message", e);
                    }
                }
            }

            boolean beganResponse = false;

            // Check if the back channel is to be used for sending messages.
            if ((currentRequest.responseTimeout >= 0) && (null != messenger)) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Wait for message from the messenger. timeout = " + currentRequest.responseTimeout);
                }

                long quitAt = (currentRequest.responseTimeout == 0)
                        ? Long.MAX_VALUE
                        : TimeUtils.toAbsoluteTimeMillis(currentRequest.requestStartTime, currentRequest.responseTimeout);

                while ((0 != (messenger.getState() & Messenger.USABLE)) && !destroyed) {
                    long remaining = TimeUtils.toRelativeTimeMillis(quitAt);

                    if ((remaining <= 0)) {
                        // done processing the request
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Terminating expired request.");
                        }

                        // We know we did not respond anything.
                        // In general it's better if jetty closes the connection
                        // here, because it could have been an unused
                        // back-channel and the client has to open a new one
                        // next time, thus making sure we get to see a different
                        // URL (if applicable). JDK should do that anyway,
                        // but ...).
                        break;
                    }

                    Message outMsg;

                    // Send a message if there is one
                    try {
                        outMsg = messenger.waitForMessage(remaining);
                    } catch (InterruptedException ie) {
                        // Ok. Leave early, then.
                        Thread.interrupted();
                        continue;
                    }

                    if (outMsg == null) {
                        // done processing the request
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Terminating request with no message to send.");
                        }

                        if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                            transportBindingMeter.connectionClosed(false,
                                    TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), currentRequest.requestStartTime));
                        }

                        // We know we did not respond anything. Do not set
                        // content-length In general it's better if jetty closes
                        // the connection here, because it could have been an
                        // unused back-channel and the client has to open a new
                        // one next time, thus making sure we get to see a
                        // different URL (if applicable). JDK should do that
                        // anyway, but ...).
                        break;
                    }

                    long startMessageSend = TimeUtils.timeNow();

                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("Sending " + outMsg + " on back channel to " + req.getRemoteHost());
                    }

                    if (!beganResponse) {
                        // valid request, send back OK response
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Sending OK in response to request");
                        }

                        beganResponse = true;
                        res.setStatus(HttpServletResponse.SC_OK);
                        res.setContentType(EndpointServiceImpl.DEFAULT_MESSAGE_TYPE.toString());
                    }

                    // send the message
                    WireFormatMessage serialed = WireFormatMessageFactory.toWire(outMsg, EndpointServiceImpl.DEFAULT_MESSAGE_TYPE, null);

                    // if only one message is being returned, set the content
                    // length, otherwise try to use chunked encoding.
                    if (currentRequest.extraResponsesTimeout < 0) {
                        res.setContentLength((int) serialed.getByteLength());
                    }

                    // Either way, we've done what had to be done.
                    mustSetContentLength = false;

                    // get the output stream for the response
                    OutputStream out = res.getOutputStream();

                    // send the message
                    try {
                        serialed.sendToStream(out);
                        out.flush();

                        messenger.messageSent(true);

                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Successfully sent " + outMsg + " on back channel to " + req.getRemoteHost());
                        }

                        if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                            lastReadWriteTime = TimeUtils.timeNow();
                            long sendTime = TimeUtils.toRelativeTimeMillis(lastReadWriteTime, startMessageSend);
                            long bytesSent = serialed.getByteLength();

                            transportBindingMeter.messageSent(false, outMsg, sendTime, bytesSent);
                        }
                    } catch (IOException ex) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Failed sending Message on back channel to " + req.getRemoteHost());
                        }

                        messenger.messageSent(false);

                        if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                            transportBindingMeter.connectionDropped(false,
                                    TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), currentRequest.requestStartTime));
                        }

                        throw ex;
                    } finally {
                        // make sure the response is pushed out
                        res.flushBuffer();
                    }

                    // Adjust the quit time based upon the extra response time available.
                    if (0 == currentRequest.extraResponsesTimeout) {
                        quitAt = Long.MAX_VALUE;
                    } else {
                        quitAt = TimeUtils.toAbsoluteTimeMillis(currentRequest.requestStartTime, currentRequest.extraResponsesTimeout);
                    }

                    // If we never generated a response then make it clear we gave up waiting.
                    if (!beganResponse) {
                        res.setStatus(HttpServletResponse.SC_NO_CONTENT);
                    }
                }
            } else {
                // No response was desired.
                res.setStatus(HttpServletResponse.SC_OK);
            }
        } finally {
            // close the messenger
            if (null != messenger) {
                messenger.close();
            }
        }

        // If contentLength was never set and we have not decided *not* to set
        // it, then we must set it to 0 (that's the truth in that case). This
        // allows Jetty to keep to keep the connection open unless what's on the
        // other side is a 1.0 proxy.
        if (mustSetContentLength) {
            res.setContentLength(0);
        }

        // make sure the response is pushed out
        res.flushBuffer();

        // done processing the request
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Finished processing the request from " + req.getRemoteHost());
        }

        if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
            transportBindingMeter.connectionClosed(false,
                    TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), currentRequest.requestStartTime));
        }
    }

    /**
     *  Returns a response to a ping request.  The response is the PeerID of
     *  this peer.
     *
     *  @param res The response to which the ping result should be sent.
     */
    private void pingResponse(HttpServletResponse res) throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Responding to \'ping\' request with 200 and peerID");
        }

        res.setStatus(HttpServletResponse.SC_OK);

        res.setContentLength(pingResponseBytes.length);
        res.setContentType(MimeMediaType.TEXTUTF8.toString());

        OutputStream out = res.getOutputStream();

        out.write(pingResponseBytes);
        out.flush();
        out.close();
    }

    /**
     *  Debugging output.
     */
    private static void printRequest(HttpServletRequest req) {
        final char nl = '\n';
        StringBuilder builder = new StringBuilder();

        builder.append("HTTP request:" + nl);
        builder.append("  AUTH_TYPE: ").append(req.getAuthType()).append(nl);
        builder.append("  CONTEXT_PATH: ").append(req.getContextPath()).append(nl);

        Cookie[] cookies = req.getCookies();

        if (cookies != null) {
            for (int i = 0; i < cookies.length; i++) {
                builder.append("  COOKIE[").append(i).append("]:" + nl);
                builder.append("    comment: ").append(cookies[i].getComment()).append(nl);
                builder.append("    domain: ").append(cookies[i].getDomain()).append(nl);
                builder.append("    max age: ").append(cookies[i].getMaxAge()).append(nl);
                builder.append("    name: ").append(cookies[i].getName()).append(nl);
                builder.append("    path: ").append(cookies[i].getPath()).append(nl);
                builder.append("    secure: ").append(cookies[i].getSecure()).append(nl);
                builder.append("    value: ").append(cookies[i].getValue()).append(nl);
                builder.append("    version: ").append(cookies[i].getVersion()).append(nl);
            }
        }

        for (Enumeration headers = req.getHeaderNames(); headers.hasMoreElements();) {
            String header = (String) headers.nextElement();
            builder.append("  HEADER[").append(header).append("]: ").append(req.getHeader(header)).append(nl);
        }

        builder.append("  METHOD: ").append(req.getMethod()).append(nl);
        builder.append("  PATH_INFO: ").append(req.getPathInfo()).append(nl);
        builder.append("  PATH_TRANSLATED: ").append(req.getPathTranslated()).append(nl);
        builder.append("  QUERY_STRING: ").append(req.getQueryString()).append(nl);
        builder.append("  REMOTE_USER: ").append(req.getRemoteUser()).append(nl);
        builder.append("  REQUESTED_SESSION_ID: ").append(req.getRequestedSessionId()).append(nl);
        builder.append("  REQUEST_URI: ").append(req.getRequestURI()).append(nl);
        builder.append("  SERVLET_PATH: ").append(req.getServletPath()).append(nl);
        builder.append("  REMOTE_USER: ").append(req.getRemoteUser()).append(nl);
        builder.append("  isSessionIdFromCookie: ").append(req.isRequestedSessionIdFromCookie()).append(nl);
        builder.append("  isSessionIdFromURL: ").append(req.isRequestedSessionIdFromURL()).append(nl);
        builder.append("  isSessionIdValid: ").append(req.isRequestedSessionIdValid()).append(nl);

        for (Enumeration attributes = req.getAttributeNames(); attributes.hasMoreElements();) {
            String attribute = (String) attributes.nextElement();
            builder.append("  ATTRIBUTE[").append(attribute).append("]: ").append(req.getAttribute(attribute)).append(nl);
        }

        builder.append("  ENCODING: ").append(req.getCharacterEncoding()).append(nl);
        builder.append("  CONTENT_LENGTH: ").append(req.getContentLength()).append(nl);
        builder.append("  CONTENT_TYPE: ").append(req.getContentType()).append(nl);
        builder.append("  LOCALE: ").append(req.getLocale().toString()).append(nl);

        for (Enumeration parameters = req.getParameterNames(); parameters.hasMoreElements();) {
            String parameter = (String) parameters.nextElement();
            builder.append("  PARAMETER[").append(parameter).append("]: ").append(req.getParameter(parameter)).append(nl);
        }

        builder.append("  PROTOCOL: ").append(req.getProtocol()).append(nl);
        builder.append("  REMOTE_ADDR: ").append(req.getRemoteAddr()).append(nl);
        builder.append("  REMOTE_HOST: ").append(req.getRemoteHost()).append(nl);
        builder.append("  SCHEME: ").append(req.getScheme()).append(nl);
        builder.append("  SERVER_NAME: ").append(req.getServerName()).append(nl);
        builder.append("  SERVER_PORT: ").append(req.getServerPort()).append(nl);
        builder.append("  isSecure: ").append(req.isSecure());

        LOG.finest(builder.toString());
    }

    /**
     *  A servlet request.
     *
     *  @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#trans-httpt-msg-msgs" target="_blank">JXTA Protocols Specification : Standard JXTA Transport Bindings : HTTP Bindings</a>
     */
    private static class JxtaRequest {

        /**
         *  Absolute time in milliseconds at which this request began processing.
         */
        final long requestStartTime;

        /**
         *  Endpoint address of the requestor.
         */
        final EndpointAddress requestorAddr;

        /**
         *  Duration of time to wait for the initial response message.
         *
         *  <p/><ul>
         *      <li><tt>&lt;0</tt> : No response message wanted.</li>
         *      <li><tt> 0</tt> : Wait indefinitely for response message.</li>
         *      <li><tt>&gt;0</tt> : Wait specified amount of time for response message.</li>
         *  </ul>
         */
        final long responseTimeout;

        /**
         *  Duration of time to wait for additional response messages.
         *
         *  <p/><ul>
         *      <li><tt>&lt;0</tt> : No additional response messages wanted.</li>
         *      <li><tt> 0</tt> : Wait indefinitely for additional response messages.</li>
         *      <li><tt>&gt;0</tt> : Wait specified amount of time for additional response messages.</li>
         *  </ul>
         */
        final long extraResponsesTimeout;

        /**
         *  Destination address for messages sent in this connection.
         */
        final EndpointAddress destAddr;

        /**
         *  If <tt>true</tt> then the requestor is providing a Message.
         */
        final boolean messageContent;

        /**
         *  Construct a request.
         */
        JxtaRequest(HttpServletRequest req) {

            requestStartTime = TimeUtils.timeNow();

            // check if a peerId was given
            String requestorPeerId = getRequestorPeerId(req);

            if (null != requestorPeerId) {
                requestorAddr = new EndpointAddress("jxta", requestorPeerId, null, null);
                try {
                  EndpointRegistry.getInstance().add("urn:jxta:"+requestorPeerId, InetAddress.getByName(req.getRemoteHost()));
               } catch (UnknownHostException e) {
                  if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                     LOG.log(Level.WARNING,"cannot register ["+requestorPeerId+"]", e);
                 }
               }
            } else {
                requestorAddr = null;
            }

            // get the query string
            String queryString = req.getQueryString();

            if (queryString != null) {
                // the query string is of the format responseTimeout,extraResponsesTimeout,destAdd
                // the times given are in milliseconds
                int commaIndex = queryString.indexOf(',');

                if (commaIndex == -1) {
                    // there is no extra responses timeout
                    responseTimeout = getResponseTimeout(queryString);
                    extraResponsesTimeout = -1;
                    destAddr = null;
                } else {
                    responseTimeout = getResponseTimeout(queryString.substring(0, commaIndex));

                    String moreQueryParams = queryString.substring(commaIndex + 1);

                    commaIndex = moreQueryParams.indexOf(',');
                    if (commaIndex == -1) {
                        extraResponsesTimeout = getExtraResponsesTimeout(moreQueryParams);
                        destAddr = null;
                    } else {
                        extraResponsesTimeout = getExtraResponsesTimeout(moreQueryParams.substring(0, commaIndex));
                        destAddr = new EndpointAddress(moreQueryParams.substring(commaIndex + 1));
                    }
                }
            } else {
                responseTimeout = 0;
                extraResponsesTimeout = -1;
                destAddr = null;
            }
           
            // check for incoming message
            messageContent = hasMessageContent(req);

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer(
                        "New JXTA Request for Requestor=" + requestorAddr + "\n\tResponse Timeout=" + responseTimeout
                        + "\tAdditional Response Timeout=" + extraResponsesTimeout + "\tRequest Destination Address=" + destAddr
                        + "\tHas Message Content=" + Boolean.toString(messageContent));
            }
        }

        /**
         * Returns the peerId of the peer making the request, if given
         */
        private static String getRequestorPeerId(HttpServletRequest req) {
            // get the potential PeerId from the PathInfo
            String requestorPeerId = req.getPathInfo();

            if (null != requestorPeerId) {
                int begin = 0;
                int end = requestorPeerId.length();

                // check for all leading "/"
                while (begin < end && requestorPeerId.charAt(begin) == '/') {
                    begin++;
                }

                // check for all trailing "/"
                while (end - begin > 0 && requestorPeerId.charAt(end - 1) == '/') {
                    end--;
                }

                if (begin == end) {
                    // nothing left of the string
                    requestorPeerId = null;
                } else {
                    // get the new substring
                    requestorPeerId = requestorPeerId.substring(begin, end);
                }
            }

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("requestorPeerId = " + requestorPeerId);
            }

            return requestorPeerId;
        }

        /**
         * Returns the request timeout or -1 if a request timeout is not given
         */
        private static long getResponseTimeout(String requestTimeoutString) {
            // the default timeout is -1, which means do not return a message
            long timeout = -1;

            try {
                timeout = Long.parseLong(requestTimeoutString);

                // Protect agains clients that will try top have us keep
                // connections for ever. If they re-establish all the time it's
                // fine, but until we have a more sophisticated mechanism, we
                // want to make sure we quit timely if the client's gone.
                if (timeout > MAXIMUM_RESPONSE_DURATION || timeout == 0) {
                    timeout = MAXIMUM_RESPONSE_DURATION;
                }
            } catch (NumberFormatException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("The requestTimeout does not contain a decimal number " + requestTimeoutString);
                }
            }

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("requestTimeout = " + timeout);
            }

            return timeout;
        }

        /**
         * Returns the lazy close timeout or -1 if a lazy close timeout is not given
         */
        private static long getExtraResponsesTimeout(String extraResponseTimeoutString) {
            // the default timeout is -1, which means do not wait for additional messages
            long timeout = -1;

            try {
                timeout = Long.parseLong(extraResponseTimeoutString);

                // Protect agains clients that will try top have us keep
                // connections for ever. If they re-establish all the time it's
                // fine, but until we have a more sophisticated mechanism, we
                // want to make sure we quit timely if the client's gone.
                if (timeout > (2 * TimeUtils.AMINUTE) || timeout == 0) {
                    timeout = (2 * TimeUtils.AMINUTE);
                }
            } catch (NumberFormatException e) {
                if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                    LOG.warning("The extraResponseTimeoutString does not contain a decimal number " + extraResponseTimeoutString);
                }
            }

            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("extraResponseTimeout = " + timeout);
            }

            return timeout;
        }

        /**
         *  Checks if the request includes a message as content.
         *
         *  @param req The request to be inspected.
         *  @return <tt>true</tt> if there is content to be read from this request.
         */
        private static boolean hasMessageContent(HttpServletRequest req) {
            boolean hasContent = false;

            int contentLength = req.getContentLength();

            // if the content length is not zero, there is an incoming message
            // Either the message length is given or it is a chunked message
            if (contentLength > 0) {
                hasContent = true;
            } else if (contentLength == -1) {
                // check if the transfer encoding is chunked
                String transferEncoding = req.getHeader("Transfer-Encoding");

                hasContent = "chunked".equals(transferEncoding);
            }

            if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                LOG.finer("hasMessageContent = " + hasContent);
            }

            return hasContent;
        }
    }
}
