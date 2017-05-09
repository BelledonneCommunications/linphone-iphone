#include <stdio.h>
#include "linphone/core.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE
static void linphone_iphone_log_handler(const char *domain, OrtpLogLevel lev, const char *fmt, va_list args) {
	char* str = bctbx_strdup_vprintf(fmt, args);
    const char *levname = "undef";

	if (!domain)
		domain = "lib";

	switch (lev) {
		case ORTP_FATAL:
			levname = "Fatal";
			break;
		case ORTP_ERROR:
			levname = "Error";
			break;
		case ORTP_WARNING:
			levname = "Warning";
			break;
		case ORTP_MESSAGE:
			levname = "Message";
			break;
		case ORTP_DEBUG:
			levname = "Debug";
			break;
		case ORTP_TRACE:
			levname = "Trace";
			break;
		case ORTP_LOGLEV_END:
			return;
	}
	fprintf(stdout,"[%s] %s\n", levname, str);
}

extern "C" void linphone_iphone_enable_logs() {
	linphone_core_enable_logs_with_cb(linphone_iphone_log_handler);
}
#endif