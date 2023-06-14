/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

import UIKit
import Foundation
import linphonesw


class ChatConversationTableViewModel {
	
	static let sharedModel = ChatConversationTableViewModel()
 
	var chatRoom: ChatRoom? = nil
	
    var refreshIndexPath = MutableLiveData<Int>(0)
	
	var onClickIndexPath = MutableLiveData<Int>(0)
	var onClickMessageIndexPath = 0
	
	
	var editModeOn = MutableLiveData<Bool>(false)
	
	var messageSelected = MutableLiveData<Int>(0)
	var messageListSelected = MutableLiveData<[Bool]>([])
	
	var messageListToDelete : [EventLog] = []

	func getMessage(index: Int) -> EventLog? {
		if (chatRoom != nil) {
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
                    index = indexRange
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
		return chatRoom!.historyEventsSize
	}
	
	func eventTypeIsOfInterestForOne(toOneRoom type: EventLogType) -> Bool {
		return type.rawValue == LinphoneEventLogTypeConferenceChatMessage.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageEnabled.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageDisabled.rawValue || type.rawValue == LinphoneEventLogTypeConferenceEphemeralMessageLifetimeChanged.rawValue
	}
    
    func reloadCollectionViewCell(){
        refreshIndexPath.value! += 1
    }
	
	func onGridClick(indexMessage: Int, index :Int){
		onClickMessageIndexPath = indexMessage
		onClickIndexPath.value! = index
	}
	
	func changeEditMode(editMode :Bool){
		editModeOn.value = editMode
	}
	
	func selectAllMessages(){
		for i in 0...messageListSelected.value!.count - 1 {
			messageListSelected.value![i] = true
			messageSelected.value! += 1
		}
		refreshIndexPath.value! += 1
	}
	
	func unSelectAllMessages(){
		for i in 0...messageListSelected.value!.count - 1 {
			messageListSelected.value![i] = false
		}
		messageSelected.value! = 0
		refreshIndexPath.value! += 1
	}
	
	func deleteMessagesSelected(){
		for i in 0...(messageListSelected.value!.count - 1) {
			if messageListSelected.value![i] == true {
				let messageEvent = getMessage(index: i)
				//if messageEvent
				messageListToDelete.append((messageEvent)!)
			}
		}

		messageListToDelete.forEach { chatMessage in
			chatMessage.deleteFromDatabase()
		}
		messageSelected.value! = 0
		refreshIndexPath.value! += 1
	}
}
