/*
 * imdn.h
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

#ifndef _IMDN_H_
#define _IMDN_H_

#include <string>

#include "chat-room.h"
#include "utils/general.h"

#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Imdn {
public:
	static void parse (ChatRoom &cr, const std::string &content);

private:
	static void parse (ChatRoom &cr, xmlparsing_context_t *xmlCtx);

private:
	static const std::string imdnPrefix;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _IMDN_H_
