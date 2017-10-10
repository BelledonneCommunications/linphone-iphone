/*
logging.h
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

#ifndef _LINPHONE_LOG_H_
#define _LINPHONE_LOG_H_

#include "types.h"
#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup logging
 * @{
 */

/**
 * @brief Singleton class giving access to logging features.
 */
typedef struct _LinphoneLoggingService LinphoneLoggingService;

/**
 * @brief Listener for #LinphoneLoggingService.
 */
typedef struct _LinphoneLoggingServiceCbs LinphoneLoggingServiceCbs;

/**
 * @brief Verbosity levels of log messages.
 */
typedef enum _LinphoneLogLevel {
	LinphoneLogLevelDebug   = 1,    /**< @brief Level for debug messages. */
	LinphoneLogLevelTrace   = 1<<1, /**< @brief Level for traces. */
	LinphoneLogLevelMessage = 1<<2, /**< @brief Level for information messages. */
	LinphoneLogLevelWarning = 1<<3, /**< @brief Level for warning messages. */
	LinphoneLogLevelError   = 1<<4, /**< @brief Level for error messages. */
	LinphoneLogLevelFatal   = 1<<5  /**< @brief Level for fatal error messages. */
} LinphoneLogLevel;

/**
 * @brief Type of callbacks called each time liblinphone write a log message.
 * 
 * @param log_service A pointer on the logging service singleton.
 * @param domain A string describing which sub-library of liblinphone the message is coming from.
 * @param lev Verbosity level of the message.
 * @param message Content of the message.
 */
typedef void (*LinphoneLoggingServiceCbsLogMessageWrittenCb)(LinphoneLoggingService *log_service, const char *domain, LinphoneLogLevel lev, const char *message);





/**
 * @brief Gets the singleton logging service object.
 * 
 * The singleton is automatically instantiated if it hasn't
 * been done yet.
 * 
 * @return A pointer on the singleton.
 */
LINPHONE_PUBLIC LinphoneLoggingService *linphone_logging_service_get(void);

/**
 * @brief Increases the reference counter.
 */
LINPHONE_PUBLIC LinphoneLoggingService *linphone_logging_service_ref(LinphoneLoggingService *service);

/**
 * @brief Decreases the reference counter and destroy the object
 * if the counter reaches 0.
 */
LINPHONE_PUBLIC void linphone_logging_service_unref(LinphoneLoggingService *service);

/**
 * @brief Gets the logging service listener.
 */
LINPHONE_PUBLIC LinphoneLoggingServiceCbs *linphone_logging_service_get_callbacks(const LinphoneLoggingService *log_service);

/**
 * @brief Set the verbosity of the log.
 * 
 * For instance, a level of #LinphoneLogLevelMessage will let pass fatal, error, warning and message-typed messages
 * whereas trace and debug messages will be dumped out.
 */
LINPHONE_PUBLIC void linphone_logging_service_set_log_level(LinphoneLoggingService *log_service, LinphoneLogLevel level);

/**
 * @brief Sets the types of messages that will be authorized to be written in the log.
 * @param log_service The logging service singleton.
 * @param mask Example: #LinphoneLogLevelMessage|#LinphoneLogLevelError will ONLY let pass message-typed and error messages.
 * @note Calling that function reset the log level that has been specified by #linphone_logging_service_set_log_level().
 */
LINPHONE_PUBLIC void linphone_logging_service_set_log_level_mask(LinphoneLoggingService *log_service, unsigned int mask);

/**
 * @brief Gets the log level mask.
 */
LINPHONE_PUBLIC unsigned int linphone_logging_service_get_log_level_mask(const LinphoneLoggingService *log_service);

/**
 * @brief Enables logging in a file.
 * 
 * That function enables an internal log handler that writes log messages in
 * log-rotated files.
 * 
 * @param dir Directory where to create the distinct parts of the log.
 * @param filename Name of the log file.
 * @param max_size The maximal size of each part of the log. The log rotating is triggered
 * each time the currently opened log part reach that limit.
 */
LINPHONE_PUBLIC void linphone_logging_service_set_log_file(const LinphoneLoggingService *service, const char *dir, const char *filename, size_t max_size);





/**
 * @brief Increases the reference counter.
 */
LINPHONE_PUBLIC LinphoneLoggingServiceCbs *linphone_logging_service_cbs_ref(LinphoneLoggingServiceCbs *cbs);

/**
 * @brief Decreases the reference counter.
 * 
 * The object is automatically destroyed once the counter reach 0.
 */
LINPHONE_PUBLIC void linphone_logging_service_cbs_unref(LinphoneLoggingServiceCbs *cbs);

/**
 * @brief Sets the callback to call each time liblinphone writes a log message.
 */
LINPHONE_PUBLIC void linphone_logging_service_cbs_set_log_message_written(LinphoneLoggingServiceCbs *cbs, LinphoneLoggingServiceCbsLogMessageWrittenCb cb);

/**
 * @brief Gets the value of the message event callback.
 */
LINPHONE_PUBLIC LinphoneLoggingServiceCbsLogMessageWrittenCb linphone_logging_service_cbs_get_log_message_written(const LinphoneLoggingServiceCbs *cbs);

/**
 * @brief Pass a pointer on a custom object.
 * 
 * That pointer can be get back by callbacks by using #linphone_logging_service_get_cbs() and #linphone_logging_service_cbs_get_user_data().
 */
LINPHONE_PUBLIC void linphone_logging_service_cbs_set_user_data(LinphoneLoggingServiceCbs *cbs, void *user_data);

/**
 * @brief Gets the user_data pointer back.
 */
LINPHONE_PUBLIC void *linphone_logging_service_cbs_get_user_data(const LinphoneLoggingServiceCbs *cbs);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // _LINPHONE_LOG_H_
