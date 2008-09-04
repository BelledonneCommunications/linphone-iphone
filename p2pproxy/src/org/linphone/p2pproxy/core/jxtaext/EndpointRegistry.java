/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

EndpointRegistry.java - .

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
package org.linphone.p2pproxy.core.jxtaext;

import java.net.InetAddress;
import java.util.HashMap;
import java.util.Map;

public class EndpointRegistry {
   final static private EndpointRegistry mInstance = new EndpointRegistry();
   final private Map<String,InetAddress> mMap = new HashMap<String,InetAddress>();
   private EndpointRegistry() {
   }
   
   static public EndpointRegistry getInstance() {
      return mInstance;
   }
   /**
    * @param aKey peer Id as retyrned by EndpointAdress.getProtocolAddress()
    * @return
    */
   synchronized public boolean isPresent(String aKey) {
      return mMap.containsKey(aKey);
   }
   /**
    * @param aKey peer Id as retyrned by EndpointAdress.getProtocolAddress()
    * @return IP address or null 
    */
   synchronized public InetAddress get(String aKey) {
      return mMap.get(aKey);
   }
   /**
    * @param a Key peer Id as retyrned by EndpointAdress.getProtocolAddress()
    * @param aValue 
    */
   synchronized public void add(String aKey,InetAddress aValue) {
      mMap.put(aKey, aValue) ;
   }
   /**
    * @param aKey peer Id as retyrned by EndpointAdress.getProtocolAddress()
    */
   synchronized public void remove(String aKey) {
      mMap.remove(aKey) ;
   }
}
