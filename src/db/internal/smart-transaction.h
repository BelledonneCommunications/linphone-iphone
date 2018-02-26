/*
 * smart-transaction.h
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

#ifndef _L_SMART_TRANSACTION_H_
#define _L_SMART_TRANSACTION_H_

#include <soci/soci.h>

#include "logger/logger.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class SmartTransaction {
public:
	SmartTransaction (soci::session *session, const char *name) : mTransaction(*session), mName(name) {
		lInfo() << "Start transaction " << this << " in `" << mName << "`.";
	}

	~SmartTransaction () {
		lInfo() << "Rollback transaction " << this << " in `" << mName << "`.";
	}

	void commit () {
		lInfo() << "Commit transaction " << this << " in `" << mName << "`.";
		mTransaction.commit();
	}

private:
	soci::transaction mTransaction;
	const char *mName;

	L_DISABLE_COPY(SmartTransaction);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SMART_TRANSACTION_H_
