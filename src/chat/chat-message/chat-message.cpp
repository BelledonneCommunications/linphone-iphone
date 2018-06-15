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

#include "linphone/api/c-content.h"
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
#include "chat/modifier/multipart-chat-message-modifier.h"
#include "chat/notification/imdn.h"
#include "conference/participant.h"
#include "conference/participant-imdn-state.h"
#include "content/content-disposition.h"
#include "content/header/header-param.h"
#include "core/core.h"
#include "core/core-p.h"
#include "logger/logger.h"
#include "sip-tools/sip-headers.h"

#include "ortp/b64.h"

// =============================================================================

using namespace std;

using namespace B64_NAMESPACE;

LINPHONE_BEGIN_NAMESPACE

ChatMessagePrivate::ChatMessagePrivate(const std::shared_ptr<AbstractChatRoom> &cr, ChatMessage::Direction dir):fileTransferChatMessageModifier(cr->getCore()->getCCore()->http_provider) {
	direction = dir;
	setChatRoom(cr);
}

void ChatMessagePrivate::setDirection (ChatMessage::Direction dir) {
	direction = dir;
}

void ChatMessagePrivate::setTime (time_t t) {
	time = t;
}

void ChatMessagePrivate::setIsReadOnly (bool readOnly) {
	isReadOnly = readOnly;
}

void ChatMessagePrivate::setParticipantState (const IdentityAddress &participantAddress, ChatMessage::State newState, time_t stateChangeTime) {
	L_Q();

	if (!dbKey.isValid())
		return;

	unique_ptr<MainDb> &mainDb = q->getChatRoom()->getCore()->getPrivate()->mainDb;
	shared_ptr<EventLog> eventLog = mainDb->getEventFromKey(dbKey);
	ChatMessage::State currentState = mainDb->getChatMessageParticipantState(eventLog, participantAddress);
	if (!validStateTransition(currentState, newState))
		return;

	lInfo() << "Chat message " << this << ": moving participant '" << participantAddress.asString() << "' state to "
		<< Utils::toString(newState);
	mainDb->setChatMessageParticipantState(eventLog, participantAddress, newState, stateChangeTime);

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	if (cbs && linphone_chat_message_cbs_get_participant_imdn_state_changed(cbs)) {
		auto participant = q->getChatRoom()->findParticipant(participantAddress);
		ParticipantImdnState imdnState(participant, newState, stateChangeTime);
		linphone_chat_message_cbs_get_participant_imdn_state_changed(cbs)(msg,
			_linphone_participant_imdn_state_from_cpp_obj(imdnState)
		);
	}

	if (linphone_config_get_bool(linphone_core_get_config(q->getChatRoom()->getCore()->getCCore()),
			"misc", "enable_simple_group_chat_message_state", FALSE
		)
	) {
		setState(newState);
		return;
	}

	list<ChatMessage::State> states = mainDb->getChatMessageParticipantStates(eventLog);
	size_t nbDisplayedStates = 0;
	size_t nbDeliveredToUserStates = 0;
	size_t nbNotDeliveredStates = 0;
	for (const auto &state : states) {
		switch (state) {
			case ChatMessage::State::Displayed:
				nbDisplayedStates++;
				break;
			case ChatMessage::State::DeliveredToUser:
				nbDeliveredToUserStates++;
				break;
			case ChatMessage::State::NotDelivered:
				nbNotDeliveredStates++;
				break;
			default:
				break;
		}
	}

	if (nbNotDeliveredStates > 0)
		setState(ChatMessage::State::NotDelivered);
	else if (nbDisplayedStates == states.size())
		setState(ChatMessage::State::Displayed);
	else if ((nbDisplayedStates + nbDeliveredToUserStates) == states.size())
		setState(ChatMessage::State::DeliveredToUser);
}

void ChatMessagePrivate::setState (ChatMessage::State newState, bool force) {
	L_Q();

	if (force)
		state = newState;

	if (!validStateTransition(state, newState))
		return;

	lInfo() << "Chat message " << this << ": moving from " << Utils::toString(state) <<
		" to " << Utils::toString(newState);
	state = newState;

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
	if (linphone_chat_message_get_message_state_changed_cb(msg))
		linphone_chat_message_get_message_state_changed_cb(msg)(
			msg,
			(LinphoneChatMessageState)state,
			linphone_chat_message_get_message_state_changed_cb_user_data(msg)
		);

	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	if (cbs && linphone_chat_message_cbs_get_msg_state_changed(cbs))
		linphone_chat_message_cbs_get_msg_state_changed(cbs)(msg, (LinphoneChatMessageState)state);

	if (state == ChatMessage::State::FileTransferDone && !hasFileTransferContent()) {
		// We wait until the file has been downloaded to send the displayed IMDN
		bool doNotStoreInDb = static_cast<ChatRoomPrivate *>(q->getChatRoom()->getPrivate())->sendDisplayNotification(q->getSharedFromThis());
		// Force the state so it is stored directly in DB, but when the IMDN has successfully been delivered
		setState(ChatMessage::State::Displayed, doNotStoreInDb);
	} else {
		updateInDb();
	}
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
	for (const Content *c : getContents()) {
		if (c->getContentType() == ContentType::PlainText) {
			return true;
		}
	}
	return false;
}

