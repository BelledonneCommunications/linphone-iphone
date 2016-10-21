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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
package org.linphone.core;


import android.content.Context;

import org.linphone.tools.OpenH264DownloadHelper;

import java.util.ArrayList;

abstract public class LinphoneCoreFactory {

	protected Context fcontext;
	private static String factoryName = "org.linphone.core.LinphoneCoreFactoryImpl";


	static LinphoneCoreFactory theLinphoneCoreFactory;
	/**
	 * Indicate the name of the class used by this factory
	 * @param className
	 */
	public static void setFactoryClassName (String className) {
		factoryName = className;
	}


	public static final synchronized LinphoneCoreFactory instance() {
		try {
			if (theLinphoneCoreFactory == null) {
				Class<?> lFactoryClass = Class.forName(factoryName);
				theLinphoneCoreFactory = (LinphoneCoreFactory) lFactoryClass.newInstance();
			}
		} catch (Exception e) {
			System.err.println("Cannot instanciate factory ["+factoryName+"]");
		}
		return theLinphoneCoreFactory;
	}
	/**
	 * Create  {@link LinphoneAuthInfo}
	 * @param username
	 * @param password
	 * @param realm
	 * @param domain
	 */
	abstract public LinphoneAuthInfo createAuthInfo(String username,String password, String realm, String domain);
	/**
	 * create  {@link LinphoneAuthInfo}
	 * @param username
	 * @param userid user id as set in auth header
	 * @param passwd
	 * @param ha1
	 * @param realm
	 * */
	abstract public LinphoneAuthInfo createAuthInfo(String username, String userid, String passwd, String ha1, String realm, String domain);

	/**
	 * Create a LinphoneCore object. The LinphoneCore is the root for all liblinphone operations. You need only one per application.
	 * @param listener listener to receive notifications from the core
	 * @param userConfig path where to read/write configuration (optional)
	 * @param factoryConfig path where to read factory configuration (optional)
	 * @param userdata any kind of application specific data
	 * @param context an application context, on android this MUST be the android.content.Context object used by the application.
	 * @return a LinphoneCore object.
	 * @throws LinphoneCoreException
	 */
	abstract public LinphoneCore createLinphoneCore(LinphoneCoreListener listener, String userConfig,String factoryConfig,Object  userdata, Object context) throws LinphoneCoreException;
	/**
	 * Create a LinphoneCore object. The LinphoneCore is the root for all liblinphone operations. You need only one per application.
	 * @param listener listener to receive notifications from the core.
	 * @param context an application context, on android this MUST be the android.content.Context object used by the application.
	 * @return the LinphoneCore object.
	 * @throws LinphoneCoreException
	 */
	abstract public LinphoneCore createLinphoneCore(LinphoneCoreListener listener, Object context) throws LinphoneCoreException;


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
	 * @throws LinphoneCoreException if address cannot be parsed
	 */
	abstract public LinphoneAddress createLinphoneAddress(String address) throws LinphoneCoreException;
	abstract public LpConfig createLpConfig(String file);
	abstract public LpConfig createLpConfigFromString(String buffer);

	/**
	 * Enable verbose traces
	 * @param enable true to enable debug mode, false to disable it
	 * @param tag Tag which prefixes each log message.
	 */
	abstract public void setDebugMode(boolean enable, String tag);

	/**
	 * Enable the linphone core log collection to upload logs on a server.
	 */
	abstract public void enableLogCollection(boolean enable);

	/**
	 * Set the path where the log files will be written for log collection.
	 * @param path The path where the log files will be written.
	 */
	abstract public void setLogCollectionPath(String path);

	abstract public void setLogHandler(LinphoneLogHandler handler);

	/**
	 * Must be calling after the LinphoneCore creation
	 * @return a new OpenH264DownloadHelper
	 */
	abstract public OpenH264DownloadHelper createOpenH264DownloadHelper();

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

	/**
	 * Create a LinphoneContent object from string data.
	 */
	abstract public LinphoneContent createLinphoneContent(String type, String subType, String data);

	/**
	 * Create a LinphoneContent object from byte array.
	 */
	abstract public LinphoneContent createLinphoneContent(String type, String subType, byte[] data, String encoding);

	/**
	 * Create a PresenceActivity object.
	 */
	abstract public PresenceActivity createPresenceActivity(PresenceActivityType type, String description);

	/**
	 * Create a PresenceService object.
	 * @param id The id of the presence service. Can be null to generate it automatically.
	 * @param status The PresenceBasicStatus to set for the PresenceService object.
	 * @param contact The contact to set for the PresenceService object. Can be null.
	 * @return A new PresenceService object.
	 */
	abstract public PresenceService createPresenceService(String id, PresenceBasicStatus status, String contact);

	/**
	 * Create a PresenceModel object.
	 */
	abstract public PresenceModel createPresenceModel();
	abstract public PresenceModel createPresenceModel(PresenceActivityType type, String description);
	abstract public PresenceModel createPresenceModel(PresenceActivityType type, String description, String note, String lang);

	/*
	 * Create TunnelConfig object, used to configure voip anti blocking extension.
	 */
	abstract public TunnelConfig createTunnelConfig();

	/**
	 * Create LinphoneAccountCreator object
	 */
	abstract public LinphoneAccountCreator createAccountCreator(LinphoneCore lc, String url);

	/**
	 * Array of countries list
	 * @return countries list
     */
	abstract public DialPlan[] getAllDialPlan();
}
