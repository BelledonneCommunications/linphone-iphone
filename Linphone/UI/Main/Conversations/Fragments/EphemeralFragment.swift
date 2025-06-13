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
import linphonesw

struct EphemeralFragment: View {
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	
	@State private var selectedOption = NSLocalizedString("conversation_ephemeral_messages_duration_disabled", comment: "")
	let options = [
		NSLocalizedString("conversation_ephemeral_messages_duration_one_minute", comment: ""),
		NSLocalizedString("conversation_ephemeral_messages_duration_one_hour", comment: ""),
		NSLocalizedString("conversation_ephemeral_messages_duration_one_day", comment: ""),
		NSLocalizedString("conversation_ephemeral_messages_duration_three_days", comment: ""),
		NSLocalizedString("conversation_ephemeral_messages_duration_one_week", comment: ""),
		NSLocalizedString("conversation_ephemeral_messages_duration_disabled", comment: "")
	]
	
	@Binding var isShowEphemeralFragment: Bool
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				ZStack {
					VStack(spacing: 1) {
						
						Rectangle()
							.foregroundStyle(Color.orangeMain500)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
						
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
										isShowEphemeralFragment = false
										conversationViewModel.setEphemeralTime(lifetimeString: selectedOption)
									}
								}
							
							Text("conversation_ephemeral_messages_title")
								.multilineTextAlignment(.leading)
								.default_text_style_orange_800(styleSize: 16)
							
							Spacer()
							
						}
						.frame(maxWidth: .infinity)
						.frame(height: 50)
						.padding(.horizontal)
						.padding(.bottom, 4)
						.background(.white)
						
						VStack(spacing: 0) {
							ScrollView {
								VStack(spacing: 20) {
									Image("ephemeral")
										.resizable()
										.scaledToFit()
										.frame(width: geometry.size.width/2.5)
									
									Text("conversation_ephemeral_messages_subtitle")
										.default_text_style(styleSize: 14)
										.multilineTextAlignment(.center)
									
									VStack {
										ForEach(options, id: \.self) { option in
											Button(action: {
												selectedOption = option
											}) {
												VStack {
													HStack {
														Image(selectedOption == option ? "radio-button-fill" : "radio-button")
														
														Text(option)
															.default_text_style(styleSize: 14)
															.frame(maxWidth: .infinity, alignment: .leading)
													}
													.padding(.top, 2)
													
													if option != NSLocalizedString("conversation_ephemeral_messages_duration_disabled", comment: "") {
														Divider()
													}
												}
												.background(.white)
											 	.frame(maxWidth: .infinity)
											}
											.background(.white)
											.frame(maxWidth: .infinity)
											.buttonStyle(PlainButtonStyle())
										}
									}
									.padding()
									.background(.white)
									.cornerRadius(10)
								}
								.padding(.horizontal, 10)
								.frame(maxWidth: .infinity)
								.padding(.vertical, 20)
								.padding(.horizontal, 10)
							}
							.frame(maxWidth: .infinity)
						}
						.frame(maxWidth: .infinity)
					}
					.background(Color.gray100)
				}
				.navigationTitle("")
				.navigationBarHidden(true)
				.onAppear {
					conversationViewModel.getEphemeralTime()
					selectedOption = conversationViewModel.ephemeralTime
				}
				.onChange(of: conversationViewModel.ephemeralTime) { _ in
					selectedOption = conversationViewModel.ephemeralTime
				}
				.onDisappear {
					withAnimation {
						isShowEphemeralFragment = false
					}
				}
			}
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
}

#Preview {
	EphemeralFragment(
		isShowEphemeralFragment: .constant(true)
	)
}
