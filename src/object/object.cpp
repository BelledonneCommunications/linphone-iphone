/*
 * object.cpp
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

#include "logger/logger.h"
#include "object-p.h"

#include "object.h"

// =============================================================================

#define GET_SHARED_FROM_THIS_FATAL_ERROR "Object was not created with `ObjectFactory::create`."

using namespace std;

LINPHONE_BEGIN_NAMESPACE

Object::~Object () {
	delete mPrivate;
}

Object::Object (ObjectPrivate &p) : mPrivate(&p) {}

shared_ptr<Object> Object::getSharedFromThis () {
	return const_pointer_cast<Object>(static_cast<const Object *>(this)->getSharedFromThis());
}

shared_ptr<const Object> Object::getSharedFromThis () const {
	shared_ptr<const Object> object;

	try {
		object = mPrivate->weak.lock();
		if (!object)
			lFatal() << GET_SHARED_FROM_THIS_FATAL_ERROR;
	} catch (const exception &) {
		lFatal() << GET_SHARED_FROM_THIS_FATAL_ERROR;
	}

	return object;
}

void ObjectFactory::setPublic (const shared_ptr<Object> &object) {
	L_ASSERT(object);
	ObjectPrivate *d = object->getPrivate();
	d->mPublic = object.get();
	d->weak = object;
}

LINPHONE_END_NAMESPACE
