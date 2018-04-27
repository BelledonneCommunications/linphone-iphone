/*
 * cpim-core-headers.cpp
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

#include <iomanip>
#include <sstream>

#include "linphone/utils/utils.h"

#include "logger/logger.h"

#include "chat/cpim/parser/cpim-parser.h"
#include "cpim-header-p.h"

#include "cpim-core-headers.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class Cpim::ContactHeaderPrivate : public HeaderPrivate {
public:
	string uri;
	string formalName;
};

Cpim::ContactHeader::ContactHeader () : Header(*new ContactHeaderPrivate) {}

Cpim::ContactHeader::ContactHeader (const string &uri, const string &formalName) : ContactHeader() {
	setUri(uri);
	setFormalName(formalName);
}

string Cpim::ContactHeader::getUri () const {
	L_D();
	return d->uri;
}

void Cpim::ContactHeader::setUri (const string &uri) {
	L_D();
	d->uri = uri;
}

string Cpim::ContactHeader::getFormalName () const {
	L_D();
	return d->formalName;
}

void Cpim::ContactHeader::setFormalName (const string &formalName) {
	L_D();
	if (formalName.front() == '\"' && formalName.back() == '\"')
		d->formalName = formalName.substr(1, formalName.size() - 2);
	else if (formalName.back() == ' ')
		d->formalName = formalName.substr(0, formalName.size() - 1);
	else
		d->formalName = formalName;
}

string Cpim::ContactHeader::getValue () const {
	L_D();
	string result;
	if (!d->formalName.empty())
		result += "\"" + d->formalName + "\"";
	result += "<" + d->uri + ">";
	return result;
}

string Cpim::ContactHeader::asString () const {
	return getName() + ": " + getValue() + "\r\n";
}

// -----------------------------------------------------------------------------

class Cpim::DateTimeHeaderPrivate : public HeaderPrivate {
public:
	tm dateTime;
	tm dateTimeOffset;
	string signOffset;
};

Cpim::DateTimeHeader::DateTimeHeader () : Header(*new DateTimeHeaderPrivate) {}

Cpim::DateTimeHeader::DateTimeHeader (time_t time) : DateTimeHeader() {
	setTime(time);
}

Cpim::DateTimeHeader::DateTimeHeader (const tm &time, const tm &timeOffset, const string &signOffset) : DateTimeHeader() {
	setTime(time, timeOffset, signOffset);
}

time_t Cpim::DateTimeHeader::getTime () const {
	L_D();

	tm result = d->dateTime;
	result.tm_year -= 1900;
	result.tm_isdst = 0;

	if (d->signOffset == "+") {
		result.tm_hour += d->dateTimeOffset.tm_hour;
		result.tm_min += d->dateTimeOffset.tm_min;

		while (result.tm_min > 59) {
			result.tm_hour++;
			result.tm_min -= 60;
		}
	}
	else if (d->signOffset == "-") {
		result.tm_hour -= d->dateTimeOffset.tm_hour;
		result.tm_hour -= d->dateTimeOffset.tm_min;

		while (result.tm_min < 0) {
			result.tm_hour--;
			result.tm_min += 60;
		}
	}

	return Utils::getTmAsTimeT(result);
}

void Cpim::DateTimeHeader::setTime (const time_t time) {
	L_D();

	d->signOffset = "Z";
	d->dateTime = Utils::getTimeTAsTm(time);
	d->dateTime.tm_year += 1900;
}

void Cpim::DateTimeHeader::setTime (const tm &time, const tm &timeOffset, const string &signOffset) {
	L_D();

	d->dateTime = time;
	d->dateTimeOffset = timeOffset;
	d->signOffset = signOffset;
}

string Cpim::DateTimeHeader::getValue () const {
 	L_D();

	stringstream ss;
	ss << setfill('0') << setw(4) << d->dateTime.tm_year << "-"
		<< setfill('0') << setw(2) << d->dateTime.tm_mon + 1 << "-"
		<< setfill('0') << setw(2) << d->dateTime.tm_mday << "T"
		<< setfill('0') << setw(2) << d->dateTime.tm_hour << ":"
		<< setfill('0') << setw(2) << d->dateTime.tm_min << ":"
		<< setfill('0') << setw(2) << d->dateTime.tm_sec;

	ss << d->signOffset;
	if (d->signOffset != "Z")
		ss << setfill('0') << setw(2) << d->dateTimeOffset.tm_hour << ":"
			<< setfill('0') << setw(2) << d->dateTimeOffset.tm_min;

 	return ss.str();
}

string Cpim::DateTimeHeader::asString () const {
	return getName() + ": " + getValue() + "\r\n";
}

struct tm Cpim::DateTimeHeader::getTimeStruct () const {
	L_D();
	return d->dateTime;
}

struct tm Cpim::DateTimeHeader::getTimeOffset () const {
	L_D();
	return d->dateTimeOffset;
}

string Cpim::DateTimeHeader::getSignOffset () const {
	L_D();
	return d->signOffset;
}

// -----------------------------------------------------------------------------

class Cpim::NsHeaderPrivate : public HeaderPrivate {
public:
	string uri;
	string prefixName;
};

Cpim::NsHeader::NsHeader () : Header(*new NsHeaderPrivate) {}

Cpim::NsHeader::NsHeader (const string &uri, const string &prefixName) : NsHeader() {
	setUri(uri);
	setPrefixName(prefixName);
}

string Cpim::NsHeader::getUri () const {
	L_D();
	return d->uri;
}

void Cpim::NsHeader::setUri (const string &uri) {
	L_D();
	d->uri = uri;
}

string Cpim::NsHeader::getPrefixName () const {
	L_D();
	return d->prefixName;
}

void Cpim::NsHeader::setPrefixName (const string &prefixName) {
	L_D();
	d->prefixName = prefixName;
}

string Cpim::NsHeader::getValue () const {
	L_D();

	string ns;
	if (!d->prefixName.empty())
		ns = d->prefixName + " ";

	return ns + "<" + d->uri + ">";
}

string Cpim::NsHeader::asString () const {
	return getName() + ": " + getValue() + "\r\n";
}

// -----------------------------------------------------------------------------

class Cpim::RequireHeaderPrivate : public HeaderPrivate {
public:
	list<string> headerNames;
};

Cpim::RequireHeader::RequireHeader () : Header(*new RequireHeaderPrivate) {}

Cpim::RequireHeader::RequireHeader (const string &headerNames) : RequireHeader() {
	for (const string &header : Utils::split(headerNames, ",")) {
		addHeaderName(header);
	}
}

Cpim::RequireHeader::RequireHeader (const list<string> &headerNames) : RequireHeader() {
	L_D();
	d->headerNames = headerNames;
}

list<string> Cpim::RequireHeader::getHeaderNames () const {
	L_D();
	return d->headerNames;
}

void Cpim::RequireHeader::addHeaderName (const string &headerName) {
	L_D();
	d->headerNames.push_back(headerName);
}

string Cpim::RequireHeader::getValue () const {
	L_D();

	string requires;
	for (const string &header : d->headerNames) {
		if (header != d->headerNames.front())
			requires += ",";
		requires += header;
	}

	return requires;
}

string Cpim::RequireHeader::asString () const {
	return getName() + ": " + getValue() + "\r\n";
}

// -----------------------------------------------------------------------------

class Cpim::SubjectHeaderPrivate : public HeaderPrivate {
public:
	string subject;
	string language;
};

Cpim::SubjectHeader::SubjectHeader () : Header(*new SubjectHeaderPrivate) {}

Cpim::SubjectHeader::SubjectHeader (const string &subject, const string &language) : SubjectHeader() {
	setSubject(subject);
	setLanguage(language);
}

string Cpim::SubjectHeader::getSubject () const {
	L_D();
	return d->subject;
}

void Cpim::SubjectHeader::setSubject (const string &subject) {
	L_D();
	d->subject = subject;
}

string Cpim::SubjectHeader::getLanguage () const {
	L_D();
	return d->language;
}

void Cpim::SubjectHeader::setLanguage (const string &language) {
	L_D();
	d->language = language;
}

string Cpim::SubjectHeader::getValue () const {
	L_D();

	string languageParam;
	if (!d->language.empty())
		languageParam = ";lang=" + d->language;

	return languageParam + " " + d->subject;
}

string Cpim::SubjectHeader::asString () const {
	return getName() + ":" + getValue() + "\r\n";
}

LINPHONE_END_NAMESPACE
