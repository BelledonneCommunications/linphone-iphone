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
					case "Successful":
						Text("QR code validated!")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_remove_call_logs":
						Text("History has been deleted")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_clear_logs":
						Text("Logs cleared")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_send_logs":
						Text("Logs URL copied into clipboard")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_address_copied_into_clipboard":
						Text("SIP address copied into clipboard")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_message_copied_into_clipboard":
						Text("Message copied into clipboard")
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
						Text("Invalid QR code!")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Invalide URI":
						Text("Invalide URI")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Registration_failed":
						Text("The user name or password is incorrects")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Unavailable_network":
						Text("Network is not reachable")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_network_connected":
						Text("Network is now reachable again")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_account_logged_out":
						Text("Account successfully logged out")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_call_transfer_successful":
						Text("Call has been successfully transferred")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_call_transfer_in_progress":
						Text("Call is being transferred")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_toast_meeting_deleted":
						Text("Successfully removed meeting")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_toast_call_transfer_failed":
						Text("Call transfer failed!")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
			
					case "Failed_uri_handler_call_failed":
						Text("Call failed")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_uri_handler_config_failed":
						Text("Configuration failed")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "uri_handler_config_success":
						Text("Configuration successfully applied")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
			
					case "Failed_uri_handler_bad_call_address":
						Text("Unable to call, invalid address")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_uri_handler_bad_config_address":
						Text("Unable to retrieve configuration, invalid address")
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
						Text("Meeting added to iPhone calendar")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_meeting_invitations_not_sent":
						Text("Could not send ICS invitations to meeting to any participant")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed_no_subject_or_participant":
						Text("A subject and at least one participant is required to create a meeting")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
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
