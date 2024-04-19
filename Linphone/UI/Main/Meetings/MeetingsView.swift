//
//  MeetingsView.swift
//  Linphone
//
//  Created by QuentinArguillere on 18/04/2024.
//

import SwiftUI

struct MeetingsView: View {
	
	@ObservedObject var scheduleMeetingViewModel: ScheduleMeetingViewModel
	
	@Binding var isShowScheduleMeetingFragment: Bool
		
	var body: some View {
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				MeetingsFragment(scheduleMeetingViewModel: scheduleMeetingViewModel)
				
				Button {
					withAnimation {
						isShowScheduleMeetingFragment.toggle()
					}
				} label: {
					Image("plus-circle")
						.renderingMode(.template)
						.foregroundStyle(.white)
						.padding()
						.background(Color.orangeMain500)
						.clipShape(Circle())
						.shadow(color: .black.opacity(0.2), radius: 4)
					
				}
				.padding()
			}
		}
		.navigationViewStyle(.stack)
	}
}

#Preview {
	MeetingsView(
		scheduleMeetingViewModel: ScheduleMeetingViewModel(),
		isShowScheduleMeetingFragment: .constant(false)
	)
}
