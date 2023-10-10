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

final class CoreContext: ObservableObject {
	
	static let shared = CoreContext()
	
	var mCore: Core!
	var mRegistrationDelegate: CoreDelegate!
	var mConfigurationDelegate: CoreDelegate!
	
	var coreVersion: String = Core.getVersion
	@Published var loggedIn: Bool = false
	@Published var loggingInProgress: Bool = false
	@Published var toastMessage: String = ""
	
	private init() {}
	
	func initialiseCore() async throws {
		LoggingService.Instance.logLevel = LogLevel.Debug
		
		let factory = Factory.Instance
		let configDir = factory.getConfigDir(context: nil)
		try? mCore = Factory.Instance.createCore(configPath: "\(configDir)/MyConfig", factoryConfigPath: "", systemContext: nil)
		try? mCore.start()
		
		// Create a Core listener to listen for the callback we need
		// In this case, we want to know about the account registration status
		
		mRegistrationDelegate =
		CoreDelegateStub(
			onConfiguringStatus: {(_: Core, state: Config.ConfiguringState, message: String) in
				NSLog("New configuration state is \(state) = \(message)\n")
				if state == .Successful {
					self.toastMessage = "Successful"
				} else {
					self.toastMessage = "Failed"
				}
			},
			
			onAccountRegistrationStateChanged: {(_: Core, account: Account, state: RegistrationState, message: String) in
				// If account has been configured correctly, we will go through Progress and Ok states
				// Otherwise, we will be Failed.
				NSLog("New registration state is \(state) for user id \( String(describing: account.params?.identityAddress?.asString())) = \(message)\n")
				if state == .Ok {
					self.loggingInProgress = false
					self.loggedIn = true
				} else if state == .Progress {
					self.loggingInProgress = true
				} else {
					self.toastMessage = "Registration failed"
					self.loggingInProgress = false
					self.loggedIn = false
				}
			}
		)
		
		mCore.addDelegate(delegate: mRegistrationDelegate)
	}
}
