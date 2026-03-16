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
import UniformTypeIdentifiers

struct SettingsAdvancedCallFragment: View {
	@ObservedObject var settingsViewModel: SettingsViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State var earlyMediaIsOpen: Bool = false
	@State var audioCodecsIsOpen: Bool = false
	@State var videoCodecsIsOpen: Bool = false
	
	var body: some View {
		ZStack {
			VStack(spacing: 1) {
				Rectangle()
					.foregroundColor(Color.orangeMain500)
					.edgesIgnoringSafeArea(.top)
					.frame(height: 0)
				
				HStack {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.orangeMain500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
						.padding(.top, 4)
						.padding(.leading, -10)
						.onTapGesture {
							dismiss()
						}
					
					Text("settings_advanced_calls")
						.default_text_style_orange_800(styleSize: 16)
						.frame(maxWidth: .infinity, alignment: .leading)
						.padding(.top, 4)
						.lineLimit(1)
					
					Spacer()
				}
				.frame(maxWidth: .infinity)
				.frame(height: 50)
				.padding(.horizontal)
				.padding(.bottom, 4)
				.background(.white)
				
				ScrollView {
					VStack(spacing: 0) {
						VStack(spacing: 0) {
							VStack(spacing: 30) {
								Toggle("settings_calls_enable_fec_title", isOn: $settingsViewModel.enableFec)
									.default_text_style_700(styleSize: 15)
								
								VStack(alignment: .leading) {
									Text("call_stats_media_encryption_title")
										.default_text_style_700(styleSize: 15)
										.padding(.bottom, -5)
									
									Menu {
										Button("None") {
											settingsViewModel.mediaEncryption = "None"
											settingsViewModel.mediaEncryptionMandatory = false
										}
										Button("SRTP") {
											settingsViewModel.mediaEncryption = "SRTP"
											settingsViewModel.mediaEncryptionMandatory = true
										}
										Button("ZRTP") {
											settingsViewModel.mediaEncryption = "ZRTP"
											settingsViewModel.mediaEncryptionMandatory = true
										}
										Button("DTLS") {
											settingsViewModel.mediaEncryption = "DTLS"
											settingsViewModel.mediaEncryptionMandatory = true
										}
									} label: {
										Text(settingsViewModel.mediaEncryption)
											.default_text_style(styleSize: 15)
											.frame(maxWidth: .infinity, alignment: .leading)
										Image("caret-down")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 20, height: 20)
									}
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(Color.gray200, lineWidth: 1)
									)
								}
								
								Toggle("settings_advanced_media_encryption_mandatory_title", isOn: $settingsViewModel.mediaEncryptionMandatory)
									.default_text_style_700(styleSize: 15)
							}
							.padding(.vertical, 20)
							.padding(.horizontal, 20)
							.background(.white)
							.cornerRadius(15)
							.background(Color.gray100)
						}
						.padding(.vertical, 20)
						.padding(.horizontal, 20)
						.background(Color.gray100)
						
						HStack(alignment: .center) {
							Text("settings_advanced_early_media_title")
								.default_text_style_800(styleSize: 18)
								.frame(maxWidth: .infinity, alignment: .leading)
							
							Spacer()
							
							Image(earlyMediaIsOpen ? "caret-up" : "caret-down")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
						}
						.padding(.vertical, 10)
						.padding(.horizontal, 20)
						.background(Color.gray100)
						.onTapGesture {
							withAnimation {
								earlyMediaIsOpen.toggle()
							}
						}
						
						if earlyMediaIsOpen {
							VStack(spacing: 0) {
								VStack(spacing: 30) {
									Toggle("settings_advanced_accept_early_media_title", isOn: $settingsViewModel.acceptEarlyMedia)
										.default_text_style_700(styleSize: 15)
									
									Toggle("settings_advanced_allow_outgoing_early_media_title", isOn: $settingsViewModel.allowOutgoingEarlyMedia)
										.default_text_style_700(styleSize: 15)
								}
								.padding(.vertical, 30)
								.padding(.horizontal, 20)
							}
							.background(.white)
							.cornerRadius(15)
							.padding(.horizontal, 20)
							.zIndex(-1)
							.transition(.move(edge: .top))
							.background(Color.gray100)
						}
						
						HStack(alignment: .center) {
							Text("settings_advanced_audio_codecs_title")
								.default_text_style_800(styleSize: 18)
								.frame(maxWidth: .infinity, alignment: .leading)
							
							Spacer()
							
							Image(audioCodecsIsOpen ? "caret-up" : "caret-down")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
						}
						.padding(.vertical, 10)
						.padding(.horizontal, 20)
						.background(Color.gray100)
						.onTapGesture {
							withAnimation {
								audioCodecsIsOpen.toggle()
							}
						}
						
						if audioCodecsIsOpen {
							VStack(spacing: 0) {
								VStack(spacing: 30) {
									ForEach(settingsViewModel.audioCodecs) { audioCodec in
										SettingsToggleWidget(title: audioCodec.mimeType, subtitle: audioCodec.subtitle, isOn: Binding(
											get: { audioCodec.isEnabled },
											set: { newValue in
												audioCodec.toggleEnabled()
											}
										))
									}
								}
								.padding(.vertical, 30)
								.padding(.horizontal, 20)
							}
							.background(.white)
							.cornerRadius(15)
							.padding(.horizontal, 20)
							.zIndex(-2)
							.transition(.move(edge: .top))
							.background(Color.gray100)
						}
						
						HStack(alignment: .center) {
							Text("settings_advanced_video_codecs_title")
								.default_text_style_800(styleSize: 18)
								.frame(maxWidth: .infinity, alignment: .leading)
							
							Spacer()
							
							Image(videoCodecsIsOpen ? "caret-up" : "caret-down")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
						}
						.padding(.vertical, 10)
						.padding(.horizontal, 20)
						.background(Color.gray100)
						.onTapGesture {
							withAnimation {
								videoCodecsIsOpen.toggle()
							}
						}
						
						if videoCodecsIsOpen {
							VStack(spacing: 0) {
								VStack(spacing: 30) {
									ForEach(settingsViewModel.videoCodecs) { videoCodec in
										SettingsToggleWidget(title: videoCodec.mimeType, subtitle: videoCodec.subtitle, isOn: Binding(
											get: { videoCodec.isEnabled },
											set: { newValue in
												videoCodec.toggleEnabled()
											}
										))
									}
								}
								.padding(.vertical, 30)
								.padding(.horizontal, 20)
							}
							.background(.white)
							.cornerRadius(15)
							.padding(.horizontal, 20)
							.zIndex(-3)
							.transition(.move(edge: .top))
							.background(Color.gray100)
						}
					}
					.background(Color.gray100)
					.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
					.frame(maxWidth: .infinity)
				}
				.background(Color.gray100)
			}
			.background(Color.gray100)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}
