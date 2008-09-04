/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

UdpSession.java 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.p2pproxy.core.media.jxtaudpproxy;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.socket.JxtaSocket;


public class RtpSessionImpl implements PipeMsgListener, RtpSession{
	UdpSession mRtpChannel = null;
	UdpSession mRtcpChannel = null;
	int mLocalRtpPort;

	public RtpSessionImpl(int portStart, String aDomaineName) throws IOException {
		mLocalRtpPort = portStart;
		while (mRtpChannel == null) {
			try {
				mRtpChannel = new UdpSession(mLocalRtpPort ,aDomaineName+"-RTP");
				mRtcpChannel = new UdpSession(mLocalRtpPort +1 ,aDomaineName+"-RCTP");
			}catch (Exception e) {
				//nop already used
				mLocalRtpPort += 2;
			}

		}
	}
	/* (non-Javadoc)
	 * @see org.linphone.p2pproxy.core.jxtaudpproxy.RtpSession#setRemoteAddress(java.net.InetSocketAddress)
	 */
	public void setRemoteAddress( InetSocketAddress aRemoteAddress) {
		mRtpChannel.setRemoteAddress(aRemoteAddress);
		InetSocketAddress lRtcpInetSocketAddress = new InetSocketAddress(aRemoteAddress.getAddress(),aRemoteAddress.getPort() + 1);
		mRtcpChannel.setRemoteAddress(lRtcpInetSocketAddress);
	}
	public void setRemotePipe(OutputPipe aRemotePipe) {
		mRtpChannel.setRemotePipe(aRemotePipe);
		mRtcpChannel.setRemotePipe(aRemotePipe);
	}
	public void close() {
		mRtpChannel.close();
		mRtcpChannel.close();
	}
	public int getPort() {
		return mRtpChannel.getPort();
	}
	public String toString() {
		return "rtp ["+mRtpChannel+"] rtcp "+mRtcpChannel+"]";
	}
	public void pipeMsgEvent(PipeMsgEvent event) {
		mRtpChannel.pipeMsgEvent(event);
		mRtcpChannel.pipeMsgEvent(event);
	}
}
