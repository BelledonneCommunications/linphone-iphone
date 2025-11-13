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
import SwiftUI

class RecordingModel: ObservableObject {
	static let TAG = "[Recording Model]"
	
	var filePath: String = ""
	var fileName: String = ""
	var sipUri: String = ""
	var displayName: String = ""
	var timestamp: Int64 = 0
	var month: String = ""
	var dateTime: String = ""
	var formattedDuration: String = ""
	var duration: Int = 0
	
	init(filePath: String, fileName: String, isLegacy: Bool = false) {
		self.filePath = filePath
		self.fileName = fileName
		
		var sipUriTmp: String = ""
		var displayNameTmp: String = ""
		var timestampTmp: Int64 = 0
		
		CoreContext.shared.doOnCoreQueue { core in
			if isLegacy {
				let parts = fileName.split(separator: "_")
				let username = String(parts.first ?? "")
				let sipAddress = core.interpretUrl(url: username, applyInternationalPrefix: false)
				sipUriTmp = sipAddress?.asStringUriOnly() ?? username
				
				if let address = sipAddress {
					ContactsManager.shared.getFriendWithAddressInCoreQueue(address: address) { friendResult in
						if let addressFriend = friendResult {
							displayNameTmp = addressFriend.name!
						} else {
							if address.displayName != nil {
								displayNameTmp = address.displayName!
							} else if address.username != nil {
								displayNameTmp = address.username!
							} else {
								displayNameTmp = String(address.asStringUriOnly().dropFirst(4))
							}
						}
					}
				} else {
					displayNameTmp = sipUriTmp
				}
				
				if parts.count > 1 {
					let parsedDate = String(parts[1])
					let formatter = DateFormatter()
					formatter.dateFormat = "dd-MM-yyyy-HH-mm-ss"
					if let date = formatter.date(from: parsedDate) {
						timestampTmp = Int64(date.timeIntervalSince1970 * 1000)
					} else {
						Log.error("\(RecordingModel.TAG) Failed to parse legacy timestamp \(parsedDate)")
					}
				}
			} else {
				let headerLength = LinphoneUtils.RECORDING_FILE_NAME_HEADER.count
				let withoutHeader = String(fileName.dropFirst(headerLength))
				guard let sepRange = withoutHeader.range(of: LinphoneUtils.RECORDING_FILE_NAME_URI_TIMESTAMP_SEPARATOR) else {
					fatalError("\(RecordingModel.TAG) Invalid file name format \(withoutHeader)")
				}
				
				sipUriTmp = String(withoutHeader[..<sepRange.lowerBound])
				let sipAddress = try? Factory.Instance.createAddress(addr: "sip:" + sipUriTmp)
				
				if let address = sipAddress {
					ContactsManager.shared.getFriendWithAddressInCoreQueue(address: address) { friendResult in
						if let addressFriend = friendResult {
							displayNameTmp = addressFriend.name!
						} else {
							if address.displayName != nil {
								displayNameTmp = address.displayName!
							} else if address.username != nil {
								displayNameTmp = address.username!
							} else {
								displayNameTmp = String(address.asStringUriOnly().dropFirst(4))
							}
						}
					}
				} else {
					displayNameTmp = sipUriTmp
				}
				
				let start = sepRange.upperBound
				if let dotRange = withoutHeader.range(of: ".", options: .backwards) {
					let parsedTimestamp = String(withoutHeader[start..<dotRange.lowerBound])
					Log.info("\(RecordingModel.TAG) Extract SIP URI \(sipUriTmp) and timestamp \(parsedTimestamp) from file \(fileName)")
					timestampTmp = Int64(parsedTimestamp) ?? 0
				}
			}
			
			self.sipUri = sipUriTmp
			self.displayName = displayNameTmp
			self.timestamp = timestampTmp
			
			self.month = self.formattedMonthYear(timestamp: timestampTmp)
			self.dateTime = self.formattedDateTime(timestamp: timestampTmp)
			
			do {
			let audioPlayer = try core.createLocalPlayer(soundCardName: nil, videoDisplayName: nil, windowId: nil)
				try? audioPlayer.open(filename: filePath)
				self.duration = audioPlayer.duration
				print("durationduration \(audioPlayer.duration) \((audioPlayer.duration/1000).convertDurationToString())")
				self.formattedDuration = (audioPlayer.duration/1000).convertDurationToString()
			} catch {
				self.duration = 0
				self.formattedDuration = "??:??"
			}
		}
	}
	
	func delete() async {
		Log.info("\(RecordingModel.TAG) Deleting call recording \(filePath)")
		//await FileUtils.deleteFile(path: filePath)
	}
	
	func formattedDateTime(timestamp: Int64) -> String {
		let locale = Locale.current
		let date = Date(timeIntervalSince1970: TimeInterval(timestamp))

		let dateFormatter = DateFormatter()
		dateFormatter.locale = locale
		
		if Calendar.current.isDate(date, equalTo: .now, toGranularity: .year) {
			dateFormatter.dateFormat = locale.identifier == "fr_FR"
			 ? "EEEE d MMMM"
			 : "EEEE, MMMM d"
		} else {
			dateFormatter.dateFormat = locale.identifier == "fr_FR"
			 ? "EEEE d MMMM yyyy"
			 : "EEEE, MMMM d, yyyy"
		}

		let timeFormatter = DateFormatter()
		timeFormatter.locale = locale
		timeFormatter.dateFormat = locale.identifier == "fr_FR"
			? "HH:mm"
			: "h:mm a"

		return "\(dateFormatter.string(from: date).capitalized) - \(timeFormatter.string(from: date))"
	}
	
	func formattedMonthYear(timestamp: Int64) -> String {
		let locale = Locale.current
		let date = Date(timeIntervalSince1970: TimeInterval(timestamp))

		let formatter = DateFormatter()
		formatter.locale = locale
		if Calendar.current.isDate(date, equalTo: .now, toGranularity: .year) {
			formatter.dateFormat = "MMMM"
		} else {
			formatter.dateFormat = "MMMM yyyy"
		}

		return formatter.string(from: date).capitalized
	}
}
