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

struct AssistantView: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject var accountLoginViewModel : AccountLoginViewModel
	
	var body: some View {
		VStack {
			ZStack {
				Image("Mountain")
					.resizable()
					.frame(width: 1080, height: 108)
				Text("assistant_account_login")
					.font(Font.custom("NotoSans-ExtraBold", size: 20))
					.foregroundColor(.white)
			}
			.padding(.top, 35)
			
			HStack(alignment: .center, spacing: 0) {
				VStack(alignment: .leading, spacing: 0) {
					Text(String(localized: "username")+"*")
						.font(Font.custom("Noto Sans", size: 15)
						.weight(.bold))
						.padding(.bottom, 5)
					
					TextField("username", text : $accountLoginViewModel.username)
						.font(Font.custom("Noto Sans", size: 15))
						.disabled(coreContext.loggedIn)
						.frame(height: 20)
						.padding(.horizontal, 20)
						.padding(.vertical, 15)
						.frame(maxWidth: .infinity, alignment: .leading)
						.background(Color(red: 0.98, green: 0.98, blue: 0.98))
						.cornerRadius(60)
						.overlay(
						RoundedRectangle(cornerRadius: 63)
							.inset(by: 0.5)
							.stroke(Color(red: 0.93, green: 0.93, blue: 0.93), lineWidth: 1)
						)
						.padding(.bottom, 15)
					
					Text(String(localized: "password")+"*")
						.font(Font.custom("Noto Sans", size: 15)
						.weight(.bold))
						.padding(.bottom, 5)
					
					SecureInputView(String(localized: "password"), text: $accountLoginViewModel.passwd)
						.disabled(coreContext.loggedIn)
						.frame(height: 20)
						.padding(.horizontal, 20)
						.padding(.vertical, 15)
						.frame(maxWidth: .infinity, alignment: .leading)
						.background(Color(red: 0.98, green: 0.98, blue: 0.98))
						.cornerRadius(63)
						.overlay(
						RoundedRectangle(cornerRadius: 63)
							.inset(by: 0.5)
							.stroke(Color(red: 0.93, green: 0.93, blue: 0.93), lineWidth: 1)
						)
						.padding(.bottom, 32)
					
					Button(action: accountLoginViewModel.login) {
						Text("assistant_account_login")
							.font(Font.custom("NotoSans-ExtraBold", size: 20))
							.foregroundColor(.white)
					}
					.disabled(coreContext.loggedIn)
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.frame(maxWidth: .infinity, alignment: .center)
					.background(Color(red: 1, green: 0.37, blue: 0))
					.cornerRadius(63)
					.overlay(
					RoundedRectangle(cornerRadius: 63)
						.inset(by: 0.5)
						.stroke(Color(red: 1, green: 0.37, blue: 0), lineWidth: 1)
					)
					
				}
			}
			.padding(.top, 5)
			.padding(.bottom, 20)
			
            VStack {
                HStack {
                    Button(action:  {
                        if (self.coreContext.loggedIn)
                        {
                            self.accountLoginViewModel.unregister()
                            self.accountLoginViewModel.delete()
                        } else {
                            self.accountLoginViewModel.login()
                        }
                    })
                    {
                        Text(coreContext.loggedIn ? "Log out & \ndelete account" : "Create & \nlog in account")
                            .font(.largeTitle)
                            .foregroundColor(Color.white)
                            .frame(width: 220.0, height: 90)
                            .background(Color.gray)
                    }
                    
                }
                HStack {
                    Text("Login State : ")
                        .font(.footnote)
                    Text(coreContext.loggedIn ? "Looged in" : "Unregistered")
                        .font(.footnote)
                        .foregroundColor(coreContext.loggedIn ? Color.green : Color.black)
                }.padding(.top, 10.0)
            }
			Group {
				Spacer()
				Text("Core Version is \(coreContext.coreVersion)")
			}
		}
		.padding()
	}
}

struct AssistantView_Previews: PreviewProvider {
	
	static var previews: some View {
		AssistantView(accountLoginViewModel: AccountLoginViewModel())
	}
}


struct SecureInputView: View {
	
	@Binding private var text: String
	@State private var isSecured: Bool = true
	private var title: String
	
	init(_ title: String, text: Binding<String>) {
		self.title = title
		self._text = text
	}
	
	var body: some View {
		ZStack(alignment: .trailing) {
			Group {
				if isSecured {
					SecureField(title, text: $text)
						.font(Font.custom("Noto Sans", size: 15))
				} else {
					TextField(title, text: $text)
						.font(Font.custom("Noto Sans", size: 15))
				}
			}.padding(.trailing, 32)

			Button(action: {
				isSecured.toggle()
			}) {
				Image(systemName: self.isSecured ? "eye.slash" : "eye")
					.accentColor(.gray)
			}
		}
	}
}
