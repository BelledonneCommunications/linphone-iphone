/*
 * chat-message.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "db/events-db.h"
#include "object/object-p.h"

#include "linphone/types.h"
#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "c-wrapper/c-wrapper.h"

#include "chat-message-p.h"
#include "chat-message.h"
#include "content/content.h"

#include "modifier/multipart-chat-message-modifier.h"
#include "modifier/cpim-chat-message-modifier.h"
#include "chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

using namespace std;

// -----------------------------------------------------------------------------

ChatMessagePrivate::ChatMessagePrivate (const std::shared_ptr<ChatRoom> &room)
: chatRoom(room) {}

ChatMessagePrivate::~ChatMessagePrivate () {}

// -----------------------------------------------------------------------------

ChatMessage::ChatMessage (const std::shared_ptr<ChatRoom> &room) : Object(*new ChatMessagePrivate(room)) {}

ChatMessage::ChatMessage (ChatMessagePrivate &p) : Object(p) {}

LinphoneChatMessage * ChatMessage::getBackPtr() {
	return L_GET_C_BACK_PTR(this);
}

shared_ptr<ChatRoom> ChatMessage::getChatRoom () const {
	L_D(const ChatMessage);
	return d->chatRoom;
}

// -----------------------------------------------------------------------------

std::string ChatMessage::getExternalBodyUrl() const {
	L_D(const ChatMessage);
	return d->externalBodyUrl;
}

void ChatMessage::setExternalBodyUrl(const string &url) {
	L_D(ChatMessage);
	d->externalBodyUrl = url;
}

time_t ChatMessage::getTime () const {
	L_D(const ChatMessage);
	return d->time;
}
void ChatMessage::setTime(time_t time) {
	L_D(ChatMessage);
	d->time = time;
}

bool ChatMessage::isSecured () const {
	L_D(const ChatMessage);
	return d->isSecured;
}

void ChatMessage::setIsSecured(bool isSecured) {
	L_D(ChatMessage);
	d->isSecured = isSecured;
}

ChatMessage::Direction ChatMessage::getDirection () const {
	L_D(const ChatMessage);
	return d->direction;
}

void ChatMessage::setDirection (ChatMessage::Direction dir) {
	L_D(ChatMessage);
	d->direction = dir;
}

bool ChatMessage::isOutgoing () const {
	L_D(const ChatMessage);
	return d->direction == Outgoing;
}

bool ChatMessage::isIncoming () const {
	L_D(const ChatMessage);
	return d->direction == Incoming;
}

ChatMessage::State ChatMessage::getState() const {
	L_D(const ChatMessage);
	return d->state;
}

void ChatMessage::setState(State state) {
	L_D(ChatMessage);
	if (state != d->state && d->chatRoom) {
		if (((d->state == Displayed) || (d->state == DeliveredToUser))
			&& ((state == DeliveredToUser) || (state == Delivered) || (state == NotDelivered))) {
			return;
		}
		/* TODO
		ms_message("Chat message %p: moving from state %s to %s", msg, linphone_chat_message_state_to_string(msg->state), linphone_chat_message_state_to_string(state));
		*/
		d->state = state;
		
		LinphoneChatMessage *msg = L_GET_C_BACK_PTR(this);
		/* TODO
		if (msg->message_state_changed_cb) {
			msg->message_state_changed_cb(msg, msg->state, msg->message_state_changed_user_data);
		}*/
		LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
		if (linphone_chat_message_cbs_get_msg_state_changed(cbs)) {
			linphone_chat_message_cbs_get_msg_state_changed(cbs)(msg, linphone_chat_message_get_state(msg));
		}
	}
}

string ChatMessage::getId () const {
	L_D(const ChatMessage);
	return d->id;
}

void ChatMessage::setId (string id) {
	L_D(ChatMessage);
	d->id = id;
}

bool ChatMessage::isRead() const {
	//L_D(const ChatMessage);
	return false;
	/*shared_ptr<ImNotifyPolicy> policy =d->chatRoom->core->getImNotifPolicy();
	if (policy->getRecvImdnDisplayed() && d->state == Displayed) return true;
	if (policy->getRecvImdnDelivered() && (d->state == DeliveredToUser || d->state == Displayed)) return true;
	return d->state == Delivered || d->state == Displayed || d->state == DeliveredToUser;*/
}

