/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

RouteAddResponse.java - .

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

import java.io.Serializable;
import java.net.InetSocketAddress;
import java.util.Map;

public class AddressResponse implements Serializable {
   private static final long serialVersionUID = 1L;
   private final Map<MediaType,InetSocketAddress> mAddressTable;
   public AddressResponse(Map<MediaType,InetSocketAddress> anAddressTable) {
      mAddressTable = anAddressTable;
   }
   /**
    * @return Returns the mMediaType.
    */
   public Map<MediaType,InetSocketAddress> getAddressTable() {
      return mAddressTable;
   }
   
   public String toString() {
      StringBuffer lReturnString = new StringBuffer();
      for (MediaType lMediaType:mAddressTable.keySet()) {
         lReturnString.append(mAddressTable.get(lMediaType)+" for type "+lMediaType+" :");
      }
      return lReturnString.toString();
   }
   
}
