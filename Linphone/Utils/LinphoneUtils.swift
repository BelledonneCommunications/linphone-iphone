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
import linphonesw

class LinphoneUtils: NSObject {
	public class func isChatRoomAGroup(chatRoom: ChatRoom) -> Bool {
		let oneToOne = chatRoom.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue)
		let conference = chatRoom.hasCapability(mask: ChatRoom.Capabilities.Conference.rawValue)
		return !oneToOne && conference
	}
	
	public class func getChatIconState(chatState: ChatMessage.State) -> String {
		return switch chatState {
		case ChatMessage.State.Displayed, ChatMessage.State.FileTransferDone:
			"checks"
		case ChatMessage.State.DeliveredToUser:
			"check"
		case ChatMessage.State.Delivered:
			"envelope-simple"
		case ChatMessage.State.NotDelivered, ChatMessage.State.FileTransferError:
			"warning-circle"
		case ChatMessage.State.InProgress, ChatMessage.State.FileTransferInProgress:
			"animated-in-progress"
		default:
			"animated-in-progress"
		}
	}
}
