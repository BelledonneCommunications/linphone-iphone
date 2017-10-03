/*
 * hacks.h
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

#ifndef _HACKS_H_
#define _HACKS_H_

#include <string>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

// This class has the purpose to centralize the temporary hacks so that they
// can be located more easily.
class Hacks {
public:
	static bool contactHasParam(const std::string &contact, const std::string &paramName);

private:
	Hacks () = default;

	L_DISABLE_COPY(Hacks);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _HACKS_H_
