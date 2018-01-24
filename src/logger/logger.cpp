/*
 * logger.cpp
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

#include <chrono>

#include <bctoolbox/logging.h>

#include "object/base-object-p.h"

#include "logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class LoggerPrivate : public BaseObjectPrivate {
public:
	Logger::Level level;
	ostringstream os;
};

// -----------------------------------------------------------------------------

Logger::Logger (Level level) : BaseObject(*new LoggerPrivate) {
	L_D();
	d->level = level;
}

Logger::~Logger () {
	L_D();

	const string str = d->os.str();

	switch (d->level) {
		case Debug:
			#if DEBUG_LOGS
				bctbx_debug("%s", str.c_str());
			#endif // if DEBUG_LOGS
			break;
		case Info:
			bctbx_message("%s", str.c_str());
			break;
		case Warning:
			bctbx_warning("%s", str.c_str());
			break;
		case Error:
			bctbx_error("%s", str.c_str());
			break;
		case Fatal:
			bctbx_fatal("%s", str.c_str());
			break;
	}
}

ostringstream &Logger::getOutput () {
	L_D();
	return d->os;
}

// -----------------------------------------------------------------------------

class DurationLoggerPrivate : public BaseObjectPrivate {
public:
	unique_ptr<Logger> logger;

	chrono::high_resolution_clock::time_point start;
};

// -----------------------------------------------------------------------------

DurationLogger::DurationLogger (const string &label, Logger::Level level) : BaseObject(*new DurationLoggerPrivate) {
	L_D();

	d->logger.reset(new Logger(level));
	d->logger->getOutput() << "Duration of [" + label + "]: ";
	d->start = chrono::high_resolution_clock::now();

	Logger(level).getOutput() << "Start measurement of [" + label + "].";
}

DurationLogger::~DurationLogger () {
	L_D();

	chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
	d->logger->getOutput() << chrono::duration_cast<chrono::milliseconds>(end - d->start).count() << "ms.";
}

LINPHONE_END_NAMESPACE
