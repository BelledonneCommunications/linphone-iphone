/*
 * background-task.cpp
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

#include "background-task.h"
#include "c-wrapper/internal/c-sal.h"
#include "logger/logger.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

static void notifier (void *context) {
	static_cast<BackgroundTask *>(context)->handleTimeout();
}

void BackgroundTask::start () {
	if (mName.empty()) {
		lError() << "No name was set on background task.";
		return;
	}

	unsigned long newId = sal_begin_background_task(mName.c_str(), notifier, this);
	lInfo() << "Starting background task [" << newId << "] with name : [" << mName << "].";
	stop();
	mId = newId;
}

void BackgroundTask::stop () {
	if (mId == 0)
		return;

	lInfo() << "Ending background task [" << mId << "] with name : [" << mName << "].";
	sal_end_background_task(mId);
	mId = 0;
}

void BackgroundTask::handleTimeout () {
	lWarning() << "Background task [" << mId << "] with name : [" << mName << "] has expired before completion...";
	stop();
}

LINPHONE_END_NAMESPACE
