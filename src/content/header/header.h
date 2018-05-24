/*
 * header.h
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

#ifndef _L_HEADER_H_
#define _L_HEADER_H_

#include <list>

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class HeaderPrivate;
class HeaderParam;

class LINPHONE_PUBLIC Header : public ClonableObject {
public:
	Header ();
	Header (const std::string &name, const std::string &value);
	Header (const std::string &name, const std::string &value, const std::list<HeaderParam> &params);
	Header (const Header &other);

	Header &operator= (const Header &other);

	bool operator== (const Header &other) const;
	bool operator!= (const Header &other) const;

	void setName (const std::string &name);
	std::string getName () const;

	void setValue (const std::string &value);
	std::string getValue () const;

	void cleanParameters ();
	const std::list<HeaderParam> &getParameters () const;
	void addParameter (const std::string &paramName, const std::string &paramValue);
	void addParameter (const HeaderParam &param);
	void addParameters(const std::list<HeaderParam> &params);
	void removeParameter (const std::string &paramName);
	void removeParameter (const HeaderParam &param);
	std::list<HeaderParam>::const_iterator findParameter (const std::string &paramName) const;
	const HeaderParam &getParameter (const std::string &paramName) const;

	std::string asString () const;

	LINPHONE_PUBLIC friend std::ostream &operator<< (std::ostream &os, const Header &header);

protected:
	explicit Header (HeaderPrivate &p);

private:
	L_DECLARE_PRIVATE(Header);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_HEADER_H_
