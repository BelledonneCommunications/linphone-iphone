/*
 * cpim-message.cpp
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

#include <algorithm>

#include "cpim/parser/cpim-parser.h"
#include "object/object-p.h"
#include "utils/utils.h"

#include "cpim-message.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class Cpim::MessagePrivate : public ObjectPrivate {
public:
	typedef list<shared_ptr<const Header> > PrivHeaderList;

	shared_ptr<PrivHeaderList> cpimHeaders = make_shared<PrivHeaderList>();
	shared_ptr<PrivHeaderList> messageHeaders = make_shared<PrivHeaderList>();
	string content;
};

Cpim::Message::Message () : Object(*new MessagePrivate) {}

// -----------------------------------------------------------------------------

Cpim::Message::HeaderList Cpim::Message::getCpimHeaders () const {
	L_D(const Message);
	return d->cpimHeaders;
}

bool Cpim::Message::addCpimHeader (const Header &cpimHeader) {
	L_D(Message);

	if (!cpimHeader.isValid())
		return false;

	d->cpimHeaders->push_back(Parser::getInstance()->cloneHeader(cpimHeader));
	return true;
}

void Cpim::Message::removeCpimHeader (const Header &cpimHeader) {
	L_D(Message);
	d->cpimHeaders->remove_if([&cpimHeader](const shared_ptr<const Header> &header) {
			return cpimHeader.getName() == header->getName() && cpimHeader.getValue() == header->getValue();
		});
}

// -----------------------------------------------------------------------------

Cpim::Message::HeaderList Cpim::Message::getMessageHeaders () const {
	L_D(const Message);
	return d->messageHeaders;
}

bool Cpim::Message::addMessageHeader (const Header &messageHeader) {
	L_D(Message);

	if (!messageHeader.isValid())
		return false;

	d->messageHeaders->push_back(Parser::getInstance()->cloneHeader(messageHeader));
	return true;
}

void Cpim::Message::removeMessageHeader (const Header &messageHeader) {
	L_D(Message);
	d->messageHeaders->remove_if([&messageHeader](const shared_ptr<const Header> &header) {
			return messageHeader.getName() == header->getName() && messageHeader.getValue() == header->getValue();
		});
}

// -----------------------------------------------------------------------------

string Cpim::Message::getContent () const {
	L_D(const Message);
	return d->content;
}

bool Cpim::Message::setContent (const string &content) {
	L_D(Message);
	d->content = content;
	return true;
}

// -----------------------------------------------------------------------------

bool Cpim::Message::isValid () const {
	L_D(const Message);

	return find_if(d->cpimHeaders->cbegin(), d->cpimHeaders->cend(),
		[](const shared_ptr<const Header> &header) {
			return Utils::iequals(header->getName(), "content-type") && header->getValue() == "Message/CPIM";
		}) != d->cpimHeaders->cend();
}

// -----------------------------------------------------------------------------

string Cpim::Message::asString () const {
	L_D(const Message);

	string output;
	for (const auto &cpimHeader : *d->cpimHeaders)
		output += cpimHeader->asString();

	output += "\r\n";

	for (const auto &messageHeader : *d->messageHeaders)
		output += messageHeader->asString();

	output += "\r\n";
	output += ""; // TODO: Headers MIME.
	output += getContent();

	return output;
}

// -----------------------------------------------------------------------------

shared_ptr<const Cpim::Message> Cpim::Message::createFromString (const string &str) {
	return Parser::getInstance()->parseMessage(str);
}

LINPHONE_END_NAMESPACE
