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
import java.util.List;

import org.linphone.mediastream.MediastreamerAndroidContext;
import org.linphone.mediastream.Version;

import android.util.Log;

public class LinphoneCoreFactoryImpl extends LinphoneCoreFactory {

	private static boolean loadOptionalLibrary(String s) {
		try {
			System.loadLibrary(s);
			return true;
		} catch (Throwable e) {
			Log.w("LinphoneCoreFactoryImpl", "Unable to load optional library lib" + s);
		}
		return false;
	}

	static {
		List<String> cpuabis=Version.getCpuAbis();
		String ffmpegAbi;
		boolean libLoaded=false;
		Throwable firstException=null;
		for (String abi : cpuabis){
			Log.i("LinphoneCoreFactoryImpl","Trying to load liblinphone for " + abi);
			loadOptionalLibrary("ffmpeg-linphone-" + abi);
			//Main library
			try {
				System.loadLibrary("bctoolbox-" + abi);
				System.loadLibrary("ortp-" + abi);
				System.loadLibrary("mediastreamer_base-" + abi);
				System.loadLibrary("mediastreamer_voip-" + abi);
				System.loadLibrary("linphone-" + abi);
				Log.i("LinphoneCoreFactoryImpl","Loading done with " + abi);
				libLoaded=true;
				break;
			}catch(Throwable e) {
				if (firstException == null) firstException=e;
			}
		}
		
		if (!libLoaded){
			throw new RuntimeException(firstException);
			
		}else{
			Version.dumpCapabilities();
		}
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
			LinphoneCore lc = new LinphoneCoreImpl(listener, user, factory, userdata);
			if(context!=null) lc.setContext(context);
			return lc;
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
	
	@Override
	public native void enableLogCollection(boolean enable);

	@Override
	public native void setLogCollectionPath(String path);

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
		return new LinphoneContentImpl(type,subType,data == null ? null : data.getBytes(), null);
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

	private native Object _createTunnelConfig();
	@Override
	public TunnelConfig createTunnelConfig() {
		return (TunnelConfig)_createTunnelConfig();
	}
}
