//
//  StartGroupCallFragment.swift
//  Linphone
//
//  Created by Benoît Martins on 24/06/2024.
//

import SwiftUI

struct StartGroupCallFragment: View {
	@ObservedObject var startCallViewModel: StartCallViewModel
	@State var addParticipantsViewModel = AddParticipantsViewModel()
	
    var body: some View {
		AddParticipantsFragment(addParticipantsViewModel: addParticipantsViewModel, confirmAddParticipantsFunc: startCallViewModel.addParticipants)
			.onAppear {
				addParticipantsViewModel.participantsToAdd = startCallViewModel.participants
			}
    }
}

#Preview {
	StartGroupCallFragment(startCallViewModel: StartCallViewModel())
}
