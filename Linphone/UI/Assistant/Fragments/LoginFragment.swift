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

struct LoginFragment: View {
    
    @ObservedObject private var coreContext = CoreContext.shared
    @ObservedObject var accountLoginViewModel : AccountLoginViewModel
    @ObservedObject var sharedMainViewModel : SharedMainViewModel
    
    @State private var isSecured: Bool = true
    
    @FocusState var isNameFocused:Bool
    @FocusState var isPasswordFocused:Bool
    
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
                            Text("assistant_account_login")
                                .default_text_style_white_800(styleSize: 20)
                                .padding(.top, 20)
                        }
                        .padding(.top, 35)
                        .padding(.bottom, 10)
                        
                        VStack(alignment: .leading) {
                            Text(String(localized: "username")+"*")
                                .default_text_style_700(styleSize: 15)
                                .padding(.bottom, -5)
                            
                            TextField("username", text : $accountLoginViewModel.username)
                                .default_text_style(styleSize: 15)
                                .disabled(coreContext.loggedIn)
                                .frame(height: 25)
                                .padding(.horizontal, 20)
                                .padding(.vertical, 15)
                                .cornerRadius(60)
                                .overlay(
                                    RoundedRectangle(cornerRadius: 60)
                                        .inset(by: 0.5)
                                        .stroke(isNameFocused ? Color.orange_main_500 : Color.gray_200, lineWidth: 1)
                                )
                                .padding(.bottom)
                                .focused($isNameFocused)
                            
                            Text(String(localized: "password")+"*")
                                .default_text_style_700(styleSize: 15)
                                .padding(.bottom, -5)
                            
                            ZStack(alignment: .trailing) {
                                Group {
                                    if isSecured {
                                        SecureField("password", text: $accountLoginViewModel.passwd)
                                            .default_text_style(styleSize: 15)
                                            .frame(height: 25)
                                            .focused($isPasswordFocused)
                                    } else {
                                        TextField("password", text: $accountLoginViewModel.passwd)
                                            .default_text_style(styleSize: 15)
                                            .frame(height: 25)
                                            .focused($isPasswordFocused)
                                    }
                                }
                                Button(action: {
                                    isSecured.toggle()
                                }) {
                                    Image(self.isSecured ? "eye-slash" : "eye")
                                        .renderingMode(.template)
                                        .resizable()
                                        .foregroundStyle(Color.gray_main2_500)
                                        .frame(width: 20, height: 20)
                                }
                            }
                            .disabled(coreContext.loggedIn)
                            .padding(.horizontal, 20)
                            .padding(.vertical, 15)
                            .cornerRadius(60)
                            .overlay(
                                RoundedRectangle(cornerRadius: 60)
                                    .inset(by: 0.5)
                                    .stroke(isPasswordFocused ? Color.orange_main_500 : Color.gray_200, lineWidth: 1)
                            )
                            .padding(.bottom)
                            
                            Button(action:  {
                                sharedMainViewModel.displayProfileMode = true
                                if (self.coreContext.loggedIn){
                                    self.accountLoginViewModel.unregister()
                                    self.accountLoginViewModel.delete()
                                } else {
                                    self.accountLoginViewModel.login()
                                }
                            }) {
                                Text(coreContext.loggedIn ? "Log out" : "assistant_account_login")
                                    .default_text_style_white_600(styleSize: 20)
                                    .frame(height: 35)
                                    .frame(maxWidth: .infinity)
                            }
                            .padding(.horizontal, 20)
                            .padding(.vertical, 10)
                            .background((accountLoginViewModel.username.isEmpty || accountLoginViewModel.passwd.isEmpty) ? Color.orange_main_100 : Color.orange_main_500)
                            .cornerRadius(60)
                            .disabled(accountLoginViewModel.username.isEmpty || accountLoginViewModel.passwd.isEmpty)
                            .padding(.bottom)
                            
                            HStack {
                                Text("[Forgotten password?](https://subscribe.linphone.org/)")
                                    .underline()
                                    .tint(Color.gray_main2_600)
                                    .default_text_style_600(styleSize: 15)
                                    .foregroundStyle(Color.gray_main2_500)
                            }
                            .frame(maxWidth: .infinity)
                            .padding(.bottom, 30)
                            
                            HStack {
                                VStack{
                                    Divider()
                                }
                                Text(" or ")
                                    .default_text_style(styleSize: 15)
                                    .foregroundStyle(Color.gray_main2_500)
                                VStack{
                                    Divider()
                                }
                            }
                            .padding(.bottom, 10)
                            
                            Button(action:  {
                                
                            }) {
                                HStack {
                                    Image("qr-code")
                                        .renderingMode(.template)
                                        .resizable()
                                        .foregroundStyle(Color.orange_main_500)
                                        .frame(width: 20, height: 20)
                                    
                                    Text("Scan QR code")
                                        .default_text_style_orange_600(styleSize: 20)
                                        .frame(height: 35)
                                }
                                .frame(maxWidth: .infinity)
                            }
                            .padding(.horizontal, 20)
                            .padding(.vertical, 10)
                            .cornerRadius(60)
                            .overlay(
                                RoundedRectangle(cornerRadius: 60)
                                    .inset(by: 0.5)
                                    .stroke(Color.orange_main_500, lineWidth: 1)
                            )
                            .padding(.bottom)
                            
                            NavigationLink(destination: {
                                ThirdPartySipAccountWarningFragment(accountLoginViewModel: accountLoginViewModel)
                            }, label: {
                                Text("Use SIP Account")
                                    .default_text_style_orange_600(styleSize: 20)
                                    .frame(height: 35)
                                    .frame(maxWidth: .infinity)
                                
                            })
                            .padding(.horizontal, 20)
                            .padding(.vertical, 10)
                            .cornerRadius(60)
                            .overlay(
                                RoundedRectangle(cornerRadius: 60)
                                    .inset(by: 0.5)
                                    .stroke(Color.orange_main_500, lineWidth: 1)
                            )
                            .padding(.bottom)
                            
                            HStack(alignment: .center) {
                                
                                Spacer()
                                
                                Text("Not account yet?")
                                    .default_text_style(styleSize: 15)
                                    .foregroundStyle(Color.gray_main2_700)
                                    .padding(.horizontal, 10)
                                
                                Button(action:  {
                                    
                                }) {
                                    Text("Register")
                                        .default_text_style_orange_600(styleSize: 20)
                                        .frame(height: 35)
                                }
                                .padding(.horizontal, 20)
                                .padding(.vertical, 10)
                                .cornerRadius(60)
                                .overlay(
                                    RoundedRectangle(cornerRadius: 60)
                                        .inset(by: 0.5)
                                        .stroke(Color.orange_main_500, lineWidth: 1)
                                )
                                .padding(.horizontal, 10)
                                
                                Spacer()
                            }
                            .padding(.bottom)
                        }
                        .padding(.horizontal, 20)
                    }
                }
            }
        }
    }
}

#Preview {
    LoginFragment(accountLoginViewModel: AccountLoginViewModel(), sharedMainViewModel: SharedMainViewModel())
}
