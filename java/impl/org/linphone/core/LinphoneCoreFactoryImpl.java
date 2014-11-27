/*
LinphoneCoreFactoryImpl.java
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

import java.io.File;
import java.io.IOException;

import org.linphone.mediastream.MediastreamerAndroidContext;
import org.linphone.mediastream.Version;

import android.util.Log;

public class LinphoneCoreFactoryImpl extends LinphoneCoreFactory {

	private static boolean loadOptionalLibrary(String s) {
		try {
			System.loadLibrary(s);
			return true;
		} catch (Throwable e) {
			Log.w("Unable to load optional library lib", s);
		}
		return false;
	}

	static {
		String eabi = "armeabi";
		if (Version.isX86()) {
			eabi = "x86";
		} else if (Version.isArmv7()) {
			eabi = "armeabi-v7a";
		}

		// FFMPEG (audio/video)
		if (Version.isX86()) {
			loadOptionalLibrary("ffmpeg-linphone-x86");
		} else if (Version.isArmv7()) {
			loadOptionalLibrary("ffmpeg-linphone-arm");
		}

		//Main library
		System.loadLibrary("linphone-" + eabi);

		Version.dumpCapabilities();
	}
	@Override
	public LinphoneAuthInfo createAuthInfo(String username, String password,
			String realm, String domain) {
		return new LinphoneAuthInfoImpl(username, password, realm, domain);
	}

	@Override
	public LinphoneAddress createLinphoneAddress(String username,
			String domain, String displayName) {
		return new LinphoneAddressImpl(username,domain,displayName);
	}

	@Override
	public LinphoneAddress createLinphoneAddress(String identity) throws LinphoneCoreException {
		return new LinphoneAddressImpl(identity);
	}
	
	@Override
	public LpConfig createLpConfig(String file) {
		return new LpConfigImpl(file);
	}

	@Override
	public LinphoneCore createLinphoneCore(LinphoneCoreListener listener,
			String userConfig, String factoryConfig, Object userdata, Object context)
			throws LinphoneCoreException {
		try {
			MediastreamerAndroidContext.setContext(context);
			File user = userConfig == null ? null : new File(userConfig);
			File factory = factoryConfig == null ? null : new File(factoryConfig);
			return new LinphoneCoreImpl(listener, user, factory, userdata);
		} catch (IOException e) {
			throw new LinphoneCoreException("Cannot create LinphoneCore",e);
		}
	}

	@Override
	public LinphoneCore createLinphoneCore(LinphoneCoreListener listener, Object context) throws LinphoneCoreException {
		try {
			MediastreamerAndroidContext.setContext(context);
			LinphoneCore lc = new LinphoneCoreImpl(listener);
			if(context!=null) lc.setContext(context);
			return lc;
		} catch (IOException e) {
			throw new LinphoneCoreException("Cannot create LinphoneCore",e);
		}
	}

	@Override
	public native void setDebugMode(boolean enable, String tag);

	
	private native void _setLogHandler(Object handler);
	@Override
	public void setLogHandler(LinphoneLogHandler handler) {
		_setLogHandler(handler);
	}

	@Override
	public LinphoneFriend createLinphoneFriend(String friendUri) {
		return new LinphoneFriendImpl(friendUri);
	}

	@Override
	public LinphoneFriend createLinphoneFriend() {
		return createLinphoneFriend(null);
	}

	public static boolean isArmv7()
	{
		return System.getProperty("os.arch").contains("armv7");
	}

	@Override
	public LinphoneAuthInfo createAuthInfo(String username, String userid,
			String passwd, String ha1, String realm, String domain) {
		return new LinphoneAuthInfoImpl(username, userid, passwd, ha1, realm, domain);
	}

	@Override
	public LinphoneContent createLinphoneContent(String type, String subType,
			byte [] data, String encoding) {
		return new LinphoneContentImpl(type,subType,data,encoding);
	}
	
	@Override
	public LinphoneContent createLinphoneContent(String type, String subType,
			String data) {
		return new LinphoneContentImpl(type,subType,data.getBytes(),null);
	}

	@Override
	public PresenceActivity createPresenceActivity(PresenceActivityType type, String description) {
		return new PresenceActivityImpl(type, description);
	}

	@Override
	public PresenceService createPresenceService(String id, PresenceBasicStatus status, String contact) {
		return new PresenceServiceImpl(id, status, contact);
	}

	@Override
	public PresenceModel createPresenceModel() {
		return new PresenceModelImpl();
	}

	@Override
	public PresenceModel createPresenceModel(PresenceActivityType type, String description) {
		return new PresenceModelImpl(type, description);
	}

	@Override
	public PresenceModel createPresenceModel(PresenceActivityType type, String description, String note, String lang) {
		return new PresenceModelImpl(type, description, note, lang);
	}

}
