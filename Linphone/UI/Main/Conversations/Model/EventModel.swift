/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

import SwiftUI
import linphonesw

class EventModel: ObservableObject {
	@Published var text: String
	@Published var icon: Image?

	var eventLog: EventLog
	var eventLogId: String
	var eventLogType: EventLog.Kind

	init(eventLog: EventLog) {
		self.eventLog = eventLog
		self.eventLogId = eventLog.chatMessage != nil ? eventLog.chatMessage!.messageId : String(eventLog.notifyId)
		self.eventLogType = eventLog.type
		self.text = ""
		self.icon = nil
		setupEventData(eventLog: eventLog)
	}
	
	private func setupEventData(eventLog: EventLog) {
		let address = eventLog.participantAddress ?? eventLog.peerAddress
		if address != nil {
			ContactsManager.shared.getFriendWithAddressInCoreQueue(address: address) { friendResult in
				var name = ""
				if let addressFriend = friendResult {
					name = addressFriend.name!
				} else {
					if address!.displayName != nil {
						name = address!.displayName!
					} else if address!.username != nil {
						name = address!.username!
					} else {
						name = String(address!.asStringUriOnly().dropFirst(4))
					}
				}
				
				let textValue: String
				let iconValue: Image?

				switch eventLog.type {
				case .ConferenceCreated:
					textValue = NSLocalizedString("conversation_event_conference_created", comment: "")
				case .ConferenceTerminated:
					textValue = NSLocalizedString("conversation_event_conference_destroyed", comment: "")
				case .ConferenceParticipantAdded:
					textValue = String(format: NSLocalizedString("conversation_event_participant_added", comment: ""), address != nil ? name : "<?>")
				case .ConferenceParticipantRemoved:
					textValue = String(format: NSLocalizedString("conversation_event_participant_removed", comment: ""), address != nil ? name : "<?>")
				case .ConferenceSubjectChanged:
					textValue = String(format: NSLocalizedString("conversation_event_subject_changed", comment: ""), eventLog.subject ?? "")
				case .ConferenceParticipantSetAdmin:
					textValue = String(format: NSLocalizedString("conversation_event_admin_set", comment: ""), address != nil ? name : "<?>")
				case .ConferenceParticipantUnsetAdmin:
					textValue = String(format: NSLocalizedString("conversation_event_admin_unset", comment: ""), address != nil ? name : "<?>")
				case .ConferenceParticipantDeviceAdded:
					textValue = String(format: NSLocalizedString("conversation_event_device_added", comment: ""), address != nil ? name : "<?>")
				case .ConferenceParticipantDeviceRemoved:
					textValue = String(format: NSLocalizedString("conversation_event_device_removed", comment: ""), address != nil ? name : "<?>")
				case .ConferenceEphemeralMessageEnabled:
					textValue = NSLocalizedString("conversation_event_ephemeral_messages_enabled", comment: "")
				case .ConferenceEphemeralMessageDisabled:
					textValue = NSLocalizedString("conversation_event_ephemeral_messages_disabled", comment: "")
				case .ConferenceEphemeralMessageLifetimeChanged:
					textValue = String(format: NSLocalizedString("conversation_event_ephemeral_messages_lifetime_changed", comment: ""),
									   self.formatEphemeralExpiration(duration: Int64(eventLog.ephemeralMessageLifetime)).lowercased())
				default:
					textValue = String(eventLog.type.rawValue)
				}

				// Icon assignment
				switch eventLog.type {
				case .ConferenceEphemeralMessageEnabled, .ConferenceEphemeralMessageDisabled, .ConferenceEphemeralMessageLifetimeChanged:
					iconValue = Image("clock-countdown")
				case .ConferenceTerminated:
					iconValue = Image("warning-circle")
				case .ConferenceSubjectChanged:
					iconValue = Image("pencil-simple")
				case .ConferenceParticipantAdded, .ConferenceParticipantRemoved, .ConferenceParticipantDeviceAdded, .ConferenceParticipantDeviceRemoved:
					iconValue = Image("door")
				default:
					iconValue = Image("user-circle")
				}

				DispatchQueue.main.async {
					self.text = textValue
					self.icon = iconValue
				}
			}
		}
	}

	private func formatEphemeralExpiration(duration: Int64) -> String {
		switch duration {
		case 0:
			return NSLocalizedString("conversation_ephemeral_messages_duration_disabled", comment: "")
		case 60:
			return NSLocalizedString("conversation_ephemeral_messages_duration_one_minute", comment: "")
		case 3600:
			return NSLocalizedString("conversation_ephemeral_messages_duration_one_hour", comment: "")
		case 86400:
			return NSLocalizedString("conversation_ephemeral_messages_duration_one_day", comment: "")
		case 259200:
			return NSLocalizedString("conversation_ephemeral_messages_duration_three_days", comment: "")
		case 604800:
			return NSLocalizedString("conversation_ephemeral_messages_duration_one_week", comment: "")
		default:
			return "\(duration) s"
		}
	}
}
