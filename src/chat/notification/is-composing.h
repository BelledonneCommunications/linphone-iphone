/*
 * is-composing.h
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

#ifndef _L_IS_COMPOSING_H_
#define _L_IS_COMPOSING_H_

#include <unordered_map>

#include "linphone/utils/general.h"

#include "chat/notification/is-composing-listener.h"

// TODO: Remove me later.
#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class IsComposing {
public:
	IsComposing (LinphoneCore *core, IsComposingListener *listener);
	~IsComposing ();

	std::string createXml (bool isComposing);
	void parse (const Address &remoteAddr, const std::string &content);
	void startIdleTimer ();
	void startRefreshTimer ();
	void stopIdleTimer ();
	void stopRefreshTimer ();
	void stopRemoteRefreshTimer (const std::string &uri);
	void stopTimers ();

private:
	unsigned int getIdleTimerDuration ();
	unsigned int getRefreshTimerDuration ();
	unsigned int getRemoteRefreshTimerDuration ();
	int idleTimerExpired ();
	int refreshTimerExpired ();
	int remoteRefreshTimerExpired (const std::string &uri);
	void startRemoteRefreshTimer (const std::string &uri, unsigned long long refresh);
	void stopAllRemoteRefreshTimers ();
	std::unordered_map<std::string, belle_sip_source_t *>::iterator stopRemoteRefreshTimer (const std::unordered_map<std::string, belle_sip_source_t *>::const_iterator it);

	static int idleTimerExpired (void *data, unsigned int revents);
	static int refreshTimerExpired (void *data, unsigned int revents);
	static int remoteRefreshTimerExpired (void *data, unsigned int revents);

private:
	static const int defaultIdleTimeout = 15;
	static const int defaultRefreshTimeout = 60;
	static const int defaultRemoteRefreshTimeout = 120;

	LinphoneCore *core = nullptr;
	IsComposingListener *listener = nullptr;
	std::unordered_map<std::string, belle_sip_source_t *>remoteRefreshTimers;
	belle_sip_source_t *idleTimer = nullptr;
	belle_sip_source_t *refreshTimer = nullptr;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_IS_COMPOSING_H_
