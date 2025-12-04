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

struct RecordingsListFragment: View {
	
	@StateObject private var recordingsListViewModel = RecordingsListViewModel()
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@Binding var isShowRecordingsListFragment: Bool
	
	@State var showingSheet: Bool = false
    @State private var showShareSheet: Bool = false
    @State private var showPicker: Bool = false
	
	var body: some View {
		NavigationView {
			ZStack {
				if #available(iOS 16.4, *), idiom != .pad {
					innerView()
						.sheet(isPresented: $showingSheet) {
                            RecordingsListBottomSheet(showingSheet: $showingSheet, showShareSheet: $showShareSheet, showPicker: $showPicker)
								.environmentObject(recordingsListViewModel)
								.presentationDetents([.fraction(0.4)])
						}
				} else {
					innerView()
						.halfSheet(showSheet: $showingSheet) {
                            RecordingsListBottomSheet(showingSheet: $showingSheet, showShareSheet: $showShareSheet, showPicker: $showPicker)
								.environmentObject(recordingsListViewModel)
						} onDismiss: {}
				}
			}
            .sheet(isPresented: $showShareSheet) {
                if let selectedRecording = recordingsListViewModel.selectedRecording, let url = URL(string: "file://" + selectedRecording.filePath) {
                    ShareAnySheet(items: [url])
                        .edgesIgnoringSafeArea(.bottom)
                }
            }
            .sheet(isPresented: $showPicker) {
                if let selectedRecording = recordingsListViewModel.selectedRecording, let url = URL(string: "file://" + selectedRecording.filePath) {
                    DocumentSaver(fileURL: url)
                        .edgesIgnoringSafeArea(.bottom)
                }
            }
			.navigationTitle("")
			.navigationBarHidden(true)
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	@ViewBuilder
	func innerView() -> some View {
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
						withAnimation {
							isShowRecordingsListFragment = false
						}
					}
				
				Text("recordings_title")
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
					VStack(spacing: 20) {
						ForEach(Array(recordingsListViewModel.recordings.enumerated()), id: \.offset) { index, recording in
							if index == 0 || recording.month != recordingsListViewModel.recordings[index-1].month {
								createMonthLine(model: recording)
									.frame(maxWidth: .infinity, alignment: .leading)
							}
							
							NavigationLink(destination: LazyView {
								RecordingMediaPlayerFragment(recording: recording)
							}) {
								HStack {
									VStack {
										HStack {
											Image("phone")
												.renderingMode(.template)
												.resizable()
												.frame(width: 25, height: 25)
												.foregroundStyle(Color.grayMain2c600)
											
											Text(recording.displayName)
												.default_text_style_700(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
										}
										
										Spacer()
										
										Text(recording.dateTime)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
									}
									
									VStack {
										Image("play-fill")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 30, height: 30)
											.padding(.leading, -6)
										
										Spacer()
										
										Text(recording.formattedDuration)
											.default_text_style(styleSize: 14)
											.frame(alignment: .center)
									}
									.padding(.trailing, 6)
								}
								.frame(height: 60)
								.padding(20)
								.background(.white)
								.clipShape(RoundedRectangle(cornerRadius: 10))
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onLongPressGesture(minimumDuration: 0.3) {
									touchFeedback()
									recordingsListViewModel.selectedRecording = recording
									showingSheet = true
								}
							}
						}
					}
					.padding(.all, 20)
				}
				.frame(maxWidth: .infinity)
			}
			.overlay(
				VStack {
					if recordingsListViewModel.recordings.count == 0 {
						Spacer()
						Image("illus-belledonne")
							.resizable()
							.scaledToFit()
							.clipped()
							.padding()
						Text("recordings_list_empty")
							.default_text_style_800(styleSize: 16)
						Spacer()
						Spacer()
					}
				}
			)
			.background(Color.gray100)
		}
		.background(Color.gray100)
	}
	
	@ViewBuilder
	func createMonthLine(model: RecordingModel) -> some View {
		Text(model.month)
			.fontWeight(.bold)
			.padding(5)
			.default_text_style_500(styleSize: 22)
	}
}

struct LazyView<Content: View>: View {
	let build: () -> Content
	init(_ build: @escaping () -> Content) {
		self.build = build
	}
	var body: some View { build() }
}
