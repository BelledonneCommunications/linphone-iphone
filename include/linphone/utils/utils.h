/*
 * utils.h
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

namespace Utils {
	template<typename T>
	LINPHONE_PUBLIC constexpr T *getPtr (std::shared_ptr<T> &object) {
		return object.get();
	}

	template<typename T>
	LINPHONE_PUBLIC constexpr T *getPtr (const std::shared_ptr<T> &object) {
		return object.get();
	}

	template<typename T>
	LINPHONE_PUBLIC constexpr T *getPtr (std::unique_ptr<T> &object) {
		return object.get();
	}

	template<typename T>
	LINPHONE_PUBLIC constexpr T *getPtr (const std::unique_ptr<T> &object) {
		return object.get();
	}

	template<typename T>
	LINPHONE_PUBLIC constexpr T *getPtr (T *object) {
		return object;
	}

	template<typename T>
	LINPHONE_PUBLIC constexpr T *getPtr (T &object) {
		return &object;
	}

	LINPHONE_PUBLIC bool iequals (const std::string &a, const std::string &b);

	LINPHONE_PUBLIC std::vector<std::string> split (const std::string &str, const std::string &delimiter);

	LINPHONE_PUBLIC inline std::vector<std::string> split (const std::string &str, char delimiter) {
		return split(str, std::string(1, delimiter));
	}

	LINPHONE_PUBLIC std::string toString (int val);
	LINPHONE_PUBLIC std::string toString (long val);
	LINPHONE_PUBLIC std::string toString (long long val);
	LINPHONE_PUBLIC std::string toString (unsigned val);
	LINPHONE_PUBLIC std::string toString (unsigned long val);
	LINPHONE_PUBLIC std::string toString (unsigned long long val);
	LINPHONE_PUBLIC std::string toString (float val);
	LINPHONE_PUBLIC std::string toString (double val);
	LINPHONE_PUBLIC std::string toString (long double val);
	LINPHONE_PUBLIC std::string toString (const void *val);

	LINPHONE_PUBLIC int stoi (const std::string &str, size_t *idx = 0, int base = 10);
	LINPHONE_PUBLIC long long stoll (const std::string &str, size_t *idx = 0, int base = 10);
	LINPHONE_PUBLIC unsigned long long stoull (const std::string &str, size_t *idx = 0, int base = 10);
	LINPHONE_PUBLIC double stod (const std::string &str, size_t *idx = 0);
	LINPHONE_PUBLIC float stof (const std::string &str, size_t *idx = 0);
	LINPHONE_PUBLIC bool stob (const std::string &str);

	LINPHONE_PUBLIC int stoi (const char *str, size_t *idx = 0, int base = 10);
	LINPHONE_PUBLIC long long stoll (const char *str, size_t *idx = 0, int base = 10);
	LINPHONE_PUBLIC unsigned long long stoull (const char *str, size_t *idx = 0, int base = 10);
	LINPHONE_PUBLIC double stod (const char *str, size_t *idx = 0);
	LINPHONE_PUBLIC float stof (const char *str, size_t *idx = 0);

	LINPHONE_PUBLIC std::string stringToLower (const std::string &str);

	// Return a buffer allocated with new.
	LINPHONE_PUBLIC char *utf8ToChar (uint32_t ic);

	LINPHONE_PUBLIC inline std::string cStringToCppString (const char *str) {
		return str ? str : "";
	}

	template<typename T>
	LINPHONE_PUBLIC const T &getEmptyConstRefObject () {
		static const T object;
		return object;
	}

	LINPHONE_PUBLIC std::tm getTimeTAsTm (time_t time);
	LINPHONE_PUBLIC time_t getTmAsTimeT (const std::tm &time);

	LINPHONE_PUBLIC std::string localeToUtf8 (const std::string &str);
	LINPHONE_PUBLIC std::string utf8ToLocale (const std::string &str);
	LINPHONE_PUBLIC std::string convertString (const std::string &str, const std::string &from, const std::string &to);
}

LINPHONE_END_NAMESPACE

#endif // ifndef _UTILS_H_
