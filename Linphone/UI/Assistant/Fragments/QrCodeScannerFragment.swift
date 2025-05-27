/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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

struct QrCodeScannerFragment: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	
	@Environment(\.dismiss) var dismiss
	
	@State var scanResult = "Scan a QR code"
	
	var body: some View {
		ZStack(alignment: .top) {
			QRScanner(result: $scanResult)
			
			Text(scanResult)
				.default_text_style_white_800(styleSize: 20)
				.padding(.top, 175)
			
			HStack {
				Button {
					dismiss()
				} label: {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
						.padding(.leading, -10)
				}
				.padding()
				.padding(.top, 50)
				
				Spacer()
			}
		}
		.edgesIgnoringSafeArea(.all)
		.navigationBarHidden(true)
		
		/*
		if $isShowToast {
			ZStack {
				
			}.onAppear {
				dismiss()
			}
		}
		 */
	}
}

#Preview {
	QrCodeScannerFragment()
}
