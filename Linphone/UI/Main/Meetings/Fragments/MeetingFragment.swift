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
import linphonesw
import UniformTypeIdentifiers

struct MeetingFragment: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@EnvironmentObject var meetingsListViewModel: MeetingsListViewModel
	
	@StateObject private var meetingViewModel = MeetingViewModel()
	
	@State private var showDatePicker = false
	@State private var showTimePicker = false
	@State private var showEventEditView = false
	
	@State var selectedDate = Date.now
	@State var setFromDate: Bool = true
	@State var selectedHours: Int = 0
	@State var selectedMinutes: Int = 0
	
	@State var addParticipantsViewModel = AddParticipantsViewModel()
	@Binding var isShowScheduleMeetingFragment: Bool
	@Binding var isShowSendCancelMeetingNotificationPopup: Bool
	
	@ViewBuilder
	func getParticipantLine(participant: SelectedAddressModel) -> some View {
		HStack(spacing: 0) {
			Avatar(contactAvatarModel: participant.avatarModel, avatarSize: 50)
				.padding(.leading, 10)
			
			Text(participant.avatarModel.name)
				.default_text_style(styleSize: 14)
				.padding(.leading, 10)
				.padding(.trailing, 40)
			
			Text("meeting_info_organizer_label")
				.font(Font.custom("NotoSans-Light", size: 12))
				.foregroundStyle(Color.grayMain2c600)
				.opacity(participant.isOrganizer ? 1 : 0)
		}.padding(.bottom, 5)
	}
	
	var body: some View {
		let displayedMeetingUpdated = NotificationCenter.default
			.publisher(for: NSNotification.Name("DisplayedMeetingUpdated"))
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				VStack(spacing: 0) {
					Rectangle()
						.foregroundColor(Color.orangeMain500)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 1)
					
					HStack {
						Image("caret-left")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
							.padding(.leading, -10)
							.onTapGesture {
								withAnimation {
									SharedMainViewModel.shared.displayedMeeting = nil
								}
							}
						Spacer()
						if meetingViewModel.myself != nil && meetingViewModel.myself!.isOrganizer {
							Image("pencil-simple")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.orangeMain500)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.trailing, 5)
								.onTapGesture {
									withAnimation {
										isShowScheduleMeetingFragment.toggle()
									}
								}
						}
						
						Menu {
							Button {
								if #available(iOS 17.0, *) {
									withAnimation {
										showEventEditView.toggle()
									}
								} else {
									meetingViewModel.addMeetingToCalendar()
								}
							} label: {
								HStack {
									Image("calendar")
										.renderingMode(.template)
										.resizable()
										.frame(width: 25, height: 25, alignment: .leading)
									Text("meeting_info_export_as_calendar_event")
										.default_text_style(styleSize: 16)
									Spacer()
								}
							}
							Button(role: .destructive) {
								withAnimation {
									meetingsListViewModel.selectedMeetingToDelete = SharedMainViewModel.shared.displayedMeeting
									if let myself = meetingViewModel.myself, myself.isOrganizer == true {
										isShowSendCancelMeetingNotificationPopup.toggle()
									} else {
										// If we're not organizer, directly delete the conference
										SharedMainViewModel.shared.displayedMeeting = nil
										meetingsListViewModel.deleteSelectedMeeting()
									}
								}
							} label: {
								HStack {
									Image("trash-simple")
										.renderingMode(.template)
										.resizable()
										.frame(width: 25, height: 25, alignment: .leading)
									Text("meeting_info_delete")
										.foregroundStyle(Color.redDanger500)
										.default_text_style(styleSize: 16)
									Spacer()
								}
							}
						} label: {
							Image("dots-three-vertical")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.orangeMain500)
								.frame(width: 25, height: 25, alignment: .leading)
						}
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 5)
					.background(.white)
					
					ScrollView(.vertical) {
						HStack(alignment: .center, spacing: 10) {
							Image("video-conference")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
							Text(meetingViewModel.subject)
								.fontWeight(.bold)
								.default_text_style(styleSize: 20)
								.frame(height: 29, alignment: .leading)
							Spacer()
						}.padding(.bottom, 5)
						
						Rectangle()
							.foregroundStyle(.clear)
							.frame(height: 1)
							.background(Color.gray200)
						
						HStack(alignment: .center, spacing: 10) {
							Image("video-camera")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
							Text(meetingViewModel.conferenceUri)
								.underline()
								.lineLimit(1)
								.default_text_style(styleSize: 14)
							Spacer()
							
							Button(action: {
								UIPasteboard.general.setValue(
									meetingViewModel.conferenceUri,
									forPasteboardType: UTType.plainText.identifier
								)
								
								DispatchQueue.main.async {
									ToastViewModel.shared.toastMessage = "Success_address_copied_into_clipboard"
									ToastViewModel.shared.displayToast = true
								}
							}, label: {
								HStack {
									Image("share-network")
									 .renderingMode(.template)
									 .resizable()
									 .foregroundStyle(Color.grayMain2c800)
									 .frame(width: 25, height: 25)
									 .padding(.trailing, 15)
								}
							})
						}
						
						HStack(alignment: .center, spacing: 10) {
							Image("clock")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
							Text(meetingViewModel.getFullDateString())
								.default_text_style(styleSize: 14)
							Spacer()
						}
						
						HStack(alignment: .center, spacing: 10) {
							Image("earth")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
							Text(meetingViewModel.selectedTimezone.formattedString())
								.default_text_style(styleSize: 14)
							Spacer()
						}
						
						Rectangle()
							.foregroundStyle(.clear)
							.frame(height: 1)
							.background(Color.gray200)
						
						if !meetingViewModel.description.isEmpty {
							HStack(alignment: .top, spacing: 10) {
								Image("note")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 24, height: 24)
									.padding(.leading, 15)
								
								Text(meetingViewModel.description)
									.default_text_style(styleSize: 14)
								Spacer()
							}.padding(.vertical, 10)
							Rectangle()
							 .foregroundStyle(.clear)
							 .frame(height: 1)
							 .background(Color.gray200)
						}
												
						HStack(alignment: .top, spacing: 10) {
							Image("users")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
							
							ScrollView {
								VStack(alignment: .leading, spacing: 0) {
									if meetingViewModel.myself != nil && meetingViewModel.myself!.isOrganizer {
										getParticipantLine(participant: meetingViewModel.myself!)
									}
									ForEach(0..<meetingViewModel.participants.count, id: \.self) { index in
										getParticipantLine(participant: meetingViewModel.participants[index])
									}
								}
							}.frame(maxHeight: .infinity)
							Spacer()
						}.padding(.top, 10)
						
						Rectangle()
							.foregroundStyle(.clear)
							.frame(height: 1)
							.background(Color.gray200)
					}
					.background(.white)
				}.frame(maxHeight: .infinity)
				
				Spacer()
				
				Button(action: {
					meetingViewModel.joinMeeting(addressUri: SharedMainViewModel.shared.displayedMeeting?.address ?? "")
				}, label: {
					Text("meeting_info_join_title")
						.bold()
						.default_text_style_white_500(styleSize: 16)
						.frame(maxWidth: .infinity, maxHeight: 47, alignment: .center)
						.frame(height: 47)
						.background(Color.orangeMain500)
						.clipShape(RoundedRectangle(cornerRadius: 48))
						.padding(.leading, 15)
						.padding(.trailing, 15)
				})
			}.sheet(isPresented: $showEventEditView, content: { // $showEventEditView only edited on iOS17+
				EventEditViewController(meetingViewModel: self.meetingViewModel)
			})
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.onReceive(displayedMeetingUpdated) { _ in
			if let displayedMeeting = SharedMainViewModel.shared.displayedMeeting {
				meetingViewModel.loadExistingMeeting(meeting: displayedMeeting)
			}
		}
	}
}
