/*
 * core-p.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _L_CORE_P_H_
#define _L_CORE_P_H_

#include "chat/chat-room/abstract-chat-room.h"
#include "core.h"
#include "db/main-db.h"
#include "object/object-p.h"
#include "sal/call-op.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CoreListener;

class CorePrivate : public ObjectPrivate {
public:
	void init ();
	void registerListener (CoreListener *listener);
	void unregisterListener (CoreListener *listener);
	void uninit ();

	void notifyNetworkReachable (bool sipNetworkReachable, bool mediaNetworkReachable);
	void notifyRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const std::string &message);

	int addCall (const std::shared_ptr<Call> &call);
	bool canWeAddCall () const;
	bool hasCalls () const { return !calls.empty(); }
	bool inviteReplacesABrokenCall (SalCallOp *op);
	bool isAlreadyInCallWithAddress (const Address &addr) const;
	void iterateCalls (time_t currentRealTime, bool oneSecondElapsed) const;
	void notifySoundcardUsage (bool used);
	int removeCall (const std::shared_ptr<Call> &call);
	void setCurrentCall (const std::shared_ptr<Call> &call) { currentCall = call; }
	void unsetVideoWindowId (bool preview, void *id);

	void parameterizeEqualizer (AudioStream *stream);
	void postConfigureAudioStream (AudioStream *stream, bool muted);
	void setPlaybackGainDb (AudioStream *stream, float gain);

	void loadChatRooms ();
	void insertChatRoom (const std::shared_ptr<AbstractChatRoom> &chatRoom);
	void insertChatRoomWithDb (const std::shared_ptr<AbstractChatRoom> &chatRoom);
	std::shared_ptr<AbstractChatRoom> createBasicChatRoom (const ChatRoomId &chatRoomId, AbstractChatRoom::CapabilitiesMask capabilities);
	std::shared_ptr<AbstractChatRoom> createClientGroupChatRoom (const std::string &subject, const std::string &uri = "", bool fallback = true);
	void replaceChatRoom (const std::shared_ptr<AbstractChatRoom> &replacedChatRoom, const std::shared_ptr<AbstractChatRoom> &newChatRoom);

	std::unique_ptr<MainDb> mainDb;

private:
	std::list<CoreListener *> listeners;

	std::list<std::shared_ptr<Call>> calls;
	std::shared_ptr<Call> currentCall;

	std::list<std::shared_ptr<AbstractChatRoom>> chatRooms;

	std::unordered_map<ChatRoomId, std::shared_ptr<AbstractChatRoom>> chatRoomsById;

	// Ugly cache to deal with C code.
	std::unordered_map<const AbstractChatRoom *, std::shared_ptr<const AbstractChatRoom>> noCreatedClientGroupChatRooms;

	L_DECLARE_PUBLIC(Core);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CORE_P_H_
