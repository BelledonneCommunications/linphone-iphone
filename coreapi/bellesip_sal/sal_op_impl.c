/*
linphone
Copyright (C) 2012  Belledonne Communications, Grenoble, France

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
#include "sal_impl.h"
#include "sal/sal.h"

SalReason _sal_reason_from_sip_code(int code) {
	if (code>=100 && code<300) return SalReasonNone;

	switch(code) {
	case 0:
		return SalReasonIOError;
	case 301:
		return SalReasonMovedPermanently;
	case 302:
		return SalReasonRedirect;
	case 401:
	case 407:
		return SalReasonUnauthorized;
	case 403:
		return SalReasonForbidden;
	case 404:
		return SalReasonNotFound;
	case 408:
		return SalReasonRequestTimeout;
	case 410:
		return SalReasonGone;
	case 415:
		return SalReasonUnsupportedContent;
	case 422:
		ms_error ("422 not implemented yet");;
		break;
	case 480:
		return SalReasonTemporarilyUnavailable;
	case 481:
		return SalReasonNoMatch;
	case 484:
		return SalReasonAddressIncomplete;
	case 486:
		return SalReasonBusy;
	case 487:
		return SalReasonNone;
	case 488:
		return SalReasonNotAcceptable;
	case 491:
		return SalReasonRequestPending;
	case 500:
		return SalReasonInternalError;
	case 501:
		return SalReasonNotImplemented;
	case 502:
		return SalReasonBadGateway;
	case 503:
		return SalReasonServiceUnavailable;
	case 504:
		return SalReasonServerTimeout;
	case 600:
		return SalReasonDoNotDisturb;
	case 603:
		return SalReasonDeclined;
	default:
		return SalReasonUnknown;
	}
	return SalReasonUnknown;
}

const SalErrorInfo *sal_error_info_none(void){
	static const SalErrorInfo none = {
		SalReasonNone,
		(char *)"Ok",
		200,
		NULL,
		NULL
	};
	return &none;
}

void sal_error_info_reset(SalErrorInfo *ei){
	if (ei->status_string){
		ms_free(ei->status_string);
		ei->status_string=NULL;
	}
	if (ei->warnings){
		ms_free(ei->warnings);
		ei->warnings=NULL;

	}
	if (ei->full_string){
		ms_free(ei->full_string);
		ei->full_string=NULL;
	}
	if (ei->protocol){
		ms_free(ei->protocol);
		ei->protocol = NULL;
	}
	ei->protocol_code=0;
	ei->reason=SalReasonNone;
	ei->sub_sei = NULL;
}

void sal_error_info_set(SalErrorInfo *ei, SalReason reason, const char *protocol, int code, const char *status_string, const char *warning){
	sal_error_info_reset(ei);
	if (reason==SalReasonUnknown && strcmp(protocol, "SIP") == 0 && code != 0) ei->reason=_sal_reason_from_sip_code(code);
	else{
		ei->reason=reason;
		if (code == 0) {
			code = LinphonePrivate::to_sip_code(reason);
		}
	}
	ei->protocol_code=code;
	ei->status_string=status_string ? ms_strdup(status_string) : NULL;
	ei->warnings=warning ? ms_strdup(warning) : NULL;
	ei->protocol = protocol ? ms_strdup(protocol) : NULL;
	if (ei->status_string){
		if (ei->warnings)
			ei->full_string=ms_strdup_printf("%s %s",ei->status_string,ei->warnings);
		else ei->full_string=ms_strdup(ei->status_string);
	}
}
