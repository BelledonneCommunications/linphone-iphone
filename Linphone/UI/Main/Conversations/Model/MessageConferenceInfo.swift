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

public enum MessageConferenceState {
	case updated
	case cancelled
}

public struct MessageConferenceInfo {
	public let id: String
	var uri: URL
	var subject: String
	var description: String
	var state: MessageConferenceState
	var dateTime: String
	//var duration: time_t
	//var participantInfos: [ParticipantInfo]

	public init(id: String, uri: URL, subject: String, description: String, state: MessageConferenceState, dateTime: String) {
		self.id = id
		self.uri = uri
		self.subject = subject
		self.description = description
		self.state = state
		self.dateTime = dateTime
	}
}
