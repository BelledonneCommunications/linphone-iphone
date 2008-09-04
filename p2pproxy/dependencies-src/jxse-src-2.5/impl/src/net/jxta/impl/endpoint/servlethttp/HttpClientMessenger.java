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
import java.net.HttpURLConnection;
import java.net.URL;

import java.io.EOFException;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.SocketTimeoutException;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;

import net.jxta.document.MimeMediaType;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.endpoint.WireFormatMessage;
import net.jxta.endpoint.WireFormatMessageFactory;

import net.jxta.impl.endpoint.BlockingMessenger;
import net.jxta.impl.endpoint.EndpointServiceImpl;
import net.jxta.impl.endpoint.transportMeter.TransportBindingMeter;
import net.jxta.impl.endpoint.transportMeter.TransportMeterBuildSettings;
import net.jxta.impl.util.TimeUtils;

/**
 *  Simple messenger that simply posts a message to a URL.
 *
 *  <p/>URL/HttpURLConnection is used, so (depending on your JDK) you will get
 *  reasonably good persistent connection management.
 */
final class HttpClientMessenger extends BlockingMessenger {
    
    /**
     *  Logger
     */
    private final static transient Logger LOG = Logger.getLogger(HttpClientMessenger.class.getName());
    
    /**
     *  Minimum amount of time between poll
     */
    private final static int MIMIMUM_POLL_INTERVAL = (int) (5 * TimeUtils.ASECOND);
    
    /**
     *  Amount of time to wait for connections to open.
     */
    private final static int CONNECT_TIMEOUT = (int) (15 * TimeUtils.ASECOND);
    
    /**
     *  Amount of time we are willing to wait for responses. This is the amount
     *  of time between our finishing sending a message or beginning a poll and 
     *  the beginning of receipt of a response.
     */
    private final static int RESPONSE_TIMEOUT = (int) (2 * TimeUtils.AMINUTE);
    
    /**
     *  Amount of time we are willing to accept for additional responses. This 
     *  is the total amount of time we are willing to wait after receiving an
     *  initial response message whether additional responses are sent or not.
     *  This setting governs the latency with which we switch back and forth 
     *  between sending and receiving messages. 
     */
    private final static int EXTRA_RESPONSE_TIMEOUT = (int) (2 * TimeUtils.AMINUTE);
    
    /**
     *  Messenger idle timeout.
     */
    private final static long MESSENGER_IDLE_TIMEOUT = 15 * TimeUtils.AMINUTE;
    
    /**
     *  Number of attempts we will attempt to make connections.
     */
    private final static int CONNECT_RETRIES = 2;
    
    /**
     *  Warn only once about obsolete proxies.
     */
    private static boolean neverWarned = true;
    
    /**
     *  The URL we send messages to.
     */
    private final URL senderURL;
    
    /**
     * The ServletHttpTransport that created this object.
     */
    private final ServletHttpTransport servletHttpTransport;

    /**
     *  The Return Address element we will add to all messages we send.
     */
    private final MessageElement srcAddressElement;
    
    /**
     *  The logical destination address of this messenger.
     */
    private final EndpointAddress logicalDest;
    
    private TransportBindingMeter transportBindingMeter;
    
    /**
     *  The last time at which we successfully received or sent a message.
     */
    private transient long lastUsed = TimeUtils.timeNow();
    
    /**
     *  Poller that we use to get our messages.
     */
    private MessagePoller poller = null;
    
    /**
     *  Constructs the messenger.
     *
     *  @param servletHttpTransport The transport this messenger will work for.
     *  @param srcAddr The source address.
     *  @param destAddr The destination address.
     */
    HttpClientMessenger(ServletHttpTransport servletHttpTransport, EndpointAddress srcAddr, EndpointAddress destAddr) throws IOException {
        
        // We do use self destruction.
        super(servletHttpTransport.getEndpointService().getGroup().getPeerGroupID(), destAddr, true);
        
        this.servletHttpTransport = servletHttpTransport;

        EndpointAddress srcAddress = srcAddr;
        this.srcAddressElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NAME, srcAddr.toString(), null);
        
        String protoAddr = destAddr.getProtocolAddress();
        
        String host;
        int port;
        int lastColon = protoAddr.lastIndexOf(':');
        
