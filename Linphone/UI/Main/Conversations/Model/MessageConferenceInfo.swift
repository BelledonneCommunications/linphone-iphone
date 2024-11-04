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

import Foundation

public enum MessageConferenceState: Codable {
	case new
	case updated
	case cancelled
}

public struct MessageConferenceInfo: Codable, Identifiable, Hashable {
	public let id: UUID
	public let meetingConferenceUri: String
	public let meetingSubject: String
	public let meetingDescription: String
	public let meetingState: MessageConferenceState
	public let meetingDate: String
	public let meetingTime: String
	public let meetingDay: String
	public let meetingDayNumber: String
	public let meetingParticipants: String

	public init(id: UUID, meetingConferenceUri: String, meetingSubject: String, meetingDescription: String, meetingState: MessageConferenceState, meetingDate: String, meetingTime: String, meetingDay: String, meetingDayNumber: String, meetingParticipants: String) {
		self.id = id
		self.meetingConferenceUri = meetingConferenceUri
		self.meetingSubject = meetingSubject
		self.meetingDescription = meetingDescription
		self.meetingState = meetingState
		self.meetingDate = meetingDate
		self.meetingTime = meetingTime
		self.meetingDay = meetingDay
		self.meetingDayNumber = meetingDayNumber
		self.meetingParticipants = meetingParticipants
	}
}
