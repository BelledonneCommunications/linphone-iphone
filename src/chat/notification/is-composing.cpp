/*
 * is-composing.cpp
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

#include <utility>

#include "linphone/utils/utils.h"

#include "chat/chat-room/chat-room-p.h"
#include "chat/notification/is-composing.h"
#include "logger/logger.h"
#include "xml/is-composing.h"


// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

struct IsRemoteComposingData {
	IsRemoteComposingData (IsComposing *isComposingHandler, string uri)
		: isComposingHandler(isComposingHandler), uri(uri) {}

	IsComposing *isComposingHandler;
	string uri;
};

// -----------------------------------------------------------------------------

IsComposing::IsComposing (LinphoneCore *core, IsComposingListener *listener)
	: core(core), listener(listener) {}

IsComposing::~IsComposing () {
	stopTimers();
}

// -----------------------------------------------------------------------------

string IsComposing::createXml (bool isComposing) {
	Xsd::IsComposing::IsComposing node(isComposing ? "active" : "idle");
	if (isComposing)
		node.setRefresh(static_cast<unsigned long long>(lp_config_get_int(core->config, "sip", "composing_refresh_timeout", defaultRefreshTimeout)));

	stringstream ss;
	Xsd::XmlSchema::NamespaceInfomap map;
	map[""].name = "urn:ietf:params:xml:ns:im-iscomposing";
	Xsd::IsComposing::serializeIsComposing(ss, node, map, "UTF-8", Xsd::XmlSchema::Flags::dont_pretty_print);
	return ss.str();
}

void IsComposing::parse (const Address &remoteAddr, const string &text) {
	istringstream data(text);
	unique_ptr<Xsd::IsComposing::IsComposing> node(Xsd::IsComposing::parseIsComposing(data, Xsd::XmlSchema::Flags::dont_validate));
	if (!node)
		return;

	if (node->getState() == "active") {
		unsigned long long refresh = 0;
		if (node->getRefresh().present())
			refresh = node->getRefresh().get();
		startRemoteRefreshTimer(remoteAddr.asStringUriOnly(), refresh);
		listener->onIsRemoteComposingStateChanged(remoteAddr, true);
	} else if (node->getState() == "idle") {
		stopRemoteRefreshTimer(remoteAddr.asStringUriOnly());
		listener->onIsRemoteComposingStateChanged(remoteAddr, false);
	}
}

void IsComposing::startIdleTimer () {
	unsigned int duration = getIdleTimerDuration();
	if (!idleTimer) {
		idleTimer = core->sal->createTimer(idleTimerExpired, this,
			duration * 1000, "composing idle timeout");
	} else {
		belle_sip_source_set_timeout(idleTimer, duration * 1000);
	}
}

void IsComposing::startRefreshTimer () {
	unsigned int duration = getRefreshTimerDuration();
	if (!refreshTimer) {
		refreshTimer = core->sal->createTimer(refreshTimerExpired, this,
			duration * 1000, "composing refresh timeout");
	} else {
		belle_sip_source_set_timeout(refreshTimer, duration * 1000);
	}
}

void IsComposing::stopTimers () {
	stopIdleTimer();
	stopRefreshTimer();
	stopAllRemoteRefreshTimers();
}

// -----------------------------------------------------------------------------

void IsComposing::stopIdleTimer () {
	if (idleTimer) {
		if (core && core->sal)
			core->sal->cancelTimer(idleTimer);
		belle_sip_object_unref(idleTimer);
		idleTimer = nullptr;
	}
}

void IsComposing::stopRefreshTimer () {
	if (refreshTimer) {
		if (core && core->sal)
			core->sal->cancelTimer(refreshTimer);
		belle_sip_object_unref(refreshTimer);
		refreshTimer = nullptr;
	}
}

void IsComposing::stopRemoteRefreshTimer (const string &uri) {
	auto it = remoteRefreshTimers.find(uri);
	if (it != remoteRefreshTimers.end())
		stopRemoteRefreshTimer(it);
}

// -----------------------------------------------------------------------------

unsigned int IsComposing::getIdleTimerDuration () {
	int idleTimerDuration = lp_config_get_int(core->config, "sip", "composing_idle_timeout", defaultIdleTimeout);
	return idleTimerDuration < 0 ? 0 : static_cast<unsigned int>(idleTimerDuration);
}

unsigned int IsComposing::getRefreshTimerDuration () {
	int refreshTimerDuration = lp_config_get_int(core->config, "sip", "composing_refresh_timeout", defaultRefreshTimeout);
	return refreshTimerDuration < 0 ? 0 : static_cast<unsigned int>(refreshTimerDuration);
}

unsigned int IsComposing::getRemoteRefreshTimerDuration () {
	int remoteRefreshTimerDuration = lp_config_get_int(core->config, "sip", "composing_remote_refresh_timeout", defaultRemoteRefreshTimeout);
	return remoteRefreshTimerDuration < 0 ? 0 : static_cast<unsigned int>(remoteRefreshTimerDuration);
}

int IsComposing::idleTimerExpired () {
	stopRefreshTimer();
	stopIdleTimer();
	listener->onIsComposingStateChanged(false);
	return BELLE_SIP_STOP;
}

int IsComposing::refreshTimerExpired () {
	listener->onIsComposingRefreshNeeded();
	return BELLE_SIP_CONTINUE;
}

int IsComposing::remoteRefreshTimerExpired (const string &uri) {
	stopRemoteRefreshTimer(uri);
	listener->onIsRemoteComposingStateChanged(Address(uri), false);
	return BELLE_SIP_STOP;
}

void IsComposing::startRemoteRefreshTimer (const string &uri, unsigned long long refresh) {
	unsigned int duration = getRemoteRefreshTimerDuration();
	if (refresh != 0)
		duration = static_cast<unsigned int>(refresh);
	auto it = remoteRefreshTimers.find(uri);
	if (it == remoteRefreshTimers.end()) {
		IsRemoteComposingData *data = new IsRemoteComposingData(this, uri);
		belle_sip_source_t *timer = core->sal->createTimer(remoteRefreshTimerExpired, data,
			duration * 1000, "composing remote refresh timeout");
		pair<string, belle_sip_source_t *> p(uri, timer);
		remoteRefreshTimers.insert(p);
	} else
		belle_sip_source_set_timeout(it->second, duration * 1000);
}

void IsComposing::stopAllRemoteRefreshTimers () {
	for (auto it = remoteRefreshTimers.begin(); it != remoteRefreshTimers.end();)
		it = stopRemoteRefreshTimer(it);
}

unordered_map<string, belle_sip_source_t *>::iterator IsComposing::stopRemoteRefreshTimer (const unordered_map<string, belle_sip_source_t *>::const_iterator it) {
	belle_sip_source_t *timer = it->second;
	if (core && core->sal) {
		core->sal->cancelTimer(timer);
		delete reinterpret_cast<IsRemoteComposingData *>(belle_sip_source_get_user_data(timer));
	}
	belle_sip_object_unref(timer);
	return remoteRefreshTimers.erase(it);
}

int IsComposing::idleTimerExpired (void *data, unsigned int revents) {
	IsComposing *d = reinterpret_cast<IsComposing *>(data);
	return d->idleTimerExpired();
}

int IsComposing::refreshTimerExpired (void *data, unsigned int revents) {
	IsComposing *d = reinterpret_cast<IsComposing *>(data);
	return d->refreshTimerExpired();
}

int IsComposing::remoteRefreshTimerExpired (void *data, unsigned int revents) {
	IsRemoteComposingData *d = reinterpret_cast<IsRemoteComposingData *>(data);
	int result = d->isComposingHandler->remoteRefreshTimerExpired(d->uri);
	return result;
}

LINPHONE_END_NAMESPACE
