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
	private native void sendMessage2(long ptr, long message, StateListener listener);
	private native long[] getHistory(long ptr, int limit);
	private native void destroy(long ptr);
	private native int getUnreadMessagesCount(long ptr);
	private native void deleteHistory(long ptr);
	private native void markAsRead(long ptr);
	private native void deleteMessage(long room, long message);
	private native void updateUrl(long room, long message);
	private native long createLinphoneChatMessage2(long ptr, String message,
			String url, int state, long timestamp, boolean isRead,
			boolean isIncoming);

	protected LinphoneChatRoomImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}

	public LinphoneAddress getPeerAddress() {
		return new LinphoneAddressImpl(getPeerAddress(nativePtr));
	}

	public void sendMessage(String message) {
		sendMessage(nativePtr,message);
	}
	
	@Override
	public void sendMessage(LinphoneChatMessage message, StateListener listener) {
		sendMessage2(nativePtr, message.getNativePtr(), listener);
	}

	@Override
	public LinphoneChatMessage createLinphoneChatMessage(String message) {
		return new LinphoneChatMessageImpl(createLinphoneChatMessage(nativePtr, message));
	}
	
	public LinphoneChatMessage[] getHistory() {
		return getHistory(0);
	}
	
	public LinphoneChatMessage[] getHistory(int limit) {
		long[] typesPtr = getHistory(nativePtr, limit);
		if (typesPtr == null) return null;
		
		LinphoneChatMessage[] messages = new LinphoneChatMessage[typesPtr.length];
		for (int i=0; i < messages.length; i++) {
			messages[i] = new LinphoneChatMessageImpl(typesPtr[i]);
		}

		return messages;
	}
	
	public void destroy() {
		destroy(nativePtr);
	}
	
	public int getUnreadMessagesCount() {
		return getUnreadMessagesCount(nativePtr);
	}
	
	public void deleteHistory() {
		deleteHistory(nativePtr);
	}
	
	public void markAsRead() {
		markAsRead(nativePtr);
	}
	
	public void deleteMessage(LinphoneChatMessage message) {
		if (message != null)
			deleteMessage(nativePtr, message.getNativePtr());
	}
	
	public void updateUrl(LinphoneChatMessage message) {
		if (message != null)
			updateUrl(nativePtr, message.getNativePtr());
	}
	
	@Override
	public LinphoneChatMessage createLinphoneChatMessage(String message,
			String url, State state, long timestamp, boolean isRead,
			boolean isIncoming) {
		return new LinphoneChatMessageImpl(createLinphoneChatMessage2(
				nativePtr, message, url, state.value(), timestamp / 1000, isRead, isIncoming));
	}
}
