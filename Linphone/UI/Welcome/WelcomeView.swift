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

struct WelcomeView: View{
	
	@ObservedObject var sharedMainViewModel : SharedMainViewModel
	
	var permissionManager = PermissionManager.shared
	
	@State private var index = 0
	@State private var isShowPopup = false
	
	var body: some View {
		GeometryReader { geometry in
			ScrollView {
				VStack {
					ZStack {
						Image("mountain")
							.resizable()
							.scaledToFill()
							.frame(width: geometry.size.width, height: 100)
							.clipped()
						
						VStack (alignment: .trailing) {
							Text("Skip")
								.underline()
								.default_text_style_600(styleSize: 15)
								.padding(.top, -35)
								.padding(.trailing, 20)
								.onTapGesture {
									withAnimation {
										self.index = 2
										self.isShowPopup.toggle()
									}
								}
							Text("Welcome")
								.welcome_text_style_white_800(styleSize: 35)
								.padding(.trailing, 100)
								.frame(width: geometry.size.width)
								.padding(.bottom, -25)
							Text("to Linphone")
								.welcome_text_style_white_800(styleSize: 25)
								.padding(.leading, 100)
								.frame(width: geometry.size.width)
								.padding(.bottom, -10)
						}
						.frame(width: geometry.size.width)
					}
					.padding(.top, 35)
					.padding(.bottom, 10)
					
					Spacer()
					
					VStack{
						TabView(selection: $index) {
							ForEach((0..<3), id: \.self) { index in
								if index == 0 {
									WelcomePage1Fragment()
								} else if index == 1 {
									WelcomePage2Fragment()
								} else if index == 2 {
									WelcomePage3Fragment()
								} else {
									WelcomePage1Fragment()
								}
							}
						}
						.tabViewStyle(PageTabViewStyle(indexDisplayMode: .always))
						.frame(minHeight: 300)
						.onAppear {
							setupAppearance()
						}
					}
					
					Spacer()
					
					Button(action:  {
						if index < 2 {
							withAnimation {
								index += 1
							}
						} else if index == 2 {
							withAnimation{
								self.isShowPopup.toggle()
							}
						}
					}) {
						Text(index == 2 ? "Start" : "Next")
							.default_text_style_white_600(styleSize: 20)
							.frame(height: 35)
							.frame(maxWidth: .infinity)
					}
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.background(Color.orange_main_500)
					.cornerRadius(60)
					.padding(.horizontal)
				}
				.frame(minHeight: geometry.size.height)
			}
			
			if self.isShowPopup {
				PopupView(isShowPopup: $isShowPopup, title: Text("Conditions de service"), content: Text("En continuant, vous acceptez ces conditions, \(Text("[notre politique de confidentialité](https://linphone.org/privacy-policy)").underline()) et \(Text("[nos conditions d’utilisation](https://linphone.org/general-terms)").underline())."), titleFirstButton: Text("Deny all"), actionFirstButton: {self.isShowPopup.toggle()}, titleSecondButton: Text("Accept all"), actionSecondButton: {permissionManager.photoLibraryRequestPermission()})
					.background(.black.opacity(0.65))
					.onTapGesture {
						self.isShowPopup.toggle()
					}
			}
		}
		.onReceive(permissionManager.$photoLibraryPermissionGranted, perform: { (granted) in
			if granted {
				withAnimation {
					sharedMainViewModel.changeGeneralTerms()
				}
			}
		})
	}
	
	func setupAppearance() {
		UIPageControl.appearance().currentPageIndicatorTintColor = UIColor(Color.orange_main_500)
		if #available(iOS 16.0, *) {
			
			let dotCurrentImage = UIImage(named: "current-dot")
			let dotImage = UIImage(named: "dot")
			
			UIPageControl.appearance().setCurrentPageIndicatorImage(dotCurrentImage, forPage: 0)
			UIPageControl.appearance().setCurrentPageIndicatorImage(dotCurrentImage, forPage: 1)
			UIPageControl.appearance().setCurrentPageIndicatorImage(dotCurrentImage, forPage: 2)
			
			UIPageControl.appearance().setIndicatorImage(dotImage, forPage: 0)
			UIPageControl.appearance().setIndicatorImage(dotImage, forPage: 1)
			UIPageControl.appearance().setIndicatorImage(dotImage, forPage: 2)
		}
		UIPageControl.appearance().pageIndicatorTintColor = UIColor(Color.gray_main2_200)
	}
}

#Preview {
	WelcomeView(sharedMainViewModel: SharedMainViewModel())
}
