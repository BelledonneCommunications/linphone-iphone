//
//  HistoryView.swift
//  Linphone
//
//  Created by Benoît Martins on 03/10/2023.
//

import SwiftUI

struct HistoryView: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	
    var body: some View {
		VStack {
			Spacer()
			Image("linphone")
				.padding(.bottom, 20)
			Text("History View")
			Spacer()
		}
    }
}

#Preview {
    HistoryView()
}
