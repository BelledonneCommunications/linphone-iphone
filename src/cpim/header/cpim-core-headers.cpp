/*
 * cpim-core-headers.cpp
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

#include "cpim-header-p.h"
#include "cpim/parser/cpim-parser.h"

#include "cpim-core-headers.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

Cpim::CoreHeader::CoreHeader () : Header(*new HeaderPrivate) {}

Cpim::CoreHeader::CoreHeader (HeaderPrivate &p) : Header(p) {}

Cpim::CoreHeader::~CoreHeader () {}

bool Cpim::CoreHeader::isValid () const {
	return !getValue().empty();
}

// -----------------------------------------------------------------------------

#define MAKE_CORE_HEADER_IMPL(CLASS_PREFIX) \
	bool Cpim::CLASS_PREFIX ## Header::setValue(const string &value) { \
		return Parser::getInstance()->coreHeaderIsValid<CLASS_PREFIX ## Header>(value) && Header::setValue(value); \
	}

MAKE_CORE_HEADER_IMPL(From);
MAKE_CORE_HEADER_IMPL(To);
MAKE_CORE_HEADER_IMPL(Cc);
MAKE_CORE_HEADER_IMPL(DateTime);

MAKE_CORE_HEADER_IMPL(Ns);
MAKE_CORE_HEADER_IMPL(Require);

#undef MAKE_CORE_HEADER_IMPL

// -----------------------------------------------------------------------------

void Cpim::CoreHeader::force (const string &value) {
	Header::setValue(value);
}

// -----------------------------------------------------------------------------

class Cpim::SubjectHeaderPrivate : public HeaderPrivate {
public:
	string language;
};

Cpim::SubjectHeader::SubjectHeader () : CoreHeader(*new SubjectHeaderPrivate) {}

bool Cpim::SubjectHeader::setValue (const string &value) {
	return Parser::getInstance()->coreHeaderIsValid<SubjectHeader>(value) && Header::setValue(value);
}

string Cpim::SubjectHeader::getLanguage () const {
	L_D(const SubjectHeader);
	return d->language;
}

bool Cpim::SubjectHeader::setLanguage (const string &language) {
	if (!language.empty() && !Parser::getInstance()->subjectHeaderLanguageIsValid(language))
		return false;

	L_D(SubjectHeader);
	d->language = language;

	return true;
}

string Cpim::SubjectHeader::asString () const {
	L_D(const SubjectHeader);

	string languageParam;
	if (!d->language.empty())
		languageParam = ";lang=" + d->language;

	return getName() + ":" + languageParam + " " + getValue() + "\r\n";
}

void Cpim::SubjectHeader::force (const string &value, const string &language) {
	L_D(SubjectHeader);
	CoreHeader::force(value);
	d->language = language;
}

LINPHONE_END_NAMESPACE
