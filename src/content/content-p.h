/*
 * content-p.h
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

#ifndef _CONTENT_P_H_
#define _CONTENT_P_H_

#include <vector>

#include "object/clonable-object-p.h"
#include "object/object-p.h"
#include "content-type.h"
#include "content.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ContentPrivate : public ClonableObjectPrivate {
public:
	std::vector<char> body;
	ContentType contentType;
	std::string contentDisposition;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CONTENT_P_H_
