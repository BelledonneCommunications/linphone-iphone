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

class HistoryListViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	@Published var callLogs: [HistoryModel] = []
	var callLogsTmp: [HistoryModel] = []
	
	var callLogsAddressToDelete = ""
	var callLogCoreDelegate: CoreDelegate?
	
	@Published var missedCallsCount: Int = 0
	
	init() {
		computeCallLogsList()
		updateMissedCallsCount()
	}
	
	func computeCallLogsList() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			let logs = account?.callLogs != nil ? account!.callLogs : core.callLogs
			
			var callLogsBis: [HistoryModel] = []
			var callLogsTmpBis: [HistoryModel] = []
			
			logs.forEach { log in
				let history = HistoryModel(callLog: log)
				callLogsBis.append(history)
				callLogsTmpBis.append(history)
			}
			
			DispatchQueue.main.async {
				self.callLogs.removeAll()
				self.callLogsTmp.removeAll()
				
				self.callLogs = callLogsBis
				self.callLogsTmp = callLogsTmpBis
			}
			
			self.callLogCoreDelegate = CoreDelegateStub(onCallLogUpdated: { (_: Core, _: CallLog) in
				let account = core.defaultAccount
				let logs = account?.callLogs != nil ? account!.callLogs : core.callLogs
				
				var callLogsBis: [HistoryModel] = []
				var callLogsTmpBis: [HistoryModel] = []
				
				logs.forEach { log in
					let history = HistoryModel(callLog: log)
					callLogsBis.append(history)
					callLogsTmpBis.append(history)
				}
				
				DispatchQueue.main.async {
					self.callLogs.removeAll()
					self.callLogsTmp.removeAll()
					
					self.callLogs = callLogsBis
					self.callLogsTmp = callLogsTmpBis
				}
				
				self.updateMissedCallsCount()
			})
			core.addDelegate(delegate: self.callLogCoreDelegate!)
		}
	}
	
	func resetMissedCallsCount() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account != nil {
				account?.resetMissedCallsCount()
				DispatchQueue.main.async {
					self.missedCallsCount = 0
				}
			} else {
				DispatchQueue.main.async {
					self.missedCallsCount = 0
				}
			}
		}
	}
	
	func updateMissedCallsCount() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account != nil {
				let count = account?.missedCallsCount != nil ? account!.missedCallsCount : core.missedCallsCount
				
				DispatchQueue.main.async {
					self.missedCallsCount = count
				}
			} else {
				DispatchQueue.main.async {
					self.missedCallsCount = 0
				}
			}
		}
	}
	
	func getCallIconResId(callStatus: Call.Status, isOutgoing: Bool) -> String {
		switch callStatus {
		case Call.Status.Missed:
			if isOutgoing {
				"outgoing-call-missed"
			} else {
				"incoming-call-missed"
			}
			
		case Call.Status.Success:
			if isOutgoing {
				"outgoing-call"
			} else {
				"incoming-call"
			}
			
		default:
			if isOutgoing {
				"outgoing-call-rejected"
			} else {
				"incoming-call-rejected"
			}
		}
	}
	
	func getCallText(callStatus: Call.Status, isOutgoing: Bool) -> String {
		switch callStatus {
		case Call.Status.Missed:
			if isOutgoing {
				"Outgoing Call"
			} else {
				"Missed Call"
			}
			
		case Call.Status.Success:
			if isOutgoing {
				"Outgoing Call"
			} else {
				"Incoming Call"
			}
			
		default:
			if isOutgoing {
				"Outgoing Call"
			} else {
				"Incoming Call"
			}
		}
	}
	
	func getCallTime(startDate: time_t) -> String {
		let timeInterval = TimeInterval(startDate)
		
		let myNSDate = Date(timeIntervalSince1970: timeInterval)
		
		if Calendar.current.isDateInToday(myNSDate) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			return "Today | " + formatter.string(from: myNSDate)
		} else if Calendar.current.isDateInYesterday(myNSDate) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			return "Yesterday | " + formatter.string(from: myNSDate)
		} else if Calendar.current.isDate(myNSDate, equalTo: .now, toGranularity: .year) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM | HH:mm" : "MM/dd | h:mm a"
			return formatter.string(from: myNSDate)
		} else {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM/yy | HH:mm" : "MM/dd/yy | h:mm a"
			return formatter.string(from: myNSDate)
		}
	}
	
	func filterCallLogs(filter: String) {
		callLogs.removeAll()
		callLogsTmp.forEach { callLog in
			if callLog.addressName.lowercased().contains(filter.lowercased()) {
				callLogs.append(callLog)
			}
		}
	}
	
	func resetFilterCallLogs() {
		callLogs = callLogsTmp
	}
	
	func removeCallLogs() {
		if callLogsAddressToDelete.isEmpty {
			coreContext.doOnCoreQueue { core in
				let account = core.defaultAccount
				if account != nil {
					account!.clearCallLogs()
				} else {
					core.clearCallLogs()
				}
				
				DispatchQueue.main.async {
					self.callLogs.removeAll()
					self.callLogsTmp.removeAll()
				}
			}
		} else {
			removeCallLogsWithAddress()
			callLogsAddressToDelete = ""
		}
	}
	
	func removeCallLogsWithAddress() {
		self.callLogs.filter { $0.address == callLogsAddressToDelete || $0.address == callLogsAddressToDelete }.forEach { historyModel in
			removeCallLog(historyModel: historyModel)
		}
	}
	
	func removeCallLog(historyModel: HistoryModel) {
		let index = self.callLogs.firstIndex(where: {$0.id == historyModel.id})
		self.callLogs.remove(at: index!)
		
		let indexTmp = self.callLogsTmp.firstIndex(where: {$0.id == historyModel.id})
		self.callLogsTmp.remove(at: indexTmp!)
		
		coreContext.doOnCoreQueue { core in
			core.removeCallLog(callLog: historyModel.callLog)
		}
	}
	
	func refreshHistoryAvatarModel() {
		callLogs.forEach { historyModel in
			historyModel.refreshAvatarModel()
		}
	}
}
