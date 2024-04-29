//
//  AddParticipantsViewModel.swift
//  Linphone
//
//  Created by QuentinArguillere on 29/04/2024.
//

import Foundation
import linphonesw
import Combine

class SelectedAddressModel: ObservableObject {
	var address: Address
	var avatarModel: ContactAvatarModel
	
	init (addr: Address, avModel: ContactAvatarModel) {
		address = addr
		avatarModel = avModel
	}
}

class AddParticipantsViewModel: ObservableObject {
	static let TAG = "[AddParticipantsViewModel]"
	
	@Published var participantsToAdd: [SelectedAddressModel] = []
	@Published var searchField: String = ""
	
	func selectParticipant(addr: Address) {
		if let idx = participantsToAdd.firstIndex(where: {$0.address.weakEqual(address2: addr)}) {
			Log.info("[\(AddParticipantsViewModel.TAG)] Removing participant \(addr.asStringUriOnly()) from selection")
			participantsToAdd.remove(at: idx)
		} else {
			Log.info("[\(AddParticipantsViewModel.TAG)] Adding participant \(addr.asStringUriOnly()) to selection")
			participantsToAdd.append(SelectedAddressModel(addr: addr, avModel: ContactAvatarModel.getAvatarModelFromAddress(address: addr)))
		}
	}
	
	func reset() {
		participantsToAdd = []
		searchField = ""
	}
}
