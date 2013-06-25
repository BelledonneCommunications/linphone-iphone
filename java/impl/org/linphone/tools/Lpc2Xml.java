package org.linphone.tools;

import org.linphone.core.LpConfig;
import org.linphone.mediastream.Log;

public class Lpc2Xml {
	
    private enum LogLevel {
    	DEBUG,
    	MESSAGE,
    	WARNING,
    	ERROR,
    }
    
    private static boolean mAvailable;
    
	private long internalPtr = 0;
	
	private native void init();
	private native void destroy();
	
	public Lpc2Xml() {
		init();
	}
	
	public void finalize() {
		destroy();
	}
	
	public native int setLpc(LpConfig lpc);

	public native int convertFile(String file);
	public native int convertString(StringBuffer content);
	
	public void printLog(int level, String message) {
		if(level > 0 && level < LogLevel.values().length) {
			switch(LogLevel.values()[level]) {
				case DEBUG:
					Log.d(message);
				break;
				case MESSAGE:
					Log.i(message);
				break;
				case WARNING:
					Log.w(message);
				break;
				case ERROR:
					Log.e(message);
				break;
			}
		}
	}
	
	static boolean isAvailable() {
		return mAvailable;
	}
	
	// Load library
	static {
		try {
			System.loadLibrary("xml2");
			//System.loadLibrary("lpc2xml");
			mAvailable = true;
		} catch (Throwable e) {
			mAvailable = false;
		}
	}
}
