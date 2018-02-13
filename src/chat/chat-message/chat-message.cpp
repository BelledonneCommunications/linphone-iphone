/*
 * chat-message.cpp
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

#include "object/object-p.h"

#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "linphone/utils/utils.h"

#include "address/address.h"
#include "c-wrapper/c-wrapper.h"
#include "call/call-p.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/client-group-to-basic-chat-room.h"
#include "chat/chat-room/real-time-text-chat-room.h"
#include "chat/modifier/cpim-chat-message-modifier.h"
#include "chat/modifier/encryption-chat-message-modifier.h"
#include "chat/modifier/file-transfer-chat-message-modifier.h"
#include "chat/modifier/multipart-chat-message-modifier.h"
#include "content/file-content.h"
#include "content/content.h"
#include "core/core.h"
#include "core/core-p.h"
#include "logger/logger.h"
#include "chat/notification/imdn.h"

#include "ortp/b64.h"

// =============================================================================

using namespace std;

using namespace B64_NAMESPACE;

LINPHONE_BEGIN_NAMESPACE

void ChatMessagePrivate::setDirection (ChatMessage::Direction dir) {
	direction = dir;
}

void ChatMessagePrivate::setTime (time_t t) {
	time = t;
}

void ChatMessagePrivate::setIsReadOnly (bool readOnly) {
	isReadOnly = readOnly;
}

void ChatMessagePrivate::setState (ChatMessage::State s, bool force) {
	L_Q();

	if (force)
		state = s;

	if (s == state)
		return;

	if (
		(state == ChatMessage::State::Displayed || state == ChatMessage::State::DeliveredToUser) &&
		(
			s == ChatMessage::State::DeliveredToUser ||
			s == ChatMessage::State::Delivered ||
			s == ChatMessage::State::NotDelivered
		)
	)
		return;

	lInfo() << "Chat message " << this << ": moving from " << Utils::toString(state) <<
		" to " << Utils::toString(s);
	state = s;

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
	if (linphone_chat_message_get_message_state_changed_cb(msg))
		linphone_chat_message_get_message_state_changed_cb(msg)(
			msg,
			(LinphoneChatMessageState)state,
			linphone_chat_message_get_message_state_changed_cb_user_data(msg)
		);

	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	if (cbs && linphone_chat_message_cbs_get_msg_state_changed(cbs))
		linphone_chat_message_cbs_get_msg_state_changed(cbs)(msg, linphone_chat_message_get_state(msg));

	updateInDb();
}

belle_http_request_t *ChatMessagePrivate::getHttpRequest () const {
	return fileTransferChatMessageModifier.getHttpRequest();
}

void ChatMessagePrivate::setHttpRequest (belle_http_request_t *request) {
	fileTransferChatMessageModifier.setHttpRequest(request);
}

SalOp *ChatMessagePrivate::getSalOp () const {
	return salOp;
}

void ChatMessagePrivate::setSalOp (SalOp *op) {
	salOp = op;
}

SalCustomHeader *ChatMessagePrivate::getSalCustomHeaders () const {
	return salCustomHeaders;
}

void ChatMessagePrivate::setSalCustomHeaders (SalCustomHeader *headers) {
	salCustomHeaders = headers;
}

void ChatMessagePrivate::addSalCustomHeader (const string &name, const string &value) {
	salCustomHeaders = sal_custom_header_append(salCustomHeaders, name.c_str(), value.c_str());
}

void ChatMessagePrivate::removeSalCustomHeader (const string &name) {
	salCustomHeaders = sal_custom_header_remove(salCustomHeaders, name.c_str());
}

string ChatMessagePrivate::getSalCustomHeaderValue (const string &name) {
	return L_C_TO_STRING(sal_custom_header_find(salCustomHeaders, name.c_str()));
}

// -----------------------------------------------------------------------------
// Below methods are only for C API backward compatibility...
// -----------------------------------------------------------------------------

bool ChatMessagePrivate::hasTextContent() const {
	for (const Content *c : contents) {
		if (c->getContentType() == ContentType::PlainText) {
			return true;
		}
	}
	return false;
}

const Content* ChatMessagePrivate::getTextContent() const {
	for (const Content *c : contents) {
		if (c->getContentType() == ContentType::PlainText) {
			return c;
		}
	}
	return &Utils::getEmptyConstRefObject<Content>();
}

bool ChatMessagePrivate::hasFileTransferContent() const {
	for (const Content *c : contents) {
		if (c->getContentType() == ContentType::FileTransfer) {
			return true;
		}
	}
	return false;
}

const Content* ChatMessagePrivate::getFileTransferContent() const {
	for (const Content *c : contents) {
		if (c->getContentType() == ContentType::FileTransfer) {
			return c;
		}
	}
	return &Utils::getEmptyConstRefObject<Content>();
}

const string &ChatMessagePrivate::getFileTransferFilepath () const {
	return fileTransferFilePath;
}

void ChatMessagePrivate::setFileTransferFilepath (const string &path) {
	fileTransferFilePath = path;
}

const string &ChatMessagePrivate::getAppdata () const {
	for (const Content *c : contents) {
		if (c->isFile()) {
			FileContent *fileContent = (FileContent *)c;
			return fileContent->getAppData("legacy");
		}
	}
	return Utils::getEmptyConstRefObject<string>();
}

void ChatMessagePrivate::setAppdata (const string &data) {
	for (const Content *c : contents) {
		if (c->isFile()) {
			FileContent *fileContent = (FileContent *)c;
			fileContent->setAppData("legacy", data);
			break;
		}
	}
	updateInDb();
}

const string &ChatMessagePrivate::getExternalBodyUrl () const {
	if (hasFileTransferContent()) {
		FileTransferContent *content = (FileTransferContent*) getFileTransferContent();
		return content->getFileUrl();
	}
	return Utils::getEmptyConstRefObject<string>();
}

const ContentType &ChatMessagePrivate::getContentType () {
	if (direction == ChatMessage::Direction::Incoming) {
		if (contents.size() > 0) {
			Content *content = contents.front();
			cContentType = content->getContentType();
		} else {
			cContentType = internalContent.getContentType();
		}
	} else {
		if (internalContent.getContentType().isValid()) {
			cContentType = internalContent.getContentType();
		} else {
			if (contents.size() > 0) {
				Content *content = contents.front();
				cContentType = content->getContentType();
			}
		}
	}
	return cContentType;
}

void ChatMessagePrivate::setContentType (const ContentType &contentType) {
	if (contents.size() > 0 && internalContent.getContentType().isEmpty() && internalContent.isEmpty()) {
		internalContent.setBody(contents.front()->getBody());
	}
	internalContent.setContentType(contentType);

	if ((currentSendStep &ChatMessagePrivate::Step::Started) != ChatMessagePrivate::Step::Started) {
		// if not started yet the sending also alter the first content
		if (contents.size() > 0)
			contents.front()->setContentType(contentType);
	}
}

const string &ChatMessagePrivate::getText () {
	if (direction == ChatMessage::Direction::Incoming) {
		if (hasTextContent()) {
			cText = getTextContent()->getBodyAsString();
		} else if (contents.size() > 0) {
			Content *content = contents.front();
			cText = content->getBodyAsString();
		} else {
			cText = internalContent.getBodyAsString();
		}
	} else {
		if (!internalContent.isEmpty()) {
			cText = internalContent.getBodyAsString();
		} else {
			if (contents.size() > 0) {
				Content *content = contents.front();
				cText = content->getBodyAsString();
			}
		}
	}
	return cText;
}

void ChatMessagePrivate::setText (const string &text) {
	if (contents.size() > 0 && internalContent.getContentType().isEmpty() && internalContent.isEmpty()) {
		internalContent.setContentType(contents.front()->getContentType());
	}
	internalContent.setBody(text);

	if ((currentSendStep &ChatMessagePrivate::Step::Started) != ChatMessagePrivate::Step::Started) {
		// if not started yet the sending also alter the first content
		if (contents.size() > 0)
			contents.front()->setBody(text);
	}
}

LinphoneContent *ChatMessagePrivate::getFileTransferInformation () const {
	if (hasFileTransferContent()) {
		return getFileTransferContent()->toLinphoneContent();
	}
	for (const Content *c : contents) {
		if (c->isFile()) {
			FileContent *fileContent = (FileContent *)c;
			return fileContent->toLinphoneContent();
		}
	}
	return NULL;
}

void ChatMessagePrivate::setFileTransferInformation (const LinphoneContent *c_content) {
	L_Q();

	// Create a FileContent, it will create the FileTransferContent at upload time
	FileContent *fileContent = new FileContent();
	ContentType contentType(linphone_content_get_type(c_content), linphone_content_get_subtype(c_content));
	fileContent->setContentType(contentType);
	fileContent->setFileSize(linphone_content_get_size(c_content));
	fileContent->setFileName(linphone_content_get_name(c_content));
	if (linphone_content_get_string_buffer(c_content) != NULL) {
		fileContent->setBody(linphone_content_get_string_buffer(c_content));
	}

	q->addContent(*fileContent);
}

bool ChatMessagePrivate::downloadFile () {
	L_Q();

	for (auto &content : contents)
		if (content->getContentType() == ContentType::FileTransfer)
			return q->downloadFile(*static_cast<FileTransferContent *>(content));

	return false;
}

void ChatMessagePrivate::loadFileTransferUrlFromBodyToContent() {
	L_Q();
	int errorCode = 0;
	fileTransferChatMessageModifier.decode(q->getSharedFromThis(), errorCode);
}

void ChatMessagePrivate::setChatRoom (const shared_ptr<AbstractChatRoom> &cr) {
	chatRoom = cr;
	chatRoomId = cr->getChatRoomId();
	if (direction == ChatMessage::Direction::Outgoing) {
		fromAddress = chatRoomId.getLocalAddress();
		toAddress = chatRoomId.getPeerAddress();
	} else {
		fromAddress = chatRoomId.getPeerAddress();
		toAddress = chatRoomId.getLocalAddress();
	}
}

// -----------------------------------------------------------------------------

void ChatMessagePrivate::sendImdn (Imdn::Type imdnType, LinphoneReason reason) {
	L_Q();

	shared_ptr<ChatMessage> msg = q->getChatRoom()->createChatMessage();

	Content *content = new Content();
	content->setContentType("message/imdn+xml");
	content->setBody(Imdn::createXml(imdnId, time, imdnType, reason));
	msg->addContent(*content);

	if (reason != LinphoneReasonNone)
		msg->getPrivate()->setEncryptionPrevented(true);
	msg->setToBeStored(false);

	msg->getPrivate()->send();
}

static void forceUtf8Content (Content &content) {
	// TODO: Deal with other content type in the future.
	ContentType contentType = content.getContentType();
	if (contentType != ContentType::PlainText)
		return;

	string charset = contentType.getParameter();
	if (charset.empty())
		return;

	size_t n = charset.find("charset=");
	if (n == string::npos)
		return;

	L_BEGIN_LOG_EXCEPTION

	size_t begin = n + sizeof("charset");
	size_t end = charset.find(";", begin);
	charset = charset.substr(begin, end - begin);

	if (Utils::stringToLower(charset) != "utf-8") {
		string utf8Body = Utils::convertString(content.getBodyAsUtf8String(), charset, "UTF-8");
		if (!utf8Body.empty()) {
			// TODO: use move operator if possible in the future!
			content.setBodyFromUtf8(utf8Body);
			contentType.setParameter(string(contentType.getParameter()).replace(begin, end - begin, "UTF-8"));
			content.setContentType(contentType);
		}
	}

	L_END_LOG_EXCEPTION
}

void ChatMessagePrivate::notifyReceiving () {
	L_Q();

	if ((getContentType() == ContentType::Imdn) || (getContentType() == ContentType::ImIsComposing))
		return;

	LinphoneChatRoom *chatRoom = L_GET_C_BACK_PTR(q->getChatRoom());
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(chatRoom);
	LinphoneChatRoomCbsParticipantAddedCb cb = linphone_chat_room_cbs_get_chat_message_received(cbs);
	shared_ptr<ConferenceChatMessageEvent> event = make_shared<ConferenceChatMessageEvent>(
		::time(nullptr), q->getSharedFromThis()
	);
	if (cb)
		cb(chatRoom, L_GET_C_BACK_PTR(event));
	// Legacy
	q->getChatRoom()->getPrivate()->notifyChatMessageReceived(q->getSharedFromThis());

	if (toBeStored)
		storeInDb();

	q->sendDeliveryNotification(LinphoneReasonNone);
}

LinphoneReason ChatMessagePrivate::receive () {
	L_Q();
	int errorCode = 0;
	LinphoneReason reason = LinphoneReasonNone;

	shared_ptr<Core> core = q->getCore();
	shared_ptr<AbstractChatRoom> chatRoom = q->getChatRoom();

	// ---------------------------------------
	// Start of message modification
	// ---------------------------------------

	if ((currentRecvStep &ChatMessagePrivate::Step::Encryption) == ChatMessagePrivate::Step::Encryption) {
		lInfo() << "Encryption step already done, skipping";
	} else {
		EncryptionChatMessageModifier ecmm;
		ChatMessageModifier::Result result = ecmm.decode(q->getSharedFromThis(), errorCode);
		if (result == ChatMessageModifier::Result::Error) {
			/* Unable to decrypt message */
			chatRoom->getPrivate()->notifyUndecryptableChatMessageReceived(q->getSharedFromThis());
			reason = linphone_error_code_to_reason(errorCode);
			q->sendDeliveryNotification(reason);
			return reason;
		} else if (result == ChatMessageModifier::Result::Suspended) {
			currentRecvStep |= ChatMessagePrivate::Step::Encryption;
			return LinphoneReasonNone;
		}
		currentRecvStep |= ChatMessagePrivate::Step::Encryption;
	}

	if ((currentRecvStep &ChatMessagePrivate::Step::Cpim) == ChatMessagePrivate::Step::Cpim) {
		lInfo() << "Cpim step already done, skipping";
	} else {
		if (internalContent.getContentType() == ContentType::Cpim) {
			CpimChatMessageModifier ccmm;
			ccmm.decode(q->getSharedFromThis(), errorCode);
		}
		currentRecvStep |= ChatMessagePrivate::Step::Cpim;
	}

	if ((currentRecvStep &ChatMessagePrivate::Step::Multipart) == ChatMessagePrivate::Step::Multipart) {
		lInfo() << "Multipart step already done, skipping";
	} else {
		MultipartChatMessageModifier mcmm;
		mcmm.decode(q->getSharedFromThis(), errorCode);
		currentRecvStep |= ChatMessagePrivate::Step::Multipart;
	}

	if ((currentRecvStep & ChatMessagePrivate::Step::FileUpload) == ChatMessagePrivate::Step::FileUpload) {
		lInfo() << "File download step already done, skipping";
	} else {
		// This will check if internal content is FileTransfer and make the appropriate changes
		loadFileTransferUrlFromBodyToContent();
		currentRecvStep |= ChatMessagePrivate::Step::FileUpload;
	}

	if (contents.size() == 0) {
		// All previous modifiers only altered the internal content, let's fill the content list
		contents.push_back(new Content(internalContent));
	}

	for (auto &content : contents)
		forceUtf8Content(*content);

	// ---------------------------------------
	// End of message modification
	// ---------------------------------------

	// Remove internal content as it is not needed anymore and will confuse some old methods like getText()
	internalContent.setBody("");
	internalContent.setContentType(ContentType(""));
	// Also remove current step so we go through all modifiers if message is re-sent
	currentRecvStep = ChatMessagePrivate::Step::None;

	setState(ChatMessage::State::Delivered);

	if (errorCode <= 0) {
		bool foundSupportContentType = false;
		for (Content *c : contents) {
			if (linphone_core_is_content_type_supported(core->getCCore(), c->getContentType().asString().c_str())) {
				foundSupportContentType = true;
				break;
			} else
			lError() << "Unsupported content-type: " << c->getContentType().asString();
		}

		if (!foundSupportContentType) {
			errorCode = 415;
			lError() << "No content-type in the contents list is supported...";
		}
	}

	// Check if this is in fact an outgoing message (case where this is a message sent by us from an other device).
	Address me(linphone_core_get_identity(core->getCCore()));
	if (me.weakEqual(q->getFromAddress()))
		setDirection(ChatMessage::Direction::Outgoing);

	// Check if this is a duplicate message.
	if (chatRoom->findChatMessage(imdnId, direction))
		return core->getCCore()->chat_deny_code;

	if (errorCode > 0) {
		reason = linphone_error_code_to_reason(errorCode);
		q->sendDeliveryNotification(reason);
		return reason;
	}

	if ((getContentType() == ContentType::ImIsComposing) || (getContentType() == ContentType::Imdn))
		toBeStored = false;

	return reason;
}

