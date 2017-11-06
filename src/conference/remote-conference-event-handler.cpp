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

	bool isFullState = (confInfo->getState() == "full");
	Address cleanedConfAddress = d->confAddress;
	cleanedConfAddress.clean();
	cleanedConfAddress.setPort(0);
	// Temporary workaround
	Address entityAddress(confInfo->getEntity().c_str());
	Address cleanedConfAddress2(cleanedConfAddress);
	cleanedConfAddress2.setDomain(entityAddress.getDomain());
	if (
		confInfo->getEntity() == cleanedConfAddress.asString() ||
		confInfo->getEntity() == cleanedConfAddress2.asString()
	) {
		if (
			confInfo->getConferenceDescription().present() &&
			confInfo->getConferenceDescription().get().getSubject().present()
		)
			d->listener->onSubjectChanged(tm, isFullState, confInfo->getConferenceDescription().get().getSubject().get());

		if (confInfo->getVersion().present())
			d->lastNotify = confInfo->getVersion().get();

		if (!confInfo->getUsers().present())
			return;

		for (const auto &user : confInfo->getUsers()->getUser()) {
			LinphoneAddress *cAddr = linphone_core_interpret_url(d->core, user.getEntity()->c_str());
			char *cAddrStr = linphone_address_as_string(cAddr);
			Address addr(cAddrStr);
			bctbx_free(cAddrStr);
			if (user.getState() == "deleted")
				d->listener->onParticipantRemoved(tm, isFullState, addr);
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
					d->listener->onParticipantAdded(tm, isFullState, addr);
				d->listener->onParticipantSetAdmin(tm, isFullState, addr, isAdmin);
				for (const auto &endpoint : user.getEndpoint()) {
					if (!endpoint.getEntity().present())
						break;

					Address gruu(endpoint.getEntity().get());
					if (endpoint.getState() == "deleted")
						d->listener->onParticipantDeviceRemoved(tm, isFullState, addr, gruu);
					else if (endpoint.getState() == "full")
						d->listener->onParticipantDeviceAdded(tm, isFullState, addr, gruu);

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