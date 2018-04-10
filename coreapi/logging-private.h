/*
logging-private.h
Copyright (C) 2017  Belledonne Communications SARL

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

#ifndef _LOGGING_PRIVATE_H_
#define _LOGGING_PRIVATE_H_

#include <bctoolbox/logging.h>
#include "linphone/logging.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts a #BctbxLogLevel into #LinphoneLogLevel.
 */
LinphoneLogLevel _bctbx_log_level_to_linphone_log_level(BctbxLogLevel level);

/**
 * @brief Converts a mask of #BctbxLogLevel into a mask of #LinphoneLogLevel.
 */
unsigned int _bctbx_log_mask_to_linphone_log_mask(unsigned int mask);

/**
 * @brief Converts a #LinphoneLogLevel into #BctbxLogLevel.
 */
BctbxLogLevel _linphone_log_level_to_bctbx_log_level(LinphoneLogLevel level);

/**
 * @brief Converts a mask of #LinphoneLogLevel into a mask of #BctbxLogLevel.
 */
unsigned int _linphone_log_mask_to_bctbx_log_mask(unsigned int mask);

/**
 * @brief Releases the instance pointer of the singleton.
 * @note You should not need to call this function since it is automatically done
 * at process ending.
 */
void _linphone_logging_service_clean(void);

#ifdef __cplusplus
}
#endif

#endif // _LOGGING_PRIVATE_H_
