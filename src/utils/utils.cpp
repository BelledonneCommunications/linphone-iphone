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

#include <cstdlib>

#include "utils.h"

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

int Utils::stoi (const string &str, size_t *idx, int base) {
	char *p;
	int v = strtol(str.c_str(), &p, base);

	if (idx)
		*idx = p - str.c_str();

	return v;
}

LINPHONE_END_NAMESPACE
