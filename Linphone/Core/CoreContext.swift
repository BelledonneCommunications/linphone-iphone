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
// swiftlint:disable line_length

import linphonesw
import Combine
import UniformTypeIdentifiers

final class CoreContext: ObservableObject {
	
	static let shared = CoreContext()
	private var sharedMainViewModel = SharedMainViewModel.shared
	
	var coreVersion: String = Core.getVersion
	@Published var loggedIn: Bool = false
	@Published var loggingInProgress: Bool = false
	@Published var defaultAccount: Account?
	@Published var coreIsStarted: Bool = false
	
	private var mCore: Core!
	private var mIterateSuscription: AnyCancellable?
	private var mCoreSuscriptions = Set<AnyCancellable?>()
	
	private init() {
		do {
			try initialiseCore()
		} catch {
			
		}
	}
	
	func doOnCoreQueue(synchronous: Bool = false, lambda: @escaping (Core) -> Void) {
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
			
			Factory.Instance.logCollectionPath = configDir
			Factory.Instance.enableLogCollection(state: LogCollectionState.Enabled)
			
			let url = NSURL(fileURLWithPath: configDir)
			if let pathComponent = url.appendingPathComponent("linphonerc") {
				let filePath = pathComponent.path
				let fileManager = FileManager.default
				if !fileManager.fileExists(atPath: filePath) {
					let path = Bundle.main.path(forResource: "linphonerc-default", ofType: nil)
					if path != nil {
						try? FileManager.default.copyItem(at: NSURL(fileURLWithPath: path!) as URL, to: pathComponent)
					}
				}
			}
			
			let config = try? Factory.Instance.createConfigWithFactory(
				path: "\(configDir)/linphonerc",
				factoryPath: Bundle.main.path(forResource: "linphonerc-factory", ofType: nil)
			)
			if config != nil {
				self.mCore = try? Factory.Instance.createCoreWithConfig(config: config!, systemContext: nil)
			}
			
			self.mCore.autoIterateEnabled = false
			self.mCore.callkitEnabled = true
			self.mCore.pushNotificationEnabled = true
			
			self.mCore.setUserAgent(name: "Linphone iOS 6.0 Beta (\(UIDevice.current.localizedModel)) - Linphone SDK : \(self.coreVersion)", version: "6.0")
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onGlobalStateChanged?.postOnMainQueue { (cbVal: (core: Core, state: GlobalState, message: String)) in
				if cbVal.state == GlobalState.On {
					self.defaultAccount = self.mCore.defaultAccount
					self.coreIsStarted = true
				} else if cbVal.state == GlobalState.Off {
					self.defaultAccount = nil
					self.coreIsStarted = true
				}
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onGlobalStateChanged?.postOnCoreQueue { (cbVal: (core: Core, state: GlobalState, message: String)) in
				if cbVal.state == GlobalState.On {
#if DEBUG
					let pushEnvironment = ".dev"
#else
					let pushEnvironment = ""
#endif
					for account in cbVal.core.accountList where account.params?.pushNotificationConfig?.provider != ("apns" + pushEnvironment) {
						let newParams = account.params?.clone()
						Log.info("Account \(String(describing: newParams?.identityAddress?.asStringUriOnly())) - updating apple push provider from \(String(describing: newParams?.pushNotificationConfig?.provider)) to apns\(pushEnvironment)")
						newParams?.pushNotificationConfig?.provider = "apns" + pushEnvironment
						account.params = newParams
					}
				}
			})
			
			self.mCore.videoCaptureEnabled = true
			self.mCore.videoDisplayEnabled = true
			
			
			// Create a Core listener to listen for the callback we need
			// In this case, we want to know about the account registration status
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onConfiguringStatus?.postOnMainQueue { (cbVal: (core: Core, status: Config.ConfiguringState, message: String)) in
				NSLog("New configuration state is \(cbVal.status) = \(cbVal.message)\n")
				if cbVal.status == Config.ConfiguringState.Successful {
					ToastViewModel.shared.toastMessage = "Successful"
					ToastViewModel.shared.displayToast = true
				} 
				/*
				 else {
				 ToastViewModel.shared.toastMessage = "Failed"
				 ToastViewModel.shared.displayToast = true
				 }
				 */
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onAccountRegistrationStateChanged?.postOnMainQueue { (cbVal: (core: Core, account: Account, state: RegistrationState, message: String)) in
				// If account has been configured correctly, we will go through Progress and Ok states
				// Otherwise, we will be Failed.
				NSLog("New registration state is \(cbVal.state) for user id " +
					  "\( String(describing: cbVal.account.params?.identityAddress?.asString())) = \(cbVal.message)\n")
				if cbVal.state == .Ok {
					self.loggingInProgress = false
					self.loggedIn = true
					if self.mCore.consolidatedPresence != ConsolidatedPresence.Online {
						self.onForeground()
					}
				} else if cbVal.state == .Progress {
					self.loggingInProgress = true
				} else {
					ToastViewModel.shared.toastMessage = "Registration failed"
					ToastViewModel.shared.displayToast = true
					self.loggingInProgress = false
					self.loggedIn = false
				}
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onAccountRegistrationStateChanged?.postOnCoreQueue { (cbVal: (core: Core, account: Account, state: RegistrationState, message: String)) in
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
				TelecomManager.shared.onAccountRegistrationStateChanged(core: cbVal.core, account: cbVal.account, state: cbVal.state, message: cbVal.message)
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onCallStateChanged?.postOnCoreQueue { (cbVal: (core: Core, call: Call, state: Call.State, message: String)) in
				TelecomManager.shared.onCallStateChanged(core: cbVal.core, call: cbVal.call, state: cbVal.state, message: cbVal.message)
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onLogCollectionUploadStateChanged?.postOnMainQueue { (cbValue: (_: Core, _: Core.LogCollectionUploadState, info: String)) in
				if cbValue.info.starts(with: "https") {
					UIPasteboard.general.setValue(
						cbValue.info,
						forPasteboardType: UTType.plainText.identifier
					)
					
					DispatchQueue.main.async {
						ToastViewModel.shared.toastMessage = "Success_send_logs"
					 	ToastViewModel.shared.displayToast = true
					}
				}
			})
			
			self.mIterateSuscription = Timer.publish(every: 0.02, on: .main, in: .common)
				.autoconnect()
				.receive(on: coreQueue)
				.sink { _ in
					self.mCore.iterate()
				}
			
			try? self.mCore.start()
		}
	}
	
	func onForeground() {
		coreQueue.async {
			// We can't rely on defaultAccount?.params?.isPublishEnabled
			// as it will be modified by the SDK when changing the presence status
			if self.mCore.config!.getBool(section: "app", key: "publish_presence", defaultValue: true) {
				NSLog("App is in foreground, PUBLISHING presence as Online")
				self.mCore.consolidatedPresence = ConsolidatedPresence.Online
			}
		}
	}
	
	func onBackground() {
		coreQueue.async {
			// We can't rely on defaultAccount?.params?.isPublishEnabled
			// as it will be modified by the SDK when changing the presence status
			if self.mCore.config!.getBool(section: "app", key: "publish_presence", defaultValue: true) {
				NSLog("App is in background, un-PUBLISHING presence info")
				// We don't use ConsolidatedPresence.Busy but Offline to do an unsubscribe,
				// Flexisip will handle the Busy status depending on other devices
				self.mCore.consolidatedPresence = ConsolidatedPresence.Offline
			}
		}
	}
}

// swiftlint:enable large_tuple
// swiftlint:enable line_length
