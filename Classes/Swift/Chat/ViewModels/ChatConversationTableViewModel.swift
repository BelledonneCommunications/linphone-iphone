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
	
	var nbEventDisplayed = MutableLiveData<Int>(20)
	
	override init() {
		super.init()
	}
	
	/*
	func updateData() {
		if (chatRoom == nil) {
			return
		}

		let oneToOne = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		let chatRoomEvents = chatRoom?.getHistoryEvents(nbEvents: 20)
		if(nbEventDisplayed.value != 20){
			nbEventDisplayed.value = 20
		}
		messageListHistory.removeAll()
		chatRoomEvents?.forEach({ eventLog in
			let event = eventLog
			let eventType = event.type
			
			if oneToOne && !eventTypeIsOfInterestForOne(toOneRoom: eventType) {
			} else {
				if let chat = event.chatMessage	{
					//messageListHistory.append(chat)
					messageListHistory.insert(chat, at: 0)
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
	
	func addData() {
		if (chatRoom == nil) {
			return
		}

		let oneToOne = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		let chatRoomEvents = chatRoom?.getHistoryRangeEvents(begin: nbEventDisplayed.value!, end: nbEventDisplayed.value! + 20)
		chatRoomEvents?.reversed().forEach({ eventLog in
			let event = eventLog
			let eventType = event.type
			
			if oneToOne && !eventTypeIsOfInterestForOne(toOneRoom: eventType) {
			} else {
				if let chat = event.chatMessage	{
					//messageListHistory.insert(chat, at: 0)
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
		if(chatRoomEvents!.count > 0){
			nbEventDisplayed.value! += 20
		}
	}
	*/
	func getMessage(index: Int) -> ChatMessage? {
		if (chatRoom == nil) {
			return nil
		}

		let oneToOne = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		let chatRoomEvents = chatRoom?.getHistoryRangeEvents(begin: index, end: index+1)
		return chatRoomEvents?.first?.chatMessage
	}
    
    func getIndexMessage(message: ChatMessage) -> Int {
        var index = -1
        if (chatRoom == nil) {
            return index
        }
        
        var indexRange = 0
        let msgId = message.messageId
        
        while index == -1 {
            let chatRoomEvents = chatRoom?.getHistoryRangeEvents(begin: indexRange, end: indexRange+20)
            if chatRoomEvents?.count == 0 {
                index = -2
            }
            chatRoomEvents?.reversed().forEach({ event in
                let chat = event.chatMessage
                if (chat != nil && msgId == chat?.messageId) {
                    index = indexRange ;
                }
                indexRange += 1
            })
        }
        return index
    }
	
	func getNBMessages() -> Int {
		if (chatRoom == nil) {
			return 0
		}
		return chatRoom!.historySize
	}
	
	func eventTypeIsOfInterestForOne(toOneRoom type: EventLogType) -> Bool {
		return type.rawValue == LinphoneEventLogTypeConferenceChatMessage.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageEnabled.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageDisabled.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageLifetimeChanged.rawValue
	}
}
