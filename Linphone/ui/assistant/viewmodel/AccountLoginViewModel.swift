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

import linphonesw

class AccountLoginViewModel : ObservableObject {
	
	var coreContext = CoreContext.shared
	@Published var username : String = "user"
	@Published var passwd : String = "pwd"
	@Published var domain : String = "sip.example.org"
	@Published var transportType : String = "TLS"
	
	init() {}
	
	func login() {
		do {
			// Get the transport protocol to use.
			// TLS is strongly recommended
			// Only use UDP if you don't have the choice
			var transport : TransportType
			if (transportType == "TLS") { transport = TransportType.Tls }
			else if (transportType == "TCP") { transport = TransportType.Tcp }
			else  { transport = TransportType.Udp }
			
			// To configure a SIP account, we need an Account object and an AuthInfo object
			// The first one is how to connect to the proxy server, the second one stores the credentials
			
			// The auth info can be created from the Factory as it's only a data class
			// userID is set to null as it's the same as the username in our case
			// ha1 is set to null as we are using the clear text password. Upon first register, the hash will be computed automatically.
			// The realm will be determined automatically from the first register, as well as the algorithm
			let authInfo = try Factory.Instance.createAuthInfo(username: username, userid: "", passwd: passwd, ha1: "", realm: "", domain: domain)
			
			// Account object replaces deprecated ProxyConfig object
			// Account object is configured through an AccountParams object that we can obtain from the Core
			
			let accountParams = try coreContext.mCore!.createAccountParams()
			
			// A SIP account is identified by an identity address that we can construct from the username and domain
			let identity = try Factory.Instance.createAddress(addr: String("sip:" + username + "@" + domain))
			try! accountParams.setIdentityaddress(newValue: identity)
			
			// We also need to configure where the proxy server is located
			let address = try Factory.Instance.createAddress(addr: String("sip:" + domain))
			
			// We use the Address object to easily set the transport protocol
			try address.setTransport(newValue: transport)
			try accountParams.setServeraddress(newValue: address)
			// And we ensure the account will start the registration process
			accountParams.registerEnabled = true
			
			// Now that our AccountParams is configured, we can create the Account object
			let account = try coreContext.mCore!.createAccount(params: accountParams)
			
			// Now let's add our objects to the Core
			coreContext.mCore!.addAuthInfo(info: authInfo)
			try coreContext.mCore!.addAccount(account: account)
			
			// Also set the newly added account as default
			coreContext.mCore!.defaultAccount = account
			
		} catch { NSLog(error.localizedDescription) }
	}
	
	func unregister() {
		// Here we will disable the registration of our Account
		if let account = coreContext.mCore!.defaultAccount {
			
			let params = account.params
			// Returned params object is const, so to make changes we first need to clone it
			let clonedParams = params?.clone()
			
			// Now let's make our changes
			clonedParams?.registerEnabled = false
			
			// And apply them
			account.params = clonedParams
		}
	}
	
	func delete() {
		// To completely remove an Account
		if let account = coreContext.mCore!.defaultAccount {
			coreContext.mCore!.removeAccount(account: account)
			
			// To remove all accounts use
			coreContext.mCore!.clearAccounts()
			
			// Same for auth info
			coreContext.mCore!.clearAllAuthInfo()
		}
	}
}
