/*
 * clonable-object.cpp
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

#include "clonable-object-p.h"

#include "clonable-object.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// TODO: Use atomic counter?

void ClonableObjectPrivate::ref () {
	++nRefs;
}

void ClonableObjectPrivate::unref () {
	if (--nRefs == 0) {
		delete mPublic;
		delete this;
	}
}

// -----------------------------------------------------------------------------

ClonableObject::ClonableObject (ClonableObjectPrivate &p) : mPrivate(&p) {
	// Q-pointer must be empty. It's a constructor that takes a new private data.
	L_ASSERT(!mPrivate->mPublic);

	mPrivate->mPublic = new remove_pointer<decltype(mPrivate->mPublic)>::type;
	(*mPrivate->mPublic)[mPrivate] = this;
	mPrivate->ref();
}

ClonableObject::ClonableObject (const ClonableObjectPrivate &p) {
	// Cannot access to Q-pointer. It's a copy constructor from private data.
	L_ASSERT(!mPrivate);

	setRef(p);
}

ClonableObject::~ClonableObject () {
	mPrivate->mPublic->erase(mPrivate);
	mPrivate->unref();
}

void ClonableObject::setRef (const ClonableObjectPrivate &p) {
	// Q-pointer must exist if private data is defined.
	L_ASSERT(!mPrivate || mPrivate->mPublic);

	// Nothing, same reference.
	if (&p == mPrivate)
		return;

	// Unref previous private data.
	if (mPrivate) {
		mPrivate->mPublic->erase(mPrivate);
		mPrivate->unref();
	}

	// Add and reference new private data.
	mPrivate = const_cast<ClonableObjectPrivate *>(&p);
	(*mPrivate->mPublic)[mPrivate] = this;
	mPrivate->ref();
}

LINPHONE_END_NAMESPACE
