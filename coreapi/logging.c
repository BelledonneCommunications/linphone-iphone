/*
log.c
Copyright (C) 2017 Belledonne Communications SARL

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

#include <map>
#include <algorithm>
#include <mediastreamer2/mscommon.h>
#include <belle-sip/object.h>
#include <bctoolbox/logging.h>
#include "linphone/logging.h"
#include "private.h"
#include "logging-private.h"


struct _LinphoneLoggingService {
	belle_sip_object_t base;
	LinphoneLoggingServiceCbs *cbs;
	bctbx_log_handler_t *log_handler;
};

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneLoggingService);
BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneLoggingService);

struct _LinphoneLoggingServiceCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneLoggingServiceCbsLogMessageWrittenCb message_event_cb;
};

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneLoggingServiceCbs);
BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneLoggingServiceCbs);

static LinphoneLoggingServiceCbs *_linphone_logging_service_cbs_new(void);





static LinphoneLoggingService *_linphone_logging_service_instance = NULL;

static std::map<LinphoneLogLevel, BctbxLogLevel> _linphone_log_level_to_bctbx_log_level_map = {
	{ LinphoneLogLevelDebug   , BCTBX_LOG_DEBUG   },
	{ LinphoneLogLevelTrace   , BCTBX_LOG_TRACE   },
	{ LinphoneLogLevelMessage , BCTBX_LOG_MESSAGE },
	{ LinphoneLogLevelWarning , BCTBX_LOG_WARNING },
	{ LinphoneLogLevelError   , BCTBX_LOG_ERROR   },
	{ LinphoneLogLevelFatal   , BCTBX_LOG_FATAL   }
};

LinphoneLogLevel _bctbx_log_level_to_linphone_log_level(BctbxLogLevel level) {
	auto &tmap = _linphone_log_level_to_bctbx_log_level_map;
	auto predicate = [level](const std::pair<LinphoneLogLevel, BctbxLogLevel> &tuple)->bool{return tuple.second==level;};
	auto response = std::find_if(tmap.cbegin(), tmap.cend(), predicate);
	if (response != tmap.cend()) {
		return response->first;
	} else {
		ms_fatal("%s(): invalid argument [%d]", __FUNCTION__, level);
		return LinphoneLogLevelDebug;
	}
}

unsigned int _bctbx_log_mask_to_linphone_log_mask(unsigned int mask) {
	unsigned int res = 0;
	auto &tmap = _linphone_log_level_to_bctbx_log_level_map;
	for (auto it=tmap.cbegin(); it!=tmap.cend(); it++) {
		if (mask & it->second) {
			mask&=~it->second;
			res|=it->first;
		}
	}
	if (mask != 0) {
		ms_warning("%s(): invalid flag set in mask [%x]", __FUNCTION__, mask);
	}
	return res;
}

BctbxLogLevel _linphone_log_level_to_bctbx_log_level(LinphoneLogLevel level) {
	try {
		return _linphone_log_level_to_bctbx_log_level_map.at(level);
	} catch (const std::out_of_range &) {
		ms_fatal("%s(): invalid argument [%d]", __FUNCTION__, level);
		return BCTBX_LOG_LOGLEV_END;
	}
}

unsigned int _linphone_log_mask_to_bctbx_log_mask(unsigned int mask) {
	unsigned int res = 0;
	auto &tmap = _linphone_log_level_to_bctbx_log_level_map;
	for (auto it=tmap.cbegin(); it!=tmap.cend(); it++) {
		if (mask & it->first) {
			mask&=~it->first;
			res|=it->second;
		}
	}
	if (mask != 0) {
		ms_fatal("%s(): invalid flag set in mask [%x]", __FUNCTION__, mask);
	}
	return res;
}

static void _log_handler_on_message_written_cb(void *info,const char *domain, BctbxLogLevel lev, const char *fmt, va_list args) {
	LinphoneLoggingService *service = (LinphoneLoggingService *)info;
	if (service->cbs->message_event_cb) {
		char *message = bctbx_strdup_vprintf(fmt, args);
		service->cbs->message_event_cb(service, domain, _bctbx_log_level_to_linphone_log_level(lev), message);
		bctbx_free(message);
	}
}

static void _log_handler_destroy_cb(bctbx_log_handler_t *handler) {
	LinphoneLoggingService *service = (LinphoneLoggingService *)bctbx_log_handler_get_user_data(handler);
	service->log_handler = NULL;
}

static LinphoneLoggingService *_linphone_logging_service_new(void) {
	LinphoneLoggingService *service = belle_sip_object_new(LinphoneLoggingService);
	service->log_handler = bctbx_create_log_handler(_log_handler_on_message_written_cb, _log_handler_destroy_cb, service);
	service->cbs = _linphone_logging_service_cbs_new();
	bctbx_add_log_handler(service->log_handler);

	return service;
}

LinphoneLoggingService *linphone_logging_service_get(void) {
	if (_linphone_logging_service_instance == NULL) {
		_linphone_logging_service_instance = _linphone_logging_service_new();
		atexit(_linphone_logging_service_clean);
	}
	return _linphone_logging_service_instance;
}

void _linphone_logging_service_clean(void) {
	if (_linphone_logging_service_instance) {
		linphone_logging_service_unref(_linphone_logging_service_instance);
		_linphone_logging_service_instance = NULL;
	}
}

LinphoneLoggingService *linphone_logging_service_ref(LinphoneLoggingService *service) {
	return (LinphoneLoggingService *)belle_sip_object_ref(service);
}

void linphone_logging_service_unref(LinphoneLoggingService *service) {
	belle_sip_object_ref(service);
}

static void _linphone_logging_service_uninit(LinphoneLoggingService *log_service) {
	if (log_service->log_handler) bctbx_remove_log_handler(log_service->log_handler);
	linphone_logging_service_cbs_unref(log_service->cbs);
}

void linphone_logging_service_release_instance(void) {
	if (_linphone_logging_service_instance) {
		belle_sip_object_unref(BELLE_SIP_OBJECT(_linphone_logging_service_instance));
	}
	_linphone_logging_service_instance = NULL;
}

LinphoneLoggingServiceCbs *linphone_logging_service_get_callbacks(const LinphoneLoggingService *log_service) {
	return log_service->cbs;
}

static const char *_linphone_logging_service_log_domains[] = {
	"bctbx",
	"ortp",
	"bzrtp",
	"mediastreamer",
	BELLE_SIP_LOG_DOMAIN,
	BCTBX_LOG_DOMAIN,
	NULL
};

void linphone_logging_service_set_log_level(LinphoneLoggingService *log_service, LinphoneLogLevel loglevel) {
	const char **domain;
	for (domain=_linphone_logging_service_log_domains; *domain; domain++) {
		bctbx_set_log_level(*domain, _linphone_log_level_to_bctbx_log_level(loglevel));
	}
}

void linphone_logging_service_set_log_level_mask(LinphoneLoggingService *log_service, unsigned int mask) {
	const char **domain;
	for (domain=_linphone_logging_service_log_domains; *domain; domain++) {
		bctbx_set_log_level_mask(*domain, _linphone_log_mask_to_bctbx_log_mask(mask));
	}
}

unsigned int linphone_logging_service_get_log_level_mask(const LinphoneLoggingService *log_service) {
	return _bctbx_log_mask_to_linphone_log_mask(bctbx_get_log_level_mask(BCTBX_LOG_DOMAIN));
}

void linphone_logging_service_set_log_file(const LinphoneLoggingService *service, const char *dir, const char *filename, size_t max_size) {
	bctbx_log_handler_t *log_handler = bctbx_create_file_log_handler((uint64_t)max_size, dir, filename, NULL);
	bctbx_add_log_handler(log_handler);
}

BELLE_SIP_INSTANCIATE_VPTR(LinphoneLoggingService, belle_sip_object_t,
	_linphone_logging_service_uninit, // uninit
	NULL,                         // clone
	NULL,                         // marshal
	FALSE                         // unown
);









static LinphoneLoggingServiceCbs *_linphone_logging_service_cbs_new(void) {
	return belle_sip_object_new(LinphoneLoggingServiceCbs);
}

LinphoneLoggingServiceCbs *linphone_logging_service_cbs_ref(LinphoneLoggingServiceCbs *cbs) {
	return (LinphoneLoggingServiceCbs *)belle_sip_object_ref(cbs);
}

void linphone_logging_service_cbs_unref(LinphoneLoggingServiceCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void linphone_logging_service_cbs_set_log_message_written(LinphoneLoggingServiceCbs *cbs, LinphoneLoggingServiceCbsLogMessageWrittenCb cb) {
	
	/* We need to set the legacy log handler to NULL here
	 because LinphoneCore have a default log handler that dump
	 all messages into the standard output. */
	/*this function is moved here to make sure default log handler is only removed when user defined logging cbs is set*/
	_linphone_core_set_log_handler(NULL);
	cbs->message_event_cb = cb;
}

LinphoneLoggingServiceCbsLogMessageWrittenCb linphone_logging_service_cbs_get_log_message_written(const LinphoneLoggingServiceCbs *cbs) {
	return cbs->message_event_cb;
}

void linphone_logging_service_cbs_set_user_data(LinphoneLoggingServiceCbs *cbs, void *user_data) {
	cbs->user_data = user_data;
}

BELLE_SIP_INSTANCIATE_VPTR(LinphoneLoggingServiceCbs, belle_sip_object_t,
	NULL, // uninit
	NULL, // clone
	NULL, // marshal
	FALSE // unown
);
