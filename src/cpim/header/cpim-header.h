/*
 * cpim-header.h
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

#ifndef _CPIM_HEADER_H_
#define _CPIM_HEADER_H_

#include <string>

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

namespace Cpim {
	class HeaderPrivate;

	class LINPHONE_PUBLIC Header : public Object {
	public:
		virtual ~Header () = default;

		virtual std::string getName () const = 0;

		std::string getValue () const;
		virtual bool setValue (const std::string &value);

		virtual bool isValid () const = 0;

		virtual std::string asString () const;

	protected:
		explicit Header (HeaderPrivate &p);

	private:
		L_DECLARE_PRIVATE(Header);
		L_DISABLE_COPY(Header);
	};
}

LINPHONE_END_NAMESPACE

#endif // ifndef _CPIM_HEADER_H_
