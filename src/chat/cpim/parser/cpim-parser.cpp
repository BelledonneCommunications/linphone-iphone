/*
 * cpim-parser.cpp
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

#include <unordered_map>

#include <belr/abnf.h>
#include <belr/grammarbuilder.h>

#include "linphone/utils/utils.h"

#include "content/content-type.h"
#include "logger/logger.h"
#include "object/object-p.h"

#include "cpim-parser.h"
#ifdef __APPLE__
#include <TargetConditionals.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#define CPIM_GRAMMAR "cpim_grammar"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

namespace Cpim {
	class Node {
	public:
		virtual ~Node () = default;
	};

	class HeaderNode : public Node {
	public:
		HeaderNode () = default;

		explicit HeaderNode (const Header &header) : mName(header.getName()), mValue(header.getValue()) {
			// Generic header.
			const GenericHeader *genericHeader = dynamic_cast<const GenericHeader *>(&header);
			if (genericHeader) {
				for (const auto &parameter : *genericHeader->getParameters())
					mParameters += ";" + parameter.first + "=" + parameter.second;
				return;
			}

			// Subject header.
			const SubjectHeader *subjectHeader = dynamic_cast<const SubjectHeader *>(&header);
			if (subjectHeader) {
				const string language = subjectHeader->getLanguage();
				if (!language.empty())
					mParameters = ";lang=" + language;
			}
		}

		string getName () const {
			return mName;
		}

		void setName (const string &name) {
			mName = name;
		}

		string getParameters () const {
			return mParameters;
		}

		void setParameters (const string &parameters) {
			mParameters = parameters;
		}

		string getValue () const {
			return mValue;
		}

		void setValue (const string &value) {
			mValue = value;
		}

		shared_ptr<Header> createHeader (bool force) const;

	private:
		template<typename T>
		shared_ptr<Header> createCoreHeader (bool force) const {
			shared_ptr<T> header = make_shared<T>();
			if (force)
				header->force(mValue);
			else if (!header->setValue(mValue)) {
				lWarning() << "Unable to set value on core header: `" << mName << "` => `" << mValue << "`.";
				return nullptr;
			}

			return header;
		}

		string mName;
		string mValue;
		string mParameters;
	};

	template<>
	shared_ptr<Header> HeaderNode::createCoreHeader<SubjectHeader>(bool force) const {
		shared_ptr<SubjectHeader> header = make_shared<SubjectHeader>();
		const string language = mParameters.length() >= 6 ? mParameters.substr(6) : "";

		if (force)
			header->force(mValue, language);
		else if (!header->setValue(mValue) || (!language.empty() && !header->setLanguage(language))) {
			lWarning() << "Unable to set value on subject header: `" <<
				mName << "` => `" << mValue << "`, `" << language << "`.";
			return nullptr;
		}

		return header;
	}

	shared_ptr<Header> HeaderNode::createHeader (bool force = false) const {
		static const unordered_map<string, shared_ptr<Header>(HeaderNode::*)(bool)const> reservedHandlers = {
			{ "From", &HeaderNode::createCoreHeader<FromHeader> },
			{ "To", &HeaderNode::createCoreHeader<ToHeader> },
			{ "cc", &HeaderNode::createCoreHeader<CcHeader> },
			{ "DateTime", &HeaderNode::createCoreHeader<DateTimeHeader> },
			{ "Message-ID", &HeaderNode::createCoreHeader<MessageIdHeader> },
			{ "Subject", &HeaderNode::createCoreHeader<SubjectHeader> },
			{ "NS", &HeaderNode::createCoreHeader<NsHeader> },
			{ "Require", &HeaderNode::createCoreHeader<RequireHeader> }
		};

		// Core Header.
		const auto it = reservedHandlers.find(mName);
		if (it != reservedHandlers.cend())
			return (this->*it->second)(force);

		// Generic Header
		shared_ptr<GenericHeader> genericHeader = make_shared<GenericHeader>();
		genericHeader->force(mName, mValue, mParameters);
		return genericHeader;
	}

	// -------------------------------------------------------------------------

	class ListHeaderNode :
		public Node,
		public list<shared_ptr<HeaderNode> > {};

	// -------------------------------------------------------------------------

	class MessageNode : public Node {
	public:
		void addHeaders (const shared_ptr<ListHeaderNode> &headers) {
			mHeaders->push_back(headers);
		}

		// Warning: Call this function one time!
		shared_ptr<Message> createMessage () const {
			size_t size = mHeaders->size();
			if (size < 2 || size > 3) { // TODO: Check that size is == 2
				lWarning() << "Bad headers lists size.";
				return nullptr;
			}

			const shared_ptr<Message> message = make_shared<Message>();

			// TODO: To remove
			if (size == 3) {
				const shared_ptr<ListHeaderNode> cpimHeaders = mHeaders->front();
				if (find_if(cpimHeaders->cbegin(), cpimHeaders->cend(),
							[](const shared_ptr<const HeaderNode> &headerNode) {
								return Utils::iequals(headerNode->getName(), "content-type") && (ContentType(headerNode->getValue()) == ContentType::Cpim);
							}) == cpimHeaders->cend()) {
					lWarning() << "No MIME `Content-Type` found!";
					return nullptr;
				}

				// Add MIME headers.
				for (const auto &headerNode : *cpimHeaders) {
					const shared_ptr<const Header> header = headerNode->createHeader();
					if (!header || !message->addCpimHeader(*header))
						return nullptr;
				}

				// Add message headers.
				for (const auto &headerNode : **(++mHeaders->cbegin())) {
					const shared_ptr<const Header> header = headerNode->createHeader();
					if (!header || !message->addMessageHeader(*header))
						return nullptr;
				}
			}
			// TODO: To remove
			else {
				// Add message headers.
				for (const auto &headerNode : *mHeaders->front()) {
					const shared_ptr<const Header> header = headerNode->createHeader();
					if (!header || !message->addMessageHeader(*header))
						return nullptr;
				}
			}

			// Add content headers.
			for (const auto &headerNode : *mHeaders->back()) {
				const shared_ptr<const Header> header = headerNode->createHeader();
				if (!header || !message->addContentHeader(*header))
					return nullptr;
			}

			return message;
		}

	private:
		shared_ptr<list<shared_ptr<ListHeaderNode> > > mHeaders = make_shared<list<shared_ptr<ListHeaderNode> > >();
	};
}

// -----------------------------------------------------------------------------

class Cpim::ParserPrivate : public ObjectPrivate {
public:
	shared_ptr<belr::Grammar> grammar;
};

Cpim::Parser::Parser () : Singleton(*new ParserPrivate) {
	L_D();
	#if TARGET_OS_IPHONE
		CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("org.linphone.linphone"));
		CFURLRef grammarUrl = CFBundleCopyResourceURL(bundle, CFSTR(CPIM_GRAMMAR), nullptr, nullptr);
		CFStringRef grammarPath = CFURLCopyFileSystemPath(grammarUrl, kCFURLPOSIXPathStyle);
		CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
		const char *path = CFStringGetCStringPtr(grammarPath, encodingMethod);
		CFRelease(grammarUrl);
		CFRelease(grammarPath);
	#else
		const char *path = CPIM_GRAMMAR;
	#endif
	d->grammar = belr::GrammarLoader::get().load(path);
	if (!d->grammar)
		lFatal() << "Unable to load CPIM grammar.";
}

// -----------------------------------------------------------------------------

shared_ptr<Cpim::Message> Cpim::Parser::parseMessage (const string &input) {
	L_D();

	typedef void (list<shared_ptr<HeaderNode> >::*pushPtr)(const shared_ptr<HeaderNode> &value);

	belr::Parser<shared_ptr<Node> > parser(d->grammar);
	parser.setHandler(
		"Message", belr::make_fn(make_shared<MessageNode> )
	)->setCollector(
		"Headers", belr::make_sfn(&MessageNode::addHeaders)
	);

	parser.setHandler(
		"Headers", belr::make_fn(make_shared<ListHeaderNode> )
	)->setCollector(
		"Header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back))
	);

	parser.setHandler(
		"Header", belr::make_fn(make_shared<HeaderNode> )
	)->setCollector(
		"Header-name", belr::make_sfn(&HeaderNode::setName)
	)->setCollector(
		"Header-value", belr::make_sfn(&HeaderNode::setValue)
	)->setCollector(
		"Header-parameters", belr::make_sfn(&HeaderNode::setParameters)
	);

	size_t parsedSize;
	shared_ptr<Node> node = parser.parseInput("Message", input, &parsedSize);
	if (!node) {
		lWarning() << "Unable to parse message.";
		return nullptr;
	}

	shared_ptr<MessageNode> messageNode = dynamic_pointer_cast<MessageNode>(node);
	if (!messageNode) {
		lWarning() << "Unable to cast belr result to message node.";
		return nullptr;
	}

	shared_ptr<Message> message = messageNode->createMessage();
	if (message) {
		message->setContent(input.substr(parsedSize));
	}
	return message;
}

// -----------------------------------------------------------------------------

shared_ptr<Cpim::Header> Cpim::Parser::cloneHeader (const Header &header) {
	return HeaderNode(header).createHeader(true);
}

// -----------------------------------------------------------------------------

class EmptyObject {};

static bool headerIsValid (const shared_ptr<belr::Grammar> &grammar, const string &input) {
	belr::Parser<shared_ptr<EmptyObject> > parser(grammar);
	parser.setHandler(
		"Header", belr::make_fn(make_shared<EmptyObject> )
	);

	size_t parsedSize;
	shared_ptr<EmptyObject> node = parser.parseInput("Header", input, &parsedSize);
	return node && parsedSize == input.length();
}

bool Cpim::Parser::headerNameIsValid (const string &headerName) const {
	L_D();
	return headerIsValid(d->grammar, headerName + ": value\r\n");
}

bool Cpim::Parser::headerValueIsValid (const string &headerValue) const {
	L_D();
	return headerIsValid(d->grammar, "key: " + headerValue + "\r\n");
}

bool Cpim::Parser::headerParameterIsValid (const string &headerParameter) const {
	L_D();
	return headerIsValid(d->grammar, "key:;" + headerParameter + " value\r\n");
}

// -----------------------------------------------------------------------------

static bool coreHeaderIsValid (
	const shared_ptr<belr::Grammar> &grammar,
	const string &headerName,
	const string &headerValue,
	const string &headerParams = string()
) {
	const string mainRule = headerName + "-header";

	belr::Parser<shared_ptr<EmptyObject> > parser(grammar);
	parser.setHandler(
		mainRule, belr::make_fn(make_shared<EmptyObject> )
	);

	const string input = headerName + ":" + headerParams + " " + headerValue;

	size_t parsedSize;
	shared_ptr<EmptyObject> node = parser.parseInput(mainRule, input, &parsedSize);
	return node && parsedSize == input.length();
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::FromHeader> (const string &headerValue) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "From", headerValue);
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::ToHeader> (const string &headerValue) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "To", headerValue);
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::CcHeader> (const string &headerValue) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "cc", headerValue);
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::DateTimeHeader> (const string &headerValue) const {
	static const int daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	L_D();
	if (!LinphonePrivate::coreHeaderIsValid(d->grammar, "DateTime", headerValue))
		return false;

	// Check date.
	const int year = Utils::stoi(headerValue.substr(0, 4));
	const bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;

	const int month = Utils::stoi(headerValue.substr(5, 2));
	if (month < 1 || month > 12)
		return false;

	const int day = Utils::stoi(headerValue.substr(8, 2));
	if (day < 1 || (month == 2 && isLeapYear ? day > 29 : day > daysInMonth[month - 1]))
		return false;

	// Check time.
	if (
		Utils::stoi(headerValue.substr(11, 2)) > 24 ||
		Utils::stoi(headerValue.substr(14, 2)) > 59 ||
		Utils::stoi(headerValue.substr(17, 2)) > 60
	)
		return false;

	// Check num offset.
	if (headerValue.back() != 'Z') {
		size_t length = headerValue.length();
		if (
			Utils::stoi(headerValue.substr(length - 5, 2)) > 24 ||
			Utils::stoi(headerValue.substr(length - 2, 2)) > 59
		)
			return false;
	}

	return true;
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::MessageIdHeader> (const string &headerValue) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "Message-ID", headerValue);
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::SubjectHeader> (const string &headerValue) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "Subject", headerValue);
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::NsHeader> (const string &headerValue) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "NS", headerValue);
}

template<>
bool Cpim::Parser::coreHeaderIsValid<Cpim::RequireHeader> (const string &headerValue) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "Require", headerValue);
}

// -----------------------------------------------------------------------------

bool Cpim::Parser::subjectHeaderLanguageIsValid (const string &language) const {
	L_D();
	return LinphonePrivate::coreHeaderIsValid(d->grammar, "Subject", "SubjectValue", ";lang=" + language);
}

LINPHONE_END_NAMESPACE
