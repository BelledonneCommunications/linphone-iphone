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

import SwiftUI
import linphonesw
import Contacts

struct MeetingsListBottomSheet: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@EnvironmentObject var meetingsListViewModel: MeetingsListViewModel
	
	@Binding var showingSheet: Bool
	@Binding var isShowSendCancelMeetingNotificationPopup: Bool
	
	var body: some View {
		VStack(alignment: .leading) {
			if idiom != .pad && (orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Spacer()
				HStack {
					Spacer()
					Button("dialog_close") {
						if #available(iOS 16.0, *) {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					}
				}
				.padding(.trailing)
			}
			
			Button {
				CoreContext.shared.doOnCoreQueue { core in
					if let organizerUri = self.meetingsListViewModel.selectedMeetingToDelete?.confInfo.organizer {
						if core.defaultAccount?.contactAddress?.weakEqual(address2: organizerUri) ?? false {
							// If we are the organizer, display popup for sending
							DispatchQueue.main.async {
								self.isShowSendCancelMeetingNotificationPopup = true
							}
						} else { 
							// If we are not the organizer, delete meeting locally without popup
							meetingsListViewModel.deleteSelectedMeeting()
						}
					}
				}
				if #available(iOS 16.0, *) {
					if idiom != .pad {
						showingSheet.toggle()
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} else {
					showingSheet.toggle()
					dismiss()
				}
			} label: {
				HStack {
					Image("trash-simple")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.redDanger500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					Text("meeting_info_delete")
						.foregroundStyle(Color.redDanger500)
						.default_text_style(styleSize: 16)
					Spacer()
				}
				.frame(maxHeight: .infinity)
			}
			.padding(.horizontal, 30)
			.background(Color.gray100)
			
		}
		.background(Color.gray100)
		.frame(maxWidth: .infinity)
		.onRotate { newOrientation in
			orientation = newOrientation
		}
	}
}
