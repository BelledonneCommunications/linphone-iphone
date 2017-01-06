/*
error_info.c
Copyright (C) 2016  Belledonne Communications SARL

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

#include "linphone/core.h"
#include "private.h"


const char *linphone_reason_to_string(LinphoneReason err){
	switch(err) {
		case LinphoneReasonNone:
			return "No error";
		case LinphoneReasonNoResponse:
			return "No response";
		case LinphoneReasonForbidden:
			return "Bad credentials";
		case LinphoneReasonDeclined:
			return "Call declined";
		case LinphoneReasonNotFound:
			return "User not found";
		case LinphoneReasonNotAnswered:
			return "Not answered";
		case LinphoneReasonBusy:
			return "Busy";
		case LinphoneReasonMedia:
			return "Incompatible media capabilities";
		case LinphoneReasonIOError:
			return "IO error";
		case LinphoneReasonDoNotDisturb:
			return "Do not disturb";
		case LinphoneReasonUnauthorized:
			return "Unauthorized";
		case LinphoneReasonNotAcceptable:
			return "Not acceptable here";
		case LinphoneReasonNoMatch:
			return "No match";
		case LinphoneReasonMovedPermanently:
			return "Moved permanently";
		case LinphoneReasonGone:
			return "Gone";
		case LinphoneReasonTemporarilyUnavailable:
			return "Temporarily unavailable";
		case LinphoneReasonAddressIncomplete:
			return "Address incomplete";
		case LinphoneReasonNotImplemented:
			return "Not implemented";
		case LinphoneReasonBadGateway:
			return "Bad gateway";
		case LinphoneReasonServerTimeout:
			return "Server timeout";
		case LinphoneReasonUnknown:
			return "Unknown error";
	}
	return "unknown error";
}

typedef struct _error_code_reason_map {
	int error_code;
	LinphoneReason reason;
} error_code_reason_map_t;

static const error_code_reason_map_t error_code_reason_map[] = {
	{ 200, LinphoneReasonNone },
	{ 301, LinphoneReasonMovedPermanently },
	{ 400, LinphoneReasonUnknown },
	{ 401, LinphoneReasonUnauthorized },
	{ 403, LinphoneReasonForbidden },
	{ 404, LinphoneReasonNotFound },
	{ 410, LinphoneReasonGone },
	{ 415, LinphoneReasonUnsupportedContent },
	{ 480, LinphoneReasonTemporarilyUnavailable },
	{ 481, LinphoneReasonNoMatch },
	{ 484, LinphoneReasonAddressIncomplete },
	{ 486, LinphoneReasonBusy },
	{ 488, LinphoneReasonNotAcceptable },
	{ 501, LinphoneReasonNotImplemented },
	{ 502, LinphoneReasonBadGateway },
	{ 503, LinphoneReasonIOError },
	{ 504, LinphoneReasonServerTimeout },
	{ 600, LinphoneReasonDoNotDisturb },
	{ 603, LinphoneReasonDeclined }
};

LinphoneReason linphone_error_code_to_reason(int err) {
	size_t i;
	for (i = 0; i < (sizeof(error_code_reason_map) / sizeof(error_code_reason_map[0])); i++) {
		if (error_code_reason_map[i].error_code == err) return error_code_reason_map[i].reason;
	}
	return LinphoneReasonUnknown;
}

int linphone_reason_to_error_code(LinphoneReason reason) {
	size_t i;
	for (i = 0; i < (sizeof(error_code_reason_map) / sizeof(error_code_reason_map[0])); i++) {
		if (error_code_reason_map[i].reason == reason) return error_code_reason_map[i].error_code;
	}
	return 400;
}


LinphoneReason linphone_error_info_get_reason(const LinphoneErrorInfo *ei) {
	const SalErrorInfo *sei = (const SalErrorInfo *)ei;
	return linphone_reason_from_sal(sei->reason);
}

const char *linphone_error_info_get_phrase(const LinphoneErrorInfo *ei) {
	const SalErrorInfo *sei = (const SalErrorInfo *)ei;
	return sei->status_string;
}

const char *linphone_error_info_get_details(const LinphoneErrorInfo *ei) {
	const SalErrorInfo *sei = (const SalErrorInfo *)ei;
	return sei->warnings;
}

int linphone_error_info_get_protocol_code(const LinphoneErrorInfo *ei) {
	const SalErrorInfo *sei = (const SalErrorInfo *)ei;
	return sei->protocol_code;
}
