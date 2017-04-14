/*
linphone
Copyright (C) 2010 Simon MORLAT (simon.morlat@linphone.org)

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

#ifndef LINPHONE_CORE_UTILS_H_
#define LINPHONE_CORE_UTILS_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LsdEndOfPlayCallback)(LsdPlayer *p);

LINPHONE_PUBLIC void lsd_player_set_callback(LsdPlayer *p, LsdEndOfPlayCallback cb);
LINPHONE_PUBLIC void lsd_player_set_user_pointer(LsdPlayer *p, void *up);
LINPHONE_PUBLIC void *lsd_player_get_user_pointer(const LsdPlayer *p);
LINPHONE_PUBLIC LinphoneStatus lsd_player_play(LsdPlayer *p, const char *filename);
LINPHONE_PUBLIC LinphoneStatus lsd_player_stop(LsdPlayer *p);
LINPHONE_PUBLIC void lsd_player_enable_loop(LsdPlayer *p, bool_t loopmode);
LINPHONE_PUBLIC bool_t lsd_player_loop_enabled(const LsdPlayer *p);
LINPHONE_PUBLIC void lsd_player_set_gain(LsdPlayer *p, float gain);
LINPHONE_PUBLIC LinphoneSoundDaemon *lsd_player_get_daemon(const LsdPlayer *p);

LINPHONE_PUBLIC LinphoneSoundDaemon * linphone_sound_daemon_new(MSFactory* factory, const char *cardname, int rate, int nchannels);
LINPHONE_PUBLIC LsdPlayer * linphone_sound_daemon_get_player(LinphoneSoundDaemon *lsd);
LINPHONE_PUBLIC void linphone_sound_daemon_release_player(LinphoneSoundDaemon *lsd, LsdPlayer *lsdplayer);
LINPHONE_PUBLIC void linphone_sound_daemon_stop_all_players(LinphoneSoundDaemon *obj);
LINPHONE_PUBLIC void linphone_sound_daemon_release_all_players(LinphoneSoundDaemon *obj);
LINPHONE_PUBLIC void linphone_core_use_sound_daemon(LinphoneCore *lc, LinphoneSoundDaemon *lsd);
LINPHONE_PUBLIC void linphone_sound_daemon_destroy(LinphoneSoundDaemon *obj);


typedef void (*LinphoneEcCalibrationCallback)(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay_ms, void *data);
typedef void (*LinphoneEcCalibrationAudioInit)(void *data);
typedef void (*LinphoneEcCalibrationAudioUninit)(void *data);

/**
 *
 * Start an echo calibration of the sound devices, in order to find adequate settings for the echo canceller automatically.
**/
LINPHONE_PUBLIC int linphone_core_start_echo_calibration(LinphoneCore *lc, LinphoneEcCalibrationCallback cb,
					 LinphoneEcCalibrationAudioInit audio_init_cb, LinphoneEcCalibrationAudioUninit audio_uninit_cb, void *cb_data);
/**
 * Start the simulation of call to test the latency with an external device
 * @param lc The core.
 * @param rate Sound sample rate.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_start_echo_tester(LinphoneCore *lc, unsigned int rate);
/**
 * Stop the simulation of call
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_stop_echo_tester(LinphoneCore *lc);
/**
 * @ingroup IOS
 * Special function to warm up  dtmf feeback stream. #linphone_core_stop_dtmf_stream must() be called before entering FG mode
 */
LINPHONE_PUBLIC void linphone_core_start_dtmf_stream(LinphoneCore* lc);
/**
 * @ingroup IOS
 * Special function to stop dtmf feed back function. Must be called before entering BG mode
 */
LINPHONE_PUBLIC void linphone_core_stop_dtmf_stream(LinphoneCore* lc);


typedef bool_t (*LinphoneCoreIterateHook)(void *data);

void linphone_core_add_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data);

void linphone_core_remove_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data);

typedef struct _LinphoneDialPlan {
	const char *country;
	const char* iso_country_code; /* ISO 3166-1 alpha-2 code, ex: FR for France*/
	char  ccc[8]; /*country calling code*/
	int nnl; /*maximum national number length*/
	const char * icp; /*international call prefix, ex: 00 in europe*/
} LinphoneDialPlan;

/**
 * @ingroup misc
 *Function to get  call country code from  ISO 3166-1 alpha-2 code, ex: FR returns 33
 *@param iso country code alpha2
 *@return call country code or -1 if not found
 */
LINPHONE_PUBLIC	int linphone_dial_plan_lookup_ccc_from_iso(const char* iso);

/**
 * @ingroup misc
 *Function to get  call country code from  an e164 number, ex: +33952650121 will return 33
 *@param e164 phone number
 *@return call country code or -1 if not found
 */
LINPHONE_PUBLIC	int linphone_dial_plan_lookup_ccc_from_e164(const char* e164);

/**
 * Return NULL-terminated array of all known dial plans
**/
LINPHONE_PUBLIC const LinphoneDialPlan* linphone_dial_plan_get_all(void);

/**
 * Find best match for given CCC
 * @return Return matching dial plan, or a generic one if none found
**/
LINPHONE_PUBLIC const LinphoneDialPlan* linphone_dial_plan_by_ccc(const char *ccc);
/**
 * Find best match for given CCC
 * @return Return matching dial plan, or a generic one if none found
 **/
LINPHONE_PUBLIC const LinphoneDialPlan* linphone_dial_plan_by_ccc_as_int(int ccc);

/**
 * Return if given plan is generic
**/
LINPHONE_PUBLIC bool_t linphone_dial_plan_is_generic(const LinphoneDialPlan *ccc);

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_CORE_UTILS_H_ */
