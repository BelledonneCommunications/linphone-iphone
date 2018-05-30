/*
 * cpim-message.h
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

#ifndef _L_CPIM_MESSAGE_H_
#define _L_CPIM_MESSAGE_H_

#include "chat/cpim/header/cpim-core-headers.h"
#include "chat/cpim/header/cpim-generic-header.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

namespace Cpim {
	class MessagePrivate;

	class LINPHONE_PUBLIC Message : public Object {
	public:
		Message ();

		typedef std::shared_ptr<std::list<std::shared_ptr<const Cpim::Header>>> HeaderList;

		HeaderList getMessageHeaders (const std::string &ns = "") const;
		bool addMessageHeader (const Header &messageHeader, const std::string &ns = "");
		void removeMessageHeader (const Header &messageHeader, const std::string &ns = "");
		std::shared_ptr<const Cpim::Header> getMessageHeader (const std::string &name, const std::string &ns = "") const;

		HeaderList getContentHeaders () const;
		bool addContentHeader (const Header &contentHeader);
		void removeContentHeader (const Header &contentHeader);
		std::shared_ptr<const Cpim::Header> getContentHeader (const std::string &name) const;

		std::string getContent () const;
		bool setContent (const std::string &content);

		std::string asString () const;

		static std::shared_ptr<const Message> createFromString (const std::string &str);

	private:
		L_DECLARE_PRIVATE(Message);
		L_DISABLE_COPY(Message);
	};
}

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CPIM_MESSAGE_H_
