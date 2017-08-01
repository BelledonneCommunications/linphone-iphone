/*
 * conference-event-package.cpp
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

#include "conference-event-package.h"
#include "conference-info.hxx"
#include "private.h"

using namespace std;
using namespace conference_info;
using namespace LinphonePrivate;

class LinphonePrivate::Conference::ConferenceEventPackagePrivate : public ObjectPrivate {
public:
	LinphoneCore *lc;
	ConferenceListener *listener;
	LinphoneAddress *confAddr;
	string confId;
	LinphoneEvent *lev;
};

Conference::ConferenceEventPackage::ConferenceEventPackage(LinphoneCore *lc, ConferenceListener *listener, LinphoneAddress *confAddr) : Object(new Conference::ConferenceEventPackagePrivate) {
	L_D(ConferenceEventPackage);
	xercesc::XMLPlatformUtils::Initialize();
	d->lc = lc;
	d->listener = listener;
	d->confAddr = confAddr;
	linphone_address_ref(confAddr);
	d->lev = NULL;
}

Conference::ConferenceEventPackage::~ConferenceEventPackage() {
	L_D(ConferenceEventPackage);
	xercesc::XMLPlatformUtils::Terminate();
	linphone_address_unref(d->confAddr);
	if(d->lev) linphone_event_unref(d->lev);
}

void Conference::ConferenceEventPackage::subscribe(string confId) {
	L_D(ConferenceEventPackage);
	d->confId = confId;
	d->lev = linphone_core_create_subscribe(d->lc, d->confAddr, "Conference", 600);
	linphone_event_ref(d->lev);
	linphone_event_set_internal(d->lev, TRUE);
	linphone_event_set_user_data(d->lev, this);
	linphone_event_add_custom_header(d->lev, "Conf-id", d->confId.c_str()); // TODO : ???
	linphone_event_send_subscribe(d->lev, NULL);
}

void Conference::ConferenceEventPackage::unsubscribe() {
	L_D(ConferenceEventPackage);
	linphone_event_terminate(d->lev);
}

void Conference::ConferenceEventPackage::notifyReceived(const char *xmlBody) {
	L_D(ConferenceEventPackage);
	istringstream data(xmlBody);
	unique_ptr<Conference_type> confInfo = parseConference_info(data, xml_schema::Flags::dont_validate);
	if(strcmp(confInfo->getEntity().c_str(), linphone_address_as_string(d->confAddr)) == 0) {
		for (const auto &user : confInfo->getUsers()->getUser()) {
			LinphoneAddress *addr = linphone_core_interpret_url(d->lc, user.getEntity()->c_str());
			if(user.getState() == "deleted") {
				d->listener->participantRemoved(addr);
			} else {
				bool isAdmin = false;
				if(user.getRoles()) {
					for (const auto &entry : user.getRoles()->getEntry()) {
						if (entry == "admin") {
							isAdmin = true;
							break;
						}
					}
				}
				if(user.getState() == "full") {
					d->listener->participantAdded(addr);
				}
				d->listener->participantSetAdmin(addr, isAdmin);
			}
			linphone_address_unref(addr);
		}
	}
}

string Conference::ConferenceEventPackage::getConfId() {
	L_D(ConferenceEventPackage);
	return d->confId;
}
