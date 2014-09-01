/*
linphone
Copyright (C) 2010-2014  Belledonne Communications SARL

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


#ifndef __LINPHONE_CALL_PARAMS_H__
#define __LINPHONE_CALL_PARAMS_H__

/**
 * @addtogroup call_control
 * @{
**/


/*******************************************************************************
 * Structures and enums                                                        *
 ******************************************************************************/

/**
 * Private structure definition for LinphoneCallParams.
**/
struct _LinphoneCallParams;

/**
 * The LinphoneCallParams is an object containing various call related parameters.
 * It can be used to retrieve parameters from a currently running call or modify
 * the call's characteristics dynamically.
**/
typedef struct _LinphoneCallParams LinphoneCallParams;


/*******************************************************************************
 * Public functions                                                            *
 ******************************************************************************/

/**
 * Add a custom SIP header in the INVITE for a call.
 * @param[in] cp The #LinphoneCallParams to add a custom SIP header to.
 * @param[in] header_name The name of the header to add.
 * @param[in] header_value The content of the header to add.
**/
LINPHONE_PUBLIC void linphone_call_params_add_custom_header(LinphoneCallParams *cp, const char *header_name, const char *header_value);

/**
 * Copy an existing LinphoneCallParams object to a new LinphoneCallParams object.
 * @param[in] cp The LinphoneCallParams object to copy.
 * @return A copy of the LinphoneCallParams object.
**/
LINPHONE_PUBLIC LinphoneCallParams * linphone_call_params_copy(const LinphoneCallParams *cp);

/**
 * Indicate whether sending of early media was enabled.
 * @param[in] cp LinphoneCallParams object
 * @return A boolean value telling whether sending of early media was enabled.
**/
LINPHONE_PUBLIC bool_t linphone_call_params_early_media_sending_enabled(const LinphoneCallParams *cp);

/**
 * Enable sending of real early media (during outgoing calls).
 * @param[in] cp LinphoneCallParams object
 * @param[in] enabled A boolean value telling whether to enable early media sending or not.
**/
LINPHONE_PUBLIC void linphone_call_params_enable_early_media_sending(LinphoneCallParams *cp, bool_t enabled);

/**
 * Indicate low bandwith mode.
 * Configuring a call to low bandwidth mode will result in the core to activate several settings for the call in order to ensure that bitrate usage
 * is lowered to the minimum possible. Typically, ptime (packetization time) will be increased, audio codec's output bitrate will be targetted to 20kbit/s provided
 * that it is achievable by the codec selected after SDP handshake. Video is automatically disabled.
 * @param[in] cp LinphoneCallParams object
 * @param[in] enabled A boolean value telling whether to activate the low bandwidth mode or not.
**/
LINPHONE_PUBLIC void linphone_call_params_enable_low_bandwidth(LinphoneCallParams *cp, bool_t enabled);

/**
 * Enable video stream.
 * @param[in] cp LinphoneCallParams object
 * @param[in] enabled A boolean value telling whether to enable video or not.
**/
LINPHONE_PUBLIC void linphone_call_params_enable_video(LinphoneCallParams *cp, bool_t enabled);

/**
 * Get a custom SIP header.
 * @param[in] cp The #LinphoneCallParams to get the custom SIP header from.
 * @param[in] header_name The name of the header to get.
 * @return The content of the header or NULL if not found.
**/
LINPHONE_PUBLIC const char *linphone_call_params_get_custom_header(const LinphoneCallParams *cp, const char *header_name);

/**
 * Tell whether the call is part of the locally managed conference.
 * @param[in] cp LinphoneCallParams object
 * @return A boolean value telling whether the call is part of the locally managed conference.
**/
LINPHONE_PUBLIC bool_t linphone_call_params_get_local_conference_mode(const LinphoneCallParams *cp);

/**
 * Get the kind of media encryption selected for the call.
 * @param[in] cp LinphoneCallParams object
 * @return The kind of media encryption selected for the call.
**/
LINPHONE_PUBLIC LinphoneMediaEncryption linphone_call_params_get_media_encryption(const LinphoneCallParams *cp);

