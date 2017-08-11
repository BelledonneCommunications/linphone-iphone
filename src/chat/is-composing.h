/*
 * is-composing.h
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

#ifndef _IS_COMPOSING_H_
#define _IS_COMPOSING_H_

#include <string>

#include "is-composing-listener.h"
#include "utils/general.h"

#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class IsComposing {
public:
	IsComposing (LinphoneCore *core, IsComposingListener *listener);
	~IsComposing ();

	std::string marshal (bool isComposing);
	void parse (const std::string &content);
	void startIdleTimer ();
	void startRefreshTimer ();
	void startRemoteRefreshTimer ();
	void stopComposing ();
	void stopIdleTimer ();
	void stopRefreshTimer ();
	void stopRemoteRefreshTimer ();
	void stopTimers ();

private:
	int getIdleTimerDuration ();
	int getRefreshTimerDuration ();
	int getRemoteRefreshTimerDuration ();
	void parse (xmlparsing_context_t *xmlCtx);
	int idleTimerExpired (unsigned int revents);
	int refreshTimerExpired (unsigned int revents);
	int remoteRefreshTimerExpired (unsigned int revents);

	static int idleTimerExpired (void *data, unsigned int revents);
	static int refreshTimerExpired (void *data, unsigned int revents);
	static int remoteRefreshTimerExpired (void *data, unsigned int revents);

private:
	static const int defaultIdleTimeout = 15;
	static const int defaultRefreshTimeout = 60;
	static const int defaultRemoteRefreshTimeout = 120;
	static const std::string isComposingPrefix;

	LinphoneCore *core = nullptr;
	IsComposingListener *listener = nullptr;
	belle_sip_source_t *remoteRefreshTimer = nullptr;
	belle_sip_source_t *idleTimer = nullptr;
	belle_sip_source_t *refreshTimer = nullptr;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _IS_COMPOSING_H_
