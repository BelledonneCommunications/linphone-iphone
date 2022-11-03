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
	
	// FROM ANDROID START (check script scripts/android_import.sh for details)
	@objc static let call_action_add_call = NSLocalizedString("Start new call",comment:"")
	@objc static let call_action_calls_list = NSLocalizedString("Calls list",comment:"")
	@objc static let call_action_change_conf_layout = NSLocalizedString("Change layout",comment:"")
	@objc static let call_action_chat = NSLocalizedString("Chat",comment:"")
	@objc static let call_action_numpad = NSLocalizedString("Numpad",comment:"")
	@objc static let call_action_participants_list = NSLocalizedString("Participants list",comment:"")
	@objc static let call_action_statistics = NSLocalizedString("Call statistics",comment:"")
	@objc static let call_action_transfer_call = NSLocalizedString("Transfer call",comment:"")
	@objc static let call_context_action_answer = NSLocalizedString("Answer call",comment:"")
	@objc static let call_context_action_hangup = NSLocalizedString("Terminate call",comment:"")
	@objc static let call_context_action_pause = NSLocalizedString("Pause call",comment:"")
	@objc static let call_context_action_resume = NSLocalizedString("Resume call",comment:"")
	@objc static let call_context_action_transfer = NSLocalizedString("Transfer call",comment:"")
	@objc static let call_error_declined = NSLocalizedString("Call has been declined",comment:"")
	@objc static let call_error_generic = NSLocalizedString("Error: %s",comment:"")
	@objc static let call_error_incompatible_media_params = NSLocalizedString("Incompatible media parameters",comment:"")
	@objc static let call_error_io_error = NSLocalizedString("Service unavailable or network error",comment:"")
	@objc static let call_error_network_unreachable = NSLocalizedString("Network is unreachable",comment:"")
	@objc static let call_error_server_timeout = NSLocalizedString("Server timeout",comment:"")
	@objc static let call_error_temporarily_unavailable = NSLocalizedString("Temporarily unavailable",comment:"")
	@objc static let call_error_user_busy = NSLocalizedString("User is busy",comment:"")
	@objc static let call_error_user_not_found = NSLocalizedString("User hasn't been found",comment:"")
	@objc static let call_incoming_title = NSLocalizedString("Incoming Call",comment:"")
	@objc static let call_locally_paused_subtitle = NSLocalizedString("Click on play button to resume it.",comment:"")
	@objc static let call_locally_paused_title = NSLocalizedString("You have paused the call.",comment:"")
	@objc static let call_notification_active = NSLocalizedString("Call running",comment:"")
	@objc static let call_notification_outgoing = NSLocalizedString("Outgoing call",comment:"")
	@objc static let call_notification_paused = NSLocalizedString("Paused call",comment:"")
	@objc static let call_outgoing_title = NSLocalizedString("Outgoing Call",comment:"")
	@objc static let call_remote_recording = NSLocalizedString("This call is being recorded.",comment:"")
	@objc static let call_remotely_paused_title = NSLocalizedString("Call has been paused by remote.",comment:"")
	@objc static let call_stats_audio = NSLocalizedString("Audio",comment:"")
	@objc static let call_stats_capture_filter = NSLocalizedString("Capture filter:",comment:"")
	@objc static let call_stats_codec = NSLocalizedString("Codec:",comment:"")
	@objc static let call_stats_decoder_name = NSLocalizedString("Decoder:",comment:"")
	@objc static let call_stats_download = NSLocalizedString("Download bandwidth:",comment:"")
	@objc static let call_stats_encoder_name = NSLocalizedString("Encoder:",comment:"")
	@objc static let call_stats_estimated_download = NSLocalizedString("Estimated download bandwidth:",comment:"")
	@objc static let call_stats_ice = NSLocalizedString("ICE connectivity:",comment:"")
	@objc static let call_stats_ip = NSLocalizedString("IP Family:",comment:"")
	@objc static let call_stats_jitter_buffer = NSLocalizedString("Jitter buffer:",comment:"")
	@objc static let call_stats_player_filter = NSLocalizedString("Player filter:",comment:"")
	@objc static let call_stats_receiver_loss_rate = NSLocalizedString("Receiver loss rate:",comment:"")
	@objc static let call_stats_sender_loss_rate = NSLocalizedString("Sender loss rate:",comment:"")
	@objc static let call_stats_upload = NSLocalizedString("Upload bandwidth:",comment:"")
	@objc static let call_stats_video = NSLocalizedString("Video",comment:"")
	@objc static let call_stats_video_fps_received = NSLocalizedString("Received video fps:",comment:"")
	@objc static let call_stats_video_fps_sent = NSLocalizedString("Sent video fps:",comment:"")
	@objc static let call_stats_video_resolution_received = NSLocalizedString("Received video resolution:",comment:"")
	@objc static let call_stats_video_resolution_sent = NSLocalizedString("Sent video resolution:",comment:"")
	@objc static let call_video_update_requested_dialog = NSLocalizedString("Correspondent would like to turn the video on",comment:"")
	@objc static let cancel = NSLocalizedString("Cancel",comment:"")
	@objc static let chat_room_group_info_admin = NSLocalizedString("Admin",comment:"")
	@objc static let conference_creation_failed = NSLocalizedString("Failed to create meeting",comment:"")
	@objc static let conference_default_title = NSLocalizedString("Remote group call",comment:"")
	@objc static let conference_description_title = NSLocalizedString("Description",comment:"")
	@objc static let conference_display_mode_active_speaker = NSLocalizedString("Active speaker mode",comment:"")
	@objc static let conference_display_mode_audio_only = NSLocalizedString("Audio only mode",comment:"")
	@objc static let conference_display_mode_mosaic = NSLocalizedString("Mosaic mode",comment:"")
	@objc static let conference_first_to_join = NSLocalizedString("You're the first to join the group call",comment:"")
	@objc static let conference_go_to_chat = NSLocalizedString("Meeting's chat room",comment:"")
	@objc static let conference_group_call_create = NSLocalizedString("Start group call",comment:"")
	@objc static let conference_group_call_title = NSLocalizedString("Start a group call",comment:"")
	@objc static let conference_incoming_title = NSLocalizedString("Incoming group call",comment:"")
	@objc static let conference_info_confirm_removal = NSLocalizedString("Do you really want to delete this meeting?",comment:"")
	@objc static let conference_infos_confirm_removal = NSLocalizedString("Do you really want to delete these meetings?",comment:"")
	@objc static let conference_info_removed = NSLocalizedString("Meeting info has been deleted",comment:"")
	@objc static let conference_infos_removed = NSLocalizedString("Meeting infos have been deleted",comment:"")

	@objc static let conference_invite_join = NSLocalizedString("Join",comment:"")
	@objc static let conference_invite_participants_count = NSLocalizedString("%d participants",comment:"")
	@objc static let conference_invite_title = NSLocalizedString("Meeting invite:",comment:"")
	@objc static let conference_update_title = NSLocalizedString("Meeting has been updated:",comment:"")
	@objc static let conference_cancel_title = NSLocalizedString("Meeting has been cancelled:",comment:"")
	@objc static let conference_last_user = NSLocalizedString("All other participants have left the group call",comment:"")
	@objc static let conference_local_title = NSLocalizedString("Local group call",comment:"")
	@objc static let conference_no_schedule = NSLocalizedString("No scheduled meeting yet.",comment:"")
	@objc static let conference_no_terminated_schedule = NSLocalizedString("No terminated meeting yet.",comment:"")	
	@objc static let conference_participant_paused = NSLocalizedString("(paused)",comment:"")
	@objc static let conference_participants_title = NSLocalizedString("Participants (%d)",comment:"")
	@objc static let conference_paused_subtitle = NSLocalizedString("Click on play button to join it back.",comment:"")
	@objc static let conference_paused_title = NSLocalizedString("You are currently out of the meeting.",comment:"")
	@objc static let conference_schedule_address_copied_to_clipboard = NSLocalizedString("Meeting address copied into clipboard",comment:"")
	@objc static let conference_schedule_address_title = NSLocalizedString("Meeting address",comment:"")
	@objc static let conference_schedule_date = NSLocalizedString("Date",comment:"")
	@objc static let conference_schedule_description_hint = NSLocalizedString("Description",comment:"")
	@objc static let conference_schedule_description_title = NSLocalizedString("Add a description",comment:"")
	@objc static let conference_schedule_duration = NSLocalizedString("Duration",comment:"")
	@objc static let conference_schedule_encryption = NSLocalizedString("Would you like to encrypt the meeting?",comment:"")
	@objc static let conference_schedule_info_created = NSLocalizedString("Meeting has been scheduled",comment:"")
	@objc static let conference_schedule_info_not_sent_to_participant = NSLocalizedString("Failed to send meeting info to a participant",comment:"")
	@objc static let conference_schedule_later = NSLocalizedString("Do you want to schedule a meeting for later?",comment:"")
	@objc static let conference_schedule_mandatory_field = NSLocalizedString("Mandatory",comment:"")
	@objc static let conference_schedule_organizer = NSLocalizedString("Organizer:",comment:"")
	@objc static let conference_schedule_participants_list = NSLocalizedString("Participants list",comment:"")
	@objc static let conference_schedule_send_invite_chat = NSLocalizedString("Send invite via &appName;",comment:"").replacingOccurrences(of: "&appName;", with: appName)
	@objc static let conference_schedule_send_invite_chat_summary = NSLocalizedString("Invite will be sent out from my &appName; account",comment:"").replacingOccurrences(of: "&appName;", with: appName)
	@objc static let conference_schedule_send_invite_email = NSLocalizedString("Send invite via email",comment:"")
	@objc static let conference_schedule_start = NSLocalizedString("Schedule meeting",comment:"")
	@objc static let conference_schedule_edit = NSLocalizedString("Edit meeting",comment:"")
	@objc static let conference_schedule_subject_hint = NSLocalizedString("Meeting subject",comment:"")
	@objc static let conference_group_call_subject_hint = NSLocalizedString("Group call subject",comment:"")
	@objc static let conference_schedule_subject_title = NSLocalizedString("Subject",comment:"")
	@objc static let conference_schedule_summary = NSLocalizedString("Meeting info",comment:"")
	@objc static let conference_schedule_time = NSLocalizedString("Time",comment:"")
	@objc static let conference_schedule_timezone = NSLocalizedString("Timezone",comment:"")
	@objc static let conference_schedule_title = NSLocalizedString("Schedule a meeting",comment:"")
	@objc static let conference_scheduled = NSLocalizedString("Meetings",comment:"")
	@objc static let conference_start_group_call_dialog_message = NSLocalizedString("Do you want to start a group call?\nEveryone in this group will receive a call to join the meeting.",comment:"")
	@objc static let conference_start_group_call_dialog_ok_button = NSLocalizedString("Start",comment:"")
	@objc static let conference_too_many_participants_for_mosaic_layout = NSLocalizedString("There is too many participants for mosaic layout, switching to active speaker",comment:"")
	@objc static let conference_waiting_room_cancel_call = NSLocalizedString("Cancel",comment:"")
	@objc static let conference_waiting_room_start_call = NSLocalizedString("Start",comment:"")
	@objc static let conference_waiting_room_video_disabled = NSLocalizedString("Video is currently disabled",comment:"")
	@objc static let dialog_accept = NSLocalizedString("Accept",comment:"")
	@objc static let dialog_decline = NSLocalizedString("Decline",comment:"")
	@objc static let conference_empty = NSLocalizedString("You are currently alone in this group call",comment:"")
	@objc static let conference_admin_set = NSLocalizedString("%s is now admin",comment:"")
	@objc static let conference_admin_unset = NSLocalizedString("%s is no longer admin",comment:"")
	@objc static let conference_scheduled_terminated_filter = NSLocalizedString("Terminated",comment:"")
	@objc static let conference_scheduled_future_filter = NSLocalizedString("Scheduled",comment:"")
	@objc static let conference_scheduled_cancelled_by_me = NSLocalizedString("You have cancelled the conference",comment:"")
	@objc static let conference_scheduled_cancelled_by_organizer = NSLocalizedString("Conference has been cancelled by organizer",comment:"")

	// FROM ANDROID END
	
	
	// Added in iOS
	static let camera_required_for_video = NSLocalizedString("Camera use is not Authorized for &appName;. This permission is required to activate Video.",comment:"").replacingOccurrences(of: "&appName;", with: appName)
	static let conference_edit_error = NSLocalizedString("Unable to edit conference this time, date is invalid",comment:"")
	static let ok =  NSLocalizedString("ok",comment:"")
	static let conference_info_confirm_removal_delete = NSLocalizedString("DELETE",comment:"")
	static let conference_unable_to_share_via_calendar = NSLocalizedString("Unable to add event to calendar. Check permissions",comment:"")
}
