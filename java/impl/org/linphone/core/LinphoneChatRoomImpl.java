/*
LinphoneChatRoomImpl.java
Copyright (C) 2010  Belledonne Communications, Grenoble, France

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.core;

import org.linphone.core.LinphoneChatMessage.State;
import org.linphone.core.LinphoneChatMessage.StateListener;

class LinphoneChatRoomImpl implements LinphoneChatRoom {
	protected final long nativePtr;
	private native long createLinphoneChatMessage(long ptr, String message);
	private native long getPeerAddress(long ptr);
	private native void sendMessage(long ptr, String message);
	private native void sendMessage2(long ptr, Object msg, long messagePtr, StateListener listener);
	private native long[] getHistory(long ptr, int limit);
	private native void destroy(long ptr);
	private native int getUnreadMessagesCount(long ptr);
	private native void deleteHistory(long ptr);
	private native void compose(long ptr);
	private native boolean isRemoteComposing(long ptr);
	private native void markAsRead(long ptr);
	private native void deleteMessage(long room, long message);
	private native void updateUrl(long room, long message);
	private native long createLinphoneChatMessage2(long ptr, String message,
			String url, int state, long timestamp, boolean isRead,
			boolean isIncoming);

	protected LinphoneChatRoomImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}

	public synchronized LinphoneAddress getPeerAddress() {
		return new LinphoneAddressImpl(getPeerAddress(nativePtr),LinphoneAddressImpl.WrapMode.FromConst);
	}

	public synchronized void sendMessage(String message) {
		synchronized(getCore()){
			sendMessage(nativePtr,message);
		}
	}
	
	@Override
	public synchronized void sendMessage(LinphoneChatMessage message, StateListener listener) {
		synchronized(getCore()){
			sendMessage2(nativePtr, message, ((LinphoneChatMessageImpl)message).getNativePtr(), listener);
		}
	}

	@Override
	public synchronized LinphoneChatMessage createLinphoneChatMessage(String message) {
		synchronized(getCore()){
			return new LinphoneChatMessageImpl(createLinphoneChatMessage(nativePtr, message));
		}
	}
	
	public synchronized LinphoneChatMessage[] getHistory() {
		synchronized(getCore()){
			return getHistory(0);
		}
	}
	
	public synchronized LinphoneChatMessage[] getHistory(int limit) {
		synchronized(getCore()){
			long[] typesPtr = getHistory(nativePtr, limit);
			if (typesPtr == null) return null;
			
			LinphoneChatMessage[] messages = new LinphoneChatMessage[typesPtr.length];
			for (int i=0; i < messages.length; i++) {
				messages[i] = new LinphoneChatMessageImpl(typesPtr[i]);
			}
	
			return messages;
		}
	}
	
	public synchronized void destroy() {
		destroy(nativePtr);
	}
	
	public synchronized int getUnreadMessagesCount() {
		synchronized(getCore()){
			return getUnreadMessagesCount(nativePtr);
		}
	}
	
	public synchronized void deleteHistory() {
		synchronized(getCore()){
			deleteHistory(nativePtr);
		}
	}

	public synchronized void compose() {
		synchronized(getCore()){
			compose(nativePtr);
		}
	}

	public synchronized boolean isRemoteComposing() {
		synchronized(getCore()){
			return isRemoteComposing(nativePtr);
		}
	}
	
	public synchronized void markAsRead() {
		synchronized(getCore()){
			markAsRead(nativePtr);
		}
	}
	
	public synchronized void deleteMessage(LinphoneChatMessage message) {
		synchronized(getCore()){
			if (message != null)
				deleteMessage(nativePtr, ((LinphoneChatMessageImpl)message).getNativePtr());
		}
	}
	
	public synchronized void updateUrl(LinphoneChatMessage message) {
		synchronized(getCore()){
			if (message != null)
				updateUrl(nativePtr, ((LinphoneChatMessageImpl)message).getNativePtr());
		}
	}
	
	@Override
	public synchronized LinphoneChatMessage createLinphoneChatMessage(String message,
			String url, State state, long timestamp, boolean isRead,
			boolean isIncoming) {
		synchronized(getCore()){
			return new LinphoneChatMessageImpl(createLinphoneChatMessage2(
					nativePtr, message, url, state.value(), timestamp / 1000, isRead, isIncoming));
		}
	}
	private native Object getCore(long nativePtr);
	@Override
	public synchronized LinphoneCore getCore() {
		return (LinphoneCore)getCore(nativePtr);
	}
}
