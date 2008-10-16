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

import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.URI;
import java.net.UnknownHostException;

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
import org.linphone.p2pproxy.core.media.rtprelay.RtpRelayService;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;
import org.linphone.p2pproxy.core.stun.StunServer;

public class MediaResourceService implements ServiceProvider {
   private final static Logger mLog = Logger.getLogger(MediaResourceService.class);   
   private GenericUdpSession mUdpSessionForStunRtp;
   private  RtpRelayServer mRtpRelayServer;
   private  StunServer mSturServer;
   private RtpRelayServerConfig mConfig;
   private final JxtaNetworkManager mJxtaNetworkManager;
   private NetworkResourceAdvertisement mStunRtpServerAdvertisement;
   public final static String ADV_NAME = "p2p-proxy-stunrtp";
   public MediaResourceService(Configurator aConfigurator, JxtaNetworkManager aJxtaNetworkManager) throws SocketException, UnknownHostException {
      URI lAudioVideoPublicUri = URI.create(aConfigurator.getProperty(RtpRelayService.AUDIO_VIDEO_PUBLIC_URI,RtpRelayService.getDefaultAudioVideoPublicUri()));
      int lAudioVideoLocalPort = Integer.valueOf(aConfigurator.getProperty(RtpRelayService.AUDIO_VIDEO_LOCAL_PORT,String.valueOf(lAudioVideoPublicUri.getPort())));
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
      try {
         mStunRtpServerAdvertisement = (NetworkResourceAdvertisement) AdvertisementFactory.newAdvertisement(NetworkResourceAdvertisement.getAdvertisementType());
         mStunRtpServerAdvertisement.setAddress("udp://"+mConfig.getAudioVideoPublicSocketAddress().getAddress().getHostAddress()+":"+mConfig.getAudioVideoPublicSocketAddress().getPort());
         mStunRtpServerAdvertisement.setID(IDFactory.newCodatID(mJxtaNetworkManager.getPeerGroup().getPeerGroupID(), mStunRtpServerAdvertisement.getAddress().toString().getBytes()));
         mStunRtpServerAdvertisement.setName(ADV_NAME);
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().publish(mStunRtpServerAdvertisement,60000,30000);
         mLog.info(mStunRtpServerAdvertisement + "published");
      } catch (Exception e) {
         throw new P2pProxyException(e);
      }
   }

   public void stop() {
      try {
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().flushAdvertisement(mStunRtpServerAdvertisement);
         mUdpSessionForStunRtp.close();
      } catch (Exception e) {
         mLog.error("cannot stop MediaResourceService",e);
      }

   }

}
