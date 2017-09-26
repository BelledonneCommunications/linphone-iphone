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

#include <vector>

#include "content-type.h"
#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ContentPrivate;

class LINPHONE_PUBLIC Content : public ClonableObject {
public:
	Content ();
	Content (const Content &src);
	Content (Content &&src);

	Content &operator= (const Content &src);
	Content &operator= (Content &&src);

	const ContentType &getContentType () const;
	void setContentType (const ContentType &contentType);
	void setContentType (const std::string &contentType);

	const std::string &getContentDisposition () const;
	void setContentDisposition (const std::string &contentDisposition);

	const std::vector<char> &getBody () const;
	std::string getBodyAsString () const;
	void setBody (const std::vector<char> &body);
	void setBody (const std::string &body);
	void setBody (const void *buffer, size_t size);
	size_t getSize () const;

private:
	L_DECLARE_PRIVATE(Content);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONTENT_H_
