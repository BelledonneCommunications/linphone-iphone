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

package net.jxta.impl.endpoint.msgframing;


import java.io.EOFException;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.ByteBuffer;
import java.nio.BufferUnderflowException;
import java.util.Arrays;
import java.util.List;
import java.text.MessageFormat;

import net.jxta.endpoint.EndpointAddress;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;

import java.util.logging.Level;
import net.jxta.logging.Logging;
import java.util.logging.Logger;


/**
 * Contains a JXTA connection Welcome Message. The Welcome Message is sent by
 * both participant peers as the first interchange on newly opened connections.
 * <p/>
 * <p/>The Welcome Message contains the following information:
 * <ul>
 * <li>The address to which the local peer believes it is connected.</li>
 * <li>The local peer's return address, the source address.</li>
 * <li>The local peer's peer id.</li>
 * <li>A flag which controls propagation behaviour for this conneciton.</li>
 * </ul>
 *
 * @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#trans-tcpipt"
 *      target="_blank">JXTA Protocols Specification : TCP/IP Message Transport</a>
 */
public class WelcomeMessage {

    /**
     * Log4J Logger
     */
    private static final Logger LOG = Logger.getLogger(WelcomeMessage.class.getName());

    /**
     * The Welcome Message Signature/Preamble
     */
    private final static String GREETING = "JXTAHELLO";

    /**
     * A space for separating elements of the welcome message.
     */
    private final static String SPACE = " ";

    /**
     * Version string for Welcome Message Version 1.1
     */
    private final static String WELCOME_VERSION_1_1 = "1.1";

    /**
     * Version string for Welcome Message Version 3.0
     */
    private final static String WELCOME_VERSION_3_0 = "3.0";

    /**
     * The current welcome message version. This is the only version we will emit.
     */
    private final static String CURRENTVERSION = WELCOME_VERSION_1_1;

    /**
     * The destination address that we believe we are connecting to.
     */
    private EndpointAddress destinationAddress;

    /**
     * Our return address, the purported source address of this connection.
     */
    private EndpointAddress publicAddress;

    /**
     * Our peerid, the logical return address.
     */
    private ID peerID;

    /**
     * This connection does not wish to receive any propagation/broadcast/notifications.
     */
    private boolean noPropagate;

    /**
     * The preferred binary wire message format version.
     */
    private int preferredMessageVersion;

    /**
     * The welcome message version we are supporting
     */
    private String versionString;

    /**
     * The welcome message as a text string.
     */
    private String welcomeString;

    /**
     * The welcome message as UTF-8 byte stream.
     */
    private byte[] welcomeBytes;
    private final int MAX_LEN = 4096;

    /**
     * Default constructor
     */
    public WelcomeMessage() {}

    /**
     * Creates a new instance of WelcomeMessage for our Welcome Message.
     *
     * @param destAddr      The destination address that we believe we are connecting to.
     * @param publicaddress Our return address, the purported source address of this connection.
     * @param peerid        Our peerid, the logical return address.
     * @param dontPropagate If <tt>true</tt> this connection does not wish to receive any propagation/broadcast/notifications.
     */
    public WelcomeMessage(EndpointAddress destAddr, EndpointAddress publicaddress, ID peerid, boolean dontPropagate) {
        this(destAddr, publicaddress, peerid, dontPropagate, 0);
    }

    /**
     * Creates a new instance of WelcomeMessage for our Welcome Message.
     *
     * @param destAddr            The destination address that we believe we are connecting to.
     * @param publicaddress       Our return address, the purported source address of this connection.
     * @param peerid              Our peerid, the logical return address.
     * @param dontPropagate       If <tt>true</tt> this connection does not wish to receive any propagation/broadcast/notifications.
     * @param preferredMsgVersion Binary Wire Messsage format we prefer.
     */
    public WelcomeMessage(EndpointAddress destAddr, EndpointAddress publicaddress, ID peerid, boolean dontPropagate, int preferredMsgVersion) {
        destinationAddress = destAddr;
        publicAddress = publicaddress;
        peerID = peerid;
        noPropagate = dontPropagate;
        versionString = CURRENTVERSION;
        preferredMessageVersion = preferredMsgVersion;

        welcomeString = GREETING + SPACE + destAddr.toString() + SPACE + publicAddress.toString() + SPACE + peerID.toString()
                + SPACE + (noPropagate ? "1" : "0") + SPACE + versionString;

        try {
            welcomeBytes = welcomeString.getBytes("UTF-8");
        } catch (UnsupportedEncodingException never) {// all implementations must support utf-8
        }
    }