void ChatMessagePrivate::send () {
	L_Q();
	SalOp *op = salOp;
	LinphoneCall *lcall = nullptr;
	int errorCode = 0;

	currentSendStep |= ChatMessagePrivate::Step::Started;

	if (toBeStored)
		storeInDb();

	if ((currentSendStep & ChatMessagePrivate::Step::FileUpload) == ChatMessagePrivate::Step::FileUpload) {
		lInfo() << "File upload step already done, skipping";
	} else {
		ChatMessageModifier::Result result = fileTransferChatMessageModifier.encode(q->getSharedFromThis(), errorCode);
		if (result == ChatMessageModifier::Result::Error) {
			setState(ChatMessage::State::NotDelivered);
			return;
		} else if (result == ChatMessageModifier::Result::Suspended) {
			setState(ChatMessage::State::InProgress);
			return;
		}
		currentSendStep |= ChatMessagePrivate::Step::FileUpload;
	}

	shared_ptr<Core> core = q->getCore();
	if (lp_config_get_int(core->getCCore()->config, "sip", "chat_use_call_dialogs", 0) != 0) {
		lcall = linphone_core_get_call_by_remote_address(core->getCCore(), q->getToAddress().asString().c_str());
		if (lcall) {
			shared_ptr<Call> call = L_GET_CPP_PTR_FROM_C_OBJECT(lcall);
			if ((call->getState() == CallSession::State::Connected)
				|| (call->getState() == CallSession::State::StreamsRunning)
				|| (call->getState() == CallSession::State::Paused)
				|| (call->getState() == CallSession::State::Pausing)
				|| (call->getState() == CallSession::State::PausedByRemote)
			) {
				lInfo() << "send SIP msg through the existing call.";
				op = linphone_call_get_op(lcall);
				string identity = linphone_core_find_best_identity(core->getCCore(), linphone_call_get_remote_address(lcall));
				if (identity.empty()) {
					LinphoneAddress *addr = linphone_address_new(q->getToAddress().asString().c_str());
					LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core->getCCore(), addr);
					if (proxy) {
						identity = L_GET_CPP_PTR_FROM_C_OBJECT(linphone_proxy_config_get_identity_address(proxy))->asString();
					} else {
						identity = linphone_core_get_primary_contact(core->getCCore());
					}
					linphone_address_unref(addr);
				}
			}
		}
	}

	if (!op) {
		LinphoneAddress *peer = linphone_address_new(q->getToAddress().asString().c_str());
		/* Sending out of call */
		salOp = op = new SalMessageOp(core->getCCore()->sal);
		linphone_configure_op(
			core->getCCore(), op, peer, getSalCustomHeaders(),
			!!lp_config_get_int(core->getCCore()->config, "sip", "chat_msg_with_contact", 0)
		);
		op->set_user_pointer(q);     /* If out of call, directly store msg */
		linphone_address_unref(peer);
	}
	op->set_from(q->getFromAddress().asString().c_str());
	op->set_to(q->getToAddress().asString().c_str());

	// ---------------------------------------
	// Start of message modification
	// ---------------------------------------

	if (applyModifiers) {
		// Do not multipart or encapsulate with CPIM in an old ChatRoom to maintain backward compatibility
		if (q->getChatRoom()->canHandleMultipart()) {
			if ((currentSendStep &ChatMessagePrivate::Step::Multipart) == ChatMessagePrivate::Step::Multipart) {
				lInfo() << "Multipart step already done, skipping";
			} else {
				if (contents.size() > 1) {
					MultipartChatMessageModifier mcmm;
					mcmm.encode(q->getSharedFromThis(), errorCode);
				}
				currentSendStep |= ChatMessagePrivate::Step::Multipart;
			}
		}

		if (q->getChatRoom()->canHandleCpim()) {
			if ((currentSendStep &ChatMessagePrivate::Step::Cpim) == ChatMessagePrivate::Step::Cpim) {
				lInfo() << "Cpim step already done, skipping";
			} else {
				int defaultValue = !!lp_config_get_string(core->getCCore()->config, "misc", "conference_factory_uri", nullptr);
				if (lp_config_get_int(core->getCCore()->config, "sip", "use_cpim", defaultValue) == 1) {
					CpimChatMessageModifier ccmm;
					ccmm.encode(q->getSharedFromThis(), errorCode);
				}
				currentSendStep |= ChatMessagePrivate::Step::Cpim;
			}
		}

		if ((currentSendStep &ChatMessagePrivate::Step::Encryption) == ChatMessagePrivate::Step::Encryption) {
			lInfo() << "Encryption step already done, skipping";
		} else {
			if (!encryptionPrevented) {
				EncryptionChatMessageModifier ecmm;
				ChatMessageModifier::Result result = ecmm.encode(q->getSharedFromThis(), errorCode);
				if (result == ChatMessageModifier::Result::Error) {
					sal_error_info_set((SalErrorInfo *)op->get_error_info(), SalReasonNotAcceptable, "SIP", errorCode, "Unable to encrypt IM", nullptr);
					setState(ChatMessage::State::NotDelivered);
					return;
				} else if (result == ChatMessageModifier::Result::Suspended) {
					currentSendStep |= ChatMessagePrivate::Step::Encryption;
					return;
				}
			}
			currentSendStep |= ChatMessagePrivate::Step::Encryption;
		}
	}

	// ---------------------------------------
	// End of message modification
	// ---------------------------------------

	if (internalContent.isEmpty()) {
		if (contents.size() > 0) {
			internalContent = *(contents.front());
		} else {
			lError() << "Trying to send a message without any content !";
			return;
		}
	}

	auto msgOp = dynamic_cast<SalMessageOpInterface *>(op);
	if (internalContent.getContentType().isValid()) {
		msgOp->send_message(internalContent.getContentType().asString().c_str(), internalContent.getBodyAsUtf8String().c_str());
	} else {
		msgOp->send_message(ContentType::PlainText.asString().c_str(), internalContent.getBodyAsUtf8String().c_str());
	}

	// Restore FileContents and remove FileTransferContents
	list<Content*>::iterator it = contents.begin();
	while (it != contents.end()) {
		Content *content = *it;
		if (content->getContentType() == ContentType::FileTransfer) {
			FileTransferContent *fileTransferContent = (FileTransferContent *)content;
			it = contents.erase(it);
			q->addContent(*fileTransferContent->getFileContent());
			delete fileTransferContent;
		} else {
			it++;
		}
	}

	// Remove internal content as it is not needed anymore and will confuse some old methods like getContentType()
	internalContent.setBody("");
	internalContent.setContentType(ContentType(""));
	// Also remove current step so we go through all modifiers if message is re-sent
	currentSendStep = ChatMessagePrivate::Step::None;

	if (imdnId.empty())
		setImdnMessageId(op->get_call_id());   /* must be known at that time */

	if (lcall && linphone_call_get_op(lcall) == op) {
		/* In this case, chat delivery status is not notified, so unrefing chat message right now */
		/* Might be better fixed by delivering status, but too costly for now */
		return;
	}

	/* If operation failed, we should not change message state */
	if (direction == ChatMessage::Direction::Outgoing) {
		setIsReadOnly(true);
		setState(ChatMessage::State::InProgress);
	}
}

