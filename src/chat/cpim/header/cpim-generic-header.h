/*
 * cpim-generic-header.h
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

#ifndef _L_CPIM_GENERIC_HEADER_H_
#define _L_CPIM_GENERIC_HEADER_H_

#include <list>

#include "cpim-header.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

namespace Cpim {
	class GenericHeaderPrivate;
	class HeaderNode;

	class LINPHONE_PUBLIC GenericHeader : public Header {
		friend class HeaderNode;

	public:
		GenericHeader ();

		GenericHeader (std::string name, std::string value, std::string parameters = "");

		std::string getName () const override;
		void setName (const std::string &name);

		std::string getValue () const override;
		void setValue (const std::string &value);

		typedef std::shared_ptr<const std::list<std::pair<std::string, std::string>>> ParameterList;

		ParameterList getParameters () const;
		void addParameter (const std::string &key, const std::string &value);
		void removeParameter (const std::string &key, const std::string &value);

		std::string asString () const override;

	private:
		L_DECLARE_PRIVATE(GenericHeader);
		L_DISABLE_COPY(GenericHeader);
	};
}

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CPIM_GENERIC_HEADER_H_
