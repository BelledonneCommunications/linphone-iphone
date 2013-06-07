package org.linphone.core;

public class LinphoneInfoMessageImpl implements LinphoneInfoMessage {
	protected long nativePtr;
	private LinphoneContent mContent;
	
	private native Object getContent(long infoptr);
	public LinphoneInfoMessageImpl(long ptr){
		nativePtr=ptr;
		mContent=(LinphoneContent)getContent(nativePtr);
	}
	
	private native void setContent(long nativePtr, String type, String subtype, String data);
	@Override
	public void setContent(LinphoneContent content) {
		mContent=content;
		setContent(nativePtr,mContent.getType(),mContent.getSubtype(),mContent.getDataAsString());
	}

	@Override
	public LinphoneContent getContent() {
		return mContent;
	}

	private native void addHeader(long nativePtr, String name, String value);
	@Override
	public void addHeader(String name, String value) {
		addHeader(nativePtr,name,value);
	}

	private native String getHeader(long nativePtr, String name);
	@Override
	public String getHeader(String name) {
		return getHeader(nativePtr,name);
	}
	
	private native void delete(long nativePtr);
	protected void finalize(){
		delete(nativePtr);
	}
}
