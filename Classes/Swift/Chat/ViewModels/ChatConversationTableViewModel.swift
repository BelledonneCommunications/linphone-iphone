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

	func getMessage(index: Int) -> EventLog? {
		if (chatRoom != nil) {
			let oneToOne = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
			let chatRoomEvents = chatRoom?.getHistoryRangeEvents(begin: index, end: index+1)
			return chatRoomEvents?.first
		}else{
			return nil
		}
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
