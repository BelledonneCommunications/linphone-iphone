/*
 * chat-message.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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
#include "c-wrapper/c-wrapper.h"
#include "address/address.h"

#include "chat/chat-message/chat-message-p.h"

#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/real-time-text-chat-room.h"
#include "chat/modifier/cpim-chat-message-modifier.h"
#include "chat/modifier/encryption-chat-message-modifier.h"
#include "chat/modifier/file-transfer-chat-message-modifier.h"
#include "chat/modifier/multipart-chat-message-modifier.h"
#include "content/file-content.h"
#include "content/content.h"
#include "core/core.h"
#include "logger/logger.h"

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

	if (s == state || !q->getChatRoom())
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

	lInfo() << "Chat message " << this << ": moving from state " <<
		linphone_chat_message_state_to_string((LinphoneChatMessageState)state) << " to " <<
		linphone_chat_message_state_to_string((LinphoneChatMessageState)s);
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
}

unsigned int ChatMessagePrivate::getStorageId () const {
	return storageId;
}

void ChatMessagePrivate::setStorageId (unsigned int id) {
	storageId = id;
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
	return &Content::Empty;
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
	return &Content::Empty;
}

const string &ChatMessagePrivate::getFileTransferFilepath () const {
	return fileTransferFilePath;
}

void ChatMessagePrivate::setFileTransferFilepath (const string &path) {
	fileTransferFilePath = path;
}

const string &ChatMessagePrivate::getAppdata () const {
	for (const Content *c : contents) {
		if (c->getContentType().isFile()) {
			FileContent *fileContent = (FileContent *)c;
			return fileContent->getFilePath();
		}
	}
	return Utils::getEmptyConstRefObject<string>();
}

void ChatMessagePrivate::setAppdata (const string &data) {
	for (const Content *c : contents) {
		if (c->getContentType().isFile()) {
			FileContent *fileContent = (FileContent *)c;
			return fileContent->setFilePath(data);
		}
	}
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
	internalContent.setContentType(contentType);
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
		if (hasTextContent()) {
			cText = getTextContent()->getBodyAsString();
		} else if (!internalContent.isEmpty()) {
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
	internalContent.setBody(text);
}

LinphoneContent *ChatMessagePrivate::getFileTransferInformation () const {
	if (hasFileTransferContent()) {
		return getFileTransferContent()->toLinphoneContent();
	}
	for (const Content *c : contents) {
		if (c->getContentType().isFile()) {
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

// -----------------------------------------------------------------------------

string ChatMessagePrivate::createImdnXml (Imdn::Type imdnType, LinphoneReason reason) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	string content;
	char *datetime = nullptr;

	// Check that the chat message has a message id.
	if (id.empty()) return nullptr;

	buf = xmlBufferCreate();
	if (buf == nullptr) {
		lError() << "Error creating the XML buffer";
		return content;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == nullptr) {
		lError() << "Error creating the XML writer";
		return content;
	}

	datetime = linphone_timestamp_to_rfc3339_string(time);
	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", nullptr);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, nullptr, (const xmlChar *)"imdn",
				(const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"linphoneimdn", nullptr, (const xmlChar *)"http://www.linphone.org/xsds/imdn.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"message-id", (const xmlChar *)id.c_str());
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"datetime", (const xmlChar *)datetime);
	}
	if (err >= 0) {
		if (imdnType == Imdn::Type::Delivery) {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivery-notification");
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"display-notification");
		}
	}
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"status");
	}
	if (err >= 0) {
		if (reason == LinphoneReasonNone) {
			if (imdnType == Imdn::Type::Delivery) {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivered");
			} else {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"displayed");
			}
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"error");
		}
	}
	if (err >= 0) {
		// Close the "delivered", "displayed" or "error" element.
		err = xmlTextWriterEndElement(writer);
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"linphoneimdn", (const xmlChar *)"reason", nullptr);
		if (err >= 0) {
			char codestr[16];
			snprintf(codestr, 16, "%d", linphone_reason_to_error_code(reason));
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"code", (const xmlChar *)codestr);
		}
		if (err >= 0) {
			err = xmlTextWriterWriteString(writer, (const xmlChar *)linphone_reason_to_string(reason));
		}
		if (err >= 0) {
			err = xmlTextWriterEndElement(writer);
		}
	}
	if (err >= 0) {
		// Close the "status" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		// Close the "delivery-notification" or "display-notification" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		// Close the "imdn" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		// xmlTextWriterEndDocument returns the size of the content.
		content = string((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	ms_free(datetime);
	return content;
}

void ChatMessagePrivate::sendImdn (Imdn::Type imdnType, LinphoneReason reason) {
	// FIXME: Add impl.
	// L_Q();
	// q->getChatRoom()->getPrivate()->sendImdn(createImdnXml(imdnType, reason), reason);
}

LinphoneReason ChatMessagePrivate::receive () {
	L_Q();
	int errorCode = 0;
	LinphoneReason reason = LinphoneReasonNone;

	shared_ptr<Core> core = q->getCore();
	shared_ptr<ChatRoom> chatRoom = q->getChatRoom();

	// ---------------------------------------
	// Start of message modification
	// ---------------------------------------


	if ((currentRecvStep &ChatMessagePrivate::Step::Cpim) == ChatMessagePrivate::Step::Cpim) {
		lInfo() << "Cpim step already done, skipping";
	} else {
		if (internalContent.getContentType() == ContentType::Cpim) {
			CpimChatMessageModifier ccmm;
			ccmm.decode(q->getSharedFromThis(), errorCode);
		}
		currentRecvStep |= ChatMessagePrivate::Step::Cpim;
	}

	if ((currentRecvStep &ChatMessagePrivate::Step::Encryption) == ChatMessagePrivate::Step::Encryption) {
		lInfo() << "Encryption step already done, skipping";
	} else {
		EncryptionChatMessageModifier ecmm;
		ChatMessageModifier::Result result = ecmm.decode(q->getSharedFromThis(), errorCode);
		if (result == ChatMessageModifier::Result::Error) {
			/* Unable to decrypt message */
			if (chatRoom)
				chatRoom->getPrivate()->notifyUndecryptableMessageReceived(q->getSharedFromThis());
			reason = linphone_error_code_to_reason(errorCode);
			q->sendDeliveryNotification(reason);
			return reason;
		} else if (result == ChatMessageModifier::Result::Suspended) {
			currentRecvStep |= ChatMessagePrivate::Step::Encryption;
			return LinphoneReasonNone;
		}
		currentRecvStep |= ChatMessagePrivate::Step::Encryption;
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
		fileTransferChatMessageModifier.decode(q->getSharedFromThis(), errorCode);
		currentRecvStep |= ChatMessagePrivate::Step::FileUpload;
	}

	if (contents.size() == 0) {
		// All previous modifiers only altered the internal content, let's fill the content list
		contents.push_back(&internalContent);
	}

	// ---------------------------------------
	// End of message modification
	// ---------------------------------------

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
	if (chatRoom && chatRoom->findMessageWithDirection(q->getImdnMessageId(), direction))
		return core->getCCore()->chat_deny_code;

	if (errorCode > 0) {
		reason = linphone_error_code_to_reason(errorCode);
		q->sendDeliveryNotification(reason);
		return reason;
	}

	bool messageToBeStored = false;
	for (Content *c : contents) {
		if (c->getContentType() == ContentType::FileTransfer || c->getContentType() == ContentType::PlainText) {
			messageToBeStored = true;
		}
	}
	if (messageToBeStored)
		q->store();

	return reason;
}

