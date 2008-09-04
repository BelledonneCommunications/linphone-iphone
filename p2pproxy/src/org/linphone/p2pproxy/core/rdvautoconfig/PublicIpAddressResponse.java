/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

PublicIpAddressResponse.java - .

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
package org.linphone.p2pproxy.core.rdvautoconfig;

import java.io.Serializable;
import java.net.InetAddress;

public class PublicIpAddressResponse implements Serializable {
   
   /**
    * 
    */
   private static final long serialVersionUID = 1L;
   final private InetAddress mPublicAddress;
   final private boolean mIsError;
   final private String mErrorText;
   public PublicIpAddressResponse(InetAddress aPublicAdress) {
      mPublicAddress = aPublicAdress;
      mIsError = false;
      mErrorText ="";
   }
   public PublicIpAddressResponse(String anErroText) {
      mPublicAddress = null;
      mIsError = true;
      mErrorText =anErroText;
      
   }
   public InetAddress getPublicAddress() {
      return mPublicAddress;
   }
   public boolean  isError() {
      return mIsError;
   }
   public String  getErrorText() {
      return mErrorText;
   }
   public String toString() {
      return  "PublicIpAddressResponse  ip ["+mPublicAddress+"] error ["+mErrorText+"]";
   }   
}