// -----------------------------------------------------------------------------

string ChatMessage::getContentType() const {
	L_D(const ChatMessage);
	if (d->internalContent) {
		return d->internalContent->getContentType().asString();
	}
	if (d->contents.size() > 0) {
		return d->contents.front()->getContentType().asString();
	}
	return "";
}

string ChatMessage::getText() const {
	L_D(const ChatMessage);
	if (d->internalContent) {
		return d->internalContent->getBodyAsString();
	}
	if (d->contents.size() > 0) {
		return d->contents.front()->getBodyAsString();
	}
	return "";
}

unsigned int ChatMessage::getStorageId() const {
	L_D(const ChatMessage);
	return d->storageId;
}

void ChatMessage::setStorageId(unsigned int id) {
	L_D(ChatMessage);
	d->storageId = id;
}

string ChatMessage::getAppdata () const {
	L_D(const ChatMessage);
	return d->appData;
}

void ChatMessage::setAppdata (const string &appData) {
	L_D(ChatMessage);
	d->appData = appData;
	// TODO: store app data in db !
	// linphone_chat_message_store_appdata(msg);
}

// -----------------------------------------------------------------------------

shared_ptr<const Address> ChatMessage::getFromAddress () const {
	// TODO.
	return nullptr;
}

shared_ptr<const Address> ChatMessage::getToAddress () const {
	// TODO.
	return nullptr;
}

shared_ptr<const Address> ChatMessage::getLocalAddress () const {
	// TODO.
	return nullptr;
}

shared_ptr<const Address> ChatMessage::getRemoteAddress () const {
	// TODO.
	return nullptr;
}

shared_ptr<const ErrorInfo> ChatMessage::getErrorInfo () const {
	L_D(const ChatMessage);
	return d->errorInfo;
}

void ChatMessage::send () {
	L_D(ChatMessage);

	if (d->contents.size() > 1) {
		MultipartChatMessageModifier mcmm;
		mcmm.encode(d);
	}

	LinphoneCore *lc = getChatRoom()->getCore();
	LpConfig *lpc = linphone_core_get_config(lc);
	if (lp_config_get_int(lpc, "sip", "use_cpim", 0) == 1) {
		CpimChatMessageModifier ccmm;
		ccmm.encode(d);
	}

	// TODO.

	d->isReadOnly = true;
}

bool ChatMessage::containsReadableText () const {
	// TODO: Check content type.
	return true;
}

bool ChatMessage::isReadOnly () const {
	L_D(const ChatMessage);
	return d->isReadOnly;
}

list<shared_ptr<const Content> > ChatMessage::getContents () const {
	L_D(const ChatMessage);
	list<shared_ptr<const Content> > contents;
	for (const auto &content : d->contents)
		contents.push_back(content);
	return contents;
}

void ChatMessage::addContent (const shared_ptr<Content> &content) {
	L_D(ChatMessage);
	if (d->isReadOnly) return;

	d->contents.push_back(content);
}

void ChatMessage::removeContent (const shared_ptr<const Content> &content) {
	L_D(ChatMessage);
	if (d->isReadOnly) return;

	d->contents.remove(const_pointer_cast<Content>(content));
}

string ChatMessage::getCustomHeaderValue (const string &headerName) const {
	L_D(const ChatMessage);
	try {
		return d->customHeaders.at(headerName);
	} catch (const exception &) {
		// Key doesn't exist.
	}
	return "";
}

void ChatMessage::addCustomHeader (const string &headerName, const string &headerValue) {
	L_D(ChatMessage);
	if (d->isReadOnly) return;

	d->customHeaders[headerName] = headerValue;
}

void ChatMessage::removeCustomHeader (const string &headerName) {
	L_D(ChatMessage);
	if (d->isReadOnly) return;

	d->customHeaders.erase(headerName);
}

LINPHONE_END_NAMESPACE
