/*
 * logger.h
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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <sstream>

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class LoggerPrivate;

class LINPHONE_PUBLIC Logger : public Object {
public:
	enum Level {
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	Logger (Level level);
	~Logger ();

	std::ostringstream &getOutput ();

private:
	L_DECLARE_PRIVATE(Logger);
	L_DISABLE_COPY(Logger);
};

LINPHONE_END_NAMESPACE

#define lDebug() LINPHONE_NAMESPACE::Logger(Logger::Debug).getOutput()
#define lInfo() LINPHONE_NAMESPACE::Logger(Logger::Info).getOutput()
#define lWarning() LINPHONE_NAMESPACE::Logger(Logger::Warning).getOutput()
#define lError() LINPHONE_NAMESPACE::Logger(Logger::Error).getOutput()
#define lFatal() LINPHONE_NAMESPACE::Logger(Logger::Fatal).getOutput()

#define L_BEGIN_LOG_EXCEPTION try {
	#define L_END_LOG_EXCEPTION \
} catch (const exception &e) { \
	lWarning() << "Error: " << e.what(); \
}

#endif // ifndef _LOGGER_H_
