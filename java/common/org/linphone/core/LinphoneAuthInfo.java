/*
LinphoneAuthInfo.java
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
 * Object holding authentication information.
 * In most case, authentication information consists of a username and password. Sometimes, a userid is required by proxy, and realm can be useful to discriminate different SIP domains.
 *<br>This object is instantiated using either {@link LinphoneCoreFactory#createAuthInfo(String, String, String)} or {@link LinphoneCoreFactory#createAuthInfo(String, String, String, String, String)}.
 *<br>
 *Once created and filled, a LinphoneAuthInfo must be added to the LinphoneCore in order to become known and used automatically when needed. 
 *Use {@link LinphoneCore#addAuthInfo(LinphoneAuthInfo)} for that purpose.
 *<br>
 *The LinphoneCore object can take the initiative to request authentication information when needed to the application 
 *through the {@link LinphoneCoreListener#authInfoRequested(LinphoneCore, String, String)} listener.
 *<br>
 *The application can respond to this information request later using  {@link LinphoneCore#addAuthInfo(LinphoneAuthInfo)}. 
 *This will unblock all pending authentication transactions and retry them with authentication headers.
 *
 */
public interface LinphoneAuthInfo {
	/**
	 * get user name
	 * @return username
	 */
	String getUsername();
	/**
	 * Sets the username.
	 * @param username
	 */
	void setUsername(String username);
	/**
	 * get password
	 * @return password
	 */
	String getPassword();
	/**
	 * sets password
	 * @param password
	 */
	void setPassword(String password);
	/**
	 * get realm
	 * @return
	 */
	String getRealm();
	/**
	 * set realm
	 * @param realm
	 */
	void setRealm(String realm);
	/**
	 * get auth userid has used in authentication header. If null, username is taken for authentication
	 * @return auth userid
	 */
	String getUserId();
	/**
	 * set auth userid has used in authentication header. If null, username is taken for authentication
	 * 
	 */
	void setUserId(String userid);
	/**
	 * get ha1
	 * @return ha1
	 */
	String getHa1();
	/**
	 * set ha1
	 */
	void setHa1(String ha1);
	
	/**
	 * Sets the domain
	 * @param domain
	 */
	void setDomain(String domain);
	
	/**
	 * Gets the domain
	 * @return the domain
	 */
	String getDomain();
	
	/**
	 * Clones a current auth info
	 * @return the clone auth info
	 */
	LinphoneAuthInfo clone();
}


