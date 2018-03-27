/*
 * app-data-container.h
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

#ifndef _L_APP_DATA_CONTAINER_H_
#define _L_APP_DATA_CONTAINER_H_

#include <string>
#include <unordered_map>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class AppDataContainerPrivate;

class LINPHONE_PUBLIC AppDataContainer {
public:
	AppDataContainer ();
	AppDataContainer (const AppDataContainer &other);
	virtual ~AppDataContainer ();

	AppDataContainer &operator= (const AppDataContainer &other);

	const std::unordered_map<std::string, std::string> &getAppDataMap () const;

	const std::string &getAppData (const std::string &name) const;
	void setAppData (const std::string &name, const std::string &appData);
	void setAppData (const std::string &name, std::string &&appData);

private:
	AppDataContainerPrivate *mPrivate = nullptr;

	L_DECLARE_PRIVATE(AppDataContainer);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_APP_DATA_CONTAINER_H_
