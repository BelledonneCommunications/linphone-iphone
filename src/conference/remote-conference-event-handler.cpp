/*
 * remote-conference-event-handler.cpp
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

#include "address/simple-address.h"
#include "private.h"
#include "logger/logger.h"
#include "remote-conference-event-handler-p.h"
#include "linphone/utils/utils.h"
#include "xml/conference-info.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

using namespace Xsd::ConferenceInfo;

// -----------------------------------------------------------------------------

RemoteConferenceEventHandler::RemoteConferenceEventHandler (LinphoneCore *core, ConferenceListener *listener)
	: Object(*new RemoteConferenceEventHandlerPrivate) {
	L_D();
	xercesc::XMLPlatformUtils::Initialize();
	d->core = core;
	d->listener = listener;
	// TODO : d->lastNotify = lastNotify
}

RemoteConferenceEventHandler::~RemoteConferenceEventHandler () {
	xercesc::XMLPlatformUtils::Terminate();
}

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandler::subscribe (const Address &addr) {
	L_D();
	d->confAddress = addr;
	LinphoneAddress *lAddr = linphone_address_new(d->confAddress.asString().c_str());
	d->lev = linphone_core_create_subscribe(d->core, lAddr, "conference", 600);
	linphone_event_add_custom_header(d->lev, "Last-Notify-Version", Utils::toString(d->lastNotify).c_str());
	linphone_address_unref(lAddr);
	linphone_event_set_internal(d->lev, TRUE);
	linphone_event_set_user_data(d->lev, this);
	linphone_event_send_subscribe(d->lev, nullptr);
}

void RemoteConferenceEventHandler::unsubscribe () {
	L_D();
	if (d->lev) {
		linphone_event_terminate(d->lev);
		d->lev = nullptr;
	}
}

void RemoteConferenceEventHandler::notifyReceived (const string &xmlBody) {
	L_D();
	lInfo() << "NOTIFY received for conference " << d->confAddress.asString();
	istringstream data(xmlBody);
	unique_ptr<ConferenceType> confInfo = parseConferenceInfo(data, Xsd::XmlSchema::Flags::dont_validate);
	time_t tm = time(nullptr);
	if (confInfo->getConferenceDescription()->getFreeText().present())
		tm = static_cast<time_t>(Utils::stoll(confInfo->getConferenceDescription()->getFreeText().get()));

	bool isFullState = (confInfo->getState() == StateType::full);
	SimpleAddress simpleConfAddress(d->confAddress);
	// Temporary workaround
	SimpleAddress entityAddress(confInfo->getEntity().c_str());
	SimpleAddress simpleConfAddress2(simpleConfAddress);
	simpleConfAddress2.setDomain(entityAddress.getDomain());
	if ((entityAddress == simpleConfAddress) || (entityAddress == simpleConfAddress2)) {
		if (
			confInfo->getConferenceDescription().present() &&
			confInfo->getConferenceDescription().get().getSubject().present()
		)
			d->listener->onSubjectChanged(make_shared<ConferenceSubjectEvent>(
				tm,
				isFullState,
				d->confAddress,
				d->lastNotify,
				confInfo->getConferenceDescription().get().getSubject().get()
			));
		if (confInfo->getVersion().present())
			d->lastNotify = confInfo->getVersion().get();

		if (!confInfo->getUsers().present())
			return;

		for (const auto &user : confInfo->getUsers()->getUser()) {
			LinphoneAddress *cAddr = linphone_core_interpret_url(d->core, user.getEntity()->c_str());
			char *cAddrStr = linphone_address_as_string(cAddr);
			Address addr(cAddrStr);
			bctbx_free(cAddrStr);
			if (user.getState() == StateType::deleted) {
				d->listener->onParticipantRemoved(make_shared<ConferenceParticipantEvent>(
					EventLog::Type::ConferenceParticipantRemoved,
					tm,
					isFullState,
					d->confAddress,
					d->lastNotify,
					addr
				));
			} else {
				bool isAdmin = false;
				if (user.getRoles()) {
					for (const auto &entry : user.getRoles()->getEntry()) {
						if (entry == "admin") {
							isAdmin = true;
							break;
						}
					}
				}
				if (user.getState() == StateType::full) {
					d->listener->onParticipantAdded(make_shared<ConferenceParticipantEvent>(
						EventLog::Type::ConferenceParticipantAdded,
						tm,
						isFullState,
						d->confAddress,
						d->lastNotify,
						addr
					));
				}
				d->listener->onParticipantSetAdmin(make_shared<ConferenceParticipantEvent>(
					isAdmin ? EventLog::Type::ConferenceParticipantSetAdmin : EventLog::Type::ConferenceParticipantUnsetAdmin,
					tm,
					isFullState,
					d->confAddress,
					d->lastNotify,
					addr
				));
				for (const auto &endpoint : user.getEndpoint()) {
					if (!endpoint.getEntity().present())
						break;

					Address gruu(endpoint.getEntity().get());
					if (endpoint.getState() == StateType::deleted) {
						d->listener->onParticipantDeviceRemoved(make_shared<ConferenceParticipantDeviceEvent>(
							EventLog::Type::ConferenceParticipantDeviceRemoved,
							tm,
							isFullState,
							d->confAddress,
							d->lastNotify,
							addr,
							gruu
						));
					} else if (endpoint.getState() == StateType::full) {
						d->listener->onParticipantDeviceAdded(make_shared<ConferenceParticipantDeviceEvent>(
							EventLog::Type::ConferenceParticipantDeviceAdded,
							tm,
							isFullState,
							d->confAddress,
							d->lastNotify,
							addr,
							gruu
						));
					}
				}
			}
			linphone_address_unref(cAddr);
		}
	}
}

// -----------------------------------------------------------------------------

const Address &RemoteConferenceEventHandler::getConfAddress () const {
	L_D();
	return d->confAddress;
}

unsigned int RemoteConferenceEventHandler::getLastNotify () const {
	L_D();
	return d->lastNotify;
};

LINPHONE_END_NAMESPACE
