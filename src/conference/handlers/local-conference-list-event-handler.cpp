/*
 * local-conference-list-event-handler.cpp
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

#include "belle-sip/utils.h"
#include "linphone/enums/chat-room-enums.h"
#include "linphone/utils/utils.h"
#include "linphone/api/c-address.h"

#include "address/address.h"
#include "c-wrapper/c-wrapper.h"
#include "chat/chat-room/abstract-chat-room.h"
#include "conference/participant-p.h"
#include "conference/participant-device.h"
#include "content/content.h"
#include "content/content-manager.h"
#include "content/content-type.h"
#include "core/core.h"
#include "local-conference-event-handler.h"
#include "local-conference-list-event-handler.h"
#include "logger/logger.h"
#include "xml/resource-lists.h"
#include "xml/rlmi.h"

// TODO: Remove me later.
#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

namespace {
	constexpr const char MultipartBoundaryListEventHandler[] = "---------------------------14737809831412343453453";
}

// -----------------------------------------------------------------------------

LocalConferenceListEventHandler::LocalConferenceListEventHandler (const std::shared_ptr<Core> &core) : CoreAccessor(core) {}

// -----------------------------------------------------------------------------

void LocalConferenceListEventHandler::subscribeReceived (LinphoneEvent *lev, const LinphoneContent *body) {
	LinphoneSubscriptionState subscriptionState = linphone_event_get_subscription_state(lev);

	const string &xmlBody = string(linphone_content_get_string_buffer(body));
	if (xmlBody.empty()) {
		linphone_event_deny_subscription(lev, LinphoneReasonDeclined);
		return;
	}

	linphone_event_accept_subscription(lev);

	if (subscriptionState != LinphoneSubscriptionIncomingReceived && subscriptionState != LinphoneSubscriptionTerminated)
		return;

	const LinphoneAddress *lAddr = linphone_event_get_from(lev);
	char *addrStr = linphone_address_as_string(lAddr);
	IdentityAddress participantAddr(addrStr);
	bctbx_free(addrStr);

	const LinphoneAddress *lDeviceAddr = linphone_event_get_remote_contact(lev);
	char *deviceAddrStr = linphone_address_as_string(lDeviceAddr);
	IdentityAddress deviceAddr(deviceAddrStr);
	bctbx_free(deviceAddrStr);

	list<Content *> contents;
	Content *rlmiContent = new Content();
	rlmiContent->setContentType(ContentType::Rlmi);

	// Create Rlmi body
	Xsd::Rlmi::List::ResourceSequence resources;

	// Parse resource list
	bool noContent = true;
	istringstream data(xmlBody);
	unique_ptr<Xsd::ResourceLists::ResourceLists> rl(Xsd::ResourceLists::parseResourceLists(
		data,
		Xsd::XmlSchema::Flags::dont_validate
	));
	for (const auto &l : rl->getList()) {
		for (const auto &entry : l.getEntry()) {
			Address addr(entry.getUri());
			string notifyIdStr = addr.getUriParamValue("Last-Notify");
			addr.removeUriParam("Last-Notify");
			ChatRoomId chatRoomId(addr, addr);
			LocalConferenceEventHandler *handler = findHandler(chatRoomId);
			if (!handler)
				continue;

			shared_ptr<AbstractChatRoom> chatRoom = L_GET_CPP_PTR_FROM_C_OBJECT(linphone_event_get_core(lev))->findChatRoom(chatRoomId);
			if (!chatRoom) {
				lError() << "Received subscribe for unknown chat room: " << chatRoomId;
				continue;
			}

			shared_ptr<Participant> participant = chatRoom->findParticipant(participantAddr);
			if (!participant) {
				lError() << "Received subscribe for unknown participant: " << participantAddr <<  " for chat room: " << chatRoomId;
				continue;
			}
			shared_ptr<ParticipantDevice> device = participant->getPrivate()->findDevice(deviceAddr);
			if (!device || (device->getState() != ParticipantDevice::State::Present && device->getState() != ParticipantDevice::State::Joining)) {
				lError() << "Received subscribe for unknown device: " << deviceAddr << " for participant: "
					<< participantAddr <<  " for chat room: " << chatRoomId;
				continue;
			}
			device->setConferenceSubscribeEvent((subscriptionState == LinphoneSubscriptionIncomingReceived) ? lev : nullptr);

			int notifyId = (notifyIdStr.empty() || device->getState() == ParticipantDevice::State::Joining) ? 0 : Utils::stoi(notifyIdStr);
			string notifyBody = handler->getNotifyForId(notifyId, !!(chatRoom->getCapabilities() & AbstractChatRoom::Capabilities::OneToOne));
			if (notifyBody.empty())
				continue;

			noContent = false;
			Content *content = new Content();
			if (notifyId > 0) {
				ContentType contentType(ContentType::Multipart);
				contentType.addParameter("boundary", string(MultipartBoundary));
				content->setContentType(contentType);
			} else
				content->setContentType(ContentType::ConferenceInfo);

			content->setBody(notifyBody);
			char token[17];
			belle_sip_random_token(token, sizeof(token));
			content->addHeader("Content-Id", token);
			content->addHeader("Content-Length", Utils::toString(notifyBody.size()));
			contents.push_back(content);

			// Add entry into the Rlmi content of the notify body
			Xsd::Rlmi::Resource resource(addr.asStringUriOnly());
			Xsd::Rlmi::Resource::InstanceSequence instances;
			Xsd::Rlmi::Instance instance(token, Xsd::Rlmi::State::Value::active);
			instances.push_back(instance);
			resource.setInstance(instances);
			resources.push_back(resource);
		}
	}

	if (noContent)
		return;

	Xsd::Rlmi::List list("", 0, TRUE);
	list.setResource(resources);
	Xsd::XmlSchema::NamespaceInfomap map;
	stringstream rlmiBody;
	Xsd::Rlmi::serializeList(rlmiBody, list, map);
	rlmiContent->setBody(rlmiBody.str());

	contents.push_front(rlmiContent);
	Content multipart = ContentManager::contentListToMultipart(contents, MultipartBoundaryListEventHandler);
	if (linphone_core_content_encoding_supported(getCore()->getCCore(), "deflate"))
		multipart.setContentEncoding("deflate");
	LinphoneContent *cContent = L_GET_C_BACK_PTR(&multipart);
	linphone_event_notify(lev, cContent);
	contents.clear();
}

// -----------------------------------------------------------------------------

void LocalConferenceListEventHandler::addHandler (LocalConferenceEventHandler *handler) {
	if (handler)
		handlers.push_back(handler);
}

void LocalConferenceListEventHandler::removeHandler (LocalConferenceEventHandler *handler) {
	if (handler)
		handlers.remove(handler);
}

LocalConferenceEventHandler *LocalConferenceListEventHandler::findHandler (const ChatRoomId &chatRoomId) const {
	for (const auto &handler : handlers) {
		if (handler->getChatRoomId() == chatRoomId)
			return handler;
	}

	return nullptr;
}

const list<LocalConferenceEventHandler *> &LocalConferenceListEventHandler::getHandlers () const {
	return handlers;
}

LINPHONE_END_NAMESPACE

