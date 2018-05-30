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
#include <map>

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
	using PrivHeaderList = list<shared_ptr<const Header>>;
	using PrivHeaderMap = map<string, shared_ptr<PrivHeaderList>>;

	PrivHeaderMap messageHeaders;
	shared_ptr<PrivHeaderList> contentHeaders = make_shared<PrivHeaderList>();
	string content;
};

Cpim::Message::Message () : Object(*new MessagePrivate) {}

// -----------------------------------------------------------------------------

Cpim::Message::HeaderList Cpim::Message::getMessageHeaders (const string &ns) const {
	L_D();

	if (d->messageHeaders.find(ns) == d->messageHeaders.end())
		return nullptr;

	return d->messageHeaders.at(ns);
}

bool Cpim::Message::addMessageHeader (const Header &messageHeader, const string &ns) {
	L_D();

	auto header = Parser::getInstance()->cloneHeader(messageHeader);
	if (header == nullptr)
		return false;

	if (d->messageHeaders.find(ns) == d->messageHeaders.end())
		d->messageHeaders[ns] = make_shared<Cpim::MessagePrivate::PrivHeaderList>();

	auto list = d->messageHeaders.at(ns);
	list->push_back(header);

	return true;
}

void Cpim::Message::removeMessageHeader (const Header &messageHeader, const string &ns) {
	L_D();

	if (d->messageHeaders.find(ns) != d->messageHeaders.end())
		d->messageHeaders.at(ns)->remove_if([&messageHeader](const shared_ptr<const Header> &header) {
				return messageHeader.getName() == header->getName() && messageHeader.getValue() == header->getValue();
			});
}

shared_ptr<const Cpim::Header> Cpim::Message::getMessageHeader (const string &name, const string &ns) const {
	L_D();

	if (d->messageHeaders.find(ns) == d->messageHeaders.end())
		return nullptr;

	auto list = d->messageHeaders.at(ns);
	for (const auto &messageHeader : *list) {
		if (messageHeader->getName() == name)
			return messageHeader;
	}

	return nullptr;
}

// -----------------------------------------------------------------------------

Cpim::Message::HeaderList Cpim::Message::getContentHeaders () const {
	L_D();
	return d->contentHeaders;
}

bool Cpim::Message::addContentHeader (const Header &contentHeader) {
	L_D();

	auto header = Parser::getInstance()->cloneHeader(contentHeader);
	if (header == nullptr)
		return false;

	d->contentHeaders->push_back(header);

	return true;
}

void Cpim::Message::removeContentHeader (const Header &contentHeader) {
	L_D();
	d->contentHeaders->remove_if([&contentHeader](const shared_ptr<const Header> &header) {
			return contentHeader.getName() == header->getName() && contentHeader.getValue() == header->getValue();
		});
}

shared_ptr<const Cpim::Header> Cpim::Message::getContentHeader(const string &name) const {
	L_D();

	for (const auto &contentHeader : *d->contentHeaders) {
		if (contentHeader->getName() == name)
			return contentHeader;
	}

	return nullptr;
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

string Cpim::Message::asString () const {
	L_D();

	string output;
	if (d->messageHeaders.size() > 0) {
		for (const auto &entry : d->messageHeaders) {
			auto list = entry.second;
			for (const auto &messageHeader : *list) {
				if (entry.first != "")
					output += entry.first + ".";
				output += messageHeader->asString();
			}
		}

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
