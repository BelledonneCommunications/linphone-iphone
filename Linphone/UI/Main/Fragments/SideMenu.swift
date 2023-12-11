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
import linphonesw
import UniformTypeIdentifiers

struct SideMenu: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	
    let width: CGFloat
    let isOpen: Bool
    let menuClose: () -> Void
    let safeAreaInsets: EdgeInsets
    
    var body: some View {
        ZStack {
            GeometryReader { _ in
                EmptyView()
            }
            .background(Color.gray.opacity(0.3))
            .opacity(self.isOpen ? 1.0 : 0.0)
            .onTapGesture {
                self.menuClose()
            }
            
            HStack {
                List {
                    Text("My Profile").onTapGesture {
                        print("My Profile")
                    }
                    Text("Send logs").onTapGesture {
						sendLogs()
                    }
					Text("Clear logs").onTapGesture {
						print("Clear logs")
						Core.resetLogCollection()
	 				}
                    Text("Logout").onTapGesture {
                        print("Logout")
                    }
                }
                .frame(width: self.width - safeAreaInsets.leading)
                .background(Color.white)
                .offset(x: self.isOpen ? 0 : -self.width)
                
                Spacer()
            }
            .padding(.leading, safeAreaInsets.leading)
            .padding(.top, safeAreaInsets.top)
            .padding(.bottom, safeAreaInsets.bottom)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
	
	func sendLogs() {
		coreContext.doOnCoreQueue { core in
			core.uploadLogCollection()
		}
	}
}