const Content* ChatMessagePrivate::getTextContent() const {
	for (const Content *c : getContents()) {
		if (c->getContentType() == ContentType::PlainText) {
			return c;
		}
	}
	return &Utils::getEmptyConstRefObject<Content>();
}

bool ChatMessagePrivate::hasFileTransferContent() const {
	for (const Content *c : contents) {
		if (c->isFileTransfer()) {
			return true;
		}
	}
	return false;
}

const Content* ChatMessagePrivate::getFileTransferContent() const {
	for (const Content *c : contents) {
		if (c->isFileTransfer()) {
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
	for (const Content *c : getContents()) {
		if (!c->getAppData("legacy").empty()) {
			return c->getAppData("legacy");
		}
	}
	return Utils::getEmptyConstRefObject<string>();
}

void ChatMessagePrivate::setAppdata (const string &data) {
	bool contentFound = false;
	for (Content *c : getContents()) {
		c->setAppData("legacy", data);
		contentFound = true;
		break;
	}
	if (contentFound) {
		updateInDb();
	}
}

const string &ChatMessagePrivate::getExternalBodyUrl () const {
	if (!externalBodyUrl.empty()) {
		return externalBodyUrl;
	}
	if (hasFileTransferContent()) {
		FileTransferContent *content = (FileTransferContent*) getFileTransferContent();
		return content->getFileUrl();
	}
	return Utils::getEmptyConstRefObject<string>();
}

void ChatMessagePrivate::setExternalBodyUrl (const string &url) {
	externalBodyUrl = url;
}

const ContentType &ChatMessagePrivate::getContentType () {
	loadContentsFromDatabase();
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
	loadContentsFromDatabase();
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
	loadContentsFromDatabase();
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
	loadContentsFromDatabase();
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

const Content *ChatMessagePrivate::getFileTransferInformation () const {
	if (hasFileTransferContent()) {
		return getFileTransferContent();
	}
	for (const Content *c : getContents()) {
		if (c->isFile()) {
			FileContent *fileContent = (FileContent *)c;
			return fileContent;
		}
	}
	return nullptr;
}

void ChatMessagePrivate::setFileTransferInformation (Content *content) {
	L_Q();

	if (content->isFile()) {
		q->addContent(content);
	} else {
		// This scenario is more likely to happen because the caller is using the C API
		LinphoneContent *c_content = L_GET_C_BACK_PTR(content);
		FileContent *fileContent = new FileContent();
		fileContent->setContentType(content->getContentType());
		fileContent->setFileSize(linphone_content_get_size(c_content)); // This information is only available from C Content if it was created from C API
		fileContent->setFileName(linphone_content_get_name(c_content)); // This information is only available from C Content if it was created from C API
		if (!content->isEmpty()) {
			fileContent->setBody(content->getBody());
		}
		q->addContent(fileContent);
	}
}

bool ChatMessagePrivate::downloadFile () {
	L_Q();

	for (auto &content : getContents())
		if (content->isFileTransfer())
			return q->downloadFile(static_cast<FileTransferContent *>(content));

	return false;
}

void ChatMessagePrivate::addContent (Content *content) {
	getContents().push_back(content);
}

void ChatMessagePrivate::removeContent (Content *content) {
	getContents().remove(content);
}

void ChatMessagePrivate::loadFileTransferUrlFromBodyToContent() {
	L_Q();
	int errorCode = 0;
	fileTransferChatMessageModifier.decode(q->getSharedFromThis(), errorCode);
}

std::string ChatMessagePrivate::createFakeFileTransferFromUrl(const std::string &url) {
	return fileTransferChatMessageModifier.createFakeFileTransferFromUrl(url);
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

static void forceUtf8Content (Content &content) {
	// TODO: Deal with other content type in the future.
	ContentType contentType = content.getContentType();
	if (contentType != ContentType::PlainText)
		return;

	string charset = contentType.getParameter("charset").getValue();
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
		string utf8Body = Utils::convertAnyToUtf8(content.getBodyAsUtf8String(), charset);
		if (!utf8Body.empty()) {
			// TODO: use move operator if possible in the future!
			content.setBodyFromUtf8(utf8Body);
			contentType.addParameter("charset", "UTF-8");
			content.setContentType(contentType);
		}
	}

	L_END_LOG_EXCEPTION
}

void ChatMessagePrivate::notifyReceiving () {
	L_Q();

	LinphoneChatRoom *chatRoom = static_pointer_cast<ChatRoom>(q->getChatRoom())->getPrivate()->getCChatRoom();
	if ((getContentType() != ContentType::Imdn) && (getContentType() != ContentType::ImIsComposing)) {
		_linphone_chat_room_notify_chat_message_should_be_stored(chatRoom, L_GET_C_BACK_PTR(q->getSharedFromThis()));
		if (toBeStored)
			storeInDb();
	} else {
		// For compatibility, when CPIM is not used
		positiveDeliveryNotificationRequired = false;
		negativeDeliveryNotificationRequired = false;
		displayNotificationRequired = false;
	}
	shared_ptr<ConferenceChatMessageEvent> event = make_shared<ConferenceChatMessageEvent>(
		::time(nullptr), q->getSharedFromThis()
	);
	_linphone_chat_room_notify_chat_message_received(chatRoom, L_GET_C_BACK_PTR(event));
	// Legacy
	q->getChatRoom()->getPrivate()->notifyChatMessageReceived(q->getSharedFromThis());

	if (getPositiveDeliveryNotificationRequired())
		static_cast<ChatRoomPrivate *>(q->getChatRoom()->getPrivate())->sendDeliveryNotification(q->getSharedFromThis());
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
			if (getNegativeDeliveryNotificationRequired()) {
				static_cast<ChatRoomPrivate *>(q->getChatRoom()->getPrivate())->sendDeliveryErrorNotification(
					q->getSharedFromThis(),
					reason
				);
			}
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
			ContentType ct(c->getContentType());
			ct.cleanParameters();
			if (linphone_core_is_content_type_supported(core->getCCore(), ct.asString().c_str())) {
				foundSupportContentType = true;
				break;
			} else
			lError() << "Unsupported content-type: " << c->getContentType();
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
		if (getNegativeDeliveryNotificationRequired()) {
			static_cast<ChatRoomPrivate *>(q->getChatRoom()->getPrivate())->sendDeliveryErrorNotification(
				q->getSharedFromThis(),
				reason
			);
		}
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

	if (toBeStored && currentSendStep == (ChatMessagePrivate::Step::Started | ChatMessagePrivate::Step::None))
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
				lInfo() << "Send SIP msg through the existing call";
				op = call->getPrivate()->getOp();
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
		op->setUserPointer(q);     /* If out of call, directly store msg */
		linphone_address_unref(peer);
	}
	op->setFrom(q->getFromAddress().asString().c_str());
	op->setTo(q->getToAddress().asString().c_str());

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
				CpimChatMessageModifier ccmm;
				ccmm.encode(q->getSharedFromThis(), errorCode);
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
					sal_error_info_set((SalErrorInfo *)op->getErrorInfo(), SalReasonNotAcceptable, "SIP", errorCode, "Unable to encrypt IM", nullptr);
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
		} else if (externalBodyUrl.empty()) { // When using external body url, there is no content
			lError() << "Trying to send a message without any content !";
			return;
		}
	}

	auto msgOp = dynamic_cast<SalMessageOpInterface *>(op);
	if (!externalBodyUrl.empty()) {
		Content content;
		ContentType contentType(ContentType::ExternalBody);
		contentType.addParameter("access-type", "URL");
		contentType.addParameter("URL", "\"" + externalBodyUrl + "\"");
		content.setContentType(contentType);
		msgOp->sendMessage(content);
	} else {
		if (!internalContent.getContentType().isValid())
			internalContent.setContentType(ContentType::PlainText);
		if (!contentEncoding.empty())
			internalContent.setContentEncoding(contentEncoding);
		msgOp->sendMessage(internalContent);
	}

	// Restore FileContents and remove FileTransferContents
	list<Content*>::iterator it = contents.begin();
	while (it != contents.end()) {
		Content *content = *it;
		if (content->isFileTransfer()) {
			FileTransferContent *fileTransferContent = static_cast<FileTransferContent *>(content);
			it = contents.erase(it);
			addContent(fileTransferContent->getFileContent());
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
		setImdnMessageId(op->getCallId());   /* must be known at that time */

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

		// Avoid transaction in transaction if contents are not loaded.
		loadContentsFromDatabase();
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

	// Avoid transaction in transaction if contents are not loaded.
	loadContentsFromDatabase();
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

bool ChatMessagePrivate::validStateTransition (ChatMessage::State currentState, ChatMessage::State newState) {
	if (newState == currentState)
		return false;

	if (
		(currentState == ChatMessage::State::Displayed || currentState == ChatMessage::State::DeliveredToUser) &&
		(
			newState == ChatMessage::State::DeliveredToUser ||
			newState == ChatMessage::State::Delivered ||
			newState == ChatMessage::State::NotDelivered
		)
	)
		return false;
	return true;
}

// -----------------------------------------------------------------------------

ChatMessage::ChatMessage (const shared_ptr<AbstractChatRoom> &chatRoom, ChatMessage::Direction direction) :
	Object(*new ChatMessagePrivate(chatRoom, direction)), CoreAccessor(chatRoom->getCore()) {
}

ChatMessage::ChatMessage (ChatMessagePrivate &p) : Object(p), CoreAccessor(p.getPublic()->getChatRoom()->getCore()) {
}

ChatMessage::~ChatMessage () {
	L_D();

	for (Content *content : d->contents) {
		if (content->isFileTransfer()) {
			FileTransferContent *fileTransferContent = static_cast<FileTransferContent *>(content);
			delete fileTransferContent->getFileContent();
		}
		delete content;
	}

	if (d->salOp) {
		d->salOp->setUserPointer(nullptr);
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

void ChatMessagePrivate::loadContentsFromDatabase () const {
	L_Q();
	if (contentsNotLoadedFromDatabase) {
		isReadOnly = false;
		contentsNotLoadedFromDatabase = false;
		q->getChatRoom()->getCore()->getPrivate()->mainDb->loadChatMessageContents(
			const_pointer_cast<ChatMessage>(q->getSharedFromThis())
		);
		isReadOnly = true;
	}
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

list<ParticipantImdnState> ChatMessage::getParticipantsByImdnState (ChatMessage::State state) const {
	L_D();

	list<ParticipantImdnState> result;
	if (!(getChatRoom()->getCapabilities() & AbstractChatRoom::Capabilities::Conference) || !d->dbKey.isValid())
		return result;

	unique_ptr<MainDb> &mainDb = getChatRoom()->getCore()->getPrivate()->mainDb;
	shared_ptr<EventLog> eventLog = mainDb->getEventFromKey(d->dbKey);
	list<MainDb::ParticipantState> dbResults = mainDb->getChatMessageParticipantsByImdnState(eventLog, state);
	for (const auto &dbResult : dbResults) {
		auto sender = getChatRoom()->findParticipant(getFromAddress());
		auto participant = getChatRoom()->findParticipant(dbResult.address);
		if (participant && (participant != sender))
			result.emplace_back(participant, dbResult.state, dbResult.timestamp);
	}

	return result;
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
	return d->getContents();
}

void ChatMessage::addContent (Content *content) {
	L_D();
	if (!d->isReadOnly)
		d->addContent(content);
}

void ChatMessage::removeContent (Content *content) {
	L_D();
	if (!d->isReadOnly)
		d->removeContent(content);
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

void ChatMessage::send () {
	L_D();

	// Do not allow sending a message that is already being sent or that has been correctly delivered/displayed
	if ((d->state == State::InProgress) || (d->state == State::Delivered) || (d->state == State::FileTransferDone) ||
			(d->state == State::DeliveredToUser) || (d->state == State::Displayed)) {
		lWarning() << "Cannot send chat message in state " << Utils::toString(d->state);
		return;
	}

	d->loadContentsFromDatabase();
	getChatRoom()->getPrivate()->sendChatMessage(getSharedFromThis());
}

bool ChatMessage::downloadFile(FileTransferContent *fileTransferContent) {
	L_D();
	return d->fileTransferChatMessageModifier.downloadFile(getSharedFromThis(), fileTransferContent);
}

bool ChatMessage::isFileTransferInProgress() {
	L_D();
	return d->fileTransferChatMessageModifier.isFileTransferInProgressAndValid();
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

	constexpr uint32_t newLine = 0x2028;
	constexpr uint32_t crlf = 0x0D0A;
	constexpr uint32_t lf = 0x0A;

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

	if (character == newLine || character == crlf || character == lf) {
		shared_ptr<Core> core = getCore();
		if (lp_config_get_int(core->getCCore()->config, "misc", "store_rtt_messages", 1) == 1) {
			lInfo() << "New line sent, forge a message with content " << d->rttMessage;
			d->state = State::Displayed;
			d->setText(d->rttMessage);
			d->storeInDb();
			d->rttMessage = "";
		}
	} else {
		char *value = LinphonePrivate::Utils::utf8ToChar(character);
		d->rttMessage += string(value);
		lDebug() << "Sent RTT character: " << value << "(" << (unsigned long)character << "), pending text is " << d->rttMessage;
		delete[] value;
	}

	text_stream_putchar32(
		reinterpret_cast<TextStream *>(call->getPrivate()->getMediaStream(LinphoneStreamTypeText)),
		character
	);
	return 0;
}

LINPHONE_END_NAMESPACE
