/*
 * content.h
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

#ifndef _CONTENT_H_
#define _CONTENT_H_

#include <memory>
#include <string>

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ContentPrivate;

class Content : public Object {
	friend class Core;

public:
	const std::string &getType () const;
	void setType (const std::string &type);

	const std::string &getSubType () const;
	void setSubType (const std::string &subType);

	const void *getBuffer () const;
	void setBuffer (const void *buffer, size_t size);

	size_t getSize () const;
	void setSize (size_t size);

	const std::string &getEncoding () const;
	void setEncoding (const std::string &encoding);

	const std::string &getName () const;
	void setName (const std::string &name);

	bool isMultipart () const;

	std::shared_ptr<Content> getPart (int index) const;
	std::shared_ptr<Content> findPartByHeader (const std::string &headerName, const std::string &headerValue) const;

	const std::string &getCustomHeaderValue (const std::string &headerName) const;

	const std::string &getKey () const;
	void setKey (const std::string &key);

private:
	Content ();

	L_DECLARE_PRIVATE(Content);
	L_DISABLE_COPY(Content);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONTENT_H_
