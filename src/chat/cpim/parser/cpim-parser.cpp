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

#include <set>

#include <belr/abnf.h>
#include <belr/grammarbuilder.h>

#include "linphone/utils/utils.h"

#include "content/content-type.h"
#include "logger/logger.h"
#include "object/object-p.h"

#include "cpim-parser.h"

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
			const GenericHeader *genericHeader = dynamic_cast<const GenericHeader *>(&header);
			if (genericHeader) {
				for (const auto &parameter : *genericHeader->getParameters())
					mParameters += ";" + parameter.first + "=" + parameter.second;
			}
		}

		string getName () const {
			return mName;
		}

		void setName (const string &name) {
			static const set<string> reserved = {
				"From", "To", "cc", "DateTime", "Subject", "NS", "Require"
			};

			if (reserved.find(name) == reserved.end())
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

		virtual shared_ptr<Header> createHeader () const;

		virtual bool isValid () const;

	private:
		string mName;
		string mValue;
		string mParameters;
	};

	bool HeaderNode::isValid () const {
		return !mName.empty() && !mValue.empty();
	}

	shared_ptr<Header> HeaderNode::createHeader () const {
		if (!isValid())
			return nullptr;

		shared_ptr<GenericHeader> genericHeader = make_shared<GenericHeader>();
		genericHeader->setName(mName);

		for (const auto &parameter : Utils::split(mParameters, ';')) {
			size_t equalIndex = parameter.find('=');
			if (equalIndex != string::npos)
				genericHeader->addParameter(parameter.substr(0, equalIndex), parameter.substr(equalIndex + 1));
		}

		genericHeader->setValue(mValue);
		return genericHeader;
	}

	// -------------------------------------------------------------------------

	class ContactHeaderNode : public HeaderNode {
	public:
		ContactHeaderNode () = default;

		string getFormalName () const {
			return mFormalName;
		}

		void setFormalName (const string &formalName) {
			mFormalName = formalName;
		}

		string getUri () const {
			return mUri;
		}

		void setUri (const string &uri) {
			mUri = uri;
		}

		bool isValid () const override;

	private:
		string mFormalName;
		string mUri;
	};

	bool ContactHeaderNode::isValid () const {
		return !mUri.empty();
	}

	// -------------------------------------------------------------------------

	class FromHeaderNode : public ContactHeaderNode {
	public:
		FromHeaderNode () = default;

		explicit FromHeaderNode (const Header &header) {
			const FromHeader *fromHeader = dynamic_cast<const FromHeader *>(&header);
			if (fromHeader) {
				setFormalName(fromHeader->getFormalName());
				setUri(fromHeader->getUri());
			}
		}

		shared_ptr<Header> createHeader () const override;
	};

	shared_ptr<Header> FromHeaderNode::createHeader () const {
		if (!isValid())
			return nullptr;

		return make_shared<FromHeader>(getUri(), getFormalName());
	}

	// -------------------------------------------------------------------------

	class ToHeaderNode : public ContactHeaderNode {
	public:
		ToHeaderNode () = default;

		explicit ToHeaderNode (const Header &header) {
			const ToHeader *toHeader = dynamic_cast<const ToHeader *>(&header);
			if (toHeader) {
				setFormalName(toHeader->getFormalName());
				setUri(toHeader->getUri());
			}
		}

		shared_ptr<Header> createHeader () const override;
	};

	shared_ptr<Header> ToHeaderNode::createHeader () const {
		if (!isValid())
			return nullptr;

		return make_shared<ToHeader>(getUri(), getFormalName());
	}

	// -------------------------------------------------------------------------

	class CcHeaderNode : public ContactHeaderNode {
	public:
		CcHeaderNode () = default;

		explicit CcHeaderNode (const Header &header) {
			const CcHeader *ccHeader = dynamic_cast<const CcHeader *>(&header);
			if (ccHeader) {
				setFormalName(ccHeader->getFormalName());
				setUri(ccHeader->getUri());
			}
		}

		shared_ptr<Header> createHeader () const override;
	};

	shared_ptr<Header> CcHeaderNode::createHeader () const {
		if (!isValid())
			return nullptr;

		return make_shared<CcHeader>(getUri(), getFormalName());
	}

	// -------------------------------------------------------------------------

	class DateTimeOffsetNode : public Node {
		friend class DateTimeHeaderNode;

	public:
		DateTimeOffsetNode () {
			mSign = "Z";
		}

		void setHour (const string &value) {
			mHour = Utils::stoi(value);
		}

		void setMinute (const string &value) {
			mMinute = Utils::stoi(value);
		}

		void setSign (const string &value) {
			mSign = value;
		}

	private:
		string mSign;
		int mHour;
		int mMinute;
	};

	class DateTimeHeaderNode : public HeaderNode {
	public:
		DateTimeHeaderNode () = default;

		explicit DateTimeHeaderNode (const Header &header) {
			const DateTimeHeader *dateTimeHeader = dynamic_cast<const DateTimeHeader *>(&header);
			if (dateTimeHeader) {
				setTime(dateTimeHeader->getTimeStruct());
				setTimeOffset(dateTimeHeader->getTimeOffset());
				setSignOffset(dateTimeHeader->getSignOffset());
			}
		}

		struct tm getTime () const {
			return mTime;
		}

		void setTime (const struct tm &time) {
			mTime = time;
		}

		struct tm getTimeOffset () const {
			return mTimeOffset;
		}

		void setTimeOffset (const struct tm &timeOffset) {
			mTimeOffset = timeOffset;
		}

		string getSignOffset () const {
			return mSignOffset;
		}

		void setSignOffset (const string &signOffset) {
			mSignOffset = signOffset;
		}

		void setYear (const string &value) {
			mTime.tm_year = Utils::stoi(value);
		}

		void setMonth (const string &value) {
			mTime.tm_mon = Utils::stoi(value) - 1;
		}

		void setMonthDay (const string &value) {
			mTime.tm_mday = Utils::stoi(value);
		}

		void setHour (const string &value) {
			mTime.tm_hour = Utils::stoi(value);
		}

		void setMinute (const string &value) {
			mTime.tm_min = Utils::stoi(value);
		}

		void setSecond (const string &value) {
			mTime.tm_sec = Utils::stoi(value);
		}

		void setOffset (const shared_ptr<DateTimeOffsetNode> &offset) {
			mTimeOffset.tm_hour = offset->mHour;
			mTimeOffset.tm_min = offset->mMinute;
			mSignOffset = offset->mSign;
		}

		bool isValid () const override;

		shared_ptr<Header> createHeader() const override;

	private:
		tm mTime;
		tm mTimeOffset;
		string mSignOffset;
	};

	bool DateTimeHeaderNode::isValid () const {
		static const int daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

		// Check date.
		const bool isLeapYear = (mTime.tm_year % 4 == 0 && mTime.tm_year % 100 != 0) || mTime.tm_year % 400 == 0;

		if (mTime.tm_mon < 1 || mTime.tm_mon > 12)
			return false;

		if (mTime.tm_mday < 1 || (mTime.tm_mon == 2 && isLeapYear ? mTime.tm_mday > 29 : mTime.tm_mday > daysInMonth[mTime.tm_mon]))
			return false;

		// Check time.
		if (mTime.tm_hour > 24 || mTime.tm_min > 59 || mTime.tm_sec > 60)
			return false;

		// Check num offset.
		if (mSignOffset != "Z") {
			if (mTimeOffset.tm_hour > 24 || mTime.tm_min > 59)
				return false;
		}

		return true;
	}

	shared_ptr<Header> DateTimeHeaderNode::createHeader () const {
		if (!isValid())
			return nullptr;

		return make_shared<DateTimeHeader>(getTime(), getTimeOffset(), getSignOffset());
	}

	// -------------------------------------------------------------------------

	class SubjectHeaderNode : public HeaderNode {
	public:
		SubjectHeaderNode () = default;

		explicit SubjectHeaderNode (const Header &header) {
			const SubjectHeader *subjectHeader = dynamic_cast<const SubjectHeader *>(&header);
			if (subjectHeader) {
				setLanguage(subjectHeader->getLanguage());
				setSubject(subjectHeader->getSubject());
			}
		}

		string getLanguage () const {
			return mLanguage;
		}

		void setLanguage (const string &language) {
			mLanguage = language;
		}

		string getSubject () const {
			return mSubject;
		}

		void setSubject (const string &subject) {
			mSubject = subject;
		}

		bool isValid () const override;

		shared_ptr<Header> createHeader () const override;

	private:
		string mLanguage;
		string mSubject;
	};

	bool SubjectHeaderNode::isValid () const {
		return !mSubject.empty();
	}

	shared_ptr<Header> SubjectHeaderNode::createHeader () const {
		if (!isValid())
			return nullptr;

		return make_shared<SubjectHeader>(getSubject(), getLanguage());
	}

	// -------------------------------------------------------------------------

	class NsHeaderNode : public HeaderNode {
	public:
		NsHeaderNode () = default;

		explicit NsHeaderNode (const Header &header) {
			const NsHeader *nsHeader = dynamic_cast<const NsHeader *>(&header);
			if (nsHeader) {
				setPrefixName(nsHeader->getPrefixName());
				setUri(nsHeader->getUri());
			}
		}

		string getPrefixName () const {
			return mPrefixName;
		}

		void setPrefixName (const string &prefixName) {
			mPrefixName = prefixName;
		}

		string getUri () const {
			return mUri;
		}

		void setUri (const string &uri) {
			mUri = uri;
		}

		bool isValid () const override;

		shared_ptr<Header> createHeader () const override;

	private:
		string mPrefixName;
		string mUri;
	};

	bool NsHeaderNode::isValid () const {
		return !mUri.empty();
	}

	shared_ptr<Header> NsHeaderNode::createHeader () const {
		if (!isValid())
			return nullptr;

		return make_shared<NsHeader>(getUri(), getPrefixName());
	}

	// -------------------------------------------------------------------------

	class RequireHeaderNode : public HeaderNode {
	public:
		RequireHeaderNode () = default;

		explicit RequireHeaderNode (const Header &header) {
			const RequireHeader *requireHeader = dynamic_cast<const RequireHeader *>(&header);
			if (requireHeader) {
				for (const auto &header : requireHeader->getHeaderNames()) {
					if (header != requireHeader->getHeaderNames().front())
						mHeaderNames += ",";
					mHeaderNames += header;
				}
			}
		}

		string getHeaderNames () const {
			return mHeaderNames;
		}

		void setHeaderNames (const string &headerNames) {
			mHeaderNames = headerNames;
		}

		bool isValid () const override;

		shared_ptr<Header> createHeader () const override;

	private:
		string mHeaderNames;
	};

	bool RequireHeaderNode::isValid () const {
		return !mHeaderNames.empty();
	}

	shared_ptr<Header> RequireHeaderNode::createHeader () const  {
		if (!isValid())
			return nullptr;

		return make_shared<RequireHeader>(mHeaderNames);
	}

	// -------------------------------------------------------------------------

	class ListHeaderNode :
		public Node,
		public list<shared_ptr<HeaderNode> > {};

	// -------------------------------------------------------------------------

	class MessageNode : public Node {
	public:
		void addMessageHeaders (const shared_ptr<ListHeaderNode> &headers) {
			for (const auto &headerNode : *headers) {
				mMessageHeaders.push_back(headerNode);
			}
		}

		void addContentHeaders (const shared_ptr<ListHeaderNode> &headers) {
			for (const auto &headerNode : *headers) {
				mContentHeaders.push_back(headerNode);
			}
		}

		// Warning: Call this function one time!
		shared_ptr<Message> createMessage () const {
			if (mContentHeaders.empty() || mMessageHeaders.empty()) {
				lWarning() << "Bad headers lists size.";
				return nullptr;
			}

			//TODO: Verify all headers from other namespaces

			const shared_ptr<Message> message = make_shared<Message>();

			// Add message headers.
			for (const auto &headerNode : mMessageHeaders) {
				string ns = "";

				string::size_type n = headerNode->getName().find(".");
				if (n != string::npos) {
					ns = headerNode->getName().substr(0, n);
					headerNode->setName(headerNode->getName().substr(n + 1));
				}

				const shared_ptr<const Header> header = headerNode->createHeader();
				if (!header)
					return nullptr;

				message->addMessageHeader(*header, ns);
			}

			// Add content headers.
			for (const auto &headerNode : mContentHeaders) {
				const shared_ptr<const Header> header = headerNode->createHeader();
				if (!header)
					return nullptr;

				message->addContentHeader(*header);
			}

			return message;
		}

	private:
		list<shared_ptr<HeaderNode>> mContentHeaders;
		list<shared_ptr<HeaderNode>> mMessageHeaders;
	};
}

