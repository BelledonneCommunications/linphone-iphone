/*
 * cpim-generic-header.h
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

#ifndef _CPIM_GENERIC_HEADER_H_
#define _CPIM_GENERIC_HEADER_H_

#include <list>
#include <memory>

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

		std::string getName () const override;
		bool setName (const std::string &name);

		bool setValue (const std::string &value) override;

		typedef std::shared_ptr<const std::list<std::pair<std::string, std::string> > > ParameterList;

		ParameterList getParameters () const;
		bool addParameter (const std::string &key, const std::string &value);
		void removeParameter (const std::string &key, const std::string &value);

		bool isValid () const override;

		std::string asString () const override;

	protected:
		void force (const std::string &name, const std::string &value, const std::string &parameters);

	private:
		L_DECLARE_PRIVATE(GenericHeader);
		L_DISABLE_COPY(GenericHeader);
	};
}

LINPHONE_END_NAMESPACE

#endif // ifndef _CPIM_GENERIC_HEADER_H_
