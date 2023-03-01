//
//  ChatConversationTableViewModel.swift
//  linphone
//
//  Created by Benoît Martins on 23/02/2023.
//

import UIKit
import Foundation
import linphonesw


class ChatConversationTableViewModel: ControlsViewModel {
	
	static let sharedModel = ChatConversationTableViewModel()
	
	var messageListHistory : [ChatMessage] = []
 
	var chatRoom: ChatRoom? = nil
	
	override init() {
		super.init()
	}
	
	func updateData() {
		if (chatRoom == nil) {
			return
		}

		let oneToOne = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		let chatRoomEvents = chatRoom?.getHistoryEvents(nbEvents: 20)
		messageListHistory.removeAll()
		messageListHistory = []
		chatRoomEvents?.forEach({ eventLog in
			let event = eventLog
			let eventType = event.type
			
			if oneToOne && !eventTypeIsOfInterestForOne(toOneRoom: eventType) {
			} else {
				if let chat = event.chatMessage	{
					messageListHistory.append(chat)
				}											//linphone_event_log_get_chat_message(event)
				/*
			 	if auto_download is available and file transfer in progress, not add event now
				if !(autoDownload && chat != nil && linphone_chat_message_is_file_transfer_in_progress(chat)) {
					totalEventList.append(NSValue(pointer: linphone_event_log_ref(event)))
					if listSize <= BASIC_EVENT_LIST {
						eventList.append(NSValue(pointer: linphone_event_log_ref(event)))
					}
				}

				listSize -= 1*/
				
				
			}
		})
	}
	
	func eventTypeIsOfInterestForOne(toOneRoom type: EventLogType) -> Bool {
		return type.rawValue == LinphoneEventLogTypeConferenceChatMessage.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageEnabled.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageDisabled.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageLifetimeChanged.rawValue
	}
}
