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
	
	public class func getChatIconState(chatState: Int) -> String {
		return switch chatState {
		case ChatMessage.State.Displayed.rawValue, ChatMessage.State.FileTransferDone.rawValue:
			"checks"
		case ChatMessage.State.DeliveredToUser.rawValue:
			"check"
		case ChatMessage.State.Delivered.rawValue:
			"envelope-simple"
		case ChatMessage.State.NotDelivered.rawValue, ChatMessage.State.FileTransferError.rawValue:
			"warning-circle"
		case ChatMessage.State.InProgress.rawValue, ChatMessage.State.FileTransferInProgress.rawValue:
			"animated-in-progress"
		default:
			"animated-in-progress"
		}
	}
	
	public class func getChatRoomId(room: ChatRoom) -> String {
		return getChatRoomId(localAddress: room.localAddress!, remoteAddress: room.peerAddress!)
	}
	
	public class func getChatRoomId(localAddress: Address, remoteAddress: Address) -> String {
		let localSipUri = localAddress.clone()
		localSipUri!.clean()
		let remoteSipUri = remoteAddress.clone()
		remoteSipUri!.clean()
		return getChatRoomId(localSipUri: localSipUri!.asStringUriOnly(), remoteSipUri: remoteSipUri!.asStringUriOnly())
	}
	
	public class func getChatRoomId(localSipUri: String, remoteSipUri: String) -> String {
		return "\(localSipUri)#~#\(remoteSipUri)"
	}
	
	public class func applyInternationalPrefix(core: Core, account: Account? = nil) -> Bool {
		return	account?.params?.useInternationalPrefixForCallsAndChats == true 
		|| core.defaultAccount?.params?.useInternationalPrefixForCallsAndChats == true
	}
	
	public class func isEndToEndEncryptedChatAvailable(core: Core) -> Bool {
		return core.limeX3DhEnabled &&
		core.defaultAccount?.params?.limeServerUrl != nil &&
		core.defaultAccount?.params?.conferenceFactoryUri != nil
	}
}
