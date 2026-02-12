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

struct ConversationDocumentsListFragment: View {
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	
	@StateObject private var conversationDocumentsListViewModel = ConversationDocumentsListViewModel()
	
	@Binding var isShowDocumentsFilesFragment: Bool
	
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
										isShowDocumentsFilesFragment = false
									}
								}
							
							Text("conversation_document_list_title")
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
							List {
								ForEach(conversationDocumentsListViewModel.documentsList, id: \.path) { file in
									DocumentRow(file: file)
										.padding(.vertical, 4)
										.padding(.horizontal, 8)
										.listRowInsets(EdgeInsets(top: 0, leading: 0, bottom: 0, trailing: 0))
										.listRowSeparator(.hidden)
										.listRowBackground(Color.clear)
										.onAppear {
										 if file == conversationDocumentsListViewModel.documentsList.last {
											 conversationDocumentsListViewModel.loadMoreData(totalItemsCount: conversationDocumentsListViewModel.documentsList.count)
										 }
									 }
								}
							}
							.safeAreaInset(edge: .top, content: {
								Spacer()
									.frame(height: 12)
							})
							.listStyle(.plain)
							.overlay(
								VStack {
									if conversationDocumentsListViewModel.documentsList.isEmpty {
										Spacer()
										Text("conversation_no_document_found")
											.multilineTextAlignment(.leading)
											.default_text_style_800(styleSize: 16)
										Spacer()
									}
								}
								.padding(.all)
							)
						}
						.frame(maxWidth: .infinity)
					}
					.background(Color.gray100)
					
					if conversationDocumentsListViewModel.operationInProgress {
						PopupLoadingView()
							.background(.black.opacity(0.65))
					}
				}
				.navigationTitle("")
				.navigationBarHidden(true)
				.onDisappear {
					withAnimation {
						isShowDocumentsFilesFragment = false
					}
				}
			}
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
}

struct DocumentRow: View {
	
	@State private var selectedURLAttachment: URL?
	@ObservedObject var file: FileModel

	var body: some View {
		HStack {
			VStack {
				Image(getImageOfType(filename: file.fileName, type: file.mimeTypeString))
					   .renderingMode(.template)
					   .resizable()
					   .foregroundStyle(Color.grayMain2c700)
					   .frame(width: 60, height: 60, alignment: .leading)
			}
			.frame(width: 100, height: 100)
			.background(Color.grayMain2c200)
			.onTapGesture {
				selectedURLAttachment = URL(fileURLWithPath: file.originalPath)
			}
			
			VStack {
				Text(file.fileName)
					.foregroundStyle(Color.grayMain2c700)
					.default_text_style_600(styleSize: 14)
					.truncationMode(.middle)
					.frame(maxWidth: .infinity, alignment: .leading)
					.lineLimit(1)
				
				if file.fileSize > 0 {
					Text(Int(file.fileSize).formatBytes())
					 .default_text_style_300(styleSize: 14)
					 .frame(maxWidth: .infinity, alignment: .leading)
					 .lineLimit(1)
				}
			}
			.padding(.horizontal, 10)
			.frame(maxWidth: .infinity, alignment: .leading)
		}
		.background(.white)
		.clipShape(RoundedRectangle(cornerRadius: 10))
		.onTapGesture {
			selectedURLAttachment = URL(fileURLWithPath: file.originalPath)
		}
	}
	
	func getImageOfType(filename: String, type: String) -> String {
		if type == "audio/mpeg" {
			return "file-audio"
		} else if type == "application/pdf"
					|| filename.lowercased().hasSuffix(".pdf") == true {
			return "file-pdf"
		} else if type.hasPrefix("text/") == true
					|| ["txt", "md", "json", "xml", "csv", "log"].contains(filename.split(separator: ".").last?.lowercased()) {
			return "file-text"
		} else {
			return "file"
		}
	}
}
