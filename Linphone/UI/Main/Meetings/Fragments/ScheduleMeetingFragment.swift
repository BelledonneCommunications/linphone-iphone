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

// swiftlint:disable type_body_length
struct ScheduleMeetingFragment: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@EnvironmentObject var meetingsListViewModel: MeetingsListViewModel
	
	@StateObject private var meetingViewModel: MeetingViewModel
	
	@State private var delayedColor = Color.white
	@State private var showDatePicker = false
	@State private var showTimePicker = false
	@State private var showTimeZonePicker = false
	
	@Binding var isShowScheduleMeetingFragment: Bool
	
	@State var selectedDate = Date.now
	@State var setFromDate: Bool = true
	@State var selectedHours: Int = 0
	@State var selectedMinutes: Int = 0
	
	@State var addParticipantsViewModel = AddParticipantsViewModel()
	@FocusState var isDescriptionTextFocused: Bool
	@FocusState var isSubjectTextFocused: Bool
	
	init(isShowScheduleMeetingFragmentSubject: String? = nil, isShowScheduleMeetingFragmentParticipants: [SelectedAddressModel]? = nil, isShowScheduleMeetingFragment: Binding<Bool>) {
		_meetingViewModel = StateObject(wrappedValue: MeetingViewModel(isShowScheduleMeetingFragmentSubject: isShowScheduleMeetingFragmentSubject, isShowScheduleMeetingFragmentParticipants: isShowScheduleMeetingFragmentParticipants))
		self._isShowScheduleMeetingFragment = isShowScheduleMeetingFragment
	}
	
	var body: some View {
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				VStack(spacing: 10) {
					if #available(iOS 16.0, *) {
						Rectangle()
							.foregroundColor(delayedColor)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
							.task(delayColor)
					} else if idiom != .pad && !(orientation == .landscapeLeft || orientation == .landscapeRight
												 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
						Rectangle()
							.foregroundColor(delayedColor)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 1)
							.task(delayColor)
					}
					
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
									isShowScheduleMeetingFragment.toggle()
								}
							}
						
						Text(SharedMainViewModel.shared.displayedMeeting != nil ? "meeting_schedule_edit_title" : "meeting_schedule_title" )
							.multilineTextAlignment(.leading)
							.default_text_style_orange_800(styleSize: 16)
						
						Spacer()
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.background(.white)
					
					ScrollView(.vertical) {
						HStack(alignment: .center, spacing: 10) {
							Image("video-conference")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
							TextField("meeting_schedule_subject_hint", text: $meetingViewModel.subject)
								.focused($isSubjectTextFocused)
								.default_text_style_700(styleSize: 20)
								.frame(height: 29, alignment: .leading)
							Spacer()
						}
						
						Rectangle()
							.foregroundStyle(.clear)
							.frame(height: 1)
							.background(Color.gray200)
						
						HStack(alignment: .center, spacing: 10) {
							Image("clock")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
							Text(meetingViewModel.fromDateStr)
								.fontWeight(.bold)
								.default_text_style_500(styleSize: 16)
								.onTapGesture {
									isDescriptionTextFocused = false
									isSubjectTextFocused = false
									
									setFromDate = true
									selectedDate = meetingViewModel.fromDate
									showDatePicker.toggle()
								}
							Spacer()
						}.padding(.bottom, -5)
						
						HStack(spacing: 10) {
							Text(meetingViewModel.fromTime)
								.fontWeight(.bold)
								.padding(.leading, 50)
								.frame(height: 29, alignment: .leading)
								.default_text_style_500(styleSize: 16)
								.onTapGesture {
									isDescriptionTextFocused = false
									isSubjectTextFocused = false
									
									setFromDate = true
									selectedDate = meetingViewModel.fromDate
									showTimePicker.toggle()
								}
							Text(meetingViewModel.toTime)
								.fontWeight(.bold)
								.padding(.leading, 10)
								.frame(height: 29, alignment: .leading)
								.default_text_style_500(styleSize: 16)
								.onTapGesture {
									isDescriptionTextFocused = false
									isSubjectTextFocused = false
									
									setFromDate = false
									selectedDate = meetingViewModel.toDate
									showTimePicker.toggle()
								}
							Spacer()
						}
						
						HStack(alignment: .center, spacing: 0) {
							Image("earth")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 15)
								.padding(.trailing, 10)
							Text("meeting_schedule_timezone_title")
								.fontWeight(.bold)
								.default_text_style_500(styleSize: 15)
							Text(": \(meetingViewModel.selectedTimezone.formattedString())")
								.fontWeight(.bold)
								.lineLimit(1)
								.default_text_style_500(styleSize: 15)
								.onTapGesture {
									isDescriptionTextFocused = false
									isSubjectTextFocused = false
									showTimeZonePicker.toggle()
								}
							Spacer()
						}
						
						/*
						 Image("arrow-clockwise")
						 .renderingMode(.template)
						 .resizable()
						 .foregroundStyle(Color.grayMain2c800)
						 .frame(width: 24, height: 24)
						 .padding(.leading, 15)
						 //Picker(selection:, label:("))
						 .fontWeight(.bold)
						 .padding(.leading, 5)
						 .default_text_style_500(styleSize: 16)
						 Spacer()
						 }
						 */
						
						Rectangle()
							.foregroundStyle(.clear)
							.frame(height: 1)
							.background(Color.gray200)
						
						HStack(alignment: .top, spacing: 10) {
							Image("note")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 24, height: 24)
								.padding(.leading, 16)
							
							if #available(iOS 16.0, *) {
								TextField("meeting_schedule_description_hint", text: $meetingViewModel.description, axis: .vertical)
									.default_text_style(styleSize: 15)
									.focused($isDescriptionTextFocused)
									.padding(5)
							} else {
								ZStack(alignment: .leading) {
									TextEditor(text: $meetingViewModel.description)
										.multilineTextAlignment(.leading)
										.frame(maxHeight: 160)
										.fixedSize(horizontal: false, vertical: true)
										.default_text_style(styleSize: 15)
										.focused($isDescriptionTextFocused)
									
									if meetingViewModel.description.isEmpty {
										Text("meeting_schedule_description_hint")
											.padding(.leading, 5)
											.foregroundStyle(Color.gray300)
											.default_text_style(styleSize: 15)
									}
								}
								.onTapGesture {
									isDescriptionTextFocused = true
								}
							}
						}.frame(maxHeight: 200)
						
						Rectangle()
							.foregroundStyle(.clear)
							.frame(height: 1)
							.background(Color.gray200)
						
						VStack {
							NavigationLink(destination: {
								AddParticipantsFragment(addParticipantsViewModel: addParticipantsViewModel, confirmAddParticipantsFunc: meetingViewModel.addParticipants, dismissOnCheckClick: true)
									.onAppear {
										addParticipantsViewModel.participantsToAdd = meetingViewModel.participants
									}
							}, label: {
								HStack(alignment: .center, spacing: 10) {
									Image("users")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c600)
										.frame(width: 24, height: 24)
										.padding(.leading, 16)
									
									Text("meeting_schedule_add_participants_title")
										.default_text_style_700(styleSize: 16)
										.frame(height: 29, alignment: .leading)
									Spacer()
								}
							})
							
							if !meetingViewModel.participants.isEmpty {
								ScrollView {
									ForEach(0..<meetingViewModel.participants.count, id: \.self) { index in
										VStack {
											HStack {
												Avatar(contactAvatarModel: meetingViewModel.participants[index].avatarModel, avatarSize: 50)
													.padding(.leading, 20)
												
												Text(meetingViewModel.participants[index].avatarModel.name)
													.default_text_style(styleSize: 16)
													.frame(maxWidth: .infinity, alignment: .leading)
												Spacer()
												Button(action: {
													meetingViewModel.participants.remove(at: index)
												}, label: {
													Image("x")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(Color.orangeMain500)
														.frame(width: 25, height: 25)
														.padding(.trailing, 16)
												})
											}
										}
									}
								}.frame(maxHeight: .infinity)
							}
						}
						
						if !CorePreferences.disableChatFeature {
							Rectangle()
								.foregroundStyle(.clear)
								.frame(height: 1)
								.background(Color.gray200)
							
							HStack(spacing: 10) {
								Toggle("", isOn: $meetingViewModel.sendInvitations)
									.padding(.leading, 16)
									.labelsHidden()
									.tint(Color.orangeMain300)
								Text("meeting_schedule_send_invitations_title")
									.default_text_style_500(styleSize: 14)
								Spacer()
							}
						}
						
						Spacer()
					}
					.background(.white)
				}.onTapGesture {
					isDescriptionTextFocused = false
					isSubjectTextFocused = false
				}
				
				Button {
					withAnimation {
						meetingViewModel.schedule()
					}
				} label: {
					Image("check")
						.renderingMode(.template)
						.foregroundStyle(.white)
						.padding()
						.background(Color.orangeMain500)
						.clipShape(Circle())
						.shadow(color: .black.opacity(0.2), radius: 4)
					
				}
				.padding()
				
				if meetingViewModel.operationInProgress {
					HStack {
						Spacer()
						VStack {
							Spacer()
							ProgressView()
								.progressViewStyle(CircularProgressViewStyle(tint: .orangeMain500))
								.scaleEffect(3.0, anchor: .center) // Makes the spinner larger
							Spacer()
						}
						Spacer()
					}.onDisappear {
						withAnimation {
							if meetingViewModel.conferenceCreatedEvent {
								meetingsListViewModel.computeMeetingsList()
								isShowScheduleMeetingFragment.toggle()
							}
						}
					}
				}
				
				if showDatePicker {
					getDatePopup(isTimeSelection: false)
				}
				
				if showTimePicker {
					getDatePopup(isTimeSelection: true)
				}
				
				if showTimeZonePicker {
					GeometryReader { geometry in
						ScrollViewReader { proxyReader in
							List(0..<meetingViewModel.knownTimezones.count, id: \.self) { idx in
								HStack {
									Text(TimeZone.init(identifier: meetingViewModel.knownTimezones[idx])?.formattedString() ?? "Unknown timezone")
										.fontWeight(meetingViewModel.selectedTimezoneIdx == idx ? Font.Weight.bold : Font.Weight.regular)
										.frame(maxWidth: .infinity, alignment: .leading)
								}
								.contentShape(Rectangle())
								.onTapGesture {
									if let timeZone = TimeZone.init(identifier: meetingViewModel.knownTimezones[idx]) {
										meetingViewModel.selectedTimezoneIdx = idx
										meetingViewModel.updateTimezone(timeZone: timeZone)
										showTimeZonePicker = false
									}
								}
								.id(idx)
								.background(.white)
							}
							.frame(width: geometry.size.width - 30, height: 300)
							.cornerRadius(20)
							.onAppear {
								proxyReader.scrollTo(meetingViewModel.selectedTimezoneIdx)
							}
							.background(.white)
							.cornerRadius(20)
							.listStyle(.plain)
						}
						.frame(width: geometry.size.width, height: geometry.size.height)
						.background(.black.opacity(0.65))
						.onTapGesture {
							showTimeZonePicker = false
						}
					}
				}
				
				/*
				 Picker(selection: $meetingViewModel.selectedTimezoneIdx, label: EmptyView()	) {
				 ForEach(0..<meetingViewModel.knownTimezones.count, id: \.self) { idx in
				 Text(TimeZone.init(identifier: meetingViewModel.knownTimezones[idx])?.formattedString() ?? "Unknown timezone").tag(idx)
				 }
				 }
				 .onReceive(meetingViewModel.$selectedTimezoneIdx) { value in
				 if let timeZone = TimeZone.init(identifier: meetingViewModel.knownTimezones[value]) {
				 meetingViewModel.updateTimezone(timeZone: timeZone)
				 } else {
				 Log.error("Could not find matching timezone for index \(value)")
				 }
				 }
				 .pickerStyle(MenuPickerStyle())
				 .tint(Color.grayMain2c800)
				 .padding(.leading, -10)
				 */
			}
			.navigationTitle("")
			.navigationBarHidden(true)
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
	
	@ViewBuilder
	func getDatePopup(isTimeSelection: Bool) -> some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text(setFromDate ?
					 (isTimeSelection ? String(localized: "meeting_schedule_pick_start_time_title") : String(localized: "meeting_schedule_pick_start_date_title"))
					 : String(localized: "meeting_schedule_pick_end_time_title"))
					.default_text_style_800(styleSize: 16)
					.frame(alignment: .leading)
					.padding(.bottom, 2)
				
				DatePicker(
					"",
					selection: $selectedDate,
					in: Date.now...,
					displayedComponents: isTimeSelection ? [.hourAndMinute] : [.date]
				)
				.if(isTimeSelection) { view in
					view.datePickerStyle(.wheel)
				}
				.datePickerStyle(.graphical)
				.tint(Color.orangeMain500)
				.padding(.bottom, 20)
				.default_text_style(styleSize: 15)
				
				HStack {
					Spacer()
					Text("dialog_cancel")
						.default_text_style_orange_500(styleSize: 16)
						.onTapGesture {
							if isTimeSelection {
								showTimePicker.toggle()
							} else {
								showDatePicker.toggle()
							}
						}
					Text("dialog_ok")
						.default_text_style_orange_500(styleSize: 16)
						.onTapGesture {
							pickDate()
							if isTimeSelection {
								showTimePicker.toggle()
							} else {
								showDatePicker.toggle()
							}
						}
				}
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 20)
			.background(.white)
			.cornerRadius(20)
			.padding(.horizontal)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
			// .frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
		}
		.background(.black.opacity(0.65))
	}

	func pickDate() {
		let duration = min(meetingViewModel.fromDate.distance(to: meetingViewModel.toDate), 86400) // Limit auto correction of dates to 24h
		if setFromDate {
			meetingViewModel.fromDate = selectedDate
			// If new startdate is after previous end date, bump up the end date
			if selectedDate > meetingViewModel.toDate {
				meetingViewModel.toDate = Calendar.current.date(byAdding: .second, value: Int(duration), to: selectedDate)!
			}
		} else {
			meetingViewModel.toDate = selectedDate
			if selectedDate < meetingViewModel.fromDate {
				// If new end date is before the previous start date, bump down the start date to the earlier possible from current time
				if Date.now.distance(to: selectedDate) < duration {
					meetingViewModel.fromDate = Date.now
				} else {
					meetingViewModel.fromDate = Calendar.current.date(byAdding: .second, value: (-1)*Int(duration), to: selectedDate)!
				}
			}
		}
		meetingViewModel.computeDateLabels()
		meetingViewModel.computeTimeLabels()
	}
	
	@Sendable private func delayColor() async {
		try? await Task.sleep(nanoseconds: 250_000_000)
		delayedColor = Color.orangeMain500
	}
	
	func delayColorDismiss() {
		Task {
			try? await Task.sleep(nanoseconds: 80_000_000)
			delayedColor = .white
		}
	}
}

#Preview {
	ScheduleMeetingFragment(isShowScheduleMeetingFragment: .constant(true))
		
}

// swiftlint:enable type_body_length
