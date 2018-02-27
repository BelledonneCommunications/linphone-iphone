/*
 * db-exception-handler.h
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

#ifndef _L_DB_EXCEPTION_HANDLER_H_
#define _L_DB_EXCEPTION_HANDLER_H_

#include <soci/soci.h>

#include "db/main-db-p.h"
#include "logger/logger.h"

// =============================================================================

#define L_DB_EXCEPTION_HANDLER_C(CONTEXT) \
	LinphonePrivate::DbExceptionHandlerInfo().set(__func__, CONTEXT) * [&](SmartTransaction &tr)

#define L_DB_EXCEPTION_HANDLER L_DB_EXCEPTION_HANDLER_C(this)

LINPHONE_BEGIN_NAMESPACE

class SmartTransaction {
public:
	SmartTransaction (soci::session *session, const char *name) :
	mSession(session), mName(name), mIsCommitted(false) {
		lInfo() << "Start transaction " << this << " in MainDb::" << mName << ".";
		mSession->begin();
	}

	~SmartTransaction () {
		if (!mIsCommitted) {
			lInfo() << "Rollback transaction " << this << " in MainDb::" << mName << ".";
			mSession->rollback();
		}
	}

	void commit () {
		if (mIsCommitted) {
			lError() << "Transaction " << this << " in MainDb::" << mName << " already committed!!!";
			return;
		}

		lInfo() << "Commit transaction " << this << " in MainDb::" << mName << ".";
		mIsCommitted = true;
		mSession->commit();
	}

private:
	soci::session *mSession;
	const char *mName;
	bool mIsCommitted;

	L_DISABLE_COPY(SmartTransaction);
};

struct DbExceptionHandlerInfo {
	DbExceptionHandlerInfo &set (const char *_name, const MainDb *_mainDb) {
		name = _name;
		mainDb = const_cast<MainDb *>(_mainDb);
		return *this;
	}

	const char *name = nullptr;
	MainDb *mainDb = nullptr;
};

template<typename Function>
class DbExceptionHandler {
	using InternalReturnType = typename std::remove_reference<
		decltype(std::declval<Function>()(std::declval<SmartTransaction &>()))
	>::type;

public:
	using ReturnType = typename std::conditional<
		std::is_same<InternalReturnType, void>::value,
		bool,
		InternalReturnType
	>::type;

	DbExceptionHandler (DbExceptionHandlerInfo &info, Function &&function) : mFunction(std::move(function)) {
		MainDb *mainDb = info.mainDb;
		const char *name = info.name;
		soci::session *session = mainDb->getPrivate()->dbSession.getBackendSession();

		try {
			SmartTransaction tr(session, name);
			mResult = exec<InternalReturnType>(tr);
		} catch (const soci::soci_error &e) {
			lWarning() << "Catched exception in MainDb::" << name << "(" << e.what() << ").";
			soci::soci_error::error_category category = e.get_error_category();
			if (
				(category == soci::soci_error::connection_error || category == soci::soci_error::unknown) &&
				mainDb->forceReconnect()
			) {
				try {
					SmartTransaction tr(session, name);
					mResult = exec<InternalReturnType>(tr);
				} catch (const std::exception &e) {
					lError() << "Unable to execute query after reconnect in MainDb::" << name << "(" << e.what() << ").";
				}
				return;
			}
			lError() << "Unhandled [" << getErrorCategoryAsString(category) << "] exception in MainDb::" <<
				name << ": `" << e.what() << "`.";
		} catch (const std::exception &e) {
			lError() << "Unhandled generic exception in MainDb::" << name << ": `" << e.what() << "`.";
		}
	}

	DbExceptionHandler (DbExceptionHandler &&dbExceptionHandler) : mFunction(std::move(dbExceptionHandler.mFunction)) {}

	operator ReturnType () const {
		return mResult;
	}

private:
	// Exec function with no return type.
	template<typename T>
	typename std::enable_if<std::is_same<T, void>::value, bool>::type exec (SmartTransaction &tr) const {
		mFunction(tr);
		return true;
	}

	// Exec function with return type.
	template<typename T>
	typename std::enable_if<!std::is_same<T, void>::value, T>::type exec (SmartTransaction &tr) const {
		return mFunction(tr);
	}

	static const char *getErrorCategoryAsString (soci::soci_error::error_category category) {
		switch (category) {
			case soci::soci_error::connection_error:
				return "CONNECTION ERROR";
			case soci::soci_error::invalid_statement:
				return "INVALID STATEMENT";
			case soci::soci_error::no_privilege:
				return "NO PRIVILEGE";
			case soci::soci_error::no_data:
				return "NO DATA";
			case soci::soci_error::constraint_violation:
				return "CONSTRAINT VIOLATION";
			case soci::soci_error::unknown_transaction_state:
				return "UNKNOWN TRANSACTION STATE";
			case soci::soci_error::system_error:
				return "SYSTEM ERROR";
			case soci::soci_error::unknown:
				return "UNKNOWN";
		}

		// Unreachable.
		L_ASSERT(false);
		return nullptr;
	}

	Function mFunction;
	ReturnType mResult{};

	L_DISABLE_COPY(DbExceptionHandler);
};

template<typename Function>
typename DbExceptionHandler<Function>::ReturnType operator* (DbExceptionHandlerInfo &info, Function &&function) {
	return DbExceptionHandler<Function>(info, std::forward<Function>(function));
}

LINPHONE_END_NAMESPACE

#endif // ifndef _L_DB_EXCEPTION_HANDLER_H_
