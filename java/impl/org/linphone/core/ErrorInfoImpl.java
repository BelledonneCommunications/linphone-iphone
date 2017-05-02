package org.linphone.core;

public class ErrorInfoImpl implements ErrorInfo {
	protected long mNativePtr;
	
	private native void ref(long nativePtr);
	private native void unref(long nativePtr);
	
	private native int getReason(long nativePtr);
	private native String getProtocol(long nativePtr);
	private native int getProtocolCode(long nativePtr);
	private native String getPhrase(long nativePtr);
	private native String getWarnings(long nativePtr);
	private native long getSubErrorInfo(long nativePtr);
	
	private native void setReason(long nativePtr, int reason);
	private native void setProtocol(long nativePtr, String proto);
	private native void setProtocolCode(long nativePtr, int code);
	private native void setPhrase(long nativePtr, String phrase);
	private native void setWarnings(long nativePtr, String warnings);
	private native void setSubErrorInfo(long nativePtr, long other_nativePtr);
	
	public ErrorInfoImpl(long nativePtr, boolean ownsRef){
		if (!ownsRef) ref(nativePtr);
		mNativePtr = nativePtr;
	}
	
	public ErrorInfoImpl(long nativePtr){
		ref(nativePtr);
		mNativePtr = nativePtr;
	}
	
	@Override
	public Reason getReason() {
		return Reason.fromInt(getReason(mNativePtr));
	}
	
	@Override
	public String getProtocol() {
		return getProtocol(mNativePtr);
	}

	@Override
	public int getProtocolCode() {
		return getProtocolCode(mNativePtr);
	}

	@Override
	public String getPhrase() {
		return getPhrase(mNativePtr);
	}
	
	@Override
	public String getWarnings(){
		return getWarnings(mNativePtr);
	}
	
	@Override
	public ErrorInfo getSubErrorInfo(){
		long sub_ei = getSubErrorInfo(mNativePtr);
		return sub_ei != 0 ? new ErrorInfoImpl(sub_ei, false) : null;
	}

	
	@Override
	public void setReason(Reason reason) {
		setReason(mNativePtr, reason.mValue); 
	}
	
	@Override
	public void setProtocol(String proto) {
		setProtocol(mNativePtr, proto);
	}

	@Override
	public void setProtocolCode(int code) {
		setProtocolCode(mNativePtr, code);
	}

	@Override
	public void setPhrase(String phrase) {
		setPhrase(mNativePtr, phrase);
	}
	
	@Override
	public void setWarnings(String warnings){
		setWarnings(mNativePtr, warnings);
	}
	
	@Override
	public void setSubErrorInfo(ErrorInfo ei){
		setSubErrorInfo(mNativePtr, ei != null ? ((ErrorInfoImpl)ei).mNativePtr : 0);
	}
	
	
	@Override
	public String getDetails() {
		return getWarnings();
	}

	protected void finalize() throws Throwable {
		if (mNativePtr != 0){
			unref(mNativePtr);
			mNativePtr = 0;
		}
	}
}
