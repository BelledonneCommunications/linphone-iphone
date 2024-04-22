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
	
	var body: some View {
		VStack {
			List {
				ForEach(0..<meetingsListViewModel.meetingsList.count, id: \.self) { index in
					HStack {
						HStack {
							Image("users-three-square")
								   .renderingMode(.template)
								   .resizable()
								   .frame(width: 28, height: 28)
								   .foregroundStyle(Color.grayMain2c600)
							
							VStack(spacing: 0) {
								Text(meetingsListViewModel.meetingsList[index].model?.subject ?? "")
									.default_text_style_500(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
							}
						}
						.frame(height: 40)
					}
					.buttonStyle(.borderless)
					.listRowInsets(EdgeInsets(top: 5, leading: 20, bottom: 5, trailing: 20))
					.listRowSeparator(.hidden)
					.background(.white)
					.onTapGesture {
						withAnimation {
							joinMeetingWaitingRoom(index: index)
						}
					}
					.onLongPressGesture(minimumDuration: 0.2) {
						//showingSheet.toggle()
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
