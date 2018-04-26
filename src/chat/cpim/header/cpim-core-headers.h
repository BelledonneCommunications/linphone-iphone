/*
 * cpim-core-headers.h
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

#ifndef _L_CPIM_CORE_HEADERS_H_
#define _L_CPIM_CORE_HEADERS_H_

#include <ctime>
#include <list>

#include "cpim-header.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

#define MAKE_CONTACT_HEADER(CLASS_PREFIX, NAME) \
	class LINPHONE_PUBLIC CLASS_PREFIX ## Header : public ContactHeader { \
	public: \
		CLASS_PREFIX ## Header () = default; \
		CLASS_PREFIX ## Header (const std::string &uri, const std::string &formalName = "") : ContactHeader (uri, formalName) {} \
		inline std::string getName () const override { \
			return NAME; \
		} \
	private: \
		L_DISABLE_COPY(CLASS_PREFIX ## Header); \
	};

namespace Cpim {
	class DateTimeHeaderNode;

	// -------------------------------------------------------------------------
	// Specific Contact headers declaration.
	// -------------------------------------------------------------------------

	class ContactHeaderPrivate;

	class LINPHONE_PUBLIC ContactHeader : public Header {
	public:
		ContactHeader ();

		ContactHeader (const std::string &uri, const std::string &formalName = "");

		std::string getUri () const;
		void setUri (const std::string &uri);

		std::string getFormalName () const;
		void setFormalName (const std::string &formalName);

		std::string getValue () const override;

		std::string asString () const override;

	private:
		L_DECLARE_PRIVATE(ContactHeader);
		L_DISABLE_COPY(ContactHeader);
	};

	// -------------------------------------------------------------------------

	MAKE_CONTACT_HEADER(From, "From");
	MAKE_CONTACT_HEADER(To, "To");
	MAKE_CONTACT_HEADER(Cc, "cc");

	// -------------------------------------------------------------------------
	// Specific DateTime declaration.
	// -------------------------------------------------------------------------

	class DateTimeHeaderPrivate;

	class LINPHONE_PUBLIC DateTimeHeader : public Header {
		friend class DateTimeHeaderNode;

	public:
		DateTimeHeader ();

		DateTimeHeader (time_t time);

		DateTimeHeader (const tm &time, const tm &timeOffset, const std::string &signOffset);

		inline std::string getName () const override {
			return "DateTime";
		}

		time_t getTime () const;
		void setTime (const time_t time);

		void setTime  (const tm &time, const tm &timeOffset, const std::string &signOffset);

		std::string getValue () const override;

		std::string asString () const override;

	private:
		tm getTimeStruct () const;
		tm getTimeOffset () const;
		std::string getSignOffset () const;

		L_DECLARE_PRIVATE(DateTimeHeader);
		L_DISABLE_COPY(DateTimeHeader);
	};

	// -------------------------------------------------------------------------
	// Specific Ns declaration.
	// -------------------------------------------------------------------------

	class NsHeaderPrivate;

	class LINPHONE_PUBLIC NsHeader : public Header {
	public:
		NsHeader ();

		NsHeader (const std::string &uri, const std::string &prefixName = "");

		inline std::string getName () const override {
			return "NS";
		}

		std::string getPrefixName () const;
		void setPrefixName (const std::string &prefixName);

		std::string getUri () const;
		void setUri (const std::string &uri);

		std::string getValue () const override;

		std::string asString () const override;

	private:
		L_DECLARE_PRIVATE(NsHeader);
		L_DISABLE_COPY(NsHeader);
	};

	// -------------------------------------------------------------------------
	// Specific Require declaration.
	// -------------------------------------------------------------------------

	class RequireHeaderPrivate;

	class LINPHONE_PUBLIC RequireHeader : public Header {
	public:
		RequireHeader ();

		RequireHeader (const std::string &headerNames);
		RequireHeader (const std::list<std::string> &headerNames);

		inline std::string getName () const override {
			return "Require";
		}

		std::list<std::string> getHeaderNames () const;
		void addHeaderName (const std::string &headerName);

		std::string getValue () const override;

		std::string asString () const override;

	private:
		L_DECLARE_PRIVATE(RequireHeader);
		L_DISABLE_COPY(RequireHeader);
	};

	// -------------------------------------------------------------------------
	// Specific Subject declaration.
	// -------------------------------------------------------------------------

	class SubjectHeaderPrivate;

	class LINPHONE_PUBLIC SubjectHeader : public Header {
	public:
		SubjectHeader ();

		SubjectHeader (const std::string &subject, const std::string &language = "");

		inline std::string getName () const override {
			return "Subject";
		}

		std::string getSubject () const;
		void setSubject (const std::string &subject);

		std::string getLanguage () const;
		void setLanguage (const std::string &language);

		std::string getValue () const override;

		std::string asString () const override;

	private:
		L_DECLARE_PRIVATE(SubjectHeader);
		L_DISABLE_COPY(SubjectHeader);
	};
}

#undef MAKE_CONTACT_HEADER

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CPIM_CORE_HEADERS_H_
