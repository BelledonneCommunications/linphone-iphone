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

struct RecordingsListBottomSheet: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@State private var orientation = UIDevice.current.orientation
	
	@EnvironmentObject var recordingsListViewModel: RecordingsListViewModel
	
	@Binding var showingSheet: Bool
    @Binding var showShareSheet: Bool
    @Binding var showPicker: Bool
	
	var body: some View {
		VStack(alignment: .leading) {
			if idiom != .pad && (orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Spacer()
				HStack {
					Spacer()
					Button("dialog_close") {
						if #available(iOS 16.0, *) {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					}
				}
				.padding(.trailing)
			}
			
			Spacer()
			
			if let selectedRecording = recordingsListViewModel.selectedRecording {
				Button {
                    showShareSheet = true
					
					if #available(iOS 16.0, *) {
						if idiom != .pad {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} label: {
					HStack {
						Image("share-network")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
						Text("menu_share_selected_item")
							.default_text_style(styleSize: 16)
						Spacer()
					}
					.frame(maxHeight: .infinity)
				}
				.padding(.horizontal, 30)
				.background(Color.gray100)
				
				VStack {
					Divider()
				}
				.frame(maxWidth: .infinity)
				
				Button {
                    showPicker = true
					
					if #available(iOS 16.0, *) {
						if idiom != .pad {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} label: {
					HStack {
						Image("download-simple")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
						Text("menu_export_selected_item")
							.default_text_style(styleSize: 16)
						Spacer()
					}
					.frame(maxHeight: .infinity)
				}
				.padding(.horizontal, 30)
				.background(Color.gray100)
				
				VStack {
					Divider()
				}
				.frame(maxWidth: .infinity)
				
				Button {
                    recordingsListViewModel.recordings.removeAll { $0.filePath == selectedRecording.filePath }
					selectedRecording.delete()
					
					if #available(iOS 16.0, *) {
						if idiom != .pad {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} label: {
					HStack {
						Image("trash")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.redDanger500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
						Text("menu_delete_selected_item")
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 16)
						Spacer()
					}
					.frame(maxHeight: .infinity)
				}
				.padding(.horizontal, 30)
				.background(Color.gray100)
			}
		}
		.background(Color.gray100)
		.frame(maxWidth: .infinity)
		.onRotate { newOrientation in
			orientation = newOrientation
		}
	}
}

#Preview {
	ConversationsListBottomSheet(showingSheet: .constant(true))
}
