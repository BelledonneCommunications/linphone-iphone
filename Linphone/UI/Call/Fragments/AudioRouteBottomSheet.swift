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
import AVFAudio

struct AudioRouteBottomSheet: View {
	@Environment(\.dismiss) private var dismiss
	
	@ObservedObject var callViewModel: CallViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var optionsAudioRoute: Int
	
	var body: some View {
		VStack(spacing: 0) {
			Button(action: {
				optionsAudioRoute = 1
				
				do {
					try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
					if callViewModel.isHeadPhoneAvailable() {
						try AVAudioSession.sharedInstance().setPreferredInput(
							AVAudioSession.sharedInstance().availableInputs?.filter({ $0.portType.rawValue.contains("Receiver") }).first)
					} else {
						try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance().availableInputs?.first)
					}
				} catch _ {
					
				}
			}, label: {
				HStack {
					Image(optionsAudioRoute == 1 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text(!callViewModel.isHeadPhoneAvailable() ? "call_audio_device_type_earpiece" : "call_audio_device_type_headphones")
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image(!callViewModel.isHeadPhoneAvailable() ? "ear" : "headset")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.frame(maxHeight: .infinity)
			
			Button(action: {
				optionsAudioRoute = 2
				
				do {
					try AVAudioSession.sharedInstance().overrideOutputAudioPort(.speaker)
				} catch _ {
					
				}
			}, label: {
				HStack {
					Image(optionsAudioRoute == 2 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text("call_audio_device_type_speaker")
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image("speaker-high")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.frame(maxHeight: .infinity)
			
			Button(action: {
				optionsAudioRoute = 3
				
				do {
					try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
					try AVAudioSession.sharedInstance().setPreferredInput(
						AVAudioSession.sharedInstance().availableInputs?.filter({ $0.portType.rawValue.contains("Bluetooth") }).first)
				} catch _ {
					
				}
			}, label: {
				HStack {
					Image(optionsAudioRoute == 3 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text(String(format: String(localized: "call_audio_device_type_bluetooth"),
								AVAudioSession.sharedInstance().currentRoute.outputs.first?.portName ?? ""))
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image("bluetooth")
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
