//
//  SplashScreen.swift
//  Linphone
//
//  Created by Benoît Martins on 03/10/2023.
//

import SwiftUI

struct SplashScreen: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@Binding var isActive: Bool
	
	var body: some View {
		GeometryReader { geometry in
			VStack {
				Spacer()
				HStack {
					Spacer()
					Image("linphone")
					Spacer()
				}
				Spacer()
			}
			
		}
		.ignoresSafeArea(.all)
		.onAppear {
			Task {
				try await coreContext.initialiseCore()
				withAnimation {
					self.isActive = true
				}
			}
		}
	}
}

#Preview {
    SplashScreen(isActive: .constant(true))
}
