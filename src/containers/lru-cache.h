/*
 * lru-cache.h
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

#ifndef _L_LRU_CACHE_H_
#define _L_LRU_CACHE_H_

#include <list>
#include <unordered_map>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

template<typename Key, typename Value>
class LruCache {
public:
	LruCache (int capacity = DefaultCapacity) : mCapacity(capacity) {
		if (capacity < MinCapacity)
			capacity = MinCapacity;
	}

	int getCapacity () const {
		return mCapacity;
	}

	int getSize () const {
		return int(mKeyToPair.size());
	}

	Value *operator[] (const Key &key) {
		auto it = mKeyToPair.find(key);
		return it == mKeyToPair.end() ? nullptr : &it->second.second;
	}

	const Value *operator[] (const Key &key) const {
		auto it = mKeyToPair.find(key);
		return it == mKeyToPair.cend() ? nullptr : &it->second.second;
	}

	void insert (const Key &key, const Value &value) {
		auto it = mKeyToPair.find(key);
		if (it != mKeyToPair.end())
			mKeys.erase(it->second.first);
		else if (int(mKeyToPair.size()) == mCapacity) {
			Key lastKey = mKeys.back();
			mKeys.pop_back();
			mKeyToPair.erase(lastKey);
		}

		mKeys.push_front(key);
		mKeyToPair.insert({ key, { mKeys.begin(), value } });
	}

	void insert (const Key &key, Value &&value) {
		auto it = mKeyToPair.find(key);
		if (it != mKeyToPair.end())
			mKeys.erase(it->second.first);
		else if (int(mKeyToPair.size()) == mCapacity) {
			Key lastKey = mKeys.back();
			mKeys.pop_back();
			mKeyToPair.erase(lastKey);
		}

		mKeys.push_front(key);
		mKeyToPair.insert({ key, std::make_pair(mKeys.begin(), std::move(value)) });
	}

	void clear () {
		mKeyToPair.clear();
		mKeys.clear();
	}

	static constexpr int MinCapacity = 10;
	static constexpr int DefaultCapacity = 1000;

private:
	using Pair = std::pair<typename std::list<Key>::iterator, Value>;

	const int mCapacity;

	// See: https://stackoverflow.com/questions/16781886/can-we-store-unordered-maptiterator
	// Do not store iterator key.
	std::list<Key> mKeys;
	std::unordered_map<Key, Pair> mKeyToPair;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_LRU_CACHE_H_
