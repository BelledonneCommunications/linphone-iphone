/*
 * cpim-grammar.cpp
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

#include "cpim-grammar.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

static const char *grammar =
// See: https://tools.ietf.org/html/rfc3862
R"==GRAMMAR==(
Message = Headers CRLF Headers CRLF [Headers CRLF]

Headers = *Header
Header = Header-name ":" Header-parameters SP Header-value CRLF

Header-name = [ Name-prefix "." ] Name
Name-prefix = Name

Header-parameters = *( ";" Parameter )

Parameter = Lang-param / Ext-param
Lang-param = "lang=" Language-tag
Ext-param = Param-name "=" Param-value
Param-name = Name
Param-value = Token / Number / String

Header-value = *HEADERCHAR

From-header = %d70.114.111.109 ": " From-header-value
From-header-value = [ Formal-name ] "<" URI ">"

To-header = %d84.111 ": " To-header-value
To-header-value = [ Formal-name ] "<" URI ">"

DateTime-header = %d68.97.116.101.84.105.109.101 ": " DateTime-header-value
DateTime-header-value = date-time

Message-ID-header = %d77.101.115.115.97.103.101.45.73.68 ": " Token

cc-header = %d99.99 ": " cc-header-value
cc-header-value = [ Formal-name ] "<" URI ">"

Subject-header = %d83.117.98.106.101.99.116 ":" Subject-header-value
Subject-header-value = [ ";" Lang-param ] SP *HEADERCHAR

NS-header = %d78.83 ": " NS-header-value
NS-header-value = [ Name-prefix SP ] "<" URI ">"

Require-header = %d82.101.113.117.105.114.101 ": " Require-header-value
Require-header-value = Header-name *( "," Header-name )

Name = 1*NAMECHAR
Token = 1*TOKENCHAR
Number = 1*DIGIT
String = DQUOTE *( Str-char / Escape ) DQUOTE
Str-char = %x20-21 / %x23-5B / %x5D-7E / UCS-high
Escape = "\" ( "u" 4(HEXDIG) / "b" / "t" / "n" / "r" / DQUOTE / "'" / "\" )

Formal-name = 1*( Token SP ) / String

HEADERCHAR = UCS-no-CTL / Escape

NAMECHAR = %x21 / %x23-27 / %x2a-2b / %x2d / %x5e-60
	/ %x7c / %x7e / ALPHA / DIGIT

TOKENCHAR = NAMECHAR / "." / UCS-high

UCS-no-CTL = UTF8-no-CTL
UCS-high = UTF8-multi
UTF8-no-CTL = %x20-7e / UTF8-multi
UTF8-multi = %xC0-DF %x80-BF
	/ %xE0-EF %x80-BF %x80-BF
	/ %xF0-F7 %x80-BF %x80-BF %x80-BF
	/ %xF8-FB %x80-BF %x80-BF %x80-BF %x80-BF
	/ %xFC-FD %x80-BF %x80-BF %x80-BF %x80-BF %x80-BF

URI = absoluteURI
)==GRAMMAR=="

// See: https://tools.ietf.org/html/rfc2396 & https://tools.ietf.org/html/rfc2732
R"==GRAMMAR==(
absoluteURI = scheme ":" ( hier-part / opaque-part )
relativeURI = ( net-path / abs-path / rel-path ) [ "?" query ]

hier-part = ( net-path / abs-path ) [ "?" query ]
opaque-part = uric-no-slash *uric

uric-no-slash = unreserved / escaped / ";" / "?" / ":" / "@" / "&" / "=" / "+" / "$" / ","

net-path = "//" authority [ abs-path ]
abs-path = "/" path-segments
rel-path = rel-segment [ abs-path ]

rel-segment = 1*( unreserved / escaped / ";" / "@" / "&" / "=" / "+" / "$" / "," )

scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

authority = server / reg-name

reg-name = 1*( unreserved / escaped / "$" / "," / ";" / ":" / "@" / "&" / "=" / "+" )

server = [ [ userinfo "@" ] hostport ]
userinfo = *( unreserved / escaped / ";" / ":" / "&" / "=" / "+" / "$" / "," )

hostport = host [ ":" port ]
host = hostname / IPv4address / IPv6address
ipv6reference = "[" IPv6address "]"
hostname = *( domainlabel "." ) toplabel [ "." ]
domainlabel = alphanum / alphanum *( alphanum / "-" ) alphanum
toplabel = ALPHA / ALPHA *( alphanum / "-" ) alphanum
IPv6address = hexpart [ ":" IPv4address ]
IPv4address = 1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT
port = *DIGIT

IPv6prefix  = hexpart "/" 1*2DIGIT
hexpart = hexseq / hexseq "::" [ hexseq ] / "::" [ hexseq ]
hexseq = hex4 *( ":" hex4)
hex4 = 1*4HEXDIG

path = [ abs-path / opaque-part ]
path-segments = segment *( "/" segment )
segment = *pchar *( ";" param )
param = *pchar
pchar = unreserved / escaped / ":" / "@" / "&" / "=" / "+" / "$" / ","

query = *uric

fragment = *uric

uric = reserved / unreserved / escaped
reserved = ";" / "/" / "?" / ":" / "@" / "&" / "=" / "+" / "$" / "," / "[" / "]"
unreserved = alphanum / mark
mark = "-" / "_" / "." / "!" / "~" / "*" / "'" / "(" / ")"

escaped = "%" HEXDIG HEXDIG

alphanum = ALPHA / DIGIT
)==GRAMMAR=="

// See: https://tools.ietf.org/html/rfc3066
R"==GRAMMAR==(
Language-Tag = Primary-subtag *( "-" Subtag )
Primary-subtag = 1*8ALPHA
Subtag = 1*8(ALPHA / DIGIT)
)==GRAMMAR=="

// See: https://tools.ietf.org/html/rfc3339
R"==GRAMMAR==(
date-fullyear = 4DIGIT
date-month = 2DIGIT
date-mday = 2DIGIT

time-hour = 2DIGIT
time-minute = 2DIGIT
time-second = 2DIGIT

time-secfrac = "." 1*DIGIT
time-numoffset = ( "+" / "-" ) time-hour ":" time-minute
time-offset = "Z" / time-numoffset

partial-time = time-hour ":" time-minute ":" time-second [ time-secfrac ]

full-date = date-fullyear "-" date-month "-" date-mday
full-time = partial-time time-offset

date-time = full-date "T" full-time
)==GRAMMAR==";

const char *Cpim::getGrammar () {
	return grammar;
}

LINPHONE_END_NAMESPACE
