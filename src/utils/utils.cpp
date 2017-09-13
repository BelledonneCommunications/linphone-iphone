/*
 * utils.cpp
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
#include <cstdlib>
#include <sstream>

#include <bctoolbox/port.h>

#include "linphone/utils/utils.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

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

vector<string> Utils::split (const string &str, const string &delimiter) {
	vector<string> out;

	size_t pos = 0, oldPos = 0;
	for (; (pos = str.find(delimiter, pos)) != string::npos; oldPos = pos + 1, pos = oldPos)
		out.push_back(str.substr(oldPos, pos - oldPos));
	out.push_back(str.substr(oldPos));

	return out;
}

#ifndef __ANDROID__
#define TO_STRING_IMPL(TYPE) \
	string Utils::toString(TYPE val) { \
		return to_string(val); \
	}
#else
#define TO_STRING_IMPL(TYPE) \
	string Utils::toString(TYPE val) { \
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

int Utils::stoi (const string &str, size_t *idx, int base) {
	char *p;
	int v = strtol(str.c_str(), &p, base);

	if (idx)
		*idx = p - str.c_str();

	return v;
}

string Utils::stringToLower (const string &str) {
	string result(str.size(), ' ');
	transform(str.cbegin(), str.cend(), result.begin(), ::tolower);
	return result;
}

bool Utils::stringToBool (const string &str) {
	const string lowerStr = stringToLower(str);
	return !lowerStr.empty() && (lowerStr == "true" || lowerStr == "1");
}

char *Utils::utf8ToChar (uint32_t ic) {
	char *result = new char[5];
	int size = 0;
	if (ic < 0x80) {
		result[0] = ic;
		size = 1;
	} else if (ic < 0x800) {
		result[1] = 0x80 + ((ic & 0x3F));
		result[0] = 0xC0 + ((ic >> 6) & 0x1F);
		size = 2;
	} else if (ic < 0x100000) {
		result[2] = 0x80 + (ic & 0x3F);
		result[1] = 0x80 + ((ic >> 6) & 0x3F);
		result[0] = 0xE0 + ((ic >> 12) & 0xF);
		size = 3;
	} else if (ic < 0x110000) {
		result[3] = 0x80 + (ic & 0x3F);
		result[2] = 0x80 + ((ic >> 6) & 0x3F);
		result[1] = 0x80 + ((ic >> 12) & 0x3F);
		result[0] = 0xF0 + ((ic >> 18) & 0x7);
		size = 4;
	}
	result[size] = '\0';
	return result;
}

LINPHONE_END_NAMESPACE
