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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef LINPHONECORE_UTILS_H
#define LINPHONECORE_UTILS_H

#ifdef IN_LINPHONE
#include "linphonecore.h"
#else
#include "linphone/linphonecore.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LsdPlayer LsdPlayer;
typedef struct _LinphoneSoundDaemon LinphoneSoundDaemon;

typedef void (*LsdEndOfPlayCallback)(LsdPlayer *p);

void lsd_player_set_callback(LsdPlayer *p, LsdEndOfPlayCallback cb);
void lsd_player_set_user_pointer(LsdPlayer *p, void *up);
void *lsd_player_get_user_pointer(const LsdPlayer *p);
int lsd_player_play(LsdPlayer *p, const char *filename);
int lsd_player_stop(LsdPlayer *p);
void lsd_player_enable_loop(LsdPlayer *p, bool_t loopmode);
bool_t lsd_player_loop_enabled(const LsdPlayer *p);
void lsd_player_set_gain(LsdPlayer *p, float gain);
LinphoneSoundDaemon *lsd_player_get_daemon(const LsdPlayer *p);

LinphoneSoundDaemon * linphone_sound_daemon_new(const char *cardname, int rate, int nchannels);
LsdPlayer * linphone_sound_daemon_get_player(LinphoneSoundDaemon *lsd);
void linphone_sound_daemon_release_player(LinphoneSoundDaemon *lsd, LsdPlayer *lsdplayer);
void linphone_sound_daemon_stop_all_players(LinphoneSoundDaemon *obj);
void linphone_sound_daemon_release_all_players(LinphoneSoundDaemon *obj);
void linphone_core_use_sound_daemon(LinphoneCore *lc, LinphoneSoundDaemon *lsd);
void linphone_sound_daemon_destroy(LinphoneSoundDaemon *obj);

/**
 * Enum describing the result of the echo canceller calibration process.
**/
typedef enum {
	LinphoneEcCalibratorInProgress,	/**< The echo canceller calibration process is on going. */
	LinphoneEcCalibratorDone,	/**< The echo canceller calibration has been performed and produced an echo delay measure. */
	LinphoneEcCalibratorFailed,	/**< The echo canceller calibration process has failed. */
	LinphoneEcCalibratorDoneNoEcho	/**< The echo canceller calibration has been performed and no echo has been detected. */
}LinphoneEcCalibratorStatus;


typedef void (*LinphoneEcCalibrationCallback)(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay_ms, void *data);
typedef void (*LinphoneEcCalibrationAudioInit)(void *data);
typedef void (*LinphoneEcCalibrationAudioUninit)(void *data);

/**
 *
 * Start an echo calibration of the sound devices, in order to find adequate settings for the echo canceller automatically.
**/
int linphone_core_start_echo_calibration(LinphoneCore *lc, LinphoneEcCalibrationCallback cb,
					 LinphoneEcCalibrationAudioInit audio_init_cb, LinphoneEcCalibrationAudioUninit audio_uninit_cb, void *cb_data);
/**
 * @ingroup IOS
 * Special function to warm up  dtmf feeback stream. #linphone_core_stop_dtmf_stream must() be called before entering FG mode
 */
void linphone_core_start_dtmf_stream(LinphoneCore* lc);
/**
 * @ingroup IOS
 * Special function to stop dtmf feed back function. Must be called before entering BG mode
 */
void linphone_core_stop_dtmf_stream(LinphoneCore* lc);


typedef bool_t (*LinphoneCoreIterateHook)(void *data);

void linphone_core_add_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data);

void linphone_core_remove_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data);
/**
 * @ingroup misc
 *Function to get  call country code from  ISO 3166-1 alpha-2 code, ex: FR returns 33
 *@param iso country code alpha2
 *@return call country code or -1 if not found
 */
int linphone_dial_plan_lookup_ccc_from_iso(const char* iso); 
/**
 * @ingroup misc
 *Function to get  call country code from  an e164 number, ex: +33952650121 will return 33
 *@param e164 phone number
 *@return call country code or -1 if not found
 */
int linphone_dial_plan_lookup_ccc_from_e164(const char* e164);

#ifdef __cplusplus
}
#endif
#endif

