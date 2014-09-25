package org.linphone.core;


public class LinphoneEventImpl implements LinphoneEvent {
	private Object mUserContext;
	private long mNativePtr;
	
	protected LinphoneEventImpl(long nativePtr){
		mNativePtr=nativePtr;
	}
	
	private native String getEventName(long nativeptr);
	@Override
	public synchronized String getEventName() {
		return getEventName(mNativePtr);
	}

	private native int acceptSubscription(long nativeptr);
	@Override
	public synchronized void acceptSubscription() {
		synchronized(getCore()){
			acceptSubscription(mNativePtr);
		}
	}

	private native int denySubscription(long nativeptr, int reason);
	@Override
	public synchronized void denySubscription(Reason reason) {
		synchronized(getCore()){
			denySubscription(mNativePtr,reason.mValue);
		}
	}

	private native int notify(long nativeptr, String type, String subtype, byte data[], String encoding);
	@Override
	public void notify(LinphoneContent content) {
		synchronized(getCore()){
			notify(mNativePtr,content.getType(),content.getSubtype(),content.getData(),content.getEncoding());
		}
	}

	private native int updateSubscribe(long nativePtr, String type, String subtype, byte data[], String encoding);
	@Override
	public void updateSubscribe(LinphoneContent content) {
		synchronized(getCore()){
			updateSubscribe(mNativePtr,content.getType(), content.getSubtype(),content.getData(),content.getEncoding());
		}
	}

	private native int updatePublish(long nativePtr, String type, String subtype, byte data[], String encoding);
	@Override
	public void updatePublish(LinphoneContent content) {
		synchronized(getCore()){
			updatePublish(mNativePtr,content.getType(), content.getSubtype(),content.getData(),content.getEncoding());
		}
	}

	private native int terminate(long nativePtr);
	@Override
	public void terminate() {
		synchronized(getCore()){
			terminate(mNativePtr);
		}
	}

	private native int getReason(long nativePtr);
	@Override
	public synchronized Reason getReason() {
		return Reason.fromInt(getReason(mNativePtr));
	}

	@Override
	public synchronized void setUserContext(Object obj) {
		mUserContext=obj;
	}

	@Override
	public synchronized Object getUserContext() {
		return mUserContext;
	}

	private native int getSubscriptionDir(long nativeptr);
	@Override
	public synchronized SubscriptionDir getSubscriptionDir() {
		return SubscriptionDir.fromInt(getSubscriptionDir(mNativePtr));
	}

	private native int getSubscriptionState(long nativeptr);
	@Override
	public synchronized SubscriptionState getSubscriptionState() {
		try {
			return SubscriptionState.fromInt(getSubscriptionState(mNativePtr));
		} catch (LinphoneCoreException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return SubscriptionState.Error;
	}
	private native void unref(long nativeptr);
	protected void finalize(){
		unref(mNativePtr);
	}

	private native void addCustomHeader(long ptr, String name, String value);
	@Override
	public synchronized void addCustomHeader(String name, String value) {
		addCustomHeader(mNativePtr, name, value);
	}

	private native String getCustomHeader(long ptr, String name);
	@Override
	public synchronized String getCustomHeader(String name) {
		return getCustomHeader(mNativePtr, name);
	}

	private native void sendSubscribe(long ptr, String type, String subtype, byte data [], String encoding);
	@Override
	public void sendSubscribe(LinphoneContent body) {
		synchronized(getCore()){
			if (body != null)
				sendSubscribe(mNativePtr, body.getType(), body.getSubtype(), body.getData(), body.getEncoding());
			else
				sendSubscribe(mNativePtr, null, null, null, null);
		}
	}
	
	private native void sendPublish(long ptr, String type, String subtype, byte data [], String encoding);
	@Override
	public void sendPublish(LinphoneContent body) {
		synchronized(getCore()){
			if (body != null)
				sendPublish(mNativePtr, body.getType(), body.getSubtype(), body.getData(), body.getEncoding());
			else
				sendPublish(mNativePtr, null, null, null, null);
		}
	}
	private native long getErrorInfo(long nativePtr);
	@Override
	public synchronized ErrorInfo getErrorInfo() {
		return new ErrorInfoImpl(getErrorInfo(mNativePtr));
	}

	private native Object getCore(long nativePtr);
	@Override
	public synchronized LinphoneCore getCore() {
		return (LinphoneCore)getCore(mNativePtr);
	}

}
