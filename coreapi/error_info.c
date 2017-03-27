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

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneErrorInfo);

#define STRING_RESET(field)	if (field) ms_free(field); field = NULL;



static void error_info_destroy(LinphoneErrorInfo *ei){
	if (ei->protocol) ms_free(ei->protocol);
	if (ei->phrase) ms_free(ei->phrase);
	if (ei->warnings) ms_free(ei->phrase);
	if (ei->full_string) ms_free(ei->full_string);
}

static void error_info_clone(LinphoneErrorInfo *ei, const LinphoneErrorInfo *other){
	ei->protocol = ms_strdup_safe(other->protocol);
	ei->phrase = ms_strdup_safe(other->phrase);
	ei->warnings = ms_strdup_safe(other->phrase);
	ei->full_string = ms_strdup_safe(other->full_string);
}

BELLE_SIP_INSTANCIATE_VPTR(LinphoneErrorInfo, belle_sip_object_t,
	error_info_destroy, // destroy
	error_info_clone, // clone
	NULL, // Marshall
	FALSE
);

LinphoneErrorInfo *linphone_error_info_new(void){
	LinphoneErrorInfo *ei = belle_sip_object_new(LinphoneErrorInfo);
	return ei;
}

LinphoneErrorInfo* linphone_error_info_ref ( LinphoneErrorInfo* ei ) {
	return (LinphoneErrorInfo*) belle_sip_object_ref(ei);
}

void linphone_error_info_unref ( LinphoneErrorInfo* ei ) {
	belle_sip_object_unref(ei);
}

void linphone_error_info_set_reason ( LinphoneErrorInfo* ei, LinphoneReason reason ) {
	ei->reason = reason;
}

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

void linphone_error_info_reset(LinphoneErrorInfo *ei){
	ei->reason = LinphoneReasonNone;
	STRING_RESET(ei->protocol);
	STRING_RESET(ei->phrase);
	STRING_RESET(ei->full_string);
	STRING_RESET(ei->warnings);
	ei->protocol_code = 0;
	if (ei->sub_ei) {
		linphone_error_info_unref(ei->sub_ei);
		ei->sub_ei = NULL;
	}
}

void linphone_error_info_from_sal(LinphoneErrorInfo *ei, const SalErrorInfo *sei){
	ei->reason = linphone_reason_from_sal(sei->reason);
	ei->phrase = ms_strdup_safe(sei->status_string);
	ei->full_string = ms_strdup_safe(sei->full_string);
	ei->warnings = ms_strdup_safe(sei->warnings);
	ei->protocol_code = sei->protocol_code;
	ei->protocol = ms_strdup_safe(sei->protocol);
}

/* If a reason header is provided (in reason_ei), then create a sub LinphoneErrorInfo attached to the first one, unless the reason header
 is in the request, in which case no primary error is given.*/
void linphone_error_info_from_sal_reason_ei(LinphoneErrorInfo *ei, const SalErrorInfo *reason_ei){
	if (ei->reason == LinphoneReasonNone){
		/*no primary error given*/
		linphone_error_info_reset(ei);
		linphone_error_info_from_sal(ei, reason_ei);
		return;
	}
	
	if (ei->sub_ei){
		if (reason_ei->reason == SalReasonNone){
			linphone_error_info_unref(ei->sub_ei);
			ei->sub_ei = NULL;
		}
	}else{
		if (reason_ei->reason != SalReasonNone){
			ei->sub_ei = linphone_error_info_new();
		}
	}
	if (reason_ei->reason != SalReasonNone){
		linphone_error_info_from_sal(ei->sub_ei, reason_ei);
	}
}

void linphone_error_info_from_sal_op(LinphoneErrorInfo *ei, const SalOp *op){
	if (op==NULL) {
		/*leave previous values in LinphoneErrorInfo, the op may have been released already.*/
		return;
	}else{
		const SalErrorInfo *sei;
		linphone_error_info_reset(ei);
		sei = sal_op_get_error_info(op);
		linphone_error_info_from_sal(ei, sei);
		sei = sal_op_get_reason_error_info(op);
		linphone_error_info_from_sal_reason_ei(ei, sei);
	}
}

void linphone_error_info_set(LinphoneErrorInfo *ei, const char *protocol, LinphoneReason reason, int code, const char *status_string, const char *warning){
	linphone_error_info_reset(ei);
	ei->reason = reason;
	ei->protocol_code = code;
	if (protocol != NULL){
		ei->protocol = bctbx_strdup(protocol);
	}
	else{
		const char* prot = "SIP";
		ei->protocol = bctbx_strdup(prot);
	}

	ei->phrase = bctbx_strdup(status_string);
	ei->warnings = bctbx_strdup(warning);
}


LinphoneReason linphone_error_info_get_reason(const LinphoneErrorInfo *ei) {
	return ei->reason;
}

const char *linphone_error_info_get_protocol(const LinphoneErrorInfo *ei){
	return ei->protocol;
}

const char *linphone_error_info_get_phrase(const LinphoneErrorInfo *ei) {
	return ei->phrase;
}

/*deprecated, kept for binary compatibility*/
const char *linphone_error_info_get_details(const LinphoneErrorInfo *ei){
	return linphone_error_info_get_warnings(ei);
}

const char *linphone_error_info_get_warnings(const LinphoneErrorInfo *ei) {
	return ei->warnings;
}

int linphone_error_info_get_protocol_code(const LinphoneErrorInfo *ei) {
	return ei->protocol_code;
}

const LinphoneErrorInfo * linphone_error_info_get_sub_error_info(const LinphoneErrorInfo *ei){
	return ei->sub_ei;
}
