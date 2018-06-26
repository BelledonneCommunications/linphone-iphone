/*
 * utils.cpp
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
#include <cstdlib>
#include <list>
#include <sstream>

#include <bctoolbox/port.h>
#include <bctoolbox/charconv.h>

#include "linphone/utils/utils.h"

#include "logger/logger.h"

#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

bool Utils::iequals (const string &a, const string &b) {
	size_t size = a.size();
	if (b.size() != size)
		return false;

	for (size_t i = 0; i < size; ++i) {
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------------

vector<string> Utils::split (const string &str, const string &delimiter) {
	vector<string> out;

	size_t pos = 0, oldPos = 0;
	for (; (pos = str.find(delimiter, pos)) != string::npos; oldPos = pos + 1, pos = oldPos)
		out.push_back(str.substr(oldPos, pos - oldPos));
	out.push_back(str.substr(oldPos));

	return out;
}

// -----------------------------------------------------------------------------

#ifndef __ANDROID__
#define TO_STRING_IMPL(TYPE) \
	string Utils::toString (TYPE val) { \
		return to_string(val); \
	}
#else
#define TO_STRING_IMPL(TYPE) \
	string Utils::toString (TYPE val) { \
		ostringstream os; \
		os << val; \
		return os.str(); \
	}
#endif // ifndef __ANDROID__

TO_STRING_IMPL(int)
TO_STRING_IMPL(long)
TO_STRING_IMPL(long long)
TO_STRING_IMPL(unsigned)
TO_STRING_IMPL(unsigned long)
TO_STRING_IMPL(unsigned long long)
TO_STRING_IMPL(float)
TO_STRING_IMPL(double)
TO_STRING_IMPL(long double)

#undef TO_STRING_IMPL

string Utils::toString (const void *val) {
	ostringstream ss;
	ss << val;
	return ss.str();
}

// -----------------------------------------------------------------------------

#define STRING_TO_NUMBER_IMPL(TYPE, SUFFIX) \
	TYPE Utils::sto ## SUFFIX (const string &str, size_t *idx, int base) { \
		return sto ## SUFFIX(str.c_str(), idx, base); \
	} \
	TYPE Utils::sto ## SUFFIX (const char *str, size_t *idx, int base) { \
		char *p; \
		TYPE v = strto ## SUFFIX(str, &p, base); \
		if (idx) \
			*idx = static_cast<size_t>(p - str); \
		return v; \
	} \

#define STRING_TO_NUMBER_IMPL_BASE_LESS(TYPE, SUFFIX) \
	TYPE Utils::sto ## SUFFIX(const string &str, size_t * idx) { \
		return sto ## SUFFIX(str.c_str(), idx); \
	} \
	TYPE Utils::sto ## SUFFIX(const char *str, size_t * idx) { \
		char *p; \
		TYPE v = strto ## SUFFIX(str, &p); \
		if (idx) \
			*idx = static_cast<size_t>(p - str); \
		return v; \
	} \

#define strtoi(STR, IDX, BASE) static_cast<int>(strtol(STR, IDX, BASE))
STRING_TO_NUMBER_IMPL(int, i)
#undef strtoi

STRING_TO_NUMBER_IMPL(long long, ll)
STRING_TO_NUMBER_IMPL(unsigned long long, ull)

STRING_TO_NUMBER_IMPL_BASE_LESS(double, d)
STRING_TO_NUMBER_IMPL_BASE_LESS(float, f)

#undef STRING_TO_NUMBER_IMPL
#undef STRING_TO_NUMBER_IMPL_BASE_LESS

bool Utils::stob (const string &str) {
	const string lowerStr = stringToLower(str);
	return !lowerStr.empty() && (lowerStr == "true" || lowerStr == "1");
}

// -----------------------------------------------------------------------------

string Utils::stringToLower (const string &str) {
	string result(str.size(), ' ');
	transform(str.cbegin(), str.cend(), result.begin(), ::tolower);
	return result;
}

// -----------------------------------------------------------------------------

char *Utils::utf8ToChar (uint32_t ic) {
	char *result = new char[5];
	int size = 0;
	if (ic < 0x80) {
		result[0] = static_cast<char>(ic);
		size = 1;
	} else if (ic < 0x800) {
		result[1] = static_cast<char>(0x80 + ((ic & 0x3F)));
		result[0] = static_cast<char>(0xC0 + ((ic >> 6) & 0x1F));
		size = 2;
	} else if (ic < 0x100000) {
		result[2] = static_cast<char>(0x80 + (ic & 0x3F));
		result[1] = static_cast<char>(0x80 + ((ic >> 6) & 0x3F));
		result[0] = static_cast<char>(0xE0 + ((ic >> 12) & 0xF));
		size = 3;
	} else if (ic < 0x110000) {
		result[3] = static_cast<char>(0x80 + (ic & 0x3F));
		result[2] = static_cast<char>(0x80 + ((ic >> 6) & 0x3F));
		result[1] = static_cast<char>(0x80 + ((ic >> 12) & 0x3F));
		result[0] = static_cast<char>(0xF0 + ((ic >> 18) & 0x7));
		size = 4;
	}
	result[size] = '\0';
	return result;
}

string Utils::trim (const string &str) {
	auto itFront = find_if_not(str.begin(), str.end(), [] (int c) { return isspace(c); });
	auto itBack = find_if_not(str.rbegin(), str.rend(), [] (int c) { return isspace(c); }).base();
	return (itBack <= itFront ? string() : string(itFront, itBack));
}

// -----------------------------------------------------------------------------

tm Utils::getTimeTAsTm (time_t time) {
	#ifdef _WIN32
		return *gmtime(&time);
	#else
		tm result;
		return *gmtime_r(&time, &result);
	#endif
}

time_t Utils::getTmAsTimeT (const tm &time) {
	time_t result;

	#if defined(LINPHONE_WINDOWS_UNIVERSAL) || defined(LINPHONE_MSC_VER_GREATER_19)
		long adjust_timezone;
	#else
		time_t adjust_timezone;
	#endif

	#if TARGET_IPHONE_SIMULATOR
		result = timegm(&const_cast<tm &>(time));
		adjust_timezone = 0;
	#else
		result = mktime(&const_cast<tm &>(time));

		#if defined(LINPHONE_WINDOWS_UNIVERSAL) || defined(LINPHONE_MSC_VER_GREATER_19)
			_get_timezone(&adjust_timezone);
		#else
			adjust_timezone = timezone;
		#endif
	#endif

	if (result == (time_t)-1) {
		lError() << "mktime failed: " << strerror(errno);
		return (time_t)-1;
	}

	return result - (time_t)adjust_timezone;
}

// -----------------------------------------------------------------------------

// TODO: Improve perf!!! Avoid c <--> cpp string conversions.
string Utils::localeToUtf8 (const string &str) {
	char *cStr = bctbx_locale_to_utf8(str.c_str());
	string utf8Str = cStringToCppString(cStr);
	bctbx_free(cStr);
	return utf8Str;
}

string Utils::utf8ToLocale (const string &str) {
	char *cStr = bctbx_utf8_to_locale(str.c_str());
	string localeStr = cStringToCppString(cStr);
	bctbx_free(cStr);
	return localeStr;
}

string Utils::convertAnyToUtf8 (const string &str, const string &encoding) {
	char *cStr = bctbx_convert_any_to_utf8(str.c_str(), encoding.c_str());
	string convertedStr = cStringToCppString(cStr);
	bctbx_free(cStr);
	return convertedStr;
}

LINPHONE_END_NAMESPACE
