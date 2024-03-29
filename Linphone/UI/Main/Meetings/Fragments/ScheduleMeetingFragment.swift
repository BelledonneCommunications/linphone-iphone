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
	
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var scheduleMeetingViewModel: ScheduleMeetingViewModel
	
	@State private var delayedColor = Color.white
	
	var body: some View {
		ZStack {
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
						.frame(width: 300, height: 29, alignment: .leading)
						.padding(.leading, 8)
						.default_text_style_500(styleSize: 16)
						.background(Color.gray200)
					Spacer()
				}
				
				HStack(alignment: .center, spacing: 8) {
					Text(scheduleMeetingViewModel.fromTime)
						.fontWeight(.bold)
						.frame(height: 29, alignment: .leading)
						.default_text_style_500(styleSize: 16)
						.opacity(scheduleMeetingViewModel.allDayMeeting ? 0 : 1)
					Text(scheduleMeetingViewModel.toTime)
						.fontWeight(.bold)
						.frame(height: 29, alignment: .leading)
						.default_text_style_500(styleSize: 16)
						.opacity(scheduleMeetingViewModel.allDayMeeting ? 0 : 1)
					Toggle("", isOn: $scheduleMeetingViewModel.allDayMeeting)
						.labelsHidden()
						.tint(Color.orangeMain300)
					Text("All day")
						.fontWeight(.bold)
						.default_text_style_500(styleSize: 16)
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
				
				HStack(alignment: .center, spacing: 8) {
					Image("note")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 24, height: 24)
						.padding(.leading, 16)
					
					TextField("Add a description", text: $scheduleMeetingViewModel.subject)
						.default_text_style_700(styleSize: 16)
						.frame(height: 29, alignment: .leading)
					Spacer()
				}
		
				Rectangle()
					.foregroundStyle(.clear)
					.frame(height: 1)
					.background(Color.gray200)
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
				ScrollView {
					
				}
				.background(Color.gray100)
			}
			.background(.white)
			.navigationBarHidden(true)
		}
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
	ScheduleMeetingFragment(scheduleMeetingViewModel: ScheduleMeetingViewModel())
}

// swiftlint:enable line_length
