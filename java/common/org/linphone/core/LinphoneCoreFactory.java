/*
LinphoneCoreFactory.java
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





abstract public class LinphoneCoreFactory {
	
	private static String factoryName = "org.linphone.core.LinphoneCoreFactoryImpl";
	
	
	static LinphoneCoreFactory theLinphoneCoreFactory; 
	/**
	 * Indicate the name of the class used by this factory
	 * @param pathName
	 */
	public static void setFactoryClassName (String className) {
		factoryName = className;
	}
	
	public static LinphoneCoreFactory instance() {
		try {
		if (theLinphoneCoreFactory == null) {
			Class lFactoryClass = Class.forName(factoryName);
			theLinphoneCoreFactory = (LinphoneCoreFactory) lFactoryClass.newInstance();
		}
		} catch (Exception e) {
			System.err.println("cannot instanciate factory ["+factoryName+"]");
		}
		return theLinphoneCoreFactory;
	}
	abstract public LinphoneAuthInfo createAuthInfo(String username,String password, String realm);
	
	abstract public LinphoneCore createLinphoneCore(LinphoneCoreListener listener, String userConfig,String factoryConfig,Object  userdata) throws LinphoneCoreException;
	abstract public LinphoneCore createLinphoneCore(LinphoneCoreListener listener) throws LinphoneCoreException;

	/**
	 * Constructs a LinphoneAddress object
	 * @param username 
	 * @param domain
	 * @param displayName
	 * @return 
	 */
	abstract public LinphoneAddress createLinphoneAddress(String username,String domain,String displayName);
	/**
	 * Constructs a LinphoneAddress object by parsing the user supplied address, given as a string.
	 * @param address should be like sip:joe@sip.linphone.org
	 * @return
	 */
	abstract public LinphoneAddress createLinphoneAddress(String address);
	
	abstract public  LinphoneProxyConfig createProxyConfig(String identity, String proxy,String route,boolean enableRegister) throws LinphoneCoreException;
	/**
	 * Enable verbose traces
	 * @param enable 
	 */
	abstract public  void setDebugMode(boolean enable);
	
	abstract public void setLogHandler(LinphoneLogHandler handler);
	/**
	 * Create a LinphoneFriend, similar to {@link #createLinphoneFriend()} + {@link LinphoneFriend#setAddress(LinphoneAddress)} 
	 * @param friendUri a buddy address, must be a sip uri like sip:joe@sip.linphone.org
	 * @return a new LinphoneFriend with address initialized
	 */
	abstract public LinphoneFriend createLinphoneFriend(String friendUri);
	/**
	 * Create a new LinphoneFriend
	 * @return
	 */
	abstract public LinphoneFriend createLinphoneFriend();
	
	
}
