package org.linphone.p2pproxy.core.utils;
import java.security.MessageDigest;

import net.jxta.id.IDFactory;

/*
 * Created on Sep 14, 2007
 *
 * To change the template for this generated file go to
 * Window&gt;Preferences&gt;Java&gt;Code Generation&gt;Code and Comments
 */

public class PeerGroupIdGenerator {

   /**
    * @param args
    */
   public static void main(String[] args) {
     
         System.out.print(createInfrastructurePeerGroupID("p2p.linphone.org","p2pproxy"));
 
   }
   public static final net.jxta.peergroup.PeerGroupID createInfrastructurePeerGroupID(String clearTextID, String function){
      System.out.println("Creating peer group ID =  clearText:'"+clearTextID+"' , function:'"+function+"'");
      byte[] digest = generateHash(clearTextID, function);
      net.jxta.peergroup.PeerGroupID peerGroupID = IDFactory.newPeerGroupID(  digest );
      return peerGroupID;
  }
  /**
   * Generates an SHA-1 digest hash of the string: clearTextID+"-"+function or: clearTextID if function was blank.<p>
   *
   * Note that the SHA-1 used only creates a 20 byte hash.<p>
   *
   * @param clearTextID A string that is to be hashed. This can be any string used for hashing or hiding data.
   * @param function A function related to the clearTextID string. This is used to create a hash associated with clearTextID so that it is a uique code.
   *
   * @return array of bytes containing the hash of the string: clearTextID+"-"+function or clearTextID if function was blank. Can return null if SHA-1 does not exist on platform.
   */
  public static final byte[] generateHash(String clearTextID, String function) {
      String id;
      String functionSeperator = "-";      
      if (function == null) {
          id = clearTextID;
      } else {
          ;
         id = clearTextID + functionSeperator + function;
      }
      byte[] buffer = id.getBytes();
      
      MessageDigest algorithm = null;
      
      try {
          algorithm = MessageDigest.getInstance("MD5");
      } catch (Exception e) {
          System.out.println("Cannot load selected Digest Hash implementation");
          e.printStackTrace();
          return null;
      }
      
      
      // Generate the digest.
      algorithm.reset();
      algorithm.update(buffer);
      
      try{
          byte[] digest1 = algorithm.digest();
          return digest1;
      }catch(Exception de){
          System.out.println("Failed to creat a digest.");
          de.printStackTrace();
          return null;
      }
  }
}
