/*
 * logger.h
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

#ifndef _L_LOGGER_H_
#define _L_LOGGER_H_

#include <sstream>

#include "object/base-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class LoggerPrivate;

class LINPHONE_PUBLIC Logger : public BaseObject {
public:
	enum Level {
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	explicit Logger (Level level);
	~Logger ();

	std::ostringstream &getOutput ();

private:
	L_DECLARE_PRIVATE(Logger);
	L_DISABLE_COPY(Logger);
};

class DurationLoggerPrivate;

class DurationLogger : public BaseObject {
public:
	DurationLogger (const std::string &label, Logger::Level level = Logger::Info);
	~DurationLogger ();

private:
	L_DECLARE_PRIVATE(DurationLogger);
	L_DISABLE_COPY(DurationLogger);
};

LINPHONE_END_NAMESPACE

#define lDebug() LinphonePrivate::Logger(LinphonePrivate::Logger::Debug).getOutput()
#define lInfo() LinphonePrivate::Logger(LinphonePrivate::Logger::Info).getOutput()
#define lWarning() LinphonePrivate::Logger(LinphonePrivate::Logger::Warning).getOutput()
#define lError() LinphonePrivate::Logger(LinphonePrivate::Logger::Error).getOutput()
#define lFatal() LinphonePrivate::Logger(LinphonePrivate::Logger::Fatal).getOutput()

#define L_BEGIN_LOG_EXCEPTION try {

#define L_END_LOG_EXCEPTION \
	} catch (const exception &e) { \
		lWarning() << "Error: " << e.what(); \
	}

#endif // ifndef _L_LOGGER_H_
