/*
 * core.cpp
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

#include <algorithm>

#include "address/address.h"
#include "chat/chat-room/basic-chat-room.h"
#include "core-p.h"
#include "db/main-db.h"
#include "linphone/core.h"
#include "object/object-p.h"

#include "core.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

Core::Core (LinphoneCore *cCore) : Object(*new CorePrivate) {
	L_D();
	d->cCore = cCore;
	const char *uri = lp_config_get_string(linphone_core_get_config(d->cCore), "server", "db_uri", NULL);
	if (uri) {
		AbstractDb::Backend backend =
			strcmp(lp_config_get_string(linphone_core_get_config(d->cCore), "server", "db_backend", NULL), "mysql") == 0
			? MainDb::Mysql
			: MainDb::Sqlite3;
		d->mainDb.connect(backend, uri);
	}	else {
		// TODO
		// d->mainDb.connect(MainDb::Sqlite3, linphone_factory_get_writable_dir()/linphone.db);
	}
}

// -----------------------------------------------------------------------------

shared_ptr<ChatRoom> Core::createClientGroupChatRoom (const string &subject) {
	// TODO.
	return shared_ptr<ChatRoom>();
}

shared_ptr<ChatRoom> Core::getOrCreateChatRoom (const string &peerAddress, bool isRtt) const {
	return shared_ptr<ChatRoom>();
}

const list<shared_ptr<ChatRoom>> &Core::getChatRooms () const {
	L_D();
	return d->chatRooms;
}

// -----------------------------------------------------------------------------

LINPHONE_END_NAMESPACE
