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

// swiftlint:disable large_tuple
import linphonesw
import Combine

final class CoreContext: ObservableObject {
	
	static let shared = CoreContext()
	
	var coreVersion: String = Core.getVersion
	@Published var loggedIn: Bool = false
	@Published var loggingInProgress: Bool = false
	@Published var toastMessage: String = ""
	@Published var defaultAccount: Account?
	
	private var mCore: Core!
	private var mIteratePublisher: AnyCancellable?
	
	private init() {}
	
	func doOnCoreQueue(synchronous : Bool = false, lambda: @escaping (Core) -> Void) {
		if synchronous {
			coreQueue.sync {
				lambda(self.mCore)
			}
		} else {
			coreQueue.async {
				lambda(self.mCore)
			}
		}
	}
	
	func initialiseCore() throws {
		LoggingService.Instance.logLevel = LogLevel.Debug
		
		coreQueue.async {
			let configDir = Factory.Instance.getConfigDir(context: nil)
			try? self.mCore = Factory.Instance.createCore(configPath: "\(configDir)/MyConfig", factoryConfigPath: "", systemContext: nil)
			self.mCore.autoIterateEnabled = false
			self.mCore.friendsDatabasePath = "\(configDir)/friends.db"
			
			self.mCore.publisher?.onGlobalStateChanged?.postOnMainQueue { (cbVal: (core: Core, state: GlobalState, message: String)) in
				if cbVal.state == GlobalState.On {
					self.defaultAccount = self.mCore.defaultAccount
				} else if cbVal.state == GlobalState.Off {
					self.defaultAccount = nil
				}
			}
			try? self.mCore.start()
			
			// Create a Core listener to listen for the callback we need
			// In this case, we want to know about the account registration status
			self.mCore.publisher?.onConfiguringStatus?.postOnMainQueue { (cbVal: (core: Core, status: Config.ConfiguringState, message: String)) in
				NSLog("New configuration state is \(cbVal.status) = \(cbVal.message)\n")
				if cbVal.status == Config.ConfiguringState.Successful {
					self.toastMessage = "Successful"
				} else {
					self.toastMessage = "Failed"
				}
			}
			
			self.mCore.publisher?.onAccountRegistrationStateChanged?.postOnMainQueue { (cbVal: (core: Core, account: Account, state: RegistrationState, message: String)) in
				// If account has been configured correctly, we will go through Progress and Ok states
				// Otherwise, we will be Failed.
				NSLog("New registration state is \(cbVal.state) for user id " +
					  "\( String(describing: cbVal.account.params?.identityAddress?.asString())) = \(cbVal.message)\n")
				if cbVal.state == .Ok {
					self.loggingInProgress = false
					self.loggedIn = true
				} else if cbVal.state == .Progress {
					self.loggingInProgress = true
				} else {
					self.toastMessage = "Registration failed"
					self.loggingInProgress = false
					self.loggedIn = false
				}
			}.postOnCoreQueue { (cbVal: (core: Core, account: Account, state: RegistrationState, message: String)) in
				// If registration failed, remove account from core
				if cbVal.state != .Ok && cbVal.state != .Progress {
					let params = cbVal.account.params
					let clonedParams = params?.clone()
					clonedParams?.registerEnabled = false
					cbVal.account.params = clonedParams
					
					cbVal.core.removeAccount(account: cbVal.account)
					cbVal.core.clearAccounts()
					cbVal.core.clearAllAuthInfo()
				}
			}
			
			self.mIteratePublisher = Timer.publish(every: 0.02, on: .main, in: .common)
				.autoconnect()
				.receive(on: coreQueue)
				.sink { _ in
					self.mCore.iterate()
				}
			
		}
	}
}

// swiftlint:enable large_tuple
