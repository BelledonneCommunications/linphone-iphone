/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

UdpRelayService.java - .

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
package org.linphone.p2pproxy.core.media.rtprelay;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.Configurator;
import org.linphone.p2pproxy.core.GenericService;
import org.linphone.p2pproxy.core.GenericUdpSession;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.ServiceProvider;


public class RtpRelayService  implements ServiceProvider{
   private final static Logger mLog = Logger.getLogger(RtpRelayService.class); 
   private final RtpRelayServer mRtpRelayServer;
   public final static String AUDIO_VIDEO_LOCAL_PORT="org.linphone.p2pproxy.udp-media-relay.audio-video.port";
   public final static String AUDIO_VIDEO_PUBLIC_URI="org.linphone.p2pproxy.udp-media-relay.audio-video.public-uri";
   private final static String SRV_NAME = "RTPRELAY";
   public final static String ADV_NAME = "JXTASPEC:LINPHONE-"+SRV_NAME;
   // 
   private final RtpRelayServerConfig mConfig;
   private final GenericService mGenericService;
   private final GenericUdpSession mGenericUdpSession;
   private class SocketHandlerFactory implements GenericService.ServiceSocketHandlerFactory {

      public Runnable create(final Socket socket) {
         return new Runnable() {
            public void run() {
               try {
                  ObjectOutputStream lOut = new ObjectOutputStream(socket.getOutputStream());
                  //work around to unlock the socket
                  lOut.writeBoolean(true);
                  lOut.flush();
                  ObjectInputStream lIn = new ObjectInputStream(socket.getInputStream());
                  Object lInputObj;
                  boolean lStop = false;
                  while (lStop == false) {
                     lInputObj = lIn.readObject();
                     mLog.info("request message ["+lInputObj+"] received");
                     if (lInputObj instanceof AddressRequest) {
                    	Map<MediaType,InetSocketAddress> lList = new HashMap<MediaType,InetSocketAddress>();
                    	lList.put(MediaType.audio, mConfig.getAudioVideoPublicSocketAddress());
                    	lList.put(MediaType.video, mConfig.getAudioVideoPublicSocketAddress());
                    	AddressResponse lAddressResponse = new AddressResponse(lList);
                        lOut.writeObject(lAddressResponse);
                        lOut.flush();
                     } else {
                        mLog.error("unknown request ["+lInputObj+"]");
                     }
                     lStop = true;
                  }
               } catch (Exception e) {
                  mLog.error("socket instance error",e);
               }
               finally {
                  try {
                     socket.close();
                  } catch (IOException e) {
                     mLog.error("cannot close socket ",e);
                  }
               }
            }
         };
      }
   }



   /**
    * @param properties mandatory parameter are:
    * {@link AUDIO_LOCAL_PORT} ,{@link AUDIO_PUBLIC_URI} , {@link VIDEO_LOCAL_PORT}, {@link VIDEO_PUBLIC_URI}
    * @param jxtaNetworkManager
    * @param serviceName
    * @param serviceSocketHandlerFactory
    * @throws SocketException
    * @throws UnknownHostException
    */
   public RtpRelayService(RtpRelayServerConfig aConfig,Configurator properties, JxtaNetworkManager jxtaNetworkManager) throws SocketException, UnknownHostException {

      mGenericService = new GenericService(properties, jxtaNetworkManager,SRV_NAME ,new SocketHandlerFactory());
      mConfig = aConfig;
      mGenericUdpSession = new GenericUdpSession(mConfig.getAudioVideoPrivateSocketAddress());
      mRtpRelayServer = new RtpRelayServer(mGenericUdpSession.getSocket());
      mGenericUdpSession.addMessageHandler(mRtpRelayServer);
      mLog.info("UdpRelayService created "+this);
   }


   public String toString() {
      return mRtpRelayServer +" public address ["+mConfig.getAudioVideoPublicSocketAddress()+"] private address address ["+mConfig.getAudioVideoPrivateSocketAddress()+"]";
   }

   public void start(long timeOut) throws P2pProxyException {
      mGenericService.start(timeOut);
      
   }

   public void stop() {
	   mGenericUdpSession.close();
      mGenericService.stop();
      
   }
   public static String getDefaultAudioVideoPublicUri() throws UnknownHostException {
      return "udp://"+InetAddress.getLocalHost().getHostAddress()+":16000";
   }

}
