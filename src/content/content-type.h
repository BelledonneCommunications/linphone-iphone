/*
 * content-type.h
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

#ifndef _CONTENT_TYPE_H_
#define _CONTENT_TYPE_H_

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ContentTypePrivate;

class LINPHONE_PUBLIC ContentType : public ClonableObject {
public:
	ContentType (const std::string &contentType = "");
	ContentType (const std::string &type, const std::string &subType);
	ContentType (const ContentType &src);

	ContentType &operator= (const ContentType &src);

	bool operator== (const ContentType &contentType);
	bool operator== (const std::string &contentType);

	bool isValid () const;

	const std::string &getType () const;
	bool setType (const std::string &type);

	const std::string &getSubType () const;
	bool setSubType (const std::string &subType);

	std::string asString () const;

	bool isFileTransfer () const;
	bool isImIsComposing () const;
	bool isImdn () const;
	bool isText () const;

	static bool isFileTransfer (const std::string &contentType);
	static bool isImIsComposing (const std::string &contentType);
	static bool isImdn (const std::string &contentType);
	static bool isText (const std::string &contentType);

private:
	L_DECLARE_PRIVATE(ContentType);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONTENT_TYPE_H_
