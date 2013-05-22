package org.linphone.core;

public class LinphoneContentImpl implements LinphoneContent {
	private String mType, mSubtype, mData;
	public LinphoneContentImpl(String type, String subtype, String data){
		mType=type;
		mSubtype=subtype;
		mData=data;
	}
	
	@Override
	public String getType() {
		return mType;
	}

	@Override
	public String getSubtype() {
		return mSubtype;
	}

	@Override
	public String getDataAsString() {
		return mData;
	}

	@Override
	public int getSize() {
		return mData.length();
	}

	@Override
	public void setType(String type) {
		mType=type;
	}

	@Override
	public void setSubtype(String subtype) {
		mSubtype=subtype;
	}

	@Override
	public void setStringData(String data) {
		mData=data;
	}

}
