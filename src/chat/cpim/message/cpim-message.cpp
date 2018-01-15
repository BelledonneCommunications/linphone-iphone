/*
 * cpim-message.cpp
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

#include <algorithm>

#include "linphone/utils/utils.h"

#include "logger/logger.h"
#include "chat/cpim/parser/cpim-parser.h"
#include "content/content-type.h"
#include "object/object-p.h"

#include "cpim-message.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class Cpim::MessagePrivate : public ObjectPrivate {
public:
	typedef list<shared_ptr<const Header> > PrivHeaderList;

	shared_ptr<PrivHeaderList> cpimHeaders = make_shared<PrivHeaderList>();
	shared_ptr<PrivHeaderList> messageHeaders = make_shared<PrivHeaderList>();
	shared_ptr<PrivHeaderList> contentHeaders = make_shared<PrivHeaderList>();
	string content;
};

Cpim::Message::Message () : Object(*new MessagePrivate) {}

// -----------------------------------------------------------------------------

Cpim::Message::HeaderList Cpim::Message::getCpimHeaders () const {
	L_D();
	return d->cpimHeaders;
}

bool Cpim::Message::addCpimHeader (const Header &cpimHeader) {
	L_D();

	if (!cpimHeader.isValid())
		return false;

	d->cpimHeaders->push_back(Parser::getInstance()->cloneHeader(cpimHeader));
	return true;
}

void Cpim::Message::removeCpimHeader (const Header &cpimHeader) {
	L_D();
	d->cpimHeaders->remove_if([&cpimHeader](const shared_ptr<const Header> &header) {
			return cpimHeader.getName() == header->getName() && cpimHeader.getValue() == header->getValue();
		});
}

// -----------------------------------------------------------------------------

Cpim::Message::HeaderList Cpim::Message::getMessageHeaders () const {
	L_D();
	return d->messageHeaders;
}

bool Cpim::Message::addMessageHeader (const Header &messageHeader) {
	L_D();

	if (!messageHeader.isValid())
		return false;

	d->messageHeaders->push_back(Parser::getInstance()->cloneHeader(messageHeader));
	return true;
}

void Cpim::Message::removeMessageHeader (const Header &messageHeader) {
	L_D();
	d->messageHeaders->remove_if([&messageHeader](const shared_ptr<const Header> &header) {
			return messageHeader.getName() == header->getName() && messageHeader.getValue() == header->getValue();
		});
}

// -----------------------------------------------------------------------------

Cpim::Message::HeaderList Cpim::Message::getContentHeaders () const {
	L_D();
	return d->contentHeaders;
}

bool Cpim::Message::addContentHeader (const Header &contentHeader) {
	L_D();

	if (!contentHeader.isValid())
		return false;

	d->contentHeaders->push_back(Parser::getInstance()->cloneHeader(contentHeader));
	return true;
}

void Cpim::Message::removeContentHeader (const Header &contentHeader) {
	L_D();
	d->contentHeaders->remove_if([&contentHeader](const shared_ptr<const Header> &header) {
			return contentHeader.getName() == header->getName() && contentHeader.getValue() == header->getValue();
		});
}

// -----------------------------------------------------------------------------

string Cpim::Message::getContent () const {
	L_D();
	return d->content;
}

bool Cpim::Message::setContent (const string &content) {
	L_D();
	d->content = content;
	return true;
}

// -----------------------------------------------------------------------------

bool Cpim::Message::isValid () const {
	L_D();

	return find_if(d->cpimHeaders->cbegin(), d->cpimHeaders->cend(),
		[](const shared_ptr<const Header> &header) {
			return Utils::iequals(header->getName(), "content-type") && (ContentType(header->getValue()) == ContentType::Cpim);
		}) != d->cpimHeaders->cend();
}

// -----------------------------------------------------------------------------

string Cpim::Message::asString () const {
	L_D();

	string output;
	for (const auto &cpimHeader : *d->cpimHeaders)
		output += cpimHeader->asString();

	output += "\r\n";

	if (d->messageHeaders->size() > 0) {
		for (const auto &messageHeader : *d->messageHeaders)
			output += messageHeader->asString();

		output += "\r\n";
	}

	for (const auto &contentHeaders : *d->contentHeaders)
		output += contentHeaders->asString();
		
	output += "\r\n";

	output += getContent();

	return output;
}

// -----------------------------------------------------------------------------

shared_ptr<const Cpim::Message> Cpim::Message::createFromString (const string &str) {
	return Parser::getInstance()->parseMessage(str);
}

LINPHONE_END_NAMESPACE
