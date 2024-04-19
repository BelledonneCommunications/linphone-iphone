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

// swiftlint:disable line_length

import SwiftUI

struct ScheduleMeetingFragment: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var scheduleMeetingViewModel: ScheduleMeetingViewModel
	
	@State private var delayedColor = Color.white
	@State private var showDatePicker = false
	@State private var showTimePicker = false
	
	@Binding var isShowScheduleMeetingFragment: Bool
	
	@State var selectedDate = Date.now
	@State var setFromDate: Bool = true
	@State var selectedHours: Int = 0
	@State var selectedMinutes: Int = 0
	
	var body: some View {
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				VStack(spacing: 16) {
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
							.padding(.top, 2)
							.padding(.leading, -10)
							.onTapGesture {
								withAnimation {
									isShowScheduleMeetingFragment.toggle()
								}
							}
						
						Text("New meeting" )
							.multilineTextAlignment(.leading)
							.default_text_style_orange_800(styleSize: 16)
						
						Spacer()
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					/*
					 HStack {
					 Spacer()
					 HStack(alignment: .center) {
					 Button(action: {
					 scheduleMeetingViewModel.isBroadcastSelected.toggle()
					 }, label: {
					 Image("users-three")
					 .renderingMode(.template)
					 .resizable()
					 .foregroundStyle(scheduleMeetingViewModel.isBroadcastSelected ? .white : Color.orangeMain500)
					 .frame(width: 25, height: 25)
					 })
					 Text("Meeting")
					 .default_text_style_orange_500( styleSize: 15)
					 .foregroundStyle(scheduleMeetingViewModel.isBroadcastSelected ? .white : Color.orangeMain500)
					 }
					 .padding(.horizontal, 40)
					 .padding(.vertical, 10)
					 .cornerRadius(60)
					 .overlay(
					 RoundedRectangle(cornerRadius: 60)
					 .inset(by: 0.5)
					 .stroke(Color.orangeMain500, lineWidth: 1)
					 .background(scheduleMeetingViewModel.isBroadcastSelected ? Color.orangeMain500 : Color.white)
					 )
					 Spacer()
					 
					 HStack(alignment: .center) {
					 Button(action: {
					 scheduleMeetingViewModel.isBroadcastSelected.toggle()
					 }, label: {
					 Image("slideshow")
					 .renderingMode(.template)
					 .resizable()
					 .foregroundStyle(Color.orangeMain500)
					 .frame(width: 25, height: 25)
					 })
					 
					 Text("Broadcast")
					 .default_text_style_orange_500( styleSize: 15)
					 }
					 .padding(.horizontal, 40)
					 .padding(.vertical, 10)
					 .cornerRadius(60)
					 .overlay(
					 RoundedRectangle(cornerRadius: 60)
					 .inset(by: 0.5)
					 .stroke(Color.orangeMain500, lineWidth: 1)
					 )
					 Spacer()
					 }
					 */
					HStack(alignment: .center, spacing: 8) {
						Image("users-three")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c600)
							.frame(width: 24, height: 24)
							.padding(.leading, 16)
						TextField("Subject", text: $scheduleMeetingViewModel.subject)
							.default_text_style_700(styleSize: 20)
							.frame(height: 29, alignment: .leading)
						Spacer()
					}
					
					Rectangle()
						.foregroundStyle(.clear)
						.frame(height: 1)
						.background(Color.gray200)
					
					HStack(alignment: .center, spacing: 8) {
						Image("clock")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c800)
							.frame(width: 24, height: 24)
							.padding(.leading, 16)
						Text(scheduleMeetingViewModel.fromDateStr)
							.fontWeight(.bold)
							.default_text_style_500(styleSize: 16)
							.onTapGesture {
								setFromDate = true
								selectedDate = scheduleMeetingViewModel.fromDate
								showDatePicker.toggle()
							}
						Spacer()
					}
					
					if !scheduleMeetingViewModel.allDayMeeting {
						HStack(spacing: 8) {
							Text(scheduleMeetingViewModel.fromTime)
								.fontWeight(.bold)
								.padding(.leading, 48)
								.frame(height: 29, alignment: .leading)
								.default_text_style_500(styleSize: 16)
								.opacity(scheduleMeetingViewModel.allDayMeeting ? 0 : 1)
								.onTapGesture {
									setFromDate = true
									selectedDate = scheduleMeetingViewModel.fromDate
									showTimePicker.toggle()
								}
							Text(scheduleMeetingViewModel.toTime)
								.fontWeight(.bold)
								.padding(.leading, 8)
								.frame(height: 29, alignment: .leading)
								.default_text_style_500(styleSize: 16)
								.opacity(scheduleMeetingViewModel.allDayMeeting ? 0 : 1)
								.onTapGesture {
									setFromDate = false
									selectedDate = scheduleMeetingViewModel.toDate
									showTimePicker.toggle()
								}
							Spacer()
							Toggle("", isOn: $scheduleMeetingViewModel.allDayMeeting)
								.labelsHidden()
								.tint(Color.orangeMain300)
							Text("All day")
								.default_text_style_500(styleSize: 16)
								.padding(.trailing, 16)
						}
					} else {
						HStack(alignment: .center, spacing: 8) {
							Image("clock")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 24, height: 24)
								.padding(.leading, 16)
							Text(scheduleMeetingViewModel.toDateStr)
								.fontWeight(.bold)
								.default_text_style_500(styleSize: 16)
								.onTapGesture {
									setFromDate = false
									selectedDate = scheduleMeetingViewModel.toDate
									showDatePicker.toggle()
								}
							Spacer()
							Toggle("", isOn: $scheduleMeetingViewModel.allDayMeeting)
								.labelsHidden()
								.tint(Color.orangeMain300)
							Text("All day")
								.default_text_style_500(styleSize: 16)
								.padding(.trailing, 16)						}
					}
					HStack(alignment: .center, spacing: 8) {
						Image("earth")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c800)
							.frame(width: 24, height: 24)
							.padding(.leading, 16)
						Text("TODO : timezone")
							.fontWeight(.bold)
							.padding(.leading, 8)
							.default_text_style_500(styleSize: 16)
						Spacer()
					}
					
					HStack(alignment: .center, spacing: 8) {
						Image("arrow-clockwise")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c800)
							.frame(width: 24, height: 24)
							.padding(.leading, 16)
						Text("TODO : repeat")
							.fontWeight(.bold)
							.padding(.leading, 8)
							.default_text_style_500(styleSize: 16)
						Spacer()
					}
					
					Rectangle()
						.foregroundStyle(.clear)
						.frame(height: 1)
						.background(Color.gray200)
					
					HStack(alignment: .top, spacing: 8) {
						Image("note")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c600)
							.frame(width: 24, height: 24)
							.padding(.leading, 16)
						
						TextField("Add a description", text: $scheduleMeetingViewModel.description)
							.default_text_style_700(styleSize: 16)
					}
					
					Rectangle()
						.foregroundStyle(.clear)
						.frame(height: 1)
						.background(Color.gray200)
					
					VStack {
						NavigationLink(destination: {
							AddParticipantsFragment(scheduleMeetingViewModel: scheduleMeetingViewModel)
						}, label: {
							HStack(alignment: .center, spacing: 8) {
								Image("users")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 24, height: 24)
									.padding(.leading, 16)
								
								Text("Add participants")
									.default_text_style_700(styleSize: 16)
									.frame(height: 29, alignment: .leading)
								Spacer()
							}
						})
						if !scheduleMeetingViewModel.participants.isEmpty {
							ScrollView {
								ForEach(0..<scheduleMeetingViewModel.participants.count, id: \.self) { index in
									VStack {
										HStack {
											Avatar(contactAvatarModel: scheduleMeetingViewModel.participants[index].avatarModel, avatarSize: 50)
												.padding(.leading, 66)
											
											Text(scheduleMeetingViewModel.participants[index].avatarModel.name)
												.default_text_style(styleSize: 16)
												.frame(maxWidth: .infinity, alignment: .leading)
											Spacer()
											Button(action: {
												scheduleMeetingViewModel.participants.remove(at: index)
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
							}.frame(maxHeight: 170)
						}
					}
					Rectangle()
						.foregroundStyle(.clear)
						.frame(height: 1)
						.background(Color.gray200)
					
					HStack(spacing: 8) {
						Toggle("", isOn: $scheduleMeetingViewModel.sendInvitations)
							.padding(.leading, 16)
							.labelsHidden()
							.tint(Color.orangeMain300)
						Text("Send invitations to participants")
							.default_text_style_500(styleSize: 14)
						Spacer()
					}
					Spacer()
				}
				.background(.white)
				
				Button {
					withAnimation {
						scheduleMeetingViewModel.schedule()
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
				if scheduleMeetingViewModel.operationInProgress {
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
							if scheduleMeetingViewModel.conferenceCreatedEvent {
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
			}
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
	
	func getDatePopup(isTimeSelection: Bool) -> some View {
		return GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text("Select \(setFromDate ? "start" : "end") \(isTimeSelection ? "time" : "date")")
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
					Text("Cancel")
						.default_text_style_orange_500(styleSize: 16)
						.onTapGesture {
							if isTimeSelection {
								showTimePicker.toggle()
							} else {
								showDatePicker.toggle()
							}
						}
					Text("Ok")
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
			//.frame(maxWidth: sharedMainViewModel.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
		}
		.background(.black.opacity(0.65))
	}

	func pickDate() {
		let duration = min(scheduleMeetingViewModel.fromDate.distance(to: scheduleMeetingViewModel.toDate), 86400) // Limit auto correction of dates to 24h
		if setFromDate {
			scheduleMeetingViewModel.fromDate = selectedDate
			// If new startdate is after previous end date, bump up the end date
			if selectedDate > scheduleMeetingViewModel.toDate {
				scheduleMeetingViewModel.toDate = Calendar.current.date(byAdding: .second, value: Int(duration), to: selectedDate)!
			}
		} else {
			scheduleMeetingViewModel.toDate = selectedDate
			if selectedDate < scheduleMeetingViewModel.fromDate {
				// If new end date is before the previous start date, bump down the start date to the earlier possible from current time
				if (Date.now.distance(to: selectedDate) < duration) {
					scheduleMeetingViewModel.fromDate = Date.now
				} else {
					scheduleMeetingViewModel.fromDate = Calendar.current.date(byAdding: .second, value: (-1)*Int(duration), to: selectedDate)!
				}
			}
		}
		scheduleMeetingViewModel.computeDateLabels()
		scheduleMeetingViewModel.computeTimeLabels()
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
	ScheduleMeetingFragment(scheduleMeetingViewModel: ScheduleMeetingViewModel()
							, isShowScheduleMeetingFragment: .constant(true))
}

// swiftlint:enable line_length
