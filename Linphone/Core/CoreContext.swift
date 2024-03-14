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
import linphone // needed for unwrapped function linphone_core_set_push_registry_dispatch_queue
import Combine
import UniformTypeIdentifiers
import Network

#if USE_CRASHLYTICS
import Firebase
#endif

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
	
	let monitor = NWPathMonitor()
	
	private var mCorePushIncomingDelegate: CoreDelegate!
	
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
#if USE_CRASHLYTICS
		FirebaseApp.configure()
#endif
		coreQueue.async {
			LoggingService.Instance.logLevel = LogLevel.Debug
			Factory.Instance.logCollectionPath = Factory.Instance.getConfigDir(context: nil)
			Factory.Instance.enableLogCollection(state: LogCollectionState.Enabled)
			
			Log.info("Checking if linphonerc file exists already. If not, creating one as a copy of linphonerc-default")
			if let rcDir = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: Config.appGroupName)?
				.appendingPathComponent("Library/Preferences/linphone") {
				let rcFileUrl = rcDir.appendingPathComponent("linphonerc")
				if !FileManager.default.fileExists(atPath: rcFileUrl.path) {
					do {
						try FileManager.default.createDirectory(at: rcDir, withIntermediateDirectories: true)
						if let pathToDefaultConfig = Bundle.main.path(forResource: "linphonerc-default", ofType: nil) {
							try FileManager.default.copyItem(at: URL(fileURLWithPath: pathToDefaultConfig), to: rcFileUrl)
							Log.info("Successfully copied linphonerc-default configuration")
						}
					} catch let error {
						Log.error("Failed to copy default linphonerc file: \(error.localizedDescription)")
					}
				} else {
					Log.info("Found existing linphonerc file, skip copying of linphonerc-default configuration")
				}
			}
			
			Log.info("Initialising core")
			self.mCore = try? Factory.Instance.createSharedCoreWithConfig(config: Config.get(), systemContext: nil, appGroupId: Config.appGroupName, mainCore: true)
			
			linphone_core_set_push_registry_dispatch_queue(self.mCore.getCobject, Unmanaged.passUnretained(coreQueue).toOpaque())
			self.mCore.autoIterateEnabled = false
			self.mCore.callkitEnabled = true
			self.mCore.pushNotificationEnabled = true

			self.mCore.setUserAgent(name: "Linphone iOS 6.0 Beta (\(UIDevice.current.localizedModel)) - Linphone SDK : \(self.coreVersion)", version: "6.0")
			
			self.mCore.videoCaptureEnabled = true
			self.mCore.videoDisplayEnabled = true
			
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
					
					// Remove specs for 6.0 first version
					Log.info("Removing spec 'conference' from core for this version")
					self.mCore.removeLinphoneSpec(spec: "conference")
					Log.info("Removing spec 'ephemeral' from core for this version")
					self.mCore.removeLinphoneSpec(spec: "ephemeral")
					/*
					Log.info("Removing spec 'groupchat' from core for this version")
					self.mCore.removeLinphoneSpec(spec: "groupchat")
					Log.info("Removing spec 'lime' from core for this version")
					self.mCore.removeLinphoneSpec(spec: "lime")
					 */
				}
			})
			
			// Create a Core listener to listen for the callback we need
			// In this case, we want to know about the account registration status
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onConfiguringStatus?.postOnMainQueue { (cbVal: (core: Core, status: ConfiguringState, message: String)) in
				Log.info("New configuration state is \(cbVal.status) = \(cbVal.message)\n")
				if cbVal.status == ConfiguringState.Successful {
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
				Log.info("New registration state is \(cbVal.state) for user id " +
						 "\( String(describing: cbVal.account.params?.identityAddress?.asString())) = \(cbVal.message)\n")
				if cbVal.state == .Ok {
					self.loggingInProgress = false
					self.loggedIn = true
				} else if cbVal.state == .Progress {
					self.loggingInProgress = true
				} else {
					self.loggingInProgress = false
					self.loggedIn = false
					ToastViewModel.shared.toastMessage = "Registration failed"
					ToastViewModel.shared.displayToast = true
					
					self.monitor.pathUpdateHandler = { path in
						if path.status == .satisfied {
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
					}
				}
			})
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onAccountRegistrationStateChanged?.postOnCoreQueue { (cbVal: (core: Core, account: Account, state: RegistrationState, message: String)) in
				if cbVal.state == .Ok {
					if self.mCore.consolidatedPresence !=  ConsolidatedPresence.Online {
						self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Online)
					}
				} else if cbVal.state != .Ok && cbVal.state != .Progress { // If registration failed, remove account from core
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
			
			self.mCorePushIncomingDelegate = CoreDelegateStub(onCallStateChanged: { (core: Core, call: Call, cstate: Call.State, message: String) in
				if cstate == .PushIncomingReceived {
					let callLog = call.callLog
					let callId = callLog?.callId ?? ""
					Log.info("PushIncomingReceived in core delegate, display callkit call")
					TelecomManager.shared.displayIncomingCall(call: call, handle: "Calling", hasVideo: false, callId: callId, displayName: "Calling")
				}
			})
			self.mCore.addDelegate(delegate: self.mCorePushIncomingDelegate)
			
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
			
			self.mCoreSuscriptions.insert(self.mCore.publisher?.onTransferStateChanged?.postOnMainQueue { (cbValue: (_: Core, transfered: Call, callState: Call.State)) in
				Log.info(
					"[CoreContext] Transferred call \(cbValue.transfered.remoteAddress!.asStringUriOnly()) state changed \(cbValue.callState)"
				)
				if cbValue.callState == Call.State.Connected {
					ToastViewModel.shared.toastMessage = "Success_toast_call_transfer_successful"
					ToastViewModel.shared.displayToast = true
				} else if cbValue.callState == Call.State.OutgoingProgress {
					ToastViewModel.shared.toastMessage = "Success_toast_call_transfer_in_progress"
					ToastViewModel.shared.displayToast = true
				} else if cbValue.callState == Call.State.End || cbValue.callState == Call.State.Error {
					ToastViewModel.shared.toastMessage = "Failed_toast_call_transfer_failed"
					ToastViewModel.shared.displayToast = true
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
	
	func updatePresence(core : Core, presence : ConsolidatedPresence) {
		if core.config!.getBool(section: "app", key: "publish_presence", defaultValue: true) {
			core.consolidatedPresence = presence
		}
	}
	
	func onEnterForeground() {
		coreQueue.async {
			// We can't rely on defaultAccount?.params?.isPublishEnabled
			// as it will be modified by the SDK when changing the presence status
		
			Log.info("App is in foreground, PUBLISHING presence as Online")
			self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Online)
			try? self.mCore.start()
		}
	}
	
	func onEnterBackground() {
		coreQueue.async {
			// We can't rely on defaultAccount?.params?.isPublishEnabled
			// as it will be modified by the SDK when changing the presence status
			Log.info("App is in background, un-PUBLISHING presence info")
			
			// We don't use ConsolidatedPresence.Busy but Offline to do an unsubscribe,
			// Flexisip will handle the Busy status depending on other devices
			self.updatePresence(core: self.mCore, presence: ConsolidatedPresence.Offline)
			// self.mCore.iterate()
			self.mCore.stop()
		}
	}
	
	func crashForCrashlytics() {
		fatalError("Crashing app to test crashlytics")
	}
}

// swiftlint:enable large_tuple
// swiftlint:enable line_length