    /**
     * Creates a new instance of WelcomeMessage for another peer's Welcome Message
     *
     * @param in The InputStream to read the welcome message from.
     * @throws IOException If there is a problem reading the welcome header.
     */
    public WelcomeMessage(InputStream in) throws IOException {
        welcomeBytes = new byte[MAX_LEN];
        int readAt = 0;
        boolean sawCR = false;
        boolean sawCRLF = false;

        // read the welcome message
        do {
            int c = in.read();

            switch (c) {
            case -1:
                throw new EOFException("Stream terminated before end of welcome message");

            case '\r':
                if (sawCR) {
                    welcomeBytes[readAt++] = (byte) 0x0D;
                }
                sawCR = true;
                break;

            case '\n':
                if (sawCR) {
                    sawCRLF = true;
                } else {
                    welcomeBytes[readAt++] = (byte) 0x0A;
                }
                break;

            default:
                welcomeBytes[readAt++] = (byte) c;
                sawCR = false;
            }

            if (readAt == welcomeBytes.length) {
                throw new IOException("Invalid welcome message, too long");
            }

        } while (!sawCRLF);

        byte[] truncatedBytes = new byte[readAt];

        System.arraycopy(welcomeBytes, 0, truncatedBytes, 0, readAt);
        welcomeBytes = truncatedBytes;
        welcomeString = new String(welcomeBytes, "UTF-8");
        parseWelcome(welcomeString);
    }

