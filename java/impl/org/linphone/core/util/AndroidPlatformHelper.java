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


package org.linphone.core.util;

import org.linphone.mediastream.Log;

import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.net.wifi.WifiManager.WifiLock;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkInfo;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;

import java.net.InetAddress;
import java.util.List;
import android.os.Build;

/**
 * This class is instanciated directly by the linphone library in order to access specific features only accessible in java.
**/

public class AndroidPlatformHelper{
	private WifiManager.WifiLock mWifiLock;
	private WifiManager.MulticastLock mMcastLock;
	private ConnectivityManager mConnectivityManager;
	private PowerManager mPowerManager;
	private WakeLock mWakeLock;

	public AndroidPlatformHelper(Object ctx_obj){
		Context ctx = (Context) ctx_obj;
		WifiManager wifiMgr = (WifiManager)ctx.getSystemService(Context.WIFI_SERVICE);
		mPowerManager = (PowerManager) ctx.getSystemService(Context.POWER_SERVICE);
		mConnectivityManager = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
		
		mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,	"AndroidPlatformHelper");
		mWakeLock.setReferenceCounted(true);
		mMcastLock = wifiMgr.createMulticastLock("AndroidPlatformHelper");
		mMcastLock.setReferenceCounted(true);
		mWifiLock = wifiMgr.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "AndroidPlatformHelper");
		mWifiLock.setReferenceCounted(true);
	}
	
	public Object getPowerManager(){
		return mPowerManager;
	}
	
	public String[] getDnsServers() {
		if (mConnectivityManager == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
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
	public void acquireWifiLock(){
		Log.i("acquireWifiLock()");
		mWifiLock.acquire();
	}
	public void releaseWifiLock(){
		Log.i("releaseWifiLock()");
		mWifiLock.release();
	}
	public void acquireMcastLock(){
		Log.i("acquireMcastLock()");
		mMcastLock.acquire();
	}
	public void releaseMcastLock(){
		Log.i("releaseMcastLock()");
		mMcastLock.release();
	}
	public void acquireCpuLock(){
		Log.i("acquireCpuLock()");
		mWakeLock.acquire();
	}
	public void releaseCpuLock(){
		Log.i("releaseCpuLock()");
		mWakeLock.release();
	}
};


