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

struct ConversationMediaListFragment: View {
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	
	@StateObject private var conversationMediaListViewModel = ConversationMediaListViewModel()
	
	@Binding var isShowMediaFilesFragment: Bool
	
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
										isShowMediaFilesFragment = false
									}
								}
							
							Text("conversation_media_list_title")
								.multilineTextAlignment(.leading)
								.default_text_style_orange_800(styleSize: 16)
							
							Spacer()
							
						}
						.frame(maxWidth: .infinity)
						.frame(height: 50)
						.padding(.horizontal)
						.padding(.bottom, 4)
						.background(.white)
						
						ConversationMediaGridView(viewModel: conversationMediaListViewModel)
					}
					.background(Color.gray100)
				}
				.navigationTitle("")
				.navigationBarHidden(true)
				.onDisappear {
					withAnimation {
						isShowMediaFilesFragment = false
					}
				}
			}
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
}

struct ConversationMediaGridView: View {

	@ObservedObject var viewModel: ConversationMediaListViewModel

	private let columns = [
		GridItem(.flexible(), spacing: 1),
		GridItem(.flexible(), spacing: 1),
		GridItem(.flexible(), spacing: 1)
	]

	var body: some View {
		VStack(spacing: 0) {
			if !viewModel.mediaList.isEmpty && !viewModel.operationInProgress {
				ScrollView {
					LazyVGrid(columns: columns, spacing: 1) {
						ForEach(viewModel.mediaList, id: \.path) { file in
							MediaGridItemView(file: file)
								.onTapGesture {
									//viewModel.openMediaEvent.send(file)
								}
								.onAppear {
									if file == viewModel.mediaList.last {
										viewModel.loadMoreData(totalItemsCount: viewModel.mediaList.count)
									}
								}
						}
					}
					.padding(.horizontal, 2)
					.padding(.top, 12)
				}
			} else if viewModel.mediaList.isEmpty && !viewModel.operationInProgress {
				Spacer()
				Text("conversation_no_media_found")
					.multilineTextAlignment(.center)
					.default_text_style_800(styleSize: 16)
				Spacer()
			}
		}
	}
}

struct MediaGridItemView: View {

	@ObservedObject var file: FileModel

	var body: some View {
		ZStack(alignment: .bottomTrailing) {
			if let previewPath = file.mediaPreview,
			   let image = UIImage(contentsOfFile: previewPath) {
				Image(uiImage: image)
					.resizable()
					.scaledToFill()
					.frame(width: 120, height: 120)
					.clipped()
			} else {
				Rectangle()
					.fill(Color.gray.opacity(0.2))
					.frame(width: 120, height: 120)
			}
			
			if file.isVideoPreview {
				VStack {
					Spacer()
					
					Image("play-fill")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 35, height: 35)
					
					Spacer()
				}
				.frame(width: 120, height: 120)
			}
			
			if let duration = file.audioVideoDuration, file.isVideoPreview {
				Text(duration)
					.font(.caption2)
					.padding(6)
					.background(Color.black.opacity(0.6))
					.foregroundColor(.white)
					.clipShape(RoundedRectangle(cornerRadius: 6))
					.padding(6)
			}
		}
		.cornerRadius(8)
	}
}
