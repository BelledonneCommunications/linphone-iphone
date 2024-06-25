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

struct ConversationFragment: View {
	
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	
	@State var isMenuOpen = false
	
	@FocusState var isMessageTextFocused: Bool
	
	@State var offset: CGPoint = .zero
	
	private let ids: [String] = []
	
	@State private var isScrolledToBottom: Bool = true
	var showMessageMenuOnLongPress: Bool = true
	
	@StateObject private var viewModel = ChatViewModel()
	@StateObject private var paginationState = PaginationState()
	
	@State private var displayFloatingButton = false
	
	@State private var isShowPhotoLibrary = false
	@State private var isShowCamera = false
	
	@State private var mediasIsLoading = false
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				VStack(spacing: 1) {
					if conversationViewModel.displayedConversation != nil {
						Rectangle()
							.foregroundColor(Color.orangeMain500)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
						
						HStack {
							if !(orientation == .landscapeLeft || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
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
											conversationViewModel.displayedConversation = nil
										}
									}
							}
							
							Avatar(contactAvatarModel: conversationViewModel.displayedConversation!.avatarModel, avatarSize: 50)
								.padding(.top, 4)
							
							Text(conversationViewModel.displayedConversation!.subject)
								.default_text_style(styleSize: 16)
								.frame(maxWidth: .infinity, alignment: .leading)
								.padding(.top, 4)
								.lineLimit(1)
							
							Spacer()
							
							Button {
							} label: {
								Image("phone")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, 4)
							}
							
