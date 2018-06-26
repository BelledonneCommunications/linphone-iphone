/*
 * content.h
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

#ifndef _L_CONTENT_H_
#define _L_CONTENT_H_

#include <list>
#include <vector>

#include "object/app-data-container.h"
#include "object/clonable-object.h"

// =============================================================================

L_DECL_C_STRUCT(LinphoneContent);

LINPHONE_BEGIN_NAMESPACE

class ContentDisposition;
class ContentType;
class ContentPrivate;
class Header;

class LINPHONE_PUBLIC Content : public ClonableObject, public AppDataContainer {
public:
	Content ();
	Content (const Content &other);
	Content (Content &&other);
	~Content ();

	Content &operator= (const Content &other);
	Content &operator= (Content &&other);

	bool operator== (const Content &other) const;

	const ContentType &getContentType () const;
	void setContentType (const ContentType &contentType);

	const ContentDisposition &getContentDisposition () const;
	void setContentDisposition (const ContentDisposition &contentDisposition);

	const std::string &getContentEncoding () const;
	void setContentEncoding (const std::string &contentEncoding);

	const std::vector<char> &getBody () const;
	std::string getBodyAsString () const;
	std::string getBodyAsUtf8String () const;

	void setBody (const std::vector<char> &body);
	void setBody (std::vector<char> &&body);
	void setBody (const std::string &body);
	void setBody (const void *buffer, size_t size);
	void setBodyFromUtf8 (const std::string &body);

	size_t getSize () const;

	bool isValid () const;

	bool isEmpty () const;

	virtual bool isFile () const;
	virtual bool isFileTransfer () const;

	const std::list<Header> &getHeaders () const;
	const Header &getHeader (const std::string &headerName) const;
	void addHeader (const std::string &headerName, const std::string &headerValue);
	void addHeader (const Header &header);
	void removeHeader (const std::string &headerName);
	std::list<Header>::const_iterator findHeader (const std::string &headerName) const;

protected:
	explicit Content (ContentPrivate &p);

private:
	L_DECLARE_PRIVATE(Content);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONTENT_H_
