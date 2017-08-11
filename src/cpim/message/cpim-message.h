/*
 * cpim-message.h
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

#ifndef _CPIM_MESSAGE_H_
#define _CPIM_MESSAGE_H_

#include "cpim/header/cpim-core-headers.h"
#include "cpim/header/cpim-generic-header.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

namespace Cpim {
	class MessagePrivate;

	class LINPHONE_PUBLIC Message : public Object {
	public:
		Message ();

		typedef std::shared_ptr<std::list<std::shared_ptr<const Cpim::Header> > > HeaderList;

		HeaderList getCpimHeaders () const;
		bool addCpimHeader (const Header &cpimHeader);
		void removeCpimHeader (const Header &cpimHeader);

		HeaderList getMessageHeaders () const;
		bool addMessageHeader (const Header &messageHeader);
		void removeMessageHeader (const Header &messageHeader);

		std::string getContent () const;
		bool setContent (const std::string &content);

		bool isValid () const;

		std::string asString () const;

		static std::shared_ptr<const Message> createFromString (const std::string &str);

	private:
		L_DECLARE_PRIVATE(Message);
		L_DISABLE_COPY(Message);
	};
}

LINPHONE_END_NAMESPACE

#endif // ifndef _CPIM_MESSAGE_H_
