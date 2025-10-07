/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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

import SwiftUI

struct ToastView: View {
	
	@ObservedObject private var toastViewModel = ToastViewModel.shared
	
	var body: some View {
		VStack {
			if toastViewModel.displayToast {
				HStack {
					if toastViewModel.toastMessage.contains("toast_call_transfer") {
						Image("phone-transfer")
							.resizable()
							.renderingMode(.template)
							.frame(width: 25, height: 25, alignment: .leading)
							.foregroundStyle(toastViewModel.toastMessage.contains("Success") ? Color.greenSuccess500 : Color.redDanger500)
					} else if toastViewModel.toastMessage.contains("is recording") {
						Image("record-fill")
							.resizable()
							.renderingMode(.template)
							.frame(width: 25, height: 25, alignment: .leading)
							.foregroundStyle(Color.redDanger500)
					} else if toastViewModel.toastMessage.contains("Info_") {
						Image("trusted")
							.resizable()
							.frame(width: 25, height: 25, alignment: .leading)
					} else {
						Image(toastViewModel.toastMessage.contains("Success") ? "check" : "warning-circle")
							.resizable()
							.renderingMode(.template)
							.frame(width: 25, height: 25, alignment: .leading)
							.foregroundStyle(toastViewModel.toastMessage.contains("Success") ? Color.greenSuccess500 : Color.redDanger500)
					}
					
					switch toastViewModel.toastMessage {
					case "Success_qr_code_validated":
						Text("qr_code_validated")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_version_up_to_date":
						Text("help_version_up_to_date_toast_message")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_remove_call_logs":
						Text("call_history_deleted_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_clear_logs":
						Text("help_troubleshooting_debug_logs_cleaned_toast_message")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_send_logs":
						Text("debug_logs_copied_to_clipboard_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_address_copied_into_clipboard":
						Text("sip_address_copied_to_clipboard_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_message_copied_into_clipboard":
						Text("message_copied_to_clipboard_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_text_copied_into_clipboard":
						Text("text_copied_to_clipboard_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Info_call_securised":
						Text("call_can_be_trusted_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.blueInfo500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case let str where str.contains("is recording"):
						Text(toastViewModel.toastMessage)
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed":
						Text("assistant_qr_code_invalid_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Invalide URI":
						Text("assistant_invalid_uri_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Registration_failed":
						Text("assistant_account_login_forbidden_error")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Unavailable_network":
						Text("network_not_reachable")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_network_connected":
						Text("network_reachable_again")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_call_transfer_successful":
						Text("call_transfer_successful_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_call_transfer_in_progress":
						Text("call_transfer_in_progress_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_meeting_deleted":
						Text("meeting_info_deleted_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_meeting_info_created_toast":
						Text("meeting_info_created_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_meeting_info_updated_toast":
						Text("meeting_info_updated_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_toast_call_transfer_failed":
						Text("call_transfer_failed_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
			
					case "Failed_uri_handler_call_failed":
						Text("uri_handler_call_failed_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_uri_handler_config_failed":
						Text("uri_handler_config_failed_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "uri_handler_config_success":
						Text("uri_handler_config_success_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
			
					case "Failed_uri_handler_bad_call_address":
						Text("uri_handler_bad_call_address_failed_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_uri_handler_bad_config_address":
						Text("uri_handler_bad_config_address_failed_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_push_notification_not_received_error":
						Text("assistant_account_register_push_notification_not_received_error")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_account_register_unexpected_error":
						Text("assistant_account_register_unexpected_error")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case let str where str.contains("Error: "):
						Text(toastViewModel.toastMessage)
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_to_create_group_call_error":
						Text("conference_failed_to_create_group_call_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_to_create_conversation_error":
						Text("conversation_failed_to_create_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_to_create_conversation_invalid_participant_error":
						Text("conversation_invalid_participant_due_to_security_mode_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
						
					case "Meeting_added_to_calendar":
						Text("meeting_exported_as_calendar_event")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "meeting_failed_to_edit_toast":
						Text("meeting_failed_to_edit_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "meeting_failed_to_schedule_toast":
						Text("meeting_failed_to_schedule_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "meeting_failed_to_send_invites_toast":
						Text("meeting_failed_to_send_invites_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "meeting_failed_to_send_part_of_invites_toast":
						Text("meeting_failed_to_send_part_of_invites_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_no_subject_or_participant":
						Text("meeting_schedule_failed_no_subject_or_participant_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_settings_contacts_carddav_sync_successful_toast":
						Text("settings_contacts_carddav_sync_successful_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "settings_contacts_carddav_sync_error_toast":
						Text("settings_contacts_carddav_sync_error_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_settings_contacts_carddav_deleted_toast":
						Text("settings_contacts_carddav_deleted_toast")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
					
					default:
						Text("Error")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
					}
				}
				.frame(maxWidth: .infinity)
				.background(.white)
				.cornerRadius(50)
				.overlay(
					RoundedRectangle(cornerRadius: 50)
						.inset(by: 0.5)
						.stroke(toastViewModel.toastMessage.contains("Success") 
								? Color.greenSuccess500 : (toastViewModel.toastMessage.contains("Info_")
														   ? Color.blueInfo500 : Color.redDanger500), lineWidth: 1)
				)
				.onTapGesture {
					if !toastViewModel.toastMessage.contains("is recording") {
						withAnimation {
							toastViewModel.toastMessage = ""
							toastViewModel.displayToast = false
						}
					}
				}
				.onAppear {
					if !toastViewModel.toastMessage.contains("is recording") {
						DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
							withAnimation {
								toastViewModel.toastMessage = ""
								toastViewModel.displayToast = false
							}
						}
					}
				}
				
				Spacer()
			}
		}
		.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
		.padding(.horizontal, 16)
		.padding(.bottom, 18)
		.transition(.move(edge: .top))
		.padding(.top, 60)
	}
}