							Menu {
								Button {
									isMenuOpen = false
								} label: {
									HStack {
										Text("See contact")
										Spacer()
										Image("user-circle")
											.resizable()
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
								
								Button {
									isMenuOpen = false
								} label: {
									HStack {
										Text("Copy SIP address")
										Spacer()
										Image("copy")
											.resizable()
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
								
								Button(role: .destructive) {
									isMenuOpen = false
								} label: {
									HStack {
										Text("Delete history")
										Spacer()
										Image("trash-simple-red")
											.resizable()
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
							} label: {
								Image("dots-three-vertical")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, 4)
							}
							.onTapGesture {
								isMenuOpen = true
							}
						}
						.frame(maxWidth: .infinity)
						.frame(height: 50)
						.padding(.horizontal)
						.padding(.bottom, 4)
						.background(.white)
						
						if #available(iOS 16.0, *) {
							ZStack(alignment: .bottomTrailing) {
								UIList(viewModel: viewModel,
									   paginationState: paginationState,
									   conversationViewModel: conversationViewModel,
									   conversationsListViewModel: conversationsListViewModel,
									   isScrolledToBottom: $isScrolledToBottom,
									   showMessageMenuOnLongPress: showMessageMenuOnLongPress,
									   geometryProxy: geometry,
									   sections: conversationViewModel.conversationMessagesSection
								)
								
								if !isScrolledToBottom {
									Button {
										NotificationCenter.default.post(name: .onScrollToBottom, object: nil)
									} label: {
										ZStack {
											
											Image("caret-down")
												.renderingMode(.template)
												.foregroundStyle(.white)
												.padding()
												.background(Color.orangeMain500)
												.clipShape(Circle())
												.shadow(color: .black.opacity(0.2), radius: 4)
											
											if conversationViewModel.displayedConversationUnreadMessagesCount > 0 {
												VStack {
													HStack {
														Spacer()
														
														HStack {
															Text(
																conversationViewModel.displayedConversationUnreadMessagesCount < 99
																? String(conversationViewModel.displayedConversationUnreadMessagesCount)
																: "99+"
															)
															.foregroundStyle(.white)
															.default_text_style(styleSize: 10)
															.lineLimit(1)
															
														}
														.frame(width: 18, height: 18)
														.background(Color.redDanger500)
														.cornerRadius(50)
													}
													
													Spacer()
												}
											}
										}
										
									}
									.frame(width: 50, height: 50)
									.padding()
								}
							}
							.onTapGesture {
								UIApplication.shared.endEditing()
							}
							.onAppear {
								conversationViewModel.getMessages()
							}
							.onDisappear {
								conversationViewModel.resetMessage()
							}
						} else {
							ScrollViewReader { proxy in
								ZStack(alignment: .bottomTrailing) {
									List {
										if conversationViewModel.conversationMessagesSection.first != nil {
											let counter = conversationViewModel.conversationMessagesSection.first!.rows.count
											ForEach(0..<counter, id: \.self) { index in
												ChatBubbleView(conversationViewModel: conversationViewModel, message: conversationViewModel.conversationMessagesSection.first!.rows[index], geometryProxy: geometry)
													.id(conversationViewModel.conversationMessagesSection.first!.rows[index].id)
													.listRowInsets(EdgeInsets(top: 2, leading: 10, bottom: 2, trailing: 10))
													.listRowSeparator(.hidden)
													.scaleEffect(x: 1, y: -1, anchor: .center)
													.onAppear {
														if index == counter - 1
															&& conversationViewModel.displayedConversationHistorySize > conversationViewModel.conversationMessagesSection.first!.rows.count {
															DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
																conversationViewModel.getOldMessages()
															}
														}
														
														if index == 0 {
															displayFloatingButton = false
														}
													}
													.onDisappear {
														if index == 0 {
															displayFloatingButton = true
														}
													}
											}
										}
									}
									.scaleEffect(x: 1, y: -1, anchor: .center)
									.listStyle(.plain)
									
									if displayFloatingButton {
										Button {
											if conversationViewModel.conversationMessagesSection.first != nil && conversationViewModel.conversationMessagesSection.first!.rows.first != nil {
												withAnimation {
													proxy.scrollTo(conversationViewModel.conversationMessagesSection.first!.rows.first!.id)
												}
											}
										} label: {
											ZStack {
												
												Image("caret-down")
													.renderingMode(.template)
													.foregroundStyle(.white)
													.padding()
													.background(Color.orangeMain500)
													.clipShape(Circle())
													.shadow(color: .black.opacity(0.2), radius: 4)
												
												if conversationViewModel.displayedConversationUnreadMessagesCount > 0 {
													VStack {
														HStack {
															Spacer()
															
															HStack {
																Text(
																	conversationViewModel.displayedConversationUnreadMessagesCount < 99
																	? String(conversationViewModel.displayedConversationUnreadMessagesCount)
																	: "99+"
																)
																.foregroundStyle(.white)
																.default_text_style(styleSize: 10)
																.lineLimit(1)
																
															}
															.frame(width: 18, height: 18)
															.background(Color.redDanger500)
															.cornerRadius(50)
														}
														
														Spacer()
													}
												}
											}
											
										}
										.frame(width: 50, height: 50)
										.padding()
									}
								}
								.onTapGesture {
									UIApplication.shared.endEditing()
								}
								.onAppear {
									conversationViewModel.getMessages()
								}
								.onDisappear {
									conversationViewModel.resetMessage()
								}
							}
						}
						
						if !conversationViewModel.mediasToSend.isEmpty || mediasIsLoading {
							ZStack(alignment: .top) {
								HStack {
									if mediasIsLoading {
										HStack {
											Spacer()
											
											ProgressView()
											
											Spacer()
										}
										.frame(height: 120)
									}
									
									if !mediasIsLoading {
										LazyVGrid(columns: [
											GridItem(.adaptive(minimum: 100), spacing: 1)
										], spacing: 3) {
											ForEach(conversationViewModel.mediasToSend, id: \.id) { attachment in
												ZStack {
													Rectangle()
														.fill(Color(.white))
														.frame(width: 100, height: 100)
													
													AsyncImage(url: attachment.thumbnail) { image in
														ZStack {
															image
																.resizable()
																.interpolation(.medium)
																.aspectRatio(contentMode: .fill)
															
															if attachment.type == .video {
																Image("play-fill")
																	.renderingMode(.template)
																	.resizable()
																	.foregroundStyle(.white)
																	.frame(width: 40, height: 40, alignment: .leading)
															}
														}
													} placeholder: {
														ProgressView()
													}
													.layoutPriority(-1)
													.onTapGesture {
														if conversationViewModel.mediasToSend.count == 1 {
															withAnimation {
																conversationViewModel.mediasToSend.removeAll()
															}
														} else {
															guard let index = self.conversationViewModel.mediasToSend.firstIndex(of: attachment) else { return }
															self.conversationViewModel.mediasToSend.remove(at: index)
														}
													}
												}
												.clipShape(RoundedRectangle(cornerRadius: 4))
												.contentShape(Rectangle())
											}
										}
										.frame(
											width: geometry.size.width > 0 && CGFloat(102 * conversationViewModel.mediasToSend.count) > geometry.size.width - 20
											? 102 * floor(CGFloat(geometry.size.width - 20) / 102)
											: CGFloat(102 * conversationViewModel.mediasToSend.count)
										)
									}
								}
								.frame(maxWidth: .infinity)
								.padding(.all, conversationViewModel.mediasToSend.isEmpty ? 0 : 10)
								.background(Color.gray100)
								
								if !mediasIsLoading {
									HStack {
										Spacer()
										
										Button(action: {
											withAnimation {
												conversationViewModel.mediasToSend.removeAll()
											}
										}, label: {
											Image("x")
												.resizable()
												.frame(width: 30, height: 30, alignment: .leading)
												.padding(.all, 10)
										})
									}
								}
							}
							.transition(.move(edge: .bottom))
						}
						
						HStack(spacing: 0) {
							Button {
							} label: {
								Image("smiley")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 28, height: 28, alignment: .leading)
									.padding(.all, 6)
									.padding(.top, 4)
							}
							.padding(.horizontal, isMessageTextFocused ? 0 : 2)
							
							Button {
								self.isShowPhotoLibrary = true
								self.mediasIsLoading = true
							} label: {
								Image("paperclip")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
									.frame(width: isMessageTextFocused ? 0 : 28, height: isMessageTextFocused ? 0 : 28, alignment: .leading)
									.padding(.all, isMessageTextFocused ? 0 : 6)
									.padding(.top, 4)
									.disabled(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading)
							}
							.padding(.horizontal, isMessageTextFocused ? 0 : 2)
							
							Button {
								self.isShowCamera = true
							} label: {
								Image("camera")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
									.frame(width: isMessageTextFocused ? 0 : 28, height: isMessageTextFocused ? 0 : 28, alignment: .leading)
									.padding(.all, isMessageTextFocused ? 0 : 6)
									.padding(.top, 4)
									.disabled(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading)
							}
							.padding(.horizontal, isMessageTextFocused ? 0 : 2)
							
							HStack {
								if #available(iOS 16.0, *) {
									TextField("Say something...", text: $conversationViewModel.messageText, axis: .vertical)
										.default_text_style(styleSize: 15)
										.focused($isMessageTextFocused)
										.padding(.vertical, 5)
								} else {
									ZStack(alignment: .leading) {
										TextEditor(text: $conversationViewModel.messageText)
											.multilineTextAlignment(.leading)
											.frame(maxHeight: 160)
											.fixedSize(horizontal: false, vertical: true)
											.default_text_style(styleSize: 15)
											.focused($isMessageTextFocused)
										
										if conversationViewModel.messageText.isEmpty {
											Text("Say something...")
												.padding(.leading, 4)
												.opacity(conversationViewModel.messageText.isEmpty ? 1 : 0)
												.foregroundStyle(Color.gray300)
												.default_text_style(styleSize: 15)
										}
									}
									.onTapGesture {
										isMessageTextFocused = true
									}
								}
								
								if conversationViewModel.messageText.isEmpty && conversationViewModel.mediasToSend.isEmpty {
									Button {
									} label: {
										Image("microphone")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 28, height: 28, alignment: .leading)
											.padding(.all, 6)
											.padding(.top, 4)
									}
								} else {
									Button {
										NotificationCenter.default.post(name: .onScrollToBottom, object: nil)
										conversationViewModel.sendMessage()
									} label: {
										Image("paper-plane-tilt")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 28, height: 28, alignment: .leading)
											.padding(.all, 6)
											.padding(.top, 4)
											.rotationEffect(.degrees(45))
									}
									.padding(.trailing, 4)
								}
							}
							.padding(.leading, 15)
							.padding(.trailing, 5)
							.padding(.vertical, 6)
							.frame(maxWidth: .infinity, minHeight: 55)
							.background(.white)
							.cornerRadius(30)
							.overlay(
								RoundedRectangle(cornerRadius: 30)
									.inset(by: 0.5)
									.stroke(Color.gray200, lineWidth: 1.5)
							)
							.padding(.horizontal, 4)
						}
						.frame(maxWidth: .infinity, minHeight: 60)
						.padding(.top, 12)
						.padding(.bottom, geometry.safeAreaInsets.bottom > 0 ? (isMessageTextFocused ? 12 : 0) : 12)
						.padding(.horizontal, 10)
						.background(Color.gray100)
					}
				}
				.background(.white)
				.navigationBarHidden(true)
				.onRotate { newOrientation in
					orientation = newOrientation
				}
				.onAppear {
					conversationViewModel.addConversationDelegate()
				}
				.onDisappear {
					conversationViewModel.removeConversationDelegate()
				}
				.sheet(isPresented: $isShowPhotoLibrary) {
					PhotoPicker(filter: nil, limit: conversationViewModel.maxMediaCount - conversationViewModel.mediasToSend.count) { results in
						PhotoPicker.convertToAttachmentArray(fromResults: results) { mediasOrNil, errorOrNil in
							if let error = errorOrNil {
								print(error)
							}
							
							if let medias = mediasOrNil {
								conversationViewModel.mediasToSend.append(contentsOf: medias)
							}
							
							self.mediasIsLoading = false
						}
					}
					.edgesIgnoringSafeArea(.all)
				}
				.fullScreenCover(isPresented: $isShowCamera) {
					ImagePicker(conversationViewModel: conversationViewModel, selectedMedia: self.$conversationViewModel.mediasToSend)
						.edgesIgnoringSafeArea(.all)
				}
			}
		}
		.navigationViewStyle(.stack)
	}
}

