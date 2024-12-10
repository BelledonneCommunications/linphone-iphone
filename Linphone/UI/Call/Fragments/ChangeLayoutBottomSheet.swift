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

struct ChangeLayoutBottomSheet: View {
	@Environment(\.dismiss) private var dismiss
	
	@ObservedObject var callViewModel: CallViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var changeLayoutSheet: Bool
	@Binding var optionsChangeLayout: Int
	
	var body: some View {
		VStack(spacing: 0) {
			Button(action: {
				optionsChangeLayout = 1
				callViewModel.toggleVideoMode(isAudioOnlyMode: false)
				changeLayoutSheet = false
				dismiss()
			}, label: {
				HStack {
					Image(optionsChangeLayout == 1 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(callViewModel.participantList.count > 5 ? Color.gray500 : .white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text("conference_layout_grid")
						.foregroundStyle(callViewModel.participantList.count > 5 ? Color.gray500 : .white)
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image("squares-four")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(callViewModel.participantList.count > 5 ? Color.gray500 : .white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.disabled(callViewModel.participantList.count > 5)
			.frame(maxHeight: .infinity)
			
			Button(action: {
				optionsChangeLayout = 2
				callViewModel.toggleVideoMode(isAudioOnlyMode: false)
				changeLayoutSheet = false
				dismiss()
			}, label: {
				HStack {
					Image(optionsChangeLayout == 2 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text("conference_layout_active_speaker")
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image("picture-in-picture")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.frame(maxHeight: .infinity)
			
			Button(action: {
				optionsChangeLayout = 3
				if callViewModel.videoDisplayed {
					callViewModel.displayMyVideo()
				}
				callViewModel.toggleVideoMode(isAudioOnlyMode: true)
				changeLayoutSheet = false
				dismiss()
			}, label: {
				HStack {
					Image(optionsChangeLayout == 3 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text("conference_layout_audio_only")
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image("waveform")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.frame(maxHeight: .infinity)
		}
		.padding(.horizontal, 20)
		.background(Color.gray600)
		.frame(maxHeight: .infinity)
	}
}
