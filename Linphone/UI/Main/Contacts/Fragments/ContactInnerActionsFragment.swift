//
//  ContactInnerActionsFragment.swift
//  Linphone
//
//  Created by Benoît Martins on 09/11/2023.
//

import SwiftUI

struct ContactInnerActionsFragment: View {
	
	@ObservedObject var magicSearch = MagicSearchSingleton.shared
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	
	@State private var informationIsOpen = true
	
	@Binding var showingSheet: Bool
	@Binding var isShowDeletePopup: Bool
	@Binding var isShowDismissPopup: Bool
	
	var actionEditButton: () -> Void
	
    var body: some View {
		HStack(alignment: .center) {
			Text("Information")
				.default_text_style_800(styleSize: 16)
			
			Spacer()
			
			Image(informationIsOpen ? "caret-up" : "caret-down")
				.renderingMode(.template)
				.resizable()
				.foregroundStyle(Color.grayMain2c600)
				.frame(width: 25, height: 25, alignment: .leading)
		}
		.padding(.top, 30)
		.padding(.bottom, 10)
		.padding(.horizontal, 16)
		.background(Color.gray100)
		.onTapGesture {
			withAnimation {
				informationIsOpen.toggle()
			}
		}
		
		if informationIsOpen {
			VStack(spacing: 0) {
				if contactViewModel.indexDisplayedFriend != nil && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
					ForEach(0..<magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses.count, id: \.self) { index in
						Button {
						} label: {
							HStack {
								VStack {
									Text("SIP address :")
										.default_text_style_700(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
									Text(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses[index].asStringUriOnly().dropFirst(4))
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
										.fixedSize(horizontal: false, vertical: true)
								}
								Spacer()
								
								Image("phone")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25)
									.onTapGesture {
										withAnimation {
											
										}
									}
							}
							.padding(.vertical, 15)
							.padding(.horizontal, 20)
						}
						.simultaneousGesture(
							LongPressGesture()
								.onEnded { _ in
									contactViewModel.stringToCopy = magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses[index].asStringUriOnly()
									showingSheet.toggle()
								}
						)
						.highPriorityGesture(
							TapGesture()
								.onEnded { _ in
									withAnimation {
										
									}
								}
						)
						
						if !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.isEmpty
							|| index < magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses.count - 1 {
							VStack {
								Divider()
							}
							.padding(.horizontal)
						}
					}
					
					ForEach(0..<magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.count, id: \.self) { index in
						Button {
						} label: {
							HStack {
								VStack {
									if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label != nil
										&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label!.isEmpty {
										Text("Phone (\(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label!)) :")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
									} else {
										Text("Phone :")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
									}
									Text(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].phoneNumber)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
										.fixedSize(horizontal: false, vertical: true)
								}
								Spacer()
								
								Image("phone")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25)
									.onTapGesture {
										withAnimation {
											
										}
									}
							}
							.padding(.vertical, 15)
							.padding(.horizontal, 20)
						}
						.simultaneousGesture(
							LongPressGesture()
								.onEnded { _ in
									contactViewModel.stringToCopy = 
									magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].phoneNumber
									showingSheet.toggle()
								}
						)
						.highPriorityGesture(
							TapGesture()
								.onEnded { _ in
									withAnimation {
										
									}
								}
						)
						
						if index < magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.count - 1 {
							VStack {
								Divider()
							}
							.padding(.horizontal)
						}
					}
				}
			}
			.background(.white)
			.cornerRadius(15)
			.padding(.horizontal)
			.zIndex(-1)
			.transition(.move(edge: .top))
		}
		
		if contactViewModel.indexDisplayedFriend != nil
			&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
			&& ((magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization != nil
				 && !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!.isEmpty)
				|| (magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle != nil
					&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!.isEmpty)) {
			VStack {
				if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization != nil
					&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!.isEmpty {
					Text("**Company :** \(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!)")
						.default_text_style(styleSize: 14)
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
				
				if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle != nil
					&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!.isEmpty {
					Text("**Job :** \(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!)")
						.default_text_style(styleSize: 14)
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
			}
			.background(.white)
			.cornerRadius(15)
			.padding(.top)
			.padding(.horizontal)
			.zIndex(-1)
			.transition(.move(edge: .top))
		}
		
		// TODO Trust Fragment
		
		// TODO Medias Fragment
		
		HStack(alignment: .center) {
			Text("Other actions")
				.default_text_style_800(styleSize: 16)
			
			Spacer()
		}
		.padding(.vertical, 10)
		.padding(.horizontal, 16)
		.background(Color.gray100)
		
		VStack(spacing: 0) {
			if contactViewModel.indexDisplayedFriend != nil && contactViewModel.indexDisplayedFriend! < magicSearch.lastSearch.count
				&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
				&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.nativeUri != nil
				&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.nativeUri!.isEmpty {
				Button {
					actionEditButton()
				} label: {
					HStack {
						Image("pencil-simple")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c600)
							.frame(width: 25, height: 25)
						
						Text("Edit")
							.default_text_style(styleSize: 14)
							.frame(maxWidth: .infinity, alignment: .leading)
							.lineLimit(1)
							.fixedSize(horizontal: false, vertical: true)
						Spacer()
					}
					.padding(.vertical, 15)
					.padding(.horizontal, 20)
				}
			} else {
				NavigationLink(destination: EditContactFragment(
						editContactViewModel: editContactViewModel,
						isShowEditContactFragment: .constant(false),
						isShowDismissPopup: $isShowDismissPopup)) {
						HStack {
							Image("pencil-simple")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 25, height: 25)
							
							Text("Edit")
								.default_text_style(styleSize: 14)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)
								.fixedSize(horizontal: false, vertical: true)
							Spacer()
						}
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
				}
				.simultaneousGesture(
					TapGesture().onEnded {
						editContactViewModel.selectedEditFriend = magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend
						editContactViewModel.resetValues()
					}
				)
			}
			/*
			Button {
			} label: {
				HStack {
					Image("pencil-simple")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 25, height: 25)
					
					Text("Edit")
						.default_text_style(styleSize: 14)
						.frame(maxWidth: .infinity, alignment: .leading)
						.lineLimit(1)
						.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
			 */
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			Button {
				if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
					contactViewModel.objectWillChange.send()
					magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred.toggle()
				}
			} label: {
				HStack {
					Image(contactViewModel.indexDisplayedFriend != nil && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
						  && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true ? "heart-fill" : "heart")
					.renderingMode(.template)
					.resizable()
					.foregroundStyle(contactViewModel.indexDisplayedFriend != nil && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
									 && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true ? Color.redDanger500 : Color.grayMain2c500)
					.frame(width: 25, height: 25)
					Text(contactViewModel.indexDisplayedFriend != nil
						 && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
						 && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true
						 ? "Remove from favourites"
						 : "Add to favourites")
					.default_text_style(styleSize: 14)
					.frame(maxWidth: .infinity, alignment: .leading)
					.lineLimit(1)
					.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			Button {
			} label: {
				HStack {
					Image("share-network")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 25, height: 25)
					
					Text("Share")
						.default_text_style(styleSize: 14)
						.frame(maxWidth: .infinity, alignment: .leading)
						.lineLimit(1)
						.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			/*
			 Button {
			 } label: {
			 HStack {
			 Image("bell-simple-slash")
			 .renderingMode(.template)
			 .resizable()
			 .foregroundStyle(Color.grayMain2c600)
			 .frame(width: 25, height: 25)
			 
			 Text("Mute")
			 .default_text_style(styleSize: 14)
			 .frame(maxWidth: .infinity, alignment: .leading)
			 .lineLimit(1)
			 .fixedSize(horizontal: false, vertical: true)
			 Spacer()
			 }
			 .padding(.vertical, 15)
			 .padding(.horizontal, 20)
			 }
			 
			 VStack {
			 Divider()
			 }
			 .padding(.horizontal)
			 
			 Button {
			 } label: {
			 HStack {
			 Image("x-circle")
			 .renderingMode(.template)
			 .resizable()
			 .foregroundStyle(Color.grayMain2c600)
			 .frame(width: 25, height: 25)
			 
			 Text("Block")
			 .default_text_style(styleSize: 14)
			 .frame(maxWidth: .infinity, alignment: .leading)
			 .lineLimit(1)
			 .fixedSize(horizontal: false, vertical: true)
			 Spacer()
			 }
			 .padding(.vertical, 15)
			 .padding(.horizontal, 20)
			 }
			 
			 VStack {
			 Divider()
			 }
			 .padding(.horizontal)
			 */
			
			Button {
				if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
					isShowDeletePopup.toggle()
				}
			} label: {
				HStack {
					Image("trash-simple")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.redDanger500)
						.frame(width: 25, height: 25)
					
					Text("Delete this contact")
						.foregroundStyle(Color.redDanger500)
						.default_text_style(styleSize: 14)
						.frame(maxWidth: .infinity, alignment: .leading)
						.lineLimit(1)
						.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
		}
		.background(.white)
		.cornerRadius(15)
		.padding(.horizontal)
		.zIndex(-1)
		.transition(.move(edge: .top))
    }
}

#Preview {
	ContactInnerActionsFragment(
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		showingSheet: .constant(false),
		isShowDeletePopup: .constant(false),
		isShowDismissPopup: .constant(false),
		actionEditButton: {}
	)
}
