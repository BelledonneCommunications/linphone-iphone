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
	let isConferenceCancelled = MutableLiveData(false)
	let canEdit = MutableLiveData(false)
	let isFinished : Bool
	let selectedForDeletion = MutableLiveData(false)
	private var conferenceSchedulerDelegate : ConferenceSchedulerDelegateStub? = nil
	private var conferenceScheduler : ConferenceScheduler? = nil
	
	init (conferenceInfo: ConferenceInfo, isFinished: Bool = false) {
		self.conferenceInfo = conferenceInfo
		self.isFinished = isFinished
		expanded.value = false
		
		address.value = conferenceInfo.uri?.asStringUriOnly()
		subject.value = conferenceInfo.subject
		description.value = conferenceInfo.description
		
		time.value = TimestampUtils.timeToString(unixTimestamp: Double(conferenceInfo.dateTime))
		date.value = TimestampUtils.toString(unixTimestamp:Double(conferenceInfo.dateTime), onlyDate:true, shortDate:false)
		rawDate = Date(timeIntervalSince1970:TimeInterval(conferenceInfo.dateTime))
		
		let durationFormatter = DateComponentsFormatter()
		durationFormatter.unitsStyle = .abbreviated
		duration.value = conferenceInfo.duration > 0 ? durationFormatter.string(from: TimeInterval(conferenceInfo.duration*60)) : nil
		
		organizer.value = conferenceInfo.organizer?.addressBookEnhancedDisplayName()
		
		computeParticipantsLists()
		
		isConferenceCancelled.value = conferenceInfo.state == .Cancelled
		
		if let organizerAddress = conferenceInfo.organizer {
			let localAccount = Core.get().accountList.filter { account in
				account.params?.identityAddress != nil && organizerAddress.weakEqual(address2: account.params!.identityAddress!)
			}.first
			canEdit.value = localAccount != nil
		} else {
			canEdit.value = false
			Log.e("[Scheduled Conference] No organizer SIP URI found for: \(conferenceInfo.uri?.asStringUriOnly())")
		}
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
		
		if (participantsShort.value?.count == 0) {
			participantsShort.value = " "
		}
		
		participantsExpanded.value = conferenceInfo.participants.map {(participant) in
			String(describing: participant.addressBookEnhancedDisplayName())+" ("+String(describing: participant.asStringUriOnly())+")"
		}.joined(separator: "\n")
	}
	
	func gotoAssociatedChat() {
		
	}
	
	
	
	func deleteConference() {
		conferenceSchedulerDelegate = ConferenceSchedulerDelegateStub(
			onStateChanged: { scheduler, state in
				Log.i("[Conference Deletion] Conference scheduler state is \(state)")
				if (state == .Ready) {
					if let chatRoomParams = ConferenceSchedulingViewModel.shared.getConferenceInvitationsChatRoomParams() {
						scheduler.sendInvitations(chatRoomParams: chatRoomParams) // Send cancel ICS
						Log.e("[Conference Deletion] sent cancel ICS.")
					}
				}
			})
		
		if (conferenceInfo.state != .Cancelled && canEdit.value == true) {
			Log.i("[Scheduled Conferences] Cancelling conference \(conferenceInfo.subject)")
			self.conferenceScheduler = try? Core.get().createConferenceScheduler()
			if (self.conferenceScheduler != nil) {
				self.conferenceScheduler?.addDelegate(delegate: conferenceSchedulerDelegate!)
				self.conferenceScheduler?.cancelConference(conferenceInfo: conferenceInfo)
			}
		} else {
			Core.get().deleteConferenceInformation(conferenceInfo: conferenceInfo)
		}
	}
}
