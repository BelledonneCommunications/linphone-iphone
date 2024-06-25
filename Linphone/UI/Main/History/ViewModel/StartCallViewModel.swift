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

class StartCallViewModel: ObservableObject {
	
	static let TAG = "[StartCallViewModel]"
	
	private var coreContext = CoreContext.shared
	
	@Published var searchField: String = ""
    
    var domain: String = ""
	
	@Published var messageText: String = ""
	
	@Published var participants: [SelectedAddressModel] = []
	
	@Published var operationInProgress: Bool = false
	
	private var conferenceSuscriptions = Set<AnyCancellable?>()
	
	init() {
		coreContext.doOnCoreQueue { core in
            self.domain = core.defaultAccount?.params?.domain ?? ""
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
					"\(StartCallViewModel.TAG) No default account found, can't create group call!"
				)
				return
			}
			
			DispatchQueue.main.async {
				self.operationInProgress = true
			}
			
			do {
				let conferenceInfo = try Factory.Instance.createConferenceInfo()
				conferenceInfo.organizer = account!.params?.identityAddress
				conferenceInfo.subject = self.messageText
				
				var participantsList: [ParticipantInfo] = []
				self.participants.forEach { participant in
					do {
						let info = try Factory.Instance.createParticipantInfo(address: participant.address)
						// For meetings, all participants must have Speaker role
						info.role = Participant.Role.Speaker
						participantsList.append(info)
					} catch let error {
						Log.error(
							"\(StartCallViewModel.TAG) Can't create ParticipantInfo: \(error)"
				  		)
					}
				}
				
				self.participants.removeAll()
				
				conferenceInfo.addParticipantInfos(participantInfos: participantsList)
				
				Log.info(
					"\(StartCallViewModel.TAG) Creating group call with subject \(self.messageText) and \(participantsList.count) participant(s)"
				)
				
				let conferenceScheduler = try core.createConferenceScheduler()
				self.conferenceAddDelegate(core: core, conferenceScheduler: conferenceScheduler)
				conferenceScheduler.account = account
				// Will trigger the conference creation/update automatically
				conferenceScheduler.info = conferenceInfo
			} catch let error {
				Log.error(
					"\(StartCallViewModel.TAG) createGroupCall: \(error)"
				)
			}
		}
	}
	
	func conferenceAddDelegate(core: Core, conferenceScheduler: ConferenceScheduler) {
		self.conferenceSuscriptions.insert(conferenceScheduler.publisher?.onStateChanged?.postOnCoreQueue {
			(conferenceScheduler: ConferenceScheduler, state: ConferenceScheduler.State) in
			Log.info("\(StartCallViewModel.TAG) Conference scheduler state is \(state)")
			if state == ConferenceScheduler.State.Ready {
				self.conferenceSuscriptions.removeAll()
				
				let conferenceAddress = conferenceScheduler.info?.uri
				if conferenceAddress != nil {
					Log.info(
						"\(StartCallViewModel.TAG) Conference info created, address is \(conferenceAddress?.asStringUriOnly() ?? "Error conference address")"
					)
					
					self.startVideoCall(core: core, conferenceAddress: conferenceAddress!)
				} else {
					Log.error("\(StartCallViewModel.TAG) Conference info URI is null!")
					
					ToastViewModel.shared.toastMessage = "Failed_to_create_group_call_error"
					ToastViewModel.shared.displayToast = true
				}
				
				DispatchQueue.main.async {
					self.operationInProgress = false
				}
			} else if state == ConferenceScheduler.State.Error {
				self.conferenceSuscriptions.removeAll()
				Log.error("\(StartCallViewModel.TAG) Failed to create group call!")
				
				ToastViewModel.shared.toastMessage = "Failed_to_create_group_call_error"
				ToastViewModel.shared.displayToast = true
				
				DispatchQueue.main.async {
					self.operationInProgress = false
				}
			}
		})
	}
	
	func startVideoCall(core: Core, conferenceAddress: Address) {
		do {
			TelecomManager.shared.doCallWithCore(addr: conferenceAddress, isVideo: true, isConference: true)
		} catch let error {
			Log.error(
				"\(StartCallViewModel.TAG) StartVideoCall: \(error)"
			)
		}
	}
}
