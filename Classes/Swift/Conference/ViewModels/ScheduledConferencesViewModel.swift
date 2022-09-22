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


class ScheduledConferencesViewModel  {

	var core : Core { get { Core.get() } }
	static let shared = ScheduledConferencesViewModel()
	
	var conferences : MutableLiveData<[ScheduledConferenceData]> = MutableLiveData([])
	var daySplitted : [Date : [ScheduledConferenceData]] = [:]
	var coreDelegate: CoreDelegateStub?
	var showTerminated = MutableLiveData(false)
	let editionEnabled = MutableLiveData(false)
	let conferenceScheduler = try? Core.get().createConferenceScheduler()

	
	init () {
		
		coreDelegate = CoreDelegateStub(
			onConferenceInfoReceived: { (core, conferenceInfo) in
				Log.i("[Scheduled Conferences] New conference info received")
				self.conferences.value!.append(ScheduledConferenceData(conferenceInfo: conferenceInfo,isFinished: false))
				self.conferences.notifyValue()
			}
			
		)
		computeConferenceInfoList()
	}

	func computeConferenceInfoList() {
		conferences.value!.removeAll()
		let now = Date().timeIntervalSince1970 // Linphone uses time_t in seconds
		
		if (showTerminated.value == true) {
			core.conferenceInformationList.filter{$0.duration != 0 && (TimeInterval($0.dateTime) + TimeInterval($0.duration) < now)}.forEach { conferenceInfo in
				conferences.value!.append(ScheduledConferenceData(conferenceInfo: conferenceInfo,isFinished: true))
			}
		} else {
			let twoHoursAgo = now - 7200 // Show all conferences from 2 hour ago and forward
			core.getConferenceInformationListAfterTime(time: time_t(twoHoursAgo)).filter{$0.duration != 0}.forEach { conferenceInfo in
				conferences.value!.append(ScheduledConferenceData(conferenceInfo: conferenceInfo,isFinished: false))
			}
		}
		
		
		
		daySplitted = [:]
		conferences.value!.forEach { (conferenceInfo) in
			let startDateDay = dateAtBeginningOfDay(for: conferenceInfo.rawDate)
			if (daySplitted[startDateDay] == nil) {
				daySplitted[startDateDay] = []
			}
			daySplitted[startDateDay]!.append(conferenceInfo)
		}
	}
	
	
	func dateAtBeginningOfDay(for inputDate: Date) -> Date {
		var calendar = Calendar.current
		let timeZone = NSTimeZone.system as NSTimeZone
		calendar.timeZone = timeZone as TimeZone
		return calendar.date(from: calendar.dateComponents([.year, .month, .day], from: inputDate))!
	}
	
	
}