// -----------------------------------------------------------------------------

class Cpim::ParserPrivate : public ObjectPrivate {
public:
	shared_ptr<belr::Grammar> grammar;
};

Cpim::Parser::Parser () : Singleton(*new ParserPrivate) {
	L_D();
	d->grammar = belr::GrammarLoader::get().load(CPIM_GRAMMAR);
	if (!d->grammar)
		lFatal() << "Unable to load CPIM grammar.";
}

// -----------------------------------------------------------------------------

shared_ptr<Cpim::Message> Cpim::Parser::parseMessage (const string &input) {
	L_D();

	typedef void (list<shared_ptr<HeaderNode> >::*pushPtr)(const shared_ptr<HeaderNode> &value);

	belr::Parser<shared_ptr<Node> > parser(d->grammar);
	parser.setHandler("Message", belr::make_fn(make_shared<MessageNode>))
		->setCollector("Message-headers", belr::make_sfn(&MessageNode::addMessageHeaders))
		->setCollector("Content-headers", belr::make_sfn(&MessageNode::addContentHeaders));

	parser.setHandler("Message-headers", belr::make_fn(make_shared<ListHeaderNode>))
		->setCollector("Header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)))
		->setCollector("From-header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)))
		->setCollector("To-header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)))
		->setCollector("DateTime-header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)))
		->setCollector("cc-header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)))
		->setCollector("Subject-header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)))
		->setCollector("NS-header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)))
		->setCollector("Require-header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)));

	parser.setHandler("Content-headers", belr::make_fn(make_shared<ListHeaderNode>))
		->setCollector("Header", belr::make_sfn(static_cast<pushPtr>(&ListHeaderNode::push_back)));

	parser.setHandler("Header", belr::make_fn(make_shared<HeaderNode>))
		->setCollector("Header-name", belr::make_sfn(&HeaderNode::setName))
		->setCollector("Header-value", belr::make_sfn(&HeaderNode::setValue))
		->setCollector("Header-parameters", belr::make_sfn(&HeaderNode::setParameters));

	parser.setHandler("From-header", belr::make_fn(make_shared<FromHeaderNode>))
		->setCollector("Formal-name", belr::make_sfn(&FromHeaderNode::setFormalName))
		->setCollector("URI", belr::make_sfn(&FromHeaderNode::setUri));

	parser.setHandler("To-header", belr::make_fn(make_shared<ToHeaderNode>))
		->setCollector("Formal-name", belr::make_sfn(&ToHeaderNode::setFormalName))
		->setCollector("URI", belr::make_sfn(&ToHeaderNode::setUri));

	parser.setHandler("cc-header", belr::make_fn(make_shared<CcHeaderNode>))
		->setCollector("Formal-name", belr::make_sfn(&CcHeaderNode::setFormalName))
		->setCollector("URI", belr::make_sfn(&CcHeaderNode::setUri));

	parser.setHandler("DateTime-header", belr::make_fn(make_shared<DateTimeHeaderNode>))
		->setCollector("date-fullyear", belr::make_sfn(&DateTimeHeaderNode::setYear))
		->setCollector("date-month", belr::make_sfn(&DateTimeHeaderNode::setMonth))
		->setCollector("date-mday", belr::make_sfn(&DateTimeHeaderNode::setMonthDay))
		->setCollector("time-hour", belr::make_sfn(&DateTimeHeaderNode::setHour))
		->setCollector("time-minute", belr::make_sfn(&DateTimeHeaderNode::setMinute))
		->setCollector("time-second", belr::make_sfn(&DateTimeHeaderNode::setSecond))
		->setCollector("time-offset", belr::make_sfn(&DateTimeHeaderNode::setOffset));

	parser.setHandler("time-offset", belr::make_fn(make_shared<DateTimeOffsetNode>))
		->setCollector("time-sign", belr::make_sfn(&DateTimeOffsetNode::setSign))
		->setCollector("time-hour", belr::make_sfn(&DateTimeOffsetNode::setHour))
		->setCollector("time-minute", belr::make_sfn(&DateTimeOffsetNode::setMinute));

	parser.setHandler("Subject-header", belr::make_fn(make_shared<SubjectHeaderNode>))
		->setCollector("Language-tag", belr::make_sfn(&SubjectHeaderNode::setLanguage))
		->setCollector("Header-value", belr::make_sfn(&SubjectHeaderNode::setSubject));

	parser.setHandler("Ns-header", belr::make_fn(make_shared<NsHeaderNode>))
		->setCollector("Name-prefix", belr::make_sfn(&NsHeaderNode::setPrefixName))
		->setCollector("URI", belr::make_sfn(&NsHeaderNode::setUri));

	parser.setHandler("Require-header", belr::make_fn(make_shared<RequireHeaderNode>))
		->setCollector("Require-header-value", belr::make_sfn(&RequireHeaderNode::setHeaderNames));

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
	if (header.getName() == "From")
		return FromHeaderNode(header).createHeader();

	if (header.getName() == "To")
		return ToHeaderNode(header).createHeader();

	if (header.getName() == "cc")
		return CcHeaderNode(header).createHeader();

	if (header.getName() == "DateTime")
		return DateTimeHeaderNode(header).createHeader();

	if (header.getName() == "Subject")
		return SubjectHeaderNode(header).createHeader();

	if (header.getName() == "NS")
		return NsHeaderNode(header).createHeader();

	if (header.getName() == "Require")
		return RequireHeaderNode(header).createHeader();

	return HeaderNode(header).createHeader();
}

LINPHONE_END_NAMESPACE
