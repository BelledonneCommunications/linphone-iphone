//
//  MeetingsFragment.swift
//  Linphone
//
//  Created by QuentinArguillere on 18/04/2024.
//

import SwiftUI

struct MeetingsFragment: View {
	
	@ObservedObject var scheduleMeetingViewModel: ScheduleMeetingViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@State var showingSheet: Bool = false
	
	var body: some View {
		VStack {
			Spacer()
			Text("Hello meetings list")
			Spacer()
			/*
			if #available(iOS 16.0, *), idiom != .pad {
				MeetingsListFragment(conversationViewModel: conversationViewModel, conversationsListViewModel: conversationsListViewModel, showingSheet: $showingSheet)
					.sheet(isPresented: $showingSheet) {
						ConversationsListBottomSheet(
							conversationsListViewModel: conversationsListViewModel,
							showingSheet: $showingSheet
						)
						.presentationDetents([.fraction(0.4)])
					}
			} else {
				ConversationsListFragment(conversationViewModel: conversationViewModel, conversationsListViewModel: conversationsListViewModel, showingSheet: $showingSheet)
					.halfSheet(showSheet: $showingSheet) {
						ConversationsListBottomSheet(
							conversationsListViewModel: conversationsListViewModel,
							showingSheet: $showingSheet
						)
					} onDismiss: {}
			} */
		}
	}
}

#Preview {
	MeetingsFragment(scheduleMeetingViewModel: ScheduleMeetingViewModel())
}
