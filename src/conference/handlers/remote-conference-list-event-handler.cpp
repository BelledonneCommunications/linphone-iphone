/*
 * remote-conference-list-event-handler.cpp
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

#include "linphone/core.h"
#include "linphone/event.h"
#include "linphone/proxy_config.h"
#include "linphone/utils/utils.h"

#include "address/address.h"
#include "c-wrapper/c-wrapper.h"
#include "content/content-manager.h"
#include "content/content-type.h"
#include "core/core-p.h"
#include "logger/logger.h"
#include "remote-conference-event-handler.h"
#include "remote-conference-list-event-handler.h"
#include "xml/conference-info.h"
#include "xml/resource-lists.h"
#include "xml/rlmi.h"

// TODO: Remove me later.
#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

RemoteConferenceListEventHandler::RemoteConferenceListEventHandler (const std::shared_ptr<Core> &core) : CoreAccessor(core) {
	getCore()->getPrivate()->registerListener(this);
}

RemoteConferenceListEventHandler::~RemoteConferenceListEventHandler () {
	try {
		getCore()->getPrivate()->unregisterListener(this);
	} catch (const bad_weak_ptr &) {
		// Unable to unregister listener here. Core is destroyed and the listener doesn't exist.
	}

	unsubscribe();
}

// -----------------------------------------------------------------------------

void RemoteConferenceListEventHandler::subscribe () {
	if (lev) {
		linphone_event_unref(lev);
		lev = nullptr;
	}

	if (handlers.size() == 0)
		return;

	Content content;
	content.setContentType(ContentType::ResourceLists);

	Xsd::ResourceLists::ResourceLists rl = Xsd::ResourceLists::ResourceLists();
	Xsd::ResourceLists::ListType l = Xsd::ResourceLists::ListType();
	for (const auto &handler : handlers) {
		const ChatRoomId &chatRoomId = handler->getChatRoomId();
		shared_ptr<AbstractChatRoom> cr = getCore()->findChatRoom(chatRoomId);
		if (!cr)
			continue;

		if (cr->hasBeenLeft())
			continue;

		Address addr = chatRoomId.getPeerAddress();
		addr.setUriParam("Last-Notify", Utils::toString(handler->getLastNotify()));
		Xsd::ResourceLists::EntryType entry = Xsd::ResourceLists::EntryType(addr.asStringUriOnly());
		l.getEntry().push_back(entry);
	}
	rl.getList().push_back(l);

	Xsd::XmlSchema::NamespaceInfomap map;
	stringstream xmlBody;
	serializeResourceLists(xmlBody, rl, map);
	content.setBody(xmlBody.str());

	LinphoneCore *lc = getCore()->getCCore();
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(lc);
	if (!cfg || (linphone_proxy_config_get_state(cfg) != LinphoneRegistrationOk))
		return;

	LinphoneAddress *rlsAddr = linphone_address_new(linphone_proxy_config_get_conference_factory_uri(cfg));

	lev = linphone_core_create_subscribe(lc, rlsAddr, "conference", 600);
	char *from = linphone_address_as_string(linphone_proxy_config_get_contact(linphone_core_get_default_proxy_config(getCore()->getCCore())));
	lev->op->setFrom(from);
	bctbx_free(from);
	linphone_address_unref(rlsAddr);
	linphone_event_set_internal(lev, TRUE);
	linphone_event_add_custom_header(lev, "Require", "recipient-list-subscribe");
	linphone_event_add_custom_header(lev, "Accept", "multipart/related, application/conference-info+xml, application/rlmi+xml");
	linphone_event_add_custom_header(lev, "Content-Disposition", "recipient-list");
	if (linphone_core_content_encoding_supported(lc, "deflate")) {
		content.setContentEncoding("deflate");
		linphone_event_add_custom_header(lev, "Accept-Encoding", "deflate");
	}
	linphone_event_set_user_data(lev, this);
	LinphoneContent *cContent = L_GET_C_BACK_PTR(&content);
	linphone_event_send_subscribe(lev, cContent);
}

void RemoteConferenceListEventHandler::unsubscribe () {
	if (lev) {
		linphone_event_terminate(lev);
		lev = nullptr;
	}
}

void RemoteConferenceListEventHandler::notifyReceived (const Content *notifyContent) {
	char *from = linphone_address_as_string(linphone_event_get_from(lev));
	const IdentityAddress local(from);

	if (notifyContent->getContentType().weakEqual(ContentType::ConferenceInfo)) {
		// Simple notify received directly from a chat-room
		const string &xmlBody = notifyContent->getBodyAsString();
		istringstream data(xmlBody);
		unique_ptr<Xsd::ConferenceInfo::ConferenceType> confInfo = Xsd::ConferenceInfo::parseConferenceInfo(data, Xsd::XmlSchema::Flags::dont_validate);

		IdentityAddress entityAddress(confInfo->getEntity().c_str());
		ChatRoomId id(entityAddress, local);
		RemoteConferenceEventHandler *handler = findHandler(id);
		if (!handler)
			return;

		handler->notifyReceived(xmlBody);
		return;
	}

	list<Content> contents = ContentManager::multipartToContentList(*notifyContent);
	bctbx_free(from);
	map<string, IdentityAddress> addresses;
	for (const auto &content : contents) {
		const string &body = content.getBodyAsString();
		const ContentType &contentType = content.getContentType();
		if (contentType == ContentType::Rlmi) {
			addresses = parseRlmi(body);
			continue;
		}

		const string &cid = content.getHeader("Content-Id").getValue();
		if (cid.empty())
			continue;

		map<string, IdentityAddress>::const_iterator it = addresses.find(cid);
		if (it == addresses.cend())
			continue;

		IdentityAddress peer = it->second;
		ChatRoomId id(peer, local);
		RemoteConferenceEventHandler *handler = findHandler(id);
		if (!handler)
			continue;

		if (contentType.weakEqual(ContentType::Multipart))
			handler->multipartNotifyReceived(body);
		else if (contentType.weakEqual(ContentType::ConferenceInfo))
			handler->notifyReceived(body);
	}
}

// -----------------------------------------------------------------------------

RemoteConferenceEventHandler *RemoteConferenceListEventHandler::findHandler (const ChatRoomId &chatRoomId) const {
	for (const auto &handler : handlers) {
		if (handler->getChatRoomId() == chatRoomId)
			return handler;
	}

	return nullptr;
}

const list<RemoteConferenceEventHandler *> &RemoteConferenceListEventHandler::getHandlers () const {
	return handlers;
}

void RemoteConferenceListEventHandler::addHandler (RemoteConferenceEventHandler *handler) {
	if (handler)
		handlers.push_back(handler);
}

void RemoteConferenceListEventHandler::removeHandler (RemoteConferenceEventHandler *handler) {
	if (handler)
		handlers.remove(handler);
}

map<string, IdentityAddress> RemoteConferenceListEventHandler::parseRlmi (const string &xmlBody) const {
	istringstream data(xmlBody);
	unique_ptr<Xsd::Rlmi::List> rlmi(Xsd::Rlmi::parseList(
		data,
		Xsd::XmlSchema::Flags::dont_validate
	));
	map<string, IdentityAddress> addresses;
	for (const auto &resource : rlmi->getResource()) {
		if (resource.getInstance().empty())
			continue;

		const string &uri = string(resource.getUri());
		if (uri.empty())
			continue;

		IdentityAddress peer(uri);
		for (const auto &instance : resource.getInstance()) {
			const string &cid = string(instance.getId());
			if (cid.empty())
				continue;

			addresses.emplace(cid, peer);
		}
	}
	return addresses;
}

// -----------------------------------------------------------------------------

void RemoteConferenceListEventHandler::onNetworkReachable (bool sipNetworkReachable, bool mediaNetworkReachable) {
	if (!sipNetworkReachable)
		unsubscribe();
}

void RemoteConferenceListEventHandler::onRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const std::string &message) {
	if (state == LinphoneRegistrationOk)
		subscribe();
}

void RemoteConferenceListEventHandler::onEnteringBackground () {
	unsubscribe();
}

void RemoteConferenceListEventHandler::onEnteringForeground () {
	subscribe();
}

LINPHONE_END_NAMESPACE