void ChatMessagePrivate::storeInDb () {
	L_Q();

	// TODO: store message in the future
	if (linphone_core_conference_server_enabled(q->getCore()->getCCore())) return;

	if (dbKey.isValid()) {
		updateInDb();
	} else {
		shared_ptr<EventLog> eventLog = make_shared<ConferenceChatMessageEvent>(time, q->getSharedFromThis());
		q->getChatRoom()->getPrivate()->addEvent(eventLog);

		if (direction == ChatMessage::Direction::Incoming) {
			if (hasFileTransferContent()) {
				// Keep the event in the transient list, message storage can be updated in near future
				q->getChatRoom()->getPrivate()->addTransientEvent(eventLog);
			}
		} else {
			// Keep event in transient to be able to store in database state changes
			q->getChatRoom()->getPrivate()->addTransientEvent(eventLog);
		}
	}
}

void ChatMessagePrivate::updateInDb () {
	L_Q();

	if (!dbKey.isValid())
		return;

	unique_ptr<MainDb> &mainDb = q->getChatRoom()->getCore()->getPrivate()->mainDb;
	shared_ptr<EventLog> eventLog = mainDb->getEventFromKey(dbKey);
	mainDb->updateEvent(eventLog);

	if (direction == ChatMessage::Direction::Incoming) {
		if (!hasFileTransferContent()) {
			// Incoming message doesn't have any download waiting anymore, we can remove it's event from the transients
			q->getChatRoom()->getPrivate()->removeTransientEvent(eventLog);
		}
	} else {
		if (state == ChatMessage::State::Delivered || state == ChatMessage::State::NotDelivered) {
			// Once message has reached this state it won't change anymore so we can remove the event from the transients
			q->getChatRoom()->getPrivate()->removeTransientEvent(eventLog);
		}
	}
}

