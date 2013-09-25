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

import org.linphone.mediastream.CpuUtils;
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
		System.loadLibrary("neon");

		// FFMPEG (audio/video)
		loadOptionalLibrary("avutil-linphone");
		loadOptionalLibrary("swscale-linphone");
		loadOptionalLibrary("avcodec-linphone");

		// OPENSSL (cryptography)
		// lin prefix avoids collision with libs in /system/lib
		loadOptionalLibrary("lincrypto");
		loadOptionalLibrary("linssl");

		// Secure RTP and key negotiation
		loadOptionalLibrary("srtp");
		loadOptionalLibrary("zrtpcpp"); // GPLv3+

		// Tunnel
		loadOptionalLibrary("tunnelclient");
		
		// g729 A implementation
		loadOptionalLibrary("bcg729");

		//Main library
		System.loadLibrary("linphone");

		Version.dumpCapabilities();
	}
	@Override
	public LinphoneAuthInfo createAuthInfo(String username, String password,
			String realm) {
		return new LinphoneAuthInfoImpl(username,password,realm);
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
			String userConfig, String factoryConfig, Object userdata)
			throws LinphoneCoreException {
		try {
			return new LinphoneCoreImpl(listener,new File(userConfig),new File(factoryConfig),userdata);
		} catch (IOException e) {
			throw new LinphoneCoreException("Cannot create LinphoneCore",e);
		}
	}

	@Override
	public LinphoneCore createLinphoneCore(LinphoneCoreListener listener) throws LinphoneCoreException {
		try {
			return new LinphoneCoreImpl(listener);
		} catch (IOException e) {
			throw new LinphoneCoreException("Cannot create LinphoneCore",e);
		}
	}

	@Override
	public LinphoneProxyConfig createProxyConfig(String identity, String proxy,
			String route, boolean enableRegister) throws LinphoneCoreException {
		return new LinphoneProxyConfigImpl(identity,proxy,route,enableRegister);
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

	public static boolean hasNeonInCpuFeatures()
	{
		CpuUtils cpu = new CpuUtils();
		return cpu.isCpuNeon();
	}
	
	public static boolean isArmv7()
	{
		return System.getProperty("os.arch").contains("armv7");
	}

	@Override
	public LinphoneAuthInfo createAuthInfo(String username, String userid,
			String passwd, String ha1, String realm) {
		return new LinphoneAuthInfoImpl(username,userid,passwd,ha1,realm);
	}

	@Override
	public LinphoneContent createLinphoneContent(String type, String subType,
			String data) {
		return new LinphoneContentImpl(type,subType,data);
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
