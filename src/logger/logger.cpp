/*
 * logger.cpp
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

#include "linphone/core.h"

#include "object/object-p.h"

#include "logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class LoggerPrivate : public ObjectPrivate {
public:
	Logger::Level level;
	ostringstream os;
};

// -----------------------------------------------------------------------------

Logger::Logger (Level level) : Object(*new LoggerPrivate) {
	L_D(Logger);
	d->level = level;
}

Logger::~Logger () {
	L_D(Logger);

	d->os << endl;
	const string str = d->os.str();

	switch (d->level) {
		case Debug:
			#if DEBUG_LOGS
				ms_debug("%s", str.c_str());
			#endif // if DEBUG_LOGS
			break;
		case Info:
			ms_message("%s", str.c_str());
			break;
		case Warning:
			ms_warning("%s", str.c_str());
			break;
		case Error:
			ms_error("%s", str.c_str());
			break;
		case Fatal:
			ms_fatal("%s", str.c_str());
			break;
	}
}

ostringstream &Logger::getOutput () {
	L_D(Logger);
	return d->os;
}

LINPHONE_END_NAMESPACE
