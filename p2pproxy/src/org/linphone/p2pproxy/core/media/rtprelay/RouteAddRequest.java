/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

RouteAddRequest.java - .

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

public class RouteAddRequest implements Serializable {
   private final SocketAddress mSource;
   private final SocketAddress mDest;
   private final MediaType mMediaType;
   private static final long serialVersionUID = 1L;
   public RouteAddRequest(SocketAddress aSource, SocketAddress aDest,MediaType aMediaType) {
      mSource = aSource;
      mDest = aDest;
      mMediaType = aMediaType;
   }
   /**
    * @return Returns the mSource.
    */
   public SocketAddress getSource() {
      return mSource;
   }
   /**
    * @return Returns the mDest.
    */
   public SocketAddress getDest() {
      return mDest;
   }
   /**
    * @return Returns the mMediaType.
    */
   public MediaType getMediaType() {
      return mMediaType;
   }
   public String toString() {
      return "source ["+mSource+"] dest ["+mDest+"] type ["+mMediaType+"]";
   }
   
}
