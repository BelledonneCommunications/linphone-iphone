/*
call.h
Copyright (C) 2016  Belledonne Communications SARL

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

#ifndef LINPHONE_CALL_H
#define LINPHONE_CALL_H

#include <mediastreamer2/mscommon.h>
#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

#include "linphone/address.h"
#include "linphone/call_log.h"
#include "linphone/call_params.h"
#include "linphone/conference.h"
#include "linphone/error_info.h"

/**
 * @addtogroup call_control
 * @{
 */

/**
 * LinphoneCallState enum represents the different state a call can reach into.
 * The application is notified of state changes through the LinphoneCoreVTable::call_state_changed callback.
**/
typedef enum _LinphoneCallState{
	LinphoneCallIdle,					/**<Initial call state */
	LinphoneCallIncomingReceived, /**<This is a new incoming call */
	LinphoneCallOutgoingInit, /**<An outgoing call is started */
	LinphoneCallOutgoingProgress, /**<An outgoing call is in progress */
	LinphoneCallOutgoingRinging, /**<An outgoing call is ringing at remote end */
	LinphoneCallOutgoingEarlyMedia, /**<An outgoing call is proposed early media */
	LinphoneCallConnected, /**<Connected, the call is answered */
	LinphoneCallStreamsRunning, /**<The media streams are established and running*/
	LinphoneCallPausing, /**<The call is pausing at the initiative of local end */
	LinphoneCallPaused, /**< The call is paused, remote end has accepted the pause */
	LinphoneCallResuming, /**<The call is being resumed by local end*/
	LinphoneCallRefered, /**<The call is being transfered to another party, resulting in a new outgoing call to follow immediately*/
	LinphoneCallError, /**<The call encountered an error*/
	LinphoneCallEnd, /**<The call ended normally*/
	LinphoneCallPausedByRemote, /**<The call is paused by remote end*/
	LinphoneCallUpdatedByRemote, /**<The call's parameters change is requested by remote end, used for example when video is added by remote */
	LinphoneCallIncomingEarlyMedia, /**<We are proposing early media to an incoming call */
	LinphoneCallUpdating, /**<A call update has been initiated by us */
	LinphoneCallReleased, /**< The call object is no more retained by the core */
	LinphoneCallEarlyUpdatedByRemote, /*<The call is updated by remote while not yet answered (early dialog SIP UPDATE received).*/
	LinphoneCallEarlyUpdating /*<We are updating the call while not yet answered (early dialog SIP UPDATE sent)*/
} LinphoneCallState;

/**
 * Enum describing type of audio route.
**/
enum _LinphoneAudioRoute {
	LinphoneAudioRouteEarpiece = MSAudioRouteEarpiece,
	LinphoneAudioRouteSpeaker = MSAudioRouteSpeaker
};

/**
 * Enum describing type of audio route.
**/
typedef enum _LinphoneAudioRoute LinphoneAudioRoute;


/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
**/
struct _LinphoneCall;

/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
**/
typedef struct _LinphoneCall LinphoneCall;

/** Callback prototype */
typedef void (*LinphoneCallCbFunc)(LinphoneCall *call, void *user_data);


