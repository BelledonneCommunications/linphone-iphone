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
		//make sur to follow same path as unix version of the sdk
		mLinphoneRootCaFile = basePath + "/share/linphone/rootca.pem";
		mRingSoundFile = basePath + "/share/sounds/linphone/rings/notes_of_the_optimistic.mkv";
		mRingbackSoundFile = basePath + "/share/sounds/linphone/ringback.wav";
		mPauseSoundFile = basePath + "/share/sounds/linphone/rings/dont_wait_too_long.mkv";
		mErrorToneFile = basePath + "/share/sounds/linphone/incoming_chat.wav";
		mGrammarCpimFile = basePath + "/share/belr/grammars/cpim_grammar";
		mGrammarVcardFile = basePath + "/share/belr/grammars/vcard_grammar";
		mUserCertificatePath = basePath;

		try {
			copyAssetsFromPackage();
		} catch (Exception e) {
			Log.e(e, "AndroidPlatformHelper: Cannot copy assets from package.");
		}
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
			if (resId == 0) {
				Log.e("App doesn't seem to embed resource " + name + "in it's res/raw/ directory, please add it");
			}
		}
		return resId;
	}

	private void copyAssetsFromPackage() throws IOException {
		Log.i("Starting copy from assets to application files directory");
		copyAssetsFromPackage(mContext, "org.linphone.core",".");
		Log.i("Copy from assets done");
		Log.i("Starting copy from legacy  resources to application files directory");
		/*legacy code for 3.X*/
		copyEvenIfExists(getResourceIdentifierFromName("cpim_grammar"), mGrammarCpimFile);
		copyEvenIfExists(getResourceIdentifierFromName("vcard_grammar"), mGrammarVcardFile);
		copyEvenIfExists(getResourceIdentifierFromName("rootca"), mLinphoneRootCaFile);
		copyEvenIfExists(getResourceIdentifierFromName("notes_of_the_optimistic"), mRingSoundFile);
		copyEvenIfExists(getResourceIdentifierFromName("ringback"), mRingbackSoundFile);
		copyEvenIfExists(getResourceIdentifierFromName("hold"), mPauseSoundFile);
		copyEvenIfExists(getResourceIdentifierFromName("incoming_chat"), mErrorToneFile);
		Log.i("Copy from legacy resources done");
	}

	public void copyEvenIfExists(int ressourceId, String target) throws IOException {
		File lFileToCopy = new File(target);
		copyFromPackage(ressourceId, lFileToCopy);
	}

	public void copyIfNotExist(int ressourceId, String target) throws IOException {
		File lFileToCopy = new File(target);
		if (!lFileToCopy.exists()) {
			copyFromPackage(ressourceId, lFileToCopy);
		}
	}

	public void copyFromPackage(int ressourceId, File target) throws IOException {
		if (ressourceId == 0) {
			Log.i("Resource identifier null for target ["+target.getName()+"]");
			return;
		}
		if (!target.getParentFile().exists())
			target.getParentFile().mkdirs();

		InputStream lInputStream = mResources.openRawResource(ressourceId);
		FileOutputStream lOutputStream = new FileOutputStream(target);
		int readByte;
		byte[] buff = new byte[8048];
		while (( readByte = lInputStream.read(buff)) != -1) {
			lOutputStream.write(buff,0, readByte);
		}
		lOutputStream.flush();
		lOutputStream.close();
		lInputStream.close();
	}

	public static void copyAssetsFromPackage(Context ctx,String fromPath, String toPath) throws IOException {
		new File(ctx.getFilesDir().getPath()+"/"+toPath).mkdir();

		for (String f :ctx.getAssets().list(fromPath)) {
			String current_name = fromPath+"/"+f;
			String current_dest = toPath+"/"+f;
			InputStream lInputStream;
			try {
				lInputStream = ctx.getAssets().open(current_name);
			} catch (IOException e) {
				//probably a dir
				copyAssetsFromPackage(ctx,current_name,current_dest);
				continue;
			}
			FileOutputStream lOutputStream =  new FileOutputStream(new File(ctx.getFilesDir().getPath()+"/"+current_dest));//ctx.openFileOutput (fromPath+"/"+f, 0);


			int readByte;
			byte[] buff = new byte[8048];
			while (( readByte = lInputStream.read(buff)) != -1) {
				lOutputStream.write(buff,0, readByte);
			}
			lOutputStream.flush();
			lOutputStream.close();
			lInputStream.close();
		}
	}
};


