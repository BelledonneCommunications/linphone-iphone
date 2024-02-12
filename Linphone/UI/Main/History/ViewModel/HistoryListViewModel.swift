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
	
	@Published var callLogs: [CallLog] = []
	var callLogsTmp: [CallLog] = []
	
	var callLogsAddressToDelete = ""
	var callLogSubscription: AnyCancellable?
	
	init() {
		computeCallLogsList()
	}
	
	func computeCallLogsList() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			let logs = account?.callLogs != nil ? account!.callLogs : core.callLogs
			
			DispatchQueue.main.async {
				self.callLogs.removeAll()
				self.callLogsTmp.removeAll()
				
				logs.forEach { log in
					self.callLogs.append(log)
					self.callLogsTmp.append(log)
				}
			}
			
			self.callLogSubscription = core.publisher?.onCallLogUpdated?.postOnCoreQueue { (_: (_: Core, _: CallLog)) in
				let account = core.defaultAccount
				let logs = account != nil ? account!.callLogs : core.callLogs
				
				DispatchQueue.main.async {
					self.callLogs.removeAll()
					self.callLogsTmp.removeAll()
					
					logs.forEach { log in
						self.callLogs.append(log)
						self.callLogsTmp.append(log)
					}
				}
			}
		}
	}
	
	func getCallIconResId(callStatus: Call.Status, callDir: Call.Dir) -> String {
		switch callStatus {
		case Call.Status.Missed:
			if callDir == .Outgoing {
				"outgoing-call-missed"
			} else {
				"incoming-call-missed"
			}
			
		case Call.Status.Success:
			if callDir == .Outgoing {
				"outgoing-call"
			} else {
				"incoming-call"
			}
			
		default:
			if callDir == .Outgoing {
				"outgoing-call-rejected"
			} else {
				"incoming-call-rejected"
			}
		}
	}
	
	func getCallText(callStatus: Call.Status, callDir: Call.Dir) -> String {
		switch callStatus {
		case Call.Status.Missed:
			if callDir == .Outgoing {
				"Outgoing Call"
			} else {
				"Missed Call"
			}
			
		case Call.Status.Success:
			if callDir == .Outgoing {
				"Outgoing Call"
			} else {
				"Incoming Call"
			}
			
		default:
			if callDir == .Outgoing {
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
			if callLog.dir == .Outgoing && callLog.toAddress != nil {
				if callLog.toAddress!.username != nil && callLog.toAddress!.username!.contains(filter) {
					callLogs.append(callLog)
				} else if callLog.toAddress!.displayName != nil && callLog.toAddress!.displayName!.contains(filter) {
					callLogs.append(callLog)
				}
			} else if callLog.fromAddress != nil {
				if callLog.fromAddress!.username != nil && callLog.fromAddress!.username!.contains(filter) {
					callLogs.append(callLog)
				} else if callLog.fromAddress!.displayName != nil && callLog.fromAddress!.displayName!.contains(filter) {
					callLogs.append(callLog)
				}
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
		self.callLogs.filter { $0.toAddress!.asStringUriOnly() == callLogsAddressToDelete || $0.fromAddress!.asStringUriOnly() == callLogsAddressToDelete }.forEach { callLog in
			removeCallLog(callLog: callLog)
			
			coreContext.doOnCoreQueue { core in
				core.removeCallLog(callLog: callLog)
			}
		}
	}
	
	func removeCallLog(callLog: CallLog) {
		let index = self.callLogs.firstIndex(where: {$0.callId == callLog.callId})
		self.callLogs.remove(at: index!)
		
		let indexTmp = self.callLogsTmp.firstIndex(where: {$0.callId == callLog.callId})
		self.callLogsTmp.remove(at: indexTmp!)
	}
}
