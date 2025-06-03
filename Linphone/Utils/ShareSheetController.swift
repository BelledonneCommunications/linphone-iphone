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

import Foundation
import SwiftUI
import linphonesw

struct ShareSheet: UIViewControllerRepresentable {
	typealias Callback = (_ activityType: UIActivity.ActivityType?, _ completed: Bool, _ returnedItems: [Any]?, _ error: Error?) -> Void
	
	let friendToShare: ContactAvatarModel
	var activityItems: [Any] = []
	let applicationActivities: [UIActivity]? = nil
	let excludedActivityTypes: [UIActivity.ActivityType]? = nil
	let callback: Callback? = nil
	
	func makeUIViewController(context: Context) -> UIActivityViewController {
		let directoryURL = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first
		
		if directoryURL != nil {
			let filename = friendToShare.name.replacingOccurrences(of: " ", with: "")
			   
			   let fileURL = directoryURL!
				   .appendingPathComponent(filename)
				   .appendingPathExtension("vcf")
			   
			   if let vCard = friendToShare.vcard {
				   try? vCard.asVcard4String().write(to: fileURL, atomically: false, encoding: String.Encoding.utf8)
				   
				   let controller = UIActivityViewController(
				   activityItems: [fileURL],
				   applicationActivities: applicationActivities
				   )
				   controller.excludedActivityTypes = excludedActivityTypes
				   controller.completionWithItemsHandler = callback
				   return controller
			   }
		}
		
		let controller = UIActivityViewController(
			activityItems: activityItems,
			applicationActivities: applicationActivities)
		controller.excludedActivityTypes = excludedActivityTypes
		controller.completionWithItemsHandler = callback
		return controller
	}
	
	func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context) {
		// nothing to do here
	}
	
	func shareContacts(friend: String) {
		
		let directoryURL = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first
		
		if directoryURL != nil {
			let filename = NSUUID().uuidString
			
			let fileURL = directoryURL!
				.appendingPathComponent(filename)
				.appendingPathExtension("vcf")
			
			try? friend.write(to: fileURL, atomically: false, encoding: String.Encoding.utf8)
		}
		
		/*
		 let activityViewController = UIActivityViewController(
		 activityItems: [fileURL],
		 applicationActivities: nil
		 )
		 */
	}
}