    /**
     * Attempts to init a welcome object from a socketChannel
     *
     * @param buffer the data buffer
     * @return null if incomplete welcome was received
     * @throws IOException if an io error occurs
     */
    public boolean read(ByteBuffer buffer) throws IOException {
        int limit = buffer.limit();

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("Reading a buffer of size :{0}", limit));
        }
        if (limit == 0) {
            throw new IOException(MessageFormat.format("Invalid welcome message. Invalid length {0}", limit));
        }
        int eomPos = findEom(buffer, 0, limit);

        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("Buffer size :{0} Welcome End-Of-Message pos :{1}", limit, eomPos));
        }
        if (eomPos < 0) {
            return false;
        }
        welcomeBytes = new byte[eomPos];
        try {
            buffer.get(welcomeBytes, 0, eomPos);
            // skip <cr><ln>
            buffer.position(eomPos + 2);
            if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
                LOG.fine(MessageFormat.format("buffer stats :{0}", buffer.toString()));
            }
        } catch (BufferUnderflowException buf) {
            // not enough data, signal for another read
            return false;
        }
        welcomeString = new String(welcomeBytes, "UTF-8");
        parseWelcome(welcomeString);
        return true;
    }

    /**
     * returns position of <cr><lf> position in buffer, otherwise -1
     *
     * @param buffer the byte buffer
     * @param offset The offset within the buffer array
     * @param length the length
     * @return terminating position, or -1 if none found
     */
    private int findEom(ByteBuffer buffer, int offset, int length) {

        int lastOffset = length - 2; // we are looking for 2 chars.
        
        for (int j = offset; j <= lastOffset; j++) {
            byte c = buffer.get(j);
           
            if (c == '\r') {
                c = buffer.get(j + 1);
               
                if (c == '\n') {
                    return j;
                }
            }
        }
        
        return -1;
    }

    private void parseWelcome(String welcomeString) throws IOException {
        List<String> thePieces = Arrays.asList(welcomeString.split("\\s"));

        if (0 == thePieces.size()) {
            throw new IOException("Invalid welcome message, did not contain any tokens.");
        }

        if (thePieces.size() < 5) {
            throw new IOException("Invalid welcome message, did not contain enough tokens.");
        }

        if (!GREETING.equals(thePieces.get(0))) {
            throw new IOException("Invalid welcome message, did not start with greeting");
        }

        try {
            destinationAddress = new EndpointAddress(thePieces.get(1));
        } catch (IllegalArgumentException badAddress) {
            IOException failed = new IOException("Invalid welcome message, bad destination address");

            failed.initCause(badAddress);
            throw failed;
        }

        try {
            publicAddress = new EndpointAddress(thePieces.get(2));
        } catch (IllegalArgumentException badAddress) {
            IOException failed = new IOException("Invalid welcome message, bad publicAddress address");

            failed.initCause(badAddress);
            throw failed;
        }

        try {
            URI peerURI = new URI(thePieces.get(3));

            peerID = IDFactory.fromURI(peerURI);
        } catch (URISyntaxException badURI) {
            IOException failed = new IOException("Invalid welcome message, bad peer id");

            failed.initCause(badURI);
            throw failed;
        }

        versionString = thePieces.get(thePieces.size() - 1);

        if (WELCOME_VERSION_1_1.equals(versionString)) {
            if (6 != thePieces.size()) {
                throw new IOException("Invalid welcome message, incorrect number of tokens.");
            }

            String noPropagateStr = thePieces.get(4);

            if (noPropagateStr.equals("1")) {
                noPropagate = true;
            } else if (noPropagateStr.equals("0")) {
                noPropagate = false;
            } else {
                throw new IOException("Invalid welcome message, illegal value for propagate flag");
            }

            // preferred message version is not set in
            preferredMessageVersion = 0;
        } else if (WELCOME_VERSION_3_0.equals(versionString)) {
            if (7 != thePieces.size()) {
                throw new IOException("Invalid welcome message, incorrect number of tokens.");
            }

            String noPropagateStr = thePieces.get(4);

            if (noPropagateStr.equals("1")) {
                noPropagate = true;
            } else if (noPropagateStr.equals("0")) {
                noPropagate = false;
            } else {
                throw new IOException("Invalid welcome message, illegal value for propagate flag");
            }

            String preferredVersionStr = thePieces.get(5);

            try {
                preferredMessageVersion = Integer.valueOf(preferredVersionStr);
            } catch (IllegalArgumentException failed) {
                IOException failure = new IOException("Invalid welcome message, illegal value for preferred message version");

                failure.initCause(failed);

                throw failure;
            }
        } else {
            // Unrecognized Welcome message version. Use default values.
            noPropagate = false;
            preferredMessageVersion = 0;
        }
        
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine("Successfuly parsed a welcome message :" + getWelcomeString());
        }
    }

    /**
     * Write the welcome message to the provided stream.
     *
     * @param theStream The OutputStream to which to write the welcome message.
     * @throws IOException If there is a problem writing the welcome message.
     */
    public void sendToStream(OutputStream theStream) throws IOException {
        theStream.write(welcomeBytes);
        theStream.write('\r');
        theStream.write('\n');
    }

    /**
     * Write the welcome to a socket channel
     *
     * @return A ByteBuffer of the welcome message
     * @throws java.io.IOException if an io error occurs
     */
    public ByteBuffer getByteBuffer() throws IOException {
        if (Logging.SHOW_FINE && LOG.isLoggable(Level.FINE)) {
            LOG.fine(MessageFormat.format("Sending welcome message of size:{0}", welcomeBytes.length + 2));
        }
        ByteBuffer buffer = ByteBuffer.allocate(welcomeBytes.length + 2);

        buffer.put(welcomeBytes);
        buffer.put((byte) '\r');
        buffer.put((byte) '\n');
        buffer.flip();
        return buffer;
    }

    /**
     * Return the peerid associated with the Welcome Message.
     *
     * @return The peer ID from the Welcome Message.
     */
    public ID getPeerID() {
        return peerID;
    }

    /**
     * Return the source address associated with the Welcome Message.
     *
     * @return The source address from the Welcome Message.
     */
    public EndpointAddress getPublicAddress() {
        return publicAddress;
    }

    /**
     * Return the destination address associated with the Welcome Message.
     *
     * @return The destination address from the Welcome Message.
     */
    public EndpointAddress getDestinationAddress() {
        return destinationAddress;
    }

    /**
     * Return the propagation preference from the Welcome Message.
     *
     * @return <tt>true</tt> if <strong>no</strong> propagation is desired
     *         otherwise <tt>false</tt>
     */
    public boolean dontPropagate() {
        return noPropagate;
    }

    /**
     * Return the preferred message version from the Welcome Message.
     *
     * @return The preferred Message Version.
     */
    public int getPreferredMessageVersion() {
        return preferredMessageVersion;
    }

    /**
     * Return the version associated with the Welcome Message.
     *
     * @return The version from the Welcome Message.
     */
    public String getWelcomeVersion() {
        return versionString;
    }

    /**
     * Return a String containing the Welcome Message.
     *
     * @return a String containing the Welcome Message.
     */
    public String getWelcomeString() {
        return welcomeString;
    }
}
