/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

import Foundation
import UIKit

@objc class VoipTexts : NSObject { // From android key names. Added intentionnally with NSLocalizedString calls for each key, so it can be picked up by translation system (Weblate or Xcode).
	
	static let appName = Bundle.main.object(forInfoDictionaryKey: "CFBundleDisplayName") as! String
	static let me = NSLocalizedString("Me",comment:"")

	// Calls
	static let call_incoming_title = NSLocalizedString("Incoming Call",comment:"")
	static let call_outgoing_title = NSLocalizedString("Outgoing Call",comment:"")
	static let call_notification_paused = NSLocalizedString("Paused call",comment:"")
	static let call_notification_outgoing = NSLocalizedString("Outgoing call",comment:"")
	static let call_notification_active = NSLocalizedString("Call running",comment:"")
	static let call_error_declined = NSLocalizedString("Call has been declined",comment:"")
	static let call_error_user_busy = NSLocalizedString("User is busy",comment:"")
	static let call_error_user_not_found = NSLocalizedString("User hasn't been found",comment:"")
	static let call_error_incompatible_media_params = NSLocalizedString("Incompatible media parameters",comment:"")
	static let call_error_network_unreachable = NSLocalizedString("Network is unreachable",comment:"")
	static let call_error_io_error = NSLocalizedString("Service unavailable or network error",comment:"")
	static let call_error_server_timeout = NSLocalizedString("Server timeout",comment:"")
	static let call_error_temporarily_unavailable = NSLocalizedString("Temporarily unavailable",comment:"")
	static let call_error_generic = NSLocalizedString("Error: %s",comment:"")
	static let call_video_update_requested_dialog = NSLocalizedString("Correspondent would like to turn the video on",comment:"")
	@objc static let call_action_participants_list = NSLocalizedString("Participants list",comment:"")
	static let call_action_chat = NSLocalizedString("Chat",comment:"")
	static let call_action_calls_list = NSLocalizedString("Calls list",comment:"")
	static let call_action_numpad = NSLocalizedString("Numpad",comment:"")
	static let call_action_change_conf_layout = NSLocalizedString("Change layout",comment:"")
	static let call_action_transfer_call = NSLocalizedString("Transfer call",comment:"")
	static let call_action_add_call = NSLocalizedString("Start new call",comment:"")

	static let call_action_statistics = NSLocalizedString("Call statistics",comment:"")
	static let call_context_action_resume = NSLocalizedString("Resume call",comment:"")
	static let call_context_action_pause = NSLocalizedString("Pause call",comment:"")
	static let call_context_action_transfer = NSLocalizedString("Transfer call",comment:"")
	static let call_context_action_answer = NSLocalizedString("Answer call",comment:"")
	static let call_context_action_hangup = NSLocalizedString("Terminate call",comment:"")
	static let call_remote_recording = NSLocalizedString("This call is being recorded.",comment:"")
	static let call_remotely_paused_title = NSLocalizedString("Call has been paused by remote.",comment:"")
	
