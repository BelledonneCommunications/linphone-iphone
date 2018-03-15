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

#include "linphone/api/c-api.h"
#include "linphone/core.h"
#include "linphone/event.h"
#include "linphone/proxy_config.h"
#include "linphone/utils/utils.h"

#include "address/address.h"
#include "c-wrapper/c-wrapper.h"
#include "content/content-manager.h"
#include "content/content-type.h"
#include "core/core-p.h"
#include "remote-conference-event-handler.h"
#include "remote-conference-list-event-handler.h"
#include "xml/resource-lists.h"
#include "xml/rlmi.h"

// TODO: Remove me later.
#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

void RemoteConferenceListEventHandler::subscribe () {
	if (lev)
		return;

	Content content;
	content.setContentType(ContentType::ResourceLists);

	Xsd::ResourceLists::ResourceLists rl = Xsd::ResourceLists::ResourceLists();
	Xsd::ResourceLists::ListType l = Xsd::ResourceLists::ListType();
	for (const auto &handler : handlers) {
		Address addr = handler->getChatRoomId().getPeerAddress();
		addr.setUriParam("Last-Notify", Utils::toString(handler->getLastNotify()));
		Xsd::ResourceLists::EntryType entry = Xsd::ResourceLists::EntryType(addr.asString());
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

	lev = linphone_event_ref(linphone_core_create_subscribe(lc, rlsAddr, "conference", 600));
	char *from = linphone_address_as_string(linphone_proxy_config_get_contact(linphone_core_get_default_proxy_config(getCore()->getCCore())));
	lev->op->set_from(from);
	bctbx_free(from);
	linphone_address_unref(rlsAddr);
	linphone_event_set_internal(lev, TRUE);
	linphone_event_set_user_data(lev, this);
	linphone_event_send_subscribe(lev, content.toLinphoneContent());
}

void RemoteConferenceListEventHandler::unsubscribe () {
	if (!lev)
		return;

	linphone_event_terminate(lev);
	lev = nullptr;
}

void RemoteConferenceListEventHandler::notifyReceived (Content *multipart) {
	list<Content> contents = ContentManager::multipartToContentList(*multipart);
	char *from = linphone_address_as_string(linphone_event_get_from(lev));
	const Address local(from);
	bctbx_free(from);
	map<Address, Address> addresses;
	for (const auto &content : contents) {
		const string &body = content.getBodyAsString();
		const ContentType &contentType = content.getContentType();
		if (contentType == ContentType::Rlmi) {
			addresses = parseRlmi(body);
			continue;
		}

		const string &value = content.getHeaderValue("Content-Id");
		if (value.empty())
			continue;

		Address cid(value);
		Address peer = addresses[cid];
		ChatRoomId id(local, peer);
		shared_ptr<RemoteConferenceEventHandler> handler = findHandler(id);
		if (!handler)
			continue;

		if (contentType == ContentType::Multipart)
			handler->multipartNotifyReceived(body);
		else if (contentType == ContentType::ConferenceInfo)
			handler->notifyReceived(body);
	}
}

map<Address, Address> RemoteConferenceListEventHandler::parseRlmi (const string &xmlBody) const {
	istringstream data(xmlBody);
	unique_ptr<Xsd::Rlmi::List> rlmi(Xsd::Rlmi::parseList(
		data,
		Xsd::XmlSchema::Flags::dont_validate
	));
	map<Address, Address> addresses;
	for (const auto &resource : rlmi->getResource()) {
		if (resource.getInstance().empty())
			continue;

		Address peer(resource.getUri());
		for (const auto &instance : resource.getInstance()) {
			if (!instance.getCid().present())
				continue;

			Address cid(instance.getCid().get());
			addresses[cid] = peer;
		}
	}
	return addresses;
}

void RemoteConferenceListEventHandler::addHandler (std::shared_ptr<RemoteConferenceEventHandler> handler) {
	handlers.push_back(handler);
}

shared_ptr<RemoteConferenceEventHandler> RemoteConferenceListEventHandler::findHandler (const ChatRoomId &chatRoomId) const {
	for (const auto &handler : handlers) {
		if (handler->getChatRoomId() == chatRoomId)
			return handler;
	}

	return nullptr;
}

const list<shared_ptr<RemoteConferenceEventHandler>> &RemoteConferenceListEventHandler::getHandlers () const {
	return handlers;
}

LINPHONE_END_NAMESPACE