void ChatMessagePrivate::send () {
	L_Q();
	SalOp *op = salOp;
	LinphoneCall *call = nullptr;
	int errorCode = 0;

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
		call = linphone_core_get_call_by_remote_address(core->getCCore(), q->getToAddress().asString().c_str());
		if (call) {
			if (linphone_call_get_state(call) == LinphoneCallConnected || linphone_call_get_state(call) == LinphoneCallStreamsRunning ||
					linphone_call_get_state(call) == LinphoneCallPaused || linphone_call_get_state(call) == LinphoneCallPausing ||
					linphone_call_get_state(call) == LinphoneCallPausedByRemote) {
				lInfo() << "send SIP msg through the existing call.";
				op = linphone_call_get_op(call);
				string identity = linphone_core_find_best_identity(core->getCCore(), linphone_call_get_remote_address(call));
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
		op->set_user_pointer(L_GET_C_BACK_PTR(q));     /* If out of call, directly store msg */
		linphone_address_unref(peer);
	}

	// ---------------------------------------
	// Start of message modification
	// ---------------------------------------

	if (applyModifiers) {
		if ((currentSendStep &ChatMessagePrivate::Step::Multipart) == ChatMessagePrivate::Step::Multipart) {
			lInfo() << "Multipart step already done, skipping";
		} else {
			if (contents.size() > 1) {
				MultipartChatMessageModifier mcmm;
				mcmm.encode(q->getSharedFromThis(), errorCode);
			}
			currentSendStep |= ChatMessagePrivate::Step::Multipart;
		}

		if ((currentSendStep &ChatMessagePrivate::Step::Encryption) == ChatMessagePrivate::Step::Encryption) {
			lInfo() << "Encryption step already done, skipping";
		} else {
			EncryptionChatMessageModifier ecmm;
			ChatMessageModifier::Result result = ecmm.encode(q->getSharedFromThis(), errorCode);
			if (result == ChatMessageModifier::Result::Error) {
				sal_error_info_set((SalErrorInfo *)op->get_error_info(), SalReasonNotAcceptable, "SIP", errorCode, "Unable to encrypt IM", nullptr);
				q->updateState(ChatMessage::State::NotDelivered);
				q->store();
				return;
			} else if (result == ChatMessageModifier::Result::Suspended) {
				currentSendStep |= ChatMessagePrivate::Step::Encryption;
				return;
			}
			currentSendStep |= ChatMessagePrivate::Step::Encryption;
		}

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

	// ---------------------------------------
	// End of message modification
	// ---------------------------------------

	if (internalContent.isEmpty()) {
		internalContent = *(contents.front());
	}

	auto msgOp = dynamic_cast<SalMessageOpInterface *>(op);
	if (internalContent.getContentType().isValid()) {
		msgOp->send_message(
			q->getFromAddress().asString().c_str(),
			q->getToAddress().asString().c_str(),
			internalContent.getContentType().asString().c_str(),
			internalContent.getBodyAsString().c_str(),
			q->getToAddress().asString().c_str()
		);
	} else {
		msgOp->send_message(
			q->getFromAddress().asString().c_str(),
			q->getToAddress().asString().c_str(),
			internalContent.getBodyAsString().c_str()
		);
	}

	for (Content *content : contents) {
		// Restore FileContents and remove FileTransferContents
		if (content->getContentType() == ContentType::FileTransfer) {
			FileTransferContent *fileTransferContent = (FileTransferContent *)content;
			q->removeContent(*content);
			q->addContent(*fileTransferContent->getFileContent());
			delete fileTransferContent;
		}
	}

	q->setImdnMessageId(op->get_call_id());   /* must be known at that time */

	if (call && linphone_call_get_op(call) == op) {
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

// -----------------------------------------------------------------------------

ChatMessage::ChatMessage (const shared_ptr<ChatRoom> &chatRoom) :
	Object(*new ChatMessagePrivate), CoreAccessor(chatRoom->getCore()) {
	L_ASSERT(chatRoom);
	L_D();

	d->chatRoom = chatRoom;
	d->chatRoomId = chatRoom->getChatRoomId();
	d->fromAddress = chatRoom->getLocalAddress();
	d->direction = Direction::Outgoing;
}

ChatMessage::ChatMessage (const shared_ptr<ChatRoom> &chatRoom, const SimpleAddress &fromAddress) :
	Object(*new ChatMessagePrivate), CoreAccessor(chatRoom->getCore()) {
	L_ASSERT(chatRoom);
	L_D();

	d->chatRoom = chatRoom;
	d->chatRoomId = chatRoom->getChatRoomId();
	d->fromAddress = fromAddress;
	d->direction = Direction::Incoming;
}

ChatMessage::~ChatMessage () {
	L_D();
	for (Content *content : d->contents)
		delete content;

	if (d->salOp)
		d->salOp->release();
}

shared_ptr<ChatRoom> ChatMessage::getChatRoom () const {
	L_D();

	shared_ptr<ChatRoom> chatRoom = d->chatRoom.lock();
	if (!chatRoom)
		chatRoom = getCore()->getOrCreateBasicChatRoom(d->chatRoomId);

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
	return d->id;
}

void ChatMessage::setImdnMessageId (const string &id) {
	L_D();
	d->id = id;
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

const SimpleAddress &ChatMessage::getFromAddress () const {
	L_D();
	return d->fromAddress;
}

const SimpleAddress &ChatMessage::getToAddress () const {
	L_D();
	return d->direction == Direction::Outgoing ? d->chatRoomId.getPeerAddress() : d->chatRoomId.getLocalAddress();
}

const SimpleAddress &ChatMessage::getLocalAddress () const {
	L_D();
	return d->chatRoomId.getLocalAddress();
}

const SimpleAddress &ChatMessage::getRemoteAddress () const {
	L_D();
	return d->direction == Direction::Outgoing ? d->chatRoomId.getPeerAddress() : d->fromAddress;
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

void ChatMessage::store () {
	L_D();

	if (d->storageId != 0) {
		/* The message has already been stored (probably because of file transfer), update it */
		// TODO: history.
		// linphone_chat_message_store_update(L_GET_C_BACK_PTR(this));
	} else {
		/* Store the new message */
		// TODO: history.
		// linphone_chat_message_store(L_GET_C_BACK_PTR(this));
	}
}

void ChatMessage::updateState (State state) {
	L_D();

	d->setState(state);
	// TODO: history.
	// linphone_chat_message_store_state(L_GET_C_BACK_PTR(this));

	if (state == State::Delivered || state == State::NotDelivered) {
		shared_ptr<ChatRoom> chatRoom = getChatRoom();
		if (chatRoom)
			chatRoom->getPrivate()->moveTransientMessageToWeakMessages(getSharedFromThis());
	}
}

void ChatMessage::send () {
	L_D();

	// Do not allow sending a message that is already being sent or that has been correctly delivered/displayed
	if ((d->state == State::InProgress) || (d->state == State::Delivered) || (d->state == State::FileTransferDone) ||
			(d->state == State::DeliveredToUser) || (d->state == State::Displayed)) {
		lWarning() << "Cannot send chat message in state " << linphone_chat_message_state_to_string((LinphoneChatMessageState)d->state);
		return;
	}

	shared_ptr<ChatRoom> chatRoom = getChatRoom();
	if (chatRoom)
		chatRoom->getPrivate()->sendMessage(getSharedFromThis());
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

	shared_ptr<Core> core = getCore();
	if (linphone_core_realtime_text_enabled(core->getCCore())) {
		static const uint32_t new_line = 0x2028;
		static const uint32_t crlf = 0x0D0A;
		static const uint32_t lf = 0x0A;

		shared_ptr<ChatRoom> chatRoom = getChatRoom();
		if (!chatRoom)
			return -1;

		shared_ptr<LinphonePrivate::RealTimeTextChatRoom> rttcr =
			static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(chatRoom);
		LinphoneCall *call = rttcr->getCall();

		if (!call || !linphone_call_get_stream(call, LinphoneStreamTypeText))
			return -1;

		if (character == new_line || character == crlf || character == lf) {
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
			delete value;
		}

		text_stream_putchar32(reinterpret_cast<TextStream *>(
			linphone_call_get_stream(call, LinphoneStreamTypeText)), character
		);
		return 0;
	}
	return -1;
}

LINPHONE_END_NAMESPACE
