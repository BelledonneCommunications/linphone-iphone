/*
 * clonable-object.cpp
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

#include "c-wrapper/internal/c-tools.h"
#include "clonable-object-p.h"
#include "clonable-object.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

L_OBJECT_IMPL(ClonableObject);

ClonableObject::ClonableObject (ClonableObjectPrivate &p) {
	setRef(p);
}

#define UNREF() \
	do { \
		auto &h = mPrivate->mPublic; \
		h.erase(this); \
		if (h.empty()) \
			delete mPrivate; \
	} while (false);

ClonableObject::~ClonableObject () {
	Wrapper::handleClonableObjectDestruction(this);
	UNREF();
}

void ClonableObject::setRef (const ClonableObjectPrivate &p) {
	// Q-pointer must exist if private data is defined.
	L_ASSERT(!mPrivate || !mPrivate->mPublic.empty());

	// Nothing, same reference.
	if (&p == mPrivate)
		return;

	// Unref previous private data.
	if (mPrivate)
		UNREF();

	// Add and reference new private data.
	mPrivate = const_cast<ClonableObjectPrivate *>(&p);
	mPrivate->mPublic.insert(this);
}

LINPHONE_END_NAMESPACE
