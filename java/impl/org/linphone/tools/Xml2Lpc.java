package org.linphone.tools;

import org.linphone.core.LpConfig;
import org.linphone.mediastream.Log;

public class Xml2Lpc {
	
    private enum LogLevel {
    	DEBUG,
    	MESSAGE,
    	WARNING,
    	ERROR
    }
    
    private static boolean mAvailable;
    
	private long internalPtr = 0;
	
	private native void init();
	private native void destroy();
	
	public Xml2Lpc() {
		init();
	}
	
	public void finalize() {
		destroy();
	}
	
	public native int setXmlFile(String filename);
	public native int setXmlString(String content);

	public native int setXsdFile(String filename);
	public native int setXsdString(String content);

	public native int validate();
	public native int convert(LpConfig config);
	
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
	
	public static boolean isAvailable() {
		return mAvailable;
	}
	
	// Load library
	static {
		try { 
			new Xml2Lpc();
			//System.loadLibrary("xml2");
			//System.loadLibrary("xml2lpc");
			mAvailable = true;
		} catch (Throwable e) {
			mAvailable = false;
		}
	}
}
