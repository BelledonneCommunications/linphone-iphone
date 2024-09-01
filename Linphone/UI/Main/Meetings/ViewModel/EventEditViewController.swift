/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

import EventKitUI
import SwiftUI

struct EventEditViewController: UIViewControllerRepresentable {
	
	@Environment(\.presentationMode) var presentationMode
	let meetingViewModel: MeetingViewModel
	
	class Coordinator: NSObject, EKEventEditViewDelegate {
		var parent: EventEditViewController

		init(_ controller: EventEditViewController) {
			self.parent = controller
		}
		
		func eventEditViewController(_ controller: EKEventEditViewController, didCompleteWith action: EKEventEditViewAction) {
			parent.presentationMode.wrappedValue.dismiss()
		}
	}
	
	func makeCoordinator() -> Coordinator {
		return Coordinator(self)
	}
	
	typealias UIViewControllerType = EKEventEditViewController
	func makeUIViewController(context: Context) -> EKEventEditViewController {
		let eventEditViewController = EKEventEditViewController()
		eventEditViewController.event = meetingViewModel.createMeetingEKEvent()
		eventEditViewController.eventStore = meetingViewModel.eventStore
		eventEditViewController.editViewDelegate = context.coordinator
		return eventEditViewController
	}
	
	func updateUIViewController(_ uiViewController: EKEventEditViewController, context: Context) {}
}