// -----------------------------------------------------------------------------

ChatMessage::ChatMessage (const shared_ptr<AbstractChatRoom> &chatRoom, ChatMessage::Direction direction) :
	Object(*new ChatMessagePrivate), CoreAccessor(chatRoom->getCore()) {
	L_D();

	d->direction = direction;
	d->setChatRoom(chatRoom);
}

ChatMessage::~ChatMessage () {
	L_D();
	for (Content *content : d->contents)
		delete content;

	if (d->salOp) {
		d->salOp->set_user_pointer(nullptr);
		d->salOp->unref();
	}
	if (d->salCustomHeaders)
		sal_custom_header_unref(d->salCustomHeaders);
}

shared_ptr<AbstractChatRoom> ChatMessage::getChatRoom () const {
	L_D();

	shared_ptr<AbstractChatRoom> chatRoom = d->chatRoom.lock();
	if (!chatRoom) {
		chatRoom = getCore()->getOrCreateBasicChatRoom(d->chatRoomId);
		if (!chatRoom) {
			lError() << "Unable to get valid chat room instance.";
			throw logic_error("Unable to get chat room of chat message.");
		}
	}

	return chatRoom;
}

// -----------------------------------------------------------------------------

time_t ChatMessage::getTime () const {
	L_D();
	return d->time;
}

