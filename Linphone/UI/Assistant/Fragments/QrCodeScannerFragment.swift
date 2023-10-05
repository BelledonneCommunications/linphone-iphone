//
//  QrCodeScannerFragment.swift
//  Linphone
//
//  Created by Benoît Martins on 05/10/2023.
//

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
			
			HStack{
				Button {
					dismiss()
				} label: {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.white)
						.frame(width: 25, height: 25, alignment: .leading)
				}
				.padding()
				.padding(.top, 50)
				
				Spacer()
			}
		}
		.edgesIgnoringSafeArea(.all)
		.navigationBarHidden(true)
		
		if coreContext.configuringSuccessful == "Successful" {
			ZStack{
				
			}.onAppear {
				dismiss()
			}
		}
    }
}

#Preview {
    QrCodeScannerFragment()
}
