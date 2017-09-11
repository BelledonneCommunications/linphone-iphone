/*
 * cpim-grammar.cpp
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

#include "cpim-grammar.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

static const char *grammar =
// See: https://tools.ietf.org/html/rfc3862
R"==GRAMMAR==(
Message = Headers CRLF Headers CRLF

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
)==GRAMMAR=="

// See: https://tools.ietf.org/html/rfc2396
R"==GRAMMAR==(
URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

hier-part = "//" authority path-abempty
	/ path-absolute
	/ path-rootless
	/ path-empty

URI-reference = URI / relative-ref

absolute-URI = scheme ":" hier-part [ "?" query ]

relative-ref = relative-part [ "?" query ] [ "#" fragment ]

relative-part = "//" authority path-abempty
	/ path-absolute
	/ path-noscheme
	/ path-empty

scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

authority = [ userinfo "@" ] host [ ":" port ]
userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
host = IP-literal / IPv4address / reg-name
port = *DIGIT

IP-literal = "[" ( IPv6address / IPvFuture  ) "]"

IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )

IPv6address = 6( h16 ":" ) ls32
	/ "::" 5( h16 ":" ) ls32
	/ [ h16 ] "::" 4( h16 ":" ) ls32
	/ [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
	/ [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
	/ [ *3( h16 ":" ) h16 ] "::" h16 ":" ls32
	/ [ *4( h16 ":" ) h16 ] "::" ls32
	/ [ *5( h16 ":" ) h16 ] "::" h16
	/ [ *6( h16 ":" ) h16 ] "::"

h16 = 1*4HEXDIG
ls32 = ( h16 ":" h16 ) / IPv4address
IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
dec-octet = DIGIT
	/ %x31-39 DIGIT
	/ "1" 2DIGIT
	/ "2" %x30-34 DIGIT
	/ "25" %x30-35

reg-name = *( unreserved / pct-encoded / sub-delims )

path = path-abempty
	/ path-absolute
	/ path-noscheme
	/ path-rootless
	/ path-empty

path-abempty = *( "/" segment )
path-absolute = "/" [ segment-nz *( "/" segment ) ]
path-noscheme = segment-nz-nc *( "/" segment )
path-rootless = segment-nz *( "/" segment )
path-empty = [pchar]

segment = *pchar
segment-nz = 1*pchar
segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )

pchar = unreserved / pct-encoded / sub-delims / ":" / "@" / "\,"

query = *( pchar / "/" / "?" )

fragment = *( pchar / "/" / "?" )

pct-encoded = "%" HEXDIG HEXDIG

unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
reserved = gen-delims / sub-delims
gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
sub-delims = "!" / "$" / "&" / "'" / "(" / ")"
	/ "*" / "+" / "," / ";" / "="
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