bool ChatMessage::isSecured () const {
	L_D();
	return d->isSecured;
}

void ChatMessage::setIsSecured (bool isSecured) {
	L_D();
	d->isSecured = isSecured;
}

ChatMessage::Direction ChatMessage::getDirection () const {
	L_D();
	return d->direction;
}

ChatMessage::State ChatMessage::getState () const {
	L_D();
	return d->state;
}

const string &ChatMessage::getImdnMessageId () const {
	L_D();
	return d->imdnId;
}

void ChatMessagePrivate::setImdnMessageId (const string &id) {
	imdnId = id;
}

bool ChatMessage::isRead () const {
	L_D();

	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(getCore()->getCCore());
	if (linphone_im_notif_policy_get_recv_imdn_displayed(policy) && d->state == State::Displayed)
		return true;

	if (
		linphone_im_notif_policy_get_recv_imdn_delivered(policy) &&
		(d->state == State::DeliveredToUser || d->state == State::Displayed)
	)
		return true;

	return d->state == State::Delivered || d->state == State::Displayed || d->state == State::DeliveredToUser;
}

const IdentityAddress &ChatMessage::getFromAddress () const {
	L_D();
	return d->fromAddress;
}

const IdentityAddress &ChatMessage::getToAddress () const {
	L_D();
	return d->toAddress;
}

