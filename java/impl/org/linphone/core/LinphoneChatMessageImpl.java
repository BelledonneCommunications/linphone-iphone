package org.linphone.core;

public class LinphoneChatMessageImpl implements LinphoneChatMessage {
	protected final long nativePtr;
	private native String getText(long ptr);
	private native long getPeerAddress(long ptr);
	private native String getExternalBodyUrl(long ptr);
	private native void setExternalBodyUrl(long ptr, String url);
	private native long getFrom(long ptr);
	private native long getTime(long ptr);
	private native int getStatus(long ptr);
	private native boolean isRead(long ptr);
	private native boolean isOutgoing(long ptr);
	private native void store(long ptr);
	private native int getStorageId(long ptr);
	private native void unref(long ptr);
	
	protected LinphoneChatMessageImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}
	
	public long getNativePtr() {
		return nativePtr;
	}
	
	@Override
	public String getText() {
		return getText(nativePtr);
	}
	
	@Override
	public LinphoneAddress getPeerAddress() {
		return new LinphoneAddressImpl(getPeerAddress(nativePtr),LinphoneAddressImpl.WrapMode.FromConst);
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
		return new LinphoneAddressImpl(getFrom(nativePtr),LinphoneAddressImpl.WrapMode.FromConst);
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
	
	public LinphoneChatMessage.State getStatus() {
		return LinphoneChatMessage.State.fromInt(getStatus(nativePtr));
	}
	
	public boolean isRead() {
		return isRead(nativePtr);
	}
	
	public boolean isOutgoing() {
		return isOutgoing(nativePtr);
	}
	
	public void store() {
		store(nativePtr);
	}
	
	public int getStorageId() {
		return getStorageId(nativePtr);
	}

	private native int getReason(long ptr);

    public Reason getReason() {
        return Reason.fromInt(getReason(nativePtr));
    }
    private native long getErrorInfo(long nativePtr);
	@Override
	public ErrorInfo getErrorInfo() {
		return new ErrorInfoImpl(getErrorInfo(nativePtr));
	}
	protected void finalize(){
		unref(nativePtr);
	}
}
