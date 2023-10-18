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

struct ContentView: View {
    
    @ObservedObject var sharedMainViewModel: SharedMainViewModel
    @ObservedObject var contactViewModel: ContactViewModel
    @ObservedObject var historyViewModel: HistoryViewModel
    @ObservedObject private var coreContext = CoreContext.shared
    
    @State var index = 0
    @State private var orientation = UIDevice.current.orientation
    @State var menuOpen: Bool = false
    
    var body: some View {
        if !sharedMainViewModel.welcomeViewDisplayed {
            WelcomeView(sharedMainViewModel: sharedMainViewModel)
        } else if coreContext.mCore.defaultAccount == nil || sharedMainViewModel.displayProfileMode {
            AssistantView(sharedMainViewModel: sharedMainViewModel)
        } else {
            GeometryReader { geometry in
                ZStack {
                    VStack(spacing: 0) {
                        HStack(spacing: 0) {
                            if orientation == .landscapeLeft 
                                || orientation == .landscapeRight
                                || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height {
                                VStack {
                                    Group {
                                        Spacer()
                                        Button(action: {
                                            self.index = 0
                                        }, label: {
                                            VStack {
                                                Image("address-book")
                                                    .renderingMode(.template)
                                                    .resizable()
                                                    .foregroundStyle(self.index == 0 ? Color.orangeMain500 : Color.grayMain2c600)
                                                    .frame(width: 25, height: 25)
                                                if self.index == 0 {
                                                    Text("Contacts")
                                                        .default_text_style_700(styleSize: 10)
                                                } else {
                                                    Text("Contacts")
                                                        .default_text_style(styleSize: 10)
                                                }
                                            }
                                        })
                                        
                                        Spacer()
                                        
                                        Button(action: {
                                            self.index = 1
                                            contactViewModel.contactTitle = ""
                                        }, label: {
                                            VStack {
                                                Image("phone")
                                                    .renderingMode(.template)
                                                    .resizable()
                                                    .foregroundStyle(self.index == 1 ? Color.orangeMain500 : Color.grayMain2c600)
                                                    .frame(width: 25, height: 25)
                                                if self.index == 1 {
                                                    Text("Calls")
                                                        .default_text_style_700(styleSize: 10)
                                                } else {
                                                    Text("Calls")
                                                        .default_text_style(styleSize: 10)
                                                }
                                            }
                                        })
                                        
                                        Spacer()
                                    }
                                }
                                .frame(width: 75)
                                .padding(.leading, 
                                         orientation == .landscapeRight && geometry.safeAreaInsets.bottom > 0
                                         ? -geometry.safeAreaInsets.leading
                                         : 0)
                            }
                            
                            VStack(spacing: 0) {
                                HStack {
                                    Image("profile-image-example")
                                        .resizable()
                                        .frame(width: 40, height: 40)
                                        .clipShape(Circle())
                                        .onTapGesture {
                                            openMenu()
                                        }
                                    
                                    Text(index == 0 ? "Contacts" : "Calls")
                                        .default_text_style_white_800(styleSize: 20)
                                        .padding(.leading, 10)
                                    
                                    Spacer()
                                    
                                    Button {
                                        
                                    } label: {
                                        Image("search")
                                    }
                                    
                                    Button {
                                        
                                    } label: {
                                        Image(index == 0 ? "filtres" : "more")
                                    }
                                    .padding(.leading)
                                }
                                .frame(maxWidth: .infinity)
                                .frame(height: 50)
                                .padding(.horizontal)
                                .background(Color.orangeMain500)
                                
                                if self.index == 0 {
                                    ContactsView(contactViewModel: contactViewModel, historyViewModel: historyViewModel)
                                } else if self.index == 1 {
                                    HistoryView()
                                }
                            }
                            .frame(maxWidth:
                                    (orientation == .landscapeLeft 
                                     || orientation == .landscapeRight
                                     || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
                                   ? geometry.size.width/100*40
                                   : .infinity
                            )
                            .background(
                                Color.white
                                    .shadow(color: Color.gray200, radius: 4, x: 0, y: 0)
                                    .mask(Rectangle().padding(.horizontal, -8))
                            )
                            
                            if orientation == .landscapeLeft 
                                || orientation == .landscapeRight
                                || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height {
                                Spacer()
                            }
                        }
                        
                        if !(orientation == .landscapeLeft 
                             || orientation == .landscapeRight
                             || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
                            HStack {
                                Group {
                                    Spacer()
                                    Button(action: {
                                        self.index = 0
                                    }, label: {
                                        VStack {
                                            Image("address-book")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(self.index == 0 ? Color.orangeMain500 : Color.grayMain2c600)
                                                .frame(width: 25, height: 25)
                                            if self.index == 0 {
                                                Text("Contacts")
                                                    .default_text_style_700(styleSize: 10)
                                            } else {
                                                Text("Contacts")
                                                    .default_text_style(styleSize: 10)
                                            }
                                        }
                                    })
                                    .padding(.top)
                                    
                                    Spacer()
                                    
                                    Button(action: {
                                        self.index = 1
                                        contactViewModel.contactTitle = ""
                                    }, label: {
                                        VStack {
                                            Image("phone")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(self.index == 1 ? Color.orangeMain500 : Color.grayMain2c600)
                                                .frame(width: 25, height: 25)
                                            if self.index == 1 {
                                                Text("Calls")
                                                    .default_text_style_700(styleSize: 10)
                                            } else {
                                                Text("Calls")
                                                    .default_text_style(styleSize: 10)
                                            }
                                        }
                                    })
                                    .padding(.top)
                                    Spacer()
                                }
                            }
                            .padding(.bottom, geometry.safeAreaInsets.bottom > 0 ? 0 : 15)
                            .background(
                                Color.white
                                    .shadow(color: Color.gray200, radius: 4, x: 0, y: 0)
                                    .mask(Rectangle().padding(.top, -8))
                            )
                        }
                    }
                    
                    if !contactViewModel.contactTitle.isEmpty || !historyViewModel.historyTitle.isEmpty {
                        HStack(spacing: 0) {
                            Spacer()
                                .frame(maxWidth:
                                        (orientation == .landscapeLeft 
                                         || orientation == .landscapeRight
                                         || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
                                       ? (geometry.size.width/100*40) + 75
                                       : 0
                                )
                            if self.index == 0 {
                                ContactFragment(contactViewModel: contactViewModel)
                                    .frame(maxWidth: .infinity)
                                    .background(Color.gray100)
                            } else if self.index == 1 {
                                HistoryContactFragment()
                                    .frame(maxWidth: .infinity)
                                    .background(Color.gray100)
                            }
                        }
                        .padding(.leading, 
                                 orientation == .landscapeRight && geometry.safeAreaInsets.bottom > 0
                                 ? -geometry.safeAreaInsets.leading
                                 : 0)
                        .transition(.move(edge: .trailing))
                    }
                    
                    SideMenu(
                        width: geometry.size.width / 5 * 4,
                        isOpen: self.menuOpen,
                        menuClose: self.openMenu,
                        safeAreaInsets: geometry.safeAreaInsets
                    )
                    .ignoresSafeArea(.all)
                }
            }
            .onRotate { newOrientation in
                orientation = newOrientation
            }
        }
    }
    
    func openMenu() {
        withAnimation {
            self.menuOpen.toggle()
        }
    }
}

#Preview {
    ContentView(sharedMainViewModel: SharedMainViewModel(), contactViewModel: ContactViewModel(), historyViewModel: HistoryViewModel())
}