bool ChatMessage::getToBeStored () const {
	L_D();
	return d->toBeStored;
}

void ChatMessage::setToBeStored (bool value) {
	L_D();
	d->toBeStored = value;
}

// -----------------------------------------------------------------------------

const LinphoneErrorInfo *ChatMessage::getErrorInfo () const {
	L_D();
	if (!d->errorInfo) d->errorInfo = linphone_error_info_new();   // let's do it mutable
	linphone_error_info_from_sal_op(d->errorInfo, d->salOp);
	return d->errorInfo;
}

bool ChatMessage::isReadOnly () const {
	L_D();
	return d->isReadOnly;
}

const list<Content *> &ChatMessage::getContents () const {
	L_D();
	return d->contents;
}

void ChatMessage::addContent (Content &content) {
	L_D();
	if (d->isReadOnly) return;

	d->contents.push_back(&content);
}

void ChatMessage::removeContent (const Content &content) {
	L_D();
	if (d->isReadOnly) return;

	d->contents.remove(&const_cast<Content &>(content));
}

const Content &ChatMessage::getInternalContent () const {
	L_D();
	return d->internalContent;
}

void ChatMessage::setInternalContent (const Content &content) {
	L_D();
	d->internalContent = content;
}

string ChatMessage::getCustomHeaderValue (const string &headerName) const {
	L_D();
	try {
		return d->customHeaders.at(headerName);
	} catch (const exception &) {
		// Key doesn't exist.
	}
	return nullptr;
}

