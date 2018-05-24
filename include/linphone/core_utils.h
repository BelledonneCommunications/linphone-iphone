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
#include "linphone/callbacks.h"


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


LINPHONE_DEPRECATED typedef void (*LinphoneEcCalibrationCallback)(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay_ms, void *data);
LINPHONE_DEPRECATED typedef void (*LinphoneEcCalibrationAudioInit)(void *data);
LINPHONE_DEPRECATED typedef void (*LinphoneEcCalibrationAudioUninit)(void *data);

/**
 * @brief Starts an echo calibration of the sound devices, in order to find adequate settings for the echo canceler automatically.
 * @deprecated Use #linphone_core_start_echo_canceller_calibration() instead. To set the callbacks create or get an already instantiated
 * #LinphoneCoreCbs and call #linphone_core_cbs_set_ec_calibration_result(), #linphone_core_cbs_set_ec_calibration_audio_init() and
 * #linphone_core_cbs_set_ec_callibration_audio_uninit(). Deprecated since 2017-10-16.
 * @ingroup misc
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC int linphone_core_start_echo_calibration(LinphoneCore *lc, LinphoneEcCalibrationCallback cb,
					 LinphoneEcCalibrationAudioInit audio_init_cb, LinphoneEcCalibrationAudioUninit audio_uninit_cb, void *cb_data);

/**
 * @brief Starts an echo calibration of the sound devices, in order to find adequate settings for the echo canceler automatically.
 * @param[in] lc #LinphoneCore object.
 * @return #LinphoneStatus whether calibration has started or not.
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_start_echo_canceller_calibration(LinphoneCore *lc);

/**
 * Start the simulation of call to test the latency with an external device
 * @param lc The core.
 * @param rate Sound sample rate.
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_start_echo_tester(LinphoneCore *lc, unsigned int rate);

/**
 * Stop the simulation of call
 * @ingroup misc
**/
LINPHONE_PUBLIC LinphoneStatus linphone_core_stop_echo_tester(LinphoneCore *lc);

/**
 * Check whether the device is flagged has crappy opengl
 * @returns TRUE if crappy opengl flag is set, FALSE otherwise
 * @ingroup misc
**/
LINPHONE_PUBLIC bool_t linphone_core_has_crappy_opengl(LinphoneCore *lc);

/**
 * Check whether the device has a hardware echo canceller
 * @returns TRUE if it does, FALSE otherwise
 * @ingroup misc
**/
LINPHONE_PUBLIC bool_t linphone_core_has_builtin_echo_canceller(LinphoneCore *lc);

/**
 * Check whether the device is echo canceller calibration is required
 * @returns TRUE if it is required, FALSE otherwise
 * @ingroup misc
**/
LINPHONE_PUBLIC bool_t linphone_core_is_echo_canceller_calibration_required(LinphoneCore *lc);

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

LINPHONE_PUBLIC void linphone_core_add_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data);

LINPHONE_PUBLIC void linphone_core_remove_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data);

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_CORE_UTILS_H_ */
