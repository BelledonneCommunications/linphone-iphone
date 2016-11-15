/*******************************************************************************
 *            conference.h
 *
 *  Thu Nov 26, 2015
 *  Copyright  2015  Belledonne Communications
 *  Author: Linphone's team
 *  Email info@belledonne-communications.com
 ******************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef CONFERENCE_H
#define CONFERENCE_H
	
#include "linphonecore.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup call_control
 * @{
 */

/**
 * LinphoneConference class
 * The _LinphoneConference struct does not exists, it's the Conference C++ class that is used behind
 */
typedef struct _LinphoneConference LinphoneConference;

/**
 * Parameters for initialization of conferences
 * The _LinphoneConferenceParams struct does not exists, it's the ConferenceParams C++ class that is used behind
 */
typedef struct _LinphoneConferenceParams LinphoneConferenceParams;




/**
 * Create a #LinphoneConferenceParams with default parameters set.
 * @param core #LinphoneCore to use to find out the default parameters. Can be NULL.
 * @return A freshly allocated #LinphoneConferenceParams
 */
LINPHONE_PUBLIC LinphoneConferenceParams *linphone_conference_params_new(const LinphoneCore *core);

/**
 * Free a #LinphoneConferenceParams
 * @param params #LinphoneConferenceParams to free
 */
LINPHONE_PUBLIC void linphone_conference_params_free(LinphoneConferenceParams *params);

/**
 * Clone a #LinphoneConferenceParams
 * @param params The #LinphoneConferenceParams to clone
 * @return An allocated #LinphoneConferenceParams with the same parameters than params
 */
LINPHONE_PUBLIC LinphoneConferenceParams *linphone_conference_params_clone(const LinphoneConferenceParams *params);

/**
 * Enable video when starting a conference
 * @param params A #LinphoneConferenceParams
 * @param enable If true, video will be enabled during conference
 */
LINPHONE_PUBLIC void linphone_conference_params_enable_video(LinphoneConferenceParams *params, bool_t enable);

/**
 * Check whether video will be enable at conference starting
 * @return if true, the video will be enable at conference starting
 */
LINPHONE_PUBLIC bool_t linphone_conference_params_video_requested(const LinphoneConferenceParams *params);





/**
 * Remove a participant from a conference
 * @param obj A #LinphoneConference
 * @param uri SIP URI of the participant to remove
 * @warning The passed SIP URI must be one of the URIs returned by linphone_conference_get_participants()
 * @return 0 if succeeded, -1 if failed
 */
LINPHONE_PUBLIC int linphone_conference_remove_participant(LinphoneConference *obj, const LinphoneAddress *uri);

/**
 * Get URIs of all participants of one conference
 * The returned bctbx_list_t contains URIs of all participant. That list must be
 * freed after use and each URI must be unref with linphone_address_unref()
 * @param obj A #LinphoneConference
 * @return \bctbx_list{LinphoneAddress}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_conference_get_participants(const LinphoneConference *obj);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // CONFERENCE_H
