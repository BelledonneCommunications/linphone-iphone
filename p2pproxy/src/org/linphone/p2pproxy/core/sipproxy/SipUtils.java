/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

SipUtils.java - .

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
package org.linphone.p2pproxy.core.sipproxy;

import org.zoolu.sip.address.NameAddress;
import org.zoolu.sip.address.SipURL;
import org.zoolu.sip.header.Header;
import org.zoolu.sip.header.RecordRouteHeader;
import org.zoolu.sip.header.ViaHeader;
import org.zoolu.sip.message.Message;
import org.zoolu.sip.provider.SipProvider;

public class SipUtils {
   public static void addVia(SipProvider aProvider, Message aMessage) {
      ViaHeader via=new ViaHeader("udp",aProvider.getViaAddress(),aProvider.getPort());
      String branch=aProvider.pickBranch(aMessage);
      via.setBranch(branch);
      aMessage.addViaHeader(via);      
   }
   public static void addRecordRoute(SipProvider aProvider, Message aMessage) {
      SipURL lRecordRoute;
      lRecordRoute=new SipURL(aProvider.getViaAddress(),aProvider.getPort());
      lRecordRoute.addLr();
      RecordRouteHeader lRecordRouteHeader=new RecordRouteHeader(new NameAddress(lRecordRoute));
      aMessage.addRecordRouteHeader(lRecordRouteHeader);    
   }
   public static void removeVia(SipProvider aProvider, Message aMessage) {
      synchronized (aMessage) {
         ViaHeader lViaHeader =new ViaHeader((Header)aMessage.getVias().getHeaders().elementAt(0));
         if (lViaHeader.getHost().equals(aProvider.getViaAddress()) && lViaHeader.getPort() == aProvider.getPort() ) {
            aMessage.removeViaHeader();
         }       
      }
   }
}