struct ScrollOffsetPreferenceKey: PreferenceKey {
	static var defaultValue: CGPoint = .zero
	
	static func reduce(value: inout CGPoint, nextValue: () -> CGPoint) {
	}
}

struct ImagePicker: UIViewControllerRepresentable {
	@ObservedObject var conversationViewModel: ConversationViewModel
	@Binding var selectedMedia: [Attachment]
	@Environment(\.presentationMode) private var presentationMode
 
	final class Coordinator: NSObject, UIImagePickerControllerDelegate, UINavigationControllerDelegate {
	 
		var parent: ImagePicker
	 
		init(_ parent: ImagePicker) {
			self.parent = parent
		}
	 
		func imagePickerController(_ picker: UIImagePickerController, didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey: Any]) {
			let mediaType = info[UIImagePickerController.InfoKey.mediaType] as? String
			switch mediaType {
			case "public.image":
				let image = info[UIImagePickerController.InfoKey.originalImage] as? UIImage
				
				let date = Date()
				let df = DateFormatter()
				df.dateFormat = "yyyy-MM-dd-HHmmss"
				let dateString = df.string(from: date)
				
				let path = FileManager.default.temporaryDirectory.appendingPathComponent((dateString.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "") + ".jpeg")
				
				if image != nil {
					let data  = image!.jpegData(compressionQuality: 1)
					if data != nil {
						do {
							let decodedData: () = try data!.write(to: path)
							let attachment = Attachment(id: UUID().uuidString, url: path, type: .image)
							parent.selectedMedia.append(attachment)
						} catch {
						}
					}
				}
			case "public.movie":
				let videoUrl = info[UIImagePickerController.InfoKey.mediaURL] as? URL
				if videoUrl != nil {
					let name = videoUrl!.lastPathComponent
					let path = videoUrl!.deletingLastPathComponent()
					let pathThumbnail = URL(string: parent.conversationViewModel.generateThumbnail(name: name, pathThumbnail: path))
					
					if pathThumbnail != nil {
						let attachment =
						Attachment(
							id: UUID().uuidString,
							thumbnail: pathThumbnail!,
							full: videoUrl!,
							type: .video
						)
						parent.selectedMedia.append(attachment)
					}
				}
			default:
				Log.info("Mismatched type: \(mediaType)")
			}
	 
			parent.presentationMode.wrappedValue.dismiss()
		}
	}
	
	func makeUIViewController(context: UIViewControllerRepresentableContext<ImagePicker>) -> UIImagePickerController {
		let imagePicker = UIImagePickerController()
		imagePicker.sourceType = .camera
		imagePicker.mediaTypes = ["public.image", "public.movie"]
		imagePicker.delegate = context.coordinator
 
		return imagePicker
	}
 
	func updateUIViewController(_ uiViewController: UIImagePickerController, context: UIViewControllerRepresentableContext<ImagePicker>) {
 
	}
	
	func makeCoordinator() -> Coordinator {
		Coordinator(self)
	}
}

/*
#Preview {
	ConversationFragment(conversationViewModel: ConversationViewModel(), conversationsListViewModel: ConversationsListViewModel(), sections: [MessagesSection], ids: [""])
}
*/
