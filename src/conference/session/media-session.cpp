/*
 * media-session.cpp
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

#include "call-session-p.h"

#include "media-session.h"

#include <mediastreamer2/mediastream.h>

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

class MediaSessionPrivate : public CallSessionPrivate {
public:
	AudioStream *audioStream = nullptr;
	VideoStream *videoStream = nullptr;
	TextStream *textStream = nullptr;
	IceSession *iceSession = nullptr;
};

// =============================================================================

MediaSession::MediaSession () : CallSession(*new MediaSessionPrivate) {}

LINPHONE_END_NAMESPACE
