/*
 * core.cpp
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

#include <mediastreamer2/mscommon.h>
#include <xercesc/util/PlatformUtils.hpp>

#include "address/address-p.h"
#include "call/call.h"
#include "core/core-listener.h"
#include "core/core-p.h"
#include "logger/logger.h"
#include "paths/paths.h"

// TODO: Remove me later.
#include "c-wrapper/c-wrapper.h"
#include "private.h"

#define LINPHONE_DB "linphone.db"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void CorePrivate::init () {
	L_Q();
	mainDb.reset(new MainDb(q->getSharedFromThis()));

	AbstractDb::Backend backend;
	string uri = L_C_TO_STRING(lp_config_get_string(linphone_core_get_config(L_GET_C_BACK_PTR(q)), "storage", "uri", nullptr));
	if (!uri.empty())
		backend = strcmp(lp_config_get_string(linphone_core_get_config(L_GET_C_BACK_PTR(q)), "storage", "backend", nullptr), "mysql") == 0
			? MainDb::Mysql
			: MainDb::Sqlite3;
	else {
		backend = AbstractDb::Sqlite3;
		uri = q->getDataPath() + LINPHONE_DB;
	}

	lInfo() << "Opening linphone database: " << uri;
	if (!mainDb->connect(backend, uri))
		lFatal() << "Unable to open linphone database.";

	loadChatRooms();
}

void CorePrivate::registerListener (CoreListener *listener) {
	listeners.push_back(listener);
}

void CorePrivate::unregisterListener (CoreListener *listener) {
	listeners.remove(listener);
}

void CorePrivate::uninit () {
	L_Q();
	while (!calls.empty()) {
		calls.front()->terminate();
		linphone_core_iterate(L_GET_C_BACK_PTR(q));
		ms_usleep(10000);
	}

	chatRooms.clear();
	chatRoomsById.clear();
	noCreatedClientGroupChatRooms.clear();

	AddressPrivate::clearSipAddressesCache();
}

// -----------------------------------------------------------------------------

void CorePrivate::notifyNetworkReachable (bool sipNetworkReachable, bool mediaNetworkReachable) {
	for (const auto &listener : listeners)
		listener->onNetworkReachable(sipNetworkReachable, mediaNetworkReachable);
}

void CorePrivate::notifyRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const string &message) {
	for (const auto &listener : listeners)
		listener->onRegistrationStateChanged(cfg, state, message);
}

// =============================================================================

Core::Core () : Object(*new CorePrivate) {
	xercesc::XMLPlatformUtils::Initialize();
}

Core::~Core () {
	lInfo() << "Destroying core: " << this;
	xercesc::XMLPlatformUtils::Terminate();
}

shared_ptr<Core> Core::create (LinphoneCore *cCore) {
	// Do not use `make_shared` => Private constructor.
	shared_ptr<Core> core = shared_ptr<Core>(new Core);
	L_SET_CPP_PTR_FROM_C_OBJECT(cCore, core);
	return core;
}

LinphoneCore *Core::getCCore () const {
	return L_GET_C_BACK_PTR(this);
}

// -----------------------------------------------------------------------------
// Paths.
// -----------------------------------------------------------------------------

string Core::getDataPath() const {
	return Paths::getPath(Paths::Data, static_cast<PlatformHelpers *>(L_GET_C_BACK_PTR(this)->platform_helper));
}

string Core::getConfigPath() const {
	return Paths::getPath(Paths::Config, static_cast<PlatformHelpers *>(L_GET_C_BACK_PTR(this)->platform_helper));
}

LINPHONE_END_NAMESPACE
