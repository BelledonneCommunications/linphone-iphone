/*
 * Copyright (c) 2010-2021 Belledonne Communications SARL.
 *
 * This file is part of linphone-android
 * (see https://www.linphone.org).
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


class ScheduledConferenceData {
	
	let conferenceInfo: ConferenceInfo
    let expanded = MutableLiveData<Bool>()

    let address = MutableLiveData<String>()
    let subject = MutableLiveData<String>()
    let description = MutableLiveData<String>()
    let time = MutableLiveData<String>()
    let date = MutableLiveData<String>()
    let duration = MutableLiveData<String>()
    let organizer = MutableLiveData<String>()
    let participantsShort = MutableLiveData<String>()
    let participantsExpanded = MutableLiveData<String>()
	let rawDate : Date
	

    init (conferenceInfo: ConferenceInfo) {
		self.conferenceInfo = conferenceInfo
        expanded.value = false

        address.value = conferenceInfo.uri?.asStringUriOnly()
        subject.value = conferenceInfo.subject
        description.value = conferenceInfo.description

		time.value = TimestampUtils.timeToString(unixTimestamp: Double(conferenceInfo.dateTime))
		date.value = TimestampUtils.toString(unixTimestamp:Double(conferenceInfo.dateTime), onlyDate:true, shortDate:false)
		rawDate = Date(timeIntervalSince1970:TimeInterval(conferenceInfo.dateTime))
		
		let durationFormatter = DateComponentsFormatter()
		durationFormatter.unitsStyle = .positional
		durationFormatter.allowedUnits = [.minute, .second ]
		durationFormatter.zeroFormattingBehavior = [ .pad ]
		duration.value = durationFormatter.string(from: TimeInterval(conferenceInfo.duration))
		
		organizer.value = conferenceInfo.organizer?.addressBookEnhancedDisplayName()
		
        computeParticipantsLists()
    }

    func destroy() {
    }

    func toggleExpand() {
        expanded.value = expanded.value == false
    }

    private func computeParticipantsLists() {
		participantsShort.value = conferenceInfo.participants.map {(participant) in
			String(describing: participant.addressBookEnhancedDisplayName())
		}.joined(separator: ", ")
		
		participantsExpanded.value = conferenceInfo.participants.map {(participant) in
			String(describing: participant.addressBookEnhancedDisplayName())+" ("+String(describing: participant.asStringUriOnly())+")"
		}.joined(separator: "\n")
    }
	
	func gotoAssociatedChat() {
		
	}
}
