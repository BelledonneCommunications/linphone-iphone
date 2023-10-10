//
//  RegisterFragment.swift
//  Linphone
//
//  Created by Benoît Martins on 09/10/2023.
//

import SwiftUI

struct RegisterFragment: View {
	
	@Environment(\.dismiss) var dismiss
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				ScrollView(.vertical) {
					VStack {
						ZStack {
							Image("mountain")
								.resizable()
								.scaledToFill()
								.frame(width: geometry.size.width, height: 100)
								.clipped()
							
							VStack(alignment: .leading) {
								HStack {
									Image("caret-left")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c500)
										.frame(width: 25, height: 25, alignment: .leading)
										.padding(.top, -65)
										.onTapGesture {
											withAnimation {
												dismiss()
											}
										}
									
									Spacer()
								}
								.padding(.leading)
							}
							.frame(width: geometry.size.width)
							
							Text("Register")
								.default_text_style_white_800(styleSize: 20)
								.padding(.top, 20)
						}
						.padding(.top, 35)
						.padding(.bottom, 10)
						
					}
				}
			}
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.navigationBarHidden(true)
	}
}

#Preview {
	RegisterFragment()
}