	// Conference
	static let conference_schedule_title = NSLocalizedString("Start a conference",comment:"")
	static let conference_schedule_later = NSLocalizedString("Do you want to schedule this conference for later?",comment:"")
	static let conference_schedule_mandatory_field = NSLocalizedString("Mandatory",comment:"")
	static let conference_schedule_subject_title = NSLocalizedString("Subject",comment:"")
	static let conference_schedule_subject_hint = NSLocalizedString("Conference subject",comment:"")
	static let conference_schedule_address_title = NSLocalizedString("Conference address",comment:"")
	static let conference_schedule_description_title = NSLocalizedString("Add a description",comment:"")
	static let conference_schedule_description_hint = NSLocalizedString("Description",comment:"")
	static let conference_schedule_date = NSLocalizedString("Date",comment:"")
	static let conference_schedule_time = NSLocalizedString("Time",comment:"")
	static let conference_schedule_duration = NSLocalizedString("Duration",comment:"")
	static let conference_schedule_timezone = NSLocalizedString("Timezone",comment:"")
	static let conference_schedule_send_invite_chat = NSLocalizedString("Send invite via \(appName)",comment:"")
	static let conference_schedule_send_invite_email = NSLocalizedString("Send invite via email",comment:"")
	static let conference_schedule_encryption = NSLocalizedString("Would you like to encrypt the conference?",comment:"")
	static let conference_schedule_send_invite_chat_summary = NSLocalizedString("Invite will be sent out from my \(appName) account",comment:"")
	static let conference_schedule_participants_list = NSLocalizedString("Participants list",comment:"")
	static let conference_schedule_summary = NSLocalizedString("Conference info",comment:"")
	static let conference_schedule_create = NSLocalizedString("Create conference",comment:"")
	static let conference_schedule = NSLocalizedString("Schedule conference",comment:"")
	static let conference_schedule_address_copied_to_clipboard = NSLocalizedString("Conference address copied into clipboard",comment:"")
	static let conference_schedule_creation_failure = NSLocalizedString("Failed to create conference!",comment:"")
	static let conference_schedule_info_not_sent_to_participant = NSLocalizedString("Failed to send conference info to a participant",comment:"")
	static let conference_paused_title = NSLocalizedString("You are currently out of the conference.",comment:"")
	static let conference_paused_subtitle = NSLocalizedString("Click on play button to join it back.",comment:"")
	static let conference_default_title = NSLocalizedString("Remote conference",comment:"")
	static let conference_local_title = NSLocalizedString("Local conference",comment:"")
	static let conference_invite_title = NSLocalizedString("Conference invite:",comment:"")
	static let conference_description_title = NSLocalizedString("Description:",comment:"")
	static let conference_invite_join = NSLocalizedString("Join",comment:"")
	static let conference_invite_participants_count = NSLocalizedString("%d participants",comment:"")
	static let conference_display_mode_mosaic = NSLocalizedString("Mosaic mode",comment:"")
	static let conference_display_mode_active_speaker = NSLocalizedString("Active speaker mode",comment:"")
	static let conference_display_no_active_speaker = NSLocalizedString("No active speaker",comment:"")
	static let conference_waiting_room_start_call = NSLocalizedString("Start",comment:"")
	static let conference_waiting_room_cancel_call = NSLocalizedString("Cancel",comment:"")
	static let conference_scheduled = NSLocalizedString("Conferences",comment:"")
	static let conference_too_many_participants_for_mosaic_layout = NSLocalizedString("You can't change conference layout as there is too many participants",comment:"")
	static let conference_participant_paused = NSLocalizedString("(paused)",comment:"")
	static let conference_no_schedule = NSLocalizedString("No scheduled conference yet.",comment:"")
	static let conference_schedule_organizer = NSLocalizedString("Organizer:",comment:"")
	static let conference_go_to_chat = NSLocalizedString("Conference's chat room",comment:"")
	static let conference_creation_failed = NSLocalizedString("Failed to create conference",comment:"")

	
	// Call Stats
	
	static let call_stats_audio = "Audio"
	static let call_stats_video = "Video"
	static let call_stats_codec = "Codec:"
	static let call_stats_ip = "IP Family:"
	static let call_stats_upload = "Upload bandwidth:"
	static let call_stats_download = "Download bandwidth:"
	static let call_stats_estimated_download = "Estimated download bandwidth:"
	static let call_stats_ice = "ICE connectivity:"
	static let call_stats_video_resolution_sent = "Sent video resolution:"
	static let call_stats_video_resolution_received = "Received video resolution:"
	static let call_stats_video_fps_sent = "Sent video fps:"
	static let call_stats_video_fps_received = "Received video fps:"
	static let call_stats_sender_loss_rate = "Sender loss rate:"
	static let call_stats_receiver_loss_rate = "Receiver loss rate:"
	static let call_stats_jitter_buffer = "Jitter buffer:"
	static let call_stats_encoder_name = "Encoder:"
	static let call_stats_decoder_name = "Decoder:"
	static let call_stats_player_filter = "Player filter:"
	static let call_stats_capture_filter = "Capture filter:"
	
	
	// Added in iOS
	static let camera_required_for_video = NSLocalizedString("Camera use is not Authorized for \(appName). This permission is required to activate Video.",comment:"")
	static let ok =  NSLocalizedString("ok",comment:"")
	static let cancel =  NSLocalizedString("cancel",comment:"")
	
	static let dialog_accept =  NSLocalizedString("Accept",comment:"")
	static let dialog_decline =  NSLocalizedString("Decline",comment:"")

	// Participants list :
	static let chat_room_group_info_admin = NSLocalizedString("Admin",comment:"")
	
	
}
