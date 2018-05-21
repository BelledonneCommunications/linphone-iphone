/*
 * background-task.h
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

#ifndef _L_BACKGROUND_TASK_H_
#define _L_BACKGROUND_TASK_H_

#include <string>

#include "linphone/utils/general.h"
#include "sal/sal.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Sal;
class Core;

class BackgroundTask {
public:
	BackgroundTask () {}
	BackgroundTask (const std::string &name) : mName(name) {}
	virtual ~BackgroundTask () { stop(); }

	void setName (const std::string &name) { mName = name; }

	const std::string &getName () const { return mName; }

	/**
	 * Start a long running task for at most max_duration_seconds, after which it is automatically terminated
	 */
	void start (const std::shared_ptr<Core> &core, int maxDurationSeconds = 15 * 60); // 15 min by default, like on iOS
	void stop ();

protected:
	virtual void handleTimeout ();

private:
	static int sHandleSalTimeout(void *data, unsigned int events);
	static void sHandleTimeout(void *data);
	void handleSalTimeout();

	belle_sip_source_t *mTimeout = nullptr;
	Sal *mSal = nullptr;
	std::string mName;
	unsigned long mId = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_BACKGROUND_TASK_H_
