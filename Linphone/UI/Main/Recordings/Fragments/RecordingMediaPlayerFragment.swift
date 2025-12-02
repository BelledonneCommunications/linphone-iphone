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

struct RecordingMediaPlayerFragment: View {
	
	@StateObject private var recordingMediaPlayerViewModel: RecordingMediaPlayerViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State private var showShareSheet: Bool = false
	@State private var showPicker: Bool = false
	@State private var isSeeking: Bool = false
	
	@State private var lastValue: Double = -1.0
	@State private var value: Double = 40.0
	
	@State private var timer: Timer?
	
	
	init(recording: RecordingModel) {
		_recordingMediaPlayerViewModel = StateObject(wrappedValue: RecordingMediaPlayerViewModel(recording: recording))
	}
	
	var body: some View {
		ZStack {
			GeometryReader { geometry in
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
									dismiss()
								}
							}
						
						VStack {
							Text(recordingMediaPlayerViewModel.recording.displayName)
								.default_text_style_700(styleSize: 14)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)
							
							Text(recordingMediaPlayerViewModel.recording.dateTime)
								.default_text_style_300(styleSize: 12)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)
						}
						.padding(.top, 4)
						
						Spacer()
						
						Button {
							showShareSheet = true
						} label: {
							Image("share-network")
								.renderingMode(.template)
								.resizable()
								.frame(width: 28, height: 28)
								.foregroundStyle(Color.grayMain2c500)
						}
						.padding(.all, 6)
						.padding(.top, 4)
						
						
						Button {
							showPicker = true
						} label: {
							Image("download-simple")
								.renderingMode(.template)
								.resizable()
								.frame(width: 28, height: 28)
								.foregroundStyle(Color.grayMain2c500)
						}
						.padding(.all, 6)
						.padding(.top, 4)
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					
					VStack {
						Spacer()
						
						Image("music-notes")
							.renderingMode(.template)
							.resizable()
							.frame(width: 80, height: 80)
							.foregroundStyle(.white)
						
						Spacer()
						
						HStack(spacing: 0) {
							Button {
								if recordingMediaPlayerViewModel.isPlaying {
									recordingMediaPlayerViewModel.pauseVoiceRecordPlayer()
								} else {
									recordingMediaPlayerViewModel.startVoiceRecordPlayer()
									playProgress()
								}
							} label: {
								Image(recordingMediaPlayerViewModel.isPlaying ? "pause-fill" : "play-fill")
									.renderingMode(.template)
									.resizable()
									.frame(width: 25, height: 25)
									.foregroundStyle(.white)
							}
							.frame(width: 50)
							
							let radius = geometry.size.height * 0.5
							let barWidth = geometry.size.width - 120
							ZStack(alignment: .leading) {
								Rectangle()
									.foregroundColor(Color.orangeMain100)
									.frame(width: barWidth, height: 5)
									.clipShape(RoundedRectangle(cornerRadius: radius))

								Rectangle()
									.foregroundColor(Color.orangeMain500)
									.frame(width: (self.value / 100) * barWidth, height: isSeeking ? 10 : 5)
									.clipShape(RoundedRectangle(cornerRadius: radius))
							}
							.frame(width: barWidth, height: 20)
							.contentShape(Rectangle())
							.gesture(
								DragGesture(minimumDistance: 0)
									.onChanged { gesture in
										isSeeking = true
										
										let x = max(0, min(barWidth, gesture.location.x))
										let percent = x / barWidth * 100
										self.value = percent
									}
									.onEnded { gesture in
										let x = max(0, min(barWidth, gesture.location.x))
										let percent = x / barWidth * 100
										self.value = percent
										
										isSeeking = false
										recordingMediaPlayerViewModel.seekTo(percent: percent)
										
										lastValue = percent
									}
							)

							
							Text(recordingMediaPlayerViewModel.recording.formattedDuration)
								.default_text_style_white_600(styleSize: 18)
								.frame(maxWidth: .infinity, alignment: .center)
								.lineLimit(1)
								.frame(width: 70)
						}
						.padding(.bottom, 20)
					}
					.frame(maxWidth: .infinity, maxHeight: .infinity)
					.background(.black)
				}
			}
			.sheet(isPresented: $showShareSheet) {
				if let url = URL(string: "file://" + recordingMediaPlayerViewModel.recording.filePath) {
					ShareAnySheet(items: [url])
						.edgesIgnoringSafeArea(.bottom)
				}
			}
			.sheet(isPresented: $showPicker) {
				if let url = URL(string: "file://" + recordingMediaPlayerViewModel.recording.filePath) {
					DocumentSaver(fileURL: url)
						.edgesIgnoringSafeArea(.bottom)
				}
			}
			.onAppear {
				playProgress()
			}
			.onDisappear {
				recordingMediaPlayerViewModel.stopVoiceRecordPlayer()
			}
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	private func playProgress() {
		timer?.invalidate()
		
		lastValue = -1.0
		
		value = recordingMediaPlayerViewModel.getPositionVoiceRecordPlayer()

		timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
			let current = recordingMediaPlayerViewModel.getPositionVoiceRecordPlayer()

			if isSeeking {
				return
			}

			if !recordingMediaPlayerViewModel.isPlaying {
				self.value = current
				lastValue = current
				return
			}

			if current > lastValue {
				self.value = current
				lastValue = current
				return
			}

			recordingMediaPlayerViewModel.stopVoiceRecordPlayer()
			self.timer?.invalidate()
			self.value = 0
		}
	}
}

struct SaveToFilesView: View {
	let fileURL: URL
	@State private var showPicker = false

	var body: some View {
		Button("Enregistrer dans Fichiers") {
			showPicker = true
		}
		.sheet(isPresented: $showPicker) {
			DocumentSaver(fileURL: fileURL)
		}
	}
}

struct DocumentSaver: UIViewControllerRepresentable {
	let fileURL: URL

	func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
		let picker = UIDocumentPickerViewController(
			forExporting: [fileURL], asCopy: true
		)
		return picker
	}

	func updateUIViewController(_ uiViewController: UIDocumentPickerViewController, context: Context) {}
}
