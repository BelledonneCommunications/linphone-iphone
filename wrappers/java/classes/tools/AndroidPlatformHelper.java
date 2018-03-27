/*
AndroidPlatformHelper.java
Copyright (C) 2017  Belledonne Communications, Grenoble, France

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


package org.linphone.core.tools;

import org.linphone.core.Core;
import org.linphone.core.Factory;
import org.linphone.mediastream.Log;
import org.linphone.mediastream.MediastreamerAndroidContext;
import org.linphone.mediastream.Version;

import android.content.res.Resources;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.net.wifi.WifiManager.WifiLock;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkInfo;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.Build;

import java.net.InetAddress;
import java.util.List;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * This class is instanciated directly by the linphone library in order to access specific features only accessible in java.
 **/

public class AndroidPlatformHelper {
	private Context mContext;
	private WifiManager.WifiLock mWifiLock;
	private WifiManager.MulticastLock mMcastLock;
	private ConnectivityManager mConnectivityManager;
	private PowerManager mPowerManager;
	private WakeLock mWakeLock;
	private Resources mResources;
	private String mLinphoneRootCaFile;
	private String mRingSoundFile;
	private String mRingbackSoundFile;
	private String mPauseSoundFile;
	private String mErrorToneFile;
	private String mGrammarCpimFile;
	private String mGrammarVcardFile ;
	private String mUserCertificatePath;

	public AndroidPlatformHelper(Object ctx_obj) {
		mContext = (Context) ctx_obj;
		mResources = mContext.getResources();
		MediastreamerAndroidContext.setContext(mContext);

		WifiManager wifiMgr = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
		mPowerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
		mConnectivityManager = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);

		mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,	"AndroidPlatformHelper");
		mWakeLock.setReferenceCounted(true);
		mMcastLock = wifiMgr.createMulticastLock("AndroidPlatformHelper");
		mMcastLock.setReferenceCounted(true);
		mWifiLock = wifiMgr.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "AndroidPlatformHelper");
		mWifiLock.setReferenceCounted(true);

		String basePath = mContext.getFilesDir().getAbsolutePath();
		mLinphoneRootCaFile = basePath + "/rootca.pem";
		mRingSoundFile = basePath + "/ringtone.mkv";
		mRingbackSoundFile = basePath + "/ringback.wav";
		mPauseSoundFile = basePath + "/hold.mkv";
		mErrorToneFile = basePath + "/error.wav";
		mGrammarCpimFile = basePath + "/cpim_grammar";
		mGrammarVcardFile = basePath + "/vcard_grammar";
		mUserCertificatePath = basePath;

		try {
			copyAssetsFromPackage();
		} catch (Exception e) {
			Log.e(e, "AndroidPlatformHelper: Cannot copy assets from package.");
		}
	}

	public void initCore(long ptrLc) {
		Core lc = Factory.instance().getCore(ptrLc);
		if (lc == null) return;
		lc.setRingback(mRingbackSoundFile);
		lc.setRootCa(mLinphoneRootCaFile);
		lc.setPlayFile(mPauseSoundFile);
		lc.setUserCertificatesPath(mUserCertificatePath);
	}

	public Object getPowerManager() {
		return mPowerManager;
	}

	public String[] getDnsServers() {
		if (mConnectivityManager == null || Build.VERSION.SDK_INT < Version.API23_MARSHMALLOW_60)
			return null;

		if (mConnectivityManager.getActiveNetwork() == null
				|| mConnectivityManager.getLinkProperties(mConnectivityManager.getActiveNetwork()) == null)
			return null;

		int i = 0;
		List<InetAddress> inetServers = null;
		inetServers = mConnectivityManager.getLinkProperties(mConnectivityManager.getActiveNetwork()).getDnsServers();

		String[] servers = new String[inetServers.size()];

		for (InetAddress address : inetServers) {
			servers[i++] = address.getHostAddress();
		}
		Log.i("getDnsServers() returning");
		return servers;
	}

	public String getDataPath() {
		return mContext.getFilesDir().getAbsolutePath();
	}

	public String getConfigPath() {
		return mContext.getFilesDir().getAbsolutePath();
	}

	public String getCachePath() {
		return mContext.getCacheDir().getAbsolutePath();
	}

	public void acquireWifiLock() {
		Log.i("acquireWifiLock()");
		mWifiLock.acquire();
	}

	public void releaseWifiLock() {
		Log.i("releaseWifiLock()");
		mWifiLock.release();
	}

	public void acquireMcastLock() {
		Log.i("acquireMcastLock()");
		mMcastLock.acquire();
	}

	public void releaseMcastLock() {
		Log.i("releaseMcastLock()");
		mMcastLock.release();
	}

	public void acquireCpuLock() {
		Log.i("acquireCpuLock()");
		mWakeLock.acquire();
	}

	public void releaseCpuLock() {
		Log.i("releaseCpuLock()");
		mWakeLock.release();
	}

	private int getResourceIdentifierFromName(String name) {
		int resId = mResources.getIdentifier(name, "raw", mContext.getPackageName());
		if (resId == 0) {
			Log.d("App doesn't seem to embed resource " + name + "in it's res/raw/ directory, use linphone's instead");
			resId = mResources.getIdentifier(name, "raw", "org.linphone");
		}
		return resId;
	}

	private void copyAssetsFromPackage() throws IOException {
		copyIfNotExist(getResourceIdentifierFromName("notes_of_the_optimistic"), mRingSoundFile);
		copyIfNotExist(getResourceIdentifierFromName("ringback"), mRingbackSoundFile);
		copyIfNotExist(getResourceIdentifierFromName("hold"), mPauseSoundFile);
		copyIfNotExist(getResourceIdentifierFromName("incoming_chat"), mErrorToneFile);
		copyIfNotExist(getResourceIdentifierFromName("cpim_grammar"), mGrammarCpimFile);
		copyIfNotExist(getResourceIdentifierFromName("vcard_grammar"), mGrammarVcardFile);
		copyIfNotExist(getResourceIdentifierFromName("rootca"), mLinphoneRootCaFile);
	}

	public void copyIfNotExist(int ressourceId, String target) throws IOException {
		File lFileToCopy = new File(target);
		if (!lFileToCopy.exists()) {
			copyFromPackage(ressourceId,lFileToCopy.getName());
		}
	}

	public void copyFromPackage(int ressourceId, String target) throws IOException {
		InputStream lInputStream = mResources.openRawResource(ressourceId);
		FileOutputStream lOutputStream = mContext.openFileOutput (target, 0);
		int readByte;
		byte[] buff = new byte[8048];
		while (( readByte = lInputStream.read(buff)) != -1) {
			lOutputStream.write(buff,0, readByte);
		}
		lOutputStream.flush();
		lOutputStream.close();
		lInputStream.close();
	}
};