void ChatMessage::addCustomHeader (const string &headerName, const string &headerValue) {
	L_D();
	if (d->isReadOnly) return;

	d->customHeaders[headerName] = headerValue;
}

void ChatMessage::removeCustomHeader (const string &headerName) {
	L_D();
	if (d->isReadOnly) return;

	d->customHeaders.erase(headerName);
}

void ChatMessage::updateState (State state) {
	L_D();

	d->setState(state);
}

void ChatMessage::send () {
	L_D();

	// Do not allow sending a message that is already being sent or that has been correctly delivered/displayed
	if ((d->state == State::InProgress) || (d->state == State::Delivered) || (d->state == State::FileTransferDone) ||
			(d->state == State::DeliveredToUser) || (d->state == State::Displayed)) {
		lWarning() << "Cannot send chat message in state " << linphone_chat_message_state_to_string((LinphoneChatMessageState)d->state);
		return;
	}

	getChatRoom()->getPrivate()->sendChatMessage(getSharedFromThis());
}

void ChatMessage::sendDeliveryNotification (LinphoneReason reason) {
	L_D();

	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(getCore()->getCCore());
	if (linphone_im_notif_policy_get_send_imdn_delivered(policy))
		d->sendImdn(Imdn::Type::Delivery, reason);
}

void ChatMessage::sendDisplayNotification () {
	L_D();

	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(getCore()->getCCore());
	if (linphone_im_notif_policy_get_send_imdn_displayed(policy))
		d->sendImdn(Imdn::Type::Display, LinphoneReasonNone);
}

