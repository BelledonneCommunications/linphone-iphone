/*
linphone, gtk-glade interface.
Copyright (C) 2015  Belledonne Communications <info@belledonne-communications.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 * Regex matching with any URI that respects the RFC 3986
 */
#define BC_REGEX_URI_PCT_ENCODED      "(%[[:xdigit:]]{2})"
#define BC_REGEX_URI_SUB_DELIMS       "[!$&'()*+,;=]"
#define BC_REGEX_URI_UNRESERVED       "[[:alnum:]\\-._~]"
#define BC_REGEX_URI_PCHAR            "(" BC_REGEX_URI_UNRESERVED "|" BC_REGEX_URI_PCT_ENCODED "|" BC_REGEX_URI_SUB_DELIMS "|" "[:@]" ")"
#define BC_REGEX_URI_SCHEME           "(" "[[:alpha:]][[:alnum:]+\\-.]*" ")"
#define BC_REGEX_URI_USERINFO         "(" "(" BC_REGEX_URI_UNRESERVED "|" BC_REGEX_URI_PCT_ENCODED "|" BC_REGEX_URI_SUB_DELIMS "|" ":" ")*" ")"
#define BC_REGEX_URI_HOST             "(" "(" BC_REGEX_URI_UNRESERVED "|" BC_REGEX_URI_PCT_ENCODED "|" BC_REGEX_URI_SUB_DELIMS ")*" ")"
#define BC_REGEX_URI_PORT             "(" "[\\d]*" ")"
#define BC_REGEX_URI_AUTHORITY        "(" "(" BC_REGEX_URI_USERINFO "@" ")?" BC_REGEX_URI_HOST "(" ":" BC_REGEX_URI_PORT ")?" ")"
#define BC_REGEX_URI_SEGMENT          "(" BC_REGEX_URI_PCHAR "*" ")"
#define BC_REGEX_URI_SEGMENT_NZ       "(" BC_REGEX_URI_PCHAR "+" ")"
#define BC_REGEX_URI_PATH_ABEMPTY     "(" "(" "/" BC_REGEX_URI_SEGMENT ")*" ")"
#define BC_REGEX_URI_PATH_ABSOLUTE    "(" "/" "(" BC_REGEX_URI_SEGMENT_NZ "(" "/" BC_REGEX_URI_SEGMENT ")*" ")?" ")"
#define BC_REGEX_URI_PATH_ROOTLESS    "(" BC_REGEX_URI_SEGMENT_NZ "(" "/" BC_REGEX_URI_SEGMENT ")*" ")"
#define BC_REGEX_URI_HIER_PART        "(" "//" BC_REGEX_URI_AUTHORITY BC_REGEX_URI_PATH_ABEMPTY "|" BC_REGEX_URI_PATH_ABSOLUTE "|" BC_REGEX_URI_PATH_ROOTLESS ")"
#define BC_REGEX_URI_QUERY            "(" "(" BC_REGEX_URI_PCHAR "|" "[/?]" ")*" ")"
#define BC_REGEX_URI_FRAGMENT         "(" "(" BC_REGEX_URI_PCHAR "|" "[/?]" ")*" ")"
#define BC_REGEX_URI                  "(" BC_REGEX_URI_SCHEME ":" BC_REGEX_URI_HIER_PART "(" "\\?" BC_REGEX_URI_QUERY ")?" "(" "#" BC_REGEX_URI_FRAGMENT ")?" ")"

/*
 * Regex matching with any domain name (RFC 1034)
 */
#define BC_REGEX_DOMAIN_LDH           "[[:alnum:]-]"
#define BC_REGEX_DOMAIN_LABEL         "(" "[[:alpha:]]" "(" BC_REGEX_DOMAIN_LDH "*" "[[:alnum:]]" ")?" ")"
#define BC_REGEX_DOMAIN               "(" BC_REGEX_DOMAIN_LABEL "(" "\\." BC_REGEX_DOMAIN_LABEL ")*" ")"

/*
 * Regex matching with email addresses (RFC 5322) 
 */
#define BC_REGEX_EMAIL_ATEXT          "[[:alnum:]!#$%&'*+\\-/=?\\^_`{}|~]"
#define BC_REGEX_EMAIL_DOT_ATOM_TEXT  "(" BC_REGEX_EMAIL_ATEXT "+" "(" "." BC_REGEX_EMAIL_ATEXT "+" ")*" ")"
#define BC_REGEX_EMAIL_LOCAL_PART     BC_REGEX_EMAIL_DOT_ATOM_TEXT
#define BC_REGEX_EMAIL_DTEXT_NO_OBS   "[!-Z\\^-~]"
#define BC_REGEX_EMAIL_DOMAIN         "(" BC_REGEX_EMAIL_DOT_ATOM_TEXT "|" "\\[" BC_REGEX_EMAIL_DTEXT_NO_OBS "*" "\\]" ")"
#define BC_REGEX_EMAIL_ADDR_SPEC      "(" BC_REGEX_EMAIL_LOCAL_PART "@" BC_REGEX_EMAIL_DOMAIN ")"

/*
 * Regex matching with email addresses but with more constraints than RFC 5322.
 * The additionnal constraints are the folowings:
 *  + the domain part is a domain name as describe in RFC 1034
 *  + the domain part must have two label at least
 *  + the last label of the domain part must have two letter (without digit and hyphen) at least.
 */
#define BC_REGEX_RESTRICTIVE_EMAIL_TLD   "(" "[[:alpha:]]" BC_REGEX_DOMAIN_LDH "*" "[[:alnum:]]" ")"
#define BC_REGEX_RESTRICTIVE_EMAIL_ADDR  "(" BC_REGEX_EMAIL_LOCAL_PART "@" "(" BC_REGEX_DOMAIN_LABEL "\\." ")+" BC_REGEX_RESTRICTIVE_EMAIL_TLD ")"
