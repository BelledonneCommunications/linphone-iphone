package org.linphone.core;

public class LinphoneBufferImpl implements LinphoneBuffer {
	private byte[] mData;
	private int mSize;

	public LinphoneBufferImpl(byte[] data, int size)
	{
		mData = data;
		mSize = size;
	}
	
	@Override
	public byte[] getContent() {
		return mData;
	}

	@Override
	public void setContent(byte[] data) {
		mData = data;
	}

	@Override
	public int getSize() {
		return mSize;
	}

	@Override
	public void setSize(int size) {
		mSize = size;
	}

}
