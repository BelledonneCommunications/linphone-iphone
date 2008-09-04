package org.linphone.p2pproxy.core.media.rtprelay;

import java.net.InetSocketAddress;

/**
 */
public class RtpRelayServerConfig {
   private final InetSocketAddress mAudioVideoPublicSocketAddress;
   private final InetSocketAddress mVideovideoPrivateSocketAddress;

   public RtpRelayServerConfig(InetSocketAddress anAudioVideoPublicSocketAddress,InetSocketAddress anAudioVideoPrivateSocketAddress) {
      mAudioVideoPublicSocketAddress = anAudioVideoPublicSocketAddress;
      mVideovideoPrivateSocketAddress = anAudioVideoPrivateSocketAddress;
   }

   /**
    * @return Returns the mAudioPublicSocketAddress.
    */
   public InetSocketAddress getAudioVideoPublicSocketAddress() {
      return mAudioVideoPublicSocketAddress;
   }
   /**
    * @return Returns the mVideoPublicSocketAddress.
    */
   public InetSocketAddress getAudioVideoPrivateSocketAddress() {
      return mVideovideoPrivateSocketAddress;
   }
}