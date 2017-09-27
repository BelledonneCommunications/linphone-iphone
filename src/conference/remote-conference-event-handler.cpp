/*
 * remote-conference-event-handler.cpp
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

#include "remote-conference-event-handler.h"
#include "object/object-p.h"

#include "private.h"

#include "xml/conference-info.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

using namespace Xsd::ConferenceInfo;

class RemoteConferenceEventHandlerPrivate : public ObjectPrivate {
public:
	LinphoneCore *core = nullptr;
	ConferenceListener *listener = nullptr;
	Address confAddr;
	string confId;
	LinphoneEvent *lev = nullptr;
};

// -----------------------------------------------------------------------------

RemoteConferenceEventHandler::RemoteConferenceEventHandler(LinphoneCore *core, ConferenceListener *listener)
	: Object(*new RemoteConferenceEventHandlerPrivate) {
	L_D();
	xercesc::XMLPlatformUtils::Initialize();
	d->core = core;
	d->listener = listener;
}

RemoteConferenceEventHandler::~RemoteConferenceEventHandler() {
	L_D();
	xercesc::XMLPlatformUtils::Terminate();
	if (d->lev)
		linphone_event_unref(d->lev);
}

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandler::subscribe(string confId) {
	L_D();
	d->confId = confId;
	LinphoneAddress *addr = linphone_address_new(d->confAddr.asString().c_str());
	d->lev = linphone_core_create_subscribe(d->core, addr, "Conference", 600);
	linphone_address_unref(addr);
	linphone_event_ref(d->lev);
	linphone_event_set_internal(d->lev, TRUE);
	linphone_event_set_user_data(d->lev, this);
	linphone_event_add_custom_header(d->lev, "Conf-id", d->confId.c_str()); // TODO : ???
	linphone_event_send_subscribe(d->lev, nullptr);
}

void RemoteConferenceEventHandler::unsubscribe() {
	L_D();
	linphone_event_terminate(d->lev);
}

void RemoteConferenceEventHandler::notifyReceived(string xmlBody) {
	L_D();
	istringstream data(xmlBody);
	unique_ptr<ConferenceType> confInfo = parseConferenceInfo(data, Xsd::XmlSchema::Flags::dont_validate);
	if (confInfo->getEntity() == d->confAddr.asString()) {
		for (const auto &user : confInfo->getUsers()->getUser()) {
			LinphoneAddress *cAddr = linphone_core_interpret_url(d->core, user.getEntity()->c_str());
			Address addr(linphone_address_as_string(cAddr));
			if (user.getState() == "deleted")
				d->listener->onParticipantRemoved(addr);
			else {
				bool isAdmin = false;
				if (user.getRoles()) {
					for (const auto &entry : user.getRoles()->getEntry()) {
						if (entry == "admin") {
							isAdmin = true;
							break;
						}
					}
				}
				if (user.getState() == "full")
					d->listener->onParticipantAdded(addr);
				d->listener->onParticipantSetAdmin(addr, isAdmin);
			}
			linphone_address_unref(cAddr);
		}
	}
}

// -----------------------------------------------------------------------------

string RemoteConferenceEventHandler::getConfId() {
	L_D();
	return d->confId;
}

void RemoteConferenceEventHandler::setConferenceAddress (const Address &addr) {
	L_D();
	d->confAddr = addr;
}

LINPHONE_END_NAMESPACE
