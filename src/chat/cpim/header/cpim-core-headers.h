/*
 * cpim-core-headers.h
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

#ifndef _CPIM_CORE_HEADERS_H_
#define _CPIM_CORE_HEADERS_H_

#include "cpim-header.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

#define MAKE_CORE_HEADER(CLASS_PREFIX, NAME) \
	class LINPHONE_PUBLIC CLASS_PREFIX ## Header : public CoreHeader { \
	public: \
		CLASS_PREFIX ## Header() = default; \
		inline std::string getName() const override { \
			return NAME; \
		} \
		bool setValue(const std::string &value) override; \
	private: \
		L_DISABLE_COPY(CLASS_PREFIX ## Header); \
	};

namespace Cpim {
	class HeaderNode;

	// -------------------------------------------------------------------------
	// Generic core header.
	// -------------------------------------------------------------------------

	class LINPHONE_PUBLIC CoreHeader : public Header {
		friend class HeaderNode;

	public:
		CoreHeader ();

		virtual ~CoreHeader () = 0;

		bool isValid () const override;

	protected:
		explicit CoreHeader (HeaderPrivate &p);

		void force (const std::string &value);

	private:
		L_DISABLE_COPY(CoreHeader);
	};

	// -------------------------------------------------------------------------
	// Core headers.
	// -------------------------------------------------------------------------

	MAKE_CORE_HEADER(From, "From");
	MAKE_CORE_HEADER(To, "To");
	MAKE_CORE_HEADER(Cc, "cc");
	MAKE_CORE_HEADER(DateTime, "DateTime");
	MAKE_CORE_HEADER(Ns, "NS");
	MAKE_CORE_HEADER(Require, "Require");

	// -------------------------------------------------------------------------
	// Specific Subject declaration.
	// -------------------------------------------------------------------------

	class SubjectHeaderPrivate;

	class LINPHONE_PUBLIC SubjectHeader : public CoreHeader {
		friend class HeaderNode;

	public:
		SubjectHeader ();

		inline std::string getName () const override {
			return "Subject";
		}

		bool setValue (const std::string &value) override;

		std::string getLanguage () const;
		bool setLanguage (const std::string &language);

		std::string asString () const override;

	protected:
		void force (const std::string &value, const std::string &language);

	private:
		L_DECLARE_PRIVATE(SubjectHeader);
		L_DISABLE_COPY(SubjectHeader);
	};
}

#undef MAKE_CORE_HEADER

LINPHONE_END_NAMESPACE

#endif // ifndef _CPIM_CORE_HEADERS_H_
