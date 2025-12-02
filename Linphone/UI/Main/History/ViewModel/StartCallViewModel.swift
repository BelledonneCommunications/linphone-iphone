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

import linphonesw
import Combine
import SwiftUI

// swiftlint:disable line_length
class StartCallViewModel: ObservableObject {
	
	static let TAG = "[StartCallViewModel]"
	
	private var coreContext = CoreContext.shared
	
	@Published var searchField: String = ""
    
    var domain: String = ""
	
	@Published var messageText: String = ""
	
	@Published var participants: [SelectedAddressModel] = []
	
	@Published var operationInProgress: Bool = false
	
	@Published var hideGroupCallButton: Bool = false
	
	private var conferenceDelegate: ConferenceDelegate?
	
	init() {
		coreContext.doOnCoreQueue { core in
            self.domain = core.defaultAccount?.params?.domain ?? ""
			self.updateGroupCallButtonVisibility(core: core)
        }
		
    }
	
	func updateGroupCallButtonVisibility(core: Core) {
		let hideGroupCall = CorePreferences.disableMeetings ||
		!LinphoneUtils.isRemoteConferencingAvailable(core: core) ||
		core.callsNb > 0
		DispatchQueue.main.async {
			self.hideGroupCallButton = hideGroupCall
		}
	}
	
	func addParticipants(participantsToAdd: [SelectedAddressModel]) {
		var list = participants
		for selectedAddr in participantsToAdd {
			if let found = list.first(where: { $0.address.weakEqual(address2: selectedAddr.address) }) {
				Log.info("\(StartCallViewModel.TAG) Participant \(found.address.asStringUriOnly()) already in list, skipping")
				continue
			}
			
			list.append(selectedAddr)
			Log.info("\(StartCallViewModel.TAG) Added participant \(selectedAddr.address.asStringUriOnly())")
		}
		Log.info("\(StartCallViewModel.TAG) [\(list.count - participants.count) participants added, now there are \(list.count) participants in list")
		
		participants = list
	}
	
	func createGroupCall() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account == nil {
				Log.error(
					"\(ConversationModel.TAG) No default account found, can't create group call!"
				)
				return
			}
			
			DispatchQueue.main.async {
				self.operationInProgress = true
			}
			
			do {
				var participantsList: [Address] = []
				self.participants.forEach { participant in
					participantsList.append(participant.address)
				}
				
				DispatchQueue.main.async {
					self.participants.removeAll()
				}
				
				Log.info(
					"\(ConversationModel.TAG) Creating group call with subject \(self.messageText) and \(participantsList.count) participant(s)"
				)
				
				if let conference = LinphoneUtils.createGroupCall(core: core, account: account, subject: self.messageText) {
					let callParams = try? core.createCallParams(call: nil)
					if let callParams = callParams {
						callParams.videoEnabled = true
						callParams.videoDirection = .RecvOnly
						
						Log.info("\(ConversationModel.TAG) Inviting \(participantsList.count) participant(s) into newly created conference")
						
						self.conferenceAddDelegate(core: core, conference: conference)
						
						try conference.inviteParticipants(addresses: participantsList, params: callParams)
						
						DispatchQueue.main.async {
							TelecomManager.shared.participantsInvited = true
						}
					}
				}
			} catch let error {
				Log.error(
					"\(ConversationModel.TAG) createGroupCall: \(error)"
				)
			}
		}
	}
	
	func conferenceAddDelegate(core: Core, conference: Conference) {
		self.conferenceDelegate = ConferenceDelegateStub(onStateChanged: { (conference: Conference, state: Conference.State) in
			Log.info("\(StartCallViewModel.TAG) Conference state is \(state)")
			if state == .Created {
				NotificationCenter.default.post(name: Notification.Name("CallViewModelReset"), object: self)
				DispatchQueue.main.async {
					self.operationInProgress = false
				}
			} else if state == .CreationFailed {
				Log.error("\(StartCallViewModel.TAG) Failed to create group call!")
				DispatchQueue.main.async {
					ToastViewModel.shared.toastMessage = "Failed_to_create_group_call_error"
					ToastViewModel.shared.displayToast = true
					self.operationInProgress = false
				}
			}
		})
		
		if self.conferenceDelegate != nil {
			conference.addDelegate(delegate: self.conferenceDelegate!)
		}
	}
	
	func startVideoCall(core: Core, conferenceAddress: Address) {
		TelecomManager.shared.doCallWithCore(addr: conferenceAddress, isVideo: true, isConference: true)
	}
	
	func interpretAndStartCall() {
		CoreContext.shared.doOnCoreQueue { core in
			let address = core.interpretUrl(url: self.searchField, applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core))
			if address != nil {
				TelecomManager.shared.doCallOrJoinConf(address: address!)
			}
		}
	}
}
// swiftlint:enable line_length
