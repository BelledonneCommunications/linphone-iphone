/*
 * content-disposition.h
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

#ifndef _L_CONTENT_DISPOSITION_H_
#define _L_CONTENT_DISPOSITION_H_

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ContentDispositionPrivate;

class LINPHONE_PUBLIC ContentDisposition : public ClonableObject {
public:
	explicit ContentDisposition (const std::string &contentDisposition = "");
	ContentDisposition (const ContentDisposition &other);

	ContentDisposition &operator= (const ContentDisposition &other);

	bool weakEqual (const ContentDisposition &other) const;
	bool operator== (const ContentDisposition &other) const;
	bool operator!= (const ContentDisposition &other) const;

	// Delete these operators to prevent putting complicated content-disposition strings
	// in the code. Instead define static const ContentDisposition objects below.
	bool operator== (const std::string &other) const = delete;
	bool operator!= (const std::string &other) const = delete;

	bool isEmpty () const;
	bool isValid () const;

	const std::string &getParameter () const;
	void setParameter (const std::string &parameter);

	std::string asString () const;

	static const ContentDisposition Notification;
	static const ContentDisposition RecipientList;
	static const ContentDisposition RecipientListHistory;

private:
	L_DECLARE_PRIVATE(ContentDisposition);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONTENT_DISPOSITION_H_
