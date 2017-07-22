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

#ifndef LINPHONE_CONFERENCE_H
#define LINPHONE_CONFERENCE_H

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup call_control
 * @{
 */

/**
 * Create a #LinphoneConferenceParams with default parameters set.
 * @param core #LinphoneCore to use to find out the default parameters. Can be NULL.
 * @return A freshly allocated #LinphoneConferenceParams
 */
LINPHONE_PUBLIC LinphoneConferenceParams *linphone_conference_params_new(const LinphoneCore *core);

/**
 * Take a reference on a #LinphoneConferencParams.
 * @param[in] params The #LinphoneConferenceParams to ref.
 * @return The freshly refed #LinphoneConferenceParams.
 */
LINPHONE_PUBLIC LinphoneConferenceParams *linphone_conference_params_ref(LinphoneConferenceParams *params);

/**
 * Release a #LinphoneConferenceParams.
 * @param[in] params The #LinphoneConferenceParams to release.
 */
LINPHONE_PUBLIC void linphone_conference_params_unref(LinphoneConferenceParams *params);

/**
 * Free a #LinphoneConferenceParams
 * @param params #LinphoneConferenceParams to free
 * @deprecated Use linphone_conference_params_unref() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_conference_params_free(LinphoneConferenceParams *params);

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
 * Take a reference on a #LinphoneConference.
 * @param[in] conf The #LinphoneConference to ref.
 * @return The freshly refed #LinphoneConference.
 */
LINPHONE_PUBLIC LinphoneConference *linphone_conference_ref(LinphoneConference *conf);

/**
 * Release a #LinphoneConference.
 * @param[in] conf The #LinphoneConference to release.
 */
LINPHONE_PUBLIC void linphone_conference_unref(LinphoneConference *conf);

/**
 * Remove a participant from a conference
 * @param obj A #LinphoneConference
 * @param uri SIP URI of the participant to remove
 * @warning The passed SIP URI must be one of the URIs returned by linphone_conference_get_participants()
 * @return 0 if succeeded, -1 if failed
 */
LINPHONE_PUBLIC LinphoneStatus linphone_conference_remove_participant(LinphoneConference *obj, const LinphoneAddress *uri);

/**
 * Get URIs of all participants of one conference
 * The returned bctbx_list_t contains URIs of all participant. That list must be
 * freed after use and each URI must be unref with linphone_address_unref()
 * @param obj A #LinphoneConference
 * @return \bctbx_list{LinphoneAddress}
 */
LINPHONE_PUBLIC bctbx_list_t *linphone_conference_get_participants(const LinphoneConference *obj);

/**
 * Invite participants to the conference, by supplying a list of LinphoneAddress
 * @param obj The conference.
 * @param addresses \bctbx_list{LinphoneAddress}
 * @param params #LinphoneCallParams to use for inviting the participants.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_conference_invite_participants(LinphoneConference *conf, const bctbx_list_t *addresses, const LinphoneCallParams *params);

/**
  * Get the conference id as string
  */
LINPHONE_PUBLIC const char *linphone_conference_get_ID(const LinphoneConference *obj);

/**
  * Set the conference id as string
  */
LINPHONE_PUBLIC void linphone_conference_set_ID(const LinphoneConference *obj, const char *conferenceID);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif // LINPHONE_CONFERENCE_H
