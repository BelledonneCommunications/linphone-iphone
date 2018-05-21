/*
 * content-type.h
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

#ifndef _L_CONTENT_TYPE_H_
#define _L_CONTENT_TYPE_H_

#include "object/clonable-object.h"
#include "header/header.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ContentTypePrivate;
class HeaderParam;

class LINPHONE_PUBLIC ContentType : public Header {
public:
	explicit ContentType (const std::string &contentType = "");
	ContentType (const std::string &type, const std::string &subType);
	ContentType (const std::string &type, const std::string &subType, const HeaderParam &parameter);
	ContentType (const std::string &type, const std::string &subType, const std::list<HeaderParam> &parameters);
	ContentType (const ContentType &other);

	ContentType &operator= (const ContentType &other);

	bool weakEqual (const ContentType &other) const;
	bool operator== (const ContentType &other) const;
	bool operator!= (const ContentType &other) const;

	// Delete these operators to prevent putting complicated content-type strings
	// in the code. Instead define static const ContentType objects below.
	bool operator== (const std::string &other) const = delete;
	bool operator!= (const std::string &other) const = delete;

	bool isEmpty () const;
	bool isValid () const;
	bool isFile () const;

	const std::string &getType () const;
	bool setType (const std::string &type);

	const std::string &getSubType () const;
	bool setSubType (const std::string &subType);

	bool isMultipart() const;

	static bool isFile (const ContentType &contentType);

	static const ContentType ConferenceInfo;
	static const ContentType Cpim;
	static const ContentType ExternalBody;
	static const ContentType FileTransfer;
	static const ContentType Imdn;
	static const ContentType ImIsComposing;
	static const ContentType Multipart;
	static const ContentType PlainText;
	static const ContentType ResourceLists;
	static const ContentType Rlmi;
	static const ContentType Sdp;

private:
	L_DECLARE_PRIVATE(ContentType);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONTENT_TYPE_H_
