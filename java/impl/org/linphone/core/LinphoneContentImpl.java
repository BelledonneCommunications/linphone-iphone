package org.linphone.core;

public class LinphoneContentImpl implements LinphoneContent {
	private String mType, mSubtype, mEncoding;
	private byte[] mData;

	public LinphoneContentImpl(String type, String subtype, byte data[], String encoding ){
		mType=type;
		mSubtype=subtype;
		mData=data;
		mEncoding=encoding;
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
		return new String(mData);
	}

	@Override
	public int getSize() {
		return mData.length;
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
		mData=data.getBytes();
	}

	@Override
	public void setData(byte data[]){
		mData=data;
	}

	@Override
	public String getEncoding() {
		return mEncoding;
	}

	@Override
	public byte[] getData() {
		return mData;
	}

	@Override
	public void setEncoding(String encoding) {
		mEncoding=encoding;
	}

}
