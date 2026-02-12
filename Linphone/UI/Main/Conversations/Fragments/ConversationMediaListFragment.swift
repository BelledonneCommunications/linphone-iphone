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
					
					if conversationMediaListViewModel.operationInProgress {
						PopupLoadingView()
							.background(.black.opacity(0.65))
					}
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
	@State private var selectedURLAttachment: URL?
	private let columns = 4
	private let spacing: CGFloat = 2

	var body: some View {
		VStack(spacing: 0) {
			if !viewModel.mediaList.isEmpty {
				GeometryReader { geometry in
					let totalSpacing = spacing * CGFloat(columns - 1)
					let itemWidth = (geometry.size.width - totalSpacing) / CGFloat(columns)

					ScrollView {
						LazyVGrid(
							columns: Array(repeating: GridItem(.fixed(itemWidth), spacing: spacing), count: columns),
							spacing: spacing
						) {
							ForEach(viewModel.mediaList, id: \.path) { file in
								MediaGridItemView(file: file)
									.aspectRatio(1, contentMode: .fit)
									.frame(width: itemWidth, height: itemWidth)
									.clipped()
									.onTapGesture {
										selectedURLAttachment = URL(fileURLWithPath: file.originalPath)
									}
									.onAppear {
										if file == viewModel.mediaList.last {
											viewModel.loadMoreData(totalItemsCount: viewModel.mediaList.count)
										}
									}
							}
						}
						.padding(.horizontal, spacing)
						.padding(.top, spacing)
					}
				}
				.quickLookPreview($selectedURLAttachment, in: viewModel.mediaList.compactMap { URL(fileURLWithPath: $0.originalPath) })
			} else if viewModel.mediaList.isEmpty && !viewModel.operationInProgress {
				Spacer()
				Text("conversation_no_media_found")
					.multilineTextAlignment(.center)
					.default_text_style_800(styleSize: 16)
				Spacer()
			} else {
				Spacer()
			}
		}
	}
}

struct MediaGridItemView: View {
	@ObservedObject var file: FileModel

	var body: some View {
		GeometryReader { geo in
			ZStack(alignment: .bottomTrailing) {
				if let previewPath = file.mediaPreview,
				   let image = UIImage(contentsOfFile: previewPath) {
					Image(uiImage: image)
						.resizable()
						.scaledToFill()
						.frame(width: geo.size.width, height: geo.size.height)
						.clipped()
				} else {
					Rectangle()
						.fill(Color.gray.opacity(0.2))
						.frame(width: geo.size.width, height: geo.size.height)
				}
				
				if file.isVideoPreview {
					Image("play-fill")
						.resizable()
						.renderingMode(.template)
						.scaledToFit()
						.frame(width: geo.size.width * 0.3, height: geo.size.height * 0.3)
						.foregroundColor(.white)
						.shadow(radius: 2)
						.position(x: geo.size.width / 2, y: geo.size.height / 2)
				}
				
				if let duration = file.audioVideoDuration, file.isVideoPreview {
					Text(duration)
						.font(.caption2)
						.padding(4)
						.background(Color.black.opacity(0.6))
						.foregroundColor(.white)
						.clipShape(RoundedRectangle(cornerRadius: 4))
						.padding(6)
				}
			}
			.cornerRadius(8)
		}
		.aspectRatio(1, contentMode: .fit)
	}
}
