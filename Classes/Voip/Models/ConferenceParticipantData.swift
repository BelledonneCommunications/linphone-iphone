/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linhome
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
import Foundation

class ConferenceParticipantData  {
	
	var conference:Conference
	var participant:Participant
	
	let isAdmin = MutableLiveData<Bool>()
	let isMeAdmin = MutableLiveData<Bool>()
	
	private var callDelegate :  CallDelegateStub?
	
	init (conference:Conference, participant:Participant)  {
		self.conference = conference
		self.participant = participant
		isAdmin.value = participant.isAdmin
		isMeAdmin.value = conference.me?.isAdmin
		Log.i("[Conference Participant] Participant \(sipUri!) is admin=\(isAdmin.value!)")

	}
	
	var sipUri:String? {
		get {
			return self.participant.address?.asString()
		}
	}
	
	func destroy() {
		isAdmin.clearObservers()
		isMeAdmin.clearObservers()
	}
}
