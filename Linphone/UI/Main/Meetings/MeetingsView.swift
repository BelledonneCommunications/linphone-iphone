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

import SwiftUI

struct MeetingsView: View {
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject var meetingsListViewModel: MeetingsListViewModel
	@ObservedObject var meetingViewModel: MeetingViewModel
	
	@Binding var isShowScheduleMeetingFragment: Bool
	@Binding var isShowSendCancelMeetingNotificationPopup: Bool
	
	@State private var showingSheet = false
	@Binding var text: String
		
	var body: some View {
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				
				if #available(iOS 16.0, *), idiom != .pad {
					MeetingsFragment(meetingsListViewModel: meetingsListViewModel, meetingViewModel: meetingViewModel, showingSheet: $showingSheet, text: $text)
						.sheet(isPresented: $showingSheet) {
							MeetingsListBottomSheet(
								meetingsListViewModel: meetingsListViewModel,
								showingSheet: $showingSheet,
								isShowSendCancelMeetingNotificationPopup: $isShowSendCancelMeetingNotificationPopup
							)
							.presentationDetents([.fraction(0.1)])
						}
				} else {
					MeetingsFragment(meetingsListViewModel: meetingsListViewModel, meetingViewModel: meetingViewModel, showingSheet: $showingSheet, text: $text)
						.halfSheet(showSheet: $showingSheet) {
							MeetingsListBottomSheet(
								meetingsListViewModel: meetingsListViewModel,
								showingSheet: $showingSheet,
								isShowSendCancelMeetingNotificationPopup: $isShowSendCancelMeetingNotificationPopup
							)
						} onDismiss: {}
				}
				
				Button {
					withAnimation {
						meetingViewModel.resetViewModelData()
						isShowScheduleMeetingFragment.toggle()
					}
				} label: {
					Image("meeting_plus")
						.renderingMode(.template)
						.foregroundStyle(.white)
						.padding()
						.background(Color.orangeMain500)
						.clipShape(Circle())
						.shadow(color: .black.opacity(0.2), radius: 4)
					
				}
				.padding()
			}
		}
		.navigationViewStyle(.stack)
	}
}

#Preview {
	MeetingsView(
		meetingsListViewModel: MeetingsListViewModel(),
		meetingViewModel: MeetingViewModel(),
		isShowScheduleMeetingFragment: .constant(false),
		isShowSendCancelMeetingNotificationPopup: .constant(false),
		text: .constant("")
	)
}
