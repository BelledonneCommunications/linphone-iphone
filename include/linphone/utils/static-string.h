/*
 * static-string.h
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

#ifndef _L_STATIC_STRING_H_
#define _L_STATIC_STRING_H_

#include "linphone/utils/general.h"

// =============================================================================
// Compile time strings. Useful to build const char * at compilation.
// =============================================================================

LINPHONE_BEGIN_NAMESPACE

template<std::size_t...>
struct IndexSequence {
	using Type = IndexSequence;
};

namespace Private {
	template<typename S1, typename S2>
	struct ConcatIndexSequenceImpl;

	template<std::size_t... S1, std::size_t... S2>
	struct ConcatIndexSequenceImpl<IndexSequence<S1...>, IndexSequence<S2...>> :
		IndexSequence<S1..., (sizeof...(S1) + S2)...> {};

	template<std::size_t N>
	struct MakeIndexSequenceImpl : ConcatIndexSequenceImpl<
		typename MakeIndexSequenceImpl<N / 2>::Type,
		typename MakeIndexSequenceImpl<N - N / 2>::Type
	> {};

	template<>
	struct MakeIndexSequenceImpl<0> : IndexSequence<> {};

	template<>
	struct MakeIndexSequenceImpl<1> : IndexSequence<0> {};
}

template<std::size_t N>
using MakeIndexSequence = typename Private::MakeIndexSequenceImpl<N>::Type;

// =============================================================================

// See: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4121.pdf
template<std::size_t N>
using RawStaticString = const char [N];

namespace Private {
	template<std::size_t N, typename = MakeIndexSequence<N>>
	struct StaticString;

	template<std::size_t N, std::size_t... Index>
	struct StaticString<N, IndexSequence<Index...>> {
		constexpr StaticString (const RawStaticString<N> &inRaw) : raw{ (inRaw[Index])... } {}

		constexpr char operator[] (std::size_t i) const {
			return raw[i];
		}

		constexpr operator const char * () const {
			return raw;
		}

		RawStaticString<N> raw;
	};

	template<std::size_t N>
	class StaticStringConcatHelper {
	public:
		template<std::size_t N1>
		constexpr StaticStringConcatHelper(
			const Private::StaticString<N1> &s1,
			const Private::StaticString<N - N1 + 1> &s2
		) : StaticStringConcatHelper(s1, s2, MakeIndexSequence<N1 - 1>{}, MakeIndexSequence<N - N1 + 1>{}) {}

		RawStaticString<N> raw;

	private:
		template<std::size_t N1, std::size_t... Index1, std::size_t... Index2>
		constexpr StaticStringConcatHelper(
			const Private::StaticString<N1> &s1,
			const Private::StaticString<N - N1 + 1> &s2,
			IndexSequence<Index1...>,
			IndexSequence<Index2...>
		) : raw{ s1[Index1]..., s2[Index2]... } {}
	};

	template<int Value, int N = getIntLength(Value) + 1>
	class StaticIntStringHelper {
	public:
		constexpr StaticIntStringHelper () : StaticIntStringHelper(MakeIndexSequence<Value >= 0 ? N - 1 : N - 2>{}) {}

		RawStaticString<N> raw;

	private:
		// Workaround for MSVC 2015.
		// See: https://stackoverflow.com/questions/41593649/why-wont-visual-studio-let-me-use-a-templatized-constexpr-function-in-enable-i/41597153
		struct IsNeg { static const bool value = Value < 0; };

		template<std::size_t... Index, typename Int = int, typename std::enable_if<!IsNeg::value, Int>::type* = nullptr>
		constexpr StaticIntStringHelper (const IndexSequence<Index...> &) :
			raw{ char('0' + Value / pow10(N - Index - 2) % 10)..., '\0' } {}

		template<std::size_t... Index, typename Int = int, typename std::enable_if<IsNeg::value, Int>::type* = nullptr>
		constexpr StaticIntStringHelper (const IndexSequence<Index...> &) :
			raw{ '-', char('0' + abs(Value) / pow10(N - Index - 3) % 10)..., '\0' } {}
	};
};

// -----------------------------------------------------------------------------

template<std::size_t N>
constexpr Private::StaticString<N> StaticString (const RawStaticString<N> &raw) {
	return Private::StaticString<N>(raw);
}

template<std::size_t N1, std::size_t N2>
constexpr Private::StaticString<N1 + N2 - 1> operator+ (
	const Private::StaticString<N1> &s1,
	const Private::StaticString<N2> &s2
) {
	return StaticString(Private::StaticStringConcatHelper<N1 + N2 - 1>(s1, s2).raw);
}

template<std::size_t N1, std::size_t N2>
constexpr Private::StaticString<N1 + N2 - 1> operator+ (
	const Private::StaticString<N1> &s1,
	const RawStaticString<N2> &s2
) {
	return StaticString(Private::StaticStringConcatHelper<N1 + N2 - 1>(s1, StaticString(s2)).raw);
}

template<std::size_t N1, std::size_t N2>
constexpr Private::StaticString<N1 + N2 - 1> operator+ (
	const RawStaticString<N2> &s1,
	const Private::StaticString<N1> &s2
) {
	return StaticString(Private::StaticStringConcatHelper<N1 + N2 - 1>(StaticString(s1), s2).raw);
}

// -----------------------------------------------------------------------------

template<int Value = 0>
constexpr Private::StaticString<getIntLength(Value) + 1> StaticIntString () {
	return StaticString(Private::StaticIntStringHelper<Value>().raw);
}

// -----------------------------------------------------------------------------

LINPHONE_END_NAMESPACE

#endif // ifndef _L_STATIC_STRING_H_
