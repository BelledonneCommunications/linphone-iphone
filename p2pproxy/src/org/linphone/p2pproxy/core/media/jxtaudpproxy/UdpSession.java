package org.linphone.p2pproxy.core.media.jxtaudpproxy;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import org.apache.log4j.Logger;
import org.linphone.p2pproxy.core.GenericUdpSession;

import net.jxta.endpoint.ByteArrayMessageElement;
import net.jxta.endpoint.MessageElement;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;

public class UdpSession implements GenericUdpSession.MessageHandler ,PipeMsgListener{
   private final static Logger mLog = Logger.getLogger(UdpSession.class);   
   private  InetSocketAddress mRemoteAddress;
   private OutputPipe mRemotePipe;
   private boolean mExit = false;
   private final String mMessageName;
   private final GenericUdpSession mGenericUdpSession;
   UdpSession(int aPort,String aMessageName) throws SocketException, UnknownHostException {
      mMessageName = aMessageName;
      mGenericUdpSession = new GenericUdpSession(new InetSocketAddress(aPort),this);
   }



   public void pipeMsgEvent(PipeMsgEvent event) {
      MessageElement lMessageElement = event.getMessage().getMessageElement(mMessageName);
      if (lMessageElement == null) {
         //nop, this is not for me
         return;
      }
      //test if we have an address to forward to
      if (mRemoteAddress == null) {
         mLog.warn("no remote adress, message discarded");
         return;
      }
      byte[] lBuff = lMessageElement.getBytes(false);
      DatagramPacket lDatagramPacket = new DatagramPacket(lBuff,(int) lMessageElement.getByteLength());
      lDatagramPacket.setSocketAddress(mRemoteAddress);
      try {
         mGenericUdpSession.getSocket().send(lDatagramPacket);
         mLog.debug("message from ["+mMessageName+"] sent to ["+mRemoteAddress+"]");
      } catch (IOException e) {
         mLog.error("cannot send message for session ["+this+"]" , e);
      }
      //
   }
   public void setRemoteAddress( InetSocketAddress aRemoteAddress) {
      mRemoteAddress = aRemoteAddress;
   }
   public void setRemotePipe(OutputPipe aRemotePipe) {
      mRemotePipe = aRemotePipe;
   }
   public void close() {
      mExit = true;
      mGenericUdpSession.close();
   }

   public String toString() {
      return "name ["+mMessageName +"] udp dest ["+mRemoteAddress+"]";
   }
   public int getPort() {
      return mGenericUdpSession.getSocket().getPort();
   }
   public void onMessage(DatagramPacket message) {
      try {
         // if destination is known just send
         if (mRemotePipe != null) {
            net.jxta.endpoint.Message lMessage = new net.jxta.endpoint.Message();
            ByteArrayMessageElement lByteArrayMessageElement = new ByteArrayMessageElement(mMessageName, null,message.getData(),0,message.getLength(), null);
            lMessage.addMessageElement(mMessageName, lByteArrayMessageElement);
            //send the message
            mRemotePipe.send(lMessage);
            mLog.debug("message from ["+message.getAddress()+":"+message.getPort()+"]sent to ["+mRemotePipe.getPipeID()+"]");
         } else {
            mLog.warn("output pipe not set for ["+this+"], discarding message");
         }
      }catch(Exception e) {
         //nop
      }

   }
}
