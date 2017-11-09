/*
 * content.h
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#ifndef _CONTENT_H_
#define _CONTENT_H_

#include <vector>

// TODO: Remove me.
#include "linphone/content.h"

#include "object/app-data-container.h"
#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ContentType;
class ContentPrivate;

class LINPHONE_PUBLIC Content : public ClonableObject, public AppDataContainer {
public:
	Content ();
	Content (const Content &src);
	Content (Content &&src);

	Content &operator= (const Content &src);
	Content &operator= (Content &&src);
	bool operator== (const Content &content) const;

	const ContentType &getContentType () const;
	void setContentType (const ContentType &contentType);
	void setContentType (const std::string &contentType);

	const std::string &getContentDisposition () const;
	void setContentDisposition (const std::string &contentDisposition);

	const std::vector<char> &getBody () const;
	std::string getBodyAsString () const;

	void setBody (const std::vector<char> &body);
	void setBody (std::vector<char> &&body);
	void setBody (const std::string &body);
	void setBody (const void *buffer, size_t size);

	size_t getSize () const;

	bool isValid() const;

	bool isEmpty () const;

	virtual LinphoneContent * toLinphoneContent() const;

	static const Content Empty;

protected:
	explicit Content (ContentPrivate &p);

private:
	L_DECLARE_PRIVATE(Content);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONTENT_H_
