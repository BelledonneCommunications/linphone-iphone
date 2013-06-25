package org.linphone.core;

public class LinphoneChatMessageImpl implements LinphoneChatMessage {
	protected final long nativePtr;
	private native void setUserData(long ptr);
	private native String getText(long ptr);
	private native long getPeerAddress(long ptr);
	private native String getExternalBodyUrl(long ptr);
	private native void setExternalBodyUrl(long ptr, String url);
	private native long getFrom(long ptr);
	private native long getTime(long ptr);
	
	protected LinphoneChatMessageImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
		setUserData();
	}
	
	public long getNativePtr() {
		return nativePtr;
	}
	
	@Override
	public Object getUserData() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setUserData() {
		setUserData(nativePtr);
	}

	@Override
	public String getText() {
		return getText(nativePtr);
	}
	
	@Override
	public LinphoneAddress getPeerAddress() {
		return new LinphoneAddressImpl(getPeerAddress(nativePtr));
	}
	
	@Override
	public String getExternalBodyUrl() {
		return getExternalBodyUrl(nativePtr);
	}
	
	@Override
	public void setExternalBodyUrl(String url) {
		setExternalBodyUrl(nativePtr, url);
	}
	
	@Override
	public LinphoneAddress getFrom() {
		return new LinphoneAddressImpl(getFrom(nativePtr));
	}
	
	private native void addCustomHeader(long nativePtr, String name, String value);
	@Override
	public void addCustomHeader(String name, String value) {
		addCustomHeader(nativePtr, name, value);
	}
	private native String getCustomHeader(long nativePtr, String name);
	@Override
	public String getCustomHeader(String name) {
		return getCustomHeader(nativePtr,name);
	}
	
	public long getTime() {
		return getTime(nativePtr) * 1000; // Need milliseconds, not seconds
	}
}
