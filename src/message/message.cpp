/*
 * message.cpp
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

#include <unordered_map>

#include "db/events-db.h"
#include "object/object-p.h"

#include "message.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

using namespace std;

class MessagePrivate : public ObjectPrivate {
private:
	weak_ptr<ChatRoom> chatRoom;
	Message::Direction direction = Message::Incoming;
	// LinphoneAddress *from;
	// LinphoneAddress *to;
	shared_ptr<ErrorInfo> errorInfo;
	string contentType;
	string text;
	bool isSecured = false;
	time_t time = 0;
	string id;
	string appData;
	list<shared_ptr<Content> > contents;
	unordered_map<string, string> customHeaders;
	Message::State state = Message::Idle;
	shared_ptr<EventsDb> eventsDb;

	L_DECLARE_PUBLIC(Message);
};

// -----------------------------------------------------------------------------

Message::Message (MessagePrivate &p) : Object(p) {}

shared_ptr<ChatRoom> Message::getChatRoom () const {
	L_D(const Message);
	shared_ptr<ChatRoom> chatRoom = d->chatRoom.lock();
	if (!chatRoom) {
		// TODO.
	}
	return chatRoom;
}

Message::Direction Message::getDirection () const {
	L_D(const Message);
	return d->direction;
}

shared_ptr<const Address> Message::getFromAddress () const {
	// TODO.
	return nullptr;
}

shared_ptr<const Address> Message::getToAddress () const {
	// TODO.
	return nullptr;
}

shared_ptr<const Address> Message::getLocalAddress () const {
	// TODO.
	return nullptr;
}

shared_ptr<const Address> Message::getRemoteAddress () const {
	// TODO.
	return nullptr;
}

Message::State Message::getState () const {
	L_D(const Message);
	return d->state;
}

shared_ptr<const ErrorInfo> Message::getErrorInfo () const {
	L_D(const Message);
	return d->errorInfo;
}

string Message::getContentType () const {
	L_D(const Message);
	return d->contentType;
}

string Message::getText () const {
	L_D(const Message);
	return d->text;
}

void Message::setText (const string &text) {
	L_D(Message);
	d->text = text;
}

void Message::send () const {
	// TODO.
}

bool Message::containsReadableText () const {
	// TODO: Check content type.
	return true;
}

bool Message::isSecured () const {
	L_D(const Message);
	return d->isSecured;
}

time_t Message::getTime () const {
	L_D(const Message);
	return d->time;
}

string Message::getId () const {
	L_D(const Message);
	return d->id;
}

string Message::getAppdata () const {
	L_D(const Message);
	return d->appData;
}

void Message::setAppdata (const string &appData) {
	L_D(Message);
	d->appData = appData;
}

list<shared_ptr<const Content> > Message::getContents () const {
	L_D(const Message);
	list<shared_ptr<const Content> > contents;
	for (const auto &content : d->contents)
		contents.push_back(content);
	return contents;
}

void Message::addContent (const shared_ptr<Content> &content) {
	L_D(Message);
	d->contents.push_back(content);
}

void Message::removeContent (const shared_ptr<const Content> &content) {
	L_D(Message);
	d->contents.remove(const_pointer_cast<Content>(content));
}

string Message::getCustomHeaderValue (const string &headerName) const {
	L_D(const Message);
	try {
		return d->customHeaders.at(headerName);
	} catch (const exception &) {
		// Key doesn't exist.
	}
	return "";
}

void Message::addCustomHeader (const string &headerName, const string &headerValue) {
	L_D(Message);
	d->customHeaders[headerName] = headerValue;
}

void Message::removeCustomHeader (const string &headerName) {
	L_D(Message);
	d->customHeaders.erase(headerName);
}

LINPHONE_END_NAMESPACE
