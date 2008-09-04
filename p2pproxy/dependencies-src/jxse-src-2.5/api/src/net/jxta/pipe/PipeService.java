/*
 *  Copyright (c) 2001-2006 Sun Microsystems, Inc. All rights reserved.
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
package net.jxta.pipe;


import java.io.IOException;
import java.util.Set;

import net.jxta.id.ID;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.service.Service;


/**
 *  Pipes are the core mechanism for exchanging messages between JXTA
 *  applications or services.
 *
 *  <p/>Pipes are uniquely identified by a
 *  {@link net.jxta.protocol.PipeAdvertisement} which is associated with each
 *  pipe.
 *
 *  <p/>Several types of Pipe can currently be used:
 *
 *  <ul>
 *      <li><tt>JxtaUnicast</tt> - unicast, unreliable and unsecured pipe
 *      <li><tt>JxtaUnicastSecure</tt> - unicast and secure pipe
 *      <li><tt>JxtaPropagate</tt> - propagated, unreliable and unsecured pipe
 *  </ul>
 *
 *  <p/>The type of a Pipe is defined when creating its
 *  {@link net.jxta.protocol.PipeAdvertisement}.
 *
 *  @see net.jxta.protocol.PipeAdvertisement
 *  @see net.jxta.pipe.InputPipe
 *  @see net.jxta.pipe.OutputPipe
 *  @see net.jxta.endpoint.Message
 *  @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#overview-pipes" target='_blank'>JXTA Protocols Specification : Pipes</a>
 *  @see <a href="https://jxta-spec.dev.java.net/nonav/JXTAProtocols.html#proto-pbp" target='_blank'>JXTA Protocols Specification : Pipe Binding Protocol</a>
 */
public interface PipeService extends Service {

    /**
     * Unicast, unreliable and unsecured type of Pipe
     */
    public final static String UnicastType = "JxtaUnicast";

    /**
     * Propagated, unsecured and unreliable type of Pipe
     */
    public final static String PropagateType = "JxtaPropagate";

    /**
     * End-to-end secured unicast pipe of Pipe
     */
    public final static String UnicastSecureType = "JxtaUnicastSecure";

    /**
     * Create an InputPipe from a pipe Advertisement
     *
     * @param adv The advertisement of the Pipe.
     * @return The InputPipe created.
     * @throws IOException  error creating input pipe
     */
    public InputPipe createInputPipe(PipeAdvertisement adv) throws IOException;

    /**
     * Create an InputPipe from a pipe Advertisement
     *
     * @param adv is the advertisement of the Pipe.
     * @param listener PipeMsgListener to receive msgs.
     * @return InputPipe The InputPipe created.
     * @throws IOException Error creating input pipe
     */
    public InputPipe createInputPipe(PipeAdvertisement adv, PipeMsgListener listener) throws IOException;

    /**
     * Attempt to create an OutputPipe using the specified Pipe Advertisement.
     * The pipe will be be resolved within the provided timeout.
     *
     * @param pipeAdv The advertisement of the pipe being resolved.
     * @param timeout Time duration in milliseconds to wait for a successful
     * pipe resolution. <tt>0</tt> will wait indefinitely.
     * @return OutputPipe the successfully resolved OutputPipe.
     * @throws IOException  If the pipe cannot be created or failed to resolve
     * within the specified time.
     */
    public OutputPipe createOutputPipe(PipeAdvertisement pipeAdv, long timeout) throws IOException;

    /**
     * Attempt to create an OutputPipe using the specified Pipe Advertisement.
     * The pipe will be be resolved to one of the peers in the set of peer ids
     * provided within the provided timeout.
     *
     * @param pipeAdv The advertisement of the pipe being resolved.
     * @param resolvablePeers The peers on which the pipe may be resolved.
     * <strong>If the Set is empty then the pipe may be resolved to any 
     * destination peer.</strong>
     * @param timeout Time duration in milliseconds to wait for a successful
     * pipe resolution. <tt>0</tt> will wait indefinitely.
     * @return The successfully resolved OutputPipe.
     * @throws IOException If the pipe cannot be created or failed to resolve
     * within the specified time.
     */
    public OutputPipe createOutputPipe(PipeAdvertisement pipeAdv, Set<? extends ID> resolvablePeers, long timeout) throws IOException;

    /**
     * Attempt to create an OutputPipe using the specified Pipe Advertisement.
     * The pipe may be resolved to any destination peer. When the pipe is
     * resolved the listener will be called.
     *
     * @param pipeAdv The advertisement of the pipe being resolved.
     * @param listener The listener to be called when the pipe is resolved.
     * @throws IOException If the pipe cannot be created.
     */
    public void createOutputPipe(PipeAdvertisement pipeAdv, OutputPipeListener listener) throws IOException;

    /**
     * Attempt to create an OutputPipe using the specified Pipe Advertisement.
     * When the pipe is resolved to one of the peers in the set of peer ids
     * provided the listener will be called.
     *
     * @param pipeAdv The advertisement of the pipe being resolved.
     * @param resolvablePeers The set of peers on which the pipe may be resolved.
     * <strong>If the Set is empty then the pipe may be resolved to any 
     * destination peer.</strong>
     * @param listener the listener to be called when the pipe is resolved.
     * @throws IOException  If the pipe cannot be created.
     */
    public void createOutputPipe(PipeAdvertisement pipeAdv, Set<? extends ID> resolvablePeers, OutputPipeListener listener) throws IOException;

    /**
     *  Remove an OutputPipeListener previously registered with
     *  <code>createOuputputPipe</code>.
     *
     * @deprecated This method is being replaced by {@link #removeOutputPipeListener(ID,OutputPipeListener)}.
     *
     * @param pipeID The pipe who's listener is to be removed.
     * @param listener The listener to remove.
     * @return The listener which was removed or null if the key did not have a mapping.
     */
    @Deprecated
    public OutputPipeListener removeOutputPipeListener(String pipeID, OutputPipeListener listener);

    /**
     *  Remove an OutputPipeListener previously registered with
     *  <code>createOuputputPipe</code>.
     *
     * @param pipeID The pipe who's listener is to be removed.
     * @param listener The listener to remove.
     * @return The listener which was removed or null if the key did not have a mapping.
     */
    public OutputPipeListener removeOutputPipeListener(ID pipeID, OutputPipeListener listener);
}
