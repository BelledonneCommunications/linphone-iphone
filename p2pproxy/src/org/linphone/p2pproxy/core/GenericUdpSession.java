/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

GenericUdpSession.java - .

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
package org.linphone.p2pproxy.core;


import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;

import java.net.SocketException;
import java.net.UnknownHostException;



import org.apache.log4j.Logger;
import org.linphone.p2pproxy.core.media.jxtaudpproxy.UdpSession;

public class GenericUdpSession implements Runnable {
   public interface MessageHandler {
      public  void onMessage(DatagramPacket lMessage);
   }
   private final static Logger mLog = Logger.getLogger(UdpSession.class);   
   private  final DatagramSocket mLocalSocket;
   private final Thread mLocalSocketThread;

   private final MessageHandler mMessageHandler;
   private boolean mExit = false;
   public GenericUdpSession(InetSocketAddress aSocketAddress,MessageHandler aMessageHandler) throws SocketException, UnknownHostException {
      mMessageHandler =  aMessageHandler;
      mLocalSocket = new DatagramSocket(aSocketAddress);
      mLocalSocketThread = new Thread(this,"udp session rtp ["+aSocketAddress+"]");
      mLocalSocketThread.start();
   }
   public void run() {

       while (mExit != true) {
           try {
               byte[] lBuff = new byte[1500];
               DatagramPacket lDatagramPacket = new DatagramPacket(lBuff,lBuff.length);
               mLocalSocket.receive(lDatagramPacket);
               // if destination is known just send
               mMessageHandler.onMessage(lDatagramPacket);
 
           }catch(Exception e) {
               //nop
           }
       }
       mLog.info("exit from thread ["+mLocalSocketThread+"]");

   }

   public void close() {
       mExit = true;
       mLocalSocket.close();
    }

   public DatagramSocket getSocket() {
      return mLocalSocket;
   }


}