/**
 * Get requested level of privacy for the call.
 * @param[in] cp LinphoneCallParams object
 * @return The privacy mode used for the call.
**/
LINPHONE_PUBLIC LinphonePrivacyMask linphone_call_params_get_privacy(const LinphoneCallParams *cp);

/**
 * Get the framerate of the video that is received.
 * @param[in] cp LinphoneCallParams object
 * @return The actual received framerate in frames per seconds, 0 if not available.
 */
LINPHONE_PUBLIC float linphone_call_params_get_received_framerate(const LinphoneCallParams *cp);

/**
 * Get the size of the video that is received.
 * @param[in] cp LinphoneCallParams object
 * @return The received video size or MS_VIDEO_SIZE_UNKNOWN if not available.
 */
LINPHONE_PUBLIC MSVideoSize linphone_call_params_get_received_video_size(const LinphoneCallParams *cp);

/**
 * Get the path for the audio recording of the call.
 * @param[in] cp LinphoneCallParams object
 * @return The path to the audio recording of the call.
**/
LINPHONE_PUBLIC const char *linphone_call_params_get_record_file(const LinphoneCallParams *cp);

/**
 * Get the RTP profile being used.
 * @param[in] cp #LinphoneCallParams object
 * @return The RTP profile.
 */
LINPHONE_PUBLIC const char * linphone_call_params_get_rtp_profile(const LinphoneCallParams *cp);

/**
 * Get the framerate of the video that is sent.
 * @param[in] cp LinphoneCallParams object
 * @return The actual sent framerate in frames per seconds, 0 if not available.
 */
LINPHONE_PUBLIC float linphone_call_params_get_sent_framerate(const LinphoneCallParams *cp);

/**
 * Gets the size of the video that is sent.
 * @param[in] cp LinphoneCalParams object
 * @return The sent video size or MS_VIDEO_SIZE_UNKNOWN if not available.
 */
LINPHONE_PUBLIC MSVideoSize linphone_call_params_get_sent_video_size(const LinphoneCallParams *cp);

/**
 * Get the session name of the media session (ie in SDP).
 * Subject from the SIP message can be retrieved using linphone_call_params_get_custom_header() and is different.
 * @param[in] cp LinphoneCallParams object
 * @return The session name of the media session.
**/
LINPHONE_PUBLIC const char *linphone_call_params_get_session_name(const LinphoneCallParams *cp);

/**
 * Get the audio codec used in the call, described as a LinphonePayloadType object.
 * @param[in] cp LinphoneCallParams object
 * @return The LinphonePayloadType object corresponding to the audio codec being used in the call.
**/
LINPHONE_PUBLIC const LinphonePayloadType* linphone_call_params_get_used_audio_codec(const LinphoneCallParams *cp);

/**
 * Get the video codec used in the call, described as a LinphonePayloadType structure.
 * @param[in] cp LinphoneCallParams object
 * @return The LinphonePayloadType object corresponding to the video codec being used in the call.
**/
LINPHONE_PUBLIC const LinphonePayloadType* linphone_call_params_get_used_video_codec(const LinphoneCallParams *cp);

/**
 * Tell whether the call has been configured in low bandwidth mode or not.
 * This mode can be automatically discovered thanks to a stun server when activate_edge_workarounds=1 in section [net] of configuration file.
 * An application that would have reliable way to know network capacity may not use activate_edge_workarounds=1 but instead manually configure
 * low bandwidth mode with linphone_call_params_enable_low_bandwidth().
 * When enabled, this param may transform a call request with video in audio only mode.
 * @param[in] cp LinphoneCallParams object
 * @return A boolean value telling whether the low bandwidth mode has been configured/detected.
 */
LINPHONE_PUBLIC bool_t linphone_call_params_low_bandwidth_enabled(const LinphoneCallParams *cp);

