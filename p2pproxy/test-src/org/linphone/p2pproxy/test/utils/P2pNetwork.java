/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

P2pNetwork.java -- create a jxta network.

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
package org.linphone.p2pproxy.test.utils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.InetAddress;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLClassLoader;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.InvalidPropertiesFormatException;
import java.util.List;

import javax.security.cert.CertificateException;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyInstance;
import org.linphone.p2pproxy.api.P2pProxyInstance.Mode;

import net.jxta.exception.JxtaException;

//import org.linphone.p2pproxy.Configurator;
//import org.linphone.p2pproxy.JxtaNetworkManager;
//import org.linphone.p2pproxy.P2pProxyException;


public class P2pNetwork {
	private List<P2pProxyInstance> mEdges = new ArrayList<P2pProxyInstance>();
    private List<P2pProxyInstance> mSuperPeers = new ArrayList<P2pProxyInstance>();
    private final static Logger mLog = Logger.getLogger(P2pNetwork.class);
    Process mSshProcess;
    //String mPrivateAdress = "192.168.145.1";
    String mPrivateAdress = "127.0.0.1";

    String mPublicAdress ;
    String mPublicAdressUser ;
   /**
	 * create a p2p network with super peers and edges
	 * @param nEdgees number of edge peer
	 * @param mRdv number of super peer
    * @throws IOException 
    * @throws FileNotFoundException 
    * @throws InvalidPropertiesFormatException 
    * @throws CertificateException 
    * @throws URISyntaxException 
    * @throws P2pProxyException 
    * @throws InterruptedException 
    * @throws JxtaException 
	 */
    public P2pNetwork(int nEdge,int mRdv) throws Exception {
       this(nEdge,mRdv,0,0,100,false,System.getProperty("user.name"),"127.0.0.1");
    }
    public P2pNetwork(int nEdge,int mRdv,int pAuto,int aBaseIndex,int aRelayCapacity, boolean isNated, String aUser,String aRemoteHost) throws Exception {
       mPublicAdress = aRemoteHost;
       mPublicAdressUser=aUser;
       try {

          if (isNated == true) {

             //1
             setupDummySocks(aBaseIndex,nEdge+mRdv+pAuto);
             //2

             System.setProperty("socksProxyHost", mPrivateAdress);
             System.setProperty("socksProxyPort", "1080");
          }
          String [] lClassPath = {
                "./antbuild/p2pproxy"
                ,"./dependencies/bcprov-jdk14.jar"
                ,"./dependencies/javax.servlet.jar"
                ,"./dependencies/org.mortbay.jetty.jar"
          };
          URL[] lUrlTab = new URL[lClassPath.length];
          for (int i=0;i<lClassPath.length;i++) {
             lUrlTab[i]=  new File(lClassPath[i]).toURL();
          }
          // build relays
          
          ClassLoader lRootClassLoader = Thread.currentThread().getContextClassLoader();
          // first relay
          ClassLoader lFirstClassLoader = new URLClassLoader(lUrlTab,lRootClassLoader);
          P2pProxyInstance lFirstP2pProxyInstance = (P2pProxyInstance) Class.forName("org.linphone.p2pproxy.core.P2pProxyInstanceImpl",true,lFirstClassLoader).newInstance();
          lFirstP2pProxyInstance.setIndex(aBaseIndex);
          lFirstP2pProxyInstance.setMode(Mode.seeding_server);
          if (isNated == true)  {
             lFirstP2pProxyInstance.setPrivateHostAdress(mPrivateAdress);
             lFirstP2pProxyInstance.setPublicHostAdress(mPublicAdress);
          }
          lFirstP2pProxyInstance.start();
          mSuperPeers.add(lFirstP2pProxyInstance);
          while (lFirstP2pProxyInstance.isStarted() == false) {
             Thread.sleep(1000);
          }
          for (int i=1;i<mRdv;i++) {
             ClassLoader lClassLoader = new URLClassLoader(lUrlTab,lRootClassLoader);
             P2pProxyInstance lP2pProxyInstance = (P2pProxyInstance) Class.forName("org.linphone.p2pproxy.core.P2pProxyInstanceImpl",true,lClassLoader).newInstance();
             lP2pProxyInstance.setIndex(aBaseIndex+i);
             lP2pProxyInstance.setMode(Mode.relay);
             lP2pProxyInstance.setRelayCapacity(aRelayCapacity);
             if (isNated == true)  {
                lP2pProxyInstance.setPrivateHostAdress(mPrivateAdress);
                lP2pProxyInstance.setPublicHostAdress(mPublicAdress);
             }
             mSuperPeers.add(lP2pProxyInstance);
             lP2pProxyInstance.start();
          }
          //edges
          for (int j=0;j<nEdge;j++) {
             ClassLoader lClassLoader = new URLClassLoader(lUrlTab,lRootClassLoader);
             P2pProxyInstance lP2pProxyInstance = (P2pProxyInstance) Class.forName("org.linphone.p2pproxy.core.P2pProxyInstanceImpl",true,lClassLoader).newInstance();
             lP2pProxyInstance.setIndex(aBaseIndex+mRdv+j);
             lP2pProxyInstance.setMode(Mode.edge);
             lP2pProxyInstance.setRelayCapacity(aRelayCapacity);
             if (isNated == true)  {
                lP2pProxyInstance.setPrivateHostAdress(mPrivateAdress);
                lP2pProxyInstance.setPublicHostAdress(mPublicAdress);
             }
             mEdges.add(lP2pProxyInstance);
             lP2pProxyInstance.start();
          }
          //auto
          for (int k=0;k<pAuto;k++) {
             ClassLoader lClassLoader = new URLClassLoader(lUrlTab,lRootClassLoader);
             P2pProxyInstance lP2pProxyInstance = (P2pProxyInstance) Class.forName("org.linphone.p2pproxy.core.P2pProxyInstanceImpl",true,lClassLoader).newInstance();
             lP2pProxyInstance.setIndex(aBaseIndex+mRdv+nEdge+k);
             lP2pProxyInstance.setMode(Mode.auto);
             lP2pProxyInstance.setRelayCapacity(aRelayCapacity);
             if (isNated == true)  {
                lP2pProxyInstance.setPrivateHostAdress(mPrivateAdress);
                lP2pProxyInstance.setPublicHostAdress(mPublicAdress);
             }
             mEdges.add(lP2pProxyInstance);
             lP2pProxyInstance.start();
          }
          
          boolean lstarted;
          // wait untill all instances are started
          do {
             lstarted = true;
             for (P2pProxyInstance lP2pProxyInstance : mSuperPeers) {
                if (lP2pProxyInstance.isStarted() == false) lstarted = false;
             } 
             for (P2pProxyInstance lP2pProxyInstance : new ArrayList<P2pProxyInstance>(mEdges)) {
                if (lP2pProxyInstance.isStarted() == false) {
                   lstarted = false;
                } else {
                   if (lP2pProxyInstance.getMode() == Mode.relay) {
                      // move from edge list to relay
                      mEdges.remove(lP2pProxyInstance);
                      mSuperPeers.add(lP2pProxyInstance);
                      mLog.info("peer [ "+lP2pProxyInstance+"] moved from edge to relay" );
                   }
                }
             }
             Thread.sleep(1000);
          }while (lstarted == false);
          mLog.info("P2pNetwork started with ["+getEdgesPeers().size()+"] egdges and ["+getSuperPeers().size()+"]super peers");
          
       } catch (Exception e) {
          mLog.fatal("cannot create network",e);
       }
       
    }
   /**
    * @return Returns the mEdges.
    */
   public List<P2pProxyInstance> getEdgesPeers() {
      return mEdges;
   }
   /**
    * @return Returns the mSuperPeers.
    */
   public List<P2pProxyInstance> getSuperPeers() {
      return mSuperPeers;
   }
   public List<P2pProxyInstance> getAllPeers() {
      List<P2pProxyInstance> lP2pProxyInstanceList = new ArrayList <P2pProxyInstance>();
      lP2pProxyInstanceList.addAll(mSuperPeers);
      lP2pProxyInstanceList.addAll(mEdges);
      return lP2pProxyInstanceList;
   }
   public void stop() {
      for (P2pProxyInstance lP2pProxyInstance:getAllPeers()) {
         try {
            lP2pProxyInstance.stop();
         } catch (Exception e) {
            mLog.error("stop error", e);
            //nop
         }
      }
      mSshProcess.destroy();
      
   }
   private void setupDummySocks(int aBaseIndex, int numberOfPeers) throws InterruptedException, IOException {
      StringBuffer lCommand =new StringBuffer("ssh -D "+mPrivateAdress+":1080 ");
      for (int i=aBaseIndex; i<aBaseIndex + numberOfPeers;i++) {
         lCommand.append("-R"+mPublicAdress+":"+(P2pProxyInstance.BASE_HTTP+i)+":"+mPrivateAdress+":"+(P2pProxyInstance.BASE_HTTP+i)+" ");
      }
      lCommand.append(mPublicAdressUser+"@"+mPublicAdress);
      mLog.info("executing ["+lCommand.toString()+"]");
      mSshProcess = Runtime.getRuntime().exec(lCommand.toString());
   }
	
}
