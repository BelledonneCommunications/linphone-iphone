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

struct MeetingsFragment: View {
    
    @EnvironmentObject var meetingsListViewModel: MeetingsListViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@Binding var showingSheet: Bool
	@Binding var text: String
	
	@ViewBuilder
	func createMonthLine(model: MeetingsListItemModel) -> some View {
		Text(model.monthStr)
			.fontWeight(.bold)
			.padding(5)
			.default_text_style_500(styleSize: 22)
	}
	
	@ViewBuilder
	func createWeekLine(model: MeetingsListItemModel) -> some View {
		Text(model.weekStr)
			.padding(.leading, 50)
			.padding(.vertical, 10)
			.default_text_style_500(styleSize: 14)
	}
	@ViewBuilder
	func createMeetingLine(model: MeetingsListItemModel) -> some View {
		VStack(alignment: .leading, spacing: 0) {
			if model.isToday {
				Text("meetings_list_no_meeting_for_today")
					.fontWeight(.bold)
					.default_text_style_500(styleSize: 15)
			} else {
				HStack(alignment: .center) {
					Image("video-conference")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 24, height: 24)
						.padding(.bottom, -5)
					Text(model.model!.subject)
						.fontWeight(.bold)
						.padding(.trailing, 5)
						.padding(.top, 5)
						.default_text_style_500(styleSize: 15)
				}
				if model.model!.confInfo.state != ConferenceInfo.State.Cancelled {
					Text(model.model!.time) 
					// this time string is formatted for the current device timezone, we use the selected timezone only when displaying details
						.default_text_style_500(styleSize: 15)
				} else {
					Text("meeting_info_cancelled_toast")
						.foregroundStyle(Color.redDanger500)
						.default_text_style_500(styleSize: 15)
				}
			}
		}
		.padding(.leading, 30)
		.frame(height: 63)
		.frame(maxWidth: .infinity, alignment: .leading)
		.background(.white)
		.clipShape(RoundedRectangle(cornerRadius: 10))
		.shadow(color: .black.opacity(0.2), radius: 4)
		.onTapGesture {
			withAnimation {
				if let meetingModel = model.model, meetingModel.confInfo.state != ConferenceInfo.State.Cancelled {
					SharedMainViewModel.shared.displayedMeeting = meetingModel
				}
			}
		}
		.onLongPressGesture(minimumDuration: 0.2) {
			if let meetingModel = model.model {
				meetingsListViewModel.selectedMeetingToDelete = meetingModel
				showingSheet.toggle()
			}
		}
	}
	
	var body: some View {
		VStack {
			ScrollViewReader { proxyReader in
				List(0..<meetingsListViewModel.meetingsList.count, id: \.self) { index in
					VStack(alignment: .leading, spacing: 0) {
						let itemModel = meetingsListViewModel.meetingsList[index]
						if index == 0 || itemModel.monthStr != meetingsListViewModel.meetingsList[index-1].monthStr {
							createMonthLine(model: meetingsListViewModel.meetingsList[index])
						}
						if index == 0 || itemModel.weekStr != meetingsListViewModel.meetingsList[index-1].weekStr {
							createWeekLine(model: itemModel)
						}
						
						if index == 0
							|| itemModel.dayStr != meetingsListViewModel.meetingsList[index-1].dayStr
							|| itemModel.weekStr != meetingsListViewModel.meetingsList[index-1].weekStr {
							HStack(alignment: .top, spacing: 0) {
								VStack(alignment: .center, spacing: 0) {
									Text(itemModel.weekDayStr)
										.default_text_style_500(styleSize: 14)
									if itemModel.isToday || Calendar.current.isDate(itemModel.model!.meetingDate, inSameDayAs: Date.now) {
										Text(itemModel.dayStr)
											.fontWeight(.bold)
											.frame(width: 30, height: 30)
											.foregroundStyle(.white)
											.background(Color.orangeMain500)
											.clipShape(Circle())
											.default_text_style_300(styleSize: 20)
									} else {
										Text(itemModel.dayStr)
											.fontWeight(.bold)
											.default_text_style_300(styleSize: 20)
									}
								}
								.frame(width: 35)
								if itemModel.isToday {
									Text("meetings_list_no_meeting_for_today")
										.fontWeight(.bold)
										.padding(.leading, 20)
										.padding(.top, 15)
										.default_text_style_500(styleSize: 15)
								} else {
									createMeetingLine(model: itemModel)
										.padding(.leading, 10)
								}
							}
						} else {
							createMeetingLine(model: itemModel)
								.padding(.leading, 45)
						}
					}
					.id(index)
					.listRowInsets(.init(top: 5, leading: 10, bottom: 5, trailing: 15))
					.listRowSeparator(.hidden)
				}
				.onAppear {
					proxyReader.scrollTo(meetingsListViewModel.todayIdx, anchor: .top)
				}
				.onReceive(NotificationCenter.default.publisher(for: MeetingsListViewModel.ScrollToTodayNotification)) { _ in
					withAnimation {
						proxyReader.scrollTo(meetingsListViewModel.todayIdx, anchor: .top)
					}
				}
				.safeAreaInset(edge: .top, content: {
					Spacer()
						.frame(height: 12)
				})
				.listStyle(.plain)
				.overlay(
					VStack {
						if meetingsListViewModel.meetingsList.isEmpty {
							Spacer()
							Image("illus-belledonne")
								.resizable()
								.scaledToFit()
								.clipped()
								.padding(.all)
							Text(!text.isEmpty ? "list_filter_no_result_found" : "meetings_list_empty")
								.default_text_style_800(styleSize: 16)
							Spacer()
							Spacer()
						}
					}
						.padding(.all)
				)
			}
			.navigationTitle("")
			.navigationBarHidden(true)
		}
	}
}

#Preview {
	MeetingsFragment(showingSheet: .constant(false), text: .constant(""))
}
