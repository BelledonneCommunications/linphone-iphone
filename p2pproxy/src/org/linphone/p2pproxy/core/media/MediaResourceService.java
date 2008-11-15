/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

MediaResourceService.java - .

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
package org.linphone.p2pproxy.core.media;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.URI;
import java.net.UnknownHostException;
import java.util.Timer;
import java.util.TimerTask;

import net.jxta.document.AdvertisementFactory;
import net.jxta.id.IDFactory;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.Configurator;
import org.linphone.p2pproxy.core.GenericUdpSession;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.ServiceProvider;
import org.linphone.p2pproxy.core.media.rtprelay.RtpRelayServer;
import org.linphone.p2pproxy.core.media.rtprelay.RtpRelayServerConfig;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;
import org.linphone.p2pproxy.core.stun.StunServer;

public class MediaResourceService implements ServiceProvider {
   private final static Logger mLog = Logger.getLogger(MediaResourceService.class);   
   public final static String AUDIO_VIDEO_LOCAL_PORT="org.linphone.p2pproxy.udp-media-relay.audio-video.port";
   public final static int AUDIO_VIDEO_LOCAL_PORT_DEFAULT_VALUE=16000;
   public final static String AUDIO_VIDEO_PUBLIC_URI="org.linphone.p2pproxy.udp-media-relay.audio-video.public-uri";

   private GenericUdpSession mUdpSessionForStunRtp;
   private  RtpRelayServer mRtpRelayServer;
   private  StunServer mSturServer;
   private RtpRelayServerConfig mConfig;
   private final JxtaNetworkManager mJxtaNetworkManager;
   private NetworkResourceAdvertisement mStunRtpServerAdvertisement;
   Timer mPublishTimer = new Timer("MediaResourceService publish timer");
   public final static String ADV_NAME = "p2p-proxy-stunrtp";
   private final int ADV_LIFE_TIME=6000000;
   TimerTask mPublishTask;
   public MediaResourceService(Configurator aConfigurator, JxtaNetworkManager aJxtaNetworkManager) throws SocketException, UnknownHostException {
	  
	  int lAudioVideoLocalPort = Integer.valueOf(aConfigurator.getProperty(MediaResourceService.AUDIO_VIDEO_LOCAL_PORT,String.valueOf(AUDIO_VIDEO_LOCAL_PORT_DEFAULT_VALUE)));
      URI lAudioVideoPublicUri = URI.create(aConfigurator.getProperty(MediaResourceService.AUDIO_VIDEO_PUBLIC_URI,"udp://"+InetAddress.getLocalHost().getHostAddress()+":"+lAudioVideoLocalPort));
      mConfig = new RtpRelayServerConfig(new InetSocketAddress(lAudioVideoPublicUri.getHost(),lAudioVideoPublicUri.getPort())
                                          ,new InetSocketAddress(lAudioVideoLocalPort));
      mUdpSessionForStunRtp = new GenericUdpSession(new InetSocketAddress(lAudioVideoLocalPort));
      mRtpRelayServer =  new RtpRelayServer(mUdpSessionForStunRtp.getSocket(),1000,1000);
      mUdpSessionForStunRtp.addMessageHandler(mRtpRelayServer);
      mSturServer = new StunServer(mUdpSessionForStunRtp.getSocket());
      mUdpSessionForStunRtp.addMessageHandler(mSturServer);
      mJxtaNetworkManager = aJxtaNetworkManager;
   }

   public void start(long timeOut) throws P2pProxyException {
	   mPublishTask = new TimerTask() {

		   @Override
		   public void run() {
			   try {
				   mStunRtpServerAdvertisement = (NetworkResourceAdvertisement) AdvertisementFactory.newAdvertisement(NetworkResourceAdvertisement.getAdvertisementType());
				   mStunRtpServerAdvertisement.setAddress("udp://"+mConfig.getAudioVideoPublicSocketAddress().getAddress().getHostAddress()+":"+mConfig.getAudioVideoPublicSocketAddress().getPort());
				   mStunRtpServerAdvertisement.setID(IDFactory.newCodatID(mJxtaNetworkManager.getPeerGroup().getPeerGroupID(), Integer.toHexString(mStunRtpServerAdvertisement.getAddress().hashCode()).getBytes("US-ASCII")));
				   mStunRtpServerAdvertisement.setName(ADV_NAME);
				   mJxtaNetworkManager.getPeerGroup().getDiscoveryService().publish(mStunRtpServerAdvertisement,ADV_LIFE_TIME,ADV_LIFE_TIME/2);
				   mLog.info(mStunRtpServerAdvertisement + "published");
				  
			   } catch (Exception e) {
				   mLog.error("Cannot publish StunRtpServerAdvertisement", e);
			   }

		   }

	   };
	   mPublishTimer.scheduleAtFixedRate(mPublishTask, 0, ADV_LIFE_TIME - ADV_LIFE_TIME/10);
   }

   public void stop() {
      try {
    	  mPublishTask.cancel();
    	  mJxtaNetworkManager.getPeerGroup().getDiscoveryService().flushAdvertisement(mStunRtpServerAdvertisement);
         mUdpSessionForStunRtp.close();
      } catch (Exception e) {
         mLog.error("cannot stop MediaResourceService",e);
      }

   }

}