/**
 * Refine bandwidth settings for this call by setting a bandwidth limit for audio streams.
 * As a consequence, codecs whose bitrates are not compatible with this limit won't be used.
 * @param[in] cp LinphoneCallParams object
 * @param[in] bw The audio bandwidth limit to set in kbit/s.
**/
LINPHONE_PUBLIC void linphone_call_params_set_audio_bandwidth_limit(LinphoneCallParams *cp, int bw);

/**
 * Set requested media encryption for a call.
 * @param[in] cp LinphoneCallParams object
 * @param[in] enc The media encryption to use for the call.
**/
LINPHONE_PUBLIC void linphone_call_params_set_media_encryption(LinphoneCallParams *cp, LinphoneMediaEncryption enc);

/**
 * Set requested level of privacy for the call.
 * \xmlonly <language-tags>javascript</language-tags> \endxmlonly
 * @param[in] cp LinphoneCallParams object
 * @param[in] privacy The privacy mode to used for the call.
**/
LINPHONE_PUBLIC void linphone_call_params_set_privacy(LinphoneCallParams *params, LinphonePrivacyMask privacy);

/**
 * Enable recording of the call.
 * This function must be used before the call parameters are assigned to the call.
 * The call recording can be started and paused after the call is established with
 * linphone_call_start_recording() and linphone_call_pause_recording().
 * @param[in] cp LinphoneCallParams object
 * @param[in] path A string containing the path and filename of the file where audio/video streams are to be written.
 * The filename must have either .mkv or .wav extention. The video stream will be written only if a MKV file is given.
**/
LINPHONE_PUBLIC void linphone_call_params_set_record_file(LinphoneCallParams *cp, const char *path);

/**
 * Set the session name of the media session (ie in SDP).
 * Subject from the SIP message (which is different) can be set using linphone_call_params_set_custom_header().
 * @param[in] cp LinphoneCallParams object
 * @param[in] name The session name to be used.
**/
LINPHONE_PUBLIC void linphone_call_params_set_session_name(LinphoneCallParams *cp, const char *name);

/**
 * Tell whether video is enabled or not.
 * @param[in] cp LinphoneCallParams object
 * @return A boolean value telling whether video is enabled or not.
**/
LINPHONE_PUBLIC bool_t linphone_call_params_video_enabled(const LinphoneCallParams *cp);


/*******************************************************************************
 * Reference and user data handling functions                                  *
 ******************************************************************************/

/**
 * Get the user data associated with the call params.
 * @param[in] cl LinphoneCallParams object
 * @return The user data associated with the call params.
**/
LINPHONE_PUBLIC void *linphone_call_params_get_user_data(const LinphoneCallParams *cp);

/**
 * Assign a user data to the call params.
 * @param[in] cl LinphoneCallParams object
 * @param[in] ud The user data to associate with the call params.
**/
LINPHONE_PUBLIC void linphone_call_params_set_user_data(LinphoneCallParams *cp, void *ud);

/**
 * Acquire a reference to the call params.
 * @param[in] cl LinphoneCallParams object
 * @return The same LinphoneCallParams object
**/
LINPHONE_PUBLIC LinphoneCallParams * linphone_call_params_ref(LinphoneCallParams *cp);

/**
 * Release a reference to the call params.
 * @param[in] cl LinphoneCallParams object
**/
LINPHONE_PUBLIC void linphone_call_params_unref(LinphoneCallParams *cp);


/*******************************************************************************
 * DEPRECATED                                                                  *
 ******************************************************************************/

/** @deprecated Use linphone_call_params_get_local_conference_mode() instead. */
#define linphone_call_params_local_conference_mode linphone_call_params_get_local_conference_mode

/**
 * Destroy a LinphoneCallParams object.
 * @param[in] cp LinphoneCallParams object
 * @deprecated Use linphone_call_params_unref() instead.
**/
LINPHONE_PUBLIC void linphone_call_params_destroy(LinphoneCallParams *cp);


/**
 * @}
**/


#endif /* __LINPHONE_CALL_PARAMS_H__ */
