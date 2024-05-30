//
//  MeetingsFragment.swift
//  Linphone
//
//  Created by QuentinArguillere on 18/04/2024.
//

import SwiftUI
import linphonesw

struct MeetingsFragment: View {
	
	@ObservedObject var meetingsListViewModel: MeetingsListViewModel
	@ObservedObject var scheduleMeetingViewModel: ScheduleMeetingViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@State var showingSheet: Bool = false
	
	func createMonthLine(model: MeetingsListItemModel) -> some View {
		return Text(model.monthStr)
			.fontWeight(.bold)
			.padding(5)
			.default_text_style_500(styleSize: 22)
	}
	
	func createWeekLine(model: MeetingsListItemModel) -> some View {
		return Text(model.weekStr)
			.padding(.leading, 43)
			.padding(.top, 3)
			.padding(.bottom, 3)
			.default_text_style_500(styleSize: 14)
	}
	
	func createMeetingLine(model: MeetingsListItemModel) -> some View {
		return VStack(alignment: .leading) {
			if model.isToday {
				Text("No meeting today")
					.fontWeight(.bold)
					.default_text_style_500(styleSize: 15)
			} else {
				HStack(alignment: .center) {
					Image("meetings")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 24, height: 24)
						.padding(.bottom, -8)
					Text(model.model!.subject)
						.fontWeight(.bold)
						.padding(.trailing, 5)
						.padding(.top, 7)
						.default_text_style_500(styleSize: 15)
				}
				Text(model.model!.time)
					.padding(.top, -8)
					.default_text_style_500(styleSize: 15)
			}
		}
		.padding(.leading, 20)
		.frame(height: 63)
		.frame(maxWidth: .infinity, alignment: .leading)
		.background(.white)
		.clipShape(RoundedRectangle(cornerRadius: 20))
		.shadow(color: .black.opacity(0.2), radius: 4)
		.onTapGesture {
			do {
				let meetingAddress = try Factory.Instance.createAddress(addr: model.model?.address ?? "")
				TelecomManager.shared.meetingWaitingRoomDisplayed = true
				TelecomManager.shared.meetingWaitingRoomSelected = meetingAddress
			} catch {
				Log.error("[MeetingsFragment] Couldn't create address from \(model.model?.address ?? "")")
			}
		}
	}
	
	var body: some View {
		VStack {
			List {
				VStack(alignment: .leading) {
					ForEach(0..<meetingsListViewModel.meetingsList.count, id: \.self) { index in
						let itemModel = meetingsListViewModel.meetingsList[index]
						if index == 0 || itemModel.monthStr != meetingsListViewModel.meetingsList[index-1].monthStr {
							createMonthLine(model: meetingsListViewModel.meetingsList[index])
							if index == 0 || itemModel.weekStr != meetingsListViewModel.meetingsList[index-1].weekStr {
								createWeekLine(model: itemModel)
							}
						}
						
						if index == 0
							|| itemModel.dayStr != meetingsListViewModel.meetingsList[index-1].dayStr
							|| itemModel.weekStr != meetingsListViewModel.meetingsList[index-1].weekStr {
							HStack {
								VStack(alignment: .center) {
									Text(itemModel.weekDayStr)
										.padding(.bottom, -5)
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
											.padding(.top, -3)
											.default_text_style_300(styleSize: 20)
									}
									/*
									Image("check")
										.renderingMode(.template)
										.foregroundStyle(.white)
										.padding()
										.background(Color.orangeMain500)
										.clipShape(Circle())
										.shadow(color: .black.opacity(0.2), radius: 4)
									*/
								}
								.padding(.top, -5)
								.frame(width: 35)
								if itemModel.isToday {
									Text("No meeting today")
										.fontWeight(.bold)
										.padding(.leading, 20)
										.default_text_style_500(styleSize: 15)
								} else {
									createMeetingLine(model: itemModel)
								}
							}
						} else {
							createMeetingLine(model: itemModel)
								.padding(.leading, 43)
								.padding(.top, -10)
						}
					}
				}
			}
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
						Text("No meeting for the moment...")
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
	
	func joinMeetingWaitingRoom(index: Int) {
		do {
			let meetingAddress = try Factory.Instance.createAddress(addr: meetingsListViewModel.meetingsList[index].model?.address ?? "")
			
			TelecomManager.shared.meetingWaitingRoomDisplayed = true
			TelecomManager.shared.meetingWaitingRoomSelected = meetingAddress
		} catch {}
	}
}

#Preview {
	MeetingsFragment(meetingsListViewModel: MeetingsListViewModel(), scheduleMeetingViewModel: ScheduleMeetingViewModel())
}
