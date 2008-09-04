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
import java.net.SocketAddress;

public class RouteAddResponse implements Serializable {
   private static final long serialVersionUID = 1L;
   private final SocketAddress mSocketAddress;
   private final MediaType mMediaType;
   
   public RouteAddResponse(SocketAddress aSocketAddress,MediaType aMediaType) {
      mSocketAddress = aSocketAddress;
      mMediaType =aMediaType;
   }
   public String toString() {
      return "sdp adress ["+mSocketAddress+"] type ["+mMediaType+"]";
   }
   /**
    * @return Returns the mMediaType.
    */
   public MediaType getMediaType() {
      return mMediaType;
   }
   /**
    * @return Returns the mSocketAddress.
    */
   public SocketAddress getSocketAddress() {
      return mSocketAddress;
   }
   
}