bool ChatMessage::downloadFile(FileTransferContent &fileTransferContent) {
	L_D();
	return d->fileTransferChatMessageModifier.downloadFile(getSharedFromThis(), &fileTransferContent);
}

void ChatMessage::cancelFileTransfer () {
	L_D();
	if (d->fileTransferChatMessageModifier.isFileTransferInProgressAndValid()) {
		if (d->state == State::InProgress) {
			d->setState(State::NotDelivered);
		}
		d->fileTransferChatMessageModifier.cancelFileTransfer();
	} else {
		lInfo() << "No existing file transfer - nothing to cancel";
	}
}

int ChatMessage::putCharacter (uint32_t character) {
	L_D();

	static const uint32_t new_line = 0x2028;
	static const uint32_t crlf = 0x0D0A;
	static const uint32_t lf = 0x0A;

	shared_ptr<AbstractChatRoom> chatRoom = getChatRoom();
	if (!(chatRoom->getCapabilities() & LinphonePrivate::ChatRoom::Capabilities::RealTimeText))
		return -1;

	shared_ptr<LinphonePrivate::RealTimeTextChatRoom> rttcr =
		static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(chatRoom);
	if (!rttcr)
		return -1;

	shared_ptr<Call> call = rttcr->getCall();
	if (!call || !call->getPrivate()->getMediaStream(LinphoneStreamTypeText))
		return -1;

	if (character == new_line || character == crlf || character == lf) {
		shared_ptr<Core> core = getCore();
		if (lp_config_get_int(core->getCCore()->config, "misc", "store_rtt_messages", 1) == 1) {
			// TODO: History.
			lDebug() << "New line sent, forge a message with content " << d->rttMessage.c_str();
			d->setTime(ms_time(0));
			d->state = State::Displayed;
			// d->direction = Direction::Outgoing;
			// setFromAddress(Address(
			// 	linphone_address_as_string(linphone_address_new(linphone_core_get_identity(core->getCCore())))
			// ));
			// linphone_chat_message_store(L_GET_C_BACK_PTR(this));
			d->rttMessage = "";
		}
	} else {
		char *value = LinphonePrivate::Utils::utf8ToChar(character);
		d->rttMessage = d->rttMessage + string(value);
		lDebug() << "Sent RTT character: " << value << "(" << (unsigned long)character <<
			"), pending text is " << d->rttMessage.c_str();
		delete[] value;
	}

	text_stream_putchar32(
		reinterpret_cast<TextStream *>(call->getPrivate()->getMediaStream(LinphoneStreamTypeText)),
		character
	);
	return 0;
}

LINPHONE_END_NAMESPACE
