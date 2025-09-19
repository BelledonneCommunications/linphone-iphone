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
import SwiftUI

class AccountLoginViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	@Published var username: String = ""
	@Published var passwd: String = ""
	@Published var domain: String = "sip.linphone.org"
	@Published var displayName: String = ""
	@Published var transportType: String = "TLS"
	
	private var mCoreDelegate: CoreDelegate!
	
	init() {}
	
	func login() {
		coreContext.doOnCoreQueue { core in
			guard self.coreContext.networkStatusIsConnected else {
				DispatchQueue.main.async {
					self.coreContext.loggingInProgress = false
					ToastViewModel.shared.toastMessage = "Unavailable_network"
					ToastViewModel.shared.displayToast = true
				}
				return
			}
			do {
				let usernameWithDomain = self.username.split(separator: "@")
				
				if usernameWithDomain.count > 1 {
					DispatchQueue.main.async {
						self.domain = String(usernameWithDomain.last ?? "")
						self.username = String(usernameWithDomain.first ?? "")
					}
				}
				
				if self.domain != "sip.linphone.org" {
					if let assistantLinphone = Bundle.main.path(forResource: "assistant_third_party_default_values", ofType: nil) {
						core.loadConfigFromXml(xmlUri: assistantLinphone)
					}
				} else {
					if let assistantLinphone = Bundle.main.path(forResource: "assistant_linphone_default_values", ofType: nil) {
						core.loadConfigFromXml(xmlUri: assistantLinphone)
					}
				}
				
				// Get the transport protocol to use.
				// TLS is strongly recommended
				// Only use UDP if you don't have the choice
				var transport: TransportType
				if self.transportType == "TLS" {
					transport = TransportType.Tls
				} else if self.transportType == "TCP" {
					transport = TransportType.Tcp
				} else { transport = TransportType.Udp }
				
				// To configure a SIP account, we need an Account object and an AuthInfo object
				// The first one is how to connect to the proxy server, the second one stores the credentials
				
				// The auth info can be created from the Factory as it's only a data class
				// userID is set to null as it's the same as the username in our case
				// ha1 is set to null as we are using the clear text password. Upon first register, the hash will be computed automatically.
				// The realm will be determined automatically from the first register, as well as the algorithm
				let authInfo = try Factory.Instance.createAuthInfo(
					username: self.username,
					userid: "",
					passwd: self.passwd,
					ha1: "",
					realm: "",
					domain: self.domain
				)
				
				// Account object replaces deprecated ProxyConfig object
				// Account object is configured through an AccountParams object that we can obtain from the Core
				
				let accountParams = try core.createAccountParams()
				
				// A SIP account is identified by an identity address that we can construct from the username and domain
				let identity = try Factory.Instance.createAddress(addr: String("sip:" + self.username + "@" + self.domain))
				try accountParams.setIdentityaddress(newValue: identity)
				
				// We also need to configure where the proxy server is located
				let address = try Factory.Instance.createAddress(addr: String("sip:" + self.domain))
				
				// We use the Address object to easily set the transport protocol
				try address.setTransport(newValue: transport)
				try accountParams.setServeraddress(newValue: address)
				// And we ensure the account will start the registration process
				accountParams.registerEnabled = true
				
				if accountParams.pushNotificationAllowed {
					accountParams.pushNotificationAllowed = true
					accountParams.remotePushNotificationAllowed = true
				}
#if DEBUG
				let pushEnvironment = ".dev"
#else
				let pushEnvironment = ""
#endif
				accountParams.pushNotificationConfig?.provider = "apns" + pushEnvironment
				
				accountParams.internationalPrefix = "33"
				accountParams.internationalPrefixIsoCountryCode = "FRA"
				accountParams.useInternationalPrefixForCallsAndChats = true
				
				self.mCoreDelegate = CoreDelegateStub(onAccountRegistrationStateChanged: { (core: Core, account: Account, state: RegistrationState, message: String) in
					
					Log.info("New registration state is \(state) for user id " +
							 "\( String(describing: account.params?.identityAddress?.asString())) = \(message)\n")
					
					switch state {
					case .Failed:  // If registration failed, remove account from core
						if let authInfo = account.findAuthInfo() {
							core.removeAuthInfo(info: authInfo)
						}
						
						Log.warn("Registration failed for account \(account.displayName()), deleting it from core")
						core.removeAccount(account: account)
					default:
						break
					}
				})
				
				self.coreContext.mCore.addDelegate(delegate: self.mCoreDelegate)
				
				// Now that our AccountParams is configured, we can create the Account object
				let account = try core.createAccount(params: accountParams)
				
				// Now let's add our objects to the Core
				core.addAuthInfo(info: authInfo)
				try core.addAccount(account: account)
				
				// Also set the newly added account as default
				core.defaultAccount = account
				
				DispatchQueue.main.async {
					self.domain = "sip.linphone.org"
					self.transportType = "TLS"
				}
				
			} catch { NSLog(error.localizedDescription) }
		}
	}
	
	func unregister() {
		coreContext.doOnCoreQueue { core in
			// Here we will disable the registration of our Account
			if let account = core.defaultAccount {
				
				let params = account.params
				// Returned params object is const, so to make changes we first need to clone it
				let clonedParams = params?.clone()
				
				// Now let's make our changes
				clonedParams?.registerEnabled = false
				
				// And apply them
				account.params = clonedParams
			}
		}
	}
	
	func delete() {
		coreContext.doOnCoreQueue { core in
			// To completely remove an Account
			if let account = core.defaultAccount {
				core.removeAccount(account: account)
				
				// To remove all accounts use
				core.clearAccounts()
				
				// Same for auth info
				core.clearAllAuthInfo()
			}
		}
	}
}
