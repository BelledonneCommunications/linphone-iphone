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
import org.linphone.core.LinphoneCall;

@SuppressWarnings("deprecation")
class LinphoneChatRoomImpl implements LinphoneChatRoom {
	protected final long nativePtr;
	private native long createLinphoneChatMessage(long ptr, String message);
	private native long getPeerAddress(long ptr);
	private native void sendMessage(long ptr, String message);
	private native void sendMessage2(long ptr, Object msg, long messagePtr, StateListener listener);
	private native long[] getHistoryRange(long ptr, int begin, int end);
	private native long[] getHistory(long ptr, int limit);
	private native void destroy(long ptr);
	private native int getUnreadMessagesCount(long ptr);
	private native int getHistorySize(long ptr);
	private native void deleteHistory(long ptr);
	private native void compose(long ptr);
	private native boolean isRemoteComposing(long ptr);
	private native void markAsRead(long ptr);
	private native void deleteMessage(long room, long message);
	private native long createLinphoneChatMessage2(long ptr, String message,
			String url, int state, long timestamp, boolean isRead,
			boolean isIncoming);
	private native void sendChatMessage(long ptr, Object message, long messagePtr);

	protected LinphoneChatRoomImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}

	public LinphoneAddress getPeerAddress() {
		return new LinphoneAddressImpl(getPeerAddress(nativePtr),LinphoneAddressImpl.WrapMode.FromConst);
	}

	public void sendMessage(String message) {
		synchronized(getCore()){
			sendMessage(nativePtr,message);
		}
	}

	@Override
	public void sendMessage(LinphoneChatMessage message, StateListener listener) {
		synchronized(getCore()){
			sendMessage2(nativePtr, message, ((LinphoneChatMessageImpl)message).getNativePtr(), listener);
		}
	}

	@Override
	public LinphoneChatMessage createLinphoneChatMessage(String message) {
		synchronized(getCore()){
			return new LinphoneChatMessageImpl(createLinphoneChatMessage(nativePtr, message));
		}
	}

	public LinphoneChatMessage[] getHistory() {
		synchronized(getCore()){
			return getHistory(0);
		}
	}

	public LinphoneChatMessage[] getHistoryRange(int begin, int end) {
		synchronized(getCore()){
			long[] typesPtr = getHistoryRange(nativePtr, begin, end);
			return getHistoryPrivate(typesPtr);
		}
	}

	public LinphoneChatMessage[] getHistory(int limit) {
		synchronized(getCore()){
			long[] typesPtr = getHistory(nativePtr, limit);
			return getHistoryPrivate(typesPtr);
		}
	}

	public int getUnreadMessagesCount() {
		synchronized(getCore()){
			return getUnreadMessagesCount(nativePtr);
		}
	}

	public int getHistorySize() {
		synchronized(getCore()){
			return getHistorySize(nativePtr);
		}
	}

	public void deleteHistory() {
		synchronized(getCore()){
			deleteHistory(nativePtr);
		}
	}

	public void compose() {
		synchronized(getCore()){
			compose(nativePtr);
		}
	}

	public boolean isRemoteComposing() {
		synchronized(getCore()){
			return isRemoteComposing(nativePtr);
		}
	}

	public void markAsRead() {
		synchronized(getCore()){
			markAsRead(nativePtr);
		}
	}

	public void deleteMessage(LinphoneChatMessage message) {
		synchronized(getCore()){
			if (message != null)
				deleteMessage(nativePtr, ((LinphoneChatMessageImpl)message).getNativePtr());
		}
	}

	@Override
	public LinphoneChatMessage createLinphoneChatMessage(String message,
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
	private LinphoneChatMessage[] getHistoryPrivate(long[] typesPtr) {
		if (typesPtr == null) return null;

		LinphoneChatMessage[] messages = new LinphoneChatMessage[typesPtr.length];
		for (int i=0; i < messages.length; i++) {
			messages[i] = new LinphoneChatMessageImpl(typesPtr[i]);
		}

		return messages;
	}
	
	private native long createFileTransferMessage(long ptr, String name, String type, String subtype, int size);
	@Override
	public LinphoneChatMessage createFileTransferMessage(LinphoneContent content) {
		synchronized(getCore()) {
			return new LinphoneChatMessageImpl(createFileTransferMessage(nativePtr, content.getName(), content.getType(), content.getSubtype(), content.getRealSize()));
		}
	}
	@Override
	public void sendChatMessage(LinphoneChatMessage message) {
		sendChatMessage(nativePtr, message, ((LinphoneChatMessageImpl)message).getNativePtr());
	}
	
	private native Object getCall(long nativePtr);
	@Override
	public LinphoneCall getCall() {
		return (LinphoneCall) getCall(nativePtr);
	}
	
	private native long getChar(long nativePtr);
	@Override
	public long getChar() {
		return getChar(nativePtr);
	}
}
