/*
LinphoneXmlRpcRequestImpl.java
Copyright (C) 2016  Belledonne Communications, Grenoble, France

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


public class LinphoneXmlRpcRequestImpl implements LinphoneXmlRpcRequest {
	protected long nativePtr;
	
	protected LinphoneXmlRpcRequestImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}
	
	private native long newLinphoneXmlRpcRequest(String methodName, int returnType);
	public LinphoneXmlRpcRequestImpl(String methodName, ArgType returnType) {
		nativePtr = newLinphoneXmlRpcRequest(methodName, returnType.value());
	}
	
	public long getNativePtr() {
		return nativePtr;
	}

	private native void unref(long ptr);
	protected void finalize(){
		unref(nativePtr);
	}
	
	private native void addIntArg(long ptr, int arg);
	@Override
	public void addIntArg(int arg) {
		addIntArg(nativePtr, arg);
	}
	
	private native void addStringArg(long ptr, String arg);
	@Override
	public void addStringArg(String arg) {
		addStringArg(nativePtr, arg);
	}
	
	private native String getContent(long ptr);
	@Override
	public String getContent() {
		return getContent(nativePtr);
	}
	
	private native int getStatus(long ptr);
	@Override
	public Status getStatus() {
		return Status.fromInt(getStatus(nativePtr));
	}
	
	private native int getIntResponse(long ptr);
	@Override
	public int getIntResponse() {
		return getIntResponse(nativePtr);
	}
	
	private native String getStringResponse(long ptr);
	@Override
	public String getStringResponse() {
		return getStringResponse(nativePtr);
	}

	private native void setListener(long ptr, LinphoneXmlRpcRequestListener listener);
	@Override
	public void setListener(LinphoneXmlRpcRequestListener listener) {
		setListener(nativePtr, listener);
	}
}
