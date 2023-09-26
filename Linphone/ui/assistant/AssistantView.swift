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
					.frame(width: 1084, height: 108)
				Text("assistant_account_login")
					.font(Font.custom("Noto Sans", size: 20))
					.foregroundColor(.white)
			}
			.padding(.top, 36)
			.padding(.bottom, 16)
            HStack {
                Text("Username:")
                    .font(.title)
                TextField("", text : $accountLoginViewModel.username)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .disabled(coreContext.loggedIn)
            }
            HStack {
                Text("Password:")
                    .font(.title)
				
				SecureField("", text : $accountLoginViewModel.passwd)
					.textFieldStyle(RoundedBorderTextFieldStyle())
	 				.disabled(coreContext.loggedIn)
            }
            HStack {
                Text("Domain:")
                    .font(.title)
                TextField("", text : $accountLoginViewModel.domain)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .disabled(coreContext.loggedIn)
            }
            Picker(selection: $accountLoginViewModel.transportType, label: Text("Transport:")) {
                Text("TLS").tag("TLS")
                Text("TCP").tag("TCP")
                Text("UDP").tag("UDP")
            }.pickerStyle(SegmentedPickerStyle()).padding()
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
