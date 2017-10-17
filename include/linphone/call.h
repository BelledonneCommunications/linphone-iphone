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
#include "linphone/types.h"

/**
 * @addtogroup call_control
 * @{
 */

/** Callback prototype */
typedef void (*LinphoneCallCbFunc)(LinphoneCall *call, void *user_data);


#ifdef __cplusplus
extern "C" {
#endif

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
 * Returns the to address with its headers associated to this call
**/
LINPHONE_PUBLIC const LinphoneAddress * linphone_call_get_to_address(const LinphoneCall * call);

/**
 * Returns the value of the header name
**/
LINPHONE_PUBLIC const char * linphone_call_get_to_header(const LinphoneCall *call, const char *name);

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
LINPHONE_PUBLIC LinphoneStatus linphone_call_take_video_snapshot(LinphoneCall *call, const char *file);

/**
 * Take a photo of currently captured video and write it into a jpeg file.
 * Note that the snapshot is asynchronous, an application shall not assume that the file is created when the function returns.
 * @param call a LinphoneCall
 * @param file a path where to write the jpeg content.
 * @return 0 if successfull, -1 otherwise (typically if jpeg format is not supported).
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_take_preview_snapshot(LinphoneCall *call, const char *file);

/**
 * Returns the reason for a call termination (either error or normal termination)
**/
LINPHONE_PUBLIC LinphoneReason linphone_call_get_reason(const LinphoneCall *call);


/**
 * Returns full details about call errors or termination reasons.
 * @param  call LinphoneCall object on which we want the information error
 * @return      LinphoneErrorInfo object holding the reason error.
 */
LINPHONE_PUBLIC const LinphoneErrorInfo *linphone_call_get_error_info(const LinphoneCall *call);

/**
 * Returns the far end's user agent description string, if available.
**/
LINPHONE_PUBLIC const char *linphone_call_get_remote_user_agent(LinphoneCall *call);

/**
 * Returns the far end's sip contact as a string, if available.
**/
LINPHONE_PUBLIC const char *linphone_call_get_remote_contact(LinphoneCall *call);

/**
 * Returns the ZRTP authentication token to verify.
 * @param call the LinphoneCall
 * @return the authentication token to verify.
**/
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
LINPHONE_PUBLIC LinphoneStatus linphone_call_send_dtmf(LinphoneCall *call, char dtmf);

/**
 * Send a list of dtmf.
 *
 * The dtmfs are automatically sent to remote, separated by some needed customizable delay.
 * Sending is canceled if the call state changes to something not LinphoneCallStreamsRunning.
 * @param call The LinphoneCall object
 * @param dtmfs A dtmf sequence such as '123#123123'
 * @return -2 if there is already a DTMF sequence, -1 if call is not ready, 0 otherwise.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_send_dtmfs(LinphoneCall *call, const char *dtmfs);

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
 * @donotwrap
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
 * @donotwrap
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
 * Pauses the call. If a music file has been setup using linphone_core_set_play_file(),
 * this file will be played to the remote user.
 * The only way to resume a paused call is to call linphone_call_resume().
 * @param[in] call LinphoneCall object
 * @return 0 on success, -1 on failure
 * @see linphone_call_resume()
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_pause(LinphoneCall *call);

/**
 * Resumes a call.
 * The call needs to have been paused previously with linphone_call_pause().
 * @param[in] call LinphoneCall object
 * @return 0 on success, -1 on failure
 * @see linphone_call_pause()
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_resume(LinphoneCall *call);

/**
 * Terminates a call.
 * @param[in] call LinphoneCall object
 * @return 0 on success, -1 on failure
**/LINPHONE_PUBLIC LinphoneStatus linphone_call_terminate(LinphoneCall *call);


/**
 * Terminates a call.
 * @param[in] call 	LinphoneCall object
 * @param[in] ei 	LinphoneErrorInfo
 * @return 0 on success, -1 on failure
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_terminate_with_error_info(LinphoneCall *call, const LinphoneErrorInfo *ei);

/**
 * Redirect the specified call to the given redirect URI.
 * @param[in] call A LinphoneCall object
 * @param[in] redirect_uri The URI to redirect the call to
 * @return 0 if successful, -1 on error.
 */
LINPHONE_PUBLIC LinphoneStatus linphone_call_redirect(LinphoneCall *call, const char *redirect_uri);

/**
 * Decline a pending incoming call, with a reason.
 * @param[in] call A LinphoneCall object that must be in the IncomingReceived state
 * @param[in] reason The reason for rejecting the call: LinphoneReasonDeclined or LinphoneReasonBusy
 * @return 0 on success, -1 on failure
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_decline(LinphoneCall * call, LinphoneReason reason);

/**
 * Decline a pending incoming call, with a LinphoneErrorInfo object.
 * @param[in] call A LinphoneCall object that must be in the IncomingReceived state
 * @param[in] ei LinphoneErrorInfo containing more information on the call rejection.
 * @return 0 on success, -1 on failure
 */
LINPHONE_PUBLIC int linphone_call_decline_with_error_info(LinphoneCall * call, const LinphoneErrorInfo *ei);

/**
 * Accept an incoming call.
 *
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #LinphoneCoreVTable structure, where it will receive
 * a LinphoneCallIncoming event with the associated LinphoneCall object.
 * The application can later accept the call using this method.
 * @param[in] call A LinphoneCall object
 * @return 0 on success, -1 on failure
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_accept(LinphoneCall *call);

/**
 * Accept an incoming call, with parameters.
 *
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #LinphoneCoreVTable structure, where it will receive
 * a LinphoneCallIncoming event with the associated LinphoneCall object.
 * The application can later accept the call using this method.
 * @param[in] call A LinphoneCall object
 * @param[in] params The specific parameters for this call, for example whether video is accepted or not. Use NULL to use default parameters
 * @return 0 on success, -1 on failure
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_accept_with_params(LinphoneCall *call, const LinphoneCallParams *params);

/**
 * Accept an early media session for an incoming call.
 * This is identical as calling linphone_call_accept_early_media_with_params() with NULL parameters.
 * @param[in] call A LinphoneCall object
 * @return 0 if successful, -1 otherwise
 * @see linphone_call_accept_early_media_with_params()
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_accept_early_media(LinphoneCall *call);

/**
 * When receiving an incoming, accept to start a media session as early-media.
 * This means the call is not accepted but audio & video streams can be established if the remote party supports early media.
 * However, unlike after call acceptance, mic and camera input are not sent during early-media, though received audio & video are played normally.
 * The call can then later be fully accepted using linphone_call_accept() or linphone_call_accept_with_params().
 * @param[in] call A LinphoneCall object
 * @param[in] params The call parameters to use (can be NULL)
 * @return 0 if successful, -1 otherwise
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_accept_early_media_with_params(LinphoneCall *call, const LinphoneCallParams *params);

/**
 * Updates a running call according to supplied call parameters or parameters changed in the LinphoneCore.
 * It triggers a SIP reINVITE in order to perform a new offer/answer of media capabilities. 
 * Changing the size of the transmitted video after calling linphone_core_set_preferred_video_size() can be used by passing NULL as params argument.
 * In case no changes are requested through the LinphoneCallParams argument, then this argument can be omitted and set to NULL.
 * WARNING: Updating a call in the LinphoneCallPaused state will still result in a paused call even if the media directions set in the
 * params are sendrecv. To resume a paused call, you need to call linphone_call_resume().
 *
 * @param[in] call A LinphoneCall object
 * @param[in] params The new call parameters to use (may be NULL)
 * @return 0 if successful, -1 otherwise.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_update(LinphoneCall *call, const LinphoneCallParams *params);

/**
 * When receiving a #LinphoneCallUpdatedByRemote state notification, prevent LinphoneCore from performing an automatic answer.
 *
 * When receiving a #LinphoneCallUpdatedByRemote state notification (ie an incoming reINVITE), the default behaviour of
 * LinphoneCore is defined by the "defer_update_default" option of the "sip" section of the config. If this option is 0 (the default)
 * then the LinphoneCore automatically answers the reINIVTE with call parameters unchanged.
 * However when for example when the remote party updated the call to propose a video stream, it can be useful
 * to prompt the user before answering. This can be achieved by calling linphone_core_defer_call_update() during
 * the call state notification, to deactivate the automatic answer that would just confirm the audio but reject the video.
 * Then, when the user responds to dialog prompt, it becomes possible to call linphone_call_accept_update() to answer
 * the reINVITE, with eventually video enabled in the LinphoneCallParams argument.
 *
 * The #LinphoneCallUpdatedByRemote notification can also arrive when receiving an INVITE without SDP. In such case, an unchanged offer is made
 * in the 200Ok, and when the ACK containing the SDP answer is received, #LinphoneCallUpdatedByRemote is triggered to notify the application of possible
 * changes in the media session. However in such case defering the update has no meaning since we just generating an offer.
 *
 * @param[in] call A LinphoneCall object
 * @return 0 if successful, -1 if the linphone_call_defer_update() was done outside a valid #LinphoneCallUpdatedByRemote notification
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_defer_update(LinphoneCall *call);

/**
 * Accept call modifications initiated by other end.
 *
 * This call may be performed in response to a #LinphoneCallUpdatedByRemote state notification.
 * When such notification arrives, the application can decide to call linphone_call_defer_update() so that it can
 * have the time to prompt the user. linphone_call_get_remote_params() can be used to get information about the call parameters
 * requested by the other party, such as whether a video stream is requested.
 *
 * When the user accepts or refuse the change, linphone_call_accept_update() can be done to answer to the other party.
 * If params is NULL, then the same call parameters established before the update request will continue to be used (no change).
 * If params is not NULL, then the update will be accepted according to the parameters passed.
 * Typical example is when a user accepts to start video, then params should indicate that video stream should be used
 * (see linphone_call_params_enable_video()).
 * @param[in] call A LinphoneCall object
 * @param[in] params A LinphoneCallParams object describing the call parameters to accept
 * @return 0 if successful, -1 otherwise (actually when this function call is performed outside ot #LinphoneCallUpdatedByRemote state)
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_accept_update(LinphoneCall *call, const LinphoneCallParams *params);

/**
 * Performs a simple call transfer to the specified destination.
 * The remote endpoint is expected to issue a new call to the specified destination.
 * The current call remains active and thus can be later paused or terminated.
 * It is possible to follow the progress of the transfer provided that transferee sends notification about it.
 * In this case, the transfer_state_changed callback of the #LinphoneCoreVTable is invoked to notify of the state of the new call at the other party.
 * The notified states are #LinphoneCallOutgoingInit , #LinphoneCallOutgoingProgress, #LinphoneCallOutgoingRinging and #LinphoneCallConnected.
 * @param[in] call The call to be transfered
 * @param[in] refer_to The destination the call is to be refered to
 * @return 0 on success, -1 on failure
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_transfer(LinphoneCall *call, const char *refer_to);

/**
 * Transfers a call to destination of another running call. This is used for "attended transfer" scenarios.
 * The transfered call is supposed to be in paused state, so that it is able to accept the transfer immediately.
 * The destination call is a call previously established to introduce the transfered person.
 * This method will send a transfer request to the transfered person. The phone of the transfered is then
 * expected to automatically call to the destination of the transfer. The receiver of the transfer will then automatically
 * close the call with us (the 'dest' call).
 * It is possible to follow the progress of the transfer provided that transferee sends notification about it.
 * In this case, the transfer_state_changed callback of the #LinphoneCoreVTable is invoked to notify of the state of the new call at the other party.
 * The notified states are #LinphoneCallOutgoingInit , #LinphoneCallOutgoingProgress, #LinphoneCallOutgoingRinging and #LinphoneCallConnected.
 * @param[in] call A running call you want to transfer
 * @param[in] dest A running call whose remote person will receive the transfer
 * @return 0 on success, -1 on failure
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_transfer_to_another(LinphoneCall *call, LinphoneCall *dest);


/**
 * Acquire a reference to the LinphoneCallCbs object.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The same LinphoneCallCbs object.
 */
LINPHONE_PUBLIC LinphoneCallCbs *linphone_call_cbs_ref(LinphoneCallCbs *cbs);

/**
 * Release reference to the LinphoneCallCbs object.
 * @param[in] cbs LinphoneCallCbs object.
 */
LINPHONE_PUBLIC void linphone_call_cbs_unref(LinphoneCallCbs *cbs);

/**
 * Retrieve the user pointer associated with the LinphoneCallCbs object.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The user pointer associated with the LinphoneCallCbs object.
 */
LINPHONE_PUBLIC void *linphone_call_cbs_get_user_data(const LinphoneCallCbs *cbs);

/**
 * Assign a user pointer to the LinphoneCallCbs object.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneCallCbs object.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_user_data(LinphoneCallCbs *cbs, void *user_data);

/**
 * Get the dtmf received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current dtmf received callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsDtmfReceivedCb linphone_call_cbs_get_dtmf_received(LinphoneCallCbs *cbs);

/**
 * Set the dtmf received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The dtmf received callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_dtmf_received(LinphoneCallCbs *cbs, LinphoneCallCbsDtmfReceivedCb cb);

/**
 * Get the encryption changed callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current encryption changed callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsEncryptionChangedCb linphone_call_cbs_get_encryption_changed(LinphoneCallCbs *cbs);

/**
 * Set the encryption changed callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The encryption changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_encryption_changed(LinphoneCallCbs *cbs, LinphoneCallCbsEncryptionChangedCb cb);

/**
 * Get the info message received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current info message received callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsInfoMessageReceivedCb linphone_call_cbs_get_info_message_received(LinphoneCallCbs *cbs);

/**
 * Set the info message received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The info message received callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_info_message_received(LinphoneCallCbs *cbs, LinphoneCallCbsInfoMessageReceivedCb cb);

/**
 * Get the state changed callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current state changed callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsStateChangedCb linphone_call_cbs_get_state_changed(LinphoneCallCbs *cbs);

/**
 * Set the state changed callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_state_changed(LinphoneCallCbs *cbs, LinphoneCallCbsStateChangedCb cb);

/**
 * Get the stats updated callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current stats updated callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsStatsUpdatedCb linphone_call_cbs_get_stats_updated(LinphoneCallCbs *cbs);

/**
 * Set the stats updated callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The stats updated callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_stats_updated(LinphoneCallCbs *cbs, LinphoneCallCbsStatsUpdatedCb cb);

/**
 * Get the transfer state changed callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current transfer state changed callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsTransferStateChangedCb linphone_call_cbs_get_transfer_state_changed(LinphoneCallCbs *cbs);

/**
 * Set the transfer state changed callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The transfer state changed callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_transfer_state_changed(LinphoneCallCbs *cbs, LinphoneCallCbsTransferStateChangedCb cb);

/**
 * Get the ACK processing callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current ack processing callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsAckProcessingCb linphone_call_cbs_get_ack_processing(LinphoneCallCbs *cbs);

/**
 * Set ACK processing callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The ack processing callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_ack_processing(LinphoneCallCbs *cbs, LinphoneCallCbsAckProcessingCb cb);

/**
 * Get the TMMBR received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @return The current TMMBR received callback.
 */
LINPHONE_PUBLIC LinphoneCallCbsTmmbrReceivedCb linphone_call_cbs_get_tmmbr_received(LinphoneCallCbs *cbs);

/**
 * Set the TMMBR received callback.
 * @param[in] cbs LinphoneCallCbs object.
 * @param[in] cb The TMMBR received callback to be used.
 */
LINPHONE_PUBLIC void linphone_call_cbs_set_tmmbr_received(LinphoneCallCbs *cbs, LinphoneCallCbsTmmbrReceivedCb cb);

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
LINPHONE_PUBLIC LinphoneChatRoom * linphone_call_get_chat_room(LinphoneCall *call);

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

/**
 * Get a player associated with the call to play a local file and stream it to the remote peer.
 * @param[in] call LinphoneCall object
 * @return A LinphonePlayer object
 */
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
 * Call generic OpenGL render for a given call.
 * @param call The call.
 */
LINPHONE_PUBLIC void linphone_call_ogl_render(const LinphoneCall *call);



/**
 * Send a LinphoneInfoMessage through an established call
 * @param call the call
 * @param info the info message
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_send_info_message(LinphoneCall *call, const LinphoneInfoMessage *info);

/**
 * Return a copy of the call statistics for a particular stream type.
 * @param call the call
 * @param type the stream type
**/
LINPHONE_PUBLIC LinphoneCallStats *linphone_call_get_stats(LinphoneCall *call, LinphoneStreamType type);

LINPHONE_PUBLIC LinphoneCallStats *linphone_call_get_audio_stats(LinphoneCall *call);

LINPHONE_PUBLIC LinphoneCallStats *linphone_call_get_video_stats(LinphoneCall *call);

LINPHONE_PUBLIC LinphoneCallStats *linphone_call_get_text_stats(LinphoneCall *call);

/**
 * Add a listener in order to be notified of LinphoneCall events. Once an event is received, registred LinphoneCallCbs are
 * invoked sequencially.
 * @param[in] call LinphoneCall object to monitor.
 * @param[in] cbs A LinphoneCallCbs object holding the callbacks you need. A reference is taken by the LinphoneCall until you invoke linphone_call_remove_callbacks().
 */
LINPHONE_PUBLIC void linphone_call_add_callbacks(LinphoneCall *call, LinphoneCallCbs *cbs);

/**
 * Remove a listener from a LinphoneCall
 * @param[in] call LinphoneCall object
 * @param[in] cbs LinphoneCallCbs object to remove.
 */
LINPHONE_PUBLIC void linphone_call_remove_callbacks(LinphoneCall *call, LinphoneCallCbs *cbs);

/**
 * Gets the current LinphoneCallCbs.
 * This is meant only to be called from a callback to be able to get the user_data associated with the LinphoneCallCbs that is calling the callback.
 * @param[in] call LinphoneCall object
 * @return The LinphoneCallCbs that has called the last callback
 */
LINPHONE_PUBLIC LinphoneCallCbs *linphone_call_get_current_callbacks(const LinphoneCall *call);

/**
 * Set call parameters - advanced and not recommended feature - use with caution.
 * Local call parameters applicable to an outgoing or incoming shall usually be passed to linphone_core_invite_address_with_params() or
 * linphone_call_accept_with_params().
 * However, in some cases it might be desirable from a software design standpoint to modify local parameters outside of the application layer, typically
 * in the purpose of implementing a custom logic including special headers in INVITE or 200Ok requests, driven by a call_state_changed listener method.
 * This function accepts to assign a new LinphoneCallParams only in LinphoneCallOutgoingInit and LinphoneCallIncomingReceived states.
 * @param call the LinphoneCall object
**/
LINPHONE_PUBLIC void linphone_call_set_params(LinphoneCall *call, const LinphoneCallParams *params);

/**
 * Returns local parameters associated with the call.
 * This is typically the parameters passed at call initiation to linphone_core_invite_address_with_params() or linphone_call_accept_with_params(), or some default
 * parameters if no LinphoneCallParams was explicitely passed during call initiation.
 * @param call the LinphoneCall object
 * @return the call's local parameters.
 **/
LINPHONE_PUBLIC const LinphoneCallParams * linphone_call_get_params(LinphoneCall *call);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_CALL_H */
