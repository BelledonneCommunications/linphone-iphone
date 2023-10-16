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

struct ContactFragment: View {
	
	@ObservedObject var contactViewModel: ContactViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
    var body: some View {
		VStack(alignment: .leading) {
			
			if !(orientation == .landscapeLeft || orientation == .landscapeRight || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				HStack {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.top, 20)
						.onTapGesture {
							withAnimation {
								contactViewModel.contactTitle = ""
							}
						}
					
					Spacer()
				}
				.padding(.leading)
			}
			
			Spacer()
			
			Text(contactViewModel.contactTitle)
				.frame(maxWidth: .infinity)
			
			List {
				ForEach(1...40, id: \.self) { index in
					Button {
						contactViewModel.contactTitle = String(index)
					} label: {
						Text("\(index)")
							.frame( maxWidth: .infinity, alignment: .leading)
					}
					.buttonStyle(.borderless)
				}
			}
		}
		.navigationBarHidden(true)
		.onRotate { newOrientation in
			orientation = newOrientation
		}

    }
}

#Preview {
	ContactFragment(contactViewModel: ContactViewModel())
}
