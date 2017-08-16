/*
 * cpim-parser.h
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

#ifndef _CPIM_PARSER_H_
#define _CPIM_PARSER_H_

#include "cpim/message/cpim-message.h"
#include "object/singleton.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

namespace Cpim {
	class ParserPrivate;

	class Parser : public Singleton<Parser> {
		friend class Singleton<Parser>;

	public:
		std::shared_ptr<Message> parseMessage (const std::string &input);

		std::shared_ptr<Header> cloneHeader (const Header &header);

		bool headerNameIsValid (const std::string &headerName) const;
		bool headerValueIsValid (const std::string &headerValue) const;
		bool headerParameterIsValid (const std::string &headerParameter) const;

		template<typename>
		bool coreHeaderIsValid (const std::string &headerValue) const {
			return false;
		}

		bool subjectHeaderLanguageIsValid (const std::string &language) const;

	private:
		Parser ();

		L_DECLARE_PRIVATE(Parser);
		L_DISABLE_COPY(Parser);
	};

	// ---------------------------------------------------------------------------

	template<>
	bool Parser::coreHeaderIsValid<FromHeader>(const std::string &headerValue) const;

	template<>
	bool Parser::coreHeaderIsValid<ToHeader>(const std::string &headerValue) const;

	template<>
	bool Parser::coreHeaderIsValid<CcHeader>(const std::string &headerValue) const;

	template<>
	bool Parser::coreHeaderIsValid<DateTimeHeader>(const std::string &headerValue) const;

	template<>
	bool Parser::coreHeaderIsValid<SubjectHeader>(const std::string &headerValue) const;

	template<>
	bool Parser::coreHeaderIsValid<NsHeader>(const std::string &headerValue) const;

	template<>
	bool Parser::coreHeaderIsValid<RequireHeader>(const std::string &headerValue) const;
}

LINPHONE_END_NAMESPACE

#endif // ifndef _CPIM_PARSER_H_