#ifdef __cplusplus
extern "C" {
#endif

LINPHONE_PUBLIC const char *linphone_call_state_to_string(LinphoneCallState cs);

/**
 * Acquire a reference to the call.
 * An application that wishes to retain a pointer to call object
 * must use this function to unsure the pointer remains
 * valid. Once the application no more needs this pointer,
 * it must call linphone_call_unref().
 * @param[in] call The call.
 * @return The same call.
**/
LINPHONE_PUBLIC LinphoneCall * linphone_call_ref(LinphoneCall *call);

/**
 * Release reference to the call.
 * @param[in] call The call.
**/
LINPHONE_PUBLIC void linphone_call_unref(LinphoneCall *call);

/**
 * Retrieve the user pointer associated with the call.
 * @param[in] call The call.
 * @return The user pointer associated with the call.
**/
LINPHONE_PUBLIC void * linphone_call_get_user_data(const LinphoneCall *call);

/**
 * Assign a user pointer to the call.
 * @param[in] call The call.
 * @param[in] ud The user pointer to associate with the call.
**/
LINPHONE_PUBLIC void linphone_call_set_user_data(LinphoneCall *call, void *ud);

/**
 * Get the core that has created the specified call.
 * @param[in] call LinphoneCall object
 * @return The LinphoneCore object that has created the specified call.
 */
LINPHONE_PUBLIC LinphoneCore * linphone_call_get_core(const LinphoneCall *call);

/**
 * Retrieves the call's current state.
**/
LINPHONE_PUBLIC LinphoneCallState linphone_call_get_state(const LinphoneCall *call);

/**
 * Tell whether a call has been asked to autoanswer
 * @param[in] call LinphoneCall object
 * @return A boolean value telling whether the call has been asked to autoanswer
**/
LINPHONE_PUBLIC bool_t linphone_call_asked_to_autoanswer(LinphoneCall *call);

/**
 * Returns the remote address associated to this call
**/
LINPHONE_PUBLIC const LinphoneAddress * linphone_call_get_remote_address(const LinphoneCall *call);

/**
 * Returns the remote address associated to this call as a string.
 * The result string must be freed by user using ms_free().
**/
LINPHONE_PUBLIC char * linphone_call_get_remote_address_as_string(const LinphoneCall *call);

/**
 * Returns the diversion address associated to this call
**/
LINPHONE_PUBLIC const LinphoneAddress * linphone_call_get_diversion_address(const LinphoneCall *call);

/**
 * Returns direction of the call (incoming or outgoing).
**/
LINPHONE_PUBLIC LinphoneCallDir linphone_call_get_dir(const LinphoneCall *call);

/**
 * Gets the call log associated to this call.
 * @param[in] call LinphoneCall object
 * @return The LinphoneCallLog associated with the specified LinphoneCall
**/
LINPHONE_PUBLIC LinphoneCallLog * linphone_call_get_call_log(const LinphoneCall *call);

/**
 * Gets the refer-to uri (if the call was transfered).
 * @param[in] call LinphoneCall object
 * @return The refer-to uri of the call (if it was transfered)
**/
LINPHONE_PUBLIC const char * linphone_call_get_refer_to(const LinphoneCall *call);

/**
 * Returns true if this calls has received a transfer that has not been
 * executed yet.
 * Pending transfers are executed when this call is being paused or closed,
 * locally or by remote endpoint.
 * If the call is already paused while receiving the transfer request, the
 * transfer immediately occurs.
**/
LINPHONE_PUBLIC bool_t linphone_call_has_transfer_pending(const LinphoneCall *call);

/**
 * Gets the transferer if this call was started automatically as a result of an incoming transfer request.
 * The call in which the transfer request was received is returned in this case.
 * @param[in] call LinphoneCall object
 * @return The transferer call if the specified call was started automatically as a result of an incoming transfer request, NULL otherwise
**/
LINPHONE_PUBLIC LinphoneCall * linphone_call_get_transferer_call(const LinphoneCall *call);

/**
 * When this call has received a transfer request, returns the new call that was automatically created as a result of the transfer.
**/
LINPHONE_PUBLIC LinphoneCall * linphone_call_get_transfer_target_call(const LinphoneCall *call);

/**
 * Returns the call object this call is replacing, if any.
 * Call replacement can occur during call transfers.
 * By default, the core automatically terminates the replaced call and accept the new one.
 * This function allows the application to know whether a new incoming call is a one that replaces another one.
**/
LINPHONE_PUBLIC LinphoneCall * linphone_call_get_replaced_call(LinphoneCall *call);

/**
 * Returns call's duration in seconds.
**/
LINPHONE_PUBLIC int linphone_call_get_duration(const LinphoneCall *call);

/**
 * Returns current parameters associated to the call.
**/
LINPHONE_PUBLIC const LinphoneCallParams * linphone_call_get_current_params(LinphoneCall *call);

/**
 * Returns call parameters proposed by remote.
 *
 * This is useful when receiving an incoming call, to know whether the remote party
 * supports video, encryption or whatever.
**/
LINPHONE_PUBLIC const LinphoneCallParams * linphone_call_get_remote_params(LinphoneCall *call);

/**
 * Indicate whether camera input should be sent to remote end.
**/
LINPHONE_PUBLIC void linphone_call_enable_camera(LinphoneCall *lc, bool_t enabled);

/**
 * Returns TRUE if camera pictures are allowed to be sent to the remote party.
**/
LINPHONE_PUBLIC bool_t linphone_call_camera_enabled(const LinphoneCall *lc);

/**
 * Take a photo of currently received video and write it into a jpeg file.
 * Note that the snapshot is asynchronous, an application shall not assume that the file is created when the function returns.
 * @param call a LinphoneCall
 * @param file a path where to write the jpeg content.
 * @return 0 if successfull, -1 otherwise (typically if jpeg format is not supported).
**/
LINPHONE_PUBLIC int linphone_call_take_video_snapshot(LinphoneCall *call, const char *file);

/**
 * Take a photo of currently captured video and write it into a jpeg file.
 * Note that the snapshot is asynchronous, an application shall not assume that the file is created when the function returns.
 * @param call a LinphoneCall
 * @param file a path where to write the jpeg content.
 * @return 0 if successfull, -1 otherwise (typically if jpeg format is not supported).
**/
LINPHONE_PUBLIC int linphone_call_take_preview_snapshot(LinphoneCall *call, const char *file);

/**
 * Returns the reason for a call termination (either error or normal termination)
**/
LINPHONE_PUBLIC LinphoneReason linphone_call_get_reason(const LinphoneCall *call);

/**
 * Returns full details about call errors or termination reasons.
**/
LINPHONE_PUBLIC const LinphoneErrorInfo *linphone_call_get_error_info(const LinphoneCall *call);

/**
 * Returns the far end's user agent description string, if available.
**/
LINPHONE_PUBLIC const char *linphone_call_get_remote_user_agent(LinphoneCall *call);

/**
 * Returns the far end's sip contact as a string, if available.
**/
LINPHONE_PUBLIC const char *linphone_call_get_remote_contact(LinphoneCall *call);

LINPHONE_PUBLIC const char * linphone_call_get_authentication_token(LinphoneCall *call);

/**
 * Returns whether ZRTP authentication token is verified.
 * If not, it must be verified by users as described in ZRTP procedure.
 * Once done, the application must inform of the results with linphone_call_set_authentication_token_verified().
 * @param call the LinphoneCall
 * @return TRUE if authentication token is verifed, false otherwise.
**/
LINPHONE_PUBLIC bool_t linphone_call_get_authentication_token_verified(LinphoneCall *call);

/**
 * Set the result of ZRTP short code verification by user.
 * If remote party also does the same, it will update the ZRTP cache so that user's verification will not be required for the two users.
 * @param call the LinphoneCall
 * @param verified whether the ZRTP SAS is verified.
**/
LINPHONE_PUBLIC void linphone_call_set_authentication_token_verified(LinphoneCall *call, bool_t verified);

/**
 * Request remote side to send us a Video Fast Update.
**/
LINPHONE_PUBLIC void linphone_call_send_vfu_request(LinphoneCall *call);

/** @deprecated Use linphone_call_get_user_data() instead. */
#define linphone_call_get_user_pointer(call) linphone_call_get_user_data(call)

/** @deprecated Use linphone_call_set_user_data() instead. */
#define linphone_call_set_user_pointer(call, ud) linphone_call_set_user_data(call, ud)

LINPHONE_PUBLIC void linphone_call_set_next_video_frame_decoded_callback(LinphoneCall *call, LinphoneCallCbFunc cb, void *user_data);

/**
 * Returns the current transfer state, if a transfer has been initiated from this call.
 * @see linphone_core_transfer_call() , linphone_core_transfer_call_to_another()
**/
LINPHONE_PUBLIC LinphoneCallState linphone_call_get_transfer_state(LinphoneCall *call);

/**
 * Perform a zoom of the video displayed during a call.
 * @param call the call.
 * @param zoom_factor a floating point number describing the zoom factor. A value 1.0 corresponds to no zoom applied.
 * @param cx a floating point number pointing the horizontal center of the zoom to be applied. This value should be between 0.0 and 1.0.
 * @param cy a floating point number pointing the vertical center of the zoom to be applied. This value should be between 0.0 and 1.0.
 *
 * cx and cy are updated in return in case their coordinates were too excentrated for the requested zoom factor. The zoom ensures that all the screen is fullfilled with the video.
**/
LINPHONE_PUBLIC void linphone_call_zoom_video(LinphoneCall *call, float zoom_factor, float *cx, float *cy);

/**
 * Send the specified dtmf.
 *
 * The dtmf is automatically played to the user.
 * @param call The LinphoneCall object
 * @param dtmf The dtmf name specified as a char, such as '0', '#' etc...
 * @return 0 if successful, -1 on error.
**/
LINPHONE_PUBLIC int linphone_call_send_dtmf(LinphoneCall *call, char dtmf);

/**
 * Send a list of dtmf.
 *
 * The dtmfs are automatically sent to remote, separated by some needed customizable delay.
 * Sending is canceled if the call state changes to something not LinphoneCallStreamsRunning.
 * @param call The LinphoneCall object
 * @param dtmfs A dtmf sequence such as '123#123123'
 * @return -2 if there is already a DTMF sequence, -1 if call is not ready, 0 otherwise.
**/
LINPHONE_PUBLIC int linphone_call_send_dtmfs(LinphoneCall *call, const char *dtmfs);

/**
 * Stop current DTMF sequence sending.
 *
 * Please note that some DTMF could be already sent,
 * depending on when this function call is delayed from #linphone_call_send_dtmfs. This
 * function will be automatically called if call state change to anything but LinphoneCallStreamsRunning.
 * @param call The LinphoneCall object
**/
LINPHONE_PUBLIC void linphone_call_cancel_dtmfs(LinphoneCall *call);

/**
 * Return TRUE if this call is currently part of a conference
 * @param call #LinphoneCall
 * @return TRUE if part of a conference.
 * @deprecated Use linphone_call_get_conference() instead.
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED bool_t linphone_call_is_in_conference(const LinphoneCall *call);

/**
 * Return the associated conference object
 * @param call #LinphoneCall
 * @return A pointer on #LinphoneConference or NULL if the call is not part of any conference.
 */
LINPHONE_PUBLIC LinphoneConference * linphone_call_get_conference(const LinphoneCall *call);

/**
 * Change the playback output device (currently only used for blackberry)
 * @param call
 * @param route the wanted audio route (earpiece, speaker, ...)
**/
LINPHONE_PUBLIC void linphone_call_set_audio_route(LinphoneCall *call, LinphoneAudioRoute route);

/**
 * Returns the number of stream for the given call.
 * Currently there is only two (Audio, Video), but later there will be more.
 * @param call
 * @return 2
**/
LINPHONE_PUBLIC int linphone_call_get_stream_count(LinphoneCall *call);

/**
 * Returns the type of stream for the given stream index.
 * @param call
 * @param stream_index
 * @return the type (MSAudio, MSVideo, MSText) of the stream of given index.
**/
LINPHONE_PUBLIC MSFormatType linphone_call_get_stream_type(LinphoneCall *call, int stream_index);

/**
 * Returns the meta rtp transport for the given stream index.
 * @param call
 * @param stream_index
 * @return a pointer to the meta rtp transport if it exists, NULL otherwise
**/
LINPHONE_PUBLIC RtpTransport * linphone_call_get_meta_rtp_transport(LinphoneCall *call, int stream_index);

/**
 * Returns the meta rtcp transport for the given stream index.
 * @param call
 * @param stream_index
 * @return a pointer to the meta rtcp transport if it exists, NULL otherwise
**/
LINPHONE_PUBLIC RtpTransport * linphone_call_get_meta_rtcp_transport(LinphoneCall *call, int stream_index);

/**
 * @}
 */


/**
 * @addtogroup media_parameters
 * @{
 */

/**
 * Get the native window handle of the video window, casted as an unsigned long.
**/
LINPHONE_PUBLIC void * linphone_call_get_native_video_window_id(const LinphoneCall *call);

/**
 * Set the native video window id where the video is to be displayed.
 * For MacOS, Linux, Windows: if not set or 0 a window will be automatically created, unless the special id -1 is given.
**/
LINPHONE_PUBLIC void linphone_call_set_native_video_window_id(LinphoneCall *call, void * id);

/**
 * Enables or disable echo cancellation for this call
 * @param call
 * @param val
**/
LINPHONE_PUBLIC void linphone_call_enable_echo_cancellation(LinphoneCall *call, bool_t val) ;

/**
 * Returns TRUE if echo cancellation is enabled.
**/
LINPHONE_PUBLIC bool_t linphone_call_echo_cancellation_enabled(LinphoneCall *lc);

/**
 * Enables or disable echo limiter for this call
 * @param call
 * @param val
**/
LINPHONE_PUBLIC void linphone_call_enable_echo_limiter(LinphoneCall *call, bool_t val);

/**
 * Returns TRUE if echo limiter is enabled.
**/
LINPHONE_PUBLIC bool_t linphone_call_echo_limiter_enabled(const LinphoneCall *call);

/**
 * @}
 */

/**
 * @addtogroup call_misc
 * @{
 */

/**
 * Create a new chat room for messaging from a call if not already existing, else return existing one.
 * No reference is given to the caller: the chat room will be deleted when the call is ended.
 * @param call #LinphoneCall object
 * @return #LinphoneChatRoom where messaging can take place.
 */
LINPHONE_PUBLIC struct _LinphoneChatRoom * linphone_call_get_chat_room(LinphoneCall *call);

/**
 * Get the mesured playback volume level (received from remote) in dbm0.
 * @param call The call.
 * @return float Volume level in percentage.
 */
LINPHONE_PUBLIC float linphone_call_get_play_volume(LinphoneCall *call);

/**
 * Get the mesured record volume level (sent to remote) in dbm0.
 * @param call The call.
 * @return float Volume level in percentage.
 */
LINPHONE_PUBLIC float linphone_call_get_record_volume(LinphoneCall *call);

/**
 * Get speaker volume gain.
 * If the sound backend supports it, the returned gain is equal to the gain set
 * with the system mixer.
 * @param call The call.
 * @return Percenatge of the max supported volume gain. Valid values are in [ 0.0 : 1.0 ].
 * In case of failure, a negative value is returned
 */
LINPHONE_PUBLIC float linphone_call_get_speaker_volume_gain(const LinphoneCall *call);

/**
 * Set speaker volume gain.
 * If the sound backend supports it, the new gain will synchronized with the system mixer.
 * @param call The call.
 * @param volume Percentage of the max supported gain. Valid values are in [ 0.0 : 1.0 ].
 */
LINPHONE_PUBLIC void linphone_call_set_speaker_volume_gain(LinphoneCall *call, float volume);

/**
 * Get microphone volume gain.
 * If the sound backend supports it, the returned gain is equal to the gain set
 * with the system mixer.
 * @param call The call.
 * @return double Percenatge of the max supported volume gain. Valid values are in [ 0.0 : 1.0 ].
 * In case of failure, a negative value is returned
 */
LINPHONE_PUBLIC float linphone_call_get_microphone_volume_gain(const LinphoneCall *call);

/**
 * Set microphone volume gain.
 * If the sound backend supports it, the new gain will synchronized with the system mixer.
 * @param call The call.
 * @param volume Percentage of the max supported gain. Valid values are in [ 0.0 : 1.0 ].
 */
LINPHONE_PUBLIC void linphone_call_set_microphone_volume_gain(LinphoneCall *call, float volume);

/**
 * Obtain real-time quality rating of the call
 *
 * Based on local RTP statistics and RTCP feedback, a quality rating is computed and updated
 * during all the duration of the call. This function returns its value at the time of the function call.
 * It is expected that the rating is updated at least every 5 seconds or so.
 * The rating is a floating point number comprised between 0 and 5.
 *
 * 4-5 = good quality <br>
 * 3-4 = average quality <br>
 * 2-3 = poor quality <br>
 * 1-2 = very poor quality <br>
 * 0-1 = can't be worse, mostly unusable <br>
 *
 * @return The function returns -1 if no quality measurement is available, for example if no
 * active audio stream exist. Otherwise it returns the quality rating.
**/
LINPHONE_PUBLIC float linphone_call_get_current_quality(LinphoneCall *call);

/**
 * Returns call quality averaged over all the duration of the call.
 *
 * See linphone_call_get_current_quality() for more details about quality measurement.
**/
LINPHONE_PUBLIC float linphone_call_get_average_quality(LinphoneCall *call);

/**
 * Start call recording.
 * The output file where audio is recorded must be previously specified with linphone_call_params_set_record_file().
**/
LINPHONE_PUBLIC void linphone_call_start_recording(LinphoneCall *call);

/**
 * Stop call recording.
**/
LINPHONE_PUBLIC void linphone_call_stop_recording(LinphoneCall *call);

LINPHONE_PUBLIC LinphonePlayer * linphone_call_get_player(LinphoneCall *call);

/**
 * Indicates whether an operation is in progress at the media side.
 * It can be a bad idea to initiate signaling operations (adding video, pausing the call, removing video, changing video parameters) while
 * the media is busy in establishing the connection (typically ICE connectivity checks). It can result in failures generating loss of time
 * in future operations in the call.
 * Applications are invited to check this function after each call state change to decide whether certain operations are permitted or not.
 * @param call the call
 * @return TRUE if media is busy in establishing the connection, FALSE otherwise.
**/
LINPHONE_PUBLIC bool_t linphone_call_media_in_progress(LinphoneCall *call);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_CALL_H */
