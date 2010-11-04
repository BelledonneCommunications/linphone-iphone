/*
LinphoneAddress.java
Copyright (C) 2010  Belledonne Communications, Grenoble, France

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
package org.linphone.core;
/**
 * Object that represents a SIP address.
 * The LinphoneAddress is an opaque object to represents SIP addresses, ie the content of SIP's 'from' and 'to' headers. 
 * A SIP address is made of display name, username, domain name, port, and various uri headers (such as tags). 
 * It looks like 'Alice <sip:alice@example.net>'. The LinphoneAddress has methods to extract and manipulate all parts of the address. 
 * When some part of the address (for example the username) is empty, the accessor methods return null.
 * <br> Can be instanciated using both  {@link LinphoneCoreFactory#createLinphoneAddress(String, String, String)} or {@link LinphoneCoreFactory#createLinphoneAddress(String)} 
 * @author jehanmonnier
 *
 */
public interface LinphoneAddress {
	/**
	 * Human display name
	 * @return null if not set
	 */
	public String getDisplayName();
	/**
	 * userinfo 
	 * @return null if not set
	 */
	public String getUserName();
	/**
	 * 
	 * @return null if not set
	 */
	public String getDomain();
	public String getPort();
	public int getPortInt();
	/**
	 * set display name 
	 * @param name
	 */
	public void setDisplayName(String name);
	public void setUserName(String username);
	public void setDomain(String domain);
	public void setPort(String port);
	public void setPortInt(int port);
	public void clean();
	
	/**
	 * 
	 * @return the address as a string.
	 */
	public String asString();
	/**
	 * 
	 * @return the address without display name as a string.
	 */
	public String asStringUriOnly();
	
	/**
	 * same as {@link #asString()}
	 * 
	 * */
	public String toString();
}