        if ((-1 == lastColon) || (lastColon < protoAddr.lastIndexOf(']')) || ((lastColon + 1) == protoAddr.length())) {
            // There's no port or it's an IPv6 addr with no port or the colon is the last character.
            host = protoAddr;
            port = 80;
        } else {
            host = protoAddr.substring(0, lastColon);
            port = Integer.parseInt(protoAddr.substring(lastColon + 1));
        }
        
        senderURL = new URL("http", host, port, "/");
        
        logicalDest = retreiveLogicalDestinationAddress();
        
        // Start receiving messages from the other peer
        poller = new MessagePoller(srcAddr.getProtocolAddress(), destAddr);
                
        if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
            LOG.info("New messenger : " + this );
        }
    }
    
    /*
     * The cost of just having a finalize routine is high. The finalizer is
     * a bottleneck and can delay garbage collection all the way to heap
     * exhaustion. Leave this comment as a reminder to future maintainers.
     * Below is the reason why finalize is not needed here.
     *
     * These messengers (normally) never go to the application layer. Endpoint
     * code does call close when necessary.
     
     protected void finalize() {
     }
     
     */
    
    /**
     *  {@inheritDoc}
     *  <p/>
     *  A simple implementation for debugging. <b>Do not parse the String
     *  returned. All of the information is available in other (simpler) ways.</b>
     */
    public String toString() {
        StringBuilder result = new StringBuilder(super.toString());
        result.append(" {");
        result.append(getDestinationAddress());
        result.append(" / ");
        result.append(getLogicalDestinationAddress());
        result.append("}");

        return result.toString();
    }
    
    /**
     *  {@inheritDoc}
     */
    void doShutdown() {
        super.shutdown();
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void closeImpl() {
        
        if (isClosed()) {
            return;
        }
        
        super.close();
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Close messenger to " + senderURL);
        }
        
        MessagePoller stopPoller = poller;
        
        poller = null;
        
        if (null != stopPoller) {
            stopPoller.stop();
        }
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public void sendMessageBImpl(Message message, String service, String serviceParam) throws IOException {
        
        if (isClosed()) {
            IOException failure = new IOException("Messenger was closed, it cannot be used to send messages.");
            
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.log(Level.WARNING, "Messenger was closed, it cannot be used to send messages.", failure);
            }
            
            throw failure;
        }
        
        // clone the message before modifying it.
        message = message.clone();
        
        // Set the message with the appropriate src and dest address
        message.replaceMessageElement(EndpointServiceImpl.MESSAGE_SOURCE_NS, srcAddressElement);
        
        EndpointAddress destAddressToUse = getDestAddressToUse(service, serviceParam);
        
        MessageElement dstAddressElement = new StringMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NAME,
                destAddressToUse.toString(), null);
        
        message.replaceMessageElement(EndpointServiceImpl.MESSAGE_DESTINATION_NS, dstAddressElement);
        
        try {
            doSend(message);
        } catch (IOException e) {
            // close this messenger
            close();
            // rethrow the exception
            throw e;
        }
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public EndpointAddress getLogicalDestinationImpl() {
        return logicalDest;
    }
    
    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isIdleImpl() {
        return isClosed() || (TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), lastUsed) > MESSENGER_IDLE_TIMEOUT);
    }
    
    /**
     *  Connects to the http server and retrieves the Logical Destination Address
     */
    private EndpointAddress retreiveLogicalDestinationAddress() throws IOException {
        long beginConnectTime = 0;
        long connectTime = 0;
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Ping (" + senderURL + ")");
        }
        
        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            beginConnectTime = TimeUtils.timeNow();
        }
        
        // open a connection to the other end
        HttpURLConnection urlConn = (HttpURLConnection) senderURL.openConnection();
        
        urlConn.setRequestMethod("GET");
        urlConn.setDoOutput(true);
        urlConn.setDoInput(true);
        urlConn.setAllowUserInteraction(false);
        urlConn.setUseCaches(false);
        urlConn.setConnectTimeout(CONNECT_TIMEOUT);
        urlConn.setReadTimeout(CONNECT_TIMEOUT);
        
        try {
            // this is where the connection is actually made, if not already
            // connected. If we can't connect, assume it is dead
            int code = urlConn.getResponseCode();
            
            if (code != HttpURLConnection.HTTP_OK) {
                if (TransportMeterBuildSettings.TRANSPORT_METERING) {
                    transportBindingMeter = servletHttpTransport.getTransportBindingMeter(null, getDestinationAddress());
                    if (transportBindingMeter != null) {
                        transportBindingMeter.connectionFailed(true, TimeUtils.timeNow() - beginConnectTime);
                    }
                }
                
                throw new IOException("Message not accepted: HTTP status " + "code=" + code + " reason=" + urlConn.getResponseMessage());
            }
            
            // check for a returned peerId
            int msglength = urlConn.getContentLength();
            
            if (msglength <= 0) {
                throw new IOException("Ping response was empty.");
            }
            
            InputStream inputStream = urlConn.getInputStream();
            
            // read the peerId
            byte[] uniqueIdBytes = new byte[msglength];
            int bytesRead = 0;
            
            while (bytesRead < msglength) {
                int thisRead = inputStream.read(uniqueIdBytes, bytesRead, msglength - bytesRead);
                
                if (thisRead < 0) {
                    break;
                }
                
                bytesRead += thisRead;
            }
            
            if (bytesRead < msglength) {
                throw new IOException("Content ended before promised Content length");
            }
            
            String uniqueIdString;
            
            try {
                uniqueIdString = new String(uniqueIdBytes, "UTF-8");
            } catch (UnsupportedEncodingException never) {
                // utf-8 is always available, but we handle it anyway.
                uniqueIdString = new String(uniqueIdBytes);
            }
            
            if (TransportMeterBuildSettings.TRANSPORT_METERING) {
                connectTime = TimeUtils.timeNow();
                transportBindingMeter = servletHttpTransport.getTransportBindingMeter(uniqueIdString, getDestinationAddress());
                if (transportBindingMeter != null) {
                    transportBindingMeter.connectionEstablished(true, connectTime - beginConnectTime);
                    transportBindingMeter.ping(connectTime);
                    transportBindingMeter.connectionClosed(true, connectTime - beginConnectTime);
                }
            }
            
            EndpointAddress remoteAddress = new EndpointAddress("jxta", uniqueIdString.trim(), null, null);
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Ping (" + senderURL + ") -> " + remoteAddress);
            }
            
            return remoteAddress;
        } catch (IOException failure) {
            if (TransportMeterBuildSettings.TRANSPORT_METERING) {
                connectTime = TimeUtils.timeNow();
                transportBindingMeter = servletHttpTransport.getTransportBindingMeter(null, getDestinationAddress());
                if (transportBindingMeter != null) {
                    transportBindingMeter.connectionFailed(true, connectTime - beginConnectTime);
                }
            }
            
            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                LOG.warning("Ping (" + senderURL + ") -> failed");
            }
            
            throw failure;
        }
    }
    
    /**
     *  Connects to the http server and POSTs the message
     */
    private void doSend(Message msg) throws IOException {
        long beginConnectTime = 0;
        long connectTime = 0;
        
        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
            beginConnectTime = TimeUtils.timeNow();
        }
        
        WireFormatMessage serialed = WireFormatMessageFactory.toWire(msg, EndpointServiceImpl.DEFAULT_MESSAGE_TYPE, null);
        
        for (int connectAttempt = 1; connectAttempt <= CONNECT_RETRIES; connectAttempt++) {
            if (connectAttempt > 1) {
                if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                    LOG.fine("Retrying connection to " + senderURL);
                }
            }
            // open a connection to the other end
            HttpURLConnection urlConn = (HttpURLConnection) senderURL.openConnection();

            try {
                urlConn.setRequestMethod("POST");
                urlConn.setDoOutput(true);
                urlConn.setDoInput(true);
                urlConn.setAllowUserInteraction(false);
                urlConn.setUseCaches(false);
                urlConn.setConnectTimeout(CONNECT_TIMEOUT);
                urlConn.setReadTimeout(CONNECT_TIMEOUT);
                // FIXME 20040907 bondolo Should set message encoding http header.
                urlConn.setRequestProperty("content-length", Long.toString(serialed.getByteLength()));
                urlConn.setRequestProperty("content-type", serialed.getMimeType().toString());
                // send the message
                OutputStream out = urlConn.getOutputStream();

                if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                    connectTime = TimeUtils.timeNow();
                    transportBindingMeter.connectionEstablished(true, connectTime - beginConnectTime);
                }
                serialed.sendToStream(out);
                out.flush();
                int responseCode;

                try {
                    responseCode = urlConn.getResponseCode();
                } catch (SocketTimeoutException expired) {
                    // maybe a retry will help.
                    continue;
                } catch (IOException ioe) {
                    // Could not connect. This seems to happen a lot with a loaded HTTP 1.0
                    // proxy. Apparently, HttpUrlConnection can be fooled by the proxy
                    // in believing that the connection is still open and thus breaks
                    // when attempting to make a second transaction. We should not have to but it
                    // seems that it befalls us to retry.
                    if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                        LOG.fine("HTTP 1.0 proxy seems in use");
                    }
                    // maybe a retry will help.
                    continue;
                }
                
                // NOTE: If the proxy closed the connection 1.0 style without returning
                // a status line, we do not get an exception: we get a -1 response code.
                // Apparently, proxies no-longer do that anymore. Just in case, we issue a
                // warning and treat it as OK.71
                if (responseCode == -1) {
                    if (neverWarned && Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                        LOG.warning("Obsolete HTTP proxy does not issue HTTP_OK response. Assuming OK");
                        neverWarned = false;
                    }
                    responseCode = HttpURLConnection.HTTP_OK;
                }
                
                if (responseCode != HttpURLConnection.HTTP_OK) {
                    if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                        transportBindingMeter.dataSent(true, serialed.getByteLength());
                        transportBindingMeter.connectionDropped(true, TimeUtils.timeNow() - beginConnectTime);
                    }
                    throw new IOException( "Message not accepted: HTTP status " + "code=" + responseCode + 
                            " reason=" + urlConn.getResponseMessage());
                }
                
                if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                    long messageSentTime = TimeUtils.timeNow();

                    transportBindingMeter.messageSent(true, msg, messageSentTime - connectTime, serialed.getByteLength());
                    transportBindingMeter.connectionClosed(true, messageSentTime - beginConnectTime);
                }
                
                // note that we successfully sent a message
                lastUsed = TimeUtils.timeNow();
                
                return;
            } finally {
                // This does prevent the creation of an infinite number of connections
                // if we happen to be going through a 1.0-only proxy or connect to a server
                // that still does not set content length to zero for the response. With this, at
                // least we close them (they eventualy close anyway because the other side closes
                // them but it takes too much time). If content-length is set, then jdk ignores
                // the disconnect AND reuses the connection, which is what we want.
                urlConn.disconnect();
            }
        }
        
        throw new IOException("Failed sending " + msg + " to " + senderURL);
    }
    
    /**
     *  Polls for messages sent to us.
     */
    private class MessagePoller implements Runnable {
        
        /**
         *  If <tt>true</tt> then this poller is stopped or stopping.
         */
        private volatile boolean stopped = false;
        
        /**
         *  The thread that does the work.
         */
        private Thread pollerThread;
        
        /**
         *  The URL we poll for messages.
         */
        private final URL pollingURL;
        
        MessagePoller(String pollAddress, EndpointAddress destAddr) {
            
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("new MessagePoller for " + senderURL);
            }
            
            /*
             * query string is of the format ?{response timeout},{extra response timeout},{dest address}
             *
             * The timeout's are expressed in milliseconds. -1 means do not wait
             * at all, 0 means wait forever.
             */
            try {
                pollingURL = new URL(senderURL,
                        "/" + pollAddress + 
                        "?" + Integer.toString(RESPONSE_TIMEOUT) + "," +
                        Integer.toString(EXTRA_RESPONSE_TIMEOUT) + "," + 
                        destAddr);
            } catch (MalformedURLException badAddr) {
                IllegalArgumentException failure = new IllegalArgumentException("Could not construct polling URL");

                failure.initCause(badAddr);
                
                throw failure;
            }
            
            pollerThread = new Thread(this, "HttpClientMessenger poller for " + senderURL);
            pollerThread.setDaemon(true);
            pollerThread.start();
        }
        
        protected void stop() {
            if (stopped) {
                return;
            }
            
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Stop polling for " + senderURL);
            }
            
            stopped = true;
            
            // Here, we are forced to abandon this object open. Because we could
            // get blocked forever trying to close it. It will rot away after
            // the current read returns. The best we can do is interrupt the
            // thread; unlikely to have an effect per the current.
            // HttpURLConnection implementation.
            
            Thread stopPoller = pollerThread;
            
            if (null != stopPoller) {
                stopPoller.interrupt();
            }
        }
        
        /**
         *  Returns {@code true} if this messenger is stopped otherwise 
         *  {@code false}.
         *
         *  @return returns {@code true} if this messenger is stopped otherwise 
         *  {@code false}.
         */
        protected boolean isStopped() {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(this + " " + senderURL + " --> " + (stopped ? "stopped" : "running"));
            }
            
            return stopped;
        }
        
        /**
         *  {@inheritDoc}
         *
         *  <p/>Connects to the http server and waits for messages to be received and processes them.
         */
        public void run() {
            try {
                long beginConnectTime = 0;
                long connectTime = 0;
                long noReconnectBefore = 0;
                HttpURLConnection conn = null;
                
                if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                    LOG.info("Message polling beings for " + pollingURL);
                }
                
                int connectAttempt = 1;
                
                // get messages until the messenger is closed
                while (!isStopped()) {
                    if (conn == null) {
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Opening new connection to " + pollingURL);
                        }
                        
                        conn = (HttpURLConnection) pollingURL.openConnection(); // Incomming data channel
                        
                        conn.setRequestMethod("GET");
                        conn.setDoOutput(false);
                        conn.setDoInput(true);
                        conn.setAllowUserInteraction(false);
                        conn.setUseCaches(false);
                        conn.setConnectTimeout(CONNECT_TIMEOUT);
                        conn.setReadTimeout(RESPONSE_TIMEOUT);
                        
                        if (TransportMeterBuildSettings.TRANSPORT_METERING) {
                            beginConnectTime = TimeUtils.timeNow();
                        }
                        
                        // Loop back and try again to connect
                        continue;
                    }
                    
                    long untilNextConnect = TimeUtils.toRelativeTimeMillis(noReconnectBefore);
                    
                    try {
                        if (untilNextConnect > 0) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Delaying for " + untilNextConnect + "ms before reconnect to " + senderURL);
                            }
                            Thread.sleep(untilNextConnect);
                        }
                    } catch (InterruptedException woken) {
                        Thread.interrupted();
                        continue;
                    }
                    
                    InputStream inputStream;
                    MimeMediaType messageType;
                    
                    try {
                        if (connectAttempt > 1) {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Reconnect attempt for " + senderURL);
                            }
                        }
                        
                        // Always connect (no cost if connected).
                        conn.connect();
                        
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.fine("Waiting for response code from " + senderURL);
                        }
                        
                        int responseCode = conn.getResponseCode();
                        
                        if (Logging.SHOW_FINER && LOG.isLoggable(Level.FINER)) {
                            LOG.finer(
                                    "Response " + responseCode + " for Connection : " + senderURL + "\n\tContent-Type : "
                                    + conn.getHeaderField("Content-Type") + "\tContent-Length : "
                                    + conn.getHeaderField("Content-Length") + "\tTransfer-Encoding : "
                                    + conn.getHeaderField("Transfer-Encoding"));
                        }
                        
                        connectTime = TimeUtils.timeNow();
                        noReconnectBefore = TimeUtils.toAbsoluteTimeMillis(MIMIMUM_POLL_INTERVAL, connectTime);
                        
                        if (0 == conn.getContentLength()) {
                            continue;
                        }
                        
                        if (HttpURLConnection.HTTP_NO_CONTENT == responseCode) {
                            // the connection timed out.
                            if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                                transportBindingMeter.connectionClosed(true, TimeUtils.toRelativeTimeMillis(beginConnectTime, connectTime));
                            }
                            
                            conn = null;
                            continue;
                        }
                        
                        if (responseCode != HttpURLConnection.HTTP_OK) {
                            if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                                transportBindingMeter.connectionClosed(true, TimeUtils.timeNow() - beginConnectTime);
                            }
                            
                            throw new IOException("HTTP Failure: " + conn.getResponseCode() + " : " + conn.getResponseMessage());
                        }
                        
                        String contentType = conn.getHeaderField("Content-Type");

                        if (null == contentType) {
                            // XXX 20051219 bondolo Figure out why the mime type is not always set.
                            messageType = EndpointServiceImpl.DEFAULT_MESSAGE_TYPE;
                        } else {
                            messageType = MimeMediaType.valueOf(contentType);
                        }
                        
                        // FIXME 20040907 bondolo Should get message content-encoding from http header.
                        
                        inputStream = conn.getInputStream();
                        
                        // reset connection attempt.
                        connectAttempt = 1;
                    } catch (InterruptedIOException broken) {
                        // We don't know where it was interrupted. Restart connection.
                        Thread.interrupted();
                        
                        if (connectAttempt > CONNECT_RETRIES) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.warning("Unable to connect to " + senderURL);
                            }
                            
                            stop();
                            break;
                        } else {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Failed connecting to " + senderURL);
                            }
                            
                            if (null != conn) {
                                conn.disconnect();
                            }
                            conn = null;
                            connectAttempt++;
                            continue;
                        }
                    } catch (IOException ioe) {
                        if (connectAttempt > CONNECT_RETRIES) {
                            if (Logging.SHOW_WARNING && LOG.isLoggable(Level.WARNING)) {
                                LOG.log(Level.WARNING, "Unable to connect to " + senderURL, ioe);
                            }
                            
                            stop();
                            break;
                        } else {
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Failed connecting to " + senderURL);
                            }
                            
                            if (null != conn) {
                                conn.disconnect();
                            }
                            conn = null;
                            connectAttempt++;
                            continue;
                        }
                    }
                    
                    // start receiving messages
                    try {
                        while (!isStopped()
                                && (TimeUtils.toRelativeTimeMillis(TimeUtils.timeNow(), connectTime) < RESPONSE_TIMEOUT)) {
                            // read a message!
                            long messageReceiveStart = TimeUtils.timeNow();
                            Message incomingMsg;
                            
                            incomingMsg = WireFormatMessageFactory.fromWire(inputStream, messageType, null);
                            
                            if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                                transportBindingMeter.messageReceived(true, incomingMsg, incomingMsg.getByteLength(),
                                        TimeUtils.timeNow() - messageReceiveStart);
                            }
                            
                            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                                LOG.fine("Received " + incomingMsg + " from " + senderURL);
                            }
                            
                            servletHttpTransport.executor.execute(new MessageProcessor(incomingMsg));
                            
                            // note that we received a message
                            lastUsed = TimeUtils.timeNow();
                        }

                        if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                            transportBindingMeter.connectionClosed(true, TimeUtils.timeNow() - beginConnectTime);
                        }
                    } catch (EOFException e) {
                        // Connection ran out of messages. let it go.
                        conn = null;
                    } catch (InterruptedIOException broken) {
                        if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                            transportBindingMeter.connectionDropped(true, TimeUtils.timeNow() - beginConnectTime);
                        }
                        
                        // We don't know where it was interrupted. Restart connection.
                        Thread.interrupted();
                        if (null != conn) {
                            conn.disconnect();
                        }
                        conn = null;
                    } catch (IOException e) {
                        if (TransportMeterBuildSettings.TRANSPORT_METERING && (transportBindingMeter != null)) {
                            transportBindingMeter.connectionDropped(true, TimeUtils.timeNow() - beginConnectTime);
                        }
                        
                        // If we managed to get down here, it is really an error.
                        // However, being disconnected from the server, for
                        // whatever reason, is a common place event. No need to
                        // clutter the screen with scary messages. When the
                        // message layer believes it's serious, it prints the
                        // scary message already.
                        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                            LOG.log(Level.FINE, "Failed to read message from " + senderURL, e);
                        }
                        // Time to call this connection dead.
                        stop();
                        break;
                    } finally {
                        try {
                            inputStream.close();
                        } catch (IOException ignored) {
                            //ignored
                        }
                    }
                }
            } catch (Throwable argh) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Poller exiting because of uncaught exception", argh);
                }
                stop();
            } finally {
                pollerThread = null;
            }
            
            if (Logging.SHOW_INFO && LOG.isLoggable(Level.INFO)) {
                LOG.info("Message polling stopped for " + senderURL);
            }
        }
    }

    /**
     * A small class for processing individual messages. 
     */ 
    private class MessageProcessor implements Runnable {
        
        private Message msg;
        
        MessageProcessor(Message msg) {
            this.msg = msg;
        }
        
        public void run() {
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine("Demuxing " + msg + " from " + senderURL);
            }
            
            servletHttpTransport.getEndpointService().demux(msg);
        }
    }
}
