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

struct WelcomeView: View {
	
	@State private var index = 0
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				if #available(iOS 16.4, *) {
					ScrollView(.vertical) {
						innerScrollView(geometry: geometry)
					}
					.scrollBounceBehavior(.basedOnSize)
				} else {
					ScrollView(.vertical) {
						innerScrollView(geometry: geometry)
					}
				}
			}
			.navigationTitle("")
   			.navigationBarHidden(true)
			.edgesIgnoringSafeArea(.bottom)
			.edgesIgnoringSafeArea(.horizontal)
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
	
	func innerScrollView(geometry: GeometryProxy) -> some View {
		VStack {
			ZStack {
				VStack(alignment: .trailing) {
					NavigationLink(destination: {
						PermissionsFragment()
					}, label: {
						Text("welcome_carousel_skip")
							.underline()
							.default_text_style_600(styleSize: 15)
						
					})
					.padding(.top, -35)
					.padding(.trailing, 20)
					.simultaneousGesture(
						TapGesture().onEnded {
							self.index = 2
						}
					)
					Text("welcome_page_title")
						.default_text_style_800(styleSize: 35)
						.padding(.trailing, 100)
						.frame(width: geometry.size.width)
						.padding(.bottom, -25)
					Text(String(format: String(localized: "welcome_page_subtitle"), Bundle.main.displayName))
						.default_text_style_800(styleSize: 25)
						.padding(.leading, 100)
						.frame(width: geometry.size.width)
						.padding(.bottom, -10)
				}
				.frame(width: geometry.size.width)
			}
			.padding(.top, 35)
			.padding(.bottom, 10)
			
			Spacer()
			
			VStack {
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
			
			if index == 2 {
				NavigationLink(destination: {
					PermissionsFragment()
				}, label: {
					Text("start")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
					
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(Color.orangeMain500)
				.cornerRadius(60)
				.padding(.horizontal)
				.padding(.bottom, geometry.safeAreaInsets.bottom.isEqual(to: 0.0) ? 20 : 0)
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			} else {
				Button(action: {
					withAnimation {
						index += 1
					}
				}, label: {
					Text("next")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(Color.orangeMain500)
				.cornerRadius(60)
				.padding(.horizontal)
				.padding(.bottom, geometry.safeAreaInsets.bottom.isEqual(to: 0.0) ? 20 : 0)
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			}
			
			Image("mountain2")
				.resizable()
				.scaledToFill()
				.frame(width: geometry.size.width, height: 60)
				.clipped()
		}
		.frame(minHeight: geometry.size.height)
		.onAppear {
			PermissionManager.shared.havePermissionsAlreadyBeenRequested()
		}
	}
	
	func setupAppearance() {
		UIPageControl.appearance().currentPageIndicatorTintColor = UIColor(Color.orangeMain500)
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
		UIPageControl.appearance().pageIndicatorTintColor = UIColor(Color.grayMain2c200)
	}
}

#Preview {
	WelcomeView()
}
