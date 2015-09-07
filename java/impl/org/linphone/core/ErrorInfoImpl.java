package org.linphone.core;

public class ErrorInfoImpl implements ErrorInfo {
	private Reason mReason;
	private int mCode;
	private String mPhrase;
	private String mDetails;
	
	private native int getReason(long nativePtr);
	private native int getProtocolCode(long nativePtr);
	private native String getPhrase(long nativePtr);
	private native String getDetails(long nativePtr);
	
	public ErrorInfoImpl(long nativePtr){
		mReason=Reason.fromInt(getReason(nativePtr));
		mCode=getProtocolCode(nativePtr);
		mPhrase=getPhrase(nativePtr);
		mDetails=getDetails(nativePtr);
	}
	
	@Override
	public Reason getReason() {
		return mReason;
	}

	@Override
	public int getProtocolCode() {
		return mCode;
	}

	@Override
	public String getPhrase() {
		return mPhrase;
	}

	@Override
	public String getDetails() {
		return mDetails;
	}

}
