/*
LinphoneContentImpl.java
Copyright (C) 2015  Belledonne Communications, Grenoble, France

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
