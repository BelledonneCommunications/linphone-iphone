package org.linphone.core;

public class LinphoneContentImpl implements LinphoneContent {
	private String mType, mSubtype, mEncoding, mName;
	private byte[] mData;
	private int mExpectedSize;

	public LinphoneContentImpl(String type, String subtype, byte data[], String encoding){
		mType = type;
		mSubtype = subtype;
		mData = data;
		mEncoding = encoding;
		mName = null;
		mExpectedSize = 0;
	}
	
	public LinphoneContentImpl(String name, String type, String subtype, byte data[], String encoding, int expectedSize){
		mType = type;
		mSubtype = subtype;
		mData = data;
		mEncoding = encoding;
		mName = name;
		mExpectedSize = expectedSize;
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
		if (mData != null)
			return new String(mData);
		return null;
	}
	
	@Override
	public void setExpectedSize(int size) {
		mExpectedSize = size;
	}

	@Override
	public int getExpectedSize() {
		return mExpectedSize;
	}

	@Override
	public int getRealSize() {
		if (mData != null)
			return mData.length;
		return 0;
	}

	@Override
	public void setType(String type) {
		mType = type;
	}

	@Override
	public void setSubtype(String subtype) {
		mSubtype = subtype;
	}

	@Override
	public void setStringData(String data) {
		if (data != null)
			mData = data.getBytes();
		else
			mData = null;
	}

	@Override
	public void setData(byte data[]){
		mData = data;
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
		mEncoding = encoding;
	}

	@Override
	public void setName(String name) {
		mName = name;
	}

	@Override
	public String getName() {
		return mName;
	}
}
