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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef __LINPHONE_CALL_PARAMS_H__
#define __LINPHONE_CALL_PARAMS_H__


#include "linphone/types.h"
#include "linphone/payload_type.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup call_control
 * @{
**/

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
 * Enable audio stream.
 * @param[in] cp LinphoneCallParams object
 * @param[in] enabled A boolean value telling whether to enable audio or not.
**/
LINPHONE_PUBLIC void linphone_call_params_enable_audio(LinphoneCallParams *cp, bool_t enabled);

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
 * @warning If a conference server is used to manage conferences,
 * that function does not return TRUE even if the conference is running.<br/>
 * If you want to test whether the conference is running, you should test
 * whether linphone_core_get_conference() return a non-null pointer.
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
 * Get the definition of the received video.
 * @param[in] cp LinphoneCallParams object
 * @return The received LinphoneVideoDefinition
 */
LINPHONE_PUBLIC const LinphoneVideoDefinition * linphone_call_params_get_received_video_definition(const LinphoneCallParams *cp);

/**
 * Get the size of the video that is received.
 * @param[in] cp LinphoneCallParams object
 * @return The received video size or MS_VIDEO_SIZE_UNKNOWN if not available.
 * @deprecated Use linphone_call_params_get_received_video_definition() instead
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED MSVideoSize linphone_call_params_get_received_video_size(const LinphoneCallParams *cp);

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
 * Get the definition of the sent video.
 * @param[in] cp LinphoneCallParams object
 * @return The sent LinphoneVideoDefinition
 */
LINPHONE_PUBLIC const LinphoneVideoDefinition * linphone_call_params_get_sent_video_definition(const LinphoneCallParams *cp);

/**
 * Gets the size of the video that is sent.
 * @param[in] cp LinphoneCalParams object
 * @return The sent video size or MS_VIDEO_SIZE_UNKNOWN if not available.
 * @deprecated Use linphone_call_params_get_sent_video_definition() instead
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED MSVideoSize linphone_call_params_get_sent_video_size(const LinphoneCallParams *cp);

/**
 * Get the session name of the media session (ie in SDP).
 * Subject from the SIP message can be retrieved using linphone_call_params_get_custom_header() and is different.
 * @param[in] cp LinphoneCallParams object
 * @return The session name of the media session.
**/
LINPHONE_PUBLIC const char *linphone_call_params_get_session_name(const LinphoneCallParams *cp);

/**
 * Get the audio payload type that has been selected by a call.
 * @param[in] cp The call.
 * @return The selected payload type. NULL is returned if no audio payload type has been seleced
 * by the call.
**/
LINPHONE_PUBLIC LinphonePayloadType *linphone_call_params_get_used_audio_payload_type(const LinphoneCallParams *cp);

/**
 * Get the video payload type that has been selected by a call.
 * @param[in] cp The call.
 * @return The selected payload type. NULL is returned if no video payload type has been seleced
 * by the call.
**/
LINPHONE_PUBLIC LinphonePayloadType *linphone_call_params_get_used_video_payload_type(const LinphoneCallParams *cp);

/**
 * Get the text payload type that has been selected by a call.
 * @param[in] cp The call.
 * @return The selected payload type. NULL is returned if no text payload type has been seleced
 * by the call.
**/
LINPHONE_PUBLIC LinphonePayloadType *linphone_call_params_get_used_text_payload_type(const LinphoneCallParams *cp);

/**
 * Get the audio payload type that has been selected by a call.
 * @param[in] cp The call.
 * @return The selected payload type. NULL is returned if no audio payload type has been seleced by the call.
 * @deprecated Use linphone_call_params_get_used_audio_payload_type() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const OrtpPayloadType *linphone_call_params_get_used_audio_codec(const LinphoneCallParams *cp);

/**
 * Get the video payload type that has been selected by a call.
 * @param[in] cp The call.
 * @return The selected payload type. NULL is returned if no video payload type has been seleced by the call.
 * @deprecated Use linphone_call_params_get_used_video_payload_type() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const OrtpPayloadType *linphone_call_params_get_used_video_codec(const LinphoneCallParams *cp);

/**
 * Get the text payload type that has been selected by a call.
 * @param[in] cp The call.
 * @return The selected payload type. NULL is returned if no text payload type has been seleced by the call.
 * @deprecated Use linphone_call_params_get_used_text_payload_type() instead.
 * @donotwrap
**/
LINPHONE_PUBLIC LINPHONE_DEPRECATED const OrtpPayloadType *linphone_call_params_get_used_text_codec(const LinphoneCallParams *cp);

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
LINPHONE_PUBLIC void linphone_call_params_set_privacy(LinphoneCallParams *cp, LinphonePrivacyMask privacy);

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
 * Tell whether audio is enabled or not.
 * @param[in] cp LinphoneCallParams object
 * @return A boolean value telling whether audio is enabled or not.
**/
LINPHONE_PUBLIC bool_t linphone_call_params_audio_enabled(const LinphoneCallParams *cp);

/**
 * Tell whether video is enabled or not.
 * @param[in] cp LinphoneCallParams object
 * @return A boolean value telling whether video is enabled or not.
**/
LINPHONE_PUBLIC bool_t linphone_call_params_video_enabled(const LinphoneCallParams *cp);

/**
 * Get the audio stream direction.
 * @param[in] cp LinphoneCallParams object
 * @return The audio stream direction associated with the call params.
**/
LINPHONE_PUBLIC  LinphoneMediaDirection linphone_call_params_get_audio_direction(const LinphoneCallParams *cp);

/**
 * Get the video stream direction.
 * @param[in] cp LinphoneCallParams object
 * @return The video stream direction associated with the call params.
**/
LINPHONE_PUBLIC  LinphoneMediaDirection linphone_call_params_get_video_direction(const LinphoneCallParams *cp);

/**
 * Set the audio stream direction.
 * @param[in] cp LinphoneCallParams object
 * @param[in] dir The audio stream direction associated with this call params.
**/
LINPHONE_PUBLIC void linphone_call_params_set_audio_direction(LinphoneCallParams *cp, LinphoneMediaDirection dir);

/**
 * Set the video stream direction.
 * @param[in] cp LinphoneCallParams object
 * @param[in] dir The video stream direction associated with this call params.
**/
LINPHONE_PUBLIC void linphone_call_params_set_video_direction(LinphoneCallParams *cp, LinphoneMediaDirection dir);


/*******************************************************************************
 * Reference and user data handling functions                                  *
 ******************************************************************************/

/**
 * Get the user data associated with the call params.
 * @param[in] cp LinphoneCallParams object
 * @return The user data associated with the call params.
**/
LINPHONE_PUBLIC void *linphone_call_params_get_user_data(const LinphoneCallParams *cp);

/**
 * Assign a user data to the call params.
 * @param[in] cp LinphoneCallParams object
 * @param[in] ud The user data to associate with the call params.
**/
LINPHONE_PUBLIC void linphone_call_params_set_user_data(LinphoneCallParams *cp, void *ud);

/**
 * Acquire a reference to the call params.
 * @param[in] cp LinphoneCallParams object
 * @return The same LinphoneCallParams object
**/
LINPHONE_PUBLIC LinphoneCallParams * linphone_call_params_ref(LinphoneCallParams *cp);

/**
 * Release a reference to the call params.
 * @param[in] cp LinphoneCallParams object
**/
LINPHONE_PUBLIC void linphone_call_params_unref(LinphoneCallParams *cp);


/**
 * Use to enable multicast rtp for audio stream.
 * * If enabled, outgoing calls put a multicast address from #linphone_core_get_video_multicast_addr into audio cline. In case of outgoing call audio stream is sent to this multicast address.
 * <br> For incoming calls behavior is unchanged.
 * @param params #LinphoneCallParams
 * @param yesno if yes, subsequent calls will propose multicast ip set by #linphone_core_set_audio_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_call_params_enable_audio_multicast(LinphoneCallParams *params, bool_t yesno);

/**
 * Use to get multicast state of audio stream.
 * @param params #LinphoneCallParams
 * @return true if  subsequent calls will propose multicast ip set by #linphone_core_set_audio_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_call_params_audio_multicast_enabled(const LinphoneCallParams *params);

/**
 * Use to enable multicast rtp for video stream.
 * If enabled, outgoing calls put a multicast address from #linphone_core_get_video_multicast_addr into video cline. In case of outgoing call video stream is sent to this  multicast address.
 * <br> For incoming calls behavior is unchanged.
 * @param params #LinphoneCallParams
 * @param yesno if yes, subsequent outgoing calls will propose multicast ip set by #linphone_core_set_video_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_call_params_enable_video_multicast(LinphoneCallParams *params, bool_t yesno);

/**
 * Use to get multicast state of video stream.
 * @param params #LinphoneCallParams
 * @return true if  subsequent calls will propose multicast ip set by #linphone_core_set_video_multicast_addr
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_call_params_video_multicast_enabled(const LinphoneCallParams *params);

/**
 * Use to enable real time text following rfc4103.
 * If enabled, outgoing calls put a m=text line in SDP offer .
 * @param params #LinphoneCallParams
 * @param yesno if yes, subsequent outgoing calls will propose rtt
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC LinphoneStatus linphone_call_params_enable_realtime_text(LinphoneCallParams *params, bool_t yesno);

/**
 * Use to get real time text following rfc4103.
 * @param params #LinphoneCallParams
 * @returns returns true if call rtt is activated.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_call_params_realtime_text_enabled(const LinphoneCallParams *params);

/**
 * Add a custom attribute related to all the streams in the SDP exchanged within SIP messages during a call.
 * @param[in] params The #LinphoneCallParams to add a custom SDP attribute to.
 * @param[in] attribute_name The name of the attribute to add.
 * @param[in] attribute_value The content value of the attribute to add.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_call_params_add_custom_sdp_attribute(LinphoneCallParams *params, const char *attribute_name, const char *attribute_value);

/**
 * Add a custom attribute related to a specific stream in the SDP exchanged within SIP messages during a call.
 * @param[in] params The #LinphoneCallParams to add a custom SDP attribute to.
 * @param[in] type The type of the stream to add a custom SDP attribute to.
 * @param[in] attribute_name The name of the attribute to add.
 * @param[in] attribute_value The content value of the attribute to add.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_call_params_add_custom_sdp_media_attribute(LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name, const char *attribute_value);

/**
 * Get a custom SDP attribute that is related to all the streams.
 * @param[in] params The #LinphoneCallParams to get the custom SDP attribute from.
 * @param[in] attribute_name The name of the attribute to get.
 * @return The content value of the attribute or NULL if not found.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char * linphone_call_params_get_custom_sdp_attribute(const LinphoneCallParams *params, const char *attribute_name);

/**
 * Get a custom SDP attribute that is related to a specific stream.
 * @param[in] params The #LinphoneCallParams to get the custom SDP attribute from.
 * @param[in] type The type of the stream to add a custom SDP attribute to.
 * @param[in] attribute_name The name of the attribute to get.
 * @return The content value of the attribute or NULL if not found.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char * linphone_call_params_get_custom_sdp_media_attribute(const LinphoneCallParams *params, LinphoneStreamType type, const char *attribute_name);

/**
 * Clear the custom SDP attributes related to all the streams in the SDP exchanged within SIP messages during a call.
 * @param[in] params The #LinphoneCallParams to clear the custom SDP attributes from.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_call_params_clear_custom_sdp_attributes(LinphoneCallParams *params);

/**
 * Clear the custom SDP attributes related to a specific stream in the SDP exchanged within SIP messages during a call.
 * @param[in] params The #LinphoneCallParams to clear the custom SDP attributes from.
 * @param[in] type The type of the stream to clear the custom SDP attributes from.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_call_params_clear_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type);


/*******************************************************************************
 * DEPRECATED                                                                  *
 ******************************************************************************/

/** @deprecated Use linphone_call_params_get_local_conference_mode() instead. */
#define linphone_call_params_local_conference_mode linphone_call_params_get_local_conference_mode

/**
 * Destroy a LinphoneCallParams object.
 * @param[in] cp LinphoneCallParams object
 * @deprecated Use linphone_call_params_unref() instead.
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_call_params_destroy(LinphoneCallParams *cp);

/**
 * @}
**/

#ifdef __cplusplus
}
#endif

#endif /* __LINPHONE_CALL_PARAMS_H__ */
